#include "stations.h"
#include "config.h"
#include <Preferences.h>

// Global station manager
StationManager stationManager;

StationCodec stationCodecForURL(const char* url, StationCodec storedCodec) {
    if (!url) return storedCodec;

    String normalizedURL(url);
    normalizedURL.toLowerCase();
    return normalizedURL.indexOf(".aac") >= 0 ? STATION_CODEC_AAC : storedCodec;
}

// ============================================
// Constructor
// ============================================
StationManager::StationManager()
    : stationCount(0),
      currentStation(0),
      recentCount(0),
      favoriteCount(0) {
}

StationManager::~StationManager() {
}

// ============================================
// Station Management
// ============================================
uint8_t StationManager::addStation(const char* name, const char* url, StationCodec codec) {
    if (stationCount >= MAX_STATIONS) {
        Serial.println("[STATION] Max stations reached");
        return 0xFF;
    }
    
    if (!name || !url) {
        Serial.println("[STATION] Invalid parameters");
        return 0xFF;
    }
    
    RadioStation& station = stations[stationCount];
    strncpy(station.name, name, MAX_NAME_LENGTH - 1);
    strncpy(station.url, url, MAX_URL_LENGTH - 1);
    station.codec = codec;
    
    Serial.printf("[STATION] Added: %s -> %s (%s)\n", name, url,
                  codec == STATION_CODEC_AAC ? "AAC" : "MP3");
    uint8_t addedIndex = stationCount++;
    saveStationsToPrefs();
    return addedIndex;
}

bool StationManager::removeStation(uint8_t idx) {
    if (idx >= stationCount) {
        Serial.println("[STATION] Invalid index");
        return false;
    }
    
    // Shift remaining stations
    for (uint8_t i = idx; i < stationCount - 1; i++) {
        memcpy(&stations[i], &stations[i + 1], sizeof(RadioStation));
    }
    
    stationCount--;
    
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
    if (idx >= stationCount) {
        return nullptr;
    }
    return &stations[idx];
}

const char* StationManager::getStationName(uint8_t idx) {
    if (idx >= stationCount) {
        return "";
    }
    return stations[idx].name;
}

const char* StationManager::getStationURL(uint8_t idx) {
    if (idx >= stationCount) {
        return "";
    }
    return stations[idx].url;
}

StationCodec StationManager::getStationCodec(uint8_t idx) {
    if (idx >= stationCount) {
        return STATION_CODEC_MP3;
    }
    return stations[idx].codec;
}

void StationManager::setCurrentStation(uint8_t idx) {
    if (idx < stationCount) {
        currentStation = idx;
        Serial.printf("[STATION] Current: %s\n", stations[idx].name);
    }
}

// ============================================
// Default Stations
// ============================================
bool StationManager::loadDefaultStations() {
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
        prefs.getString(nameKey, stations[i].name, MAX_NAME_LENGTH);
        prefs.getString(urlKey, stations[i].url, MAX_URL_LENGTH);
        stations[i].codec = stationCodecForURL(
            stations[i].url, (StationCodec)prefs.getUChar(codecKey, STATION_CODEC_MP3));
    }
    prefs.end();

    Serial.printf("[STATION] Loaded %d saved stations\n", stationCount);
    return true;
}

void StationManager::saveStationsToPrefs() {
    Preferences prefs;
    prefs.begin("stations", false);
    prefs.putUChar("count", stationCount);
    for (uint8_t i = 0; i < stationCount; i++) {
        char nameKey[8], urlKey[8], codecKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%d", i);
        snprintf(urlKey, sizeof(urlKey), "u%d", i);
        snprintf(codecKey, sizeof(codecKey), "c%d", i);
        prefs.putString(nameKey, stations[i].name);
        prefs.putString(urlKey, stations[i].url);
        prefs.putUChar(codecKey, stations[i].codec);
    }
    prefs.end();
}

// ============================================
// Recent / Favorites - persisted via Preferences (NVS)
// ============================================
void StationManager::loadPersistedLists() {
    Preferences prefs;

    prefs.begin("recent", true); // read-only
    recentCount = prefs.getUChar("count", 0);
    if (recentCount > MAX_RECENT) recentCount = MAX_RECENT;
    for (uint8_t i = 0; i < recentCount; i++) {
        char nameKey[8], urlKey[8], codecKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%d", i);
        snprintf(urlKey, sizeof(urlKey), "u%d", i);
        snprintf(codecKey, sizeof(codecKey), "c%d", i);
        prefs.getString(nameKey, recent[i].name, MAX_NAME_LENGTH);
        prefs.getString(urlKey, recent[i].url, MAX_URL_LENGTH);
        recent[i].codec = stationCodecForURL(
            recent[i].url, (StationCodec)prefs.getUChar(codecKey, STATION_CODEC_MP3));
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
        prefs.getString(nameKey, favorites[i].name, MAX_NAME_LENGTH);
        prefs.getString(urlKey, favorites[i].url, MAX_URL_LENGTH);
        favorites[i].codec = stationCodecForURL(
            favorites[i].url, (StationCodec)prefs.getUChar(codecKey, STATION_CODEC_MP3));
    }
    prefs.end();

    Serial.printf("[STATION] Loaded %d recent, %d favorites from flash\n", recentCount, favoriteCount);
}

