#ifndef STATIONS_H
#define STATIONS_H

#include <Arduino.h>
#include "config.h"

enum StationCodec {
    STATION_CODEC_MP3,
    STATION_CODEC_AAC
};

struct RadioStation {
    char name[MAX_NAME_LENGTH];
    char url[MAX_URL_LENGTH];
    StationCodec codec;
};

class StationManager {
public:
    StationManager();
    ~StationManager();
    
    // Station management
    uint8_t addStation(const char* name, const char* url, StationCodec codec = STATION_CODEC_MP3);
    bool removeStation(uint8_t idx);
    
    // Station queries
    uint8_t getStationCount() { return stationCount; }
    RadioStation* getStation(uint8_t idx);
    const char* getStationName(uint8_t idx);
    const char* getStationURL(uint8_t idx);
    StationCodec getStationCodec(uint8_t idx);
    
    // Current station
    void setCurrentStation(uint8_t idx);
    uint8_t getCurrentStation() { return currentStation; }
    
    // Default stations
    bool loadDefaultStations();
    
    // Recent (last MAX_RECENT played) and Favorites (user-curated) -
    // persisted across reboots via Preferences (NVS), not just in-RAM.
    // Reuses the same RadioStation shape rather than a separate struct.
    void loadPersistedLists(); // call once at boot
    
    void addToRecent(const char* name, const char* url, StationCodec codec);
    uint8_t getRecentCount() { return recentCount; }
    RadioStation* getRecent(uint8_t idx);
    
    bool addToFavorites(const char* name, const char* url, StationCodec codec);
    bool removeFavorite(uint8_t idx);
    uint8_t getFavoriteCount() { return favoriteCount; }
    RadioStation* getFavorite(uint8_t idx);
    bool isFavorite(const char* url); // for showing "already a favorite"
                                        // in the UI rather than letting
                                        // duplicates pile up
    
private:
    RadioStation stations[MAX_STATIONS];
    uint8_t stationCount;
    uint8_t currentStation;
    
    RadioStation recent[MAX_RECENT];
    uint8_t recentCount;
    void saveRecentToPrefs();
    
    RadioStation favorites[MAX_FAVORITES];
    uint8_t favoriteCount;
    void saveFavoritesToPrefs();
};

extern StationManager stationManager;

#endif // STATIONS_H
