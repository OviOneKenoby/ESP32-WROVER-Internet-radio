#include "audio.h"
#include "config.h"
#include "correctness_guards.h"
#include <driver/i2s.h> // legacy I2S API - still needed here for
                         // i2s_pin_config_t/I2S_PIN_NO_CHANGE used by
                         // enableBluetooth() below (the A2DP library uses
                         // this older API; AudioOutputI2S uses the newer
                         // i2s_std.h channel API internally instead, which
                         // is why audio.h itself no longer needs this include)
#include <BluetoothA2DPSink.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_err.h>
#include <esp_heap_caps.h>

// Global audio player
AudioPlayer audioPlayer;

// The pinned ESP32-A2DP start()/end() API is void, and its default I2S
// wrapper discards begin()/end() results. These small wrappers preserve the
// same library implementation while exposing enough postconditions for the
// application to avoid reporting a Bluetooth mode that did not start or did
// not release I2S cleanly.
class CheckedBluetoothOutput : public BluetoothA2DPOutputLegacy {
public:
    bool begin() override {
        beginAttempted = true;
        beginSucceeded = BluetoothA2DPOutputLegacy::begin();
        active = beginSucceeded;
        return beginSucceeded;
    }

    void end() override {
        if (!beginAttempted || (!active && endAttempted)) return;

        endAttempted = true;
        const esp_err_t result = i2s_driver_uninstall(i2s_port);
        // If begin() failed before installing the driver, INVALID_STATE means
        // that there is no I2S driver left to release, which is a clean state.
        endSucceeded = result == ESP_OK ||
                       (!beginSucceeded && result == ESP_ERR_INVALID_STATE);
        if (!endSucceeded) {
            Serial.printf("[AUDIO] Bluetooth I2S release failed: %s (%d)\n",
                          esp_err_to_name(result), (int)result);
        }
        active = !endSucceeded;
    }

    void resetForStart() {
        beginAttempted = false;
        beginSucceeded = false;
        endAttempted = false;
        endSucceeded = true;
        active = false;
    }

    bool didBegin() const { return beginAttempted && beginSucceeded; }
    bool didEnd() const { return endAttempted && endSucceeded; }
    bool isActive() const { return active; }

private:
    bool beginAttempted = false;
    bool beginSucceeded = false;
    bool endAttempted = false;
    bool endSucceeded = true;
    bool active = false;
};

class CheckedBluetoothSink : public BluetoothA2DPSink {
public:
    explicit CheckedBluetoothSink(CheckedBluetoothOutput& checkedOutput)
        : checkedOutput(checkedOutput) {
        set_output(checkedOutput);
    }

    bool startChecked(const char* name) {
        if (is_start_disabled || app_task_queue != nullptr ||
            app_task_handle != nullptr) {
            Serial.printf(
                "[AUDIO] Bluetooth start rejected: disabled=%s, queue=%s, task=%s\n",
                is_start_disabled ? "true" : "false",
                app_task_queue ? "present" : "none",
                app_task_handle ? "present" : "none");
            return false;
        }

        bluetoothInitSucceeded = false;
        i2sInitSucceeded = false;
        stackEventCompleted = false;
        checkedOutput.resetForStart();
        BluetoothA2DPSink::start(name);

        // start() queues the profile setup on the library task. Wait for the
        // virtual stack handler to return before claiming discoverability or
        // attempting cleanup after a partial start.
        const uint32_t stackDeadline = millis() + 1000;
        while (app_task_handle != nullptr && !stackEventCompleted &&
               (int32_t)(millis() - stackDeadline) < 0) {
            delay(10);
        }

        const bool controllerReady =
            esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED;
        const bool bluedroidReady =
            esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED;
        const bool taskReady = app_task_queue != nullptr &&
                               app_task_handle != nullptr;
        const bool started = bluetoothInitSucceeded && i2sInitSucceeded &&
                             controllerReady && bluedroidReady && taskReady &&
                             stackEventCompleted;

        if (!started) {
            Serial.printf(
                "[AUDIO] Bluetooth start failed: bt=%s, i2s=%s, controller=%s, bluedroid=%s, task=%s, stack-event=%s\n",
                bluetoothInitSucceeded ? "ok" : "failed",
                i2sInitSucceeded ? "ok" : "failed",
                controllerReady ? "enabled" : "not-enabled",
                bluedroidReady ? "enabled" : "not-enabled",
                taskReady ? "ready" : "missing",
                stackEventCompleted ? "complete" : "timeout");
        }
        return started;
    }

