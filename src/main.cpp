#include <Arduino.h>

#include "GpsUtil.h"
#include "SscmaUtil.h"
#include "WifiModule.h"

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
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

void setup()
{
    Serial.begin(115200);
    delay(4000);

    Serial.println();
    Serial.println("Start Seeed XIAO ESP32S3");

    wifi.begin(Serial, 15000);
    gps.begin();
    ai.begin(atSerial, SSCMA_RX_PIN, SSCMA_TX_PIN, Serial);
}

void loop()
{
    ai.invokeEvery(5000, Serial, true);

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
