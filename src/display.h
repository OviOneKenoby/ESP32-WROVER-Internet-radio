#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_154_D67.h>
#include "config.h"

enum DisplayState {
    DISPLAY_BOOT,
    DISPLAY_MENU,
    DISPLAY_PLAYING,
    DISPLAY_SEEKING,
    DISPLAY_BT_MODE,
    DISPLAY_SETTINGS,
    DISPLAY_ERROR,
    DISPLAY_LOADING
};

enum UIMode {
    UI_RADIO,
    UI_BLUETOOTH,
    UI_SETTINGS
};

class Display {
public:
    Display();
    ~Display();
    
    bool init();
    void update();
    
    // Display updates
    void showBoot();
    void showPlaying(const char* stationName, const char* statusText, const char* footer = "< Prev  Pause  Next >");
    void updateNowPlayingTitle(const char* title); // ICY metadata text, if any
    void showBluetooth(const char* btName, bool connected);
    void showStationList(const char** stations, uint8_t count, uint8_t selected, const char* title = "Select Station", const char* footer = "Select with Encoder");
    void showVolume(uint8_t volume);
    void showError(const char* message);
    void showLoading(const char* message, const char* message2 = nullptr);
    void showWiFiConnecting();
    void showWiFiConnected(const char* ssid, const char* ip);
    void showWiFiPortal(const char* ssid, const char* password);
    
    // State management
    void setState(DisplayState state);
    DisplayState getState() { return currentState; }
    void setMode(UIMode mode);
    UIMode getMode() { return currentMode; }
    
    // Partial updates
    void updateStatus(const char* status);
    void updateNowPlaying(const char* artist, const char* title);
    void updateVolume(uint8_t vol);
    
    // Screen control
    void sleep();
    void wake();
    void clearScreen();
    
private:
    GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> epd;
    
    DisplayState currentState;
    UIMode currentMode;
    
    // Layout helpers
    void drawHeader(const char* title);
    void drawFooter(const char* status);
    void drawCenteredText(int16_t x, int16_t y, const char* text, const GFXfont* font = nullptr);
    void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t progress);
    void drawBluetoothIcon(int16_t x, int16_t y, int16_t size);
    String truncateToFit(const String& text, int16_t maxWidth);
    
    // Header scrolling (marquee) - for station names too long to fit.
    // E-paper can't do smooth pixel scrolling like an OLED (each update is
    // a real panel refresh, too slow/flicker-prone for that), so this
    // advances in visible chunks on a timer instead.
    String headerFullText;
    bool headerNeedsScroll = false;
    uint16_t headerScrollOffset = 0;
    uint32_t headerLastScrollTime = 0;
    void updateHeaderScrollIfNeeded();
    
    // Same idea, second independent region - shared between two mutually
    // exclusive screens: the now-playing title text (ICY metadata) on the
    // Now Playing screen, and the connected device name on the Bluetooth
    // screen. Gated by currentState (DISPLAY_PLAYING or DISPLAY_BT_MODE),
    // not a separate flag, so it can't keep refreshing this screen region
    // after switching to some other screen.
    String titleFullText;
    bool titleNeedsScroll = false;
    uint16_t titleScrollOffset = 0;
    uint32_t titleLastScrollTime = 0;
    void updateTitleScrollIfNeeded();
    void drawTitleText(const char* text); // sets up titleFullText + draws initial state
    
    // Partial update tracking
    uint32_t lastUpdateTime;
    bool needsFullUpdate;
    uint8_t partialScreenRefreshes = 0;
    bool useFullScreenRefresh();
    void noteFullScreenRefresh();
};

extern Display display;

#endif // DISPLAY_H
