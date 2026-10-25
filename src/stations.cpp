#include "stations.h"
#include "config.h"
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <new>
#include <type_traits>

static_assert(std::is_trivially_copyable<RadioStation>::value,
              "RadioStation shifting relies on trivial copy semantics");
static_assert(std::is_trivially_destructible<RadioStation>::value,
              "StationManager owns a fixed raw-storage record block");

// Global station manager
StationManager stationManager;

// ============================================
// Constructor
// ============================================
StationManager::StationManager()
    : recordStorage(nullptr),
      stationCount(0),
      currentStation(0),
      recentCount(0),
      favoriteCount(0),
      catalogRevision(1) {
}

void StationManager::advanceCatalogRevision() {
    if (catalogRevision != UINT64_MAX) ++catalogRevision;
}

StationManager::~StationManager() {
    if (!recordStorage) return;
    for (size_t i = 0; i < RECORD_STORAGE_COUNT; ++i) {
        recordStorage[i].~RadioStation();
    }
    heap_caps_free(recordStorage);
    recordStorage = nullptr;
}

bool StationManager::begin() {
    if (recordStorage) return true;

    constexpr size_t storageBytes =
        RECORD_STORAGE_COUNT * sizeof(RadioStation);
    void* raw = heap_caps_malloc(
        storageBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!raw) {
        Serial.printf("[STATION] ERROR: PSRAM record allocation failed (%u bytes)\n",
                      static_cast<unsigned>(storageBytes));
        return false;
    }

    RadioStation* records = static_cast<RadioStation*>(raw);
    for (size_t i = 0; i < RECORD_STORAGE_COUNT; ++i) {
        ::new (static_cast<void*>(&records[i])) RadioStation{};
    }
    recordStorage = records;
    Serial.printf("[STATION] PSRAM record storage ready: %u records, %u bytes\n",
                  static_cast<unsigned>(RECORD_STORAGE_COUNT),
                  static_cast<unsigned>(storageBytes));
    return true;
}

// ============================================
// Station Management
// ============================================
uint8_t StationManager::addStation(const char* name, const char* url, StationCodec codec) {
    if (!recordStorage) {
        Serial.println("[STATION] Record storage unavailable");
        return 0xFF;
    }
    if (stationCount >= MAX_STATIONS) {
        Serial.println("[STATION] Max stations reached");
        return 0xFF;
    }
    
    if (!name || !url) {
        Serial.println("[STATION] Invalid parameters");
        return 0xFF;
    }
    
    RadioStation& station = stationRecords()[stationCount];
    strncpy(station.name, name, MAX_NAME_LENGTH - 1);
    strncpy(station.url, url, MAX_URL_LENGTH - 1);
    station.codec = codec;
    
    Serial.printf("[STATION] Added: %s -> %s (%s)\n", name, url,
                  codec == STATION_CODEC_AAC ? "AAC" : "MP3");
    uint8_t addedIndex = stationCount++;
    advanceCatalogRevision();
    saveStationsToPrefs();
    return addedIndex;
}

bool StationManager::removeStation(uint8_t idx) {
    if (!recordStorage || idx >= stationCount) {
        Serial.println("[STATION] Invalid index");
        return false;
    }
    
    // Shift remaining stations
    for (uint8_t i = idx; i < stationCount - 1; i++) {
        memcpy(&stationRecords()[i], &stationRecords()[i + 1], sizeof(RadioStation));
    }
    
    stationCount--;
    advanceCatalogRevision();
    
    if (currentStation >= stationCount) {
        currentStation = 0;
    }
    
    Serial.printf("[STATION] Removed station at index %d\n", idx);
    saveStationsToPrefs();
    return true;
}

// ============================================
// Station Queries
// ============================================
RadioStation* StationManager::getStation(uint8_t idx) {
    if (!recordStorage || idx >= stationCount) {
        return nullptr;
    }
    return &stationRecords()[idx];
}

