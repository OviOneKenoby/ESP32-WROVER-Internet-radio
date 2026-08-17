#include "display.h"
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>

// Global display object
Display display;

// ============================================
// Constructor and initialization
// ============================================
Display::Display()
    : epd(GxEPD2_154_D67(EPD_CS_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_BUSY_PIN)),
      currentState(DISPLAY_BOOT),
      currentMode(UI_RADIO),
      lastUpdateTime(0),
      needsFullUpdate(true) {
}

Display::~Display() {
    epd.hibernate();
}

bool Display::init() {
    epd.init(115200, true, 50, false);
    epd.setRotation(1);
    epd.setFont(&FreeMonoBold9pt7b);
    epd.setTextColor(GxEPD_BLACK);
    
    showBoot();
    return true;
}

void Display::update() {
    // Handle periodic updates and screen refresh
    uint32_t now = millis();
    if (now - lastUpdateTime > 1000) {
        lastUpdateTime = now;
    }
    updateHeaderScrollIfNeeded();
    updateTitleScrollIfNeeded();
}

bool Display::useFullScreenRefresh() {
    // Full-window partial redraws are cheap and do not flash, but repeated
    // ones leave residue on this panel. Every Nth complete repaint uses the
    // panel's full waveform to clear that ghosting without penalizing every
    // encoder movement or ticker step.
    if (++partialScreenRefreshes >= EPD_FULL_REFRESH_INTERVAL) {
        partialScreenRefreshes = 0;
        return true;
    }
    return false;
}

void Display::noteFullScreenRefresh() { partialScreenRefreshes = 0; }

// ============================================
// Display Functions - Boot Screen
// ============================================
void Display::showBoot() {
    currentState = DISPLAY_BOOT;
    needsFullUpdate = true;
    
    epd.setFullWindow();
    noteFullScreenRefresh();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        
        epd.setFont(&FreeMonoBold9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        
        // Title
        drawCenteredText(100, 60, "ESP32 Radio");
        
        epd.setFont(&FreeSerif9pt7b);
        epd.setTextSize(1);
        
        // Subtitle
        drawCenteredText(100, 90, "Internet + Bluetooth");
        drawCenteredText(100, 120, "Initializing...");
        
        epd.setFont(&FreeMonoBold9pt7b);
        epd.setTextSize(0);
        drawCenteredText(100, 180, "WeAct 1.54\" E-paper");
        
    } while (epd.nextPage());
}

// ============================================
// Display Functions - WiFi Connecting
// ============================================
void Display::showWiFiConnecting() {
    currentState = DISPLAY_PLAYING;
    needsFullUpdate = true;
    
    epd.setFullWindow();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        
        drawHeader("Connecting WiFi");
        
        epd.setFont(&FreeSerif9pt7b);
        drawCenteredText(100, 110, "Scanning networks...");
        
        // Draw animated dots
        static uint8_t dotCount = (millis() / 500) % 4;
        String dots = "";
        for (uint8_t i = 0; i < dotCount; i++) dots += ".";
        drawCenteredText(100, 130, dots.c_str());
        
    } while (epd.nextPage());
}

// ============================================
// Display Functions - WiFi Connected
// ============================================
void Display::showWiFiConnected(const char* ssid, const char* ip) {
    epd.setPartialWindow(0, 100, 200, 50);
    epd.firstPage();
    do {
        epd.fillRect(0, 100, 200, 50, GxEPD_WHITE);
        
        epd.setFont(&FreeSerif9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        
        drawCenteredText(100, 120, ssid);
        drawCenteredText(100, 140, ip);
        
    } while (epd.nextPage());
}

void Display::showWiFiPortal(const char* ssid, const char* password) {
    currentState = DISPLAY_SETTINGS;
    epd.setFullWindow();
    noteFullScreenRefresh();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        drawHeader("WiFi Setup Portal");
        epd.setFont(&FreeSerif9pt7b);
        drawCenteredText(100, 55, "Join WiFi:");
        drawCenteredText(100, 78, ssid);
        drawCenteredText(100, 108, "Password:");
        drawCenteredText(100, 130, password);
        drawCenteredText(100, 160, "Open 192.168.4.1");
        drawFooter("Configure WiFi from phone");
    } while (epd.nextPage());
}

