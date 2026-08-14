/*
    AudioFileSourceHTTPSStream

    Vendored from ESP8266Audio's AudioFileSourceHTTPStream (Copyright (C)
    2017 Earle F. Philhower, III, GPLv3), with the single change of using
    WiFiClientSecure instead of NetworkClient/WiFiClient, so it can connect
    to HTTPS streaming servers. Everything else - reconnect logic, partial
    read handling, HTTPClient usage - is the library's original, well-tested
    implementation, unmodified. Vendored (rather than patched in the
    upstream git dependency) so the pinned library commit stays untouched
    and this addition is clearly scoped and reviewable on its own.

    Certificate validation is skipped (setInsecure()) since managing CA
    certificates for arbitrary, unknown streaming servers isn't practical
    here - standard, accepted trade-off for this kind of project, but
    worth knowing: this does not protect against a spoofed/MITM server.
*/

#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "AudioFileSource.h"

class AudioFileSourceHTTPSStream : public AudioFileSource {
    friend class AudioFileSourceICYSStream;

public:
    AudioFileSourceHTTPSStream();
    AudioFileSourceHTTPSStream(const char *url);
    virtual ~AudioFileSourceHTTPSStream() override;

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
    void useHTTP10() {
        http.useHTTP10(true);
    }

    enum { STATUS_HTTPFAIL = 2, STATUS_DISCONNECTED, STATUS_RECONNECTING, STATUS_RECONNECTED, STATUS_NODATA };

private:
    virtual uint32_t readInternal(void *data, uint32_t len, bool nonBlock);
    WiFiClientSecure client; // confirmed by direct compiler error on the
                              // actual build that this exact framework
                              // version doesn't recognize
                              // NetworkClient/NetworkClientSecure at all -
                              // it uses the classic WiFiClient naming
    HTTPClient http;
    int pos;
    int size;
    int reconnectTries;
    int reconnectDelayMs;
    char saveURL[128];
};
