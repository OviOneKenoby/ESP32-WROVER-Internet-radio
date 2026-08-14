/*
    AudioFileSourceHTTPSStream

    Vendored from ESP8266Audio's AudioFileSourceHTTPStream.cpp (Copyright
    (C) 2017 Earle F. Philhower, III, GPLv3). Logic is unchanged from the
    original except: WiFiClientSecure member instead of NetworkClient, a
    client.setInsecure() call before connecting (skips certificate
    validation - see header comment), and the ESP32/ESP8266 #ifdef
    branches removed since this project only targets ESP32.
*/

#include "AudioFileSourceHTTPSStream.h"

AudioFileSourceHTTPSStream::AudioFileSourceHTTPSStream() {
    pos = 0;
    reconnectTries = 0;
    saveURL[0] = 0;
}

AudioFileSourceHTTPSStream::AudioFileSourceHTTPSStream(const char *url) {
    saveURL[0] = 0;
    reconnectTries = 0;
    open(url);
}

bool AudioFileSourceHTTPSStream::open(const char *url) {
    pos = 0;
    client.setInsecure();
    http.begin(client, url);
    // Shorter than the library's original 5s default, added deliberately
    // (not part of the original vendored code) - this call is
    // synchronous on the main loop, so a slow/unreachable station
    // freezes all input (encoder, serial) for as long as it takes.
    http.setConnectTimeout(3000);
    http.setTimeout(3000);
    http.setReuse(true);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        http.end();
        cb.st(STATUS_HTTPFAIL, PSTR("Can't open HTTPS request"));
        return false;
    }
    size = http.getSize();
    strncpy(saveURL, url, sizeof(saveURL));
    saveURL[sizeof(saveURL) - 1] = 0;
    return true;
}

AudioFileSourceHTTPSStream::~AudioFileSourceHTTPSStream() {
    http.end();
}

uint32_t AudioFileSourceHTTPSStream::read(void *data, uint32_t len) {
    if (data == NULL) {
        audioLogger->printf_P(PSTR("ERROR! AudioFileSourceHTTPSStream::read passed NULL data\n"));
        return 0;
    }
    return readInternal(data, len, false);
}

uint32_t AudioFileSourceHTTPSStream::readNonBlock(void *data, uint32_t len) {
    if (data == NULL) {
        audioLogger->printf_P(PSTR("ERROR! AudioFileSourceHTTPSStream::readNonBlock passed NULL data\n"));
        return 0;
    }
    return readInternal(data, len, true);
}

uint32_t AudioFileSourceHTTPSStream::readInternal(void *data, uint32_t len, bool nonBlock) {
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

    auto *stream = http.getStreamPtr();

    // Can't read past EOF...
    if ((size > 0) && (len > (uint32_t)(pos - size))) {
        len = pos - size;
    }

    if (!nonBlock) {
        int start = millis();
        while ((stream->available() < (int)len) && (millis() - start < 500)) {
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

    int read = stream->read(reinterpret_cast<uint8_t*>(data), len);
    pos += read;
    return read;
}

bool AudioFileSourceHTTPSStream::seek(int32_t pos, int dir) {
    audioLogger->printf_P(PSTR("ERROR! AudioFileSourceHTTPSStream::seek not implemented!"));
    (void) pos;
    (void) dir;
    return false;
}

bool AudioFileSourceHTTPSStream::close() {
    http.end();
    return true;
}

bool AudioFileSourceHTTPSStream::isOpen() {
    return http.connected();
}

uint32_t AudioFileSourceHTTPSStream::getSize() {
    return size;
}

uint32_t AudioFileSourceHTTPSStream::getPos() {
    return pos;
}
