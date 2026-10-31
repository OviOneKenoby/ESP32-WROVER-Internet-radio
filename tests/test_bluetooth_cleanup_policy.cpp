#include "bluetooth_cleanup_policy.h"
#include <cassert>
#include <cstdio>

int main() {
    constexpr int ok = 0;
    constexpr int invalid_state = 0x103;
    assert(bluetoothProfileStopped(ok, ok, invalid_state));
    assert(bluetoothProfileStopped(invalid_state, ok, invalid_state));
    assert(!bluetoothProfileStopped(-1, ok, invalid_state));

    BluetoothCleanupState clean{true, true, true, true, true,
                                true, true, true, true};
    assert(clean.complete());
    clean.i2s_released = false;
    assert(!clean.complete());
    clean.i2s_released = true;
    clean.disconnect_complete = false;
    assert(!clean.complete());
    clean.disconnect_complete = true;
    clean.restart_enabled = false;
    assert(!clean.complete());
    std::puts("Bluetooth cleanup policy: PASS");
}
