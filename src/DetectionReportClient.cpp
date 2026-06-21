#include "DetectionReportClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

DetectionReportClient::DetectionReportClient(const char *url, const char *authToken, const char *authScheme,
                                             const char *refreshToken, const char *refreshTokenHeader,
                                             bool tlsInsecure)
    : _url(url),
      _authToken(authToken),
      _authScheme(authScheme),
      _refreshToken(refreshToken),
      _refreshTokenHeader(refreshTokenHeader),
      _tlsInsecure(tlsInsecure)
{
}

bool DetectionReportClient::send(const DetectionReportPayload &payload, WifiModule &wifi, Stream &output)
{
    if (_url == nullptr || _url[0] == '\0')
    {
        output.println("Report: REPORT_URL is leeg, request overgeslagen.");
        return false;
    }

    if (_authToken == nullptr || _authToken[0] == '\0')
    {
        output.println("Report: REPORT_AUTH_TOKEN is leeg, request overgeslagen.");
        return false;
    }

    if (_refreshToken == nullptr || _refreshToken[0] == '\0')
    {
        output.println("Report: REPORT_REFRESH_TOKEN is leeg, request overgeslagen.");
        return false;
    }

    if (_refreshTokenHeader == nullptr || _refreshTokenHeader[0] == '\0')
    {
        output.println("Report: REPORT_REFRESH_TOKEN_HEADER is leeg, request overgeslagen.");
        return false;
    }

    if (!wifi.isConnected())
    {
        output.println("Report: WiFi niet verbonden, opnieuw verbinden...");
        wifi.begin(output, 5000);
    }

    if (!wifi.isConnected())
    {
        output.println("Report: geen WiFi, request overgeslagen.");
        return false;
    }

    HTTPClient http;
    WiFiClient client;
    WiFiClientSecure secureClient;
    String url = _url;
    bool httpStarted = false;

    if (url.startsWith("https://"))
    {
        if (_tlsInsecure)
        {
            secureClient.setInsecure();
        }
        httpStarted = http.begin(secureClient, url);
    }
    else
    {
        httpStarted = http.begin(client, url);
    }

    if (!httpStarted)
    {
        output.println("Report: HTTP client kon niet starten.");
        return false;
    }

    String body = buildBody(payload);

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", buildAuthorizationHeader());
    http.addHeader(_refreshTokenHeader, _refreshToken);

    output.print("Report: POST ");
    output.println(url);
    output.print("Report body: ");
    output.println(body);

    int statusCode = http.POST(body);
    String response = http.getString();
    http.end();

    output.print("Report: response code ");
    output.println(statusCode);
    if (response.length() > 0)
    {
        output.print("Report: response body ");
        output.println(response);
    }

    return statusCode >= 200 && statusCode < 300;
}

String DetectionReportClient::buildBody(const DetectionReportPayload &payload) const
{
    JsonDocument document;
    document["captureDate"] = payload.captureDate;
    document["garbageType"] = payload.garbageType;
    document["location"] = payload.location;
    document["confidence"] = payload.confidence;
    document["externalParameters"] = payload.externalParameters;

    String body;
    serializeJson(document, body);
    return body;
}

String DetectionReportClient::buildAuthorizationHeader() const
{
    String authorization = _authScheme;
    authorization += " ";
    authorization += _authToken;
    return authorization;
}
