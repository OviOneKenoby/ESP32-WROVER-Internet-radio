#include "rtcp_server.h"

#include "audio.h"
#include "ccm_wire.h"
#include "config.h"
#include "system_info.h"
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <cstring>

namespace rtcp {
namespace {
constexpr uint8_t kVersion = 1;
constexpr uint8_t kApplication = 1;
constexpr uint8_t kIdentityRequest = 1, kCapabilityRequest = 3;
constexpr uint8_t kSnapshotRequest = 5, kVolumeSet = 7;
constexpr uint8_t kPlaybackCommand = 9, kNowPlayingRequest = 10;
constexpr uint8_t kBluetoothStateRequest = 13, kBluetoothCommand = 15;
constexpr size_t kDeferredResponse = SIZE_MAX;

uint32_t freeInternal() {
    return heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}
uint32_t largestInternal() {
    return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

uint16_t get16(const uint8_t* p) { return uint16_t(p[0]) << 8 | p[1]; }
uint64_t get64(const uint8_t* p) {
    uint64_t v = 0; for (unsigned i = 0; i < 8; ++i) v = (v << 8) | p[i]; return v;
}
void put16(uint8_t* p, uint16_t v) { p[0] = uint8_t(v >> 8); p[1] = uint8_t(v); }
void put64(uint8_t* p, uint64_t v) { for (int i = 7; i >= 0; --i) { p[i] = uint8_t(v); v >>= 8; } }

bool constantEqual(const uint8_t* a, const uint8_t* b, size_t n) {
    uint8_t d = 0; for (size_t i = 0; i < n; ++i) d |= a[i] ^ b[i]; return d == 0;
}
bool sha256(const uint8_t* data, size_t size, uint8_t out[32]) {
    const auto* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    return md && mbedtls_md(md, data, size, out) == 0;
}
bool hmac(const uint8_t* key, size_t key_size, const uint8_t* data, size_t size, uint8_t out[32]) {
    const auto* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    return md && mbedtls_md_hmac(md, key, key_size, data, size, out) == 0;
}
bool labeledMac(const uint8_t* key, const char* label, const uint8_t* a, size_t an,
                const uint8_t* b, size_t bn, const uint8_t* c, size_t cn, uint8_t out[32]) {
    std::array<uint8_t, 160> input{};
    const size_t ln = strlen(label), total = ln + an + bn + cn;
    if (total > input.size()) return false;
    size_t at = 0; memcpy(input.data() + at, label, ln); at += ln;
    memcpy(input.data() + at, a, an); at += an;
    memcpy(input.data() + at, b, bn); at += bn;
    if (cn) memcpy(input.data() + at, c, cn);
    const bool ok = hmac(key, 32, input.data(), total, out);
    memset(input.data(), 0, input.size()); return ok;
}
bool hkdfExpand(const uint8_t prk[32], const char* label, const uint8_t transcript[32],
                uint8_t* out, size_t out_size) {
    std::array<uint8_t, 96> input{};
    const size_t ln = strlen(label);
    if (ln + 32 + 1 > input.size() || out_size > 32) return false;
    memcpy(input.data(), label, ln); memcpy(input.data() + ln, transcript, 32);
    input[ln + 32] = 1;
    uint8_t block[32]{};
    const bool ok = hmac(prk, 32, input.data(), ln + 33, block);
    if (ok) memcpy(out, block, out_size);
    memset(block, 0, sizeof block); memset(input.data(), 0, input.size()); return ok;
}
bool derive(const Pairing& pairing, const uint8_t cn[16], const uint8_t sn[16],
            const uint8_t* transcript, size_t transcript_size, uint8_t c2s[16],
            uint8_t s2c[16], uint8_t cp[4], uint8_t sp[4], uint8_t sid[8]) {
    std::array<uint8_t, 47> salt_input{};
    constexpr size_t label_size = 15;
    memcpy(salt_input.data(), "RTCP1 HKDF salt", label_size);
    memcpy(salt_input.data() + label_size, cn, 16);
    memcpy(salt_input.data() + label_size + 16, sn, 16);
    uint8_t salt[32]{}, prk[32]{}, th[32]{};
    bool ok = sha256(salt_input.data(), salt_input.size(), salt) &&
              hmac(salt, sizeof salt, pairing.psk.data(), pairing.psk.size(), prk) &&
              sha256(transcript, transcript_size, th) &&
              hkdfExpand(prk, "RTCP1 c2s key", th, c2s, 16) &&
              hkdfExpand(prk, "RTCP1 s2c key", th, s2c, 16) &&
              hkdfExpand(prk, "RTCP1 c2s nonce", th, cp, 4) &&
              hkdfExpand(prk, "RTCP1 s2c nonce", th, sp, 4) &&
              hkdfExpand(prk, "RTCP1 session id", th, sid, 8);
    memset(salt, 0, sizeof salt); memset(prk, 0, sizeof prk); memset(th, 0, sizeof th);
    return ok;
}
bool gcmEncrypt(const uint8_t key[16], const uint8_t nonce[12], const uint8_t* aad,
                size_t aad_size, const uint8_t* plain, size_t size, uint8_t* cipher,
                uint8_t tag[16]) {
    mbedtls_gcm_context ctx; mbedtls_gcm_init(&ctx);
    bool ok = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128) == 0 &&
              mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, size, nonce, 12,
                                       aad, aad_size, plain, cipher, 16, tag) == 0;
    mbedtls_gcm_free(&ctx); return ok;
}
bool gcmDecrypt(const uint8_t key[16], const uint8_t nonce[12], const uint8_t* aad,
                size_t aad_size, const uint8_t* cipher, size_t size, const uint8_t tag[16],
                uint8_t* plain) {
    mbedtls_gcm_context ctx; mbedtls_gcm_init(&ctx);
    bool ok = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128) == 0 &&
              mbedtls_gcm_auth_decrypt(&ctx, size, nonce, 12, aad, aad_size,
                                       tag, 16, cipher, plain) == 0;
    mbedtls_gcm_free(&ctx); return ok;
}
}  // namespace

