#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <WiFiUdp.h>
#include <NTPClient.h> // NTP Client by Fabrice Weinberg
#include <ESP8266mDNS.h>
#include "FS.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> // Using adafruit lcd library

#define WATER_FILE "/water.csv"
#define TOILET_FILE "/toilet.csv"
#define DHT_FILE "/dht.csv"
String newHostname = "bathroommaster";

ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(8080);    // Create a webserver object that listens for HTTP request on port 80

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // 0 = UTC offset in seconds, 60000 = update interval (ms)
unsigned long unixTime;

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
#define WIFI_SSID "Dunder Milffin"
#define WIFI_PASSWORD "BodomReaper666"

void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();

void setup(void){
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting:");
  lcd.setCursor(0,1);
  lcd.print(WIFI_SSID);

  
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);   // add Wi-Fi networks you want to connect to
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
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connected, IP:");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());

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
  server.on("/submitBathroomDHT", submitBathroomDHT);
  server.on("/windowState", getWindowState);
  server.on("/temperature", getTemperature);

  initializeFile(WATER_FILE, "timestamp,water_amt\n");
  initializeFile(TOILET_FILE, "timestamp,state\n");
  initializeFile(DHT_FILE, "timestamp,temp,hum\n");
  timeClient.begin();
  server.begin();
  Serial.println("HTTP server started");
}

void initializeFile(char* filename, char* firstline) {
  if (!SPIFFS.exists(filename)) {
    File water_filed = SPIFFS.open(filename, "w");
    water_filed.write(firstline, strlen(firstline));
    water_filed.close();
  } else {
    Serial.print(filename);
    Serial.println(" already exists");
  }
}

int appendToFile(const char* filename, const char* format, ...) {
  if (SPIFFS.exists(filename)) {
    File file = SPIFFS.open(filename, "a");
    if (!file) {
      Serial.println("Failed to open file for appending!");
      return 1;
    }

    char buffer[128];  // Adjust size as needed
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0 && len < sizeof(buffer)) {
      file.write((const uint8_t*)buffer, len);
    } else {
      Serial.println("Error formatting data or buffer overflow.");
      file.close();
      return 1;
    }

    file.close();
    return 0;
  } else {
    Serial.print("File not found: ");
    Serial.println(filename);
    return 1;
  }
}


void loop(void){
  MDNS.update(); // Important!
  timeClient.update(); // Update unix time
  unixTime = timeClient.getEpochTime(); // Set global unixTime
  
  server.handleClient();                    // Listen for HTTP requests from clients
  updateLCD();
}

void handleRoot() {
  server.send(200, "text/plain", "Hello world!");   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

int toilet_state = 0;
void updateToilet() {
  String value = server.arg("state");
  toilet_state = value.toInt();
  bool success = !appendToFile(TOILET_FILE, "%d,%d\n", unixTime, toilet_state);
  if (success) { server.send(200, "text/plain", "OK"); }
  else { server.send(500, "text/plain", "ERROR WRITING TO FILE"); }
}

float water_number = 0.0;
void updateWater() {
  String water_amt = server.arg("water_amt");
  water_number = water_amt.toFloat();;
  bool success = !appendToFile(WATER_FILE, "%d,%f\n", unixTime, water_number);
  if (success) { server.send(200, "text/plain", "OK"); }
  else { server.send(500, "text/plain", "ERROR WRITING TO FILE"); }
}

float temp = 0.0;
float hum = 0.0;
void submitBathroomDHT() {
  temp = server.arg("temp").toFloat();
  hum = server.arg("hum").toFloat();
  bool success = !appendToFile(DHT_FILE, "%d,%f,%f\n", unixTime, temp, hum);
  if (success) { server.send(200, "text/plain", "OK"); }
  else { server.send(500, "text/plain", "ERROR WRITING TO FILE"); }
}
void getTemperature() {
  server.send(200, "text/plain", String(temp));
}

bool windowState = false;
void getWindowState() {
  windowState = !windowState;
  Serial.println("Window");
  server.send(200, "text/plain", String(windowState ? "true" : "false"));
}

#define LCD_maxindex 4
int LCD_index = 0;
void updateLCD() {
  char msg[17];
  lcd.clear();
  lcd.setCursor(0,0);
  
  switch(LCD_index) {
    case 0: lcd.print("Toilet State"); lcd.setCursor(0,1); lcd.print(toilet_state ? "Active" : "Inactive"); break;
    case 1: lcd.print("Shower Usage"); lcd.setCursor(0,1); sprintf(msg, "%.2fL", water_number); lcd.print(msg); break;
    case 2: lcd.print("Room Temperature"); lcd.setCursor(0,1); sprintf(msg, "%.2f C", temp); lcd.print(msg); break;
    case 3: lcd.print("Room Humidity"); lcd.setCursor(0,1); sprintf(msg, "%.2f %", hum); lcd.print(msg); break;
  }
  LCD_index = (LCD_index+1)%LCD_maxindex;
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
  
