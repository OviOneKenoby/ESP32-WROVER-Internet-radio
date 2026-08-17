#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// DISPLAY CONFIGURATION (WeAct 1.54" E-paper)
// ============================================
// Retargeted for the genuine ESP32-WROVER-DEV board (ESP32-WROVER-E
// module) - a much simpler situation than the ESP32-CAM plan this
// project briefly targeted: no camera interface, no onboard flash LED,
// no onboard status LED sharing a GPIO. The only real restriction here
// is GPIO16/17, which this chip's PSRAM uses internally (confirmed via
// Espressif's own ESP-WROVER-KIT docs: "GPIO16 and GPIO17 are used as
// chip select and clock signals for PSRAM... not broken out to the
// board's pin headers"). Every other pin below is unrestricted normal
// GPIO on this board.
#define EPD_CS_PIN      5
#define EPD_CLK_PIN     18   // SCK
#define EPD_MOSI_PIN    23   // SDA/MOSI
#define EPD_RES_PIN     19   // was 16 on the original DevKit V1 config -
                              // moved because 16 is one of this board's
                              // two PSRAM-reserved pins
#define EPD_DC_PIN      21   // was 17 - same reason, PSRAM-reserved
#define EPD_BUSY_PIN    4    // BUSY - back to its original DevKit V1
                              // pin; the GPIO4-is-the-flash-LED conflict
                              // was specific to the ESP32-CAM board and
                              // doesn't apply here

#define DISPLAY_WIDTH   200
#define DISPLAY_HEIGHT  200
#define EPD_FULL_REFRESH_INTERVAL  12  // whole-screen partial redraws

// ============================================
// AUDIO OUTPUT CONFIGURATION (PCM5102A DAC)
// ============================================
#define I2S_BCK_PIN     26   // Bit Clock
#define I2S_WS_PIN      25   // Word Select (LRCK)
#define I2S_DIN_PIN     33   // Data In - back to its original DevKit V1
                              // pin; the GPIO33-is-the-onboard-LED
                              // conflict was specific to the ESP32-CAM
                              // board and doesn't apply here
#define I2S_PORT        I2S_NUM_0

// PCM5102A settings
#define SAMPLE_RATE     44100
#define I2S_CHANNELS    2    // Stereo
#define I2S_BITS        16

// ============================================
// BUTTON CONFIGURATION
// ============================================
#define BUTTON_PLAY_PIN     34   // GPIO34 (Input only)
#define BUTTON_NEXT_PIN     35   // GPIO35 (Input only)
#define BUTTON_PREV_PIN     39   // GPIO39 (Input only)

#define BUTTON_DEBOUNCE_MS  50

// ============================================
// ROTARY ENCODER CONFIGURATION
// ============================================
#define ENCODER_CLK_PIN     27   // GPIO27 - normal GPIO, has a working
                                  // internal pull-up. Moved from GPIO36
                                  // (input-only, no internal pull-up
                                  // possible - see CHANGELOG.md item 5/9)
                                  // to here. IMPORTANT: this value must
                                  // always match wherever the CLK wire is
                                  // physically connected - a mismatch here
                                  // is what caused the "encoder moves on
                                  // its own" symptom to persist even after
                                  // rewiring to GPIO27, because the code
                                  // was still reading the now-disconnected
                                  // GPIO36 instead.
#define ENCODER_DT_PIN      32   // GPIO32
#define ENCODER_SW_PIN      14   // GPIO14 (Encoder click/push)

#define ENCODER_RESOLUTION  4    // Full steps (4 changes per click)

// Many common rotary encoder modules produce 2 real electrical edges per
// mechanical detent (not electrical noise - confirmed by this persisting
// even on a GPIO with a proper working pull-up). This divides the raw
// edge count so one physical click = one logical UP/DOWN event. If your
// specific encoder produces 1 or 4 edges per detent instead, adjust this.
#define ENCODER_STEPS_PER_DETENT  2

// ============================================
// WiFi & NETWORK CONFIGURATION
// ============================================
#define MAX_SSID_LENGTH     32
#define MAX_PASS_LENGTH     64

// Set these to your actual network before flashing. This device has no
// keyboard (only the encoder + 3 buttons), so there's no on-device way to
// type credentials - the first-boot connection uses these directly. Once
// connected successfully, the SSID/password are saved to EEPROM and every
// later boot reconnects from there instead (see WiFiManager::reconnect()),
// so you only need these correct for the very first boot after flashing.
#define WIFI_SSID           "YourNetworkName"
#define WIFI_PASSWORD       "YourNetworkPassword"

// Web configuration portal. It is hosted on the normal Wi-Fi address when
// connected, or on a temporary WPA2-protected setup AP at 192.168.4.1 when
// a saved connection fails. The AP password is derived from the ESP32 MAC at
// runtime and shown on the serial monitor/display; it is never hard-coded.
#define WEB_SERVER_PORT      80
#define PORTAL_AP_PREFIX     "ESP32-Radio-Setup-"

// Idle clock: Europe/Bucharest, including its DST transition rules. NTP is
// configured only while Wi-Fi is connected; normal radio operation never
// waits for a time-server response.
#define DEFAULT_NTP_TIMEZONE "EET-2EEST,M3.5.0/3,M10.5.0/4" // Bucharest
#define NTP_SERVER_PRIMARY   "pool.ntp.org"
#define NTP_SERVER_SECONDARY "time.google.com"
#define IDLE_CLOCK_DELAY_MS  60000UL