const char* StationManager::getStationName(uint8_t idx) {
    if (!recordStorage || idx >= stationCount) {
        return "";
    }
    return stationRecords()[idx].name;
}

const char* StationManager::getStationURL(uint8_t idx) {
    if (!recordStorage || idx >= stationCount) {
        return "";
    }
    return stationRecords()[idx].url;
}

StationCodec StationManager::getStationCodec(uint8_t idx) {
    if (!recordStorage || idx >= stationCount) {
        return STATION_CODEC_MP3;
    }
    return stationRecords()[idx].codec;
}

void StationManager::setCurrentStation(uint8_t idx) {
    if (recordStorage && idx < stationCount) {
        currentStation = idx;
        Serial.printf("[STATION] Current: %s\n", stationRecords()[idx].name);
    }
}

// ============================================
// Default Stations
// ============================================
bool StationManager::loadDefaultStations() {
    if (!recordStorage) {
        Serial.println("[STATION] Cannot load saved stations: record storage unavailable");
        return false;
    }
    // Hardcoded stations removed per request - now using the Browse/
    // Favorites/Recent flow exclusively (see main.cpp's
    // enterBrowseCountry() and friends, and browser.h for the
    // radio-browser.info API client).
    //
    // Left the previous list here disabled (not deleted) rather than
    // erased, given how much verification went into finding real, working
    // URLs for each of these (DevTools captures, the SBR/OOM risk
    // analysis per station, etc.) - flip this back to #if 1 to restore
    // them if useful later.
#if 0
    // This firmware reads raw audio bytes directly - it does not parse
    // HLS (.m3u8) or playlist-wrapper (.pls/.m3u) formats, so a station's
    // URL needs to be a genuine direct stream (MP3 or AAC now that both
    // are supported), not a link to one of those wrapper formats. Both
    // HTTP and HTTPS work (see AudioFileSourceICYSStream for the HTTPS
    // path) - codec and scheme are both auto/explicitly selected per
    // station, see addStation() calls below.

    // Previous placeholder stations (SomaFM x4, Radio Paradise, WFUV)
    // removed per request - replaced with real, user-confirmed stations
    // instead of more placeholder URLs I can't verify from this
    // environment (no network path to arbitrary streaming servers from
    // here).
    //
    // West City Radio - the DevTools URL turned out to be a relay
    // (api.3.5.2.webradio.tools) sitting in front of the real Icecast
    // server, over HTTPS with an AAC+SBR stream. SBR needs ~50KB extra to
    // decode (confirmed: "OOM in SBR, can't allocate 50788 bytes" in
    // testing), which this board can't spare on top of WiFi+TLS+display.
    // The relay's own status API (fetched by the user, port 8000, "mount":
    // "/mp3") revealed the real underlying server's direct IP and MP3
    // mount point - confirmed working by the user directly. Using that
    // instead: plain HTTP (no TLS overhead at all) and MP3 (no SBR issue),
    // going straight to the actual Icecast server rather than through the
    // relay layer.
    // Caveat worth knowing: this is a bare IP, not a hostname - more
    // fragile long-term if the station ever migrates servers, unlike a
    // proper domain name. Worth revisiting if this stops working someday.
    addStation("West City Radio",
               "http://188.214.156.114:8000/mp3",
               STATION_CODEC_MP3);
    
    // All below confirmed by the user via browser DevTools (Network tab
    // while each stream played) - not guessed URLs.
    //
    // Plain MP3, plain HTTP - lowest risk, no TLS overhead and no SBR
    // decode-memory concern at all. These three share the same
    // "edge*.rdsnet.ro" infrastructure.
    addStation("Digi FM",
               "http://edge76.rdsnet.ro:84/digifm/digifm.mp3",
               STATION_CODEC_MP3);
    addStation("Dance FM",
               "http://edge126.rdsnet.ro:84/profm/dancefm.mp3",
               STATION_CODEC_MP3);
    addStation("ProFM",
               "http://edge126.rdsnet.ro:84/profm/profm.mp3",
               STATION_CODEC_MP3);
    
    // AAC, plain HTTP - real stream file extension is ".aac" (not ".aacp"),
    // which is a weaker signal than the explicit "aacp" ones below, but
    // still no guarantee this isn't HE-AAC/SBR under the hood. At least
    // not combined with TLS overhead if it does turn out to need it.
    addStation("Radio Guerrilla",
               "http://live.guerrillaradio.ro:8010/guerrilla.aac",
               STATION_CODEC_AAC);
    
    // AAC+ (HE-AAC/SBR) - user-confirmed codec directly, plain HTTP at
    // least. This is the exact codec family that caused
    // "OOM in SBR, can't allocate 50788 bytes" with West City Radio's
    // original stream - real chance of hitting the same crash.
    addStation("Radio ZU",
               "http://zuicast.digitalag.ro:9420/zu",
               STATION_CODEC_AAC);
    
    // AAC+ - "aacp48k" in the URL itself is the standard naming convention
    // for HE-AAC/SBR streams. Same SBR risk as above, plain HTTP at least.
    addStation("Europa FM",
               "http://astreaming.europafm.ro:8000/europafm_aacp48k",
               STATION_CODEC_AAC);
    
    // Codec not confirmed either way - URL has no clear extension hint.
    // Defaulting to MP3 as the safer guess: if this is actually AAC, the
    // MP3 decoder will just fail to make sense of it (no sound, no
    // crash) rather than risk anything worse. Worth confirming and
    // correcting if you find out for certain.
    addStation("Radio Impuls",
               "https://live.radio-impuls.ro/stream",
               STATION_CODEC_MP3);
    
    // AAC+ over HTTPS - the highest-risk combination in this list. This is
    // the EXACT pairing (TLS overhead + SBR decode buffer, both at once)
    // that caused West City Radio's original OOM crash. Genuinely likely
    // to hit the same wall on this board with no PSRAM.
    addStation("Kiss FM",
               "https://live.kissfm.ro/kissfm.aacp",
               STATION_CODEC_AAC);
    addStation("Rock FM",
               "https://live.rockfm.ro/rockfm.aacp",
               STATION_CODEC_AAC);
    addStation("Magic FM",
               "https://live.magicfm.ro/magicfm.aacp",
               STATION_CODEC_AAC);
    // Hosted on the same live.kissfm.ro infrastructure as the confirmed
    // AAC+ Kiss FM stream above - same risk profile assumed, though this
    // specific URL's codec wasn't independently confirmed.
    addStation("Magic FM 90s Hits",
               "https://live.kissfm.ro/Magic-90s",
               STATION_CODEC_AAC);
#endif
    
    Preferences prefs;
    // The user-station namespace does not exist until the first web add.
    // Opening it read-only makes Preferences emit a misleading NVS
    // NOT_FOUND error on every fresh device. Read-write opens/creates the
    // empty namespace without writing station data or touching Favorites/
    // Recent, which are separate namespaces.
    if (!prefs.begin("stations", false)) {
        Serial.println("[STATION] Could not open saved-stations NVS");
        stationCount = 0;
        return false;
    }
    stationCount = prefs.getUChar("count", 0);
    if (stationCount > MAX_STATIONS) stationCount = MAX_STATIONS;
    for (uint8_t i = 0; i < stationCount; i++) {
        char nameKey[8], urlKey[8], codecKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%d", i);
        snprintf(urlKey, sizeof(urlKey), "u%d", i);
        snprintf(codecKey, sizeof(codecKey), "c%d", i);
        prefs.getString(nameKey, stationRecords()[i].name, MAX_NAME_LENGTH);
        prefs.getString(urlKey, stationRecords()[i].url, MAX_URL_LENGTH);
        stationRecords()[i].codec = stationCodecForURL(
            stationRecords()[i].url,
            (StationCodec)prefs.getUChar(codecKey, STATION_CODEC_MP3));
    }
    prefs.end();

    advanceCatalogRevision();
    Serial.printf("[STATION] Loaded %d saved stations\n", stationCount);
    return true;
}

