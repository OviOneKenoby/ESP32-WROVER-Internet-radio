#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "audio.h"
#include "input.h"
#include "net_manager.h"
#include "stations.h"
#include "browser.h"
#include "web_portal.h"

// ============================================
// Global State
// ============================================
enum AppMode {
    MODE_BOOT,
    MODE_WIFI_SETUP,
    MODE_STATION_SELECT,
    MODE_PLAYING_RADIO,
    MODE_BLUETOOTH,
    MODE_MENU,
    MODE_ERROR,
    MODE_BROWSE_LETTER,
    MODE_BROWSE_COUNTRY,
    MODE_BROWSE_STATION_LETTER,
    MODE_BROWSE_TAG,
    MODE_BROWSE_RESULTS,
    MODE_FAVORITES,
    MODE_RECENT
};

AppMode currentMode = MODE_BOOT;
AppMode modeBeforeBrowse = MODE_STATION_SELECT; // where "back" returns to
uint32_t lastUIUpdate = 0;
uint8_t selectedStation = 0;
PlaybackState lastKnownPlaybackState = STATE_STOPPED;
bool lastKnownBluetoothConnected = false;
uint32_t volumeDisplayUntil = 0; // 0 = not currently showing volume overlay
char lastBluetoothMetadata[256] = "";

// Shared cursor for all the browse-mode list screens (country/tag/results/
// favorites/recent) - these are always mutually exclusive (only one such
// list is ever being viewed at a time), so one shared variable is simpler
// than five separate ones. Reset to 0 whenever entering any of these modes.
uint8_t browseIndex = 0;

// Remembers the country, station-name prefix, and tag picked while stepping
// through country -> station letter -> tag -> results.
char selectedCountryCode[BROWSE_COUNTRY_CODE_LEN] = "";
char selectedCountryName[BROWSE_COUNTRY_NAME_LEN] = "";
char selectedTagName[BROWSE_TAG_NAME_LEN] = "";
char selectedStationLetter[2] = "";

// Alphabet-first country navigation - scrolling one-by-one through ~250
// countries took several minutes to reach anything starting with a
// later letter. This filters the already-fetched countries[] array
// client-side (no extra network call needed) down to just the ones
// starting with a chosen letter.
uint16_t filteredCountryIndices[MAX_BROWSE_COUNTRIES];
uint16_t filteredCountryCount = 0;

// Tracks whether "Any Country" was chosen at the letter screen (skipping
// country selection entirely) vs a specific country - "back" from the
// genre screen needs to know which screen to return to.
bool skippedCountrySelection = false;

// Tracks the currently-playing station's name and whether it came from
// browse/favorites/recent (a "discovered" station, not part of the
// hardcoded stationManager list) - needed because handlePlayingInput()
// previously read the name directly via
// stationManager.getStationName(selectedStation), which would show the
// WRONG name for a discovered station (selectedStation wouldn't point at
// it). Next/Prev are disabled for discovered stations for the same
// reason - there's no sensible "next" in a one-off search result.
char nowPlayingName[MAX_NAME_LENGTH] = "";
bool nowPlayingIsDiscovered = false;

// Next has no sensible meaning for a discovered (browse/favorites/
// recent) station - there's no "next" in a one-off search result, so
// EVENT_NEXT is repurposed there to add the current station to
// Favorites instead (see handlePlayingInput()). This footer reflects
// that; used consistently everywhere showPlaying() is called so the
// on-screen hint can't fall out of sync with what Next actually does.
const char* nowPlayingFooter() {
    return nowPlayingIsDiscovered ? "< Back  Pause  +Fav >" : "< Prev  Pause  Next >";
}

// ============================================
// Forward declarations
// ============================================
void handleInput(InputEvent event);
void updateUI();
void setupWiFi();
void playStation(uint8_t stationIdx);
void handleStationSelectInput(InputEvent event);
void handlePlayingInput(InputEvent event);
void handleBluetoothInput(InputEvent event);
void handleErrorInput(InputEvent event);
void refreshStationListDisplay();
void checkSerialCommands();
void printCurrentScreen();
void showVolumeOverlay();

// Station discovery (browse by country/genre), Favorites, Recent - see
// browser.h for the underlying API client. Each "enter*" function does
// the (blocking) network fetch behind a loading screen and moves into
// the corresponding mode; each "refresh*Display" redraws that mode's
// list; each "handle*Input" processes input while in that mode.
void enterBrowseLetter();
void handleBrowseLetterInput(InputEvent event);
void refreshBrowseLetterDisplay();
void applyLetterFilter(char letter);

void handleBrowseCountryInput(InputEvent event);
void refreshBrowseCountryDisplay();

void enterBrowseStationLetter();
void handleBrowseStationLetterInput(InputEvent event);
void refreshBrowseStationLetterDisplay();

void enterBrowseTag();
void handleBrowseTagInput(InputEvent event);
void refreshBrowseTagDisplay();

void enterBrowseResults();
void handleBrowseResultsInput(InputEvent event);
void refreshBrowseResultsDisplay();

void enterFavorites();
void handleFavoritesInput(InputEvent event);
void refreshFavoritesDisplay();

void enterRecent();
void handleRecentInput(InputEvent event);
void refreshRecentDisplay();

void playDiscoveredStation(const char* name, const char* url, StationCodec codec, bool addFavorite);