// ============================================
// BLUETOOTH CONFIGURATION
// ============================================
#define BT_DEVICE_NAME      "ESP32-Radio"

// Station discovery API (radio-browser.info). Officially, clients are
// asked not to hardcode one mirror and instead discover healthy servers
// via a DNS lookup of all.api.radio-browser.info - implementing that
// properly would mean parsing multiple raw DNS A records, which is a
// meaningful amount of added complexity for a real but small risk (this
// specific mirror going down). Using the single mirror directly as a
// deliberate simplification, matching what was actually tested working.
// Station discovery API (radio-browser.info). Officially, clients are
// asked not to hardcode one mirror and instead discover healthy servers
// via a DNS lookup of all.api.radio-browser.info - implementing that
// properly would mean parsing multiple raw DNS A records, which is a
// meaningful amount of added complexity. As a simpler middle ground,
// a small list of real, known mirrors (confirmed via search during
// development, not guessed) is tried in sequence if one fails - see
// StationBrowser::httpsGetJson() - rather than a single hardcoded host
// with no fallback at all, which turned out to matter in practice (a
// real 503 from de1 specifically, confirmed by direct testing).
#define RADIO_BROWSER_MIRROR_COUNT 5
static const char* const RADIO_BROWSER_MIRRORS[RADIO_BROWSER_MIRROR_COUNT] = {
    "de1.api.radio-browser.info",
    "de2.api.radio-browser.info",
    "fi1.api.radio-browser.info",
    "nl1.api.radio-browser.info",
    "at1.api.radio-browser.info"
};
#define RADIO_BROWSER_USER_AGENT  "ESP32-InternetRadio/1.0"
#define BT_DISCOVERABLE     true

// ============================================
// RADIO STREAMING
// ============================================
#define BUFFER_SIZE         4096
#define MAX_STATIONS        15
#define MAX_URL_LENGTH      256
#define MAX_NAME_LENGTH     64

// Station discovery (radio-browser.info) - browsing by country/genre.
// Fixed-size arrays, matching this project's existing style (no
// dynamic containers on an embedded target with this little RAM).
// Sized as fixed static allocations - a different, much smaller/safer
// kind of memory cost than the dynamic TLS/decode-time pressure that
// caused the earlier OOM crash (see round 23/25 in CHANGELOG.md).
#define MAX_BROWSE_COUNTRIES   250   // radio-browser.info has ~250ish
#define MAX_BROWSE_TAGS        40    // top N most popular tags only
#define MAX_BROWSE_RESULTS     20    // per search - was 30; reduced since
                                       // a smaller cap directly shrinks
                                       // the response body size that needs
                                       // to be buffered, giving more
                                       // margin within this board's
                                       // confirmed-tight, fragmented heap.
                                       // Still plenty for a small e-paper
                                       // list scrolled one at a time.

// Response buffer for browse API calls - reserved upfront to a fixed
// capacity (see StationBrowser::httpsGetJson()) rather than letting a
// String grow incrementally as data streams in, which was the confirmed
// cause of "IncompleteInput" parse errors: a reallocation failing to find
// a big enough contiguous block partway through receiving a chunked
// response. 24KB comfortably covers the largest observed response
// (~29-35KB before the MAX_BROWSE_RESULTS reduction above; smaller now)
// with real margin, while still being small enough to actually fit
// within this board's typical ~45KB largest-contiguous-block reality.
#define RESPONSE_BUFFER_RESERVE  24576
#define MAX_RECENT              5
#define MAX_FAVORITES          10

#define BROWSE_COUNTRY_NAME_LEN  40
#define BROWSE_COUNTRY_CODE_LEN   3  // ISO 3166-1 alpha-2 + null
#define BROWSE_TAG_NAME_LEN      32

// ============================================
// SYSTEM CONFIGURATION
// ============================================
#define SERIAL_BAUD         115200
#define CORE_DEBUG_LEVEL    1

// Task priorities
#define TASK_PRIORITY_HIGH      24
#define TASK_PRIORITY_NORMAL    10
#define TASK_PRIORITY_LOW       1

// Dedicated priority for the audio streaming task - deliberately NOT using
// TASK_PRIORITY_HIGH (24). That task runs a loop with no yield point while
// stream data keeps arriving (which for a continuous radio stream is
// almost always true), and at priority 24 it could plausibly starve the
// WiFi stack's own internal processing on a shared core - which would show
// up as exactly this symptom: new connections failing/timing out once the
// audio task actually has real, continuous work to do.
// Higher than the main loop task's default priority (1) - audio decode
// is genuinely time-sensitive (missing timing causes audible glitches,
// unlike a briefly sluggish UI). Safe to prioritize because
// audioTaskFunc() now yields explicitly on a bounded time interval (see
// its comments) rather than either never yielding (the original
// starvation bug) or yielding after every single decode call (which
// throttled real decode throughput below what real-time playback needs -
// a bug from the previous round's fix). An explicit, deterministic
// bounded yield doesn't depend on assumptions about same-priority
// round-robin scheduling behavior that aren't fully verifiable without
// live hardware access to check exact FreeRTOS tick configuration.
#define TASK_PRIORITY_AUDIO     5

// Stack sizes (bytes)
#define AUDIO_TASK_STACK        8192
#define UI_TASK_STACK           4096
#define NETWORK_TASK_STACK      4096

#endif // CONFIG_H
