"""Compile the actual policy with host allocator/context doubles; no hardware claim."""
import subprocess
import os
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
os.environ["PATH"] = str(Path(sys.argv[1]).resolve().parent) + os.pathsep + os.environ["PATH"]
with tempfile.TemporaryDirectory(prefix="tuner-psram-test-") as tmp:
    p = Path(tmp)
    headers = {
        "Arduino.h": """#include <cstdint>
#include <cstddef>
#define DRAM_ATTR
#define IRAM_ATTR
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
extern bool xPortInIsrContext();
struct SerialStub { template<class... A> void printf(const char*, A...) {} };
extern SerialStub Serial;
""",
        "lwip/mem.h": "#include <cstddef>\nusing mem_size_t=size_t;\n",
        "lwip/opt.h": "#define MEM_LIBC_MALLOC 1\n#define MEM_STATS 0\n#define MEM_ALIGNMENT 4\n",
        "esp_heap_caps.h": "#include <cstddef>\n#define MALLOC_CAP_SPIRAM 0x400\n#define MALLOC_CAP_8BIT 4\nextern void* heap_caps_malloc(size_t, unsigned);\n",
        "esp_spi_flash.h": "extern bool spi_flash_cache_enabled();\n",
        "soc/soc_memory_layout.h": "",
    }
    for name, content in headers.items():
        target = p / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content)
    (p / "test.cpp").write_text(r'''
#include <Arduino.h>
#include <cassert>
SerialStub Serial;
bool cache=true, isr=false, failExternal=false, failOriginal=false;
unsigned extCalls=0, originalCalls=0, capsSeen=0;
size_t sizeSeen=0; int externalBlock, originalBlock;
bool spi_flash_cache_enabled() { return cache; }
bool xPortInIsrContext() { return isr; }
void* heap_caps_malloc(size_t n, unsigned caps) {
    ++extCalls; sizeSeen=n; capsSeen=caps;
    return failExternal ? nullptr : &externalBlock;
}
extern "C" void* __real_mem_malloc(size_t n) {
    ++originalCalls; sizeSeen=n;
    return failOriginal ? nullptr : &originalBlock;
}
extern "C" void* __wrap_mem_malloc(size_t);
void check(size_t size, bool c, bool i, bool ef, bool of,
           unsigned expectedExternal, unsigned expectedOriginal, void* expected) {
    cache=c; isr=i; failExternal=ef; failOriginal=of;
    extCalls=originalCalls=0;
    assert(__wrap_mem_malloc(size)==expected);
    assert(extCalls==expectedExternal && originalCalls==expectedOriginal);
    assert(sizeSeen==size);
    if (extCalls) assert(capsSeen==0x404);
}
int main() {
    check(511,true,false,false,false,0,1,&originalBlock);
    check(512,true,false,false,false,1,0,&externalBlock);
    check(1622,true,false,true,false,1,1,&originalBlock);
    check(4096,true,false,true,true,1,1,nullptr);
    check(512,false,false,false,false,0,1,&originalBlock);
    check(512,true,true,false,false,0,1,&originalBlock);
    check(0,true,false,false,false,0,1,&originalBlock);
}
''')
    exe = p / "test.exe"
    subprocess.run([sys.argv[1], "-std=c++17", "-DTUNER_LWIP_PSRAM_TX=1", "-I"+str(p),
                    "-I"+str(root / "src"), str(root / "src/lwip_psram_policy.cpp"),
                    str(p / "test.cpp"), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("PASS 7 actual-wrapper cases: boundary, PSRAM caps/size, fallback once, NULL, cache/ISR, zero")
print("Host doubles only: target ABI/free/routing require exact-ELF checks; hardware NOT RUN")
