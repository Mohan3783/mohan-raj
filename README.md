# MOHAN RAJ M
# Project Title: Smart Helmet for Drunk & Drive Detection

## Overview
This project is an IoT-enabled smart safety helmet designed to prevent drunk driving and provide real-time accident detection and alerting. The system uses an MQ-3 alcohol sensor, IMU sensor, GSM/GPS modules, and Raspberry Pi to monitor the rider's condition and send alerts if necessary.
 Features
-  Alcohol detection using MQ-3 sensor  
-  Automatic ignition lock if alcohol level exceeds threshold  
-  Live GPS tracking  
-  SMS alert via GSM to emergency contacts  
-  Accident detection using MPU6050 sensor  
-  Web dashboard for live monitoring (Flask/Firebase/MQTT)  
-  Local data logging for analysis  
## Tech Stack
| Category | Technologies Used |

| Microcontroller / SBC | Raspberry Pi / ESP32 |
| Programming | Python, Embedded C |
| Sensors | MQ-3, MPU6050, GPS Module |
| Connectivity | GSM, MQTT, Bluetooth |
| Cloud / Dashboard | Firebase, Flask App |
| Tools | KiCad/EasyEDA, Git |
## 📡 System Architecture
 Helmet Sensors → Raspberry Pi → Vehicle Ignition Control  →  GSM + GPS → Emergency Alerts → Cloud Dashboard → Live Monitoring
## Hardware Components

| Component | Quantity | Purpose |
|-----------|----------|---------|
| Raspberry Pi | 1 | Processing Unit |
| MQ-3 Alcohol Sensor | 1 | Detect Alcohol Level |
| MPU6050 | 1 | Accident / Tilt Detection |
| GSM Module (SIM800L) | 1 | SMS Alerts |
| GPS Module | 1 | Live Tracking |
| Relay Module | 1 | Ignition Lock |
##  Installation & Setup

### 1️⃣ Install Dependencies
a single main.py file that does:

MQ-3 alcohol reading

MPU6050 accident detection

Relay (ignition lock) control

GSM SMS alert sending

Basic event logging

You can split it later into modules if you want.

🧾 Assumptions (very important)

This code assumes:

Board: Raspberry Pi

MQ-3 sensor: Connected via MCP3008 ADC (channel 0)

MPU6050: Connected via I2C at address 0x68

GSM (SIM800L): Connected via UART (/dev/ttyS0 or /dev/ttyAMA0)

Relay: Connected to GPIO 17 (BCM numbering)

You already enabled: I2C + Serial via raspi-config

You can adjust pin numbers easily in the constants.

🧠 Install required libraries
sudo apt update
sudo apt install python3-pip python3-smbus i2c-tools -y
pip3 install smbus2 pyserial


Also you need spidev:

sudo apt install python3-spidev -y

🧩 main.py – Smart Helmet Core Code
#!/usr/bin/env python3
"""
Smart Helmet for Drunk & Drive Detection
----------------------------------------
Features:
- Read MQ-3 alcohol sensor via MCP3008
- Read MPU6050 (accelerometer) via I2C
- Detect alcohol > threshold -> lock ignition
- Detect crash (high acceleration) -> send SMS
- Send SMS using SIM800L GSM module
- Log events to local file

NOTE: Adjust thresholds, phone number, and port names as per your setup.
"""

import time
import math
import serial
import spidev
import smbus2
import RPi.GPIO as GPIO
from datetime import datetime

# ==========================
# CONFIGURATION
# ==========================

# ADC (MCP3008) Config for MQ-3
SPI_BUS = 0
SPI_DEVICE = 0
MQ3_CHANNEL = 0  # ADC Channel 0

# MPU6050 Config
I2C_BUS_ID = 1
MPU6050_ADDR = 0x68
ACCEL_SENSITIVITY = 16384.0  # LSB/g for +/-2g

# GPIO Pins (BCM)
RELAY_PIN = 17          # Relay to control ignition
STATUS_LED_PIN = 27     # Optional LED for status

# Alcohol & Crash thresholds (tune these)
ALCOHOL_THRESHOLD = 500      # Raw ADC value (0–1023); adjust via calibration
CRASH_ACCEL_THRESHOLD_G = 3  # 3g – adjust after testing
CRASH_DEBOUNCE_SECS = 1.0