void StationManager::saveStationsToPrefs() {
    if (!recordStorage) return;
    Preferences prefs;
    prefs.begin("stations", false);
    prefs.putUChar("count", stationCount);
    for (uint8_t i = 0; i < stationCount; i++) {
        char nameKey[8], urlKey[8], codecKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%d", i);
        snprintf(urlKey, sizeof(urlKey), "u%d", i);
        snprintf(codecKey, sizeof(codecKey), "c%d", i);
        prefs.putString(nameKey, stationRecords()[i].name);
        prefs.putString(urlKey, stationRecords()[i].url);
        prefs.putUChar(codecKey, stationRecords()[i].codec);
    }
    prefs.end();
}

// ============================================
// Recent / Favorites - persisted via Preferences (NVS)
// ============================================
void StationManager::loadPersistedLists() {
    if (!recordStorage) {
        Serial.println("[STATION] Cannot load persisted lists: record storage unavailable");
        return;
    }
    Preferences prefs;

    prefs.begin("recent", true); // read-only
    recentCount = prefs.getUChar("count", 0);
    if (recentCount > MAX_RECENT) recentCount = MAX_RECENT;
    for (uint8_t i = 0; i < recentCount; i++) {
        char nameKey[8], urlKey[8], codecKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%d", i);
        snprintf(urlKey, sizeof(urlKey), "u%d", i);
        snprintf(codecKey, sizeof(codecKey), "c%d", i);
        prefs.getString(nameKey, recentRecords()[i].name, MAX_NAME_LENGTH);
        prefs.getString(urlKey, recentRecords()[i].url, MAX_URL_LENGTH);
        recentRecords()[i].codec = stationCodecForURL(
            recentRecords()[i].url,
            (StationCodec)prefs.getUChar(codecKey, STATION_CODEC_MP3));
    }
    prefs.end();

    prefs.begin("favorites", true);
    favoriteCount = prefs.getUChar("count", 0);
    if (favoriteCount > MAX_FAVORITES) favoriteCount = MAX_FAVORITES;
    for (uint8_t i = 0; i < favoriteCount; i++) {
        char nameKey[8], urlKey[8], codecKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%d", i);
        snprintf(urlKey, sizeof(urlKey), "u%d", i);
        snprintf(codecKey, sizeof(codecKey), "c%d", i);
        prefs.getString(nameKey, favoriteRecords()[i].name, MAX_NAME_LENGTH);
        prefs.getString(urlKey, favoriteRecords()[i].url, MAX_URL_LENGTH);
        favoriteRecords()[i].codec = stationCodecForURL(
            favoriteRecords()[i].url,
            (StationCodec)prefs.getUChar(codecKey, STATION_CODEC_MP3));
    }
    prefs.end();

    advanceCatalogRevision();
    Serial.printf("[STATION] Loaded %d recent, %d favorites from flash\n", recentCount, favoriteCount);
}

