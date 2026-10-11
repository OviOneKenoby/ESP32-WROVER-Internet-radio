#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <Arduino.h>
#include <esp_system.h>
#include "audio.h"
#include "allocation_failure_records.h"

#ifndef BUILD_GIT_ID
#define BUILD_GIT_ID "unknown"
#endif

namespace SystemInfo {
struct AllocationFailures {
    uint32_t count = 0, internalCount = 0;
    uint32_t lastSize = 0, lastCaps = 0, largestSize = 0;
    int registrationResult = ESP_ERR_INVALID_STATE;
};
void beginAllocationDiagnostics();
AllocationFailures allocationFailures();
uint32_t allocationFailureCount();
AllocationFailureRecords allocationFailureRecords();
const char* resetReasonName(esp_reset_reason_t reason);
const char* audioSourceName(AudioSource source);
const char* playbackStateName(PlaybackState state);
const char* audioCodecName(AudioCodec codec);
const char* bluetoothLifecycleName(BluetoothLifecycleState state);
void logBootInfo();
}

#endif
