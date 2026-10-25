#ifndef STATIONS_H
#define STATIONS_H

#include <Arduino.h>
#include "config.h"
#include "station_codec.h"

struct RadioStation {
    char name[MAX_NAME_LENGTH];
    char url[MAX_URL_LENGTH];
    StationCodec codec;
};

class StationManager {
public:
    StationManager();
    ~StationManager();

    // Allocate the fixed station/favorite/recent record block in PSRAM.
    // Must be called once from setup() before any persistence load or record
    // access. Failure is fail-closed: no internal-memory fallback is used.
    bool begin();
    bool isReady() const { return recordStorage != nullptr; }
    uint64_t getCatalogRevision() const { return catalogRevision; }
    
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
    static constexpr size_t RECORD_STORAGE_COUNT =
        MAX_STATIONS + MAX_RECENT + MAX_FAVORITES;

    RadioStation* stationRecords() { return recordStorage; }
    RadioStation* recentRecords() { return recordStorage + MAX_STATIONS; }
    RadioStation* favoriteRecords() {
        return recordStorage + MAX_STATIONS + MAX_RECENT;
    }

    RadioStation* recordStorage;
    uint8_t stationCount;
    uint8_t currentStation;

    uint8_t recentCount;
    void saveRecentToPrefs();

    uint8_t favoriteCount;
    uint64_t catalogRevision;
    void saveFavoritesToPrefs();
    void advanceCatalogRevision();

    // User-managed station list, persisted separately from Favorites and
    // Recent so the web UI can safely manage it through NVS.
    void saveStationsToPrefs();
};

extern StationManager stationManager;

#endif // STATIONS_H
