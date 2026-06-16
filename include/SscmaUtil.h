#ifndef SSCMA_UTIL_H
#define SSCMA_UTIL_H

#include <Arduino.h>
#include <Seeed_Arduino_SSCMA.h>
#include <Wire.h>

class SscmaUtil
{
public:
    explicit SscmaUtil(TwoWire &wire = Wire, int32_t resetPin = -1, uint16_t address = I2C_ADDRESS);

    bool begin(Stream &output = Serial);
    bool begin(HardwareSerial &serial, Stream &output = Serial, uint32_t baud = SSCMA_UART_BAUD);
    bool invoke(bool filter = false, bool showImage = false);
    bool invokeEvery(uint32_t intervalMillis, Stream &output = Serial, bool filter = false, bool showImage = false);

    bool isReady() const;
    int lastError() const;

    size_t boxCount();
    size_t classCount();
    size_t pointCount();
    size_t keypointCount();

    boxes_t box(size_t index);
    classes_t classification(size_t index);
    point_t point(size_t index);
    perf_t performance();
    String lastImage();

    void printSummary(Stream &output = Serial);
    void printDetections(Stream &output = Serial);

private:
    SSCMA _ai;
    TwoWire *_wire;
    int32_t _resetPin;
    uint16_t _address;
    bool _ready;
    int _lastError;
    uint32_t _lastInvokeMillis;
};

#endif
