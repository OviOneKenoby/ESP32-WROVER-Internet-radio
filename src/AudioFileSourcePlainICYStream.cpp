/*
    AudioFileSourcePlainICYStream
    Streaming Shoutcast ICY source over plain HTTP

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

    Vendored from ESP8266Audio 2.2.0 (AudioFileSourceHTTPStream.cpp and
    AudioFileSourceICYStream.cpp). See the header for what differs.
*/

#include "AudioFileSourcePlainICYStream.h"
#include "icy_metadata.h"
#include <string.h>

AudioFileSourcePlainICYStream::AudioFileSourcePlainICYStream() {
    pos = 0;
    reconnectTries = 0;
    saveURL[0] = 0;
}

AudioFileSourcePlainICYStream::AudioFileSourcePlainICYStream(const char *url) {
    saveURL[0] = 0;
    reconnectTries = 0;
    open(url);
}

bool AudioFileSourcePlainICYStream::open(const char *url) {
    static const char *hdr[] = { "icy-metaint", "icy-name", "icy-genre", "icy-br" };
    pos = 0;
    http.begin(client, url);
    http.addHeader("Icy-MetaData", "1");
    http.collectHeaders(hdr, 4);
    http.setReuse(true);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        http.end();
        cb.st(STATUS_HTTPFAIL, PSTR("Can't open HTTP request"));
        return false;
    }
    if (http.hasHeader(hdr[0])) {
        String ret = http.header(hdr[0]);
        icyMetaInt = ret.toInt();
    } else {
        icyMetaInt = 0;
    }
    if (http.hasHeader(hdr[1])) {
        String ret = http.header(hdr[1]);
        //    cb.md("SiteName", false, ret.c_str());
    }
    if (http.hasHeader(hdr[2])) {
        String ret = http.header(hdr[2]);
        //    cb.md("Genre", false, ret.c_str());
    }
    if (http.hasHeader(hdr[3])) {
        String ret = http.header(hdr[3]);
        //    cb.md("Bitrate", false, ret.c_str());
    }

    icyByteCount = 0;
    size = http.getSize();
    strncpy(saveURL, url, sizeof(saveURL));
    saveURL[sizeof(saveURL) - 1] = 0;
    return true;
}

AudioFileSourcePlainICYStream::~AudioFileSourcePlainICYStream() {
    http.end();
}

uint32_t AudioFileSourcePlainICYStream::read(void *data, uint32_t len) {
    if (data == NULL) {
        audioLogger->printf_P(PSTR("ERROR! AudioFileSourcePlainICYStream::read passed NULL data\n"));
        return 0;
    }
    return readInternal(data, len, false);
}

uint32_t AudioFileSourcePlainICYStream::readNonBlock(void *data, uint32_t len) {
    if (data == NULL) {
        audioLogger->printf_P(PSTR("ERROR! AudioFileSourcePlainICYStream::readNonBlock passed NULL data\n"));
        return 0;
    }
    return readInternal(data, len, true);
}

uint32_t AudioFileSourcePlainICYStream::readInternal(void *data, uint32_t len, bool nonBlock) {
    // Ensure we can't possibly read 2 ICY headers in a single go #355
    if (icyMetaInt > 1) {
        len = std::min((int)(icyMetaInt >> 1), (int)len);
    }
retry:
    if (!http.connected()) {
        cb.st(STATUS_DISCONNECTED, PSTR("Stream disconnected"));
        http.end();
        for (int i = 0; i < reconnectTries; i++) {
            char buff[64];
            sprintf_P(buff, PSTR("Attempting to reconnect, try %d"), i);
            cb.st(STATUS_RECONNECTING, buff);
            delay(reconnectDelayMs);
            if (open(saveURL)) {
                cb.st(STATUS_RECONNECTED, PSTR("Stream reconnected"));
                break;
            }
        }
        if (!http.connected()) {
            cb.st(STATUS_DISCONNECTED, PSTR("Unable to reconnect"));
            return 0;
        }
    }
    if ((size > 0) && (pos >= size)) {
        return 0;
    }

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    NetworkClient *stream = http.getStreamPtr();
#else
    WiFiClient *stream = http.getStreamPtr();
#endif

    // Can't read past EOF...
    if ((size > 0) && (len > (uint32_t)(pos - size))) {
        len = pos - size;
    }

    if (!nonBlock) {
        int start = millis();
        while ((stream->available() < (int)len) && (millis() - start < 3000)) {
            yield();
        }
    }

    size_t avail = stream->available();
    if (!nonBlock && !avail) {
        cb.st(STATUS_NODATA, PSTR("No stream data available"));
        http.end();
        goto retry;
    }
    if (avail == 0) {
        return 0;
    }
    if (avail < len) {
        len = avail;
    }

    int read = 0;
    int ret = 0;
    // If the read would hit an ICY block, split it up...
    if (((int)(icyByteCount + len) > (int)icyMetaInt) && (icyMetaInt > 0)) {
        int beforeIcy = icyMetaInt - icyByteCount;
        if (beforeIcy > 0) {
            ret = stream->read(reinterpret_cast<uint8_t*>(data), beforeIcy);
            if (ret < 0) {
                ret = 0;
            }
            read += ret;
            pos += ret;
            len -= ret;
            data = (void *)(reinterpret_cast<char*>(data) + ret);
            icyByteCount += ret;
            if (ret != beforeIcy) {
                return read;    // Partial read
            }
        }

        // ICY MD handling
        int mdSize;
        uint8_t c;
        int mdret = stream->read(&c, 1);
        if (mdret == 0) {
            return read;
        }
        mdSize = c * 16;
        if ((mdret == 1) && (mdSize > 0)) {
            // The block is read, bounded and handed to the callback by
            // icy_metadata::consume(); a stream error ends this read as before.
            const bool ok = icy_metadata::consume(
                *stream, mdSize,
                [this](const char *title) { cb.md("StreamTitle", false, title); },
                []() { delay(1); });
            if (!ok) {
                return read;
            }
        }
        icyByteCount = 0;
    }

    ret = stream->read(reinterpret_cast<uint8_t*>(data), len);
    if (ret < 0) {
        ret = 0;
    }
    read += ret;
    pos += ret;
    icyByteCount += ret;
    return read;
}

bool AudioFileSourcePlainICYStream::seek(int32_t pos, int dir) {
    audioLogger->printf_P(PSTR("ERROR! AudioFileSourcePlainICYStream::seek not implemented!"));
    (void) pos;
    (void) dir;
    return false;
}

bool AudioFileSourcePlainICYStream::close() {
    http.end();
    return true;
}

bool AudioFileSourcePlainICYStream::isOpen() {
    return http.connected();
}

uint32_t AudioFileSourcePlainICYStream::getSize() {
    return size;
}

uint32_t AudioFileSourcePlainICYStream::getPos() {
    return pos;
}
