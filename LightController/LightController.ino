#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

int lightPin = D2;
int motionSensorPin = D1;

bool lightOnBool = false;
int milliSecondsToKeepLightOn = 5000;
unsigned long timeStamp = 0;
int lightValueLimit = 600;

const char* newHostname = "LightController";

const char* ssid = "NPJYOGA9I";
const char* password =  "aaaabbbb";

const char* hostname = "http://192.168.137.174/updateLight?state=";
String url = hostname;

void sendState(int value)
{
  int code = -1;
  while (code != 200)
  {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, url + String(value));
    Serial.println(code);
    Serial.println(url + String(value));
    code = http.GET();
    http.end();
    delay(1000);
  }
}

void lightOn()
{
  WiFiClient client;
  HTTPClient http;
  digitalWrite(lightPin, HIGH);
  timeStamp = millis();
  if (!lightOnBool)
  {
    http.begin(client, url + "1");
    http.GET();
    Serial.println(url + "1");
    http.end();
  }
  lightOnBool = true;
}

void lightOff()
{
  WiFiClient client;
  HTTPClient http;
  digitalWrite(lightPin, LOW);
  if (lightOnBool)
  {
    http.begin(client, url + "0");
    http.GET();
    Serial.println(url + "0");
    http.end();
  }
  lightOnBool = false;
}

void setup() 
{
  WiFi.hostname(newHostname);
  Serial.begin(9600);
  pinMode(lightPin, OUTPUT);
  pinMode(motionSensorPin, INPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() 
{
  int LightValue = analogRead(A0);
  if (LightValue > lightValueLimit && !lightOnBool)
  {
    return;
  }

  if (digitalRead(motionSensorPin) == 1)
  {
    lightOn();
  }
  else if (millis() - timeStamp > milliSecondsToKeepLightOn)
  {
    lightOff();
  }

}
