#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>

#include "AppConfig.h"
#include "DetectionReportClient.h"
#include "GpsUtil.h"
#include "SscmaUtil.h"
#include "WifiModule.h"

GpsUtil gps;
SscmaUtil ai;
WifiModule wifi(WIFI_SSID, WIFI_PASSWORD);
DetectionReportClient reportClient(API_BASE_URL, REPORT_PATH, ACCOUNT_LOGIN_EMAIL, ACCOUNT_LOGIN_PASSWORD,
                                   REPORT_REFRESH_TOKEN_HEADER, REPORT_TLS_INSECURE);
HardwareSerial atSerial(0);

uint32_t lastMapsLinkMillis = 0;
uint32_t lastGpsStatusMillis = 0;
uint32_t lastAiSearchMillis = 0;
uint32_t lastAiCheckMillis = 0;
uint32_t lastReportAttemptMillis = 0;
uint32_t lastAiGpsWaitMillis = 0;
bool lastGpsFix = false;
bool aiReleasedAfterGpsFix = false;
bool firstGpsUpdateStarted = false;
bool firstGpsUpdateFinished = false;
bool rawInvokePrinted = false;

struct DetectionReportSource
{
    size_t detectionCount = 0;
    int target = -1;
    uint8_t confidence = 0;
    const char *sourceType = "";
};

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

void configureTimeSync()
{
    if (!wifi.isConnected())
    {
        return;
    }

    configTime(0, 0, TIME_NTP_SERVER);
    Serial.print("Tijd: NTP sync gestart via ");
    Serial.println(TIME_NTP_SERVER);
}

String captureDateUtc()
{
    struct tm timeInfo;
    if (!getLocalTime(&timeInfo, 10))
    {
        return "1970-01-01T00:00:00.000Z";
    }

    char buffer[25];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S.000Z", &timeInfo);
    return String(buffer);
}

String currentGpsLocation()
{
#if GPS_ENABLED
    if (!gps.hasFix())
    {
        return "";
    }

    GpsCoordinates coordinates = gps.getCoordinates();
    if (!coordinates.isValid())
    {
        return "";
    }

    String location = String(coordinates.latitude, 6);
    location += ",";
    location += String(coordinates.longitude, 6);
    return location;
#else
    return "";
#endif
}

DetectionReportSource collectDetectionReportSource()
{
    DetectionReportSource result;

    for (size_t i = 0; i < ai.boxCount(); i++)
    {
        boxes_t detection = ai.box(i);
        if (detection.score < SSCMA_MIN_SCORE)
        {
            continue;
        }

        result.detectionCount++;
        if (detection.score >= result.confidence)
        {
            result.target = detection.target;
            result.confidence = detection.score;
            result.sourceType = "box";
        }
    }

    for (size_t i = 0; i < ai.classCount(); i++)
    {
        classes_t classification = ai.classification(i);
        if (classification.score < SSCMA_MIN_SCORE)
        {
            continue;
        }

        result.detectionCount++;
        if (classification.score >= result.confidence)
        {
            result.target = classification.target;
            result.confidence = classification.score;
            result.sourceType = "class";
        }
    }

    return result;
}

String garbageTypeFromTarget(const DetectionReportSource &detection)
{
    if (detection.target < 0)
    {
        return "";
    }

    String garbageType = "target-";
    garbageType += String(detection.target);
    return garbageType;
}

String buildExternalParameters(const DetectionReportSource &detection)
{
    JsonDocument document;
    perf_t perf = ai.performance();

    document["source"] = "sscma";
    document["sourceType"] = detection.sourceType;
    document["target"] = detection.target;
    document["detections"] = detection.detectionCount;
    document["minimumScore"] = SSCMA_MIN_SCORE;
    document["perfPreprocess"] = perf.prepocess;
    document["perfInference"] = perf.inference;
    document["perfPostprocess"] = perf.postprocess;

    String value;
    serializeJson(document, value);
    return value;
}

DetectionReportPayload buildDetectionReportPayload(const DetectionReportSource &detection)
{
    DetectionReportPayload payload;
    payload.captureDate = captureDateUtc();
    payload.garbageType = garbageTypeFromTarget(detection);
    payload.location = currentGpsLocation();
    payload.confidence = detection.confidence;
    payload.externalParameters = buildExternalParameters(detection);
    return payload;
}

