#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include "rtcp_diagnostics.h"

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
    bool begin();
    void poll(AudioPlayer& audio);
    bool enabled() const { return enabled_; }
    bool authenticated() const { return state_ == State::records; }
    size_t staticBufferBytes() const { return rx_.size() + tx_.size(); }
    const Timing& timing() const { return timing_; }
    const Diagnostics& diagnostics() const { return diagnostics_; }

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
    uint64_t media_generation_{1};
    int last_source_{-1};
    Timing timing_{};
    Diagnostics diagnostics_{};

    void acceptClient();
    void closeClient(CloseReason reason);
    bool readBounded();
    bool processHandshake();
    bool processRecord(AudioPlayer& audio);
    bool sendApplication(const uint8_t* data, size_t size);
    size_t handleApplication(AudioPlayer& audio, const uint8_t* data, size_t size,
                             uint8_t* output, size_t capacity);
};

}  // namespace rtcp

extern rtcp::Server rtcpServer;
