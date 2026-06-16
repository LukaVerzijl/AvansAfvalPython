#include "WifiModule.h"

WifiModule::WifiModule(const char *ssid, const char *password)
    : _ssid(ssid), _password(password), _lastStatusMillis(0)
{
}

bool WifiModule::begin(Stream &output, uint32_t timeoutMillis)
{
    output.println("WiFi: starten verbinding...");
    if (_ssid == nullptr || _ssid[0] == '\0')
    {
        output.println("WiFi: geen SSID ingesteld, verbinden overgeslagen.");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

    output.print("WiFi: verbinden met ");
    output.print(_ssid);

    uint32_t startMillis = millis();
    while (!isConnected() && millis() - startMillis < timeoutMillis)
    {
        output.print(".");
        delay(500);
    }
    output.println();

    printStatus(output);
    return isConnected();
}

bool WifiModule::isConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}

void WifiModule::printStatus(Stream &output) const
{
    if (!isConnected())
    {
        output.println("WiFi: niet verbonden.");
        return;
    }

    output.print("WiFi: verbonden, IP ");
    output.println(WiFi.localIP());
}

void WifiModule::printStatusEvery(Stream &output, uint32_t intervalMillis)
{
    if (_lastStatusMillis == 0 || millis() - _lastStatusMillis >= intervalMillis)
    {
        printStatus(output);
        _lastStatusMillis = millis();
    }
}
