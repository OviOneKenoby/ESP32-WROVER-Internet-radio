#include "tx_refusal_evidence.h"
#include <array>
#include <cstdlib>
#include <iostream>
static unsigned checks;
void expect(bool pass) { ++checks; if (!pass) std::exit(1); }
int main() {
    std::array<uint8_t, 74> frame{};
    frame[12] = 8; frame[14] = 0x45; frame[17] = 40; frame[23] = 6;
    frame[37] = 80;
    for (size_t length = 0; length < 54; ++length)
        expect(packetKind(frame.data(), length) == PacketKind::Other);
    expect(packetKind(nullptr, 54) == PacketKind::Other);
    expect(packetKind(frame.data(), 54) == PacketKind::Tcp80);
    frame[17] = 28; frame[23] = 1; frame[34] = 8;
    expect(packetKind(frame.data(), 42) == PacketKind::EchoRequest);
    frame[34] = 0;
    expect(packetKind(frame.data(), 42) == PacketKind::EchoReply);
    frame[20] = 0x20;
    expect(packetKind(frame.data(), 42) == PacketKind::Other);
    frame[20] = 0; frame[14] = 0x4f;
    expect(packetKind(frame.data(), 42) == PacketKind::Other);
    frame[14] = 0x45; frame[17] = 20;
    expect(packetKind(frame.data(), 74) == PacketKind::Other); // padding isn't IP payload
    RefLedger ledger;
    ledger.acquire(1, 7); ledger.acquire(1, 9);
    expect(ledger.entries[0].refs == 2 && ledger.entries[0].firstTick == 7);
    ledger.release(1); ledger.release(1);
    expect(ledger.entries[0].handle == 0 && ledger.entries[0].refs == 0);
    ledger.release(1); expect(ledger.unmatchedFree == 1);
    for (unsigned handle = 1; handle <= 9; ++handle) ledger.acquire(handle, 10);
    expect(ledger.overflow == 1);
    TxRefusalEvidence evidence;
    TxRefusalRecord event;
    event.kind = PacketKind::EchoReply; event.pbufHandle = 42;
    for (unsigned i = 0; i < 8; ++i) evidence.append(event);
    expect(evidence.refusals == 8 && evidence.refusedEchoReplies == 8 &&
           evidence.first[0].sequence == 1 && evidence.last.sequence == 8);
    evidence.refusals = UINT32_MAX; evidence.append(event);
    expect(evidence.refusals == UINT32_MAX && evidence.first[0].pbufHandle == 42);
    std::cout << "PASS: " << checks << " refusal evidence checks\n";
}
