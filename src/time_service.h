#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <Arduino.h>

class TimeService {
public:
    void update();
    bool isSynchronized() const;
    bool getTimeText(char* destination, size_t size) const;
    bool getDateText(char* destination, size_t size) const;

private:
    bool configured = false;
    uint32_t lastConfigurationAttempt = 0;
    static constexpr uint32_t RETRY_INTERVAL_MS = 60000UL;
};

extern TimeService timeService;

#endif
