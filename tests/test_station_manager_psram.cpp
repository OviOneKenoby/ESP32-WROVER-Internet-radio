#include "../src/stations.h"

#include <Preferences.h>
#include <esp_heap_caps.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

TestSerial Serial;

namespace {
bool failAllocation = false;
unsigned allocations = 0;
unsigned releases = 0;
size_t allocatedBytes = 0;
uint32_t allocatedCaps = 0;
void* allocatedBase = nullptr;
unsigned checks = 0;
unsigned persistenceServices = 0;

void servicePersistence(const void*) { ++persistenceServices; }

void expect(bool value, const char* message) {
    ++checks;
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
}

extern "C" void* heap_caps_malloc(size_t size, uint32_t caps) {
    ++allocations;
    allocatedBytes = size;
    allocatedCaps = caps;
    if (failAllocation) return nullptr;
    allocatedBase = std::malloc(size);
    return allocatedBase;
}

extern "C" void* heap_caps_realloc(void*, size_t, uint32_t) { return nullptr; }

extern "C" void heap_caps_free(void* pointer) {
    expect(pointer == allocatedBase, "lifetime releases the single allocation");
    ++releases;
    std::free(pointer);
    allocatedBase = nullptr;
}

int main() {
    static_assert(sizeof(RadioStation) == 324, "record size baseline");

    Preferences::clear();
    {
        StationManager unavailable;
        failAllocation = true;
        const size_t writesBefore = Preferences::writes;
        expect(!unavailable.begin(), "allocation failure is reported");
        expect(!unavailable.isReady(), "failed storage remains unavailable");
        expect(allocatedBytes == 30 * sizeof(RadioStation), "one full record block requested");
        expect(allocatedCaps == (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
               "allocation requires byte-addressable PSRAM");
        expect(unavailable.addStation("blocked", "http://blocked") == 0xff,
               "uninitialized station mutation rejected");
        unavailable.addToRecent("blocked", "http://blocked", STATION_CODEC_MP3);
        expect(!unavailable.addToFavorites("blocked", "http://blocked", STATION_CODEC_MP3),
               "uninitialized favorite mutation rejected");
        expect(!unavailable.loadDefaultStations(), "uninitialized persistence load rejected");
        unavailable.loadPersistedLists();
        expect(Preferences::writes == writesBefore, "failure path never writes NVS");
    }

    failAllocation = false;
    const unsigned allocationsBefore = allocations;
    {
        StationManager manager;
        expect(manager.begin(), "PSRAM record storage initializes");
        expect(manager.begin(), "repeat begin is idempotent");
        expect(allocations == allocationsBefore + 1, "storage allocated exactly once");
        expect(manager.isReady(), "manager reports ready");
        manager.setPersistenceService(servicePersistence);

        for (unsigned i = 0; i < MAX_STATIONS; ++i) {
            char name[16]; char url[32];
            std::snprintf(name, sizeof name, "Station %u", i);
            std::snprintf(url, sizeof url, "http://station/%u", i);
            expect(manager.addStation(name, url, STATION_CODEC_MP3) == i,
                   "station added in order");
        }
        expect(manager.addStation("overflow", "http://overflow") == 0xff,
               "station capacity preserved");
        expect(manager.getStationCount() == MAX_STATIONS, "station count capacity exact");
        expect(manager.removeStation(1), "station removal succeeds");
        expect(std::strcmp(manager.getStationName(1), "Station 2") == 0,
               "station removal preserves ordering");

        for (unsigned i = 0; i < MAX_RECENT + 2; ++i) {
            char name[16]; char url[32];
            std::snprintf(name, sizeof name, "Recent %u", i);
            std::snprintf(url, sizeof url, "http://recent/%u", i);
            manager.addToRecent(name, url, STATION_CODEC_MP3);
        }
        expect(manager.getRecentCount() == MAX_RECENT, "recent capacity preserved");
        expect(persistenceServices > MAX_RECENT,
               "recent persistence yields repeatedly to the application owner");
        expect(std::strcmp(manager.getRecent(0)->name, "Recent 6") == 0,
               "recent list remains newest-first");
        manager.addToRecent("Recent moved", "http://recent/4", STATION_CODEC_AAC);
        expect(manager.getRecentCount() == MAX_RECENT &&
               std::strcmp(manager.getRecent(0)->name, "Recent moved") == 0,
               "duplicate recent moves to front without growing");

        for (unsigned i = 0; i < MAX_FAVORITES; ++i) {
            char name[16]; char url[32];
            std::snprintf(name, sizeof name, "Favorite %u", i);
            std::snprintf(url, sizeof url, "http://favorite/%u", i);
            expect(manager.addToFavorites(name, url, STATION_CODEC_MP3),
                   "favorite added");
        }
        expect(!manager.addToFavorites("overflow", "http://favorite/overflow", STATION_CODEC_MP3),
               "favorite capacity preserved");
        expect(!manager.addToFavorites("duplicate", "http://favorite/2", STATION_CODEC_MP3),
               "duplicate favorite rejected");
        expect(manager.removeFavorite(1), "favorite removal succeeds");
        expect(std::strcmp(manager.getFavorite(1)->name, "Favorite 2") == 0,
               "favorite removal preserves ordering");

        const auto base = reinterpret_cast<uintptr_t>(allocatedBase);
        expect(reinterpret_cast<uintptr_t>(manager.getStation(0)) == base,
               "saved stations start at allocation base");
        expect(reinterpret_cast<uintptr_t>(manager.getRecent(0)) ==
                   base + MAX_STATIONS * sizeof(RadioStation),
               "recent records follow stations in the same allocation");
        expect(reinterpret_cast<uintptr_t>(manager.getFavorite(0)) ==
                   base + (MAX_STATIONS + MAX_RECENT) * sizeof(RadioStation),
               "favorite records follow recent records in the same allocation");
    }
    expect(releases == 1, "successful storage released once at lifetime end");

    const unsigned allocationsBeforeReload = allocations;
    {
        StationManager reloaded;
        expect(reloaded.begin(), "second lifetime initializes");
        expect(allocations == allocationsBeforeReload + 1,
               "second manager owns one independent lifetime allocation");
        expect(reloaded.loadDefaultStations(), "saved station namespace loads");
        reloaded.loadPersistedLists();
        expect(reloaded.getStationCount() == MAX_STATIONS - 1,
               "saved station count retains compatible NVS layout");
        expect(std::strcmp(reloaded.getStationName(1), "Station 2") == 0,
               "saved station key layout remains compatible");
        expect(reloaded.getRecentCount() == MAX_RECENT &&
               std::strcmp(reloaded.getRecent(0)->name, "Recent moved") == 0,
               "recent persistence remains compatible");
        expect(reloaded.getFavoriteCount() == MAX_FAVORITES - 1 &&
               std::strcmp(reloaded.getFavorite(1)->name, "Favorite 2") == 0,
               "favorite persistence remains compatible");
    }
    expect(releases == 2, "each initialized manager releases its own block");

    std::cout << "PASS " << checks << "/" << checks
              << " StationManager PSRAM assertions\n";
}
