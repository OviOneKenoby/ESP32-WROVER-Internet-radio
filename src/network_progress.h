#pragma once
#include <cstdint>
#include "tx_refusal_evidence.h"

namespace NetworkProgress {
struct Snapshot {
    uint32_t txCalls = 0, txAccepted = 0, txNoMem = 0, txOtherError = 0;
    uint32_t refs = 0, frees = 0, rxCalls = 0, rxAccepted = 0;
    uint32_t txLastTick = 0, freeLastTick = 0, rxLastTick = 0;
    int32_t txLastResult = 0;
    uint32_t ticksUnavailable = 0;
};
Snapshot snapshot();
TxRefusalEvidence refusalEvidence();
}
