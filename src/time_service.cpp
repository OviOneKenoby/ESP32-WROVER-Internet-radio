#include "time_service.h"
#include "config.h"
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>

TimeService timeService;

void TimeService::loadTimezone() {
    Preferences prefs;
    if (prefs.begin("time", false)) {
        prefs.getString("tz", timezone, sizeof(timezone));
        prefs.end();
    }
    if (!timezone[0]) {
        strncpy(timezone, DEFAULT_NTP_TIMEZONE, sizeof(timezone) - 1);
        timezone[sizeof(timezone) - 1] = '\0';
    }
    setenv("TZ", timezone, 1);
    tzset();
    Serial.printf("[TIME] Timezone: %s\n", timezone);
}

bool TimeService::setTimezone(const char* posixTimezone) {
    if (!posixTimezone || !posixTimezone[0] || strlen(posixTimezone) >= sizeof(timezone)) return false;
    for (const char* p = posixTimezone; *p; ++p) {
        if ((unsigned char)*p < 32 || *p == '"' || *p == '\\') return false;
    }
    strncpy(timezone, posixTimezone, sizeof(timezone) - 1);
    timezone[sizeof(timezone) - 1] = '\0';
    setenv("TZ", timezone, 1);
    tzset();
    Preferences prefs;
    if (!prefs.begin("time", false)) return false;
    prefs.putString("tz", timezone);
    prefs.end();
    configured = false; // request a fresh NTP sync using the new local rule
    lastConfigurationAttempt = 0;
    Serial.printf("[TIME] Timezone saved: %s\n", timezone);
    return true;
}

void TimeService::update() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (configured && isSynchronized()) return;
    if (lastConfigurationAttempt != 0 &&
        millis() - lastConfigurationAttempt < RETRY_INTERVAL_MS) return;

    lastConfigurationAttempt = millis();
    setenv("TZ", timezone[0] ? timezone : DEFAULT_NTP_TIMEZONE, 1);
    tzset();
    // configTime starts the SNTP client asynchronously. No wait loop here:
    // the UI, audio task, and web server remain responsive while it syncs.
    configTime(0, 0, NTP_SERVER_PRIMARY, NTP_SERVER_SECONDARY);
    configured = true;
    Serial.println("[TIME] NTP synchronization requested");
}

bool TimeService::isSynchronized() const {
    return time(nullptr) > 1700000000; // reject the ESP32's unsynchronized epoch
}

bool TimeService::getTimeText(char* destination, size_t size) const {
    if (!destination || size == 0 || !isSynchronized()) return false;
    struct tm localTime;
    time_t now = time(nullptr);
    if (!localtime_r(&now, &localTime)) return false;
    return strftime(destination, size, "%H:%M", &localTime) > 0;
}

bool TimeService::getDateText(char* destination, size_t size) const {
    if (!destination || size == 0 || !isSynchronized()) return false;
    struct tm localTime;
    time_t now = time(nullptr);
    if (!localtime_r(&now, &localTime)) return false;
    return strftime(destination, size, "%A, %d %B", &localTime) > 0;
}
