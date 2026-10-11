#ifdef TUNER_UART_TX_PROBE
#include "software_tx_probe.h"
#include <Arduino.h>
#include <esp_private/wifi_os_adapter.h>
#include <esp_debug_helpers.h>
#include <esp_spi_flash.h>
#include <soc/soc_memory_layout.h>
#include <stddef.h>

// Symbol addresses only: never call private functions or inspect private data.
extern "C" const unsigned char esf_buf_alloc[];
extern "C" const unsigned char esf_buf_alloc_dynamic[];
extern "C" const unsigned char ieee80211_output_process[];
namespace {
static_assert(offsetof(wifi_osi_funcs_t, _mutex_unlock) == 88, "fixed SDK ABI");
static_assert(offsetof(wifi_osi_funcs_t, _malloc_internal) == 348, "fixed SDK ABI");
static_assert(offsetof(wifi_osi_funcs_t, _wifi_malloc) == 364, "fixed SDK ABI");
static_assert(sizeof(wifi_osi_funcs_t) == 468, "fixed SDK ABI");
enum Kind { Empty, Pop, RequeueNull, PendingEmpty, DynamicPassed, DynamicNull, Kinds };
struct ClassificationSite { const unsigned char* base; uint32_t offset; };
// Relocate BASE symbols only. Volatile loads force the offset addition to run
// on target, avoiding a linker-relaxed symbol+addend literal. The final ELF
// verifier evaluates every predicate and checks this complete DRAM table.
DRAM_ATTR const volatile ClassificationSite classificationSites[] = {
    {esf_buf_alloc, 0x7a}, {ieee80211_output_process, 0x4f},
    {ieee80211_output_process, 0x108}, {ieee80211_output_process, 0x226},
    {esf_buf_alloc_dynamic, 0x78}, {esf_buf_alloc_dynamic, 0x1a8},
    {ieee80211_output_process, 0xb7}, {ieee80211_output_process, 0xa9},
    {esf_buf_alloc_dynamic, 0xa8}
};
uint32_t IRAM_ATTR __attribute__((noinline)) classificationAddress(unsigned index) {
    return reinterpret_cast<uint32_t>(classificationSites[index].base) +
           classificationSites[index].offset;
}
bool IRAM_ATTR __attribute__((noinline)) conversionPc(uint32_t pc) {
    return pc == classificationAddress(6) || pc == classificationAddress(7);
}
bool IRAM_ATTR __attribute__((noinline)) mallocPc(uint32_t pc) {
    return pc == classificationAddress(8);
}
struct Record { uint32_t sequence, tick, task, pc; uint32_t pcs[12]; uint8_t kind, frames; };
struct Scope { uint32_t task, empty; };
struct Conversion { uint32_t task; bool uncertain, mallocSeen; };
struct State {
    uint32_t counts[Kinds], lastTick[Kinds], sequence, skipped, scopeOverflow;
    uint32_t pairedNoMem, unpairedNoMem, unexpectedEmpty;
    uint32_t conversionPassed, conversionPreHeapNull, conversionAfterMallocNull, conversionUnpaired;
    uint32_t conversionMalloc, conversionMallocOk, conversionMallocNull;
    Record first[4], latest[4];
    Scope scopes[4];
    Conversion conversion[2];
    bool ready;
};
DRAM_ATTR State state;
DRAM_ATTR wifi_osi_funcs_t adapter;
DRAM_ATTR wifi_init_config_t config;
DRAM_ATTR int32_t (*originalUnlock)(void*);
DRAM_ATTR void* (*originalInternalMalloc)(size_t);
DRAM_ATTR void* (*originalWifiMalloc)(size_t);
DRAM_ATTR portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR inc(uint32_t& n) { if (n != UINT32_MAX) ++n; }
// -Os can outline the SDK static-inline helper into flash. Force this
// equivalent normalization inline, including the pre-cache-check entry path.
uint32_t __attribute__((always_inline)) inline callPc(uint32_t pc) {
    if (pc & 0x80000000) pc = (pc & 0x3fffffff) | 0x40000000;
    return pc - 3;
}
int IRAM_ATTR __attribute__((noinline)) kindFor(uint32_t pc) {
    for (unsigned k = 0; k < Kinds; ++k)
        if (pc == classificationAddress(k)) return k;
    return -1;
}
int32_t IRAM_ATTR unlock(void* lock) {
    // IDF's stack-PC helper subtracts three (call instruction). Add it back:
    // these sites are RETURN addresses, not the decoded backtrace call PCs.
    const uint32_t pc = callPc(reinterpret_cast<uint32_t>(__builtin_return_address(0))) + 3;
    // Release the driver's lock BEFORE tracing. Never nest our mux inside it.
    const int32_t result = originalUnlock(lock);
    const int kind = kindFor(pc);
    if (kind < 0) return result;
    if (!spi_flash_cache_enabled() || xPortInIsrContext()) {
        portENTER_CRITICAL(&mux); inc(state.skipped); portEXIT_CRITICAL(&mux);
        return result;
    }
    const uint32_t task = reinterpret_cast<uint32_t>(xTaskGetCurrentTaskHandleForCPU(xPortGetCoreID()));
    const uint32_t tick = xTaskGetTickCountFromISR();
    Record record = {};
    record.pc = pc; record.task = task; record.tick = tick; record.kind = kind;
    // Frequent normal pop/admission calls get counters only. Rare refusal paths
    // get a bounded non-printing unwind; no allocation or heap queries.
    bool conversion = false;
    if (kind == Empty || kind == RequeueNull || kind == DynamicNull || kind == DynamicPassed) {
        esp_backtrace_frame_t frame = {};
        esp_backtrace_get_start(&frame.pc, &frame.sp, &frame.next_pc);
        for (unsigned i = 0; i < 12; ++i) {
            if (!esp_stack_ptr_is_sane(frame.sp)) break;
            const uint32_t framePc = callPc(frame.pc);
            if (!esp_ptr_executable(reinterpret_cast<void*>(framePc))) break;
            record.pcs[record.frames++] = framePc;
            if (conversionPc(framePc)) conversion = true;
            if (!frame.next_pc || !esp_backtrace_get_next_frame(&frame)) break;
        }
    }
    portENTER_CRITICAL(&mux);
    inc(state.counts[kind]); state.lastTick[kind] = tick;
    auto& stage = state.conversion[xPortGetCoreID()];
    if (kind == Pop) stage = {task, false, false};
    if (kind == DynamicPassed && !conversion && stage.task == task) stage.uncertain = true;
    if (kind == DynamicPassed && conversion) {
        inc(state.conversionPassed);
        if (stage.task != task) inc(state.conversionUnpaired);
        else { stage.uncertain = false; stage.mallocSeen = false; }
    }
    if (kind == DynamicNull && conversion) {
        if (stage.task != task || stage.uncertain) inc(state.conversionUnpaired);
        else if (stage.mallocSeen) inc(state.conversionAfterMallocNull);
        else inc(state.conversionPreHeapNull);
    }
    if (kind == Empty) for (auto& scope : state.scopes)
        if (scope.task == task) inc(scope.empty);
    if (record.frames && kind != DynamicPassed) {
        inc(state.sequence); record.sequence = state.sequence;
        if (state.sequence <= 4) state.first[state.sequence - 1] = record;
        else state.latest[(state.sequence - 5) % 4] = record;
    }
    portEXIT_CRITICAL(&mux);
    return result;
}
void IRAM_ATTR observeMalloc(uint32_t pc, void* result) {
    if (!mallocPc(pc)) return;
    if (!spi_flash_cache_enabled() || xPortInIsrContext()) {
        portENTER_CRITICAL(&mux); inc(state.skipped); portEXIT_CRITICAL(&mux);
        return;
    }
    const uint32_t task = reinterpret_cast<uint32_t>(xTaskGetCurrentTaskHandleForCPU(xPortGetCoreID()));
    bool conversion = false;
    esp_backtrace_frame_t frame = {};
    esp_backtrace_get_start(&frame.pc, &frame.sp, &frame.next_pc);
    for (unsigned i = 0; i < 12; ++i) {
        if (!esp_stack_ptr_is_sane(frame.sp)) break;
        const uint32_t framePc = callPc(frame.pc);
        if (!esp_ptr_executable(reinterpret_cast<void*>(framePc))) break;
        if (conversionPc(framePc)) conversion = true;
        if (!frame.next_pc || !esp_backtrace_get_next_frame(&frame)) break;
    }
    portENTER_CRITICAL(&mux);
    auto& stage = state.conversion[xPortGetCoreID()];
    if (conversion) {
        inc(state.conversionMalloc);
        inc(result ? state.conversionMallocOk : state.conversionMallocNull);
        if (stage.task == task) { stage.mallocSeen = true; stage.uncertain = false; }
        else inc(state.conversionUnpaired);
    } else if (stage.task == task) stage.uncertain = true;
    portEXIT_CRITICAL(&mux);
}
void* IRAM_ATTR internalMalloc(size_t size) {
    const uint32_t pc = callPc(reinterpret_cast<uint32_t>(__builtin_return_address(0))) + 3;
    void* const result = originalInternalMalloc(size);
    observeMalloc(pc, result);
    return result;
}
void* IRAM_ATTR wifiMalloc(size_t size) {
    const uint32_t pc = callPc(reinterpret_cast<uint32_t>(__builtin_return_address(0))) + 3;
    void* const result = originalWifiMalloc(size);
    observeMalloc(pc, result);
    return result;
}
}
const wifi_init_config_t* SoftwareTxProbe::prepare(const wifi_init_config_t* cfg) {
    // Only before init, once. Never mutate the global SDK table or a live table.
    if (!cfg || !cfg->osi_funcs || state.ready || cfg->tx_buf_type != 1 ||
        cfg->dynamic_tx_buf_num != 32 || cfg->cache_tx_buf_num != 4 ||
        cfg->static_tx_buf_num != 0 || !(cfg->feature_caps & 2) ||
        cfg->osi_funcs->_version != ESP_WIFI_OS_ADAPTER_VERSION ||
        cfg->osi_funcs->_magic != ESP_WIFI_OS_ADAPTER_MAGIC ||
        !cfg->osi_funcs->_mutex_unlock || !cfg->osi_funcs->_malloc_internal ||
        !cfg->osi_funcs->_wifi_malloc) return cfg;
    config = *cfg; adapter = *cfg->osi_funcs;
    originalUnlock = adapter._mutex_unlock;
    adapter._mutex_unlock = unlock;
    originalInternalMalloc = adapter._malloc_internal;
    originalWifiMalloc = adapter._wifi_malloc;
    adapter._malloc_internal = internalMalloc;
    adapter._wifi_malloc = wifiMalloc;
    config.osi_funcs = &adapter;
    state.ready = true;
    return &config;
}
int IRAM_ATTR SoftwareTxProbe::beginTx() {
    if (xPortInIsrContext() || !spi_flash_cache_enabled()) return -1;
    const uint32_t task = reinterpret_cast<uint32_t>(xTaskGetCurrentTaskHandleForCPU(xPortGetCoreID()));
    int slot = -1;
    portENTER_CRITICAL(&mux);
    bool duplicate = false;
    for (const auto& scope : state.scopes) if (scope.task == task) duplicate = true;
    if (!duplicate) for (int i = 0; i < 4; ++i) if (!state.scopes[i].task) {
        slot = i; state.scopes[i] = {task, 0}; break;
    }
    if (slot < 0) inc(state.scopeOverflow);
    portEXIT_CRITICAL(&mux);
    return slot;
}
void IRAM_ATTR SoftwareTxProbe::endTx(int slot, esp_err_t result, bool eligible) {
    portENTER_CRITICAL(&mux);
    const uint32_t empty = slot >= 0 ? state.scopes[slot].empty : 0;
    if (result == ESP_ERR_NO_MEM) {
        if (eligible && slot >= 0 && empty == 1) inc(state.pairedNoMem);
        else inc(state.unpairedNoMem);
    } else if (empty) inc(state.unexpectedEmpty);
    if (slot >= 0) state.scopes[slot] = {};
    portEXIT_CRITICAL(&mux);
}
void SoftwareTxProbe::printSnapshot() {
    State copy;
    portENTER_CRITICAL(&mux); copy = state; portEXIT_CRITICAL(&mux);
    // Normal serial snapshot path only. Each printf fragment fits its 64-byte
    // Arduino stack buffer, preventing printf's long-line malloc fallback.
    Serial.printf("[SW_TX] ready=%u skipped=%u\n", copy.ready, copy.skipped);
    Serial.printf("[SW_TX] scope_overflow=%u\n", copy.scopeOverflow);
    Serial.printf("[SW_TX] paired=%u unpaired=%u\n", copy.pairedNoMem, copy.unpairedNoMem);
    Serial.printf("[SW_TX] unexpected=%u\n", copy.unexpectedEmpty);
    Serial.printf("[SW_CONVERT] quota_passed=%u\n", copy.conversionPassed);
    Serial.printf("[SW_CONVERT] preheap_null=%u\n", copy.conversionPreHeapNull);
    Serial.printf("[SW_CONVERT] after_malloc_null=%u unpaired=%u\n", copy.conversionAfterMallocNull, copy.conversionUnpaired);
    Serial.printf("[SW_MALLOC] calls=%u ok=%u null=%u\n", copy.conversionMalloc, copy.conversionMallocOk, copy.conversionMallocNull);
    for (int k = 0; k < Kinds; ++k)
        Serial.printf("[SW_SITE] k=%u n=%u tick=%u\n", k, copy.counts[k], copy.lastTick[k]);
    const auto print = [](const Record& r) {
        if (!r.sequence) return;
        Serial.printf("[SW_TRACE] seq=%u kind=%u tick=%u\n", r.sequence, r.kind, r.tick);
        Serial.printf("[SW_TRACE] task=0x%08x site=0x%08x frames=%u\n", r.task, r.pc, r.frames);
        for (unsigned i = 0; i < r.frames; ++i)
            Serial.printf("[SW_PC] seq=%u frame=%u pc=0x%08x\n", r.sequence, i, r.pcs[i]);
    };
    for (const auto& r : copy.first) print(r);
    for (const auto& r : copy.latest) print(r);
    Serial.printf("[SW_TRACE] total=%u retained_max=8\n", copy.sequence);
}
#endif
