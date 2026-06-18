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
    output.println("SSCMA: check detecties...");

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

String SscmaUtil::info(bool refresh)
{
    return _ai.info(!refresh);
}

void SscmaUtil::printInfo(Stream &output)
{
    String deviceInfo = info(true);
    if (deviceInfo.length() == 0)
    {
        output.println("SSCMA info: leeg of niet beschikbaar.");
        return;
    }

    output.print("SSCMA info: ");
    output.println(deviceInfo);
}

bool SscmaUtil::configureScoreThreshold(uint8_t score, Stream &output)
{
    if (!_ready)
    {
        return false;
    }

    output.print("SSCMA: confidence threshold instellen op ");
    output.println(score);

    printRawCommand("TSCORE?", output, 1500);
    printRawCommand("INVOKE=-1,0,1", output, 2000);
    delay(100);

    String thresholdCommand = "TSCORE=";
    thresholdCommand += String(score);
    bool configured = printRawCommand(thresholdCommand.c_str(), output, 2000);

    printRawCommand("TSCORE?", output, 1500);
    printRawCommand("BREAK", output, 2000);

    return configured;
}

bool SscmaUtil::printRawCommand(const char *command, Stream &output, uint32_t timeoutMillis)
{
    if (!_ready)
    {
        return false;
    }

    output.print("SSCMA raw command: ");
    output.println(command);

    String fullCommand = "AT+";
    fullCommand += command;
    fullCommand += "\r\n";
    _ai.write(fullCommand.c_str(), fullCommand.length());

    bool received = false;
    uint32_t startMillis = millis();
    while (!received && millis() - startMillis < timeoutMillis)
    {
        _ai.fetch([&output, &received](const char *response, size_t length)
                  {
                      output.print("SSCMA raw response: ");
                      output.write(reinterpret_cast<const uint8_t *>(response), length);
                      output.println();
                      received = true;
                  });
        delay(10);
    }

    if (!received)
    {
        output.println("SSCMA raw response: timeout");
    }

    return received;
}

bool SscmaUtil::printRawInvoke(Stream &output, uint32_t timeoutMillis)
{
    if (!_ready)
    {
        return false;
    }

    output.println("SSCMA raw invoke: INVOKE=1,0,1");
    _ai.write("AT+INVOKE=1,0,1\r\n", strlen("AT+INVOKE=1,0,1\r\n"));

    bool received = false;
    uint32_t startMillis = millis();
    while (millis() - startMillis < timeoutMillis)
    {
        _ai.fetch([&output, &received](const char *response, size_t length)
                  {
                      output.print("SSCMA raw invoke response: ");
                      output.write(reinterpret_cast<const uint8_t *>(response), length);
                      output.println();
                      received = true;
                  });
        delay(10);
    }

    if (!received)
    {
        output.println("SSCMA raw invoke response: timeout");
    }

    return received;
}

void SscmaUtil::printRawDiagnostics(Stream &output)
{
    printRawCommand("INFO?", output);
    printRawCommand("MODEL?", output);
    printRawCommand("MODELS?", output);
    printRawCommand("ALGOS?", output);
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

void SscmaUtil::printDetections(Stream &output, uint8_t minimumScore)
{
    bool hasDetection = false;
    for (size_t i = 0; i < boxCount(); i++)
    {
        if (box(i).score >= minimumScore)
        {
            hasDetection = true;
            break;
        }
    }

    for (size_t i = 0; i < classCount() && !hasDetection; i++)
    {
        if (classification(i).score >= minimumScore)
        {
            hasDetection = true;
        }
    }

    if (!hasDetection)
    {
        output.print("AI: geen detecties boven confidence ");
        output.print(minimumScore);
        output.println(".");
        return;
    }

    output.println("AI found:");

    for (size_t i = 0; i < boxCount(); i++)
    {
        boxes_t detection = box(i);
        if (detection.score < minimumScore)
        {
            continue;
        }

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
        if (result.score < minimumScore)
        {
            continue;
        }

        output.print("Class[");
        output.print(i);
        output.print("] target=");
        output.print(result.target);
        output.print(", score=");
        output.println(result.score);
    }
}