# GSM Config
GSM_PORT = "/dev/ttyS0"   # or "/dev/ttyAMA0" depending on Pi model
GSM_BAUDRATE = 9600
EMERGENCY_PHONE = "+91XXXXXXXXXX"  # <-- Put your phone number here

# Logging
LOG_FILE = "helmet_events.log"


# ==========================
# UTILITY FUNCTIONS
# ==========================

def log_event(message: str):
    """Append events with timestamps to a log file and print to console."""
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] {message}"
    print(line)
    try:
        with open(LOG_FILE, "a") as f:
            f.write(line + "\n")
    except Exception as e:
        print("Log write error:", e)


# ==========================
# HARDWARE INITIALIZATION
# ==========================

def init_gpio():
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)
    GPIO.setup(RELAY_PIN, GPIO.OUT)
    GPIO.setup(STATUS_LED_PIN, GPIO.OUT)

    # Default: ignition allowed, LED off
    GPIO.output(RELAY_PIN, GPIO.LOW)      # LOW = ignition ON (depends on relay wiring)
    GPIO.output(STATUS_LED_PIN, GPIO.LOW)


def init_spi():
    spi = spidev.SpiDev()
    spi.open(SPI_BUS, SPI_DEVICE)
    spi.max_speed_hz = 1350000
    return spi


def init_mpu6050():
    bus = smbus2.SMBus(I2C_BUS_ID)
    # Wake MPU6050 up (exit sleep mode)
    bus.write_byte_data(MPU6050_ADDR, 0x6B, 0x00)
    time.sleep(0.1)
    return bus


def init_gsm():
    try:
        ser = serial.Serial(GSM_PORT, GSM_BAUDRATE, timeout=1)
        time.sleep(2)  # Allow GSM module to boot
        # Basic AT check
        ser.write(b'AT\r')
        time.sleep(0.5)
        response = ser.read(ser.in_waiting or 64)
        log_event(f"GSM Init response: {response}")
        return ser
    except Exception as e:
        log_event(f"Error initializing GSM: {e}")
        return None


# ==========================
# SENSOR READ FUNCTIONS
# ==========================

def read_adc(spi, channel: int) -> int:
    """
    MCP3008 ADC read.
    channel: 0-7
    Returns: value 0–1023 (10-bit)
    """
    if not 0 <= channel <= 7:
        raise ValueError("ADC channel must be 0-7")

    # MCP3008 protocol
    adc = spi.xfer2([1, (8 + channel) << 4, 0])
    data = ((adc[1] & 3) << 8) + adc[2]
    return data


def read_mq3(spi) -> int:
    """Read raw MQ-3 ADC value via MCP3008."""
    value = read_adc(spi, MQ3_CHANNEL)
    return value


def read_mpu6050_accel(bus):
    """Read accelerometer (x, y, z) in g from MPU6050."""
    def read_word(reg):
        high = bus.read_byte_data(MPU6050_ADDR, reg)
        low = bus.read_byte_data(MPU6050_ADDR, reg + 1)
        value = (high << 8) + low
        if value >= 0x8000:
            value = -((65535 - value) + 1)
        return value

    accel_x = read_word(0x3B) / ACCEL_SENSITIVITY
    accel_y = read_word(0x3D) / ACCEL_SENSITIVITY
    accel_z = read_word(0x3F) / ACCEL_SENSITIVITY
    return accel_x, accel_y, accel_z


def calc_accel_magnitude_g(ax, ay, az):
    """Calculate magnitude of acceleration vector in g."""
    return math.sqrt(ax**2 + ay**2 + az**2)


# ==========================
# GSM / SMS FUNCTIONS
# ==========================

