#include <Wire.h>
#include <MPU6050.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define DATA_ENDPOINT "http://192.168.137.107/submitWater?"
String newHostname = "showerguy";
ESP8266WiFiMulti wifiMulti;

MPU6050 mpu;

float wmin = 170.0f;
float threshhold = 2.0f;
float wmax = 160.0f;
float lpm_max = 15.0f; // Liters per minute at max shower pressure 
int sumbit_delay = 30000;

void setup() {
  Serial.begin(115200);

  // Begin all IO
  Wire.begin();
  mpu.initialize();

  // Wait for MPU6050 to wake up
  while (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed! Retrying in 2 seconds");
    delay(2000);
  }
  Serial.println("MPU6050 ready.");

  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("NPJYOGA9I", "aaaabbbb");
  WiFi.hostname(newHostname.c_str());

  Serial.println("Connecting ...");
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250);
    Serial.print('.');
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("Local IP Address: ");
  Serial.println(WiFi.localIP());
  
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Convert raw accelerometer data to roll (X) and pitch (Y) - Thanks to chatgpt conversions
  float roll  = atan2(ay, az) * 180.0 / PI;
  float pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  // Map to 0–360 degrees
  if (roll < 0) roll += 360;
  if (pitch < 0) pitch += 360;

  Serial.print("X-Axis Rotation (Roll): ");
  Serial.println(roll);
  //Serial.print("  |  Y-Axis Rotation (Pitch): ");
  //Serial.println(pitch);

  float wateramt = convertDegreetoWaterAmt(roll);

  sendDataToWebserver(wateramt);

  delay(sumbit_delay);
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) { // <- Credit to chatgpt
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
float convertDegreetoWaterAmt(float roll) {
  float roll_fixed = roll >= wmin ? roll-wmin : roll + (360-wmin);
  if (roll_fixed <= threshhold) { return 0.0f; } // If water usage under threshhold, return zero
  return mapFloat(roll_fixed, 0.0f, 360-wmin+wmax, 0.0f, lpm_max);
}

void sendDataToWebserver(float water_amt) {
  // check connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to WiFi");
    return;
  }
  WiFiClient client;
  HTTPClient http;

  char url[128];
  sprintf(url, "%swater_amt=%f", DATA_ENDPOINT, water_amt);
  http.begin(client, url);
  int httpCode = http.GET();

  Serial.println(url);
  if (httpCode > 0) {
      Serial.printf("HTTP GET code: %d\n", httpCode);
      String payload = http.getString();
      //Serial.println("Response:");
      //Serial.println(payload);
  } else {
    Serial.printf("GET request failed. Error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();  // Free resources
  delay(250);
  
}
