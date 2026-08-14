#include "audio.h"
#include "config.h"
#include <driver/i2s.h> // legacy I2S API - still needed here for
                         // i2s_pin_config_t/I2S_PIN_NO_CHANGE used by
                         // enableBluetooth() below (the A2DP library uses
                         // this older API; AudioOutputI2S uses the newer
                         // i2s_std.h channel API internally instead, which
                         // is why audio.h itself no longer needs this include)
#include <BluetoothA2DPSink.h>

// Global audio player
AudioPlayer audioPlayer;

// Bluetooth A2DP sink instance (pschatzmann/ESP32-A2DP library)
BluetoothA2DPSink a2dp_sink;

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
      btEnabled(false),
      audioTaskHandle(nullptr),
      audioMutex(nullptr) {

    memset(nowPlaying, 0, sizeof(nowPlaying));
    memset(currentURL, 0, sizeof(currentURL));
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
    strcpy(nowPlaying, "Live stream"); // replaced when ICY metadata arrives

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
        setVolume(currentVolume - 5);
    }
}

// ============================================
// Bluetooth Control (pschatzmann/ESP32-A2DP)
// ============================================
void AudioPlayer::enableBluetooth() {
    if (btEnabled) return;

    if (audioMutex) xSemaphoreTake(audioMutex, portMAX_DELAY);

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

    a2dp_sink.start(BT_DEVICE_NAME);

    btEnabled = true;
    currentSource = AUDIO_SOURCE_BLUETOOTH;
    playbackState = STATE_PLAYING;

    if (audioMutex) xSemaphoreGive(audioMutex);

    Serial.printf("[AUDIO] Bluetooth A2DP sink started - discoverable as '%s'\n", BT_DEVICE_NAME);
}

void AudioPlayer::disableBluetooth() {
    if (!btEnabled) return;

    a2dp_sink.stop();
    btEnabled = false;
    currentSource = AUDIO_SOURCE_NONE;
    playbackState = STATE_STOPPED;

    // Give the A2DP library's I2S teardown a moment to complete before
    // anything tries to reclaim the peripheral.
    delay(100);

    // i2sOutput->begin() happens automatically the next time play() runs
    // (via mp3Decoder->begin() internally) - nothing further needed here.
    Serial.println("[AUDIO] Bluetooth A2DP sink stopped, I2S released for radio mode");
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

void AudioPlayer::metadataCallback(void* cbData, const char* type, bool isUnicode, const char* str) {
    (void)isUnicode;
    AudioPlayer* self = (AudioPlayer*)cbData;
    if (!self || !type || !str) return;

    if (strcmp(type, "StreamTitle") == 0 && str[0] != '\0') {
        simplifyStreamTitle(self->nowPlaying, sizeof(self->nowPlaying), str);
        Serial.printf("[AUDIO] Now playing: %s\n", self->nowPlaying);
    }
}

void AudioPlayer::statusCallback(void* cbData, int code, const char* str) {
    (void)cbData;
    Serial.printf("[AUDIO] Status(%d): %s\n", code, str ? str : "");
}
