#ifndef CURRENT_STATION_CONTEXT_H
#define CURRENT_STATION_CONTEXT_H

#include "station_codec.h"

struct CurrentStationContext {
    StationCodec codec = STATION_CODEC_MP3;
    bool discovered = false;

    void set(StationCodec stationCodec, bool isDiscovered) {
        codec = stationCodec;
        discovered = isDiscovered;
    }
};

#endif // CURRENT_STATION_CONTEXT_H
