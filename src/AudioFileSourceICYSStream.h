/*
    AudioFileSourceHTTPStream
    Connect to a HTTP based streaming service

    Copyright (C) 2017  Earle F. Philhower, III

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// This file was modified from the upstream file above; the changes are described in the next comment.
/*
    AudioFileSourceICYSStream

    Vendored from ESP8266Audio's AudioFileSourceICYStream (Copyright (C)
    2017 Earle F. Philhower, III, GPLv3), retargeted to inherit from
    AudioFileSourceHTTPSStream (this project's vendored HTTPS variant)
    instead of the library's plain-HTTP AudioFileSourceHTTPStream. The
    metadata block is read by icy_metadata::consume() (bounded and
    NUL-terminated); the rest of readInternal() is the library's original.
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
