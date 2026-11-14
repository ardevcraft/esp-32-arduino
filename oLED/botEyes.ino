#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_system.h>

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS = 0x3C;

Adafruit_SSD1306 display(
    SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1, 100000, 100000);

constexpr int EYE_WIDTH = 36;
constexpr int EYE_HEIGHT = 38;
constexpr int EYE_GAP = 16;
constexpr uint32_t FRAME_INTERVAL_MS = 40;

float gazeX = 0;
float gazeY = 0;
int targetX = 0;
int targetY = 0;

bool blinking = false;
uint32_t blinkStartedAt = 0;
uint32_t lastBlinkEndedAt = 0;
uint32_t blinkWaitMs = 2000;

uint32_t lastLookAt = 0;
uint32_t lookWaitMs = 1500;
uint32_t lastFrameAt = 0;

uint32_t randomBetween(uint32_t minimum, uint32_t maximum)
{
    return minimum + esp_random() % (maximum - minimum + 1);
}

void drawEyes(float openness)
{
    display.clearDisplay();

    const int height = max(3, static_cast<int>(EYE_HEIGHT * openness));
    const int radius = min(9, height / 2);

    const int totalWidth = EYE_WIDTH * 2 + EYE_GAP;
    const int leftX =
        (SCREEN_WIDTH - totalWidth) / 2 + static_cast<int>(gazeX);
    const int topY =
        (SCREEN_HEIGHT - height) / 2 + static_cast<int>(gazeY);

    display.fillRoundRect(
        leftX, topY,
        EYE_WIDTH, height, radius,
        SSD1306_WHITE);

    display.fillRoundRect(
        leftX + EYE_WIDTH + EYE_GAP, topY,
        EYE_WIDTH, height, radius,
        SSD1306_WHITE);

    display.display();
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    if (!Wire.begin(SDA_PIN, SCL_PIN, 100000))
    {
        Serial.println("I2C initialization failed.");
        while (true)
            delay(1000);
    }

    Wire.beginTransmission(OLED_ADDRESS);
    if (Wire.endTransmission() != 0)
    {
        Serial.println("Display not found at 0x3C.");
        while (true)
            delay(1000);
    }

    if (!display.begin(
            SSD1306_SWITCHCAPVCC, OLED_ADDRESS, false, false))
    {
        Serial.println("Display buffer allocation failed.");
        while (true)
            delay(1000);
    }

    display.clearDisplay();
    display.display();

    lastBlinkEndedAt = millis();
    lastLookAt = millis();

    Serial.println("Robot eyes started!");
}

void loop()
{
    const uint32_t now = millis();

    if (now - lastFrameAt < FRAME_INTERVAL_MS)
    {
        delay(1);
        return;
    }

    lastFrameAt = now;

    // Occasionally choose a new direction.
    if (!blinking && now - lastLookAt >= lookWaitMs)
    {
        targetX = static_cast<int>(randomBetween(0, 20)) - 10;
        targetY = static_cast<int>(randomBetween(0, 10)) - 5;

        lastLookAt = now;
        lookWaitMs = randomBetween(1000, 2600);
    }

    // Smooth movement toward the new position.
    gazeX += (targetX - gazeX) * 0.22f;
    gazeY += (targetY - gazeY) * 0.22f;

    if (!blinking && now - lastBlinkEndedAt >= blinkWaitMs)
    {
        blinking = true;
        blinkStartedAt = now;
    }

    float openness = 1.0f;

    if (blinking)
    {
        const uint32_t elapsed = now - blinkStartedAt;

        if (elapsed < 120)
        {
            // Close.
            openness = 1.0f - elapsed / 120.0f;
        }
        else if (elapsed < 160)
        {
            // Briefly stay closed.
            openness = 0.0f;
        }
        else if (elapsed < 320)
        {
            // Open.
            openness = (elapsed - 160) / 160.0f;
        }
        else
        {
            blinking = false;
            lastBlinkEndedAt = now;
            blinkWaitMs = randomBetween(1800, 4500);
            openness = 1.0f;
        }
    }

    drawEyes(openness);
}
