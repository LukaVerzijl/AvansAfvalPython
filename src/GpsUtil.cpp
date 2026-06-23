#include "GpsUtil.h"

bool GpsCoordinates::isValid() const
{
    return !(latitude == 0.0f && longitude == 0.0f);
}

GpsUtil::GpsUtil(TwoWire &wire, uint8_t i2cAddress)
    : _gps(&wire), _wire(&wire), _i2cAddress(i2cAddress)
{
}

void GpsUtil::begin()
{
    _wire->begin();
    _gps.begin(_i2cAddress);
    _gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    _gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
}

bool GpsUtil::update(Stream *gpsInput, Print *rawOutput)
{
    const uint16_t maxBytesPerUpdate = 256;
    uint16_t bytesRead = 0;

    if (gpsInput != nullptr)
    {
        while (gpsInput->available() && bytesRead < maxBytesPerUpdate)
        {
            _gps.write(gpsInput->read());
            bytesRead++;
        }
    }

    while (_gps.available() && bytesRead < maxBytesPerUpdate)
    {
        char c = _gps.read();
        if (rawOutput != nullptr && c != 0)
        {
            rawOutput->write(c);
        }
        bytesRead++;
    }

    if (!_gps.newNMEAreceived())
    {
        return false;
    }

    _lastNmea = _gps.lastNMEA();
    _lastNmea.trim();
    return _gps.parse(_gps.lastNMEA());
}

bool GpsUtil::hasFix() const
{
    return _gps.fix;
}

uint8_t GpsUtil::satelliteCount() const
{
    return _gps.satellites;
}

float GpsUtil::hdop() const
{
    return _gps.HDOP;
}

String GpsUtil::lastNmea() const
{
    return _lastNmea;
}

GpsCoordinates GpsUtil::getCoordinates() const
{
    return {_gps.latitudeDegrees, _gps.longitudeDegrees};
}

String GpsUtil::getGoogleMapsLink() const
{
    if (!hasFix())
    {
        return "";
    }

    GpsCoordinates coordinates = getCoordinates();
    if (!coordinates.isValid())
    {
        return "";
    }

    String link = "https://www.google.com/maps?q=";
    link += String(coordinates.latitude, 6);
    link += ",";
    link += String(coordinates.longitude, 6);
    return link;
}

bool GpsUtil::printGoogleMapsLink(Print &output) const
{
    String link = getGoogleMapsLink();
    if (link.length() == 0)
    {
        return false;
    }

    output.print("Google Maps: ");
    output.println(link);
    return true;
}
