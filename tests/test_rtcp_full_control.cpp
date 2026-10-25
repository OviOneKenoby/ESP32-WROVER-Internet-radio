#include "rtcp_full_control.h"

#include <cassert>
#include <cstdint>
#include <cstdio>

using namespace rtcp_full_control;

int main() {
    uint8_t bytes[64]{};
    assert(capabilities(bytes, sizeof bytes) == 20);
    assert(bytes[0] == kCapabilityResponse && bytes[1] == kMajor);
    assert(bytes[7] == kFeatureSourceSwitch);

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
    std::puts("full-control source model: PASS");
}
