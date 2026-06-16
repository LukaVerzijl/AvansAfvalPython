#include "GpsUtil.h"

bool GpsCoordinates::isValid() const
{
    return !(latitude == 0.0f && longitude == 0.0f);
}

GpsUtil::GpsUtil(TwoWire &wire, uint8_t i2cAddress)
    : _gps(&wire), _i2cAddress(i2cAddress)
{
}

void GpsUtil::begin()
{
    _gps.begin(_i2cAddress);
    _gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    _gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
}

bool GpsUtil::update(Stream *gpsInput, Print *rawOutput)
{
    if (gpsInput != nullptr)
    {
        while (gpsInput->available())
        {
            _gps.write(gpsInput->read());
        }
    }

    while (_gps.available())
    {
        char c = _gps.read();
        if (rawOutput != nullptr && c != 0)
        {
            rawOutput->write(c);
        }
    }

    if (!_gps.newNMEAreceived())
    {
        return false;
    }

    return _gps.parse(_gps.lastNMEA());
}

bool GpsUtil::hasFix() const
{
    return _gps.fix;
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
