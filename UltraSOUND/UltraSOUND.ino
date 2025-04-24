/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/Blink
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Bing Chilling";
const char* password =  "deanospharos";

const char* hostname = "http://DeanPhar/updateToilet?state=";
String url = hostname;
HTTPClient http;
WiFiClientSecure client;

#define SOUND_VELOCITY 0.034

bool personOnToilet = false;
int pingsSincePersonDetected = 0;
int pingsWithPerson = 0;
int pingsBeforeStateSwitch = 5;
int distanceSplitValue = 200;

const int trigPin = D0;
const int echoPin = D1;

void emitSoundWaves()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
}

int getDistance()
{
  long duration = pulseIn(echoPin,HIGH);
  return duration * SOUND_VELOCITY / 2;
}


void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin,INPUT);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  client.setInsecure();
}

void toiletEmpty()
{
  personOnToilet = false;
  http.begin(client, url + "0");
  http.end();
}

void toiletOccupied()
{
  personOnToilet = true;
  http.begin(client, url + "1");
  http.end();
}

void loop() {
  emitSoundWaves();
  float distance = getDistance();
  Serial.println(personOnToilet);
  if (personOnToilet && distance > distanceSplitValue)
  {
    pingsSincePersonDetected++;
    pingsWithPerson = 0;
    if (pingsSincePersonDetected >= pingsBeforeStateSwitch)
    {
      toiletEmpty();
    }
  }
  else if (distance <= distanceSplitValue)
  {
    pingsSincePersonDetected = 0;
    pingsWithPerson++;
    if (pingsWithPerson >= pingsBeforeStateSwitch)
    {
      toiletOccupied();
    }
  }
  delay(1000);
}