void Display::showIdleClock(const char* dateText, const char* timeText, bool synchronized) {
    currentState = DISPLAY_SETTINGS;
    epd.setFullWindow();
    noteFullScreenRefresh();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        drawHeader("ESP32 Radio");
        epd.setFont(&FreeSerif9pt7b);
        drawCenteredText(100, 70, synchronized ? dateText : "Waiting for NTP time");
        epd.setFont(&FreeMonoBold9pt7b);
        drawCenteredText(100, 125, synchronized ? timeText : "--:--");
        drawFooter("Press any control to wake");
    } while (epd.nextPage());
}

void Display::updateIdleClockTime(const char* timeText) {
    epd.setPartialWindow(0, 90, 200, 50);
    epd.firstPage();
    do {
        epd.fillRect(0, 90, 200, 50, GxEPD_WHITE);
        epd.setFont(&FreeMonoBold9pt7b);
        drawCenteredText(100, 125, timeText);
    } while (epd.nextPage());
}

// ============================================
// Display Functions - Now Playing
// ============================================
void Display::showPlaying(const char* stationName, const char* statusText, const char* footer) {
    currentState = DISPLAY_PLAYING;
    currentMode = UI_RADIO;
    needsFullUpdate = true;
    
    // Partial window - this now redraws automatically on playback state
    // transitions and pause/resume, so a flashing full refresh every time
    // would be just as disruptive here as it was on the station list.
    if (useFullScreenRefresh()) epd.setFullWindow();
    else epd.setPartialWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        
        // Header with station name
        drawHeader(stationName);
        
        // Single status line reflecting the actual playback state
        // (Buffering.../Playing/Paused) - previously this always showed a
        // hardcoded "PLAYING" alongside a separate "Buffering..." placeholder
        // that nothing ever updated, so both appeared together and
        // contradicted each other regardless of what was actually happening.
        // Moved up from the previous y=110 to leave room for the now-playing
        // title text scrolling underneath it.
        epd.setFont(&FreeMonoBold9pt7b);
        drawCenteredText(100, 75, statusText);
        
        // Footer
        drawFooter(footer);
        
    } while (epd.nextPage());
    
    // Reset title scroll state for this fresh screen - updateNowPlayingTitle()
    // (called separately from main.cpp as ICY metadata arrives/changes) will
    // populate this region once/if the stream actually sends any.
    titleFullText = "";
    titleNeedsScroll = false;
}

// ============================================
// Display Functions - Bluetooth Mode
// ============================================
void Display::showBluetooth(const char* btName, bool connected) {
    currentState = DISPLAY_BT_MODE;
    currentMode = UI_BLUETOOTH;
    needsFullUpdate = true;
    
    epd.setFullWindow();
    noteFullScreenRefresh();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        
        drawHeader("Bluetooth Audio");
        
        // Icon, centered above the status text
        drawBluetoothIcon(90, 40, 20);
        
        epd.setFont(&FreeMonoBold9pt7b);
        if (connected) {
            drawCenteredText(100, 85, "Connected");
        } else {
            drawCenteredText(100, 85, "Listening...");
        }
        
        epd.setFont(&FreeSerif9pt7b);
        if (!connected) {
            drawCenteredText(100, 130, "Pair with:");
            drawCenteredText(100, 150, btName);
        }
        // If connected, the device name region (y=105-125) is populated
        // separately below, after the page loop - it needs its own
        // measure-and-possibly-scroll setup (drawTitleText()), same as the
        // now-playing title on the radio screen, not a plain draw here.
        
        drawFooter(connected ? "Streaming Audio" : "Waiting for device");
        
    } while (epd.nextPage());
    
    if (connected) {
        String label = "From: " + String(btName);
        drawTitleText(label.c_str());
    } else {
        titleFullText = "";
        titleNeedsScroll = false;
    }
}

