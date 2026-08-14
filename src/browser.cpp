#include "browser.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <StreamString.h>

StationBrowser stationBrowser;

StationBrowser::StationBrowser()
    : countries(nullptr), countryCount(0),
      tags(nullptr), tagCount(0),
      results(nullptr), resultCount(0) {
}

void StationBrowser::freeBrowseMemory() {
    delete[] countries;
    countries = nullptr;
    countryCount = 0;
    
    delete[] tags;
    tags = nullptr;
    tagCount = 0;
    
    delete[] results;
    results = nullptr;
    resultCount = 0;
    
    Serial.printf("[BROWSER] Freed browse memory (heap: free=%u, largest block=%u)\n",
                  ESP.getFreeHeap(), ESP.getMaxAllocHeap());
}

void StationBrowser::releaseConnection() {
    http.end();
    httpsClient.stop();
    Serial.printf("[BROWSER] Released connection (heap: free=%u, largest block=%u)\n",
                  ESP.getFreeHeap(), ESP.getMaxAllocHeap());
}

// ============================================
// Small helper: percent-encode a string for use in a URL query value.
// Only handles what actually shows up in tag/country names (spaces,
// occasional accented characters aren't expected here since tags are
// simple English words like "classic rock") - not a general-purpose
// encoder, just enough for this specific use.
// ============================================
static String urlEncodeSimple(const char* input) {
    String out;
    for (const char* p = input; *p; p++) {
        if (*p == ' ') {
            out += "%20";
        } else if (isalnum((unsigned char)*p) || *p == '-' || *p == '_' || *p == '.') {
            out += *p;
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)*p);
            out += buf;
        }
    }
    return out;
}

// ============================================
// Shared HTTPS GET + filtered-JSON-parse helper - tries each known
// mirror in sequence (see RADIO_BROWSER_MIRRORS in config.h) until one
// succeeds, but only retries a different mirror for connection/server-
// level failures (begin() failure, non-200 status). A failure that
// happens after a successful 200 response (buffer reservation, body
// read, JSON parsing) is a client-side problem that would fail
// identically on any mirror - no point burning another 20s timeout
// retrying those elsewhere.
// ============================================
bool StationBrowser::httpsGetJson(const String& path, JsonDocument& filter, JsonDocument& doc) {
    for (int i = 0; i < RADIO_BROWSER_MIRROR_COUNT; i++) {
        bool serverLevelFailure = false;
        if (httpsGetJsonOnce(RADIO_BROWSER_MIRRORS[i], path, filter, doc, serverLevelFailure)) {
            return true;
        }
        if (!serverLevelFailure) {
            return false; // client-side issue - retrying elsewhere won't help
        }
        Serial.printf("[BROWSER] %s failed, trying next mirror\n", RADIO_BROWSER_MIRRORS[i]);
    }
    return false; // every mirror failed
}