def gsm_send_sms(ser, phone: str, text: str):
    """Send SMS using SIM800L via AT commands."""
    if ser is None:
        log_event("GSM not initialized, cannot send SMS.")
        return

    try:
        log_event(f"Sending SMS to {phone}: {text}")
        ser.write(b'AT+CMGF=1\r')  # SMS text mode
        time.sleep(0.5)
        cmd = f'AT+CMGS="{phone}"\r'.encode()
        ser.write(cmd)
        time.sleep(0.5)
        ser.write(text.encode() + b"\r")
        time.sleep(0.5)
        ser.write(bytes([26]))  # Ctrl+Z to send
        time.sleep(3)
        resp = ser.read(ser.in_waiting or 64)
        log_event(f"GSM SMS response: {resp}")
    except Exception as e:
        log_event(f"Error sending SMS: {e}")


# ==========================
# CONTROL LOGIC
# ==========================

def lock_ignition():
    """Turn off ignition using relay and turn LED ON."""
    GPIO.output(RELAY_PIN, GPIO.HIGH)  # depends on relay wiring
    GPIO.output(STATUS_LED_PIN, GPIO.HIGH)
    log_event("IGNITION LOCKED (relay ON).")


def unlock_ignition():
    """Allow ignition and turn LED OFF."""
    GPIO.output(RELAY_PIN, GPIO.LOW)
    GPIO.output(STATUS_LED_PIN, GPIO.LOW)
    log_event("Ignition unlocked (relay OFF).")


# ==========================
# MAIN LOOP
# ==========================

def main():
    log_event("=== Smart Helmet System Starting ===")

    init_gpio()
    spi = init_spi()
    mpu_bus = init_mpu6050()
    gsm_ser = init_gsm()

    last_crash_time = 0
    ignition_locked_due_to_alcohol = False

    try:
        while True:
            # 1. Read sensors
            mq3_value = read_mq3(spi)
            ax, ay, az = read_mpu6050_accel(mpu_bus)
            accel_mag = calc_accel_magnitude_g(ax, ay, az)

            # 2. Alcohol logic
            if mq3_value > ALCOHOL_THRESHOLD:
                if not ignition_locked_due_to_alcohol:
                    log_event(f"Alcohol detected! MQ-3 = {mq3_value}")
                    lock_ignition()
                    ignition_locked_due_to_alcohol = True
                    # Send SMS notification
                    gsm_send_sms(
                        gsm_ser,
                        EMERGENCY_PHONE,
                        f"[SMART HELMET] Alcohol detected. MQ-3 value: {mq3_value}"
                    )
            else:
                # If no alcohol, you may choose to unlock ignition automatically
                if ignition_locked_due_to_alcohol:
                    log_event(f"Alcohol level back to normal. MQ-3 = {mq3_value}")
                    unlock_ignition()
                    ignition_locked_due_to_alcohol = False

            # 3. Crash detection
            current_time = time.time()
            if accel_mag > CRASH_ACCEL_THRESHOLD_G:
                if current_time - last_crash_time > CRASH_DEBOUNCE_SECS:
                    last_crash_time = current_time
                    log_event(f"CRASH DETECTED! Accel magnitude = {accel_mag:.2f} g")

                    # Send SMS alert
                    gsm_send_sms(
                        gsm_ser,
                        EMERGENCY_PHONE,
                        f"[SMART HELMET] Crash detected! Accel = {accel_mag:.2f} g"
                    )

            # 4. Print debug info (optional)
            print(
                f"MQ-3={mq3_value}  "
                f"Accel(g): x={ax:.2f}, y={ay:.2f}, z={az:.2f}, |a|={accel_mag:.2f}"
            )

            time.sleep(0.3)  # Adjust refresh rate

    except KeyboardInterrupt:
        log_event("KeyboardInterrupt – shutting down...")
    except Exception as e:
        log_event(f"ERROR in main loop: {e}")
    finally:
        GPIO.cleanup()
        if 'spi' in locals():
            spi.close()
        if 'gsm_ser' in locals() and gsm_ser is not None:
            gsm_ser.close()
        log_event("=== Smart Helmet System Stopped ===")


if __name__ == "__main__":
    main()
What you should customize

EMERGENCY_PHONE = "+91XXXXXXXXXX"

ALCOHOL_THRESHOLD – Calibrate using your MQ-3 values.

CRASH_ACCEL_THRESHOLD_G – Tune based on real riding tests.

GSM_PORT – Check whether your Pi uses /dev/ttyS0 or /dev/ttyAMA0.
