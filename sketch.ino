#include <TM1637Display.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Replace with your network credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Define the NTP client and server details
WiFiUDP ntpUDP;
unsigned long _updateInterval = (1024*1000);
NTPClient timeClient(ntpUDP, "ntp.time.nl", _updateInterval);

// Pins for TM1637 display
#define CLK_PIN 12 // (D5)
#define DIO_PIN 14 // (D6)

// Create a TM1637Display object
TM1637Display display(CLK_PIN, DIO_PIN);

// Weather API
const char* host = "api.tomorrow.io";
const char* endpoint = "/v4/weather/realtime?location=LOCATION&apikey=";
const int httpsPort = 443;
const char* apiKey = "APIKEY";
int updateLoop = 0;
bool flip = false;
float internalTemperature = 1337;

void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);
  // Connect to Wi-Fi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  // Initialize the NTP client and get the time
  timeClient.begin();
  timeClient.setTimeOffset(3600);  // Set your timezone offset in seconds (e.g., GMT+1)
  // Initialize the display
  display.setBrightness(0x1); // Adjust the brightness (0x00 to 0x0f)
  Serial.println("Clock setup complete");

  internalTemperature = getTemperature();
}
void loop() {
  timeClient.update();
  // After 60 ticks get new temperature
  if(updateLoop == 300)
  {
    updateLoop = 0;
    internalTemperature = getTemperature();
  }

  if (updateLoop % 4 == 1)
  {
    flip = !flip; 
  }

  if(flip){
    // Get the current time
    int hours = timeClient.getHours();
    int minutes = timeClient.getMinutes();

    // Display the time on the TM1637 display
    display.showNumberDecEx((hours * 100) + minutes, 0b01000000, true);
 
  }
  else{
    display.showNumberDecEx(internalTemperature * 100, 0b01000000, false);
  }
  delay(1000); // Update every second
  // Display the temperature on the TM1637 display
  updateLoop++;
}

float getTemperature(){

  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return 0;
  }

  // Make request
  client.print(String("GET ") + endpoint + apiKey + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

 // Wait for response headers
  while (!client.available()) {
    delay(100);
  }

  // Read JSON response
  String response = "";
  bool bodyStarted = false;
  while (client.available()) {
    char c = client.read();
    if (c == '{' || bodyStarted) {
      response += c;
      bodyStarted = true;
    }
  }

  // Parse JSON
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, response);

  // Extract temperature
  float temperature = doc["data"]["values"]["temperature"];
  Serial.println(temperature);
  return temperature;
}