bool StationBrowser::httpsGetJsonOnce(const char* host, const String& path, JsonDocument& filter, JsonDocument& doc, bool& serverLevelFailure) {
    serverLevelFailure = false;
    http.end(); // defensive - ensures clean state even if a previous
                // call didn't get to its own end() (e.g. an early
                // failure return), before reusing these persistent
                // objects for a new request.
    httpsClient.setInsecure(); // see AudioFileSourceHTTPSStream.h - same
                                // reasoning: managing CA certs for this
                                // isn't practical here, standard tradeoff.

    String url = String("https://") + host + path;
    Serial.printf("[BROWSER] GET %s\n", url.c_str());
    Serial.printf("[BROWSER] Heap before request: free=%u, largest block=%u\n",
                  ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    if (!http.begin(httpsClient, url)) {
        lastError = "HTTPS begin() failed";
        Serial.printf("[BROWSER] %s\n", lastError.c_str());
        serverLevelFailure = true; // couldn't even start - worth trying a different host
        return false;
    }
    // 20s connect+response timeout - confirmed by direct measurement
    // (round 32) that a broad, country-only search genuinely takes
    // ~15s server-side to rank results by popularity when there's no
    // genre to narrow the candidate set. This whole call is synchronous
    // on the main loop, so the whole UI is unresponsive for as long as
    // it takes - a known, real tradeoff of this architecture, not
    // something silently ignored.
    http.setConnectTimeout(20000);
    http.setTimeout(20000);
    http.addHeader("User-Agent", RADIO_BROWSER_USER_AGENT);

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        // Negative codes are HTTPClient's own internal error codes (not
        // real HTTP status codes) - e.g. -1 connection refused, -8 too
        // little RAM available, -11 read timeout. Logging heap state
        // alongside this makes a RAM-related failure immediately
        // diagnosable without needing to ask for another log capture.
        lastError = "HTTP error " + String(code);
        Serial.printf("[BROWSER] HTTP GET failed, code %d (heap: free=%u, largest block=%u)\n",
                      code, ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        http.end();
        serverLevelFailure = true; // connection/server issue - worth trying a different host
        return false;
    }

    // getString() can only pre-reserve its buffer when Content-Length is
    // known upfront (confirmed directly in HTTPClient's source) - for a
    // chunked response (unknown length, exactly what a dynamically-sized
    // search result would be), that reserve is a silent no-op, and the
    // buffer has to grow incrementally as data streams in instead. If a
    // reallocation during that growth fails to find a big enough
    // contiguous block (this board's confirmed, recurring fragmentation
    // problem), the result is silently truncated at whatever size it
    // reached - which is exactly what "IncompleteInput" with a
    // shorter-than-expected body looks like.
    // Fixed by reserving a generous fixed capacity BEFORE any data
    // arrives, via the same underlying de-chunking logic (writeToStream()
    // is what getString() calls internally) - avoids the incremental
    // growth entirely instead of hoping each reallocation succeeds.
    StreamString body;
    if (!body.reserve(RESPONSE_BUFFER_RESERVE)) {
        lastError = "Could not reserve response buffer";
        Serial.printf("[BROWSER] %s (heap: free=%u, largest block=%u)\n",
                      lastError.c_str(), ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        http.end();
        return false; // client-side (RAM) issue, not worth trying another mirror
    }
    int written = http.writeToStream(&body);
    http.end();

    if (written < 0) {
        lastError = "Body read failed, code " + String(written);
        Serial.printf("[BROWSER] %s (heap: free=%u, largest block=%u)\n",
                      lastError.c_str(), ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        return false; // client-side, not worth trying another mirror
    }
    Serial.printf("[BROWSER] Body length: %d bytes\n", written);

    // Pass the plain null-terminated C-string (via c_str()), not the
    // StreamString object directly - StreamString inherits from BOTH
    // Stream and String simultaneously, and deserializeJson() has
    // separate template specializations for each, causing an ambiguous
    // template instantiation (confirmed by the actual compile error).
    // A raw pointer matches neither specialization, sidestepping the
    // conflict entirely - and this exact (pointer, Filter) 2-argument
    // pattern was already verified working in this project's very first
    // version of this function, before switching to the Stream-based
    // approach.
    DeserializationError err = deserializeJson(doc, body.c_str(),
                                                DeserializationOption::Filter(filter));

    if (err) {
        lastError = String("Parse error: ") + err.c_str();
        Serial.printf("[BROWSER] %s (heap: free=%u, largest block=%u)\n",
                      lastError.c_str(), ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        return false; // client-side, not worth trying another mirror
    }
    Serial.printf("[BROWSER] Parsed OK - %u top-level elements (heap: free=%u, largest block=%u)\n",
                  (unsigned)doc.size(), ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    return true;
}

// ============================================
// Countries
// ============================================
bool StationBrowser::fetchCountries() {
    if (!countries) {
        countries = new BrowseCountry[MAX_BROWSE_COUNTRIES];
    }
    
    JsonDocument filter;
    filter[0]["name"] = true;
    filter[0]["iso_3166_1"] = true;

    JsonDocument doc;
    if (!httpsGetJson("/json/countries?order=name", filter, doc)) {
        return false;
    }

    countryCount = 0;
    for (JsonObject obj : doc.as<JsonArray>()) {
        if (countryCount >= MAX_BROWSE_COUNTRIES) break;

        const char* name = obj["name"] | "";
        const char* code = obj["iso_3166_1"] | "";
        if (!name[0] || !code[0]) continue; // skip entries without a
                                             // usable code - can't search
                                             // by country without one

        strncpy(countries[countryCount].name, name, BROWSE_COUNTRY_NAME_LEN - 1);
        countries[countryCount].name[BROWSE_COUNTRY_NAME_LEN - 1] = '\0';
        strncpy(countries[countryCount].code, code, BROWSE_COUNTRY_CODE_LEN - 1);
        countries[countryCount].code[BROWSE_COUNTRY_CODE_LEN - 1] = '\0';
        countryCount++;
    }

    Serial.printf("[BROWSER] Loaded %d countries\n", countryCount);
    if (countryCount == 0) {
        lastError = "0 countries returned";
    }
    return countryCount > 0;
}

BrowseCountry* StationBrowser::getCountry(uint16_t idx) {
    if (idx >= countryCount) return nullptr;
    return &countries[idx];
}

// ============================================
// Tags (genres) - top N by popularity, not the full ~6700+ tag list
// ============================================
bool StationBrowser::fetchTags() {
    if (!tags) {
        tags = new BrowseTag[MAX_BROWSE_TAGS];
    }
    
    JsonDocument filter;
    filter[0]["name"] = true;

    JsonDocument doc;
    String path = "/json/tags?order=stationcount&reverse=true&limit=" + String(MAX_BROWSE_TAGS);
    if (!httpsGetJson(path, filter, doc)) {
        return false;
    }

    tagCount = 0;
    for (JsonObject obj : doc.as<JsonArray>()) {
        if (tagCount >= MAX_BROWSE_TAGS) break;

        const char* name = obj["name"] | "";
        if (!name[0]) continue;

        strncpy(tags[tagCount].name, name, BROWSE_TAG_NAME_LEN - 1);
        tags[tagCount].name[BROWSE_TAG_NAME_LEN - 1] = '\0';
        tagCount++;
    }

    Serial.printf("[BROWSER] Loaded %d tags\n", tagCount);
    if (tagCount == 0) {
        lastError = "0 tags returned";
    }
    return tagCount > 0;
}

BrowseTag* StationBrowser::getTag(uint8_t idx) {
    if (idx >= tagCount) return nullptr;
    return &tags[idx];
}

// ============================================
// Station search - always MP3-only per requirements, both to sidestep
// the AAC+/SBR memory risk (see CHANGELOG round 23) and because it was
// explicitly requested.
// ============================================
bool StationBrowser::searchStations(const char* countryCode, const char* tagName,
                                    const char* stationNameFilter) {
    if (!results) {
        results = new BrowseResult[MAX_BROWSE_RESULTS];
    }
    
    // No longer restricted to codec=mp3 server-side - both MP3 and AAC
    // are real, working decode paths now (AudioGeneratorMP3a and
    // AudioGeneratorAAC), and with PSRAM available the AAC+/SBR memory
    // requirement that caused the original crash (round 18/23 -
    // "OOM in SBR, can't allocate 50788 bytes") should no longer be the
    // hard blocker it was. Codec is read per-result below instead, and
    // anything we don't have a decoder for (OGG, FLAC, WMA, etc.) is
    // simply skipped rather than fetched and then failing to play.
    String path = "/json/stations/search?hidebroken=true&order=clickcount&reverse=true&limit=" + String(MAX_BROWSE_RESULTS);
    if (countryCode && countryCode[0]) {
        path += "&countrycode=" + urlEncodeSimple(countryCode);
    }
    if (tagName && tagName[0]) {
        path += "&tag=" + urlEncodeSimple(tagName);
    }
    if (stationNameFilter && stationNameFilter[0]) {
        // Radio Browser's name parameter is a case-insensitive substring
        // search. It cuts the response down to a practical size, while the
        // first-character check below turns it into the promised A-Z list.
        path += "&name=" + urlEncodeSimple(stationNameFilter);
    }

    JsonDocument filter;
    filter[0]["name"] = true;
    filter[0]["url_resolved"] = true;
    filter[0]["codec"] = true;

    JsonDocument doc;
    if (!httpsGetJson(path, filter, doc)) {
        return false;
    }

    resultCount = 0;
    for (JsonObject obj : doc.as<JsonArray>()) {
        if (resultCount >= MAX_BROWSE_RESULTS) break;

        const char* name = obj["name"] | "";
        const char* url = obj["url_resolved"] | "";
        const char* codecStr = obj["codec"] | "";
        if (!name[0] || !url[0]) continue;

        if (stationNameFilter && stationNameFilter[0] &&
            toupper((unsigned char)name[0]) != toupper((unsigned char)stationNameFilter[0])) {
            continue;
        }

        // Case-insensitive match - "MP3" has been the consistently
        // observed format throughout this project's testing, but
        // matching case-insensitively is cheap insurance against a
        // differently-cased entry. AAC also covers "AAC+"/"AACP" (HE-AAC
        // with SBR) - AudioGeneratorAAC's Helix decoder handles this,
        // the earlier crash was a RAM problem, not a format one.
        String codecUpper = String(codecStr);
        codecUpper.toUpperCase();
        StationCodec codec;
        if (codecUpper.indexOf("MP3") >= 0) {
            codec = STATION_CODEC_MP3;
        } else if (codecUpper.indexOf("AAC") >= 0) {
            codec = STATION_CODEC_AAC;
        } else {
            continue; // unsupported codec (OGG, FLAC, WMA, etc.) - no
                      // decoder for this, skip rather than add a result
                      // that will fail to play
        }

        strncpy(results[resultCount].name, name, MAX_NAME_LENGTH - 1);
        results[resultCount].name[MAX_NAME_LENGTH - 1] = '\0';
        strncpy(results[resultCount].url, url, MAX_URL_LENGTH - 1);
        results[resultCount].url[MAX_URL_LENGTH - 1] = '\0';
        results[resultCount].codec = codec;
        resultCount++;
    }

    Serial.printf("[BROWSER] Found %d stations\n", resultCount);
    if (resultCount == 0) {
        lastError = "0 stations matched";
    }
    return resultCount > 0;
}

BrowseResult* StationBrowser::getResult(uint8_t idx) {
    if (idx >= resultCount) return nullptr;
    return &results[idx];
}
