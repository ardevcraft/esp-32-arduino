const int BUZZER_PIN = 25;

void setup()
{
    Serial.begin(115200);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void loop()
{
    Serial.println("Buzzer ON");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);

    Serial.println("Buzzer OFF");
    digitalWrite(BUZZER_PIN, LOW);
    delay(1000);
}
