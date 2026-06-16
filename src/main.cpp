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
WifiModule wifi(WIFI_SSID, WIFI_PASSWORD);

uint32_t lastMapsLinkMillis = 0;

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
    if (gps.update(&Serial, &Serial))
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
