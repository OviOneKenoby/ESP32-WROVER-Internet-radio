#pragma once

#include <cstddef>
#include <cstdint>

namespace rtcp_full_control {

constexpr uint8_t kCapabilityRequest = 16;
constexpr uint8_t kCapabilityResponse = 17;
constexpr uint8_t kSourceStateRequest = 18;
constexpr uint8_t kSourceStateResponse = 19;
constexpr uint8_t kSourceSelectRequest = 20;
constexpr uint8_t kMajor = 1;
constexpr uint8_t kMinor = 0;
constexpr uint16_t kFreshnessMs = 2000;
constexpr uint64_t kCapabilityRevision = 1;
constexpr uint32_t kFeatureSourceSwitch = 1u << 0;

enum class Source : uint8_t { none = 0, radio = 1, bluetooth = 2 };
enum class Transition : uint8_t {
    stable = 0,
    starting_bluetooth = 1,
    switching_radio = 2,
    failed = 3
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
    uint64_t generation{1};
    Source source{Source::none};
    Transition transition{Transition::stable};
    bool bluetooth_connected{false};
    uint8_t last_error{success};
};

struct SourceCommand {
    uint64_t request{0};
    uint64_t expected_generation{0};
    Source target{Source::none};
};

struct Decision {
    uint8_t error{invalid_argument};
    bool idempotent{false};
};

class SourceMailbox {
public:
    bool put(const SourceCommand& command);
    bool take(SourceCommand& command);
    void clear();
    bool occupied() const { return occupied_; }

private:
    uint64_t request_{0};
    uint64_t expected_generation_{0};
    Source target_{Source::none};
    bool occupied_{false};
};

size_t capabilities(uint8_t* output, size_t capacity);
size_t sourceState(const SourceState& state, uint64_t sequence,
                   uint8_t* output, size_t capacity);
bool parseSourceCommand(const uint8_t* input, size_t size,
                        SourceCommand& command);
Decision admitSource(const SourceState& state, const SourceCommand& command,
                     bool command_pending);

static_assert(sizeof(SourceState) <= 16,
              "full-control source state exceeded its static budget");
static_assert(sizeof(SourceMailbox) <= 24,
              "full-control mailbox exceeded its static budget");

}  // namespace rtcp_full_control
