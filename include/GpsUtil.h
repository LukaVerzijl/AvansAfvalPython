#ifndef GPS_UTIL_H
#define GPS_UTIL_H

#include <Adafruit_GPS.h>
#include <Arduino.h>
#include <Wire.h>

struct GpsCoordinates
{
    float latitude;
    float longitude;

    bool isValid() const;
};

class GpsUtil
{
public:
    explicit GpsUtil(TwoWire &wire = Wire, uint8_t i2cAddress = 0x10);

    void begin();
    bool update(Stream *gpsInput = nullptr, Print *rawOutput = nullptr);

    bool hasFix() const;
    uint8_t satelliteCount() const;
    float hdop() const;
    String lastNmea() const;
    GpsCoordinates getCoordinates() const;
    String getGoogleMapsLink() const;
    bool printGoogleMapsLink(Print &output) const;

private:
    Adafruit_GPS _gps;
    TwoWire *_wire;
    uint8_t _i2cAddress;
    String _lastNmea;
};

#endif
