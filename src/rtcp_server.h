#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include "rtcp_diagnostics.h"
#include "rtcp_discovery.h"
#include "rtcp_d009.h"
#include "rtcp_btx.h"
#include "rtcp_full_control.h"

class AudioPlayer;

namespace rtcp {

constexpr uint16_t kPort = 46411;
constexpr size_t kMaxPlaintext = 512;
constexpr size_t kRecordHeader = 28;
constexpr size_t kTagBytes = 16;
constexpr size_t kMaxWire = kRecordHeader + kMaxPlaintext + kTagBytes;

struct Pairing {
    std::array<uint8_t, 16> id{};
    std::array<uint8_t, 32> psk{};
};

struct Timing {
    uint32_t commands{0};
    uint32_t lastApplyUs{0};
    uint32_t maxApplyUs{0};
    uint32_t lastResponseUs{0};
    uint32_t maxResponseUs{0};
};

// Reads an owner-provisioned pairing from the dedicated read-only NVS
// namespace. It never creates, changes, or logs pairing material.
bool loadPairing(Pairing& out);

class Server {
public:
    using PlaybackOwner = uint8_t (*)(ccm_wire::PlaybackOperation, uint8_t target_station);
    using BluetoothOwner = uint8_t (*)(const rtcp_btx::Command& command);
    using SourceOwner = uint8_t (*)(rtcp_full_control::Source target);
    bool begin();
    void poll(AudioPlayer& audio);
    void setPlaybackOwner(PlaybackOwner owner) { playback_owner_ = owner; }
    void setBluetoothOwner(BluetoothOwner owner) { bluetooth_owner_ = owner; }
    void setSourceOwner(SourceOwner owner) { source_owner_ = owner; }
    void publishNowPlaying(const rtcp_d009::Input& input) { projection_.publish(input); }
    void invalidateMediaContext() { projection_.invalidateMediaContext(); }
    bool enabled() const { return enabled_; }
    bool authenticated() const { return state_ == State::records; }
    size_t staticBufferBytes() const { return rx_.size() + tx_.size(); }
    const Timing& timing() const { return timing_; }
    const Diagnostics& diagnostics() const { return diagnostics_; }
    const DiscoveryAdvertisementModel& discoveryDiagnostics() const {
        return discovery_.diagnostics();
    }

private:
    enum class State { disabled, listening, client_hello, client_finish, records };
    WiFiServer listener_{kPort};
    WiFiClient client_;
    Pairing pairing_{};
    State state_{State::disabled};
    bool enabled_{false};
    uint32_t deadline_ms_{0};
    std::array<uint8_t, kMaxWire> rx_{};
    std::array<uint8_t, kMaxWire> tx_{};
    size_t rx_used_{0};
    size_t rx_needed_{40};
    std::array<uint8_t, 40> client_hello_{};
    std::array<uint8_t, 56> server_hello_{};
    std::array<uint8_t, 16> client_nonce_{};
    std::array<uint8_t, 16> server_nonce_{};
    std::array<uint8_t, 16> c2s_key_{};
    std::array<uint8_t, 16> s2c_key_{};
    std::array<uint8_t, 4> c2s_prefix_{};
    std::array<uint8_t, 4> s2c_prefix_{};
    std::array<uint8_t, 8> session_id_{};
    uint64_t rx_counter_{1};
    uint64_t tx_counter_{1};
    uint64_t sample_sequence_{0};
    uint64_t last_playback_request_{0};
    uint32_t playback_started_us_{0};
    rtcp_d009::Projection projection_{};
    rtcp_d009::Mailbox playback_mailbox_{};
    PlaybackOwner playback_owner_{nullptr};
    rtcp_btx::Session btx_session_{};
    rtcp_btx::Mailbox btx_mailbox_{};
    BluetoothOwner bluetooth_owner_{nullptr};
    rtcp_full_control::SourceState source_state_{};
    rtcp_full_control::SourceMailbox source_mailbox_{};
    SourceOwner source_owner_{nullptr};
    uint32_t source_started_us_{0};
    Timing timing_{};
    Diagnostics diagnostics_{};
    DiscoveryAdvertisement discovery_{};

    void acceptClient();
    void closeClient(CloseReason reason);
    bool readBounded();
    bool processHandshake();
    bool processRecord(AudioPlayer& audio);
    bool servicePlayback();
    bool serviceBluetooth();
    bool serviceSource(AudioPlayer& audio);
    bool refreshSourceState(AudioPlayer& audio);
    bool sendApplication(const uint8_t* data, size_t size);
    size_t handleApplication(AudioPlayer& audio, const uint8_t* data, size_t size,
                             uint8_t* output, size_t capacity);
};

}  // namespace rtcp

extern rtcp::Server rtcpServer;

static_assert(sizeof(rtcp_d009::Projection) + sizeof(rtcp_d009::Mailbox) <= 512,
              "D-009 persistent projection and mailbox exceed the approved budget");
static_assert(sizeof(rtcp_btx::SourceState) + sizeof(rtcp_btx::Session) +
                  sizeof(rtcp_btx::Mailbox) + sizeof(rtcp::Server::BluetoothOwner) <= 64,
              "BTX persistent scalar/session state exceeds the approved budget");
static_assert(sizeof(rtcp_full_control::SourceState) +
                  sizeof(rtcp_full_control::SourceMailbox) +
                  sizeof(rtcp::Server::SourceOwner) <= 48,
              "full-control source state exceeds the approved budget");
