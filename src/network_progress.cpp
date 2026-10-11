#include "network_progress.h"
#include "system_info.h"
#include <Arduino.h>
#include <esp_netif.h>
#include <esp_netif_net_stack.h>
#include <esp_private/wifi.h>
#include <esp_spi_flash.h>

// Only documented-in-pinned-headers boundaries. No private queue access,
// callback replacement, payload logging, allocation or retry. Header summaries
// are bounded and made before handing the readable buffer to the original.
// The original ABI/arguments/results and pbuf ownership are preserved.
namespace {
DRAM_ATTR NetworkProgress::Snapshot progress;
DRAM_ATTR TxRefusalEvidence evidence;
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
esp_err_t __real_esp_wifi_init(const wifi_init_config_t*);

esp_err_t IRAM_ATTR __wrap_esp_wifi_init(const wifi_init_config_t* cfg) {
    // Copy only declared scalar fields; never modify cfg or retain its pointer.
    const bool captured = cfg && spi_flash_cache_enabled();
    const int32_t type = captured ? cfg->tx_buf_type : -1;
    const int32_t dynamic = captured ? cfg->dynamic_tx_buf_num : -1;
    const int32_t cache = captured ? cfg->cache_tx_buf_num : -1;
    const int32_t stat = captured ? cfg->static_tx_buf_num : -1;
    const uint64_t features = captured ? cfg->feature_caps : 0;
    const esp_err_t result = __real_esp_wifi_init(cfg);
    portENTER_CRITICAL(&progressMux);
    increment(evidence.initCalls);
    evidence.configCaptured = captured;
    evidence.txType = type; evidence.dynamicTx = dynamic;
    evidence.cacheTx = cache; evidence.staticTx = stat;
    evidence.featureLow = uint32_t(features); evidence.featureHigh = uint32_t(features >> 32);
    evidence.initResult = result;
    portEXIT_CRITICAL(&progressMux);
    return result;
}

esp_err_t IRAM_ATTR __wrap_esp_wifi_internal_tx_by_ref(
        wifi_interface_t ifx, void* buffer, size_t len, void* pbuf) {
    const PacketKind kind = spi_flash_cache_enabled() ? packetKind(buffer, len) : PacketKind::Other;
    const uint32_t failuresBefore = SystemInfo::allocationFailureCount();
    const esp_err_t result = __real_esp_wifi_internal_tx_by_ref(ifx, buffer, len, pbuf);
    const uint32_t failuresAfter = SystemInfo::allocationFailureCount();
    const uint32_t tick = observationTick();
    portENTER_CRITICAL(&progressMux);
    increment(progress.txCalls);
    if (result == ESP_OK) increment(progress.txAccepted);
    else if (result == ESP_ERR_NO_MEM) {
        increment(progress.txNoMem);
        TxRefusalRecord record;
        record.tick = tick; record.length = len;
        record.pbufHandle = reinterpret_cast<uint32_t>(pbuf);
        record.ifx = uint8_t(ifx); record.kind = kind; record.hasBuffer = buffer != nullptr;
        record.failuresBefore = failuresBefore; record.failuresAfter = failuresAfter;
        evidence.append(record);
    }
    else increment(progress.txOtherError);
    progress.txLastResult = result;
    setTick(progress.txLastTick, tick);
    portEXIT_CRITICAL(&progressMux);
    return result;
}
void IRAM_ATTR __wrap_esp_netif_netstack_buf_ref(void* pbuf) {
    __real_esp_netif_netstack_buf_ref(pbuf);
    const uint32_t tick = observationTick();
    portENTER_CRITICAL(&progressMux);
    increment(progress.refs);
    evidence.ledger.acquire(reinterpret_cast<uint32_t>(pbuf), tick);
    portEXIT_CRITICAL(&progressMux);
}
void IRAM_ATTR __wrap_esp_netif_netstack_buf_free(void* pbuf) {
    __real_esp_netif_netstack_buf_free(pbuf);
    const uint32_t tick = observationTick();
    portENTER_CRITICAL(&progressMux);
    increment(progress.frees);
    evidence.ledger.release(reinterpret_cast<uint32_t>(pbuf)); // identity only, no dereference
    setTick(progress.freeLastTick, tick);
    portEXIT_CRITICAL(&progressMux);
}
esp_err_t IRAM_ATTR __wrap_esp_netif_receive(
        esp_netif_t* netif, void* buffer, size_t len, void* eb) {
    const PacketKind kind = spi_flash_cache_enabled() ? packetKind(buffer, len) : PacketKind::Other;
    const esp_err_t result = __real_esp_netif_receive(netif, buffer, len, eb);
    const uint32_t tick = observationTick();
    portENTER_CRITICAL(&progressMux);
    increment(progress.rxCalls);
    if (result == ESP_OK) increment(progress.rxAccepted);
    if (result == ESP_OK && kind == PacketKind::EchoRequest) increment(evidence.rxEchoRequests);
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
TxRefusalEvidence NetworkProgress::refusalEvidence() {
    portENTER_CRITICAL(&progressMux);
    const TxRefusalEvidence copy = evidence;
    portEXIT_CRITICAL(&progressMux);
    return copy;
}
