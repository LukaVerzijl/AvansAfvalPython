#include "WeatherClient.h"

#include <ArduinoJson.h>

WeatherClient::WeatherClient(const char *baseUrl, bool tlsInsecure)
    : _baseUrl(baseUrl), _tlsInsecure(tlsInsecure)
{
}

bool WeatherClient::fetchCurrent(const GpsCoordinates &coordinates, String &weatherJson, WifiModule &wifi,
                                 Stream &output) const
{
    if (_baseUrl == nullptr || _baseUrl[0] == '\0')
    {
        output.println("Weather: OPEN_METEO_BASE_URL is leeg, request overgeslagen.");
        return false;
    }

    if (!coordinates.isValid())
    {
        output.println("Weather: GPS coordinaten zijn niet geldig.");
        return false;
    }

    if (!ensureWifi(wifi, output))
    {
        return false;
    }

    HTTPClient http;
    WiFiClient client;
    WiFiClientSecure secureClient;
    String url = buildUrl(coordinates);

    if (!beginHttp(http, client, secureClient, url))
    {
        output.println("Weather: HTTP client kon niet starten.");
        return false;
    }

    output.print("Weather: GET ");
    output.println(url);

    int statusCode = http.GET();
    String response = http.getString();
    http.end();

    output.print("Weather: response code ");
    output.println(statusCode);

    if (statusCode < 200 || statusCode >= 300)
    {
        if (response.length() > 0)
        {
            output.print("Weather: response body ");
            output.println(response);
        }
        return false;
    }

    return parseCurrentWeather(response, weatherJson, output);
}

bool WeatherClient::ensureWifi(WifiModule &wifi, Stream &output) const
{
    if (!wifi.isConnected())
    {
        output.println("Weather: WiFi niet verbonden, opnieuw verbinden...");
        wifi.begin(output, 5000);
    }

    if (!wifi.isConnected())
    {
        output.println("Weather: geen WiFi, request overgeslagen.");
        return false;
    }

    return true;
}

bool WeatherClient::beginHttp(HTTPClient &http, WiFiClient &client, WiFiClientSecure &secureClient,
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

String WeatherClient::buildUrl(const GpsCoordinates &coordinates) const
{
    String url = _baseUrl;
    if (url.endsWith("/"))
    {
        url.remove(url.length() - 1);
    }

    url += "/v1/forecast?latitude=";
    url += String(coordinates.latitude, 6);
    url += "&longitude=";
    url += String(coordinates.longitude, 6);
    url += "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,";
    url += "showers,snowfall,weather_code,cloud_cover,wind_speed_10m,wind_direction_10m,wind_gusts_10m";
    url += "&timezone=auto&forecast_days=1";
    return url;
}

bool WeatherClient::parseCurrentWeather(const String &response, String &weatherJson, Stream &output) const
{
    JsonDocument responseDocument;
    DeserializationError error = deserializeJson(responseDocument, response);
    if (error)
    {
        output.print("Weather: response JSON kon niet gelezen worden: ");
        output.println(error.c_str());
        return false;
    }

    JsonObject current = responseDocument["current"];
    if (current.isNull())
    {
        output.println("Weather: current ontbreekt in response.");
        return false;
    }

    JsonDocument weatherDocument;
    weatherDocument["provider"] = "open-meteo";
    weatherDocument["available"] = true;
    weatherDocument["time"] = current["time"] | "";
    weatherDocument["temperature2m"] = current["temperature_2m"] | 0.0;
    weatherDocument["relativeHumidity2m"] = current["relative_humidity_2m"] | 0;
    weatherDocument["apparentTemperature"] = current["apparent_temperature"] | 0.0;
    weatherDocument["isDay"] = current["is_day"] | 0;
    weatherDocument["precipitation"] = current["precipitation"] | 0.0;
    weatherDocument["rain"] = current["rain"] | 0.0;
    weatherDocument["showers"] = current["showers"] | 0.0;
    weatherDocument["snowfall"] = current["snowfall"] | 0.0;
    weatherDocument["weatherCode"] = current["weather_code"] | 0;
    weatherDocument["cloudCover"] = current["cloud_cover"] | 0;
    weatherDocument["windSpeed10m"] = current["wind_speed_10m"] | 0.0;
    weatherDocument["windDirection10m"] = current["wind_direction_10m"] | 0;
    weatherDocument["windGusts10m"] = current["wind_gusts_10m"] | 0.0;

    JsonObject units = weatherDocument["units"].to<JsonObject>();
    JsonObject currentUnits = responseDocument["current_units"];
    units["temperature2m"] = currentUnits["temperature_2m"] | "C";
    units["relativeHumidity2m"] = currentUnits["relative_humidity_2m"] | "%";
    units["apparentTemperature"] = currentUnits["apparent_temperature"] | "C";
    units["precipitation"] = currentUnits["precipitation"] | "mm";
    units["rain"] = currentUnits["rain"] | "mm";
    units["showers"] = currentUnits["showers"] | "mm";
    units["snowfall"] = currentUnits["snowfall"] | "cm";
    units["cloudCover"] = currentUnits["cloud_cover"] | "%";
    units["windSpeed10m"] = currentUnits["wind_speed_10m"] | "km/h";
    units["windDirection10m"] = currentUnits["wind_direction_10m"] | "deg";
    units["windGusts10m"] = currentUnits["wind_gusts_10m"] | "km/h";

    serializeJson(weatherDocument, weatherJson);
    return true;
}
