#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace rtcp_browser {

constexpr uint8_t kStateRequest = 26;
constexpr uint8_t kStateResponse = 27;
constexpr uint8_t kCommandRequest = 28;
constexpr uint8_t kMajor = 1;
constexpr size_t kWindowItems = 5;
constexpr size_t kNameBytes = 48;
constexpr uint16_t kAnyToken = 0xffff;

enum class Stage : uint8_t { countries = 1, genres = 2, results = 3 };
enum class Status : uint8_t {
    idle = 0,
    loading = 1,
    ready = 2,
    empty = 3,
    failed = 4
};
enum class Operation : uint8_t {
    start = 1,
    select_country = 2,
    select_name = 3,
    select_genre = 4,
    play = 5,
    add_favorite = 6,
    cancel = 7
};
enum Error : uint8_t {
    success = 0,
    invalid_argument = 1,
    unsupported = 2,
    unavailable = 3,
    stale = 4,
    execution_failed = 5,
    busy = 6,
    internal = 7,
    network = 8,
    response_too_large = 9,
    parse_failed = 10
};

struct Query {
    Stage stage{Stage::countries};
    uint16_t offset{0};
    uint8_t limit{0};
    uint8_t filter{0};
    uint64_t generation{0};
};

struct ItemView {
    uint16_t token{0};
    bool active{false};
    const char* name{nullptr};
};

struct StateView {
    Stage stage{Stage::countries};
    Status status{Status::idle};
    uint8_t error{success};
    uint16_t count{0};
    uint16_t start{0};
    uint8_t item_count{0};
    uint64_t generation{0};
    uint64_t revision{0};
    std::array<ItemView, kWindowItems> items{};
};

struct Command {
    uint64_t request{0};
    Operation operation{Operation::start};
    uint16_t token{0};
    uint8_t value{0};
    uint64_t expected_generation{0};
    uint64_t expected_revision{0};
};

struct Decision {
    uint8_t error{invalid_argument};
};

class Mailbox {
public:
    bool put(const Command& command);
    bool take(Command& command);
    void clear();
    bool occupied() const { return occupied_; }

private:
    Command command_{};
    bool occupied_{false};
};

bool parseQuery(const uint8_t* input, size_t size, Query& query);
size_t state(const StateView& view, uint64_t sequence,
             uint8_t* output, size_t capacity);
bool parseCommand(const uint8_t* input, size_t size, Command& command);
Decision admit(const Command& command, uint64_t generation, uint64_t revision,
               Stage stage, Status status, uint16_t count, bool pending);

static_assert(sizeof(Mailbox) <= 40,
              "browser mailbox exceeded its fixed storage budget");

}  // namespace rtcp_browser
