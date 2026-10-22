#pragma once

#include <cstddef>
#include <cstdint>

namespace rtcp_btx {

constexpr uint8_t kCapabilityResponse = 12;
constexpr uint8_t kStateRequest = 13;
constexpr uint8_t kStateResponse = 14;
constexpr uint8_t kCommandRequest = 15;
constexpr uint8_t kMajor = 1;
constexpr uint8_t kMinor = 0;
constexpr uint8_t kFeatures = 0x3f;
constexpr uint8_t kMaxArtistBytes = 96;
constexpr uint8_t kMaxTitleBytes = 127;
constexpr uint16_t kFreshnessMs = 2000;
constexpr uint64_t kCapabilityRevision = 1;
constexpr size_t kFixedStateBytes = 32;
constexpr size_t kMaxStateBytes = 255;

enum class Source : uint8_t { none = 0, radio = 1, bluetooth = 2, other = 3 };
enum class Connection : uint8_t {
    not_applicable = 0,
    unknown = 1,
    disconnected = 2,
    connected = 3
};
enum class Playback : uint8_t { unknown = 0, stopped = 1, playing = 2, paused = 3 };
enum class Operation : uint8_t { play = 1, pause = 2, previous = 3, next = 4 };
enum class Negotiation : uint8_t { before_first = 0, probe_window = 1, legacy = 2, btx = 3 };
enum class CapabilityReply : uint8_t { legacy = 0, btx = 1, invalid = 2 };

enum Flag : uint8_t {
    artist_valid = 1u << 0,
    title_valid = 1u << 1,
    play_available = 1u << 2,
    pause_available = 1u << 3,
    previous_available = 1u << 4,
    next_available = 1u << 5
};

enum Error : uint8_t {
    success = 0,
    invalid_argument = 1,
    unsupported = 2,
    unavailable = 3,
    stale = 4,
    submission_failed = 5,
    busy = 6,
    internal = 7
};

struct SourceState {
    uint64_t source_generation{1};
    uint64_t metadata_revision{1};
    Source source{Source::none};
    Connection connection{Connection::not_applicable};
    Playback playback{Playback::unknown};
};

struct Snapshot {
    SourceState state{};
    char artist[kMaxArtistBytes + 1]{};
    char title[kMaxTitleBytes + 1]{};
    uint8_t artist_size{0};
    uint8_t title_size{0};
};

struct Command {
    uint64_t request{0};
    uint64_t source_generation{0};
    Operation operation{Operation::play};
    Playback expected_playback{Playback::unknown};
};

class Session {
public:
    CapabilityReply capabilityRequest();
    void observeNonCapability();
    void clear();
    bool negotiated() const { return negotiation_ == Negotiation::btx; }
    bool legacy() const { return negotiation_ == Negotiation::legacy; }
    void commandStarted(uint32_t started_us) { command_started_us_ = started_us; }
    uint32_t commandStartedUs() const { return command_started_us_; }

private:
    uint32_t command_started_us_{0};
    Negotiation negotiation_{Negotiation::before_first};
};

class Mailbox {
public:
    bool put(const Command& command);
    bool take(Command& command);
    void clear();
    bool occupied() const { return occupied_; }

private:
    uint64_t request_{0};
    uint64_t source_generation_{0};
    Operation operation_{Operation::play};
    Playback expected_playback_{Playback::unknown};
    bool occupied_{false};
};

struct Decision { uint8_t error{invalid_argument}; };

bool validUtf8Text(const char* text, size_t size);
bool copyLocalText(const char* source, size_t maximum, char* destination,
                   size_t capacity, uint8_t& size);
bool advance(uint64_t& value);
size_t capabilities(uint8_t* output, size_t capacity);
size_t stateResponse(const Snapshot& snapshot, uint64_t sequence,
                     uint8_t* output, size_t capacity);
bool parseCommand(const uint8_t* input, size_t size, Command& command);
Decision admit(const Snapshot& snapshot, const Command& command);

static_assert(sizeof(SourceState) == 24, "BTX source state layout changed");
static_assert(sizeof(Mailbox) == 24, "BTX mailbox layout changed");
static_assert(sizeof(Session) == 8, "BTX session layout changed");

}  // namespace rtcp_btx
