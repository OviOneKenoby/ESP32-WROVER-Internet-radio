#pragma once
#include <esp_wifi.h>
namespace SoftwareTxProbe {
const wifi_init_config_t* prepare(const wifi_init_config_t*);
int beginTx();
void endTx(int slot, esp_err_t result, bool eligible);
void printSnapshot();
}
