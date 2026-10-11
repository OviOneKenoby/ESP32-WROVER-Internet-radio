#pragma once
#include <cstdint>
#include <cstddef>

// Pure bounded Ethernet summary, no payload retained. VLAN/fragmented packets
// are deliberately not guessed. Caller owns a valid readable length-byte span.
enum class PacketKind : uint8_t { Other, Arp, Tcp80, EchoRequest, EchoReply };
inline __attribute__((always_inline)) PacketKind packetKind(const void* buffer, size_t length) {
    if (!buffer || length < 14) return PacketKind::Other;
    const auto* p = static_cast<const uint8_t*>(buffer);
    if (p[12] == 8 && p[13] == 6) return PacketKind::Arp;
    if (p[12] != 8 || p[13] != 0 || length < 34 || (p[14] >> 4) != 4) return PacketKind::Other;
    const size_t ihl = (p[14] & 15) * 4;
    if (ihl < 20 || length < 14 + ihl || (p[20] & 0x3f) || p[21]) return PacketKind::Other;
    const size_t total = (size_t(p[16]) << 8) | p[17];
    if (total < ihl || length < 14 + total) return PacketKind::Other;
    const size_t offset = 14 + ihl;
    if (p[23] == 1 && total >= ihl + 8) {
        if (p[offset] == 8 && p[offset + 1] == 0) return PacketKind::EchoRequest;
        if (p[offset] == 0 && p[offset + 1] == 0) return PacketKind::EchoReply;
    }
    if (p[23] == 6 && total >= ihl + 20) {
        if ((p[offset] == 0 && p[offset + 1] == 80) ||
            (p[offset + 2] == 0 && p[offset + 3] == 80)) return PacketKind::Tcp80;
    }
    return PacketKind::Other;
}

struct RefLedger {
    struct Entry { uint32_t handle = 0, refs = 0, firstTick = 0; } entries[8];
    uint32_t overflow = 0, unmatchedFree = 0;
    inline __attribute__((always_inline)) void acquire(uint32_t handle, uint32_t tick) {
        if (!handle) { if (overflow != UINT32_MAX) ++overflow; return; }
        Entry* empty = nullptr;
        for (auto& entry : entries) {
            if (entry.handle == handle) {
                if (entry.refs != UINT32_MAX) ++entry.refs;
                else if (overflow != UINT32_MAX) ++overflow;
                return;
            }
            if (!entry.refs && !empty) empty = &entry;
        }
        if (empty) { empty->handle = handle; empty->refs = 1; empty->firstTick = tick; }
        else if (overflow != UINT32_MAX) ++overflow;
    }
    inline __attribute__((always_inline)) void release(uint32_t handle) {
        for (auto& entry : entries) if (entry.refs && entry.handle == handle) {
            if (--entry.refs == 0) entry = {};
            return;
        }
        if (unmatchedFree != UINT32_MAX) ++unmatchedFree;
    }
};
struct TxRefusalRecord {
    uint32_t sequence = 0, tick = 0, length = 0, pbufHandle = 0;
    uint32_t failuresBefore = 0, failuresAfter = 0;
    uint8_t ifx = 0;
    PacketKind kind = PacketKind::Other;
    bool hasBuffer = false;
};
struct TxRefusalEvidence {
    RefLedger ledger;
    TxRefusalRecord first[4], last;
    uint32_t refusals = 0, rxEchoRequests = 0, refusedEchoReplies = 0, refusedTcp80 = 0;
    uint32_t initCalls = 0, featureLow = 0, featureHigh = 0;
    int32_t initResult = -1, txType = -1, dynamicTx = -1, cacheTx = -1, staticTx = -1;
    bool configCaptured = false;
    inline __attribute__((always_inline)) void append(TxRefusalRecord record) {
        if (refusals != UINT32_MAX) ++refusals;
        record.sequence = refusals;
        if (refusals <= 4) first[refusals - 1] = record;
        last = record;
        if (record.kind == PacketKind::EchoReply && refusedEchoReplies != UINT32_MAX) ++refusedEchoReplies;
        if (record.kind == PacketKind::Tcp80 && refusedTcp80 != UINT32_MAX) ++refusedTcp80;
    }
};
