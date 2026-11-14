#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS = 0x3C;

// No separate reset pin. Use 100 kHz for I2C transfers.
Adafruit_SSD1306 display(
    SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1, 100000, 100000);

void setup() {
  Serial.begin(115200);
  delay(500);

  if (!Wire.begin(SDA_PIN, SCL_PIN, 100000)) {
    Serial.println("Failed to start I2C.");
    while (true) delay(1000);
  }

  Wire.beginTransmission(OLED_ADDRESS);
  if (Wire.endTransmission() != 0) {
    Serial.println("No display responding at 0x3C.");
    while (true) delay(1000);
  }

  // Wire is already initialized with our chosen pins.
  if (!display.begin(
          SSD1306_SWITCHCAPVCC, OLED_ADDRESS, false, false)) {
    Serial.println("Display buffer allocation failed.");
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  display.setTextSize(2);
  display.setCursor(4, 4);
  display.println("Hello");
  display.setCursor(4, 24);
  display.println("World!");

  display.setTextSize(1);
  display.setCursor(4, 50);
  display.print("AR's ESP32");

  // Send the drawing buffer to the screen.
  display.display();

  Serial.println("Hello World sent to the display.");
}

void loop() {
  delay(100);
}
