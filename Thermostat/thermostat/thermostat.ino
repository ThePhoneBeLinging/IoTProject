#include <Stepper.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "NPJYOGA9I";
const char* password = "aaaabbbb";

const int stepsPerRevolution = 2038;

const char* url = "http://bathroommaster.local";

bool isTurnedUp = false;
bool shouldTurnOn = false;
Stepper myStepper = Stepper(stepsPerRevolution, 16, 5, 4, 0);

bool getShouldTurnUp() {
  if (WiFi.status() == WL_CONNECTED) {
		WiFiClient client;
    HTTPClient http;
    http.begin(client, String(url) + "/shouldTurnOn");
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
			payload.toLowerCase();
			shouldTurnOn = (payload == "true" || payload == "1");      
			Serial.print("Should turn on: ");
      Serial.println(shouldTurnOn);
			return shouldTurnOn;
    } else {
      Serial.println("Error getting shouldTurnOn");
			return shouldTurnOn;
    }
    http.end();
  }
	return shouldTurnOn;
}

void turnUpThermostat() {
  isTurnedUp = true;
  myStepper.setSpeed(10);

 	for(int i = 0; i < 2038; i++) {
  	myStepper.step(1);
		yield();
	}
}

void turnDownThermostat() {
  isTurnedUp = false;
  myStepper.setSpeed(10);

	for(int i = 0; i < 2038; i++) {
  	myStepper.step(-1);
		yield();
	}
}

void setup() {
	WiFi.begin(ssid, password);
	Serial.begin(9600);

	while(WiFi.status() != WL_CONNECTED){
		Serial.println(".");
		delay(500);
	}
	Serial.print("Connected to: ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

void loop() {
	shouldTurnOn = getShouldTurnUp();
	if (shouldTurnOn && !isTurnedUp){
		turnUpThermostat();
  } else if (!shouldTurnOn && isTurnedUp) {
    turnDownThermostat();
  }
  delay(10000);
}