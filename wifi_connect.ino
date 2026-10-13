#include <WiFi.h>

const char* ssid = "Operation_dhbd";
const char* password = "";

void setup() {
  Serial.begin(115200);
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname("AR-ESP32");  // Set the device's network name
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected!");
  Serial.print("Device name: ");
  Serial.println(WiFi.getHostname());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {}
