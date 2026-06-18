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

#ifndef GPS_ENABLED
#define GPS_ENABLED 0
#endif

#ifndef SSCMA_MIN_SCORE
#define SSCMA_MIN_SCORE 60
#endif

#ifndef SSCMA_DEVICE_TSCORE
#define SSCMA_DEVICE_TSCORE 60
#endif

GpsUtil gps;
SscmaUtil ai;
WifiModule wifi(WIFI_SSID, WIFI_PASSWORD);
HardwareSerial atSerial(0);

uint32_t lastMapsLinkMillis = 0;
uint32_t lastAiSearchMillis = 0;
uint32_t lastAiCheckMillis = 0;
bool rawInvokePrinted = false;

void configureAiVision()
{
    ai.printInfo(Serial);
    ai.printRawDiagnostics(Serial);
    ai.configureScoreThreshold(SSCMA_DEVICE_TSCORE, Serial);
}

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
        configureAiVision();
        return true;
    }

    Serial.println("SSCMA: UART default proberen...");
    if (ai.begin(atSerial, Serial))
    {
        configureAiVision();
        return true;
    }

    Serial.println("SSCMA: I2C proberen...");
    if (ai.begin(Serial))
    {
        configureAiVision();
        return true;
    }

    return false;
}

void setup()
{
    Serial.begin(115200);
    delay(4000);

    Serial.println();
    Serial.println("Start Seeed XIAO ESP32S3");

    wifi.begin(Serial, 15000);
#if GPS_ENABLED
    gps.begin();
    Serial.println("GPS: gestart");
#else
    Serial.println("GPS: tijdelijk uitgeschakeld voor AI-debug.");
#endif
}

void loop()
{
    if (findAiVision())
    {
        if (lastAiCheckMillis == 0 || millis() - lastAiCheckMillis >= 5000)
        {
            lastAiCheckMillis = millis();
            Serial.println("SSCMA: check detecties...");

            if (!ai.invoke(true))
            {
                if (ai.lastError() == CMD_ETIMEDOUT)
                {
                    Serial.println("SSCMA: geen nieuwe data, zonder filter proberen...");
                    ai.invoke(false);
                }
                else
                {
                    Serial.print("SSCMA: invoke fout ");
                    Serial.println(ai.lastError());
                }
            }

            ai.printSummary(Serial);
            ai.printDetections(Serial, SSCMA_MIN_SCORE);

            if (!rawInvokePrinted && ai.boxCount() == 0 && ai.classCount() == 0 &&
                ai.pointCount() == 0 && ai.keypointCount() == 0)
            {
                rawInvokePrinted = true;
                ai.printRawInvoke(Serial);
            }
        }
    }

#if GPS_ENABLED
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
#endif
}
