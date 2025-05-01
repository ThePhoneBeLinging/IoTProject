#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

int lightPin = D0;
int motionSensorPin = D1;

bool lightOnBool = false;
int milliSecondsToKeepLightOn = 5000;
unsigned long timeStamp = 0;
int lightValueLimit = 75;

const char* ssid = "NPJYOGA9I";
const char* password =  "aaaabbbb";

const char* hostname = "http://bathroommaster.local/updateLight?state=";
String url = hostname;

void lightOn()
{
  lightOnBool = true;
  lightPin = true;
  timeStamp = millis();
  digitalWrite(lightPin, HIGH);
  
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url + "0");
  http.GET();
  Serial.println(url + "0");
  http.end();
}

void lightOff()
{
  lightOnBool = false;
  digitalWrite(lightPin, LOW);

  WiFiClient client;
  HTTPClient http;
  http.begin(client, url + "0");
  http.GET();
  Serial.println(url + "0");
  http.end();
}

void setup() 
{
  Serial.begin(9600);
  pinMode(lightPin,  OUTPUT);
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
  Serial.println(LightValue);
  if (LightValue > lightValueLimit)
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
