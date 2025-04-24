#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>

// We have a DHT11 on pin D3
DHT dht(D3, DHT11);

// WiFi credentials
const char *ssid = "NPJYOGA9I";
const char *pswd = "aaaabbbb";

// Webserver
const char *host = "bathroommaster.local";
const char *endpointFormat = "/submitBathroomDHT?temp=%.1f&hum=%.1f";
const uint16_t port = 80;

void setup() {
	// Initialise everything
	Serial.begin(115200);
	dht.begin();
	WiFi.begin(ssid, pswd);
}

void loop() {
	// Always wait a while before doing stuff again
	delay(15000);

	// If we aren't connected to WiFi, don't do anything
	if (WiFi.status() != WL_CONNECTED) return;

	// Read data from sensor
	float temperature = dht.readTemperature(false, true);
	float humidity = dht.readHumidity();

	// Send data over serial
	Serial.print("Temp = ");
	if (isnan(temperature)) Serial.print("N/A"); else { Serial.print(temperature); Serial.print("°C"); }
	Serial.print(", Humi = ");
	if (isnan(humidity)) Serial.print("N/A"); else { Serial.print(humidity); Serial.print("%"); }
	Serial.print("\n");

	// Don't bother sending NaNs to the server
	if (isnan(temperature) || isnan(humidity)) return;

	WiFiClient client;
	HTTPClient http;
	// Format the data into an endpoint (a path with query parameters)
	char endpoint[40]; // 40 characters is enough to store everything
	sprintf(endpoint, endpointFormat, temperature, humidity);
	// Connect to the server and actually send the data
	http.begin(client, host, port, endpoint);
	int statusCode = http.GET();
	if (statusCode < 0) {
		Serial.printf("Web request failed: %s\n", http.errorToString(statusCode).c_str());
	}
}
