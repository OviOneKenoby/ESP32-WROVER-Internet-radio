#include "rtcp_full_control.h"

#include <cassert>
#include <cstdint>
#include <cstdio>

using namespace rtcp_full_control;

int main() {
    uint8_t bytes[512]{};
    assert(capabilities(bytes, sizeof bytes) == 20);
    assert(bytes[0] == kCapabilityResponse && bytes[1] == kMajor);
    assert(bytes[7] == (kFeatureSourceSwitch | kFeatureMenuProjection));
    assert(bytes[2] == 1 && bytes[19] == kCapabilityRevision);

    SourceState state{};
    state.generation = 9;
    state.source = Source::radio;
    assert(sourceState(state, 7, bytes, sizeof bytes) == 24);
    assert(bytes[0] == kSourceStateResponse && bytes[2] == uint8_t(Source::radio));

    uint8_t command_bytes[18]{};
    command_bytes[0] = kSourceSelectRequest;
    command_bytes[8] = 3;
    command_bytes[9] = uint8_t(Source::bluetooth);
    command_bytes[17] = 9;
    SourceCommand command{};
    assert(parseSourceCommand(command_bytes, sizeof command_bytes, command));
    assert(command.request == 3 && command.expected_generation == 9);

    Decision decision = admitSource(state, command, false);
    assert(decision.error == success && !decision.idempotent);
    assert(admitSource(state, command, true).error == busy);
    command.expected_generation = 8;
    assert(admitSource(state, command, false).error == stale);
    command.expected_generation = 9;
    command.target = Source::radio;
    decision = admitSource(state, command, false);
    assert(decision.error == success && decision.idempotent);

    SourceMailbox mailbox;
    assert(mailbox.put(command));
    assert(!mailbox.put(command));
    SourceCommand taken{};
    assert(mailbox.take(taken));
    assert(taken.request == command.request && taken.target == Source::radio);
    assert(!mailbox.take(taken));

    command_bytes[9] = 99;
    assert(!parseSourceCommand(command_bytes, sizeof command_bytes, command));
    assert(sourceState(SourceState{}, 0, bytes, sizeof bytes) == 0);

    MenuView menu{};
    menu.mode = MenuMode::favorites;
    menu.selected = 1;
    menu.count = 3;
    menu.window_count = 3;
    menu.items[0] = {0, false, "Alpha"};
    menu.items[1] = {1, true, "Béta"};
    menu.items[2] = {2, false, "Gamma"};
    MenuRevision revision;
    assert(revision.observe(menu) == 1);
    assert(revision.observe(menu) == 1);
    menu.selected = 2;
    assert(revision.observe(menu) == 2);
    const size_t menu_size = menuState(menu, 12, revision.value(), bytes, sizeof bytes);
    assert(menu_size == 28 + 9 + 9 + 9);
    assert(bytes[0] == kMenuStateResponse && bytes[1] == kMajor);
    assert(bytes[2] == uint8_t(MenuMode::favorites) && bytes[10] == 3);
    assert(bytes[31] == 5 && bytes[40] == 5 && bytes[49] == 5);

    MenuView empty{};
    empty.mode = MenuMode::recent;
    assert(menuState(empty, 1, 1, bytes, sizeof bytes) == 28);
    empty.selected = 1;
    assert(menuState(empty, 1, 1, bytes, sizeof bytes) == 0);

    menu.selected = 1;
    menu.items[1].name = "bad\x01text";
    assert(menuState(menu, 1, 1, bytes, sizeof bytes) == 0);
    menu.items[1].name = "Béta";
    menu.window_count = kMenuWindowItems + 1;
    assert(menuState(menu, 1, 1, bytes, sizeof bytes) == 0);
    std::puts("full-control source/menu model: PASS");
}
