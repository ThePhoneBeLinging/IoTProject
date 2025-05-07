#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>

#define USE_SERIAL
#define USE_LEDS

// The delay in ms to use between each sensor read
// Don't set it too low as to not overload the server
const int DELAY = 15000;

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
	#ifdef USE_SERIAL
		Serial.begin(115200);
	#endif
	dht.begin();
	WiFi.begin(ssid, pswd);
	#ifdef USE_LEDS
		pinMode(LED_BUILTIN, OUTPUT);
		pinMode(LED_BUILTIN_AUX, OUTPUT);
		digitalWrite(LED_BUILTIN, HIGH);
		digitalWrite(LED_BUILTIN_AUX, HIGH);
	#endif
}

void loop() {
	#ifdef USE_LEDS
		digitalWrite(LED_BUILTIN_AUX, WiFi.status() == WL_CONNECTED);
	#endif

	// If we aren't connected to WiFi, don't do anything
	if (WiFi.status() != WL_CONNECTED) { delay(500); return; }

	// Read data from sensor
	float temperature = dht.readTemperature(false, true);
	float humidity = dht.readHumidity();

	#ifdef USE_SERIAL
		// Send data over serial
		Serial.print("Temp = ");
		if (isnan(temperature)) Serial.print("N/A"); else { Serial.print(temperature); Serial.print("°C"); }
		Serial.print(", Humi = ");
		if (isnan(humidity)) Serial.print("N/A"); else { Serial.print(humidity); Serial.print("%"); }
		Serial.print("\n");
	#endif

	// Don't bother sending NaNs to the server
	if (isnan(temperature) || isnan(humidity)) { delay(DELAY); return; }

	WiFiClient client;
	HTTPClient http;
	// Format the data into an endpoint (a path with query parameters)
	char endpoint[40]; // 40 characters is enough to store everything
	sprintf(endpoint, endpointFormat, temperature, humidity);
	// Connect to the server and actually send the data
	http.begin(client, host, port, endpoint);
	int statusCode = http.GET();
	#ifdef USE_SERIAL
		if (statusCode < 0) {
			Serial.printf("Web request failed: %s\n", http.errorToString(statusCode).c_str());
		}
	#endif
	#ifdef USE_LEDS
		int duration = statusCode < 0 ? 1000 : 100;
		digitalWrite(LED_BUILTIN, LOW);
		delay(duration);
		digitalWrite(LED_BUILTIN, HIGH);
		delay(DELAY - duration);
	#else
		delay(DELAY);
	#endif
}
