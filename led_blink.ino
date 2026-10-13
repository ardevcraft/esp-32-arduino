const int LED_PIN = 23;

void setup()
{
    pinMode(LED_PIN, OUTPUT);

    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32 LED test started");
}

void loop()
{
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED ON");
    delay(1000);

    digitalWrite(LED_PIN, LOW);
    Serial.println("LED OFF");
    delay(1000);
}
