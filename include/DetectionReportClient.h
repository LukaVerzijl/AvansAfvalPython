#ifndef DETECTION_REPORT_CLIENT_H
#define DETECTION_REPORT_CLIENT_H

#include <Arduino.h>

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
    DetectionReportClient(const char *url, const char *authToken, const char *authScheme,
                          const char *refreshToken, const char *refreshTokenHeader, bool tlsInsecure = true);

    bool send(const DetectionReportPayload &payload, WifiModule &wifi, Stream &output = Serial);

private:
    String buildBody(const DetectionReportPayload &payload) const;
    String buildAuthorizationHeader() const;

    const char *_url;
    const char *_authToken;
    const char *_authScheme;
    const char *_refreshToken;
    const char *_refreshTokenHeader;
    bool _tlsInsecure;
};

#endif
