#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// - Time: Bangladesh time, synchronized over Wi-Fi.
// - Network: actual Wi-Fi connection and signal strength—not an internet availability check.
// - Charging: CHG: -- for now. USB power alone doesn’t tell us whether a battery is charging;
// we need your charger module ’s status signal.
    // Enter your 2.4 GHz Wi-Fi credentials.
    const char *WIFI_SSID = "YOUR_WIFI_NAME";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;

// Bangladesh: UTC+6, no daylight-saving adjustment.
constexpr long UTC_OFFSET_SECONDS = 6L * 60L * 60L;

Adafruit_SSD1306 display(
    SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1, 100000, 100000);

uint32_t lastDrawAt = 0;
uint32_t lastReconnectAt = 0;
bool ntpStarted = false;

void stopWithError(const char *message)
{
    Serial.println(message);
    while (true)
    {
        delay(1000);
    }
}

void drawCentered(const char *text, int y, uint8_t size)
{
    display.setTextSize(size);

    int16_t x1, y1;
    uint16_t width, height;
    display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

    const int x = (SCREEN_WIDTH - static_cast<int>(width)) / 2 - x1;
    display.setCursor(x, y);
    display.print(text);
}

void drawSignalBars(bool connected)
{
    constexpr int startX = 106;
    constexpr int baseline = 10;

    int bars = 0;

    if (connected)
    {
        const int rssi = WiFi.RSSI();
        if (rssi >= -55)
        {
            bars = 4;
        }
        else if (rssi >= -67)
        {
            bars = 3;
        }
        else if (rssi >= -75)
        {
            bars = 2;
        }
        else
        {
            bars = 1;
        }
    }

    for (int i = 0; i < 4; ++i)
    {
        const int height = 3 + i * 2;
        const int x = startX + i * 5;
        const int y = baseline - height;

        if (i < bars)
        {
            display.fillRect(x, y, 3, height, SSD1306_WHITE);
        }
        else
        {
            display.drawRect(x, y, 3, height, SSD1306_WHITE);
        }
    }
}

void drawStatus()
{
    const bool connected = WiFi.status() == WL_CONNECTED;

    time_t currentTime = time(nullptr);
    struct tm localTime = {};

    // Do not display a bogus date before network time arrives.
    const bool timeReady = currentTime > 1700000000;

    if (timeReady)
    {
        localtime_r(&currentTime, &localTime);
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);

    // Top status bar.
    display.setCursor(0, 2);
    display.print("CHG: --");

    display.setCursor(56, 2);
    display.print(connected ? "WiFi ON" : "WiFi --");

    drawSignalBars(connected);
    display.drawFastHLine(0, 14, SCREEN_WIDTH, SSD1306_WHITE);

    if (timeReady)
    {
        char clockText[9];
        char dateText[20];

        strftime(clockText, sizeof(clockText), "%H:%M:%S", &localTime);
        strftime(dateText, sizeof(dateText), "%d %b %Y", &localTime);

        drawCentered(clockText, 22, 2);
        drawCentered(dateText, 43, 1);
    }
    else
    {
        drawCentered("--:--:--", 22, 2);
        drawCentered(connected ? "Syncing time..." : "Waiting for WiFi", 43, 1);
    }

    drawCentered("DHAKA UTC+6", 56, 1);
    display.display();
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    if (!Wire.begin(SDA_PIN, SCL_PIN, 100000))
    {
        stopWithError("Failed to start I2C.");
    }

    Wire.beginTransmission(OLED_ADDRESS);
    if (Wire.endTransmission() != 0)
    {
        stopWithError("Display not found at 0x3C.");
    }

    if (!display.begin(
            SSD1306_SWITCHCAPVCC, OLED_ADDRESS, false, false))
    {
        stopWithError("Display buffer allocation failed.");
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    drawStatus();
    Serial.println("Status screen started. Connecting to WiFi...");
}

void loop()
{
    const uint32_t now = millis();
    const bool connected = WiFi.status() == WL_CONNECTED;

    if (connected && !ntpStarted)
    {
        configTime(
            UTC_OFFSET_SECONDS,
            0,
            "pool.ntp.org",
            "time.google.com");

        ntpStarted = true;
        Serial.println("WiFi connected. Network time sync started.");
    }

    // Retry without blocking the screen.
    if (!connected && now - lastReconnectAt >= 20000)
    {
        lastReconnectAt = now;
        WiFi.reconnect();
    }

    if (now - lastDrawAt >= 500)
    {
        lastDrawAt = now;
        drawStatus();
    }

    delay(5);
}
