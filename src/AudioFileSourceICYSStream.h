/*
    AudioFileSourceICYSStream

    Vendored from ESP8266Audio's AudioFileSourceICYStream (Copyright (C)
    2017 Earle F. Philhower, III, GPLv3), retargeted to inherit from
    AudioFileSourceHTTPSStream (this project's vendored HTTPS variant)
    instead of the library's plain-HTTP AudioFileSourceHTTPStream. ICY
    metadata parsing logic itself (readInternal()) is the library's
    original, unmodified implementation.
*/

#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include "AudioFileSourceHTTPSStream.h"

class AudioFileSourceICYSStream : public AudioFileSourceHTTPSStream {
public:
    AudioFileSourceICYSStream();
    AudioFileSourceICYSStream(const char *url);
    virtual ~AudioFileSourceICYSStream() override;

    virtual bool open(const char *url) override;

private:
    virtual uint32_t readInternal(void *data, uint32_t len, bool nonBlock) override;
    int icyMetaInt;
    int icyByteCount;
};
