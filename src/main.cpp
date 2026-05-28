#include <Adafruit_GPS.h>
#include <Arduino.h>
Adafruit_GPS GPS(&Wire);

uint32_t lastMapsLinkMillis = 0;

bool printGoogleMapsLink()
{
    if (!GPS.fix)
        return false;

    float latitude = GPS.latitudeDegrees;
    float longitude = GPS.longitudeDegrees;

    if (latitude == 0.0f && longitude == 0.0f)
        return false;

    Serial.print("Google Maps: https://www.google.com/maps?q=");
    Serial.print(latitude, 6);
    Serial.print(",");
    Serial.println(longitude, 6);

    return true;
}

void setup()
{
    // put your setup code here, to run once:

    Serial.begin(115200);
    GPS.begin(0x10);
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
}

void loop()
{
    // put your main code here, to run repeatedly:
    if (Serial.available())
    {
        char c = Serial.read();
        GPS.write(c);
    }
    if (GPS.available())
    {
        char c = GPS.read();
        if (c != 0)
            Serial.write(c);
    }

    if (GPS.newNMEAreceived())
    {
        if (!GPS.parse(GPS.lastNMEA()))
            return;

        if (lastMapsLinkMillis == 0 || millis() - lastMapsLinkMillis >= 5000)
        {
            if (printGoogleMapsLink())
                lastMapsLinkMillis = millis();
        }
    }
}
