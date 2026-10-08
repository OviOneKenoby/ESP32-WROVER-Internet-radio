#ifndef STATION_CODEC_H
#define STATION_CODEC_H

#include <stddef.h>

enum StationCodec {
    STATION_CODEC_MP3,
    STATION_CODEC_AAC
};

// Preserve the codec supplied by discovery/persistence.  Only upgrade an
// older MP3/default record when the URL itself unambiguously contains .aac.
// This deliberately does not infer MP3 from an opaque URL.
inline StationCodec stationCodecForURL(const char* url, StationCodec storedCodec) {
    if (!url) return storedCodec;

    static const char marker[] = ".aac";
    for (const char* start = url; *start; ++start) {
        size_t i = 0;
        while (marker[i] && start[i]) {
            char c = start[i];
            if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
            if (c != marker[i]) break;
            ++i;
        }
        if (!marker[i]) return STATION_CODEC_AAC;
    }
    return storedCodec;
}

#endif // STATION_CODEC_H
