#pragma once

#include <cstdint>

struct BluetoothCleanupState {
    BluetoothCleanupState(bool disconnect, bool avrc_controller,
                          bool avrc_target, bool a2dp, bool task, bool i2s,
                          bool controller, bool bluedroid, bool restart)
        : disconnect_complete(disconnect),
          avrc_controller_stopped(avrc_controller),
          avrc_target_stopped(avrc_target), a2dp_stopped(a2dp),
          task_stopped(task), i2s_released(i2s),
          controller_retained(controller), bluedroid_retained(bluedroid),
          restart_enabled(restart) {}

    bool disconnect_complete{false};
    bool avrc_controller_stopped{false};
    bool avrc_target_stopped{false};
    bool a2dp_stopped{false};
    bool task_stopped{false};
    bool i2s_released{false};
    bool controller_retained{false};
    bool bluedroid_retained{false};
    bool restart_enabled{false};

    bool complete() const {
        return disconnect_complete && avrc_controller_stopped &&
               avrc_target_stopped && a2dp_stopped && task_stopped &&
               i2s_released && controller_retained && bluedroid_retained &&
               restart_enabled;
    }
};

inline bool bluetoothProfileStopped(int32_t result, int32_t ok,
                                    int32_t invalid_state) {
    // Deinit is idempotent for teardown purposes: INVALID_STATE means the
    // profile is already absent.  Other errors must remain fatal.
    return result == ok || result == invalid_state;
}
