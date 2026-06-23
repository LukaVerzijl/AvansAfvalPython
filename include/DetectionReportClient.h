#ifndef DETECTION_REPORT_CLIENT_H
#define DETECTION_REPORT_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "WifiModule.h"

struct DetectionReportPayload
{
    String captureDate;
    String garbageType;
    String location;
    uint8_t confidence = 0;
    String externalParameters;
};

class DetectionReportClient
{
public:
    DetectionReportClient(const char *baseUrl, const char *reportPath, const char *loginEmail,
                          const char *loginPassword, const char *refreshTokenHeader, bool tlsInsecure = true);

    bool login(WifiModule &wifi, Stream &output = Serial);
    bool send(const DetectionReportPayload &payload, WifiModule &wifi, Stream &output = Serial);

private:
    bool ensureWifi(WifiModule &wifi, Stream &output) const;
    bool beginHttp(HTTPClient &http, WiFiClient &client, WiFiClientSecure &secureClient, const String &url) const;
    bool parseLoginResponse(const String &response, Stream &output);
    String buildUrl(const char *path) const;
    String buildLoginBody() const;
    String buildBody(const DetectionReportPayload &payload) const;
    String buildAuthorizationHeader() const;

    const char *_baseUrl;
    const char *_reportPath;
    const char *_loginEmail;
    const char *_loginPassword;
    const char *_refreshTokenHeader;
    bool _tlsInsecure;

    String _accessToken;
    String _refreshToken;
    String _authScheme;
    bool _authenticated;
};

#endif
