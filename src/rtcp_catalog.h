#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace rtcp_catalog {

constexpr uint8_t kQueryRequest = 23;
constexpr uint8_t kPageResponse = 24;
constexpr uint8_t kCommandRequest = 25;
constexpr uint8_t kMajor = 1;
constexpr size_t kWindowItems = 5;
constexpr size_t kNameBytes = 48;

enum class Collection : uint8_t { saved = 1, favorites = 2, recent = 3 };
enum class Operation : uint8_t { play = 1, remove = 2, add_favorite = 3 };
enum Flag : uint8_t { full = 1u << 0 };

enum Error : uint8_t {
    success = 0,
    invalid_argument = 1,
    unsupported = 2,
    unavailable = 3,
    stale = 4,
    execution_failed = 5,
    busy = 6,
    internal = 7,
    already_present = 8,
    capacity_full = 9
};

struct Query {
    Collection collection{Collection::saved};
    uint16_t offset{0};
    uint8_t limit{0};
};

struct ItemView {
    uint16_t token{0};
    bool active{false};
    const char* name{nullptr};
};

struct PageView {
    Collection collection{Collection::saved};
    uint8_t flags{0};
    uint8_t capacity{0};
    uint16_t count{0};
    uint16_t start{0};
    uint8_t item_count{0};
    uint64_t revision{0};
    std::array<ItemView, kWindowItems> items{};
};

struct Command {
    uint64_t request{0};
    Operation operation{Operation::play};
    Collection collection{Collection::saved};
    uint16_t token{0};
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
size_t page(const PageView& view, uint64_t sequence,
            uint8_t* output, size_t capacity);
bool parseCommand(const uint8_t* input, size_t size, Command& command);
Decision admit(const Command& command, uint64_t current_revision,
               uint16_t current_count, bool pending);

static_assert(sizeof(Mailbox) <= 32,
              "catalog mailbox exceeded its fixed storage budget");

}  // namespace rtcp_catalog
