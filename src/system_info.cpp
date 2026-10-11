#include "system_info.h"
#include "config.h"
#include <esp_heap_caps.h>
#include <esp_debug_helpers.h>
#include <esp_spi_flash.h>
#include <soc/soc_memory_layout.h>

namespace SystemInfo {

namespace {
portMUX_TYPE allocationMux = portMUX_INITIALIZER_UNLOCKED;
DRAM_ATTR AllocationFailures failures;
DRAM_ATTR AllocationFailureRecords records;
void IRAM_ATTR copyName(char* output, size_t capacity, const char* input) {
    size_t i = 0;
    if (input) while (i + 1 < capacity && input[i]) { output[i] = input[i]; ++i; }
    output[i] = '\0';
}
// Fixed loops/storage only. Task getters and non-printing backtrace helpers
// were inspected in IDF 4.4.7. No allocator, logger, heap query or network call.
// Do not walk an ISR stack or dereference flash text when its cache is off.
void IRAM_ATTR allocationFailed(size_t size, uint32_t caps, const char* function) {
    AllocationFailureRecord record;
    record.size = size; record.caps = caps;
    record.core = xPortGetCoreID();
    record.isr = xPortInIsrContext();
    record.cacheEnabled = spi_flash_cache_enabled();
    if (record.cacheEnabled) {
        copyName(record.allocator, sizeof(record.allocator), function);
        record.tick = xTaskGetTickCountFromISR();
        if (!record.isr) {
            const TaskHandle_t task = xTaskGetCurrentTaskHandleForCPU(record.core);
            record.taskHandle = reinterpret_cast<uint32_t>(task);
            if (task) copyName(record.task, sizeof(record.task), pcTaskGetName(task));
            esp_backtrace_frame_t frame = {};
            esp_backtrace_get_start(&frame.pc, &frame.sp, &frame.next_pc);
            for (unsigned i = 0; i < AllocationFailureRecord::maxFrames; ++i) {
                if (!esp_stack_ptr_is_sane(frame.sp)) break;
                const uint32_t pc = esp_cpu_process_stack_pc(frame.pc);
                if (!esp_ptr_executable(reinterpret_cast<void*>(pc))) break;
                record.pcs[record.frameCount++] = pc;
                if (!frame.next_pc || !esp_backtrace_get_next_frame(&frame)) break;
            }
        }
    }
    portENTER_CRITICAL(&allocationMux);
    ++failures.count;
    if (caps & MALLOC_CAP_INTERNAL) ++failures.internalCount;
    failures.lastSize = size;
    failures.lastCaps = caps;
    if (size > failures.largestSize) failures.largestSize = size;
    records.append(record); // always_inline; no flash helper in cache-off path
    portEXIT_CRITICAL(&allocationMux);
}
}

void beginAllocationDiagnostics() {
    const esp_err_t result = heap_caps_register_failed_alloc_callback(allocationFailed);
    portENTER_CRITICAL(&allocationMux);
    failures.registrationResult = result;
    portEXIT_CRITICAL(&allocationMux);
}

AllocationFailures allocationFailures() {
    portENTER_CRITICAL(&allocationMux);
    const AllocationFailures copy = failures;
    portEXIT_CRITICAL(&allocationMux);
    return copy;
}

AllocationFailureRecords allocationFailureRecords() {
    portENTER_CRITICAL(&allocationMux);
    const AllocationFailureRecords copy = records;
    portEXIT_CRITICAL(&allocationMux);
    return copy;
}

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
    switch (codec) {
        case AUDIO_CODEC_MP3: return "MP3";
        case AUDIO_CODEC_AAC: return "AAC";
        case AUDIO_CODEC_SBC: return "SBC";
        case AUDIO_CODEC_NONE:
        default: return "none";
    }
}

const char* bluetoothLifecycleName(BluetoothLifecycleState state) {
    switch (state) {
        case BT_LIFECYCLE_STARTING: return "starting";
        case BT_LIFECYCLE_ACTIVE: return "active";
        case BT_LIFECYCLE_STOPPING: return "stopping";
        case BT_LIFECYCLE_CLEANUP_FAILED: return "cleanup_failed";
        case BT_LIFECYCLE_INACTIVE:
        default: return "inactive";
    }
}

void logBootInfo() {
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.printf("[SYSTEM] Firmware %s, build %s\n", FIRMWARE_VERSION, BUILD_GIT_ID);
    Serial.printf("[SYSTEM] Reset reason: %s (%d)\n", resetReasonName(reason), (int)reason);
    Serial.printf("[SYSTEM] Flash: %u bytes; PSRAM: %u bytes\n",
                  ESP.getFlashChipSize(), ESP.getPsramSize());
}

}
