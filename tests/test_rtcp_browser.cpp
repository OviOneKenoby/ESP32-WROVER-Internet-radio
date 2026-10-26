#include "rtcp_browser.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

using namespace rtcp_browser;

namespace {
void put64(uint8_t* p, uint64_t value) {
    for (int i = 7; i >= 0; --i) { p[i] = uint8_t(value); value >>= 8; }
}
}

int main() {
    uint8_t bytes[512]{};
    bytes[0] = kStateRequest; bytes[1] = uint8_t(Stage::genres);
    bytes[3] = 5; bytes[4] = 5; put64(bytes + 6, 9);
    Query query{};
    assert(parseQuery(bytes, 14, query));
    assert(query.stage == Stage::genres && query.offset == 5 &&
           query.limit == 5 && query.generation == 9);
    bytes[4] = 6; assert(!parseQuery(bytes, 14, query)); bytes[4] = 5;
    bytes[1]=uint8_t(Stage::countries);bytes[5]='R';
    assert(parseQuery(bytes,14,query)&&query.filter=='R');
    bytes[5]='1';assert(!parseQuery(bytes,14,query));bytes[5]=0;
    std::memset(bytes+6,0,8);assert(parseQuery(bytes,14,query));
    bytes[1]=uint8_t(Stage::results);assert(!parseQuery(bytes,14,query));

    StateView view{};
    view.stage = Stage::results; view.status = Status::ready;
    view.count = 2; view.item_count = 2; view.generation = 4; view.revision = 7;
    view.items[0] = {0, false, "Alpha"};
    view.items[1] = {1, true, "B\xc3\xa9ta"};
    const size_t written = state(view, 3, bytes, sizeof bytes);
    assert(written == 36 + 9 + 9 && bytes[0] == kStateResponse);
    view.status = Status::failed; view.error = network; view.item_count = 0;
    assert(state(view, 4, bytes, sizeof bytes) == 36);
    view.error = success; assert(state(view, 4, bytes, sizeof bytes) == 0);

    uint8_t command_bytes[29]{};
    command_bytes[0] = kCommandRequest; command_bytes[8] = 1;
    command_bytes[9] = uint8_t(Operation::select_genre);
    command_bytes[10] = 0; command_bytes[11] = 1;
    put64(command_bytes + 13, 4); put64(command_bytes + 21, 7);
    Command command{};
    assert(parseCommand(command_bytes, sizeof command_bytes, command));
    assert(admit(command, 4, 7, Stage::genres, Status::ready, 2, false).error == success);
    assert(admit(command, 5, 7, Stage::genres, Status::ready, 2, false).error == stale);
    assert(admit(command, 4, 7, Stage::genres, Status::loading, 2, false).error == busy);
    command.token = 3;
    assert(admit(command, 4, 7, Stage::genres, Status::ready, 2, false).error == invalid_argument);
    command.operation = Operation::select_country;
    command.token = 37; // Opaque index into the complete country catalog.
    assert(admit(command, 4, 7, Stage::countries, Status::ready, 2, false).error == success);

    std::memset(command_bytes, 0, sizeof command_bytes);
    command_bytes[0] = kCommandRequest; command_bytes[8] = 2;
    command_bytes[9] = uint8_t(Operation::start);
    assert(parseCommand(command_bytes, sizeof command_bytes, command));
    assert(admit(command, 0, 0, Stage::countries, Status::idle, 0, false).error == success);
    Mailbox mailbox;
    assert(mailbox.put(command) && !mailbox.put(command));
    Command taken{}; assert(mailbox.take(taken) && taken.request == 2);
    assert(!mailbox.take(taken));
    std::puts("RTCP browser model: PASS");
}
