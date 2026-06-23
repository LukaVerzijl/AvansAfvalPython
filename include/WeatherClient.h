#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "GpsUtil.h"
#include "WifiModule.h"

class WeatherClient
{
public:
    explicit WeatherClient(const char *baseUrl, bool tlsInsecure = true);

    bool fetchCurrent(const GpsCoordinates &coordinates, String &weatherJson, WifiModule &wifi,
                      Stream &output = Serial) const;

private:
    bool ensureWifi(WifiModule &wifi, Stream &output) const;
    bool beginHttp(HTTPClient &http, WiFiClient &client, WiFiClientSecure &secureClient, const String &url) const;
    String buildUrl(const GpsCoordinates &coordinates) const;
    bool parseCurrentWeather(const String &response, String &weatherJson, Stream &output) const;

    const char *_baseUrl;
    bool _tlsInsecure;
};

#endif
