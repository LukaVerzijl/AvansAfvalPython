#include <Arduino.h>

#include "GpsUtil.h"
#include "SscmaUtil.h"
#include "WifiModule.h"

#ifndef WIFI_SSID
#define WIFI_SSID "Huize Heysterbach"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "VPYQew9NUfb5Kz"
#endif

#ifndef SSCMA_RX_PIN
#define SSCMA_RX_PIN D7
#endif

#ifndef SSCMA_TX_PIN
#define SSCMA_TX_PIN D6
#endif

GpsUtil gps;
SscmaUtil ai;
WifiModule wifi(WIFI_SSID, WIFI_PASSWORD);
HardwareSerial atSerial(0);

uint32_t lastMapsLinkMillis = 0;
uint32_t lastAiSearchMillis = 0;

bool findAiVision()
{
    if (ai.isReady())
    {
        return true;
    }

    if (lastAiSearchMillis != 0 && millis() - lastAiSearchMillis < 5000)
    {
        return false;
    }
    lastAiSearchMillis = millis();

    Serial.println("SSCMA: zoeken naar Seeed Vision AI...");

    if (ai.begin(atSerial, SSCMA_RX_PIN, SSCMA_TX_PIN, Serial))
    {
        return true;
    }

    Serial.println("SSCMA: UART default proberen...");
    if (ai.begin(atSerial, Serial))
    {
        return true;
    }

    Serial.println("SSCMA: I2C proberen...");
    return ai.begin(Serial);
}

void setup()
{
    Serial.begin(115200);
    delay(4000);

    Serial.println();
    Serial.println("Start Seeed XIAO ESP32S3");

    wifi.begin(Serial, 15000);
    gps.begin();
}

void loop()
{
    if (findAiVision())
    {
        ai.invokeEvery(5000, Serial, true);
    }

    if (gps.update())
    {
        if (lastMapsLinkMillis == 0 || millis() - lastMapsLinkMillis >= 5000)
        {
            if (gps.printGoogleMapsLink(Serial))
            {
                lastMapsLinkMillis = millis();
            }
        }
    }
}