    bool stopRestartableChecked() {
        if (is_start_disabled) {
            Serial.println("[AUDIO] Bluetooth cleanup cannot restore memory released by end(true)");
            return false;
        }

        // This is the pinned library's end(false) sequence with return values
        // retained instead of discarded. It intentionally does not disable,
        // deinitialize or release the controller/Bluedroid base stack.
        is_autoreconnect_allowed = false;
        clean_last_connection();

        bool disconnectComplete = true;
        if (is_connected()) {
            disconnect();
            int limit = A2DP_DISCONNECT_LIMIT;
            while (get_connection_state() != ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
                delay_ms(100);
                if (limit-- < 0) {
                    disconnectComplete = false;
                    Serial.println("[AUDIO] Bluetooth disconnect timed out");
                    break;
                }
            }
        }

        delay_ms(50);
        const esp_err_t avrcControllerResult = esp_avrc_ct_deinit();
        delay_ms(50);
        const esp_err_t avrcTargetResult = esp_avrc_tg_deinit();
        delay_ms(50);
        const esp_err_t a2dpResult = esp_a2d_sink_deinit();
        delay_ms(50);

        const bool profilesExpected = stackEventCompleted;
        const bool avrcControllerStopped =
            deinitResultIsClean(avrcControllerResult, profilesExpected,
                                "AVRCP controller");
        const bool avrcTargetStopped =
            deinitResultIsClean(avrcTargetResult, profilesExpected,
                                "AVRCP target");
        const bool a2dpStopped =
            deinitResultIsClean(a2dpResult, profilesExpected, "A2DP sink");

        app_task_shut_down();
        delay_ms(50);
        if (is_output) checkedOutput.end();
        is_i2s_active = checkedOutput.isActive();

        const bool taskStopped = app_task_queue == nullptr &&
                                 app_task_handle == nullptr;
        const bool i2sReleased = checkedOutput.didEnd() &&
                                 !checkedOutput.isActive();
        const bool controllerRetained =
            esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED;
        const bool bluedroidRetained =
            esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED;
        const bool profilesStopped = disconnectComplete &&
                                     avrcControllerStopped &&
                                     avrcTargetStopped && a2dpStopped;
        const bool stopped = profilesStopped && taskStopped && i2sReleased &&
                             controllerRetained && bluedroidRetained &&
                             !is_start_disabled;

        if (!stopped) {
            Serial.printf(
                "[AUDIO] Bluetooth cleanup incomplete: profiles=%s, task=%s, i2s=%s, controller=%s, bluedroid=%s, restart-disabled=%s\n",
                profilesStopped ? "stopped" : "error",
                taskStopped ? "stopped" : "present",
                i2sReleased ? "released" : "not-released",
                controllerRetained ? "enabled" : "not-enabled",
                bluedroidRetained ? "enabled" : "not-enabled",
                is_start_disabled ? "true" : "false");
        }
        stackEventCompleted = false;
        return stopped;
    }

    bool isRestartable() const { return !is_start_disabled; }

protected:
    int init_bluetooth() override {
        const int result = BluetoothA2DPSink::init_bluetooth();
        bluetoothInitSucceeded = result;
        return result;
    }

    void init_i2s() override {
        if (!is_output) {
            i2sInitSucceeded = true;
            is_i2s_active = false;
            return;
        }
        i2sInitSucceeded = out->begin();
        is_i2s_active = i2sInitSucceeded;
    }

    void av_hdl_stack_evt(uint16_t event, void* param) override {
        BluetoothA2DPSink::av_hdl_stack_evt(event, param);
        if (event == BT_APP_EVT_STACK_UP) stackEventCompleted = true;
    }

private:
    static bool deinitResultIsClean(esp_err_t result, bool wasInitialized,
                                    const char* component) {
        const bool clean = result == ESP_OK ||
                           (!wasInitialized && result == ESP_ERR_INVALID_STATE);
        if (!clean) {
            Serial.printf("[AUDIO] %s deinit failed: %s (%d)\n",
                          component, esp_err_to_name(result), (int)result);
        }
        return clean;
    }

