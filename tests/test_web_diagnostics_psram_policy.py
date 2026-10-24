from pathlib import Path


root = Path(__file__).resolve().parents[1]
source = (root / "src" / "web_portal.cpp").read_text(encoding="utf-8")
allocator = (root / "src" / "diagnostics_psram_allocator.h").read_text(
    encoding="utf-8"
)


def function(name, next_name):
    start = source.index(f"void WebPortal::{name}")
    end = source.index(f"void WebPortal::{next_name}", start)
    return source[start:end]


checks = 0


def check(value, message):
    global checks
    checks += 1
    assert value, message


diagnostics = function("handleDiagnostics()", "handleStations()")
status = function("handleStatus()", "handleDiagnostics()")
stations = function("handleStations()", "handleAddStation()")
send_json = function("sendJson(int code, JsonDocument& document)",
                     "sendJsonError(int code, const char* message)")

check("public ArduinoJson::Allocator" in allocator,
      "diagnostics uses the supported ArduinoJson allocator interface")
check("MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT" in allocator,
      "allocator requires byte-addressable PSRAM")
check("heap_caps_malloc" in allocator and "heap_caps_realloc" in allocator and
      "heap_caps_free" in allocator,
      "allocator supplies the complete allocation lifecycle")
check("MALLOC_CAP_INTERNAL" not in allocator,
      "diagnostics allocator has no internal-memory fallback")
check("JsonDocument doc(&diagnosticsPsramAllocator);" in diagnostics,
      "only the full diagnostics document selects PSRAM")
check("JsonDocument doc;" in status and "JsonDocument doc;" in stations,
      "unrelated endpoint allocation policy remains unchanged")
check("setContentLength(expectedBytes)" in send_json and
      "BufferedWebWriter<ObservedWebServer>" in send_json,
      "fixed-length bounded streaming remains unchanged")
check("static const char WEB_PAGE[] PROGMEM" in source and
      "server.send_P(200, \"text/html; charset=utf-8\", WEB_PAGE)" in source,
      "flash-resident portal asset remains flash-served")

print(f"PASS {checks}/{checks} web diagnostics PSRAM policy checks")
