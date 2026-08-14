#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include <AudioFileSource.h>
#include <AudioFileSourceICYStream.h>
#include "AudioFileSourceICYSStream.h" // HTTPS variant, vendored into this
                                        // project (see its own file header
                                        // for why) - the library itself
                                        // only supports plain HTTP
#include <AudioFileSourceBuffer.h>
#include <AudioGenerator.h>
#include <AudioGeneratorMP3a.h>
#include <AudioGeneratorAAC.h>
#include <AudioOutputI2S.h>

enum AudioCodec {
    AUDIO_CODEC_MP3,
    AUDIO_CODEC_AAC
};

enum AudioSource {
    AUDIO_SOURCE_NONE,
    AUDIO_SOURCE_INTERNET_RADIO,
    AUDIO_SOURCE_BLUETOOTH
};

enum PlaybackState {
    STATE_STOPPED,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_BUFFERING
};

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    // Initialization
    bool init();

    // Playback control
    bool play(const char* streamURL, AudioCodec codec = AUDIO_CODEC_MP3);
    void pause();
    void resume();
    void stop();

    // Volume control
    void setVolume(uint8_t vol);
    uint8_t getVolume() { return currentVolume; }
    void volumeUp();
    void volumeDown();

    // State queries
    PlaybackState getState() { return playbackState; }
    AudioSource getSource() { return currentSource; }
    const char* getNowPlaying() { return nowPlaying; }
    const char* getCurrentURL() { return currentURL; }

    // Bluetooth control
    void enableBluetooth();
    void disableBluetooth();
    bool isBluetoothConnected(); // queries the real A2DP connection state
                                  // live, rather than a cached member that
                                  // was previously declared but never
                                  // actually updated (always false)
    const char* getBluetoothDeviceName(); // connected device's name, or
                                           // "unknown" if not connected yet
    const char* getBluetoothNowPlaying();
    void bluetoothPlayPause();
    void bluetoothNext();
    void bluetoothPrevious();

private:
    // Real MP3 decoding pipeline (ESP8266Audio library):
    //   AudioFileSourceICYStream  - HTTP connection + ICY metadata parsing
    //   AudioFileSourceBuffer     - ring buffer decoupling network reads
    //                               from decode timing (per the library's
    //                               own recommended usage pattern)
    //   AudioGeneratorMP3a        - Helix MP3 decoder (chosen over the
    //                               libmad-based AudioGeneratorMP3 for its
    //                               much smaller, fixed-size buffers - this
    //                               board has no PSRAM)
    //   AudioOutputI2S            - I2S output to the PCM5102A DAC
    // Previously this class read raw compressed bytes and wrote them
    // straight to I2S as if they were already PCM samples - real decoding
    // was completely absent. This is the actual fix for that gap.
    AudioFileSource* audioSource; // base pointer - actually either
                                   // AudioFileSourceICYStream (http://) or
                                   // AudioFileSourceICYSStream (https://),
                                   // chosen in play() based on the URL
    AudioFileSourceBuffer* audioSourceBuf;
    uint8_t* audioBufferMemory; // the actual ring buffer backing
                                 // audioSourceBuf - allocated via
                                 // ps_malloc() (PSRAM) with a fallback to
                                 // regular malloc() if PSRAM isn't
                                 // actually available. Owned and freed by
                                 // this class, not by AudioFileSourceBuffer
                                 // - see its pre-allocated-buffer
                                 // constructor, which sets
                                 // deallocateBuffer=false (confirmed in
                                 // its source) specifically so the caller
                                 // manages this memory instead.
    void* aacDecoderMemory;      // PSRAM backing for AudioGeneratorAAC's
                                 // preallocated decoder state. HE-AAC/SBR
                                 // needs a roughly 50KB state block, which
                                 // must not come from the limited internal
                                 // heap used by WiFi and TLS.
    AudioGenerator* mp3Decoder; // base pointer - actually either
                                // AudioGeneratorMP3a or AudioGeneratorAAC,
                                // chosen in play() based on the station's
                                // codec. Kept the name "mp3Decoder" rather
                                // than a rename sweep across every
                                // reference to it, since it's an internal
                                // member name, not part of the public API.
    AudioOutputI2S* i2sOutput;

    void teardownRadioPlayback(); // stops decoder/source, frees them

    // Audio task - drives mp3Decoder->loop() repeatedly
    static void audioTask(void* param);
    void audioTaskFunc();

    // Metadata callback (static - required C function pointer signature).
    // cbData is used to pass the AudioPlayer* instance through, since this
    // must be a plain function pointer, not a member function pointer.
    static void metadataCallback(void* cbData, const char* type, bool isUnicode, const char* str);
    static void statusCallback(void* cbData, int code, const char* str);
    static void bluetoothMetadataCallback(uint8_t attribute, const uint8_t* value);

    // State variables
    PlaybackState playbackState;
    AudioSource currentSource;
    uint8_t currentVolume;

    // Stream info
    char nowPlaying[256];
    char currentURL[512];

    // Bluetooth
    bool btEnabled;
    bool bluetoothPlaybackPaused = false;
    char bluetoothTitle[128];
    char bluetoothArtist[128];
    char bluetoothNowPlaying[256];
    char bluetoothNowPlayingSnapshot[256];
    portMUX_TYPE bluetoothMetadataMux = portMUX_INITIALIZER_UNLOCKED;
    void setBluetoothMetadata(uint8_t attribute, const char* value);

    // Task handle
    TaskHandle_t audioTaskHandle;

    // Mutex for thread safety
    SemaphoreHandle_t audioMutex;
};

extern AudioPlayer audioPlayer;

#endif // AUDIO_H