    CheckedBluetoothOutput& checkedOutput;
    bool bluetoothInitSucceeded = false;
    bool i2sInitSucceeded = false;
    volatile bool stackEventCompleted = false;
};

CheckedBluetoothOutput a2dp_output;
CheckedBluetoothSink a2dp_sink(a2dp_output);

// ============================================
// Constructor
// ============================================
AudioPlayer::AudioPlayer()
    : audioSource(nullptr),
      audioSourceBuf(nullptr),
      audioBufferMemory(nullptr),
      aacDecoderMemory(nullptr),
      mp3Decoder(nullptr),
      i2sOutput(nullptr),
      playbackState(STATE_STOPPED),
      currentSource(AUDIO_SOURCE_NONE),
      currentVolume(80),
      currentCodec(AUDIO_CODEC_NONE),
      btEnabled(false),
      bluetoothLifecycle(BT_LIFECYCLE_INACTIVE),
      audioTaskHandle(nullptr),
      audioMutex(nullptr) {

    memset(nowPlaying, 0, sizeof(nowPlaying));
    memset(currentURL, 0, sizeof(currentURL));
    memset(bluetoothTitle, 0, sizeof(bluetoothTitle));
    memset(bluetoothArtist, 0, sizeof(bluetoothArtist));
    memset(bluetoothNowPlaying, 0, sizeof(bluetoothNowPlaying));
    memset(bluetoothNowPlayingSnapshot, 0, sizeof(bluetoothNowPlayingSnapshot));
    strcpy(nowPlaying, "Live stream");
}

AudioPlayer::~AudioPlayer() {
    teardownRadioPlayback();
    disableBluetooth();

    if (audioMutex) vSemaphoreDelete(audioMutex);
}

// ============================================
// Initialization
// ============================================
bool AudioPlayer::init() {
    audioMutex = xSemaphoreCreateMutex();
    if (!audioMutex) {
        Serial.println("[AUDIO] Failed to create mutex");
        return false;
    }

    // Configure the I2S output now, but don't begin() it yet -
    // AudioGeneratorMP3a::begin() calls output->begin() internally each
    // time playback starts, and we need this NOT actively holding the I2S
    // peripheral until then (Bluetooth mode needs to be able to claim it
    // instead, via a completely different, legacy I2S driver API - see
    // enableBluetooth()/disableBluetooth() for that handoff).
    i2sOutput = new AudioOutputI2S();
    i2sOutput->SetPinout(I2S_BCK_PIN, I2S_WS_PIN, I2S_DIN_PIN);
    i2sOutput->SetGain(currentVolume / 100.0f);

    // Create the persistent audio task now - it idles until play() gives
    // it something to decode, and is reused across every station change
    // rather than being created/destroyed each time.
    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        AUDIO_TASK_STACK,
        this,
        TASK_PRIORITY_AUDIO,
        &audioTaskHandle,
        1
    );

    Serial.println("[AUDIO] Initialization complete");
    return true;
}