// ============================================
// Setup
// ============================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(500);
    
    Serial.println("\n\n");
    Serial.println("=====================================");
    Serial.println("   ESP32 Internet Radio + Bluetooth");
    Serial.println("   WeAct 1.54\" E-paper Display");
    Serial.println("=====================================\n");
    
    // Real PSRAM detection, not an assumption baked into the rest of the
    // code - this is the actual test. psramFound() reflects genuine
    // hardware detection at boot, independent of the build_flags above
    // (those just tell the core to *look* for PSRAM; this confirms
    // whether it actually found working PSRAM on this specific board).
    if (psramFound()) {
        Serial.printf("[MAIN] PSRAM detected: %u bytes total, %u bytes free\n",
                      ESP.getPsramSize(), ESP.getFreePsram());
    } else {
        Serial.println("[MAIN] No PSRAM detected - falling back to regular heap everywhere (see audio.cpp/browser.cpp for the fallback logic)");
    }
    
    // Initialize display
    Serial.println("[MAIN] Initializing display...");
    if (!display.init()) {
        Serial.println("[MAIN] ERROR: Display initialization failed");
        while (1) delay(1000);
    }
    
    // Initialize input controls
    Serial.println("[MAIN] Initializing input controls...");
    if (!inputControl.init()) {
        Serial.println("[MAIN] ERROR: Input initialization failed");
        while (1) delay(1000);
    }
    
    // Initialize audio system
    Serial.println("[MAIN] Initializing audio system...");
    if (!audioPlayer.init()) {
        Serial.println("[MAIN] ERROR: Audio initialization failed");
        while (1) delay(1000);
    }
    
    // Load default radio stations
    Serial.println("[MAIN] Loading radio stations...");
    stationManager.loadDefaultStations();
    stationManager.loadPersistedLists();
    
    // Load WiFi configuration
    Serial.println("[MAIN] Loading WiFi configuration...");
    wifiManager.loadConfig();
    
    // Show boot message
    currentMode = MODE_BOOT;
    display.showBoot();
    delay(2000);
    
    // Try to connect to WiFi
    setupWiFi();
    
    // Load default stations and switch to station selection
    currentMode = MODE_STATION_SELECT;
    if (webPortal.isConfigPortalActive()) {
        display.showWiFiPortal(webPortal.getPortalSSID(), webPortal.getPortalPassword());
    } else {
        refreshStationListDisplay();
    }
    
    Serial.println("[MAIN] Setup complete!");
    Serial.println();
    Serial.println("=== Remote control via serial monitor ===");
    Serial.println("  u / d   = encoder up / down (scroll stations, volume)");
    Serial.println("  k       = encoder click (select / confirm / back)");
    Serial.println("  space   = play / pause");
    Serial.println("  n / p   = next/prev station (fixed list); n = add to Favorites when playing a discovered station");
    Serial.println("  l       = long-press (Bluetooth mode from main list; Back everywhere in Browse/Favorites/Recent)");
    Serial.println("  s       = print current screen contents as text");
    Serial.println("==========================================");
    Serial.println();
}

