#include "system_info.h"
#include "config.h"
#include <esp_heap_caps.h>

namespace SystemInfo {

const char* resetReasonName(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON: return "power_on";
        case ESP_RST_EXT: return "external_pin";
        case ESP_RST_SW: return "software";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "interrupt_watchdog";
        case ESP_RST_TASK_WDT: return "task_watchdog";
        case ESP_RST_WDT: return "watchdog";
        case ESP_RST_DEEPSLEEP: return "deep_sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "sdio";
        case ESP_RST_UNKNOWN:
        default: return "unknown";
    }
}

const char* audioSourceName(AudioSource source) {
    switch (source) {
        case AUDIO_SOURCE_INTERNET_RADIO: return "internet_radio";
        case AUDIO_SOURCE_BLUETOOTH: return "bluetooth";
        case AUDIO_SOURCE_NONE:
        default: return "none";
    }
}

const char* playbackStateName(PlaybackState state) {
    switch (state) {
        case STATE_PLAYING: return "playing";
        case STATE_PAUSED: return "paused";
        case STATE_BUFFERING: return "buffering";
        case STATE_STOPPED:
        default: return "stopped";
    }
}

const char* audioCodecName(AudioCodec codec) {
    return codec == AUDIO_CODEC_AAC ? "AAC" : "MP3";
}

void logBootInfo() {
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.printf("[SYSTEM] Firmware %s, build %s\n", FIRMWARE_VERSION, BUILD_GIT_ID);
    Serial.printf("[SYSTEM] Reset reason: %s (%d)\n", resetReasonName(reason), (int)reason);
    Serial.printf("[SYSTEM] Flash: %u bytes; PSRAM: %u bytes\n",
                  ESP.getFlashChipSize(), ESP.getPsramSize());
}

}
