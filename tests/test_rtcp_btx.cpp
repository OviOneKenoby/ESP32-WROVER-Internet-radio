#include "../src/rtcp_btx.h"
#include "../src/ccm_wire.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

namespace {
unsigned checks = 0;
void expect(bool value, const char* message) {
    ++checks;
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
void put64(uint8_t* p, uint64_t value) {
    for (int i = 7; i >= 0; --i) { p[i] = uint8_t(value); value >>= 8; }
}
rtcp_btx::Snapshot connected(rtcp_btx::Playback playback = rtcp_btx::Playback::playing) {
    rtcp_btx::Snapshot value;
    value.state.source = rtcp_btx::Source::bluetooth;
    value.state.connection = rtcp_btx::Connection::connected;
    value.state.playback = playback;
    value.state.source_generation = 3;
    value.state.metadata_revision = 5;
    return value;
}
rtcp_btx::Command command(rtcp_btx::Operation operation, rtcp_btx::Playback expected) {
    rtcp_btx::Command value;
    value.request = 0x0102030405060708ULL;
    value.source_generation = 3;
    value.operation = operation;
    value.expected_playback = expected;
    return value;
}
}

int main() {
    using namespace rtcp_btx;
    uint8_t output[512]{};

    Session old_remote;
    expect(old_remote.capabilityRequest() == CapabilityReply::legacy,
           "first capability must remain legacy");
    uint8_t legacy[16]{};
    expect(ccm_wire::capabilities(legacy, sizeof legacy) == 16,
           "legacy capability size");
    const uint8_t expected_legacy[16] = {4,255,0,0,0,0,0,0,0,2,0,0,7,208,1,100};
    expect(memcmp(legacy, expected_legacy, sizeof legacy) == 0,
           "legacy capability bytes changed");
    old_remote.observeNonCapability();
    expect(old_remote.legacy(), "intervening request must lock legacy mode");
    expect(old_remote.capabilityRequest() == CapabilityReply::invalid,
           "out-of-window capability must be invalid");

    Session new_remote;
    expect(new_remote.capabilityRequest() == CapabilityReply::legacy,
           "BTX session first response legacy");
    expect(new_remote.capabilityRequest() == CapabilityReply::btx && new_remote.negotiated(),
           "second consecutive capability negotiates BTX");
    expect(new_remote.capabilityRequest() == CapabilityReply::invalid,
           "third capability request rejected");
    new_remote.clear();
    expect(!new_remote.negotiated() && !new_remote.legacy() &&
               new_remote.capabilityRequest() == CapabilityReply::legacy,
           "disconnect clears negotiation");

    expect(capabilities(output, sizeof output) == 16, "BTX capability size");
    const uint8_t expected_btx[16] = {12,1,0,0x3f,96,127,0x07,0xd0,0,0,0,0,0,0,0,1};
    expect(memcmp(output, expected_btx, 16) == 0, "BTX capability vector");

    Snapshot radio;
    radio.state.source = Source::radio;
    radio.state.source_generation = 2;
    radio.state.metadata_revision = 4;
    strcpy(radio.artist, "stale"); radio.artist_size = 5;
    expect(stateResponse(radio, 6, output, sizeof output) == 32,
           "non-Bluetooth canonical response");
    expect(output[2] == 1 && output[3] == 0 && output[4] == 0 && output[5] == 0 &&
               output[6] == 0 && output[7] == 0,
           "non-Bluetooth fields cleared");

    Snapshot disconnected = connected();
    disconnected.state.connection = Connection::disconnected;
    strcpy(disconnected.artist, "stale"); disconnected.artist_size = 5;
    expect(stateResponse(disconnected, 7, output, sizeof output) == 32,
           "disconnected response size");
    expect(output[4] == 0 && output[5] == 0 && output[6] == 0,
           "disconnected state clears playback metadata and operations");

    Snapshot state = connected();
    strcpy(state.artist, "Example Artist"); state.artist_size = 14;
    strcpy(state.title, "Example Title"); state.title_size = 13;
    expect(stateResponse(state, 7, output, sizeof output) == 59,
           "metadata state response size");
    expect(output[0] == 14 && output[1] == 1 && output[2] == 2 && output[3] == 3 &&
               output[4] == 2 && output[5] == 0x3b,
           "metadata state header and availability");
    expect(memcmp(output + 32, "Example ArtistExample Title", 27) == 0,
           "metadata field order");

    Snapshot artist_only = connected(Playback::paused);
    strcpy(artist_only.artist, "Artist"); artist_only.artist_size = 6;
    expect(stateResponse(artist_only, 8, output, sizeof output) == 38 &&
               output[5] == (artist_valid | play_available | previous_available | next_available),
           "artist-only paused state");
    Snapshot title_only = connected(Playback::stopped);
    strcpy(title_only.title, "Title"); title_only.title_size = 5;
    expect(stateResponse(title_only, 8, output, sizeof output) == 37 &&
               output[5] == (title_valid | play_available | previous_available | next_available),
           "title-only stopped state");
    Snapshot empty = connected(Playback::unknown);
    expect(stateResponse(empty, 8, output, sizeof output) == 32 && output[5] == 0x30,
           "empty connected state retains traversal availability");

    Snapshot maximum = connected(Playback::paused);
    memset(maximum.artist, 'A', kMaxArtistBytes); maximum.artist[kMaxArtistBytes] = 0;
    memset(maximum.title, 'T', kMaxTitleBytes); maximum.title[kMaxTitleBytes] = 0;
    maximum.artist_size = kMaxArtistBytes; maximum.title_size = kMaxTitleBytes;
    expect(stateResponse(maximum, 8, output, sizeof output) == 255,
           "maximum state plaintext is 255 bytes");
    expect(255 + 28 + 16 == 299, "maximum authenticated record is 299 bytes");

    char text[128]{}; uint8_t text_size = 0;
    char utf8[110]{}; memset(utf8, 'A', 95); utf8[95] = char(0xe2);
    utf8[96] = char(0x82); utf8[97] = char(0xac); utf8[98] = 0;
    expect(copyLocalText(utf8, 96, text, sizeof text, text_size) && text_size == 95,
           "UTF-8 truncation preserves code-point boundary");
    const char malformed[] = {char(0xc0), char(0xaf), 0};
    expect(!copyLocalText(malformed, 96, text, sizeof text, text_size) && text_size == 0,
           "malformed local UTF-8 rejected");
    const char control[] = {'A', '\n', 0};
    expect(!copyLocalText(control, 96, text, sizeof text, text_size),
           "control characters rejected");

    uint8_t wire_command[19] = {15};
    put64(wire_command + 1, 0x0102030405060708ULL);
    wire_command[9] = 2; wire_command[10] = 2; put64(wire_command + 11, 3);
    Command parsed;
    expect(parseCommand(wire_command, sizeof wire_command, parsed), "command vector parsed");
    expect(parsed.request == 0x0102030405060708ULL && parsed.operation == Operation::pause &&
               parsed.expected_playback == Playback::playing && parsed.source_generation == 3,
           "command fields decoded");
    expect(!parseCommand(wire_command, sizeof wire_command - 1, parsed),
           "short command rejected");
    wire_command[9] = 5;
    expect(!parseCommand(wire_command, sizeof wire_command, parsed), "invalid operation rejected");

    Command pause = command(Operation::pause, Playback::playing);
    expect(admit(state, pause).error == success, "pause admitted while playing");
    Command resume = command(Operation::play, Playback::paused);
    expect(admit(artist_only, resume).error == success, "play admitted while paused");
    Command previous = command(Operation::previous, Playback::playing);
    expect(admit(state, previous).error == success, "previous admitted while connected");
    Command forward = command(Operation::next, Playback::playing);
    expect(admit(state, forward).error == success, "next admitted while connected");
    pause.source_generation = 4;
    expect(admit(state, pause).error == stale, "source generation mismatch rejected");
    pause.source_generation = 3; pause.expected_playback = Playback::paused;
    expect(admit(state, pause).error == stale, "playback mismatch rejected");
    pause.expected_playback = Playback::playing; disconnected.state.playback = Playback::playing;
    expect(admit(disconnected, pause).error == unavailable, "disconnected command rejected");
    Command play = command(Operation::play, Playback::playing);
    expect(admit(state, play).error == unavailable, "unavailable play rejected");
    Command next = command(Operation::next, Playback::unknown);
    expect(admit(empty, next).error == success, "next admitted with connected unknown playback");

    Mailbox mailbox;
    expect(mailbox.put(pause), "first command queued");
    expect(!mailbox.put(pause), "mailbox busy rejects second command");
    Command taken;
    expect(mailbox.take(taken) && taken.request == pause.request && !mailbox.occupied(),
           "mailbox command correlated and cleared");
    expect(!mailbox.take(taken), "empty mailbox has no stale command");
    mailbox.put(pause); mailbox.clear();
    expect(!mailbox.occupied(), "disconnect clears mailbox");

    uint64_t counter = 1;
    expect(advance(counter) && counter == 2, "generation advances monotonically");
    counter = std::numeric_limits<uint64_t>::max();
    expect(!advance(counter) && counter == 0, "generation exhaustion fails closed without wrap");

    std::cout << "PASS " << checks << "/" << checks << " BTX assertions\n";
    return 0;
}
