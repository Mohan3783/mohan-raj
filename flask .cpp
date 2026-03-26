#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* server = "http://YOUR_PC_IP:5000/api/data";

const int SENSOR_PIN = 34;
const int RELAY_PIN = 26;
const int SEND_INTERVAL = 5000;
const int BUFFER_SIZE = 100;

unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Default OFF
  
  connectToWiFi();
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
  } else {
    Serial.println("\nFailed to connect!");
  }
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
    return;
  }

  // Only send data at intervals
  if (millis() - lastSend < SEND_INTERVAL) {
    return;
  }
  lastSend = millis();

  // Read sensor
  int sensorValue = analogRead(SENSOR_PIN);
  float current = sensorValue * (3.3 / 4095.0);
  float voltage = 220.0;

  // Create JSON (more efficient)
  char json[BUFFER_SIZE];
  snprintf(json, BUFFER_SIZE, "{\"current\":%.2f,\"voltage\":%.2f}", current, voltage);

  // Send HTTP POST
  HTTPClient http;
  http.setTimeout(5000);  // Add timeout
  http.begin(server);
  http.addHeader("Content-Type", "application/json");

  int responseCode = http.POST(json);

  if (responseCode == 200) {
    String response = http.getString();
    
    if (response.indexOf("OFF") >= 0) {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Relay: OFF");
    } else if (response.indexOf("ON") >= 0) {
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Relay: ON");
    }
  } else {
    Serial.printf("HTTP Error: %d\n", responseCode);
  }

  http.end();
}
