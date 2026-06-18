#include "SscmaUtil.h"

SscmaUtil::SscmaUtil(TwoWire &wire, int32_t resetPin, uint16_t address)
    : _wire(&wire),
      _resetPin(resetPin),
      _address(address),
      _ready(false),
      _lastError(CMD_OK),
      _lastInvokeMillis(0)
{
}

bool SscmaUtil::begin(Stream &output)
{
    output.println("SSCMA: starten via I2C...");
    _ready = _ai.begin(_wire, _resetPin, _address);

    if (!_ready)
    {
        output.println("SSCMA: niet gevonden.");
        return false;
    }

    output.print("SSCMA: verbonden met ");
    output.print(_ai.name());
    output.print(" (");
    output.print(_ai.ID());
    output.println(")");
    return true;
}

bool SscmaUtil::begin(HardwareSerial &serial, Stream &output, uint32_t baud)
{
    output.println("SSCMA: starten via UART...");
    _ready = _ai.begin(&serial, _resetPin, baud);

    if (!_ready)
    {
        output.println("SSCMA: niet gevonden via UART.");
        return false;
    }

    output.print("SSCMA: verbonden met ");
    output.print(_ai.name());
    output.print(" (");
    output.print(_ai.ID());
    output.println(")");
    return true;
}

bool SscmaUtil::begin(HardwareSerial &serial, int8_t rxPin, int8_t txPin, Stream &output, uint32_t baud)
{
    output.print("SSCMA: starten via UART RX=");
    output.print(rxPin);
    output.print(", TX=");
    output.println(txPin);

    serial.begin(baud, SERIAL_8N1, rxPin, txPin);
    serial.setTimeout(1000);

    _ready = _ai.begin(&serial, _resetPin, baud);

    if (!_ready)
    {
        output.println("SSCMA: niet gevonden via UART.");
        return false;
    }

    output.print("SSCMA: verbonden met ");
    output.print(_ai.name());
    output.print(" (");
    output.print(_ai.ID());
    output.println(")");
    return true;
}

bool SscmaUtil::invoke(bool filter, bool showImage)
{
    if (!_ready)
    {
        _lastError = CMD_EIO;
        return false;
    }

    _lastError = _ai.invoke(1, filter, showImage);
    return _lastError == CMD_OK;
}

bool SscmaUtil::invokeEvery(uint32_t intervalMillis, Stream &output, bool filter, bool showImage)
{
    if (_lastInvokeMillis != 0 && millis() - _lastInvokeMillis < intervalMillis)
    {
        return false;
    }

    _lastInvokeMillis = millis();
    if (!invoke(filter, showImage))
    {
        if (_lastError == CMD_ETIMEDOUT && filter)
        {
            output.println("SSCMA: geen nieuwe data, zonder filter proberen...");
            if (invoke(false, showImage))
            {
                printSummary(output);
                printDetections(output);
                return true;
            }
        }

        output.print("SSCMA: invoke fout ");
        output.println(_lastError);
        return false;
    }

    printSummary(output);
    printDetections(output);
    return true;
}

bool SscmaUtil::isReady() const
{
    return _ready;
}

int SscmaUtil::lastError() const
{
    return _lastError;
}

size_t SscmaUtil::boxCount()
{
    return _ai.boxes().size();
}

size_t SscmaUtil::classCount()
{
    return _ai.classes().size();
}

size_t SscmaUtil::pointCount()
{
    return _ai.points().size();
}

size_t SscmaUtil::keypointCount()
{
    return _ai.keypoints().size();
}

boxes_t SscmaUtil::box(size_t index)
{
    return _ai.boxes()[index];
}

classes_t SscmaUtil::classification(size_t index)
{
    return _ai.classes()[index];
}

point_t SscmaUtil::point(size_t index)
{
    return _ai.points()[index];
}

perf_t SscmaUtil::performance()
{
    return _ai.perf();
}

String SscmaUtil::lastImage()
{
    return _ai.last_image();
}

void SscmaUtil::printSummary(Stream &output)
{
    perf_t perf = performance();

    output.print("SSCMA: boxes=");
    output.print(boxCount());
    output.print(", classes=");
    output.print(classCount());
    output.print(", points=");
    output.print(pointCount());
    output.print(", keypoints=");
    output.print(keypointCount());
    output.print(", perf=");
    output.print(perf.prepocess);
    output.print("/");
    output.print(perf.inference);
    output.print("/");
    output.println(perf.postprocess);
}

void SscmaUtil::printDetections(Stream &output)
{
    if (boxCount() == 0 && classCount() == 0 && pointCount() == 0 && keypointCount() == 0)
    {
        output.println("AI: geen detecties.");
        return;
    }

    output.println("AI found:");

    for (size_t i = 0; i < boxCount(); i++)
    {
        boxes_t detection = box(i);
        output.print(" - box target=");
        output.print(detection.target);
        output.print(", score=");
        output.print(detection.score);
        output.print(", x=");
        output.print(detection.x);
        output.print(", y=");
        output.print(detection.y);
        output.print(", w=");
        output.print(detection.w);
        output.print(", h=");
        output.println(detection.h);
    }

    for (size_t i = 0; i < classCount(); i++)
    {
        classes_t result = classification(i);
        output.print("Class[");
        output.print(i);
        output.print("] target=");
        output.print(result.target);
        output.print(", score=");
        output.println(result.score);
    }
}