// ============================================
// Playback Control - Internet Radio
// ============================================
bool AudioPlayer::play(const char* streamURL, AudioCodec codec) {
    if (!streamURL) return false;

    xSemaphoreTake(audioMutex, portMAX_DELAY);

    // Clean up any previous playback before starting a new one.
    teardownRadioPlayback();

    strncpy(currentURL, streamURL, sizeof(currentURL) - 1);
    currentURL[sizeof(currentURL) - 1] = '\0';
    currentCodec = codec;
    setNowPlaying("Live stream"); // replaced when ICY metadata arrives

    // Pick the source class based on the URL's scheme - both can coexist
    // in the same station list. HTTPS uses more RAM during the TLS
    // handshake (no PSRAM on this board), so it's only used when the URL
    // actually needs it.
    bool isSecure = (strncmp(streamURL, "https://", 8) == 0);
    if (isSecure) {
        audioSource = new AudioFileSourceICYSStream();
        Serial.println("[AUDIO] Using HTTPS source");
    } else {
        audioSource = new AudioFileSourceICYStream();
    }
    audioSource->RegisterMetadataCB(metadataCallback, (void*)this);
    audioSource->RegisterStatusCB(statusCallback, (void*)this);

    if (!audioSource->open(streamURL)) {
        Serial.println("[AUDIO] Failed to open stream");
        delete audioSource;
        audioSource = nullptr;
        currentURL[0] = '\0';
        xSemaphoreGive(audioMutex);
        return false;
    }

    // Ring buffer decoupling network reads from decode timing - this is
    // the library's own recommended pattern, not something extra I added.
    // 16KB instead of a smaller buffer - at typical 128kbps MP3 that's
    // roughly 1 second of cushion against WiFi jitter/scheduling gaps,
    // instead of ~128ms. Adjustable if this turns out to be too large
    // (out-of-memory) or still not enough (still audible cutting) on
    // actual hardware.
    // Allocated from PSRAM when available (this board's whole reason for
    // existing this session - keeps this 16KB off the tight internal
    // SRAM that WiFi/Bluetooth/TLS have been fighting over all along),
    // with a real fallback to regular heap if PSRAM isn't actually
    // present or the allocation fails - not assumed to always succeed.
    const uint32_t audioBufferSize = 16384;
    audioBufferMemory = (uint8_t*)ps_malloc(audioBufferSize);
    if (audioBufferMemory) {
        Serial.println("[AUDIO] Ring buffer allocated in PSRAM");
    } else {
        audioBufferMemory = (uint8_t*)malloc(audioBufferSize);
        if (audioBufferMemory) {
            Serial.println("[AUDIO] Ring buffer allocated in regular heap (PSRAM not available)");
        } else {
            Serial.println("[AUDIO] Ring buffer allocation failed entirely - out of memory");
            teardownRadioPlayback();
            xSemaphoreGive(audioMutex);
            return false;
        }
    }
    audioSourceBuf = new AudioFileSourceBuffer(audioSource, audioBufferMemory, audioBufferSize);

    if (codec == AUDIO_CODEC_AAC) {
        // AudioGeneratorAAC's normal constructor places its Helix decoder
        // state in the ordinary heap. That includes the ~50KB SBR state
        // allocation which previously exhausted internal RAM during
        // HE-AAC/AAC+ playback. Its preallocated constructor routes the
        // complete decoder state through this PSRAM allocation instead.
        // The resolved decoder's SBR state alone is roughly 50KB. Its
        // baseline state (including the SBR work buffer), input buffer,
        // and corrected AAC+ PCM output buffer require more than 80KB.
        constexpr size_t AAC_DECODER_MEMORY_SIZE = 128 * 1024;
        aacDecoderMemory = ps_malloc(AAC_DECODER_MEMORY_SIZE);
        if (!aacDecoderMemory) {
            Serial.println("[AUDIO] AAC requires 128KB of available PSRAM");
            teardownRadioPlayback();
            xSemaphoreGive(audioMutex);
            return false;
        }
        mp3Decoder = new AudioGeneratorAAC(aacDecoderMemory,
                                           AAC_DECODER_MEMORY_SIZE);
    } else {
        mp3Decoder = new AudioGeneratorMP3a();
    }

    if (!mp3Decoder->begin(audioSourceBuf, i2sOutput)) {
        Serial.printf("[AUDIO] Failed to start %s decoder\n",
                      codec == AUDIO_CODEC_AAC ? "AAC" : "MP3");
        teardownRadioPlayback();
        xSemaphoreGive(audioMutex);
        return false;
    }

    currentSource = AUDIO_SOURCE_INTERNET_RADIO;
    playbackState = STATE_PLAYING;

    xSemaphoreGive(audioMutex);

    Serial.printf("[AUDIO] Playing: %s\n", streamURL);
    return true;
}

// ============================================
// Teardown helper - stops and frees the radio playback chain
// ============================================
void AudioPlayer::teardownRadioPlayback() {
    if (mp3Decoder) {
        if (mp3Decoder->isRunning()) {
            mp3Decoder->stop();
        }
        delete mp3Decoder;
        mp3Decoder = nullptr;
    }
    if (audioSourceBuf) {
        delete audioSourceBuf;
        audioSourceBuf = nullptr;
    }
    if (audioBufferMemory) {
        free(audioBufferMemory);
        audioBufferMemory = nullptr;
    }
    if (aacDecoderMemory) {
        free(aacDecoderMemory);
        aacDecoderMemory = nullptr;
    }
    if (audioSource) {
        // AudioFileSourceBuffer's destructor only frees its own ring
        // buffer, not the source it wraps (verified in the library
        // source) - both need deleting separately.
        delete audioSource;
        audioSource = nullptr;
    }
    currentURL[0] = '\0';
}