void StationManager::addToRecent(const char* name, const char* url, StationCodec codec) {
    if (!recordStorage || !name || !url) {
        Serial.println("[STATION] Recent record storage unavailable or invalid input");
        return;
    }
    codec = stationCodecForURL(url, codec);
    // If already present, remove the old entry first - it gets re-added
    // at the front below, avoiding duplicate entries piling up.
    for (uint8_t i = 0; i < recentCount; i++) {
        if (strcmp(recentRecords()[i].url, url) == 0) {
            for (uint8_t j = i; j < recentCount - 1; j++) {
                recentRecords()[j] = recentRecords()[j + 1];
            }
            recentCount--;
            break;
        }
    }

    // Shift everything down to make room at the front, dropping the
    // oldest entry if already at capacity.
    uint8_t insertCount = (recentCount < MAX_RECENT) ? recentCount : (MAX_RECENT - 1);
    for (uint8_t i = insertCount; i > 0; i--) {
        recentRecords()[i] = recentRecords()[i - 1];
    }

    strncpy(recentRecords()[0].name, name, MAX_NAME_LENGTH - 1);
    recentRecords()[0].name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(recentRecords()[0].url, url, MAX_URL_LENGTH - 1);
    recentRecords()[0].url[MAX_URL_LENGTH - 1] = '\0';
    recentRecords()[0].codec = codec;

    if (recentCount < MAX_RECENT) recentCount++;

    advanceCatalogRevision();
    saveRecentToPrefs();
}

