#include <WiFi.h>
#include <HTTPClient.h>

#define ACS_PIN 34          // Analog pin connected to ACS712 OUT
#define RELAY_PIN 25        // IN1 of relay module

float sensitivity = 0.066;  // For ACS712-30A
float offset = 2.5;         // Will be calibrated
float threshold = 0.8;      // Overload threshold in Amps

// --- WiFi & ThingSpeak setup ---
const char* ssid = "xxxx";
const char* password = "xxxx";
String writeAPIKey = "UYCQDIGQZ5BA0T4K";   // Replace with your ThingSpeak Write API Key
const char* server = "https://api.thingspeak.com/update";

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Fan ON initially

  Serial.println("Smart Load Balancer + IoT Starting...");
  
  // --- WiFi connection ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());

  // --- Calibrate Zero Offset ---
  Serial.println("Calibrating sensor...");
  long sum = 0;
  for (int i = 0; i < 1000; i++) {
    sum += analogRead(ACS_PIN);
    delay(2);
  }
  offset = (sum / 1000.0) * (3.3 / 4095.0);
  Serial.print("Calibrated Offset: ");
  Serial.println(offset, 3);
  Serial.println("-----------------------------------");
}

void loop() {
  int sensorValue = analogRead(ACS_PIN);
  float voltage = (sensorValue * 3.3) / 4095.0;
  float current = fabs((voltage - offset) / sensitivity);
  int relayStatus;

  if (current > threshold) {
    Serial.println("⚠️ Overload detected! Turning fan OFF...");
    digitalWrite(RELAY_PIN, HIGH);  // Relay OFF
    relayStatus = 0;
  } else {
    Serial.println("✅ Normal Load. Fan remains ON.");
    digitalWrite(RELAY_PIN, LOW);   // Relay ON
    relayStatus = 1;
  }

  Serial.print("Current: ");
  Serial.print(current, 3);
  Serial.print(" A | Relay: ");
  Serial.println(relayStatus);
  Serial.println("-----------------------------------");

  // --- Send Data to ThingSpeak ---
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = server;
    url += "?api_key=" + writeAPIKey;
    url += "&field1=" + String(current, 3);
    url += "&field2=" + String(relayStatus);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("Data sent to ThingSpeak.");
    } else {
      Serial.println("Failed to send data.");
    }
    http.end();
  }

  delay(15000);  // ThingSpeak allows one update every 15 seconds
}