// ============================================
// Playback State Control
// ============================================
void AudioPlayer::pause() {
    // This library has no true pause concept for a live stream (there's
    // nothing in AudioGenerator's API for it - makes sense, a live
    // broadcast keeps moving forward regardless). Simulated by muting
    // output while leaving the stream/decoder running in the background,
    // so resume is instant with no reconnection delay or missed-audio
    // backlog to deal with.
    if (playbackState == STATE_PLAYING) {
        if (i2sOutput) i2sOutput->SetGain(0.0f);
        playbackState = STATE_PAUSED;
        Serial.println("[AUDIO] Paused (muted, stream stays connected)");
    }
}

void AudioPlayer::resume() {
    if (playbackState == STATE_PAUSED) {
        if (i2sOutput) i2sOutput->SetGain(currentVolume / 100.0f);
        playbackState = STATE_PLAYING;
        Serial.println("[AUDIO] Resumed");
    }
}

void AudioPlayer::stop() {
    if (audioMutex) xSemaphoreTake(audioMutex, portMAX_DELAY);
    playbackState = STATE_STOPPED;
    teardownRadioPlayback();
    currentSource = AUDIO_SOURCE_NONE;
    currentCodec = AUDIO_CODEC_NONE;
    if (audioMutex) xSemaphoreGive(audioMutex);
    Serial.println("[AUDIO] Stopped");
}

// ============================================
// Volume Control
// ============================================
void AudioPlayer::setVolume(uint8_t vol) {
    currentVolume = min(vol, (uint8_t)100);

    if (currentSource == AUDIO_SOURCE_BLUETOOTH) {
        // Bluetooth audio doesn't go through i2sOutput at all - the A2DP
        // library writes to I2S internally via its own separate path, so
        // volume has to go through its own API instead.
        a2dp_sink.set_volume((uint8_t)((uint16_t)currentVolume * 127 / 100));
    } else if (i2sOutput && playbackState != STATE_PAUSED) {
        // Skip while paused - gain is intentionally held at 0 until
        // resume() restores it, so a volume change during pause doesn't
        // audibly un-mute early.
        i2sOutput->SetGain(currentVolume / 100.0f);
    }

    Serial.printf("[AUDIO] Volume: %d%%\n", currentVolume);
}

void AudioPlayer::volumeUp() {
    if (currentVolume < 100) {
        setVolume(currentVolume + 5);
    }
}

void AudioPlayer::volumeDown() {
    if (currentVolume > 0) {
        setVolume(CorrectnessGuards::volumeDown(currentVolume));
    }
}

// ============================================
// Bluetooth Control (pschatzmann/ESP32-A2DP)
// ============================================
bool AudioPlayer::enableBluetooth() {
    if (btEnabled && bluetoothLifecycle == BT_LIFECYCLE_ACTIVE) return true;
    if (bluetoothLifecycle == BT_LIFECYCLE_CLEANUP_FAILED) {
        Serial.println("[AUDIO] Bluetooth start blocked after incomplete cleanup; reboot required");
        return false;
    }

    if (audioMutex) xSemaphoreTake(audioMutex, portMAX_DELAY);
    bluetoothLifecycle = BT_LIFECYCLE_STARTING;

    // Release the I2S peripheral if radio playback currently holds it.
    // AudioOutputI2S uses the newer i2s_std.h channel API; the A2DP
    // library uses the legacy driver/i2s.h API - they can't both own the
    // peripheral at once. stop() safely no-ops if it was never begun.
    if (i2sOutput) i2sOutput->stop();
    teardownRadioPlayback();

    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DIN_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    a2dp_sink.set_pin_config(pin_config);
    a2dp_sink.set_volume((uint8_t)((uint16_t)currentVolume * 127 / 100));
    // Ask for only the two human-readable AVRCP fields we render. The
    // callback is registered before start(), as required by this library.
    a2dp_sink.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST);
    a2dp_sink.set_avrc_metadata_callback(bluetoothMetadataCallback);

    const bool started = a2dp_sink.startChecked(BT_DEVICE_NAME);

    if (!started) {
        const bool cleaned = a2dp_sink.stopRestartableChecked();
        btEnabled = false;
        bluetoothPlaybackPaused = false;
        currentSource = AUDIO_SOURCE_NONE;
        currentCodec = AUDIO_CODEC_NONE;
        playbackState = STATE_STOPPED;
        bluetoothLifecycle = cleaned
            ? BT_LIFECYCLE_INACTIVE
            : BT_LIFECYCLE_CLEANUP_FAILED;

        if (audioMutex) xSemaphoreGive(audioMutex);
        Serial.printf("[AUDIO] Bluetooth mode not entered; cleanup=%s\n",
                      cleaned ? "complete" : "incomplete");
        return false;
    }

    btEnabled = true;
    bluetoothPlaybackPaused = false;
    currentSource = AUDIO_SOURCE_BLUETOOTH;
    currentCodec = AUDIO_CODEC_SBC;
    playbackState = STATE_PLAYING;
    bluetoothLifecycle = BT_LIFECYCLE_ACTIVE;

    if (audioMutex) xSemaphoreGive(audioMutex);

    Serial.printf("[AUDIO] Bluetooth A2DP sink started - discoverable as '%s'\n", BT_DEVICE_NAME);
    return true;
}

