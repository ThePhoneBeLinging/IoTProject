#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "";
const char* password = "";
const char* url = "http://bathroommaster.local/windowState"

const int fan = 4;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  pinMode(fan, OUTPUT);
  
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  Serial.println("\nConnected to WiFi!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (getShouldTurnOn()) {
      digitalWrite(fan, HIGH);
    } else {
      digitalWrite(fan, LOW);
    }
  } else {
    Serial.println("WiFi Disconnected");
  }

  delay(10000);
}

bool getShouldTurnOn() {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);
  
  int httpCode = http.GET(); 
  if (httpCode == 200) { 
    String payload = http.getString();
    payload.trim(); 
    http.end();
    return payload == "true";
  } else {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}

