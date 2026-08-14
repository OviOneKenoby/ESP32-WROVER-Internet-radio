/*
    AudioFileSourceICYSStream

    Vendored from ESP8266Audio's AudioFileSourceICYStream.cpp (Copyright
    (C) 2017 Earle F. Philhower, III, GPLv3). ICY metadata parsing logic
    (readInternal - partial-read handling, StreamTitle extraction) is the
    library's original implementation, unmodified. Only change: builds on
    top of AudioFileSourceHTTPSStream (WiFiClientSecure) instead of the
    plain-HTTP AudioFileSourceHTTPStream, and the ESP32/ESP8266 #ifdef
    branches were removed since this project only targets ESP32.
*/

#ifdef _GNU_SOURCE
#undef _GNU_SOURCE
#endif
#define _GNU_SOURCE

#include "AudioFileSourceICYSStream.h"
#include <string.h>

AudioFileSourceICYSStream::AudioFileSourceICYSStream() {
    pos = 0;
    reconnectTries = 0;
    saveURL[0] = 0;
}

AudioFileSourceICYSStream::AudioFileSourceICYSStream(const char *url) {
    saveURL[0] = 0;
    reconnectTries = 0;
    open(url);
}

bool AudioFileSourceICYSStream::open(const char *url) {
    static const char *hdr[] = { "icy-metaint", "icy-name", "icy-genre", "icy-br" };
    pos = 0;
    client.setInsecure();
    http.begin(client, url);
    // This source reads directly from HTTPClient's underlying stream.
    // Europe 1's redirected HTTPS endpoint uses HTTP/1.1 chunked transfer
    // encoding, whose chunk framing is not audio data. Request HTTP/1.0 so
    // the server sends the continuous raw AAC/ICY body this reader expects.
    http.useHTTP10(true);
    // See AudioFileSourceHTTPSStream::open() - same deliberate addition,
    // same reasoning (bounds how long a slow/unreachable station can
    // freeze the whole UI, since this call is synchronous).
    http.setConnectTimeout(3000);
    http.setTimeout(3000);
    http.addHeader("Icy-MetaData", "1");
    http.collectHeaders(hdr, 4);
    http.setReuse(true);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        http.end();
        cb.st(STATUS_HTTPFAIL, PSTR("Can't open HTTPS request"));
        return false;
    }
    if (http.hasHeader(hdr[0])) {
        String ret = http.header(hdr[0]);
        icyMetaInt = ret.toInt();
    } else {
        icyMetaInt = 0;
    }

    icyByteCount = 0;
    size = http.getSize();
    strncpy(saveURL, url, sizeof(saveURL));
    saveURL[sizeof(saveURL) - 1] = 0;
    return true;
}

AudioFileSourceICYSStream::~AudioFileSourceICYSStream() {
    http.end();
}

uint32_t AudioFileSourceICYSStream::readInternal(void *data, uint32_t len, bool nonBlock) {
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
            // This is going to get ugly fast.
            char icyBuff[256 + 16 + 1];
            char *readInto = icyBuff + 16;
            memset(icyBuff, 0, 16); // Ensure no residual matches occur
            while (mdSize) {
                int toRead = mdSize > 256 ? 256 : mdSize;
                int ret = stream->read((uint8_t*)readInto, toRead);
                if (ret < 0) {
                    return read;
                }
                if (ret == 0) {
                    delay(1);
                    continue;
                }
                mdSize -= ret;
                // At this point we have 0...15 = last 15 chars read from prior read plus new data
                int end = 16 + ret; // The last byte of valid data
                char *header = (char *)memmem((void*)icyBuff, end, (void*)"StreamTitle=", 12);
                if (!header) {
                    // No match, so move the last 16 bytes back to the start and continue
                    memmove(icyBuff, icyBuff + end - 16, 16);
                    delay(1);
                    continue;
                }
                // Found header, now move it to the front
                int lastValidByte = end - (header - icyBuff) + 1;
                memmove(icyBuff, header, lastValidByte);
                // Now fill the buffer to the end with read data
                while (mdSize && lastValidByte < 255) {
                    int toRead = mdSize > (256 - lastValidByte) ? (256 - lastValidByte) : mdSize;
                    ret = stream->read((uint8_t*)icyBuff + lastValidByte, toRead);
                    if (ret == -1) {
                        return read;    // error
                    }
                    if (ret == 0) {
                        delay(1);
                        continue;
                    }
                    mdSize -= ret;
                    lastValidByte += ret;
                }
                // Buffer now contains StreamTitle=....., parse it
                char *p = icyBuff + 12;
                if (*p == '\'' || *p == '"') {
                    char closing[] = { *p, ';', '\0' };
                    char *psz = strstr(p + 1, closing);
                    if (!psz) {
                        psz = strchr(&icyBuff[13], ';');
                    }
                    if (psz) {
                        *psz = '\0';
                    }
                    p++;
                } else {
                    char *psz = strchr(p, ';');
                    if (psz) {
                        *psz = '\0';
                    }
                }
                cb.md("StreamTitle", false, p);

                // Now skip rest of MD block
                while (mdSize) {
                    int toRead = mdSize > 256 ? 256 : mdSize;
                    ret = stream->read((uint8_t*)icyBuff, toRead);
                    if (ret < 0) {
                        return read;
                    }
                    if (ret == 0) {
                        delay(1);
                        continue;
                    }
                    mdSize -= ret;
                }
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
