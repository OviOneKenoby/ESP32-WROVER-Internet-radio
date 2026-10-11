#include "network_progress.h"
#include <Arduino.h>
#include <esp_netif.h>
#include <esp_netif_net_stack.h>
#include <esp_private/wifi.h>
#include <esp_spi_flash.h>

// Only documented-in-pinned-headers boundaries. No private queue access,
// callback replacement, packet inspection, logging, allocation or retry.
// The original ABI/arguments/results and pbuf ownership are preserved.
namespace {
DRAM_ATTR NetworkProgress::Snapshot progress;
DRAM_ATTR portMUX_TYPE progressMux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR increment(uint32_t& value) { if (value != UINT32_MAX) ++value; }
uint32_t IRAM_ATTR observationTick() {
    return spi_flash_cache_enabled() ? xTaskGetTickCountFromISR() : UINT32_MAX;
}
void IRAM_ATTR setTick(uint32_t& field, uint32_t tick) {
    if (tick != UINT32_MAX) field = tick;
    else increment(progress.ticksUnavailable);
}
}
extern "C" {
esp_err_t __real_esp_wifi_internal_tx_by_ref(wifi_interface_t, void*, size_t, void*);
void __real_esp_netif_netstack_buf_ref(void*);
void __real_esp_netif_netstack_buf_free(void*);
esp_err_t __real_esp_netif_receive(esp_netif_t*, void*, size_t, void*);

esp_err_t IRAM_ATTR __wrap_esp_wifi_internal_tx_by_ref(
        wifi_interface_t ifx, void* buffer, size_t len, void* pbuf) {
    const esp_err_t result = __real_esp_wifi_internal_tx_by_ref(ifx, buffer, len, pbuf);
    const uint32_t tick = observationTick();
    portENTER_CRITICAL(&progressMux);
    increment(progress.txCalls);
    if (result == ESP_OK) increment(progress.txAccepted);
    else if (result == ESP_ERR_NO_MEM) increment(progress.txNoMem);
    else increment(progress.txOtherError);
    progress.txLastResult = result;
    setTick(progress.txLastTick, tick);
    portEXIT_CRITICAL(&progressMux);
    return result;
}
void IRAM_ATTR __wrap_esp_netif_netstack_buf_ref(void* pbuf) {
    __real_esp_netif_netstack_buf_ref(pbuf);
    portENTER_CRITICAL(&progressMux);
    increment(progress.refs);
    portEXIT_CRITICAL(&progressMux);
}
void IRAM_ATTR __wrap_esp_netif_netstack_buf_free(void* pbuf) {
    __real_esp_netif_netstack_buf_free(pbuf);
    const uint32_t tick = observationTick();
    portENTER_CRITICAL(&progressMux);
    increment(progress.frees);
    setTick(progress.freeLastTick, tick);
    portEXIT_CRITICAL(&progressMux);
}
esp_err_t IRAM_ATTR __wrap_esp_netif_receive(
        esp_netif_t* netif, void* buffer, size_t len, void* eb) {
    const esp_err_t result = __real_esp_netif_receive(netif, buffer, len, eb);
    const uint32_t tick = observationTick();
    portENTER_CRITICAL(&progressMux);
    increment(progress.rxCalls);
    if (result == ESP_OK) increment(progress.rxAccepted);
    setTick(progress.rxLastTick, tick);
    portEXIT_CRITICAL(&progressMux);
    return result;
}
}
NetworkProgress::Snapshot NetworkProgress::snapshot() {
    portENTER_CRITICAL(&progressMux);
    const Snapshot copy = progress;
    portEXIT_CRITICAL(&progressMux);
    return copy;
}
