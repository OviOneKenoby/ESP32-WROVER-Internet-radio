#ifndef BROWSER_H
#define BROWSER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "config.h"
#include "stations.h" // for StationCodec - search results now carry
                       // their real codec (MP3 or AAC) instead of
                       // always being assumed MP3

// Station discovery via the radio-browser.info API - browsing all
// countries and genres/tags, always filtered to MP3 (per requirements).
// Fixed-size static arrays throughout, matching this project's existing
// style - no dynamic containers on an embedded target this RAM-limited.

struct BrowseCountry {
    char name[BROWSE_COUNTRY_NAME_LEN];
    char code[BROWSE_COUNTRY_CODE_LEN]; // ISO 3166-1 alpha-2, e.g. "RO"
};

struct BrowseTag {
    char name[BROWSE_TAG_NAME_LEN];
};

struct BrowseResult {
    char name[MAX_NAME_LENGTH];
    char url[MAX_URL_LENGTH];
    StationCodec codec;
};

class StationBrowser {
public:
    StationBrowser();

    // Each of these performs a blocking HTTPS GET + JSON parse, populating
    // the corresponding array and count. Returns false on any network or
    // parse failure (array/count are left at whatever they were before -
    // callers should check the return value, not just assume success).
    // Each also allocates its backing array on first use if not already
    // allocated (see freeBrowseMemory()).
    bool fetchCountries();
    bool fetchTags();
    bool searchStations(const char* countryCode, const char* tagName,
                        const char* stationNameFilter);

    uint16_t getCountryCount() { return countryCount; }
    BrowseCountry* getCountry(uint16_t idx);

    uint8_t getTagCount() { return tagCount; }
    BrowseTag* getTag(uint8_t idx);

    uint8_t getResultCount() { return resultCount; }
    BrowseResult* getResult(uint8_t idx);
    
    // Set whenever any fetch/search call above returns false - shows the
    // actual reason (HTTP error code, JSON parse error, or heap
    // exhaustion) rather than a generic "failed" - lets a failure be
    // diagnosed from the on-screen error alone, not just the serial log.
    const char* getLastError() { return lastError.c_str(); }
    
    // Explicitly and fully releases the browse API's persistent HTTPS
    // connection - http.end() alone may not be enough. Call this before
    // any station playback attempt: the persistent httpsClient/http
    // members (kept alive across browse calls specifically to fight
    // fragmentation within the browse flow - see round 34) can otherwise
    // still be holding TLS session resources even once "idle", competing
    // with the audio stream's own separate TLS connection for the same
    // limited RAM the moment you try to actually play something.
    void releaseConnection();
    
    // Frees the countries/tags/results arrays - call when leaving the
    // browse flow entirely (back to the main station list, or once a
    // station starts playing). These were previously permanent static
    // arrays (~18KB reserved for the device's entire runtime, whether
    // actively browsing or just listening to a station); now allocated
    // lazily on first use and freed here instead, giving that memory back
    // the rest of the time. Safe to call even if nothing was ever
    // allocated (checks for null internally, same as a no-op delete).
    void freeBrowseMemory();

private:
    BrowseCountry* countries; // allocated lazily, see fetchCountries()
    uint16_t countryCount;

    BrowseTag* tags; // allocated lazily, see fetchTags()
    uint8_t tagCount;

    BrowseResult* results; // allocated lazily, see searchStations()
    uint8_t resultCount;
    
    String lastError;

    // Shared HTTPS GET + filtered-JSON-parse helper - tries each known
    // mirror in sequence, see .cpp for the retry logic.
    bool httpsGetJson(const String& path, JsonDocument& filter, JsonDocument& doc);
    bool httpsGetJsonOnce(const char* host, const String& path, JsonDocument& filter, JsonDocument& doc, bool& serverLevelFailure);
    
    // Persistent across calls rather than fresh local objects each time -
    // constructing/destroying a WiFiClientSecure (and its internal TLS
    // session buffers) repeatedly for country -> tag -> search back to
    // back was a real, confirmed contributor to heap fragmentation
    // (measured: largest contiguous block dropped from 94KB to 53KB
    // across just two sequential requests). Reusing the same objects
    // keeps that allocation stable instead of tearing it down and
    // rebuilding it fresh each time.
    WiFiClientSecure httpsClient;
    HTTPClient http;
};

extern StationBrowser stationBrowser;

#endif // BROWSER_H