// ============================================
// Display Functions - Station List
// ============================================
void Display::showStationList(const char** stations, uint8_t count, uint8_t selected, const char* title, const char* footer) {
    currentState = DISPLAY_MENU;
    needsFullUpdate = true;
    
    // Partial window, not full - this redraws on every encoder scroll step,
    // and setFullWindow() deliberately does a flashing black/white refresh
    // cycle (to prevent e-paper ghosting) that looked like the screen
    // "inverting" on every single scroll tick. Partial updates skip that
    // flash. Trade-off: partial updates can accumulate faint ghosting over
    // many cycles without an occasional full refresh - acceptable here
    // since this screen isn't shown continuously for very long stretches.
    if (useFullScreenRefresh()) epd.setFullWindow();
    else epd.setPartialWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        
        drawHeader(title);
        
        epd.setFont(&FreeSerif9pt7b);
        epd.setTextWrap(false); // was unset (defaults true) - long names were
                                 // wrapping onto the next row's space instead
                                 // of being clipped/truncated on one line
        
        uint8_t startIdx = selected > 2 ? selected - 2 : 0;
        uint8_t endIdx = startIdx + 4;
        if (endIdx > count) endIdx = count;
        
        uint16_t yPos = 45; // was 100 - nearly halfway down the available
                            // 20-180 header-to-footer space, leaving the
                            // top half of the list area empty regardless
                            // of how many stations there were
        for (uint8_t i = startIdx; i < endIdx; i++) {
            if (i == selected) {
                epd.fillRect(10, yPos - 12, 180, 16, GxEPD_BLACK);
                epd.setTextColor(GxEPD_WHITE);
            } else {
                epd.setTextColor(GxEPD_BLACK);
            }
            
            String prefix = String(i + 1) + ". ";
            // 180px highlight box, minus ~5px margin on each side, minus
            // the width of the "N. " prefix
            int16_t px1, py1;
            uint16_t prefixW, prefixH;
            epd.getTextBounds(prefix, 0, 0, &px1, &py1, &prefixW, &prefixH);
            int16_t maxNameWidth = 170 - prefixW;
            
            String line = prefix + truncateToFit(String(stations[i]), maxNameWidth);
            epd.setCursor(15, yPos);
            epd.println(line.c_str());
            
            yPos += 20;
        }
        
        epd.setTextWrap(true); // restore default for other screens
        epd.setTextColor(GxEPD_BLACK);
        drawFooter(footer);
        
    } while (epd.nextPage());
}

