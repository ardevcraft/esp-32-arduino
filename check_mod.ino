#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

void setup()
{
    Serial.begin(115200);

    SPI.begin(18, 19, 23, SS_PIN); // SCK, MISO, MOSI, SS
    rfid.PCD_Init();
    delay(50);

    Serial.println("RC522 reader check:");
    rfid.PCD_DumpVersionToSerial();
    Serial.println("Now tap a card.");
}

void loop()
{
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
    {
        Serial.print("Card detected. UID: ");

        for (byte i = 0; i < rfid.uid.size; i++)
        {
            if (rfid.uid.uidByte[i] < 0x10)
                Serial.print("0");
            Serial.print(rfid.uid.uidByte[i], HEX);
            if (i < rfid.uid.size - 1)
                Serial.print(":");
        }

        Serial.println();
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        delay(500);
    }
}
