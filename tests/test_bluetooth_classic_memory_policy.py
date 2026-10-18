from pathlib import Path


source = (Path(__file__).resolve().parents[1] / "src" / "audio.cpp").read_text(
    encoding="utf-8"
)
init = source.split("int init_bluetooth() override {", 1)[1].split(
    "void init_i2s() override", 1
)[0]

# The first A2DP start must release only the unused BLE controller region,
# then initialize and enable the controller in Classic-only mode.  The base
# A2DP initialization runs only after the controller is confirmed enabled.
assert "esp_bt_controller_mem_release(ESP_BT_MODE_BLE)" in init
assert "config.mode = ESP_BT_MODE_CLASSIC_BT;" in init
assert "esp_bt_controller_init(&config)" in init
assert "esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)" in init
assert init.index("esp_bt_controller_mem_release") < init.index(
    "esp_bt_controller_init"
)
assert init.index("esp_bt_controller_init") < init.index(
    "esp_bt_controller_enable"
)
assert init.index("ESP_BT_CONTROLLER_STATUS_ENABLED") < init.index(
    "BluetoothA2DPSink::init_bluetooth()"
)

# Re-entry must retain the existing restartable controller rather than trying
# to release memory twice.  Every controller API failure remains fail-closed.
assert "if (!bleControllerMemoryReleased)" in init
assert "bleControllerMemoryReleased = true;" in init
assert init.count("return false;") >= 4

print("Bluetooth Classic-only controller memory policy: 10/10 PASS")