void reportIfEnoughDetections()
{
    DetectionReportSource detection = collectDetectionReportSource();
    if (detection.detectionCount < REPORT_DETECTION_COUNT)
    {
        return;
    }

    if (lastReportAttemptMillis != 0 && millis() - lastReportAttemptMillis < REPORT_COOLDOWN_MS)
    {
        Serial.println("Report: genoeg detecties, maar cooldown is nog actief.");
        return;
    }

    lastReportAttemptMillis = millis();

    Serial.print("Report: ");
    Serial.print(detection.detectionCount);
    Serial.println(" detecties gevonden, request versturen...");

    DetectionReportPayload payload = buildDetectionReportPayload(detection);
    if (reportClient.send(payload, wifi, Serial))
    {
        Serial.println("Report: succesvol verstuurd.");
    }
    else
    {
        Serial.println("Report: versturen mislukt.");
    }
}

void printGpsStatus(const char *reason)
{
#if GPS_ENABLED
    Serial.print("GPS: ");
    Serial.print(reason);

    if (!gps.hasFix())
    {
        Serial.print(" | geen fix");
        Serial.print(" | satellieten ");
        Serial.println(gps.satelliteCount());
        return;
    }

    GpsCoordinates coordinates = gps.getCoordinates();
    Serial.print(" | fix");
    Serial.print(" | satellieten ");
    Serial.print(gps.satelliteCount());
    Serial.print(" | HDOP ");
    Serial.print(gps.hdop(), 2);
    Serial.print(" | lat ");
    Serial.print(coordinates.latitude, 6);
    Serial.print(" | lon ");
    Serial.println(coordinates.longitude, 6);
#endif
}

void updateGps()
{
#if GPS_ENABLED
    if (lastGpsStatusMillis == 0 || millis() - lastGpsStatusMillis >= 5000)
    {
        printGpsStatus(gps.hasFix() ? "status" : "zoekt naar fix");
        lastGpsStatusMillis = millis();
    }

    if (!firstGpsUpdateStarted)
    {
        firstGpsUpdateStarted = true;
        Serial.println("GPS: eerste update starten");
    }

    bool parsedGpsMessage = gps.update();

#if GPS_RAW_DEBUG
    static String lastPrintedGpsRaw;
    String lastNmea = gps.lastNmea();
    if (lastNmea.length() > 0 && lastNmea != lastPrintedGpsRaw)
    {
        Serial.print("GPS raw: ");
        Serial.println(lastNmea);
        lastPrintedGpsRaw = lastNmea;
    }
#endif

    if (!firstGpsUpdateFinished)
    {
        firstGpsUpdateFinished = true;
        Serial.println("GPS: eerste update klaar");
    }

    bool hasFix = gps.hasFix();
    if (parsedGpsMessage && hasFix != lastGpsFix)
    {
        printGpsStatus(hasFix ? "fix gevonden" : "fix verloren");
        lastGpsFix = hasFix;
        lastGpsStatusMillis = millis();
    }

    if (parsedGpsMessage)
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

bool gpsReadyForAi()
{
#if GPS_ENABLED
    if (!gps.hasFix() || gps.satelliteCount() == 0)
    {
        if (lastAiGpsWaitMillis == 0 || millis() - lastAiGpsWaitMillis >= 5000)
        {
            Serial.print("SSCMA: wacht op GPS-fix voordat AI start | satellieten ");
            Serial.println(gps.satelliteCount());
            lastAiGpsWaitMillis = millis();
        }

        return false;
    }

    if (!aiReleasedAfterGpsFix)
    {
        aiReleasedAfterGpsFix = true;
        Serial.println("SSCMA: GPS-fix gevonden, AI starten...");
    }

    return true;
#else
    return true;
#endif
}

void setup()
{
    Serial.begin(115200);
    delay(4000);

    Serial.println();
    Serial.println("Start Seeed XIAO ESP32S3");

    wifi.begin(Serial, 15000);
    configureTimeSync();
    reportClient.login(wifi, Serial);
#if GPS_ENABLED
    gps.begin();
    Serial.println("GPS: gestart");
#else
    Serial.println("GPS: tijdelijk uitgeschakeld voor AI-debug.");
#endif
}

void loop()
{
    updateGps();

    if (!gpsReadyForAi())
    {
        return;
    }

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
            reportIfEnoughDetections();

            if (!rawInvokePrinted && ai.boxCount() == 0 && ai.classCount() == 0 &&
                ai.pointCount() == 0 && ai.keypointCount() == 0)
            {
                rawInvokePrinted = true;
                ai.printRawInvoke(Serial);
            }
        }
    }
}