// ============================================
// Display Functions - Volume
// ============================================
void Display::showVolume(uint8_t volume) {
    epd.setPartialWindow(0, 150, 200, 50);
    epd.firstPage();
    do {
        epd.fillRect(0, 150, 200, 50, GxEPD_WHITE);
        
        epd.setFont(&FreeMonoBold9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        
        String volStr = "VOL: " + String(volume) + "%";
        drawCenteredText(100, 170, volStr.c_str());
        
        drawProgressBar(30, 180, 140, 8, volume);
        
    } while (epd.nextPage());
}

// ============================================
// Display Functions - Error
// ============================================
void Display::showError(const char* message) {
    currentState = DISPLAY_ERROR;
    needsFullUpdate = true;
    
    epd.setFullWindow();
    noteFullScreenRefresh();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        
        epd.setFont(&FreeMonoBold9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        drawCenteredText(100, 80, "ERROR");
        
        epd.setFont(&FreeSerif9pt7b);
        drawCenteredText(100, 120, message);
        
        drawFooter("Press to continue");
        
    } while (epd.nextPage());
}

// Loading indicator for the blocking radio-browser.info API calls
// (fetching countries/genres/search results) - these take a real amount
// of time over the network, and with nothing shown the display would
// just look frozen. Modeled on showError() but without the "ERROR"
// framing or a "press to continue" footer, since this clears itself
// automatically once the request finishes - nothing to dismiss.
void Display::showLoading(const char* message, const char* message2) {
    currentState = DISPLAY_LOADING;
    needsFullUpdate = true;
    
    epd.setFullWindow();
    noteFullScreenRefresh();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
        
        epd.setFont(&FreeSerif9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        drawCenteredText(100, message2 ? 90 : 100, message);
        if (message2) {
            drawCenteredText(100, 115, message2);
        }
        
    } while (epd.nextPage());
}

// ============================================
// State Management
// ============================================
void Display::setState(DisplayState state) {
    if (currentState != state) {
        currentState = state;
        needsFullUpdate = true;
    }
}

void Display::setMode(UIMode mode) {
    if (currentMode != mode) {
        currentMode = mode;
        needsFullUpdate = true;
    }
}

// ============================================
// Helper Functions
// ============================================
void Display::drawHeader(const char* title) {
    epd.drawLine(0, 20, 200, 20, GxEPD_BLACK);
    
    epd.setFont(&FreeMonoBold9pt7b);
    epd.setTextColor(GxEPD_BLACK);
    
    headerFullText = String(title);
    int16_t x1, y1;
    uint16_t w, h;
    epd.getTextBounds(headerFullText, 0, 0, &x1, &y1, &w, &h);
    
    if (w <= 190) {
        // Fits fine on one line - no scrolling needed, previous behavior.
        headerNeedsScroll = false;
        drawCenteredText(100, 16, title);
    } else {
        // Doesn't fit - set up marquee state. update() (called
        // periodically) will advance and redraw this via
        // updateHeaderScrollIfNeeded(). Draw the starting position now so
        // something correct is visible immediately rather than waiting for
        // the first scroll tick.
        headerNeedsScroll = true;
        headerScrollOffset = 0;
        headerLastScrollTime = millis();
        epd.setTextWrap(false);
        epd.setCursor(5, 16);
        epd.print(headerFullText.c_str());
        epd.setTextWrap(true);
    }
}

void Display::updateHeaderScrollIfNeeded() {
    if (!headerNeedsScroll) return;
    if (millis() - headerLastScrollTime < 1300) return; // step interval
    headerLastScrollTime = millis();
    
    // Loop the text with a separator so it reads as a continuous marquee
    // rather than abruptly jumping back to the start.
    String looped = headerFullText + "   *   ";
    headerScrollOffset++;
    if (headerScrollOffset >= looped.length()) {
        headerScrollOffset = 0;
    }
    String rotated = looped.substring(headerScrollOffset) + looped.substring(0, headerScrollOffset);
    
    // Partial refresh of just the header text strip, not the whole screen -
    // this runs repeatedly while a long name is showing, so it needs to be
    // cheap and non-flashing (see showStationList/showPlaying notes on why
    // partial vs full window matters here).
    epd.setPartialWindow(0, 2, 200, 18);
    epd.firstPage();
    do {
        epd.fillRect(0, 2, 200, 18, GxEPD_WHITE);
        epd.setFont(&FreeMonoBold9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        epd.setTextWrap(false);
        epd.setCursor(5, 16);
        epd.print(rotated.c_str());
    } while (epd.nextPage());
    epd.setTextWrap(true);
}

void Display::updateNowPlayingTitle(const char* title) {
    if (currentState != DISPLAY_PLAYING && currentState != DISPLAY_BT_MODE) return;
    if (titleFullText == String(title)) return;  // no change, skip redraw
    drawTitleText(title);
}

void Display::drawTitleText(const char* text) {
    titleFullText = String(text);
    titleScrollOffset = 0;
    titleLastScrollTime = millis();
    
    epd.setFont(&FreeSerif9pt7b);
    int16_t x1, y1;
    uint16_t w, h;
    epd.getTextBounds(titleFullText, 0, 0, &x1, &y1, &w, &h);
    
    epd.setPartialWindow(0, 105, 200, 20);
    epd.firstPage();
    do {
        epd.fillRect(0, 105, 200, 20, GxEPD_WHITE);
        epd.setFont(&FreeSerif9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        if (w <= 190) {
            titleNeedsScroll = false;
            drawCenteredText(100, 120, titleFullText.c_str());
        } else {
            titleNeedsScroll = true;
            epd.setTextWrap(false);
            epd.setCursor(5, 120);
            epd.print(titleFullText.c_str());
            epd.setTextWrap(true);
        }
    } while (epd.nextPage());
}

void Display::updateTitleScrollIfNeeded() {
    if (currentState != DISPLAY_PLAYING && currentState != DISPLAY_BT_MODE) return;
    if (!titleNeedsScroll) return;
    if (millis() - titleLastScrollTime < 1300) return; // step interval
    titleLastScrollTime = millis();
    
    String looped = titleFullText + "   *   ";
    titleScrollOffset++;
    if (titleScrollOffset >= looped.length()) {
        titleScrollOffset = 0;
    }
    String rotated = looped.substring(titleScrollOffset) + looped.substring(0, titleScrollOffset);
    
    epd.setPartialWindow(0, 105, 200, 20);
    epd.firstPage();
    do {
        epd.fillRect(0, 105, 200, 20, GxEPD_WHITE);
        epd.setFont(&FreeSerif9pt7b);
        epd.setTextColor(GxEPD_BLACK);
        epd.setTextWrap(false);
        epd.setCursor(5, 120);
        epd.print(rotated.c_str());
    } while (epd.nextPage());
    epd.setTextWrap(true);
}

void Display::drawFooter(const char* status) {
    epd.drawLine(0, 180, 200, 180, GxEPD_BLACK);
    
    epd.setFont(&FreeSerif9pt7b);
    epd.setTextColor(GxEPD_BLACK);
    epd.setTextSize(0);
    
    int16_t x16, y16;
    uint16_t w, h;
    epd.getTextBounds(status, 0, 0, &x16, &y16, &w, &h);
    epd.setCursor((200 - w) / 2, 195);
    epd.println(status);
}

void Display::drawCenteredText(int16_t x, int16_t y, const char* text, const GFXfont* font) {
    if (font) epd.setFont(font);
    
    int16_t x16, y16;
    uint16_t w, h;
    epd.getTextBounds(text, 0, 0, &x16, &y16, &w, &h);
    
    epd.setCursor(x - w / 2, y);
    epd.println(text);
}

void Display::drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t progress) {
    // Border
    epd.drawRect(x, y, w, h, GxEPD_BLACK);
    
    // Fill
    if (progress > 0) {
        uint16_t fillW = (w - 2) * progress / 100;
        epd.fillRect(x + 1, y + 1, fillW, h - 2, GxEPD_BLACK);
    }
}

void Display::drawBluetoothIcon(int16_t x, int16_t y, int16_t size) {
    // The classic Bluetooth "bind rune" logo shape, built from 5 line
    // segments - no bitmap/glyph asset needed for this on a monochrome
    // e-paper display. (x,y) is the top-left of the icon's bounding box.
    int16_t midX = x + size / 2;
    int16_t rightX = x + (size * 4) / 5;
    int16_t leftX = x + size / 5;
    int16_t topY = y;
    int16_t upperY = y + size / 4;
    int16_t lowerY = y + (size * 3) / 4;
    int16_t bottomY = y + size;
    
    epd.drawLine(midX, topY, midX, bottomY, GxEPD_BLACK);       // vertical spine
    epd.drawLine(midX, topY, rightX, upperY, GxEPD_BLACK);      // top to upper-right
    epd.drawLine(rightX, upperY, leftX, lowerY, GxEPD_BLACK);   // cross to lower-left
    epd.drawLine(midX, bottomY, rightX, lowerY, GxEPD_BLACK);   // bottom to lower-right
    epd.drawLine(rightX, lowerY, leftX, upperY, GxEPD_BLACK);   // cross to upper-left
}

String Display::truncateToFit(const String& text, int16_t maxWidth) {
    int16_t x1, y1;
    uint16_t w, h;
    epd.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    if (w <= (uint16_t)maxWidth) {
        return text;
    }
    
    // Doesn't fit - trim characters and re-measure with "..." appended
    // until it does. This is what showStationList() was missing entirely -
    // it previously drew the raw, unmeasured string, and Adafruit GFX's
    // default text wrap would wrap long names onto the next line's space,
    // visually overlapping with whatever was drawn there.
    String truncated = text;
    while (truncated.length() > 1) {
        truncated.remove(truncated.length() - 1);
        String withEllipsis = truncated + "...";
        epd.getTextBounds(withEllipsis, 0, 0, &x1, &y1, &w, &h);
        if (w <= (uint16_t)maxWidth) {
            return withEllipsis;
        }
    }
    return truncated;
}

// ============================================
// Screen Control
// ============================================
void Display::sleep() {
    epd.powerOff();
}

void Display::wake() {
    // powerOff() only cuts panel driving voltage, it doesn't hibernate the
    // controller - the library's own documented way to resume from that
    // state is re-running init() with initial=false (no full reset needed
    // since power was kept). There is no separate powerOn() in this library.
    epd.init(115200, false);
}

void Display::clearScreen() {
    epd.setFullWindow();
    noteFullScreenRefresh();
    epd.firstPage();
    do {
        epd.fillScreen(GxEPD_WHITE);
    } while (epd.nextPage());
}

void Display::updateStatus(const char* status) {
    // Partial update for status bar
}

void Display::updateNowPlaying(const char* artist, const char* title) {
    // Partial update for now playing info
}

void Display::updateVolume(uint8_t vol) {
    showVolume(vol);
}