// ============================================
// Main Loop
// ============================================
void loop() {
    webPortal.handle();

    // Update input controls
    inputControl.update();
    
    // Handle any input events
    InputEvent event = inputControl.getEvent();
    if (event != EVENT_NONE) {
        handleInput(event);
    }
    
    // Remote/headless testing - lets you drive the whole UI from the
    // serial monitor when you can't physically reach the encoder/buttons
    // or see the e-paper display (e.g. over remote desktop). See
    // checkSerialCommands() for the key mapping.
    checkSerialCommands();
    
    // Periodic UI updates
    uint32_t now = millis();
    if (now - lastUIUpdate > 1000) {
        lastUIUpdate = now;
        updateUI();
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}

// ============================================
// WiFi Setup
// ============================================
void setupWiFi() {
    display.showWiFiConnecting();
    
    // Try to connect with saved credentials
    if (wifiManager.isConnected()) {
        Serial.println("[MAIN] Already connected to WiFi");
        display.showWiFiConnected(wifiManager.getSSID(), wifiManager.getIP());
        webPortal.begin();
        delay(2000);
        return;
    }
    
    // Try to reconnect with saved credentials
    if (strlen(wifiManager.getSSID()) > 0) {
        Serial.printf("[MAIN] Attempting to reconnect to: %s\n", wifiManager.getSSID());
        wifiManager.reconnect();
        
        if (wifiManager.isConnected()) {
            display.showWiFiConnected(wifiManager.getSSID(), wifiManager.getIP());
            webPortal.begin();
            delay(2000);
            return;
        }
    }
    
    // No saved credentials yet (first boot after flashing, or EEPROM was
    // cleared) - connect using the credentials set in config.h. Previously
    // this scanned and connected to whatever network was found first, with
    // an empty password, which only ever worked for open networks - there
    // was no way to specify a password for an encrypted one at all.
    Serial.printf("[MAIN] Connecting to configured network: %s\n", WIFI_SSID);
    if (wifiManager.connect(WIFI_SSID, WIFI_PASSWORD)) {
        display.showWiFiConnected(WIFI_SSID, wifiManager.getIP());
        webPortal.begin();
        delay(2000);
        return;
    }
    
    Serial.println("[MAIN] WiFi connection failed; starting setup portal");
    webPortal.beginConfigPortal();
    if (webPortal.isConfigPortalActive()) {
        display.showError("WiFi setup: 192.168.4.1");
        Serial.printf("[MAIN] Join '%s' then open http://192.168.4.1/\n",
                      webPortal.getPortalSSID());
    } else {
        display.showError("WiFi setup AP failed");
    }
}

// ============================================
// Input Handling
// ============================================
void handleInput(InputEvent event) {
    switch (currentMode) {
        case MODE_STATION_SELECT:
            handleStationSelectInput(event);
            break;
            
        case MODE_PLAYING_RADIO:
            handlePlayingInput(event);
            break;
            
        case MODE_BLUETOOTH:
            handleBluetoothInput(event);
            break;
            
        case MODE_ERROR:
            handleErrorInput(event);
            break;
            
        case MODE_BROWSE_LETTER:
            handleBrowseLetterInput(event);
            break;
            
        case MODE_BROWSE_COUNTRY:
            handleBrowseCountryInput(event);
            break;

        case MODE_BROWSE_STATION_LETTER:
            handleBrowseStationLetterInput(event);
            break;
            
        case MODE_BROWSE_TAG:
            handleBrowseTagInput(event);
            break;
            
        case MODE_BROWSE_RESULTS:
            handleBrowseResultsInput(event);
            break;
            
        case MODE_FAVORITES:
            handleFavoritesInput(event);
            break;
            
        case MODE_RECENT:
            handleRecentInput(event);
            break;
            
        default:
            break;
    }
}

void handleStationSelectInput(InputEvent event) {
    uint8_t stationCount = stationManager.getStationCount();
    uint8_t totalCount = stationCount + 3; // +3 for Browse/Favorites/Recent
    
    switch (event) {
        case EVENT_ENCODER_UP:
            if (selectedStation < totalCount - 1) {
                selectedStation++;
            } else {
                selectedStation = 0;
            }
            Serial.printf("[INPUT] Station: %d/%d\n", selectedStation + 1, totalCount);
            refreshStationListDisplay();
            break;
            
        case EVENT_ENCODER_DOWN:
            if (selectedStation > 0) {
                selectedStation--;
            } else {
                selectedStation = totalCount - 1;
            }
            Serial.printf("[INPUT] Station: %d/%d\n", selectedStation + 1, totalCount);
            refreshStationListDisplay();
            break;
            
        case EVENT_ENCODER_CLICK:
        case EVENT_PLAY_PAUSE:
            if (selectedStation == 0) {
                enterBrowseLetter();
            } else if (selectedStation == 1) {
                enterFavorites();
            } else if (selectedStation == 2) {
                enterRecent();
            } else {
                playStation(selectedStation - 3);
            }
            break;
            
        case EVENT_LONG_PRESS:
            currentMode = MODE_BLUETOOTH;
            audioPlayer.enableBluetooth();
            lastKnownBluetoothConnected = false;
            display.showBluetooth(BT_DEVICE_NAME, false);
            Serial.println("[MAIN] Switched to Bluetooth mode");
            break;
            
        default:
            break;
    }
}

void handlePlayingInput(InputEvent event) {
    switch (event) {
        case EVENT_PLAY_PAUSE:
            if (audioPlayer.getState() == STATE_PLAYING) {
                audioPlayer.pause();
                lastKnownPlaybackState = STATE_PAUSED;
                display.showPlaying(nowPlayingName, "Paused", nowPlayingFooter());
            } else if (audioPlayer.getState() == STATE_PAUSED) {
                audioPlayer.resume();
                lastKnownPlaybackState = STATE_PLAYING;
                display.showPlaying(nowPlayingName, "Playing", nowPlayingFooter());
            }
            break;
            
        case EVENT_NEXT:
            if (nowPlayingIsDiscovered) {
                // Repurposed here specifically - Next has no sensible
                // meaning for a one-off discovered station (there's no
                // ordered "next" the way there is in the fixed list), so
                // it's reused as the "add to favorites" gesture instead.
                if (stationManager.addToFavorites(nowPlayingName, audioPlayer.getCurrentURL(), STATION_CODEC_MP3)) {
                    Serial.printf("[MAIN] Added to favorites: %s\n", nowPlayingName);
                } else {
                    Serial.printf("[MAIN] Not added to favorites (already there, or list full): %s\n", nowPlayingName);
                }
                break;
            }
            selectedStation = (selectedStation + 1) % stationManager.getStationCount();
            playStation(selectedStation);
            break;
            
        case EVENT_PREV:
            if (nowPlayingIsDiscovered) {
                Serial.println("[MAIN] Prev not available for a discovered station - Next adds to Favorites, encoder click goes back");
                break;
            }
            selectedStation = (selectedStation == 0) ? stationManager.getStationCount() - 1 : selectedStation - 1;
            playStation(selectedStation);
            break;
            
        case EVENT_ENCODER_UP:
            audioPlayer.volumeUp();
            showVolumeOverlay();
            break;
            
        case EVENT_ENCODER_DOWN:
            audioPlayer.volumeDown();
            showVolumeOverlay();
            break;
            
        case EVENT_ENCODER_CLICK:
            currentMode = MODE_STATION_SELECT;
            refreshStationListDisplay();
            Serial.println("[MAIN] Switched to station select");
            break;
            
        default:
            break;
    }
}

void handleBluetoothInput(InputEvent event) {
    switch (event) {
        case EVENT_LONG_PRESS:
            currentMode = MODE_STATION_SELECT;
            audioPlayer.disableBluetooth();
            refreshStationListDisplay();
            Serial.println("[MAIN] Switched back to station select");
            break;
        case EVENT_ENCODER_CLICK:
        case EVENT_PLAY_PAUSE:
            audioPlayer.bluetoothPlayPause();
            break;
        case EVENT_NEXT:
            audioPlayer.bluetoothNext();
            break;
        case EVENT_PREV:
            audioPlayer.bluetoothPrevious();
            break;
            
        case EVENT_ENCODER_UP:
            audioPlayer.volumeUp();
            showVolumeOverlay();
            break;
            
        case EVENT_ENCODER_DOWN:
            audioPlayer.volumeDown();
            showVolumeOverlay();
            break;
            
        default:
            break;
    }
}

// ============================================
// Play Station
// ============================================
void playStation(uint8_t stationIdx) {
    if (stationIdx >= stationManager.getStationCount()) {
        return;
    }
    
    selectedStation = stationIdx;
    stationManager.setCurrentStation(stationIdx);
    
    const char* stationName = stationManager.getStationName(stationIdx);
    const char* stationURL = stationManager.getStationURL(stationIdx);
    StationCodec stationCodec = stationManager.getStationCodec(stationIdx);
    AudioCodec codec = (stationCodec == STATION_CODEC_AAC) ? AUDIO_CODEC_AAC : AUDIO_CODEC_MP3;
    
    Serial.printf("[MAIN] Playing: %s\n", stationName);
    
    if (audioPlayer.play(stationURL, codec)) {
        currentMode = MODE_PLAYING_RADIO;
        lastKnownPlaybackState = STATE_BUFFERING;
        strncpy(nowPlayingName, stationName, MAX_NAME_LENGTH - 1);
        nowPlayingName[MAX_NAME_LENGTH - 1] = '\0';
        nowPlayingIsDiscovered = false;
        display.showPlaying(stationName, "Buffering...", nowPlayingFooter());
    } else {
        currentMode = MODE_ERROR;
        display.showError("Failed to play station");
    }
}

// ============================================
// Station Discovery - plays anything not from the fixed stationManager
// list (browse results, favorites, recent) - always MP3 per requirements,
// which also means none of these can hit the AAC+/SBR memory crash.
// Every entry point below (browse results, favorites, recent) funnels
// through this one function rather than duplicating play logic three
// times.
// ============================================
void playDiscoveredStation(const char* name, const char* url, StationCodec codec, bool addFavorite) {
    // `name`/`url` may point into the Recent array. addToRecent() shifts
    // that array, so keep an independent copy before updating the list.
    char stationName[MAX_NAME_LENGTH];
    char stationURL[MAX_URL_LENGTH];
    strncpy(stationName, name, sizeof(stationName) - 1);
    stationName[sizeof(stationName) - 1] = '\0';
    strncpy(stationURL, url, sizeof(stationURL) - 1);
    stationURL[sizeof(stationURL) - 1] = '\0';

    codec = stationCodecForURL(stationURL, codec);
    stationManager.addToRecent(stationName, stationURL, codec);
    if (addFavorite) {
        if (stationManager.addToFavorites(stationName, stationURL, codec)) {
            Serial.printf("[MAIN] Added to favorites: %s\n", stationName);
        } else {
            Serial.printf("[MAIN] Not added to favorites (already there, or list full): %s\n", stationName);
        }
    }
    
    // Fully release the browse API's own TLS connection and free its
    // countries/tags/results arrays before starting playback - both were
    // only needed for the browse flow itself, and holding onto either
    // (the connection's TLS resources, or ~18KB of arrays) competes with
    // the audio stream's own fresh TLS connection for the same limited
    // RAM right when it matters most. Same settling-delay pattern as
    // entering Browse.
    stationBrowser.releaseConnection();
    stationBrowser.freeBrowseMemory();
    delay(200);
    
    AudioCodec audioCodec = (codec == STATION_CODEC_AAC) ? AUDIO_CODEC_AAC : AUDIO_CODEC_MP3;
    Serial.printf("[MAIN] Playing (discovered): %s (%s)\n", stationName, codec == STATION_CODEC_AAC ? "AAC" : "MP3");
    
    if (audioPlayer.play(stationURL, audioCodec)) {
        currentMode = MODE_PLAYING_RADIO;
        lastKnownPlaybackState = STATE_BUFFERING;
        strncpy(nowPlayingName, stationName, MAX_NAME_LENGTH - 1);
        nowPlayingName[MAX_NAME_LENGTH - 1] = '\0';
        nowPlayingIsDiscovered = true;
        display.showPlaying(stationName, "Buffering...", nowPlayingFooter());
    } else {
        currentMode = MODE_ERROR;
        display.showError("Failed to play station");
    }
}

// ---- Letter (alphabet-first country navigation) ----
void enterBrowseLetter() {
    // Stop any currently-playing station before making a browse-mode
    // network call. Running an ongoing audio stream (its own open
    // connection, possibly HTTPS/TLS) at the same time as this HTTPS
    // request - two TLS sessions competing for the same limited RAM,
    // no PSRAM on this board - was freezing/crashing the device. This
    // single stop() covers the whole browse flow downstream (letter ->
    // country -> genre -> results all happen after this point), so nothing
    // further into the flow needs its own stop() call.
    audioPlayer.stop();
    
    // Brief settling delay - stop() deletes the decoder/buffer objects,
    // but immediately demanding a new large contiguous allocation (a TLS
    // session) right after freeing a large one is exactly the kind of
    // sequence that can expose heap fragmentation even when total free
    // memory looks fine. This is cheap insurance either way.
    delay(200);
    Serial.printf("[MAIN] Heap after stopping playback: free=%u, largest block=%u\n",
                  ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    
    display.showLoading("Fetching countries...");
    if (stationBrowser.fetchCountries()) {
        browseIndex = 0;
        currentMode = MODE_BROWSE_LETTER;
        refreshBrowseLetterDisplay();
    } else {
        currentMode = MODE_ERROR;
        display.showError(stationBrowser.getLastError());
    }
}

void refreshBrowseLetterDisplay() {
    static const char* names[27];
    static char letterLabels[26][2];
    names[0] = "Any Country";
    for (int i = 0; i < 26; i++) {
        letterLabels[i][0] = 'A' + i;
        letterLabels[i][1] = '\0';
        names[i + 1] = letterLabels[i];
    }
    display.showStationList(names, 27, browseIndex, "Browse By Letter", "Click=Next  Long=Back");
}

void applyLetterFilter(char letter) {
    filteredCountryCount = 0;
    uint16_t total = stationBrowser.getCountryCount();
    for (uint16_t i = 0; i < total; i++) {
        const char* name = stationBrowser.getCountry(i)->name;
        if (name[0] && toupper((unsigned char)name[0]) == letter) {
            if (filteredCountryCount < MAX_BROWSE_COUNTRIES) {
                filteredCountryIndices[filteredCountryCount++] = i;
            }
        }
    }
}

void handleBrowseLetterInput(InputEvent event) {
    const uint8_t total = 27; // "Any Country" + A-Z
    switch (event) {
        case EVENT_ENCODER_UP:
            browseIndex = (browseIndex < total - 1) ? browseIndex + 1 : 0;
            refreshBrowseLetterDisplay();
            break;
        case EVENT_ENCODER_DOWN:
            browseIndex = (browseIndex > 0) ? browseIndex - 1 : (total - 1);
            refreshBrowseLetterDisplay();
            break;
        case EVENT_ENCODER_CLICK:
        case EVENT_PLAY_PAUSE:
            if (browseIndex == 0) {
                // "Any Country" - skip country selection entirely
                skippedCountrySelection = true;
                selectedCountryCode[0] = '\0';
                strncpy(selectedCountryName, "Any Country", BROWSE_COUNTRY_NAME_LEN - 1);
                selectedCountryName[BROWSE_COUNTRY_NAME_LEN - 1] = '\0';
                enterBrowseStationLetter();
            } else {
                char letter = 'A' + (browseIndex - 1);
                applyLetterFilter(letter);
                if (filteredCountryCount > 0) {
                    skippedCountrySelection = false;
                    browseIndex = 0;
                    currentMode = MODE_BROWSE_COUNTRY;
                    refreshBrowseCountryDisplay();
                } else {
                    Serial.printf("[MAIN] No countries start with '%c'\n", letter);
                }
            }
            break;
        case EVENT_LONG_PRESS:
            stationBrowser.freeBrowseMemory();
            currentMode = MODE_STATION_SELECT;
            refreshStationListDisplay();
            break;
        default:
            break;
    }
}

// ---- Country (filtered by the chosen letter) ----
void refreshBrowseCountryDisplay() {
    static const char* names[MAX_BROWSE_COUNTRIES];
    for (uint16_t i = 0; i < filteredCountryCount; i++) {
        names[i] = stationBrowser.getCountry(filteredCountryIndices[i])->name;
    }
    display.showStationList(names, (uint8_t)filteredCountryCount, browseIndex, "Select Country", "Click=Next  Long=Back");
}

void handleBrowseCountryInput(InputEvent event) {
    uint16_t count = filteredCountryCount;
    if (count == 0) {
        if (event == EVENT_LONG_PRESS) {
            currentMode = MODE_BROWSE_LETTER;
            refreshBrowseLetterDisplay();
        }
        return;
    }
    switch (event) {
        case EVENT_ENCODER_UP:
            browseIndex = (browseIndex < count - 1) ? browseIndex + 1 : 0;
            refreshBrowseCountryDisplay();
            break;
        case EVENT_ENCODER_DOWN:
            browseIndex = (browseIndex > 0) ? browseIndex - 1 : (uint8_t)(count - 1);
            refreshBrowseCountryDisplay();
            break;
        case EVENT_ENCODER_CLICK:
        case EVENT_PLAY_PAUSE: {
            BrowseCountry* c = stationBrowser.getCountry(filteredCountryIndices[browseIndex]);
            if (c) {
                strncpy(selectedCountryCode, c->code, BROWSE_COUNTRY_CODE_LEN - 1);
                selectedCountryCode[BROWSE_COUNTRY_CODE_LEN - 1] = '\0';
                strncpy(selectedCountryName, c->name, BROWSE_COUNTRY_NAME_LEN - 1);
                selectedCountryName[BROWSE_COUNTRY_NAME_LEN - 1] = '\0';
                enterBrowseStationLetter();
            }
            break;
        }
        case EVENT_LONG_PRESS:
            currentMode = MODE_BROWSE_LETTER;
            refreshBrowseLetterDisplay();
            break;
        default:
            break;
    }
}

// ---- Station-name letter (within the selected country) ----
void enterBrowseStationLetter() {
    browseIndex = 0;
    selectedStationLetter[0] = '\0'; // Popular Stations
    currentMode = MODE_BROWSE_STATION_LETTER;
    refreshBrowseStationLetterDisplay();
}

void refreshBrowseStationLetterDisplay() {
    static const char* names[27];
    static char letterLabels[26][2];
    names[0] = "Popular Stations";
    for (int i = 0; i < 26; i++) {
        letterLabels[i][0] = 'A' + i;
        letterLabels[i][1] = '\0';
        names[i + 1] = letterLabels[i];
    }
    display.showStationList(names, 27, browseIndex, "Station Letter", "Click=Next  Long=Back");
}

void handleBrowseStationLetterInput(InputEvent event) {
    const uint8_t total = 27; // Popular Stations + A-Z
    switch (event) {
        case EVENT_ENCODER_UP:
            browseIndex = (browseIndex < total - 1) ? browseIndex + 1 : 0;
            refreshBrowseStationLetterDisplay();
            break;
        case EVENT_ENCODER_DOWN:
            browseIndex = (browseIndex > 0) ? browseIndex - 1 : (total - 1);
            refreshBrowseStationLetterDisplay();
            break;
        case EVENT_ENCODER_CLICK:
        case EVENT_PLAY_PAUSE:
            if (browseIndex == 0) {
                selectedStationLetter[0] = '\0';
            } else {
                selectedStationLetter[0] = 'A' + (browseIndex - 1);
                selectedStationLetter[1] = '\0';
            }
            enterBrowseTag();
            break;
        case EVENT_LONG_PRESS:
            if (skippedCountrySelection) {
                currentMode = MODE_BROWSE_LETTER;
                refreshBrowseLetterDisplay();
            } else {
                currentMode = MODE_BROWSE_COUNTRY;
                refreshBrowseCountryDisplay();
            }
            break;
        default:
            break;
    }
}

// ---- Genre/Tag ----
void enterBrowseTag() {
    display.showLoading("Fetching genres...");
    if (stationBrowser.fetchTags()) {
        browseIndex = 0;
        currentMode = MODE_BROWSE_TAG;
        refreshBrowseTagDisplay();
    } else {
        currentMode = MODE_ERROR;
        display.showError(stationBrowser.getLastError());
    }
}

void refreshBrowseTagDisplay() {
    uint8_t count = stationBrowser.getTagCount();
    static const char* names[MAX_BROWSE_TAGS + 1];
    names[0] = "Any Genre";
    for (uint8_t i = 0; i < count; i++) {
        names[i + 1] = stationBrowser.getTag(i)->name;
    }
    display.showStationList(names, count + 1, browseIndex, "Select Genre", "Click=Next  Long=Back");
}

void handleBrowseTagInput(InputEvent event) {
    uint8_t total = stationBrowser.getTagCount() + 1; // +1 for "Any Genre"
    switch (event) {
        case EVENT_ENCODER_UP:
            browseIndex = (browseIndex < total - 1) ? browseIndex + 1 : 0;
            refreshBrowseTagDisplay();
            break;
        case EVENT_ENCODER_DOWN:
            browseIndex = (browseIndex > 0) ? browseIndex - 1 : (uint8_t)(total - 1);
            refreshBrowseTagDisplay();
            break;
        case EVENT_ENCODER_CLICK:
        case EVENT_PLAY_PAUSE:
            if (browseIndex == 0) {
                selectedTagName[0] = '\0'; // "Any Genre" - no tag filter
            } else {
                BrowseTag* t = stationBrowser.getTag(browseIndex - 1);
                if (t) {
                    strncpy(selectedTagName, t->name, BROWSE_TAG_NAME_LEN - 1);
                    selectedTagName[BROWSE_TAG_NAME_LEN - 1] = '\0';
                }
            }
            enterBrowseResults();
            break;
        case EVENT_LONG_PRESS:
            browseIndex = 0;
            currentMode = MODE_BROWSE_STATION_LETTER;
            refreshBrowseStationLetterDisplay();
            break;
        default:
            break;
    }
}

// ---- Search Results ----
void enterBrowseResults() {
    display.showLoading("Searching stations...", "(15-20 seconds)");
    if (stationBrowser.searchStations(selectedCountryCode, selectedTagName,
                                      selectedStationLetter)) {
        browseIndex = 0;
        currentMode = MODE_BROWSE_RESULTS;
        refreshBrowseResultsDisplay();
    } else {
        currentMode = MODE_ERROR;
        display.showError(stationBrowser.getLastError());
    }
}

void refreshBrowseResultsDisplay() {
    uint8_t count = stationBrowser.getResultCount();
    static const char* names[MAX_BROWSE_RESULTS];
    for (uint8_t i = 0; i < count; i++) {
        names[i] = stationBrowser.getResult(i)->name;
    }
    display.showStationList(names, count, browseIndex, "Search Results", "Click=Play Space=+Fav Long=Back");
}

void handleBrowseResultsInput(InputEvent event) {
    uint8_t count = stationBrowser.getResultCount();
    if (count == 0) {
        if (event == EVENT_LONG_PRESS) {
            browseIndex = 0;
            currentMode = MODE_BROWSE_TAG;
            refreshBrowseTagDisplay();
        }
        return;
    }
    switch (event) {
        case EVENT_ENCODER_UP:
            browseIndex = (browseIndex < count - 1) ? browseIndex + 1 : 0;
            refreshBrowseResultsDisplay();
            break;
        case EVENT_ENCODER_DOWN:
            browseIndex = (browseIndex > 0) ? browseIndex - 1 : (uint8_t)(count - 1);
            refreshBrowseResultsDisplay();
            break;
        case EVENT_ENCODER_CLICK: {
            // Play only - "should I choose to" add to favorites is the
            // separate action below, not automatic.
            BrowseResult* r = stationBrowser.getResult(browseIndex);
            if (r) playDiscoveredStation(r->name, r->url, r->codec, false);
            break;
        }
        case EVENT_PLAY_PAUSE: {
            // Play AND add to favorites - the explicit "yes, save this
            // one" gesture.
            BrowseResult* r = stationBrowser.getResult(browseIndex);
            if (r) playDiscoveredStation(r->name, r->url, r->codec, true);
            break;
        }
        case EVENT_LONG_PRESS:
            browseIndex = 0;
            currentMode = MODE_BROWSE_TAG;
            refreshBrowseTagDisplay();
            break;
        default:
            break;
    }
}

// ---- Favorites ----
void enterFavorites() {
    browseIndex = 0;
    currentMode = MODE_FAVORITES;
    refreshFavoritesDisplay();
}

void refreshFavoritesDisplay() {
    uint8_t count = stationManager.getFavoriteCount();
    static const char* names[MAX_FAVORITES];
    for (uint8_t i = 0; i < count; i++) {
        names[i] = stationManager.getFavorite(i)->name;
    }
    display.showStationList(names, count, browseIndex, "Favorites", "Click=Play Space=Remove Long=Back");
}

void handleFavoritesInput(InputEvent event) {
    uint8_t count = stationManager.getFavoriteCount();
    if (count == 0) {
        if (event == EVENT_LONG_PRESS) {
            currentMode = MODE_STATION_SELECT;
            refreshStationListDisplay();
        }
        return;
    }
    switch (event) {
        case EVENT_ENCODER_UP:
            browseIndex = (browseIndex < count - 1) ? browseIndex + 1 : 0;
            refreshFavoritesDisplay();
            break;
        case EVENT_ENCODER_DOWN:
            browseIndex = (browseIndex > 0) ? browseIndex - 1 : (uint8_t)(count - 1);
            refreshFavoritesDisplay();
            break;
        case EVENT_ENCODER_CLICK: {
            RadioStation* f = stationManager.getFavorite(browseIndex);
            if (f) playDiscoveredStation(f->name, f->url, f->codec, false); // already favorited
            break;
        }
        case EVENT_PLAY_PAUSE: {
            // Remove from favorites - deliberately a different action
            // from encoder click (play), so pruning your list doesn't
            // need a separate confirmation screen.
            if (stationManager.removeFavorite(browseIndex)) {
                Serial.println("[MAIN] Removed favorite");
                uint8_t newCount = stationManager.getFavoriteCount();
                if (browseIndex >= newCount && newCount > 0) browseIndex = newCount - 1;
                refreshFavoritesDisplay();
            }
            break;
        }
        case EVENT_LONG_PRESS:
            currentMode = MODE_STATION_SELECT;
            refreshStationListDisplay();
            break;
        default:
            break;
    }
}

// ---- Recent ----
void enterRecent() {
    browseIndex = 0;
    currentMode = MODE_RECENT;
    refreshRecentDisplay();
}

void refreshRecentDisplay() {
    uint8_t count = stationManager.getRecentCount();
    static const char* names[MAX_RECENT];
    for (uint8_t i = 0; i < count; i++) {
        names[i] = stationManager.getRecent(i)->name;
    }
    display.showStationList(names, count, browseIndex, "Recent", "Click=Play Space=+Fav Long=Back");
}

void handleRecentInput(InputEvent event) {
    uint8_t count = stationManager.getRecentCount();
    if (count == 0) {
        if (event == EVENT_LONG_PRESS) {
            currentMode = MODE_STATION_SELECT;
            refreshStationListDisplay();
        }
        return;
    }
    switch (event) {
        case EVENT_ENCODER_UP:
            browseIndex = (browseIndex < count - 1) ? browseIndex + 1 : 0;
            refreshRecentDisplay();
            break;
        case EVENT_ENCODER_DOWN:
            browseIndex = (browseIndex > 0) ? browseIndex - 1 : (uint8_t)(count - 1);
            refreshRecentDisplay();
            break;
        case EVENT_ENCODER_CLICK: {
            RadioStation* r = stationManager.getRecent(browseIndex);
            if (r) playDiscoveredStation(r->name, r->url, r->codec, false);
            break;
        }
        case EVENT_PLAY_PAUSE: {
            // Play AND add to favorites - same pattern as the Results
            // screen, for consistency.
            RadioStation* r = stationManager.getRecent(browseIndex);
            if (r) playDiscoveredStation(r->name, r->url, r->codec, true);
            break;
        }
        case EVENT_LONG_PRESS:
            currentMode = MODE_STATION_SELECT;
            refreshStationListDisplay();
            break;
        default:
            break;
    }
}

// ============================================
// UI Updates
// ============================================
void updateUI() {
    // Periodic updates
    display.update();
    
    // Update WiFi signal strength if connected
    if (wifiManager.isConnected()) {
        // Could update WiFi indicator here
    }
    
    // Detect playback state transitions (e.g. Buffering -> Playing) and
    // refresh the display to match. Previously the "Buffering..." message
    // set right after starting a stream was never replaced by anything,
    // so it stayed on screen indefinitely even once audio was actually
    // flowing - this polls the real state and catches the transition.
    if (currentMode == MODE_PLAYING_RADIO) {
        PlaybackState nowState = audioPlayer.getState();
        if (nowState != lastKnownPlaybackState) {
            lastKnownPlaybackState = nowState;
            const char* statusText = "Buffering...";
            if (nowState == STATE_PLAYING) statusText = "Playing";
            else if (nowState == STATE_PAUSED) statusText = "Paused";
            else if (nowState == STATE_STOPPED) statusText = "Stopped";
            display.showPlaying(nowPlayingName, statusText, nowPlayingFooter());
        }
    }
    
    // Update the scrolling now-playing title from ICY metadata, if the
    // current stream provides any (not every station does).
    // updateNowPlayingTitle() itself skips the redraw if the text hasn't
    // actually changed since last checked.
    if (currentMode == MODE_PLAYING_RADIO) {
        const char* npTitle = audioPlayer.getNowPlaying();
        if (npTitle && strcmp(npTitle, "Live stream") != 0) {
            display.updateNowPlayingTitle(npTitle);
        }
    }
    
    // Detect Bluetooth connection state transitions and refresh the
    // display to match. Previously the Bluetooth screen only ever redrew
    // when first entering Bluetooth mode (always "not connected" at that
    // point) or when a volume-overlay timeout happened to fire - a phone
    // connecting without the user touching the volume would never actually
    // update the screen to show it.
    if (currentMode == MODE_BLUETOOTH) {
        bool nowConnected = audioPlayer.isBluetoothConnected();
        if (nowConnected != lastKnownBluetoothConnected) {
            lastKnownBluetoothConnected = nowConnected;
            display.showBluetooth(
                nowConnected ? audioPlayer.getBluetoothDeviceName() : BT_DEVICE_NAME,
                nowConnected
            );
        }
        const char* metadata = audioPlayer.getBluetoothNowPlaying();
        if (metadata && strcmp(metadata, lastBluetoothMetadata) != 0) {
            strncpy(lastBluetoothMetadata, metadata, sizeof(lastBluetoothMetadata) - 1);
            lastBluetoothMetadata[sizeof(lastBluetoothMetadata) - 1] = '\0';
            display.updateNowPlayingTitle(lastBluetoothMetadata);
        }
    }
    
    // Auto-clear the volume overlay after its display window. Previously
    // showVolume() drew a partial-window overlay that nothing ever cleared,
    // so it stayed on screen (and covered the footer text underneath it)
    // permanently after the first volume change.
    if (volumeDisplayUntil != 0 && millis() > volumeDisplayUntil) {
        volumeDisplayUntil = 0;
        Serial.println("[SCREEN] Volume overlay cleared, back to normal screen");
        if (currentMode == MODE_PLAYING_RADIO) {
            const char* statusText = "Buffering...";
            PlaybackState st = audioPlayer.getState();
            if (st == STATE_PLAYING) statusText = "Playing";
            else if (st == STATE_PAUSED) statusText = "Paused";
            else if (st == STATE_STOPPED) statusText = "Stopped";
            display.showPlaying(nowPlayingName, statusText, nowPlayingFooter());
        } else if (currentMode == MODE_BLUETOOTH) {
            bool nowConnected = audioPlayer.isBluetoothConnected();
            display.showBluetooth(
                nowConnected ? audioPlayer.getBluetoothDeviceName() : BT_DEVICE_NAME,
                nowConnected
            );
        }
    }
}

// ============================================
// Station List Display Helper
// ============================================
// Builds a real array of station name pointers from stationManager and
// redraws the list. Previously the three call sites for this used
// `(const char**)new char*[count]` - freshly allocated, NEVER populated
// pointers - so the display was reading garbage heap memory as station
// name strings. This is what produced the "weird characters."
void refreshStationListDisplay() {
    uint8_t count = stationManager.getStationCount();
    static const char* names[MAX_STATIONS + 3];
    static char favLabel[24];
    static char recentLabel[24];

    snprintf(favLabel, sizeof(favLabel), "Favorites (%d)", stationManager.getFavoriteCount());
    snprintf(recentLabel, sizeof(recentLabel), "Recent (%d)", stationManager.getRecentCount());

    names[0] = "Browse All Stations";
    names[1] = favLabel;
    names[2] = recentLabel;
    for (uint8_t i = 0; i < count && i < MAX_STATIONS; i++) {
        names[i + 3] = stationManager.getStationName(i);
    }
    display.showStationList(names, count + 3, selectedStation);
}

// ============================================
// Volume Overlay Helper
// ============================================
// Consolidates what used to be 4 duplicated call sites (encoder up/down,
// in both radio and Bluetooth modes). Logs explicitly when the overlay is
// shown and, separately in updateUI()'s auto-clear check, when it's
// cleared - useful for confirming the show/clear cycle is actually
// happening when you can't see the physical e-paper panel directly (e.g.
// testing over remote desktop via the serial command interface).
void showVolumeOverlay() {
    uint8_t vol = audioPlayer.getVolume();
    display.updateVolume(vol);
    volumeDisplayUntil = millis() + 2000;
    Serial.printf("[SCREEN] Volume overlay shown: %d%% (clears in 2s)\n", vol);
}

// ============================================
// Error Screen Input Handling
// ============================================
// Previously there was no MODE_ERROR at all - showError() only drew text,
// it never changed currentMode. So the app silently stayed in whatever
// mode it was already in (usually MODE_STATION_SELECT), meaning a button
// press after an error just re-ran station-select logic - which, for
// Play/Click, immediately retried the same failing station and drew the
// same error again. That's the "press to continue just inverts and
// shows the same error" loop. Now any input here just returns to the
// station list without retrying anything.
void handleErrorInput(InputEvent event) {
    currentMode = MODE_STATION_SELECT;
    refreshStationListDisplay();
    Serial.println("[MAIN] Dismissed error, returned to station select");
}

// ============================================
// Remote/Headless Testing via Serial
// ============================================
// Maps single keystrokes to the exact same InputEvent values real
// hardware produces, then feeds them through the same handleInput()
// pipeline - this is not a separate/parallel control path, so anything
// that works via the physical encoder/buttons works identically here.
void checkSerialCommands() {
    if (!Serial.available()) return;
    char c = Serial.read();
    
    InputEvent event = EVENT_NONE;
    switch (c) {
        case 'u': case 'U': event = EVENT_ENCODER_UP; break;
        case 'd': case 'D': event = EVENT_ENCODER_DOWN; break;
        case 'k': case 'K': event = EVENT_ENCODER_CLICK; break;
        case ' ':           event = EVENT_PLAY_PAUSE; break;
        case 'n': case 'N': event = EVENT_NEXT; break;
        case 'p': case 'P': event = EVENT_PREV; break;
        case 'l': case 'L': event = EVENT_LONG_PRESS; break;
        case 's': case 'S':
            printCurrentScreen();
            return;
        case '\r': case '\n':
            return; // ignore bare line endings from the monitor
        default:
            Serial.printf("[SERIAL] Unknown command '%c' - press 's' for a screen dump, or see the command list printed at boot\n", c);
            return;
    }
    
    Serial.printf("[SERIAL] Simulating input: %c\n", c);
    handleInput(event);
}

// Text summary of whatever the e-paper is currently showing - reads the
// same state the display draws from, without needing to touch display.cpp
// at all. Useful when you can't physically see the panel (e.g. remote
// desktop, panel not yet wired up, etc).
void printCurrentScreen() {
    Serial.println("--- Current screen ---");
    switch (currentMode) {
        case MODE_BOOT:
            Serial.println("Booting...");
            break;
        case MODE_STATION_SELECT: {
            uint8_t count = stationManager.getStationCount();
            Serial.println("Select Station:");
            Serial.printf("  %s 1. Browse All Stations\n", selectedStation == 0 ? ">" : " ");
            Serial.printf("  %s 2. Favorites (%d)\n", selectedStation == 1 ? ">" : " ", stationManager.getFavoriteCount());
            Serial.printf("  %s 3. Recent (%d)\n", selectedStation == 2 ? ">" : " ", stationManager.getRecentCount());
            for (uint8_t i = 0; i < count; i++) {
                Serial.printf("  %s %d. %s\n",
                    (i + 3) == selectedStation ? ">" : " ",
                    i + 4,
                    stationManager.getStationName(i));
            }
            Serial.println("[Select with Encoder]");
            break;
        }
        case MODE_PLAYING_RADIO: {
            PlaybackState st = audioPlayer.getState();
            const char* statusText = "Buffering...";
            if (st == STATE_PLAYING) statusText = "Playing";
            else if (st == STATE_PAUSED) statusText = "Paused";
            else if (st == STATE_STOPPED) statusText = "Stopped";
            if (volumeDisplayUntil != 0) {
                Serial.printf("[Volume overlay currently showing: %d%%, %lums left]\n",
                    audioPlayer.getVolume(),
                    (unsigned long)(volumeDisplayUntil - millis()));
            }
            Serial.printf("%s\n", nowPlayingName);
            Serial.printf("Status: %s\n", statusText);
            Serial.printf("Now playing: %s\n", audioPlayer.getNowPlaying());
            Serial.printf("Volume: %d%%\n", audioPlayer.getVolume());
            Serial.printf("[%s]\n", nowPlayingFooter());
            break;
        }
        case MODE_BLUETOOTH: {
            bool connected = audioPlayer.isBluetoothConnected();
            Serial.println("Bluetooth Audio");
            if (connected) {
                Serial.printf("Connected to: %s\n", audioPlayer.getBluetoothDeviceName());
            } else {
                Serial.printf("Listening... (pair with: %s)\n", BT_DEVICE_NAME);
            }
            Serial.printf("Volume: %d%%\n", audioPlayer.getVolume());
            break;
        }
        case MODE_ERROR:
            Serial.println("ERROR screen (press any key to dismiss)");
            break;
        case MODE_BROWSE_LETTER: {
            Serial.println("Browse By Letter:");
            Serial.printf("  %s 1. Any Country\n", browseIndex == 0 ? ">" : " ");
            for (int i = 0; i < 26; i++) {
                Serial.printf("  %s %d. %c\n", (i + 1) == browseIndex ? ">" : " ", i + 2, 'A' + i);
            }
            Serial.println("[Click=select, Long-press=back]");
            break;
        }
        case MODE_BROWSE_COUNTRY: {
            Serial.println("Select Country:");
            for (uint16_t i = 0; i < filteredCountryCount; i++) {
                Serial.printf("  %s %d. %s\n", i == browseIndex ? ">" : " ", i + 1, stationBrowser.getCountry(filteredCountryIndices[i])->name);
            }
            Serial.println("[Click=select, Long-press=back]");
            break;
        }
        case MODE_BROWSE_STATION_LETTER: {
            Serial.println("Station Letter:");
            Serial.printf("  %s 1. Popular Stations\n", browseIndex == 0 ? ">" : " ");
            for (int i = 0; i < 26; i++) {
                Serial.printf("  %s %d. %c\n", (i + 1) == browseIndex ? ">" : " ", i + 2, 'A' + i);
            }
            Serial.println("[Click=select, Long-press=back]");
            break;
        }
        case MODE_BROWSE_TAG: {
            uint8_t count = stationBrowser.getTagCount();
            Serial.println("Select Genre:");
            Serial.printf("  %s 1. Any Genre\n", browseIndex == 0 ? ">" : " ");
            for (uint8_t i = 0; i < count; i++) {
                Serial.printf("  %s %d. %s\n", (i + 1) == browseIndex ? ">" : " ", i + 2, stationBrowser.getTag(i)->name);
            }
            Serial.println("[Click=select, Long-press=back]");
            break;
        }
        case MODE_BROWSE_RESULTS: {
            uint8_t count = stationBrowser.getResultCount();
            Serial.printf("Search Results (%s / %s):\n", selectedCountryName, selectedTagName[0] ? selectedTagName : "Any Genre");
            for (uint8_t i = 0; i < count; i++) {
                Serial.printf("  %s %d. %s\n", i == browseIndex ? ">" : " ", i + 1, stationBrowser.getResult(i)->name);
            }
            Serial.println("[Click=play, Space=play+favorite, Long-press=back]");
            break;
        }
        case MODE_FAVORITES: {
            uint8_t count = stationManager.getFavoriteCount();
            Serial.println("Favorites:");
            for (uint8_t i = 0; i < count; i++) {
                Serial.printf("  %s %d. %s\n", i == browseIndex ? ">" : " ", i + 1, stationManager.getFavorite(i)->name);
            }
            Serial.println("[Click=play, Space=remove, Long-press=back]");
            break;
        }
        case MODE_RECENT: {
            uint8_t count = stationManager.getRecentCount();
            Serial.println("Recent:");
            for (uint8_t i = 0; i < count; i++) {
                Serial.printf("  %s %d. %s\n", i == browseIndex ? ">" : " ", i + 1, stationManager.getRecent(i)->name);
            }
            Serial.println("[Click=play, Space=+favorite, Long-press=back]");
            break;
        }
        default:
            Serial.println("(unknown mode)");
            break;
    }
    Serial.println("-----------------------");
}


