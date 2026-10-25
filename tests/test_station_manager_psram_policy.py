from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "src/stations.h").read_text(encoding="utf-8")
source = (root / "src/stations.cpp").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")

checks = 0


def check(value, message):
    global checks
    checks += 1
    assert value, message


check("RadioStation stations[MAX_STATIONS]" not in header and
      "RadioStation recent[MAX_RECENT]" not in header and
      "RadioStation favorites[MAX_FAVORITES]" not in header,
      "record arrays are no longer embedded in internal BSS")
check("RadioStation* recordStorage" in header,
      "manager retains one lifetime storage pointer")
check(source.count("heap_caps_malloc(") == 1,
      "station storage has one allocation site")
check("MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT" in source,
      "allocation explicitly requires byte-addressable PSRAM")
check("MALLOC_CAP_INTERNAL" not in source,
      "station storage has no internal fallback")
check("::new (static_cast<void*>(&records[i])) RadioStation{}" in source,
      "every record has an explicit C++ object lifetime")
check("recordStorage[i].~RadioStation()" in source and
      "heap_caps_free(recordStorage)" in source,
      "record lifetime and allocation lifetime are paired")
check(main.index("stationManager.begin()") <
      main.index("stationManager.loadDefaultStations()") <
      main.index("stationManager.loadPersistedLists()"),
      "setup allocates before the first record access")
check("persistence left untouched" in main,
      "allocation failure leaves persistence untouched")
for namespace in ('"stations"', '"recent"', '"favorites"'):
    check(namespace in source, f"NVS namespace {namespace} preserved")
for key_format in ('"n%d"', '"u%d"', '"c%d"'):
    check(key_format in source, f"NVS key format {key_format} preserved")
check("MAX_STATIONS + MAX_RECENT + MAX_FAVORITES" in header,
      "all existing capacities form the one record block")

print(f"PASS {checks}/{checks} StationManager PSRAM policy checks")