bool loadPairing(Pairing& out) {
    Preferences prefs;
    if (!prefs.begin("remote_pair", true)) return false;
    const bool exact = prefs.getBytesLength("pair_id") == out.id.size() &&
                       prefs.getBytesLength("psk") == out.psk.size();
    const bool ok = exact && prefs.getBytes("pair_id", out.id.data(), out.id.size()) == out.id.size() &&
                    prefs.getBytes("psk", out.psk.data(), out.psk.size()) == out.psk.size();
    prefs.end();
    if (!ok) { memset(&out, 0, sizeof out); return false; }
    return true;
}

bool Server::begin() {
    if (enabled_) return true;
    if (!loadPairing(pairing_)) { state_ = State::disabled; return false; }
    listener_.begin(); listener_.setNoDelay(true);
    enabled_ = true; state_ = State::listening;
    discovery_.poll(true, millis());
    return true;
}
void Server::closeClient(CloseReason reason) {
    diagnostics_.closed(reason, freeInternal(), largestInternal());
    if (client_) client_.stop();
    memset(c2s_key_.data(), 0, c2s_key_.size()); memset(s2c_key_.data(), 0, s2c_key_.size());
    memset(c2s_prefix_.data(), 0, c2s_prefix_.size()); memset(s2c_prefix_.data(), 0, s2c_prefix_.size());
    memset(session_id_.data(), 0, session_id_.size());
    rx_used_ = 0; rx_needed_ = 40; rx_counter_ = tx_counter_ = 1;
    playback_mailbox_.clear(); last_playback_request_ = 0;
    btx_mailbox_.clear(); btx_session_.clear();
    state_ = enabled_ ? State::listening : State::disabled;
}
void Server::acceptClient() {
    WiFiClient candidate = listener_.available();
    if (!candidate) return;
    if (client_) { candidate.stop(); return; }
    client_ = candidate; client_.setNoDelay(true); rx_used_ = 0; rx_needed_ = 40;
    btx_mailbox_.clear(); btx_session_.clear();
    diagnostics_.accepted(freeInternal(), largestInternal());
    deadline_ms_ = millis() + 3000; state_ = State::client_hello;
}
bool Server::readBounded() {
    if (!client_ || !client_.connected()) return false;
    size_t budget = 128;
    while (budget && client_.available() && rx_used_ < rx_needed_) {
        const int c = client_.read(); if (c < 0) break;
        rx_[rx_used_++] = uint8_t(c); --budget;
    }
    return true;
}
bool Server::processHandshake() {
    if (int32_t(millis() - deadline_ms_) >= 0) {
        diagnostics_.close_reason = CloseReason::timeout;
        return false;
    }
    if (rx_used_ != rx_needed_) return true;
    if (state_ == State::client_hello) {
        if (memcmp(rx_.data(), "RTCP\x01\x01\x00\x20", 8) != 0 ||
            !constantEqual(rx_.data() + 8, pairing_.id.data(), pairing_.id.size())) return false;
        memcpy(client_hello_.data(), rx_.data(), 40); memcpy(client_nonce_.data(), rx_.data() + 24, 16);
        esp_fill_random(server_nonce_.data(), server_nonce_.size());
        memcpy(server_hello_.data(), "RTCP\x01\x02\x00\x30", 8);
        memcpy(server_hello_.data() + 8, server_nonce_.data(), 16);
        uint8_t proof[32]{};
        if (!labeledMac(pairing_.psk.data(), "RTCP1 server proof", client_hello_.data(), 40,
                        server_hello_.data(), 24, nullptr, 0, proof)) return false;
        memcpy(server_hello_.data() + 24, proof, 32); memset(proof, 0, sizeof proof);
        if (client_.write(server_hello_.data(), server_hello_.size()) != server_hello_.size()) return false;
        rx_used_ = 0; rx_needed_ = 40; state_ = State::client_finish; return true;
    }
    if (memcmp(rx_.data(), "RTCP\x01\x03\x00\x20", 8) != 0) return false;
    uint8_t expected[32]{};
    if (!labeledMac(pairing_.psk.data(), "RTCP1 client proof", client_hello_.data(), 40,
                    server_hello_.data(), 56, rx_.data(), 8, expected) ||
        !constantEqual(expected, rx_.data() + 8, 32)) { memset(expected, 0, sizeof expected); return false; }
    memset(expected, 0, sizeof expected);
    std::array<uint8_t, 136> transcript{};
    memcpy(transcript.data(), client_hello_.data(), 40); memcpy(transcript.data() + 40, server_hello_.data(), 56);
    memcpy(transcript.data() + 96, rx_.data(), 40);
    if (!derive(pairing_, client_nonce_.data(), server_nonce_.data(), transcript.data(), transcript.size(),
                c2s_key_.data(), s2c_key_.data(), c2s_prefix_.data(), s2c_prefix_.data(), session_id_.data())) return false;
    memset(transcript.data(), 0, transcript.size());
    diagnostics_.clientFinish();
    rx_used_ = 0; rx_needed_ = kRecordHeader; deadline_ms_ = millis() + 5000;
    state_ = State::records; return true;
}
bool Server::sendApplication(const uint8_t* data, size_t size) {
    if (size > kMaxPlaintext || tx_counter_ == UINT64_MAX) return false;
    uint8_t* h = tx_.data(); memcpy(h, "RTCR", 4); h[4] = 1; h[5] = kApplication;
    h[6] = h[7] = 0; memcpy(h + 8, session_id_.data(), 8); put64(h + 16, tx_counter_);
    put16(h + 24, uint16_t(size)); h[26] = h[27] = 0;
    uint8_t nonce[12]{}; memcpy(nonce, s2c_prefix_.data(), 4); put64(nonce + 4, tx_counter_);
    if (!gcmEncrypt(s2c_key_.data(), nonce, h, kRecordHeader, data, size,
                    h + kRecordHeader, h + kRecordHeader + size)) return false;
    const size_t wire = kRecordHeader + size + kTagBytes;
    diagnostics_.responseBuilt(wire);
    const uint32_t allocation_before = SystemInfo::allocationFailureCount();
    const size_t written = client_.write(tx_.data(), wire);
    const uint32_t allocation_after = SystemInfo::allocationFailureCount();
    diagnostics_.responseWrite(written, allocation_after >= allocation_before
                                            ? allocation_after - allocation_before : 0);
    if (written != wire) return false;
    ++tx_counter_; return true;
}
size_t Server::handleApplication(AudioPlayer& audio, const uint8_t* p, size_t n,
                                 uint8_t* out, size_t cap) {
    if (!n || cap < 32) return 0;
    if (p[0] != kCapabilityRequest) btx_session_.observeNonCapability();
    if (p[0] == kIdentityRequest && n == 4 && p[1] == 1 && p[2] == 1 && p[3] == 0)
        return ccm_wire::identity(FIRMWARE_VERSION, out, cap);
    if (p[0] == kCapabilityRequest && n == 1) {
        const rtcp_btx::CapabilityReply reply = btx_session_.capabilityRequest();
        if (reply == rtcp_btx::CapabilityReply::legacy)
            return ccm_wire::capabilities(out, cap);
        if (reply == rtcp_btx::CapabilityReply::btx)
            return rtcp_btx::capabilities(out, cap);
        return 0;
    }
    if (p[0] == kSnapshotRequest && n == 1) {
        if (sample_sequence_ == UINT64_MAX) return 0;
        const PlaybackState state = audio.getState();
        const uint8_t ccm_state = state == STATE_PLAYING ? 0
                                  : state == STATE_PAUSED ? 1
                                  : state == STATE_BUFFERING ? 3 : 2;
        return ccm_wire::snapshot(audio.getVolume(), ccm_state,
                                  ++sample_sequence_, projection_.mediaGeneration(), out, cap);
    }
    if (p[0] == kVolumeSet && n == 17) {
        ccm_wire::VolumeSet command; uint8_t percent=0;
        if (!ccm_wire::parseVolumeSet(p,n,command)) return 0;
        if (!ccm_wire::volumePercent(command,percent))
            return ccm_wire::result(command.request,false,1,out,cap);
        const uint32_t started = micros();
        audio.setVolume(percent);
        timing_.lastApplyUs = micros() - started;
        if (timing_.lastApplyUs > timing_.maxApplyUs)
            timing_.maxApplyUs = timing_.lastApplyUs;
        if (timing_.commands != UINT32_MAX) ++timing_.commands;
        return ccm_wire::result(command.request,true,0,out,cap);
    }
    if (p[0] == kPlaybackCommand && n == 19) {
        ccm_wire::PlaybackCommand command;
        if (!ccm_wire::parsePlaybackCommand(p,n,command)) return 0;
        if (command.request == last_playback_request_)
            return ccm_wire::result(command.request,false,rtcp_d009::busy,out,cap);
        const rtcp_d009::Decision decision=projection_.admit(command);
        if (decision.error!=rtcp_d009::success)
            return ccm_wire::result(command.request,false,decision.error,out,cap);
        if (!playback_mailbox_.put(command,decision.target_station))
            return ccm_wire::result(command.request,false,rtcp_d009::busy,out,cap);
        last_playback_request_=command.request;
        playback_started_us_=micros();
        return kDeferredResponse;
    }
    if (p[0] == kNowPlayingRequest && n == 1) {
        if (sample_sequence_ == UINT64_MAX) return 0;
        return ccm_wire::nowPlaying(projection_.wire(++sample_sequence_),out,cap);
    }
    if (p[0] == kBluetoothStateRequest && n == 1 && btx_session_.negotiated()) {
        if (sample_sequence_ == UINT64_MAX) return 0;
        rtcp_btx::Snapshot snapshot;
        if (!audio.getBluetoothRtcpState(snapshot)) return 0;
        return rtcp_btx::stateResponse(snapshot, ++sample_sequence_, out, cap);
    }
    if (p[0] == kBluetoothCommand && n == 19 && btx_session_.negotiated()) {
        rtcp_btx::Command command;
        if (!rtcp_btx::parseCommand(p, n, command)) return 0;
        rtcp_btx::Snapshot snapshot;
        if (!audio.getBluetoothRtcpState(snapshot))
            return ccm_wire::result(command.request, false, rtcp_btx::internal, out, cap);
        const rtcp_btx::Decision decision = rtcp_btx::admit(snapshot, command);
        if (decision.error != rtcp_btx::success)
            return ccm_wire::result(command.request, false, decision.error, out, cap);
        if (!btx_mailbox_.put(command))
            return ccm_wire::result(command.request, false, rtcp_btx::busy, out, cap);
        btx_session_.commandStarted(micros());
        return kDeferredResponse;
    }
    return 0;
}
bool Server::processRecord(AudioPlayer& audio) {
    const uint32_t started = micros();
    if (rx_used_ < kRecordHeader) return true;
    if (rx_needed_ == kRecordHeader) {
        if (memcmp(rx_.data(), "RTCR\x01\x01\x00\x00", 8) != 0 ||
            !constantEqual(rx_.data() + 8, session_id_.data(), 8) ||
            get64(rx_.data() + 16) != rx_counter_ || rx_[26] || rx_[27]) return false;
        const size_t length = get16(rx_.data() + 24);
        if (length > kMaxPlaintext) return false;
        rx_needed_ = kRecordHeader + length + kTagBytes;
        if (rx_used_ != rx_needed_) return true;
    }
    if (rx_used_ != rx_needed_) return true;
    const size_t length = get16(rx_.data() + 24);
    uint8_t nonce[12]{}; memcpy(nonce, c2s_prefix_.data(), 4); put64(nonce + 4, rx_counter_);
    std::array<uint8_t, kMaxPlaintext> plain{}, response{};
    if (!gcmDecrypt(c2s_key_.data(), nonce, rx_.data(), kRecordHeader,
                    rx_.data() + kRecordHeader, length,
                    rx_.data() + kRecordHeader + length, plain.data())) {
        diagnostics_.close_reason = CloseReason::authentication_failed;
        return false;
    }
    ++rx_counter_;
    diagnostics_.received(length ? diagnosticRequest(plain[0]) : DiagnosticRequest::unknown,
                          freeInternal(), largestInternal());
    const size_t response_size = handleApplication(audio, plain.data(), length, response.data(), response.size());
    memset(plain.data(), 0, plain.size());
    if (response_size == kDeferredResponse) {
        rx_used_ = 0; rx_needed_ = kRecordHeader; deadline_ms_ = millis() + 5000;
        return true;
    }
    if (!response_size) {
        diagnostics_.close_reason = CloseReason::application_invalid;
        return false;
    }
    if (!sendApplication(response.data(), response_size)) return false;
    timing_.lastResponseUs = micros() - started;
    if (timing_.lastResponseUs > timing_.maxResponseUs)
        timing_.maxResponseUs = timing_.lastResponseUs;
    memset(response.data(), 0, response.size()); rx_used_ = 0; rx_needed_ = kRecordHeader;
    deadline_ms_ = millis() + 5000; return true;
}
bool Server::servicePlayback() {
    ccm_wire::PlaybackCommand command;uint8_t target_station=0;
    if (!playback_mailbox_.take(command,target_station)) return true;
    const uint8_t error=playback_owner_
        ?playback_owner_(ccm_wire::PlaybackOperation(command.operation),target_station)
        :rtcp_d009::internal;
    uint8_t response[11]{};
    const size_t response_size=ccm_wire::result(command.request,error==rtcp_d009::success,
                                                 error,response,sizeof response);
    const bool sent=response_size&&sendApplication(response,response_size);
    timing_.lastResponseUs=micros()-playback_started_us_;
    if(timing_.lastResponseUs>timing_.maxResponseUs)timing_.maxResponseUs=timing_.lastResponseUs;
    memset(response,0,sizeof response);deadline_ms_=millis()+5000;return sent;
}
bool Server::serviceBluetooth() {
    rtcp_btx::Command command;
    if (!btx_mailbox_.take(command)) return true;
    const uint8_t error = bluetooth_owner_
        ? bluetooth_owner_(command)
        : rtcp_btx::internal;
    uint8_t response[11]{};
    const size_t response_size = ccm_wire::result(command.request,
        error == rtcp_btx::success, error, response, sizeof response);
    const bool sent = response_size && sendApplication(response, response_size);
    timing_.lastResponseUs = micros() - btx_session_.commandStartedUs();
    if (timing_.lastResponseUs > timing_.maxResponseUs)
        timing_.maxResponseUs = timing_.lastResponseUs;
    memset(response, 0, sizeof response);
    deadline_ms_ = millis() + 5000;
    return sent;
}
void Server::poll(AudioPlayer& audio) {
    const bool wifi_connected = WiFi.status() == WL_CONNECTED;
    discovery_.poll(enabled_ && wifi_connected, millis());
    if (!enabled_) { if (wifi_connected) begin(); return; }
    if (state_ == State::listening) { acceptClient(); return; }
    if (!readBounded()) { closeClient(CloseReason::peer_eof); return; }
    // Process a complete buffered record before evaluating the idle deadline.
    // An incomplete or absent record still expires at the original deadline.
    if (state_ == State::records && rx_used_ != rx_needed_ &&
        int32_t(millis() - deadline_ms_) >= 0) {
        closeClient(CloseReason::timeout); return;
    }
    const bool ok = state_ == State::records ? processRecord(audio) : processHandshake();
    const bool playback_ok = ok && state_ == State::records ? servicePlayback() : ok;
    const bool owner_ok = playback_ok && state_ == State::records ? serviceBluetooth() : playback_ok;
    if (!owner_ok) {
        CloseReason reason = diagnostics_.close_reason;
        if (reason == CloseReason::none)
            reason = state_ == State::records ? CloseReason::framing_invalid
                                              : CloseReason::handshake_invalid;
        closeClient(reason);
    }
}
}  // namespace rtcp

rtcp::Server rtcpServer;
