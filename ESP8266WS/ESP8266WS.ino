#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266mDNS.h>
#include "FS.h"

#define WATER_FILE "/water.csv"
#define TOILET_FILE "/toilet.csv"
String newHostname = "bathroommaster";

ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // 0 = UTC offset in seconds, 60000 = update interval (ms)
unsigned long unixTime;

void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();

void setup(void){
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  
  wifiMulti.addAP("NPJYOGA9I", "aaaabbbb");   // add Wi-Fi networks you want to connect to
  WiFi.hostname(newHostname.c_str());


  Serial.println("Connecting ...");
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer

     // Set mDNS hostname
  if (MDNS.begin(newHostname)) {
    Serial.println("mDNS responder started");
    Serial.print("You can now access it via http://");
    Serial.print(newHostname);
    Serial.println(".local");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }
  
  server.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  //server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.onNotFound([]() {
    String path = server.uri();
    Serial.println("Requested URI: " + path);

    // Block any path starting with /data
    if (path.startsWith("/data")) {
      server.send(404, "text/plain", "Forbidden");
      return;
    }

    if (SPIFFS.exists(path)) {
      File file = SPIFFS.open(path, "r");
      String contentType = getContentType(path);
      server.streamFile(file, contentType);
      file.close();
    } else {
      server.send(404, "text/plain", "File Not Found");
    }
  });

  
  server.on("/updateToilet", updateToilet);
  server.on("/submitWater", updateWater);
  server.on("/windowState", getWindowState);
  server.on("/temperature", getTemperature);

  initializeWaterFile();
  initializeToiletFile();
  timeClient.begin();
  server.begin();
  Serial.println("HTTP server started");
}

void initializeWaterFile() {
  if (!SPIFFS.exists(WATER_FILE)) {
    File water_filed = SPIFFS.open(WATER_FILE, "w");
    water_filed.write("timestamp,water_amt\n");
    water_filed.close();
  } else {
    Serial.println("Water file already exists");
  }
}
void appendToWaterFile(float water_amt) {
  if (SPIFFS.exists(WATER_FILE)) {
    File water_filed = SPIFFS.open(WATER_FILE, "a");
    char waterdata[128];
    int len = sprintf(waterdata, "%d,%f\n", unixTime, water_amt);
    water_filed.write(waterdata, len);
    water_filed.close();
  }
}
void initializeToiletFile() {
  if (!SPIFFS.exists(TOILET_FILE)) {
    File toilet_filed = SPIFFS.open(TOILET_FILE, "w");
    toilet_filed.write("timestamp,state\n");
    toilet_filed.close();
  } else {
    Serial.println("Toilet file already exists");
  }
}
void appendToToiletFile(int tstate) {
  if (SPIFFS.exists(TOILET_FILE)) {
    File toilet_filed = SPIFFS.open(TOILET_FILE, "a");
    char toiletdata[64];
    int len = sprintf(toiletdata, "%d,%d\n", unixTime, tstate);
    toilet_filed.write(toiletdata, len);
    toilet_filed.close();
  }
}

void loop(void){
  MDNS.update(); // Important!
  timeClient.update(); // Update unix time
  unixTime = timeClient.getEpochTime(); // Set global unixTime
  
  server.handleClient();                    // Listen for HTTP requests from clients
}

void handleRoot() {
  server.send(200, "text/plain", "Hello world!");   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void updateToilet() {
  String value = server.arg("state");
  int number = value.toInt();
  appendToToiletFile(number);
  server.send(200, "text/plain", "OK");
}

void updateWater() {
  String water_amt = server.arg("water_amt");
  float number = water_amt.toFloat();;
  appendToWaterFile(number);
  server.send(200, "text/plain", "OK");
}
float temperature = 19.0;
void getTemperature() {
  temperature += 0.5;
  if (temperature > 24) { temperature = 19.0; }
  Serial.println("Temp");
  server.send(200, "text/plain", String(temperature));
}

bool windowState = false;
void getWindowState() {
  windowState = !windowState;
  Serial.println("Window");
  server.send(200, "text/plain", String(windowState ? "true" : "false"));
}


// Determine MIME type
String getContentType(String filename) {
  if (filename.endsWith(".htm") || filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/pdf";
  else if (filename.endsWith(".zip")) return "application/zip";
  return "text/plain";
}
