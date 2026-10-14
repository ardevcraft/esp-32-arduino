#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 22

const char *WIFI_SSID = "Operation_dhbd";
const char *WIFI_PASSWORD = "";
const char *API_URL = "https://YOUR_DOMAIN.com/takeattendance";

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
WiFiClientSecure tlsClient;

const byte BLOCK_ADDRESS = 4;

String pendingName;
String pendingUuid;
bool hasPendingAttendance = false;
bool hasTriedPost = false;
unsigned long lastPostAttempt = 0;
unsigned long lastWifiRetry = 0;

void maintainWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }

    if (millis() - lastWifiRetry >= 5000)
    {
        lastWifiRetry = millis();
        Serial.println("Wi-Fi disconnected; trying to reconnect...");
        WiFi.reconnect();
    }
}

String escapeJson(const String &value)
{
    String escaped;

    for (unsigned int i = 0; i < value.length(); i++)
    {
        char c = value[i];

        if (c == '"' || c == '\\')
        {
            escaped += '\\';
        }

        if (c == '\n')
        {
            escaped += "\\n";
        }
        else if (c == '\r')
        {
            escaped += "\\r";
        }
        else if (c == '\t')
        {
            escaped += "\\t";
        }
        else
        {
            escaped += c;
        }
    }

    return escaped;
}

bool readCardRecord(String &name, String &uuid)
{
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    {
        return false;
    }

    byte buffer[18];
    byte bufferSize = sizeof(buffer);

    MFRC522::StatusCode status = rfid.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        BLOCK_ADDRESS,
        &key,
        &rfid.uid);

    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Card authentication failed: ");
        Serial.println(rfid.GetStatusCodeName(status));
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        return false;
    }

    status = rfid.MIFARE_Read(BLOCK_ADDRESS, buffer, &bufferSize);

    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Card read failed: ");
        Serial.println(rfid.GetStatusCodeName(status));
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        return false;
    }

    char record[17];
    memcpy(record, buffer, 16);
    record[16] = '\0';

    String savedRecord(record);
    int separator = savedRecord.indexOf('|');

    if (separator <= 0)
    {
        Serial.println("Card has no valid name|UUID record. Run the card setup sketch first.");
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        return false;
    }

    name = savedRecord.substring(0, separator);
    uuid = savedRecord.substring(separator + 1);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return name.length() > 0 && uuid.length() > 0;
}

void sendAttendance()
{
    HTTPClient http;
    http.setTimeout(8000);

    if (!http.begin(tlsClient, API_URL))
    {
        Serial.println("Could not start HTTP connection; will retry.");
        return;
    }

    http.addHeader("Content-Type", "application/json");

    String body = "{\"name\":\"" + escapeJson(pendingName) +
                  "\",\"uuid\":\"" + escapeJson(pendingUuid) + "\"}";

    Serial.print("POST body: ");
    Serial.println(body);

    int statusCode = http.POST(body);

    if (statusCode >= 200 && statusCode < 300)
    {
        Serial.print("Attendance recorded. HTTP ");
        Serial.println(statusCode);
        Serial.println(http.getString());
        hasPendingAttendance = false;
        hasTriedPost = false;
    }
    else if (statusCode < 0 || statusCode >= 500)
    {
        Serial.print("Network/server error: ");
        Serial.println(statusCode);
        Serial.println("Keeping this record and retrying.");
    }
    else
    {
        Serial.print("Server rejected the request. HTTP ");
        Serial.println(statusCode);
        Serial.println(http.getString());
        hasPendingAttendance = false;
        hasTriedPost = false;
    }

    http.end();
}

void setup()
{
    Serial.begin(115200);

    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    SPI.begin(18, 19, 23, SS_PIN);
    rfid.PCD_Init();

    WiFi.mode(WIFI_STA);
    WiFi.setHostname("AR-ESP32");
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Prototype only: this skips HTTPS certificate verification.
    // For deployment, replace with tlsClient.setCACert(yourRootCACertificate).
    tlsClient.setInsecure();

    Serial.println("AR-ESP32 attendance reader starting.");
}

void loop()
{
    maintainWiFi();

    if (hasPendingAttendance)
    {
        if (WiFi.status() == WL_CONNECTED &&
            (!hasTriedPost || millis() - lastPostAttempt >= 5000))
        {
            hasTriedPost = true;
            lastPostAttempt = millis();
            sendAttendance();
        }

        delay(10);
        return;
    }

    String name;
    String uuid;

    if (readCardRecord(name, uuid))
    {
        pendingName = name;
        pendingUuid = uuid;
        hasPendingAttendance = true;
        hasTriedPost = false;

        Serial.print("Card punched: ");
        Serial.print(name);
        Serial.print(" / ");
        Serial.println(uuid);

        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Saved in memory; waiting for Wi-Fi to reconnect.");
        }
    }

    delay(10);
}
