#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <Arduino.h>
#include <WiFi.h>

class WifiModule
{
public:
    WifiModule(const char *ssid, const char *password);

    bool begin(Stream &output = Serial, uint32_t timeoutMillis = 15000);
    bool isConnected() const;
    void printStatus(Stream &output = Serial) const;

private:
    const char *_ssid;
    const char *_password;
};

#endif
