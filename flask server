#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

const char* server = "http://YOUR_PC_IP:5000/api/data";

int sensorPin = 34;
int relayPin = 26;

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println("Connected!");
}

void loop() {
  int sensorValue = analogRead(sensorPin);
  float current = sensorValue * (3.3 / 4095.0); // convert

  float voltage = 220;

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(server);
    http.addHeader("Content-Type", "application/json");

    String json = "{\"current\":" + String(current) + ",\"voltage\":" + String(voltage) + "}";

    int responseCode = http.POST(json);

    if (responseCode > 0) {
      String response = http.getString();
      Serial.println(response);

      if (response.indexOf("OFF") > 0) {
        digitalWrite(relayPin, LOW); // Cut power
      } else {
        digitalWrite(relayPin, HIGH); // Allow power
      }
    }

    http.end();
  }

  delay(5000);
}
