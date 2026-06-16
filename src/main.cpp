#include <Arduino.h>

#include "GpsUtil.h"
#include "WifiModule.h"

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
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
    ai.begin(atSerial, Serial);
}

void loop()
{
    if (gps.update(&Serial, &Serial))
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