bool AudioPlayer::disableBluetooth() {
    if (!btEnabled && bluetoothLifecycle == BT_LIFECYCLE_INACTIVE) return true;
    if (bluetoothLifecycle == BT_LIFECYCLE_CLEANUP_FAILED) {
        Serial.println("[AUDIO] Bluetooth cleanup already failed; reboot required before radio mode");
        return false;
    }

    if (audioMutex) xSemaphoreTake(audioMutex, portMAX_DELAY);
    bluetoothLifecycle = BT_LIFECYCLE_STOPPING;

    // end(true) calls esp_bt_controller_mem_release(), which ESP-IDF documents
    // as irreversible until reboot and which the pinned A2DP library marks by
    // setting is_start_disabled. end(false) still disconnects/deinitializes
    // A2DP + AVRCP, stops the library task and uninstalls I2S, while retaining
    // the controller/Bluedroid base allocations required by the next cycle.
    Serial.printf("[AUDIO] Stopping Bluetooth restartably (before: free=%u, largest=%u, allocator-min=%u)\n",
                  heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                  heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                  heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    const bool cleaned = a2dp_sink.stopRestartableChecked();
    btEnabled = false;
    currentSource = AUDIO_SOURCE_NONE;
    currentCodec = AUDIO_CODEC_NONE;
    playbackState = STATE_STOPPED;
    bluetoothLifecycle = cleaned
        ? BT_LIFECYCLE_INACTIVE
        : BT_LIFECYCLE_CLEANUP_FAILED;

    // Give the A2DP library's I2S teardown a moment to complete before
    // anything tries to reclaim the peripheral.
    delay(200);

    // i2sOutput->begin() happens automatically the next time play() runs
    // (via mp3Decoder->begin() internally) - nothing further needed here.
    Serial.printf("[AUDIO] Bluetooth stop %s (after: free=%u, largest=%u, allocator-min=%u, restartable=%s)\n",
                  cleaned ? "complete" : "incomplete",
                  heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                  heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                  heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                  a2dp_sink.isRestartable() ? "yes" : "no");

    if (audioMutex) xSemaphoreGive(audioMutex);
    return cleaned;
}

bool AudioPlayer::isBluetoothConnected() {
    if (!btEnabled) return false;
    return a2dp_sink.is_connected();
}

const char* AudioPlayer::getBluetoothDeviceName() {
    if (!btEnabled) return "unknown";
    // get_connected_source_name() itself is protected - get_peer_name()
    // is the public wrapper around it (confirmed against the library
    // source), same underlying data.
    return a2dp_sink.get_peer_name();
}

// ============================================
// Audio Task - drives the decoder
// ============================================
void AudioPlayer::audioTask(void* param) {
    AudioPlayer* player = (AudioPlayer*)param;
    player->audioTaskFunc();
}

void AudioPlayer::audioTaskFunc() {
    // Persistent task, reused across every station change - created once
    // in init(), never self-deletes. Idles via vTaskDelay when there's
    // nothing to decode (no station playing, Bluetooth mode active, or
    // paused), and calls mp3Decoder->loop() repeatedly otherwise, which
    // internally handles pulling buffered data, decoding, and writing to
    // I2S in one call.
    uint32_t lastYield = millis();
    while (true) {
        // play(), stop(), and the Bluetooth handoff delete and replace the
        // decoder/source chain. Use their mutex here as well, so the audio
        // task never calls loop() on a decoder another task just freed.
        xSemaphoreTake(audioMutex, portMAX_DELAY);
        bool isDecoding = currentSource == AUDIO_SOURCE_INTERNET_RADIO &&
                          playbackState == STATE_PLAYING &&
                          mp3Decoder && mp3Decoder->isRunning();
        if (isDecoding) {
            if (!mp3Decoder->loop()) {
                Serial.println("[AUDIO] Stream ended or decode error, stopping");
                playbackState = STATE_STOPPED;
                mp3Decoder->stop();
            }
        }
        xSemaphoreGive(audioMutex);

        if (isDecoding) {
            // Time-bounded yield, not a per-call one. The decoder's
            // loop() is designed to be called back-to-back as fast as
            // possible - it returns almost immediately once the I2S
            // output's small internal buffer is momentarily full, relying
            // on being called again very soon after. Yielding after
            // EVERY call (the previous round's fix) capped this loop
            // near ~1000 calls/sec regardless of how little work each one
            // did, which throttled real decode throughput below what
            // real-time playback needs - that's what caused the repeated
            // cutting. This instead lets the loop run freely and only
            // yields once at least 10ms of wall time has passed since the
            // last yield, which still gives the main loop a guaranteed,
            // regular scheduling opportunity (fixing the original
            // starvation/unresponsiveness bug) without limiting how much
            // real work happens between those yields.
            uint32_t now = millis();
            if (now - lastYield >= 10) {
                lastYield = now;
                vTaskDelay(1);
            }
        } else {
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }
}

// ============================================
// Callbacks (static - required plain C function pointer signatures)
// ============================================
static void simplifyStreamTitle(char* destination, size_t destinationSize, const char* source) {
    const char* marker = strstr(source, " - text=\"");
    if (marker) {
        const char* titleStart = marker + strlen(" - text=\"");
        const char* titleEnd = strchr(titleStart, '\"');
        if (titleEnd && titleEnd > titleStart) {
            size_t prefixLength = marker - source;
            size_t titleLength = titleEnd - titleStart;
            if (prefixLength > destinationSize - 1) prefixLength = destinationSize - 1;
            if (titleLength > destinationSize - 1) titleLength = destinationSize - 1;

            if (prefixLength > 0) {
                snprintf(destination, destinationSize, "%.*s - %.*s",
                         (int)prefixLength, source, (int)titleLength, titleStart);
            } else {
                snprintf(destination, destinationSize, "%.*s", (int)titleLength, titleStart);
            }
            return;
        }
    }

    strncpy(destination, source, destinationSize - 1);
    destination[destinationSize - 1] = '\0';
}

void AudioPlayer::setNowPlaying(const char* value) {
    portENTER_CRITICAL(&nowPlayingMux);
    strncpy(nowPlaying, value ? value : "", sizeof(nowPlaying) - 1);
    nowPlaying[sizeof(nowPlaying) - 1] = '\0';
    portEXIT_CRITICAL(&nowPlayingMux);
}

bool AudioPlayer::getNowPlaying(char* destination, size_t size) {
    if (!destination || size == 0) return false;
    portENTER_CRITICAL(&nowPlayingMux);
    strncpy(destination, nowPlaying, size - 1);
    destination[size - 1] = '\0';
    portEXIT_CRITICAL(&nowPlayingMux);
    return destination[0] != '\0';
}

bool AudioPlayer::getDiagnostics(AudioDiagnostics& diagnostics) {
    if (!audioMutex) return false;

    xSemaphoreTake(audioMutex, portMAX_DELAY);
    diagnostics.source = currentSource;
    diagnostics.state = playbackState;
    diagnostics.codec = currentCodec;
    diagnostics.bluetoothLifecycle = bluetoothLifecycle;
    diagnostics.bluetoothRestartable = a2dp_sink.isRestartable();
    strncpy(diagnostics.streamURL, currentURL,
            sizeof(diagnostics.streamURL) - 1);
    diagnostics.streamURL[sizeof(diagnostics.streamURL) - 1] = '\0';
    diagnostics.taskStackHighWaterMark = audioTaskHandle
        ? uxTaskGetStackHighWaterMark(audioTaskHandle)
        : 0;
    xSemaphoreGive(audioMutex);
    return true;
}

const char* AudioPlayer::getBluetoothNowPlaying() {
    portENTER_CRITICAL(&bluetoothMetadataMux);
    strncpy(bluetoothNowPlayingSnapshot, bluetoothNowPlaying,
            sizeof(bluetoothNowPlayingSnapshot) - 1);
    bluetoothNowPlayingSnapshot[sizeof(bluetoothNowPlayingSnapshot) - 1] = '\0';
    portEXIT_CRITICAL(&bluetoothMetadataMux);
    return bluetoothNowPlayingSnapshot[0] ? bluetoothNowPlayingSnapshot : "Waiting for metadata";
}

void AudioPlayer::bluetoothPlayPause() {
    if (!btEnabled) return;
    // AVRCP commands control the connected phone/source; they do not mute
    // the local I2S output like pause() does for an Internet radio stream.
    if (bluetoothPlaybackPaused) {
        a2dp_sink.play();
        bluetoothPlaybackPaused = false;
        Serial.println("[BT] AVRCP play requested");
    } else {
        a2dp_sink.pause();
        bluetoothPlaybackPaused = true;
        Serial.println("[BT] AVRCP pause requested");
    }
}

void AudioPlayer::bluetoothNext() { if (btEnabled) { a2dp_sink.next(); Serial.println("[BT] AVRCP next requested"); } }
void AudioPlayer::bluetoothPrevious() { if (btEnabled) { a2dp_sink.previous(); Serial.println("[BT] AVRCP previous requested"); } }

void AudioPlayer::bluetoothMetadataCallback(uint8_t attribute, const uint8_t* value) {
    if (!value) return;
    audioPlayer.setBluetoothMetadata(attribute, reinterpret_cast<const char*>(value));
}

void AudioPlayer::setBluetoothMetadata(uint8_t attribute, const char* value) {
    portENTER_CRITICAL(&bluetoothMetadataMux);
    if (attribute == ESP_AVRC_MD_ATTR_TITLE) {
        strncpy(bluetoothTitle, value, sizeof(bluetoothTitle) - 1);
        bluetoothTitle[sizeof(bluetoothTitle) - 1] = '\0';
    } else if (attribute == ESP_AVRC_MD_ATTR_ARTIST) {
        strncpy(bluetoothArtist, value, sizeof(bluetoothArtist) - 1);
        bluetoothArtist[sizeof(bluetoothArtist) - 1] = '\0';
    } else {
        portEXIT_CRITICAL(&bluetoothMetadataMux);
        return;
    }
    if (bluetoothArtist[0] && bluetoothTitle[0]) {
        snprintf(bluetoothNowPlaying, sizeof(bluetoothNowPlaying), "%s - %s", bluetoothArtist, bluetoothTitle);
    } else {
        strncpy(bluetoothNowPlaying, bluetoothTitle[0] ? bluetoothTitle : bluetoothArtist,
                sizeof(bluetoothNowPlaying) - 1);
        bluetoothNowPlaying[sizeof(bluetoothNowPlaying) - 1] = '\0';
    }
    portEXIT_CRITICAL(&bluetoothMetadataMux);
    Serial.printf("[BT] Metadata: %s\n", bluetoothNowPlaying);
}

void AudioPlayer::metadataCallback(void* cbData, const char* type, bool isUnicode, const char* str) {
    (void)isUnicode;
    AudioPlayer* self = (AudioPlayer*)cbData;
    if (!self || !type || !str) return;

    if (strcmp(type, "StreamTitle") == 0 && str[0] != '\0') {
        char title[sizeof(self->nowPlaying)];
        simplifyStreamTitle(title, sizeof(title), str);
        self->setNowPlaying(title);
        Serial.printf("[AUDIO] Now playing: %s\n", title);
    }
}

void AudioPlayer::statusCallback(void* cbData, int code, const char* str) {
    (void)cbData;
    Serial.printf("[AUDIO] Status(%d): %s\n", code, str ? str : "");
}
