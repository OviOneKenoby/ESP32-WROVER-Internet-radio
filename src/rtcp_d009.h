#pragma once

#include "ccm_wire.h"
#include <cstddef>
#include <cstdint>

namespace rtcp_d009 {

enum class Context : uint8_t {
    none=0, fixed_radio=1, discovered_radio=2, bluetooth=3, other=4
};

enum Flag : uint8_t {
    station_valid=1u<<0, metadata_valid=1u<<1, split_metadata=1u<<2,
    truncated=1u<<3, play_available=1u<<4, pause_available=1u<<5,
    previous_available=1u<<6, next_available=1u<<7
};

enum Error : uint8_t {
    success=0, invalid_argument=1, unsupported_operation=2,
    unavailable=3, stale=4, execution_failed=5, busy=6, internal=7
};

struct Input {
    Context context{Context::none};
    uint8_t playback_state{2};
    const char* station{nullptr};
    const char* combined{nullptr};
    uint8_t selected_station{0};
    uint8_t station_count{0};
    uint8_t source_identity{0};
};

struct Decision {
    uint8_t error{invalid_argument};
    uint8_t target_station{0};
};

class Projection {
public:
    void publish(const Input&);
    void invalidateMediaContext();
    Decision admit(const ccm_wire::PlaybackCommand&) const;
    ccm_wire::NowPlaying wire(uint64_t sequence) const;

    uint64_t mediaGeneration() const { return media_generation_; }
    uint64_t metadataRevision() const { return metadata_revision_; }
    uint8_t playbackState() const { return playback_state_; }
    uint8_t flags() const { return flags_; }
    Context context() const { return context_; }

private:
    char station_[64]{};
    char combined_[256]{};
    uint8_t station_size_{0};
    uint8_t combined_size_{0};
    uint8_t flags_{0};
    Context context_{Context::none};
    uint8_t playback_state_{2};
    uint8_t selected_station_{0};
    uint8_t station_count_{0};
    uint8_t source_identity_{0};
    bool initialized_{false};
    bool generation_preincremented_{false};
    uint64_t media_generation_{1};
    uint64_t metadata_revision_{1};
};

class Mailbox {
public:
    bool put(const ccm_wire::PlaybackCommand& command, uint8_t target_station) {
        if (occupied_) return false;
        command_ = command; target_station_ = target_station; occupied_ = true; return true;
    }
    bool take(ccm_wire::PlaybackCommand& command, uint8_t& target_station) {
        if (!occupied_) return false;
        command = command_; target_station = target_station_; occupied_ = false; return true;
    }
    void clear() { occupied_ = false; command_ = {}; target_station_ = 0; }
    bool occupied() const { return occupied_; }
private:
    ccm_wire::PlaybackCommand command_{};
    uint8_t target_station_{0};
    bool occupied_{false};
};

bool validUtf8Text(const char* text, size_t size);

} // namespace rtcp_d009
