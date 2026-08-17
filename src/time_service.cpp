#include "time_service.h"
#include "config.h"
#include <WiFi.h>
#include <time.h>

TimeService timeService;

void TimeService::update() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (configured && isSynchronized()) return;
    if (lastConfigurationAttempt != 0 &&
        millis() - lastConfigurationAttempt < RETRY_INTERVAL_MS) return;

    lastConfigurationAttempt = millis();
    setenv("TZ", NTP_TIMEZONE, 1);
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