RadioStation* StationManager::getRecent(uint8_t idx) {
    if (!recordStorage || idx >= recentCount) return nullptr;
    return &recentRecords()[idx];
}

void StationManager::saveRecentToPrefs() {
    if (!recordStorage) return;
    Preferences prefs;
    prefs.begin("recent", false); // read-write
    prefs.putUChar("count", recentCount);
    for (uint8_t i = 0; i < recentCount; i++) {
        char nameKey[8], urlKey[8], codecKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%d", i);
        snprintf(urlKey, sizeof(urlKey), "u%d", i);
        snprintf(codecKey, sizeof(codecKey), "c%d", i);
        prefs.putString(nameKey, recentRecords()[i].name);
        prefs.putString(urlKey, recentRecords()[i].url);
        prefs.putUChar(codecKey, recentRecords()[i].codec);
    }
    prefs.end();
}

bool StationManager::addToFavorites(const char* name, const char* url, StationCodec codec) {
    if (!recordStorage || !name || !url) {
        Serial.println("[STATION] Favorite record storage unavailable or invalid input");
        return false;
    }
    codec = stationCodecForURL(url, codec);
    if (isFavorite(url)) {
        return false; // already there - caller can tell the user
    }
    if (favoriteCount >= MAX_FAVORITES) {
        Serial.println("[STATION] Favorites list full");
        return false;
    }
    strncpy(favoriteRecords()[favoriteCount].name, name, MAX_NAME_LENGTH - 1);
    favoriteRecords()[favoriteCount].name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(favoriteRecords()[favoriteCount].url, url, MAX_URL_LENGTH - 1);
    favoriteRecords()[favoriteCount].url[MAX_URL_LENGTH - 1] = '\0';
    favoriteRecords()[favoriteCount].codec = codec;
    favoriteCount++;
    advanceCatalogRevision();
    saveFavoritesToPrefs();
    return true;
}

bool StationManager::removeFavorite(uint8_t idx) {
    if (!recordStorage || idx >= favoriteCount) return false;
    for (uint8_t i = idx; i < favoriteCount - 1; i++) {
        favoriteRecords()[i] = favoriteRecords()[i + 1];
    }
    favoriteCount--;
    advanceCatalogRevision();
    saveFavoritesToPrefs();
    return true;
}

RadioStation* StationManager::getFavorite(uint8_t idx) {
    if (!recordStorage || idx >= favoriteCount) return nullptr;
    return &favoriteRecords()[idx];
}

bool StationManager::isFavorite(const char* url) {
    if (!recordStorage || !url) return false;
    for (uint8_t i = 0; i < favoriteCount; i++) {
        if (strcmp(favoriteRecords()[i].url, url) == 0) return true;
    }
    return false;
}

void StationManager::saveFavoritesToPrefs() {
    if (!recordStorage) return;
    Preferences prefs;
    prefs.begin("favorites", false);
    prefs.putUChar("count", favoriteCount);
    for (uint8_t i = 0; i < favoriteCount; i++) {
        char nameKey[8], urlKey[8], codecKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%d", i);
        snprintf(urlKey, sizeof(urlKey), "u%d", i);
        snprintf(codecKey, sizeof(codecKey), "c%d", i);
        prefs.putString(nameKey, favoriteRecords()[i].name);
        prefs.putString(urlKey, favoriteRecords()[i].url);
        prefs.putUChar(codecKey, favoriteRecords()[i].codec);
    }
    prefs.end();
}