void StationManager::addToRecent(const char* name, const char* url, StationCodec codec) {
    codec = stationCodecForURL(url, codec);
    // If already present, remove the old entry first - it gets re-added
    // at the front below, avoiding duplicate entries piling up.
    for (uint8_t i = 0; i < recentCount; i++) {
        if (strcmp(recent[i].url, url) == 0) {
            for (uint8_t j = i; j < recentCount - 1; j++) {
                recent[j] = recent[j + 1];
            }
            recentCount--;
            break;
        }
    }

    // Shift everything down to make room at the front, dropping the
    // oldest entry if already at capacity.
    uint8_t insertCount = (recentCount < MAX_RECENT) ? recentCount : (MAX_RECENT - 1);
    for (uint8_t i = insertCount; i > 0; i--) {
        recent[i] = recent[i - 1];
    }

    strncpy(recent[0].name, name, MAX_NAME_LENGTH - 1);
    recent[0].name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(recent[0].url, url, MAX_URL_LENGTH - 1);
    recent[0].url[MAX_URL_LENGTH - 1] = '\0';
    recent[0].codec = codec;

    if (recentCount < MAX_RECENT) recentCount++;

    saveRecentToPrefs();
}

RadioStation* StationManager::getRecent(uint8_t idx) {
    if (idx >= recentCount) return nullptr;
    return &recent[idx];
}

void StationManager::saveRecentToPrefs() {
    Preferences prefs;
    prefs.begin("recent", false); // read-write
    prefs.putUChar("count", recentCount);
    for (uint8_t i = 0; i < recentCount; i++) {
        char nameKey[8], urlKey[8], codecKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%d", i);
        snprintf(urlKey, sizeof(urlKey), "u%d", i);
        snprintf(codecKey, sizeof(codecKey), "c%d", i);
        prefs.putString(nameKey, recent[i].name);
        prefs.putString(urlKey, recent[i].url);
        prefs.putUChar(codecKey, recent[i].codec);
    }
    prefs.end();
}

bool StationManager::addToFavorites(const char* name, const char* url, StationCodec codec) {
    codec = stationCodecForURL(url, codec);
    if (isFavorite(url)) {
        return false; // already there - caller can tell the user
    }
    if (favoriteCount >= MAX_FAVORITES) {
        Serial.println("[STATION] Favorites list full");
        return false;
    }
    strncpy(favorites[favoriteCount].name, name, MAX_NAME_LENGTH - 1);
    favorites[favoriteCount].name[MAX_NAME_LENGTH - 1] = '\0';
    strncpy(favorites[favoriteCount].url, url, MAX_URL_LENGTH - 1);
    favorites[favoriteCount].url[MAX_URL_LENGTH - 1] = '\0';
    favorites[favoriteCount].codec = codec;
    favoriteCount++;
    saveFavoritesToPrefs();
    return true;
}

bool StationManager::removeFavorite(uint8_t idx) {
    if (idx >= favoriteCount) return false;
    for (uint8_t i = idx; i < favoriteCount - 1; i++) {
        favorites[i] = favorites[i + 1];
    }
    favoriteCount--;
    saveFavoritesToPrefs();
    return true;
}

RadioStation* StationManager::getFavorite(uint8_t idx) {
    if (idx >= favoriteCount) return nullptr;
    return &favorites[idx];
}

bool StationManager::isFavorite(const char* url) {
    for (uint8_t i = 0; i < favoriteCount; i++) {
        if (strcmp(favorites[i].url, url) == 0) return true;
    }
    return false;
}

void StationManager::saveFavoritesToPrefs() {
    Preferences prefs;
    prefs.begin("favorites", false);
    prefs.putUChar("count", favoriteCount);
    for (uint8_t i = 0; i < favoriteCount; i++) {
        char nameKey[8], urlKey[8], codecKey[8];
        snprintf(nameKey, sizeof(nameKey), "n%d", i);
        snprintf(urlKey, sizeof(urlKey), "u%d", i);
        snprintf(codecKey, sizeof(codecKey), "c%d", i);
        prefs.putString(nameKey, favorites[i].name);
        prefs.putString(urlKey, favorites[i].url);
        prefs.putUChar(codecKey, favorites[i].codec);
    }
    prefs.end();
}
