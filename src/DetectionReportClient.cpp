#include "DetectionReportClient.h"

#include <ArduinoJson.h>

DetectionReportClient::DetectionReportClient(const char *baseUrl, const char *reportPath, const char *loginEmail,
                                             const char *loginPassword, const char *refreshTokenHeader,
                                             bool tlsInsecure)
    : _baseUrl(baseUrl),
      _reportPath(reportPath),
      _loginEmail(loginEmail),
      _loginPassword(loginPassword),
      _refreshTokenHeader(refreshTokenHeader),
      _tlsInsecure(tlsInsecure),
      _authScheme("Bearer"),
      _authenticated(false)
{
}

bool DetectionReportClient::login(WifiModule &wifi, Stream &output)
{
    if (_baseUrl == nullptr || _baseUrl[0] == '\0')
    {
        output.println("Login: API_BASE_URL is leeg, login overgeslagen.");
        return false;
    }

    if (_loginEmail == nullptr || _loginEmail[0] == '\0')
    {
        output.println("Login: ACCOUNT_LOGIN_EMAIL is leeg, login overgeslagen.");
        return false;
    }

    if (_loginPassword == nullptr || _loginPassword[0] == '\0')
    {
        output.println("Login: ACCOUNT_LOGIN_PASSWORD is leeg, login overgeslagen.");
        return false;
    }

    if (_refreshTokenHeader == nullptr || _refreshTokenHeader[0] == '\0')
    {
        output.println("Login: REPORT_REFRESH_TOKEN_HEADER is leeg, login overgeslagen.");
        return false;
    }

    if (!ensureWifi(wifi, output))
    {
        return false;
    }

    HTTPClient http;
    WiFiClient client;
    WiFiClientSecure secureClient;
    String url = buildUrl("/account/login");

    if (!beginHttp(http, client, secureClient, url))
    {
        output.println("Login: HTTP client kon niet starten.");
        return false;
    }

    String body = buildLoginBody();

    http.addHeader("Content-Type", "application/json");

    output.print("Login: POST ");
    output.println(url);

    int statusCode = http.POST(body);
    String response = http.getString();
    http.end();

    output.print("Login: response code ");
    output.println(statusCode);

    if (statusCode < 200 || statusCode >= 300)
    {
        if (response.length() > 0)
        {
            output.print("Login: response body ");
            output.println(response);
        }
        return false;
    }

    return parseLoginResponse(response, output);
}

bool DetectionReportClient::send(const DetectionReportPayload &payload, WifiModule &wifi, Stream &output)
{
    if (_baseUrl == nullptr || _baseUrl[0] == '\0')
    {
        output.println("Report: API_BASE_URL is leeg, request overgeslagen.");
        return false;
    }

    if (_reportPath == nullptr || _reportPath[0] == '\0')
    {
        output.println("Report: REPORT_PATH is leeg, request overgeslagen.");
        return false;
    }

    if (!_authenticated && !login(wifi, output))
    {
        output.println("Report: niet ingelogd, request overgeslagen.");
        return false;
    }

    if (!ensureWifi(wifi, output))
    {
        return false;
    }

    HTTPClient http;
    WiFiClient client;
    WiFiClientSecure secureClient;
    String url = buildUrl(_reportPath);

    if (!beginHttp(http, client, secureClient, url))
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

bool DetectionReportClient::ensureWifi(WifiModule &wifi, Stream &output) const
{
    if (!wifi.isConnected())
    {
        output.println("WiFi: niet verbonden, opnieuw verbinden...");
        wifi.begin(output, 5000);
    }

    if (!wifi.isConnected())
    {
        output.println("WiFi: geen verbinding.");
        return false;
    }

    return true;
}

bool DetectionReportClient::beginHttp(HTTPClient &http, WiFiClient &client, WiFiClientSecure &secureClient,
                                      const String &url) const
{
    if (url.startsWith("https://"))
    {
        if (_tlsInsecure)
        {
            secureClient.setInsecure();
        }
        return http.begin(secureClient, url);
    }

    return http.begin(client, url);
}

bool DetectionReportClient::parseLoginResponse(const String &response, Stream &output)
{
    JsonDocument document;
    DeserializationError error = deserializeJson(document, response);
    if (error)
    {
        output.print("Login: response JSON kon niet gelezen worden: ");
        output.println(error.c_str());
        return false;
    }

    const char *accessToken = document["accessToken"] | "";
    const char *refreshToken = document["refreshToken"] | "";
    const char *tokenType = document["tokenType"] | "Bearer";

    if (accessToken[0] == '\0')
    {
        output.println("Login: accessToken ontbreekt in response.");
        return false;
    }

    if (refreshToken[0] == '\0')
    {
        output.println("Login: refreshToken ontbreekt in response.");
        return false;
    }

    _accessToken = accessToken;
    _refreshToken = refreshToken;
    _authScheme = tokenType;
    _authenticated = true;

    output.println("Login: succesvol ingelogd.");
    return true;
}

String DetectionReportClient::buildUrl(const char *path) const
{
    String url = _baseUrl;
    if (url.endsWith("/"))
    {
        url.remove(url.length() - 1);
    }

    if (path == nullptr || path[0] == '\0')
    {
        return url;
    }

    if (path[0] != '/')
    {
        url += "/";
    }
    url += path;
    return url;
}

String DetectionReportClient::buildLoginBody() const
{
    JsonDocument document;
    document["email"] = _loginEmail;
    document["password"] = _loginPassword;

    String body;
    serializeJson(document, body);
    return body;
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
    authorization += _accessToken;
    return authorization;
}
