#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <Arduino.h>
#include <esp_system.h>
#include "audio.h"

#ifndef BUILD_GIT_ID
#define BUILD_GIT_ID "unknown"
#endif

namespace SystemInfo {
const char* resetReasonName(esp_reset_reason_t reason);
const char* audioSourceName(AudioSource source);
const char* playbackStateName(PlaybackState state);
const char* audioCodecName(AudioCodec codec);
void logBootInfo();
}

#endif
