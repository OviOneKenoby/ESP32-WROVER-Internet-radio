/*
    AudioFileSourcePlainICYStream

    Vendored from ESP8266Audio's AudioFileSourceHTTPStream and
    AudioFileSourceICYStream (Copyright (C) 2017 Earle F. Philhower, III,
    GPLv3), merged into one class that derives from AudioFileSource directly.
    The library classes keep their HTTPClient and counters private and only
    befriend each other, so a corrected metadata reader cannot be added to them
    from outside; this class therefore carries the same members and the same
    behaviour (HTTP request, headers, reconnect logic, read sizes, the 3000 ms
    wait, partial-read handling) and differs only in that the metadata block is
    read by icy_metadata::consume(), which never reads or writes outside its
    buffer. Plain http:// streams use this class; https:// streams use
    AudioFileSourceICYSStream.
*/

#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include "AudioFileSource.h"

class AudioFileSourcePlainICYStream : public AudioFileSource {
public:
    AudioFileSourcePlainICYStream();
    AudioFileSourcePlainICYStream(const char *url);
    virtual ~AudioFileSourcePlainICYStream() override;

    virtual bool open(const char *url) override;
    virtual uint32_t read(void *data, uint32_t len) override;
    virtual uint32_t readNonBlock(void *data, uint32_t len) override;
    virtual bool seek(int32_t pos, int dir) override;
    virtual bool close() override;
    virtual bool isOpen() override;
    virtual uint32_t getSize() override;
    virtual uint32_t getPos() override;
    bool SetReconnect(int tries, int delayms) {
        reconnectTries = tries;
        reconnectDelayMs = delayms;
        return true;
    }

    enum { STATUS_HTTPFAIL = 2, STATUS_DISCONNECTED, STATUS_RECONNECTING, STATUS_RECONNECTED, STATUS_NODATA };

private:
    uint32_t readInternal(void *data, uint32_t len, bool nonBlock);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    NetworkClient client;
#else
    WiFiClient client;
#endif
    HTTPClient http;
    int pos = 0;
    int size = 0;
    int reconnectTries = 0;
    int reconnectDelayMs = 0;
    char saveURL[128];
    int icyMetaInt = 0;
    int icyByteCount = 0;
};
