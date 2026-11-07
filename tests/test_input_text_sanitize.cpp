// Externally supplied text (station names, stream titles, stored records, URLs)
// must never make an RTCP encoder refuse a whole response, and must never be
// lost silently. This test drives the PRODUCTION encoders (catalog page, main
// station menu, now-playing projection), the production StationManager with a
// host Preferences stub (add / load / favorites / recent) and the URL rule.
#include "../src/browse_text.h"
#include "../src/ccm_wire.h"
#include "../src/correctness_guards.h"
#include "../src/rtcp_catalog.h"
#include "../src/rtcp_d009.h"
#include "../src/rtcp_full_control.h"
#include "../src/station_text.h"
#include "../src/stations.h"

#include <Preferences.h>
#include <esp_heap_caps.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

TestSerial Serial;

extern "C" void* heap_caps_malloc(size_t size, uint32_t) { return std::malloc(size); }
extern "C" void* heap_caps_realloc(void*, size_t, uint32_t) { return nullptr; }
extern "C" void heap_caps_free(void* pointer) { std::free(pointer); }

namespace {

unsigned checks = 0;
void expect(bool value, const char* message) {
    ++checks;
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

// Independent UTF-8 check (not the production one): valid, no control character.
bool printableUtf8(const std::string& text) {
    for (size_t at = 0; at < text.size();) {
        const uint8_t c = uint8_t(text[at]);
        size_t width = 1;
        uint32_t cp = c;
        if (c < 0x80) {
            if (c < 0x20 || c == 0x7f) return false;
        } else if (c >= 0xc2 && c <= 0xdf) { width = 2; cp = c & 0x1f; }
        else if (c >= 0xe0 && c <= 0xef) { width = 3; cp = c & 0x0f; }
        else if (c >= 0xf0 && c <= 0xf4) { width = 4; cp = c & 0x07; }
        else return false;
        if (at + width > text.size()) return false;
        for (size_t i = 1; i < width; ++i) {
            const uint8_t n = uint8_t(text[at + i]);
            if ((n & 0xc0) != 0x80) return false;
            cp = (cp << 6) | (n & 0x3f);
        }
        if ((width == 2 && cp < 0x80) || (width == 3 && cp < 0x800) ||
            (width == 4 && cp < 0x10000) || (cp >= 0xd800 && cp <= 0xdfff) ||
            cp > 0x10ffff) return false;
        at += width;
    }
    return true;
}

struct Outcome {
    bool produced = false;  // the encoder returned a non-empty response
    std::string text;       // decoded text of the single item / field
};

Outcome viaCatalog(const std::string& name) {
    rtcp_catalog::PageView view{};
    view.collection = rtcp_catalog::Collection::saved;
    view.capacity = 15;
    view.count = 1;
    view.start = 0;
    view.item_count = 1;
    view.revision = 1;
    view.items[0] = {0, false, name.c_str()};
    uint8_t out[512];
    const size_t n = rtcp_catalog::page(view, 1, out, sizeof out);
    Outcome o;
    o.produced = n >= 32;
    if (o.produced) o.text.assign(reinterpret_cast<char*>(out + 32), out[31]);
    return o;
}

Outcome viaMenu(const std::string& name) {
    rtcp_full_control::MenuView view{};
    view.mode = rtcp_full_control::MenuMode::station_select;
    view.count = 1;
    view.window_count = 1;
    view.items[0] = {0, false, name.c_str()};
    uint8_t out[512];
    const size_t n = rtcp_full_control::menuState(view, 1, 1, out, sizeof out);
    Outcome o;
    o.produced = n >= 32;
    if (o.produced) o.text.assign(reinterpret_cast<char*>(out + 32), out[31]);
    return o;
}

struct Playing {
    bool station_ok = false, title_ok = false, truncated = false;
    std::string station, title;
};

Playing viaNowPlaying(const std::string& station, const std::string& title) {
    rtcp_d009::Projection projection;
    rtcp_d009::Input input;
    input.context = rtcp_d009::Context::fixed_radio;
    input.playback_state = 0;
    input.station = station.c_str();
    input.combined = title.c_str();
    input.source_identity = 1;
    projection.publish(input);
    const auto wire = projection.wire(1);
    Playing p;
    p.station_ok = wire.flags & rtcp_d009::station_valid;
    p.title_ok = wire.flags & rtcp_d009::metadata_valid;
    p.truncated = wire.flags & rtcp_d009::truncated;
    p.station.assign(wire.station, wire.station_size);
    p.title.assign(wire.combined, wire.combined_size);
    return p;
}

std::string repeat(const std::string& unit, size_t times) {
    std::string out;
    for (size_t i = 0; i < times; ++i) out += unit;
    return out;
}

std::vector<std::string> corpus() {
    std::vector<std::string> c = {
        "Radio Test", std::string("\x01Radio\x01"), "Line\nBreak", "Car\rReturn",
        "Tab\tSeparated", std::string("Del\x7f" "Char"), "\x80Lonely continuation",
        "Truncated \xc3", "Truncated three \xe2\x82", "Overlong \xc0\xaf slash",
        "Overlong \xe0\x80\xaf", "Surrogate \xed\xa0\x80", "Beyond \xf4\x90\x80\x80",
        "Bad lead \xff\xfe", "", "   ", "\n\n\n", "say \"hi\" \\ back</script>",
        "<b>bold</b> & more", std::string("nul\0inside", 10),
        repeat("a", 47) + "\xc3\xa9",             // 2-byte char ends exactly at 49 (cut at 48)
        repeat("a", 48) + "\xc3\xa9",
        repeat("a", 46) + "\xe2\x82\xac",        // 3-byte char straddles 48
        repeat("a", 45) + "\xf0\x9f\x8e\xb5",    // 4-byte char straddles 48
        repeat("a", 48), repeat("a", 49), repeat("a", 63), repeat("a", 64),
        repeat("a", 96), repeat("a", 480), repeat("a", 640),
        repeat("\xc3\xa9", 40), repeat("\xe6\x97\xa5", 30), repeat("\xf0\x9f\x8e\xb5", 20),
        repeat("a", 253) + "\xc3\xa9",            // 2-byte char straddles 255
        repeat("a", 254) + "\xe2\x82\xac",
        repeat("a", 252) + "\xf0\x9f\x8e\xb5",
    };
    return c;
}

std::string randomText(std::mt19937& rng) {
    std::string out;
    const unsigned length = rng() % 300;
    for (unsigned i = 0; i < length; ++i) {
        switch (rng() % 7) {
            case 0: out += char(0x20 + rng() % 0x5f); break;
            case 1: out += char(rng() % 0x20); break;
            case 2: out += char(0x80 + rng() % 0x80); break;
            case 3: out += "\xc3\xa9"; break;
            case 4: out += "\xe6\x97\xa5"; break;
            case 5: out += "\xf0\x9f\x8e\xb5"; break;
            default: out += ' '; break;
        }
    }
    return out;
}

void checkEncodedName(const std::string& input, const Outcome& o, size_t cap, const char* who) {
    (void)input;
    expect(o.produced, who);                  // never a zero-size response
    expect(!o.text.empty(), who);
    expect(o.text.size() <= cap, who);
    expect(printableUtf8(o.text), who);
}

void corpusEncoders() {
    for (const std::string& name : corpus()) {
        checkEncodedName(name, viaCatalog(name), 48, "catalog accepts the corpus");
        checkEncodedName(name, viaMenu(name), 48, "menu accepts the corpus");
        const Playing p = viaNowPlaying(name, name);
        expect(p.station.size() <= 63 && p.title.size() <= 255, "now-playing sizes bounded");
        expect(printableUtf8(p.station) && printableUtf8(p.title), "now-playing text is clean");
        // A string that is only blanks/controls is absent, never an error state.
        if (p.station_ok) expect(!p.station.empty(), "valid station flag implies text");
        if (p.title_ok) expect(!p.title.empty(), "valid title flag implies text");
    }
}

void cleanNamesAreIdentical() {
    const std::vector<std::string> clean = {
        "West City Radio", "Radio Rom\xc3\xa2nia Actualit\xc4\x83\xc8\x9bi",
        "\xe6\x97\xa5\xe6\x9c\xac\xe6\x94\xbe\xe9\x80\x81",
        "\xf0\x9f\x8e\xb5 Chill \xf0\x9f\x8e\xb6", "DIGI FM", "Kiss FM (Bucure\xc8\x99ti)",
        repeat("a", 48), repeat("\xc3\xa9", 24),
    };
    for (const std::string& name : clean) {
        expect(viaCatalog(name).text == name, "catalog: clean name byte-identical");
        expect(viaMenu(name).text == name, "menu: clean name byte-identical");
    }
    const std::string title = "Artist - Song \xc4\x83 (live) \xf0\x9f\x8e\xb5";
    const Playing p = viaNowPlaying("DIGI FM", title);
    expect(p.station == "DIGI FM" && p.title == title, "now-playing: clean text byte-identical");
    expect(p.station_ok && p.title_ok && !p.truncated, "now-playing flags for clean text");
}

void expectedBehaviour() {
    // A 2-byte character straddling the 48-byte limit: cut before it, not dropped.
    const std::string s = repeat("a", 47) + "\xc3\xa9";
    expect(viaCatalog(s).text == repeat("a", 47), "catalog cut at character boundary");
    expect(viaMenu(s).text == repeat("a", 47), "menu cut at character boundary");
    // Empty / blank names get the placeholder and still encode.
    expect(viaCatalog("").text == "(no name)", "catalog placeholder for empty name");
    expect(viaMenu("  \n ").text == "(no name)", "menu placeholder for blank name");
    expect(viaCatalog("A\nB").text == "A B", "control character becomes a space");
    expect(viaCatalog("A\xff" "B").text == "A?B", "invalid byte becomes '?'");
    // A title cut by a fixed copy in the middle of a character is no longer lost.
    std::string title = repeat("a", 254) + "\xc3\xa9";
    char stored[256];
    bool cut = false;
    browse_text::sanitize(title.c_str(), title.size(), stored, sizeof stored, nullptr, &cut);
    expect(std::string(stored) == repeat("a", 254) && cut, "stored title cut at a character boundary");
    const Playing p = viaNowPlaying("S", std::string(stored));
    expect(p.title_ok && p.title == repeat("a", 254), "long title is kept, not dropped");
    // Control characters / invalid UTF-8 in a title: cleaned text is kept.
    const Playing q = viaNowPlaying("S", "Song\x01Name\xff");
    expect(q.title_ok && q.title == "Song Name?", "dirty title is cleaned, not dropped");
    // Only blanks: no metadata, no error.
    const Playing r = viaNowPlaying("S", " \x01 ");
    expect(!r.title_ok && r.title.empty(), "blank title is absent");
}

void propertyTests() {
    std::mt19937 rng(20261107);
    for (unsigned i = 0; i < 100000; ++i) {
        const std::string text = randomText(rng);
        const Outcome c = viaCatalog(text);
        expect(c.produced && !c.text.empty() && c.text.size() <= 48 && printableUtf8(c.text),
               "property: catalog");
    }
    for (unsigned i = 0; i < 100000; ++i) {
        const std::string text = randomText(rng);
        const Outcome m = viaMenu(text);
        expect(m.produced && !m.text.empty() && m.text.size() <= 48 && printableUtf8(m.text),
               "property: menu");
    }
    for (unsigned i = 0; i < 100000; ++i) {
        const std::string text = randomText(rng);
        const Playing p = viaNowPlaying(text, text);
        expect(p.station.size() <= 63 && p.title.size() <= 255 &&
               printableUtf8(p.station) && printableUtf8(p.title) &&
               (p.station_ok == !p.station.empty()) && (p.title_ok == !p.title.empty()),
               "property: now-playing");
    }
}

void urlRule() {
    using station_text::urlAcceptable;
    expect(urlAcceptable("http://188.214.156.114:8000/mp3"), "plain URL accepted");
    expect(urlAcceptable("https://live.example.org/a-b_c.mp3?x=1&y=%20z"), "encoded URL accepted");
    const std::vector<std::string> bad = {
        "http://x/a b", "http://x/a\tb", "http://x/a\nb", "http://x/a\rb",
        "http://x/a\x7f" "b", "http://x/\xc3\xa9", "http://x/\x80", std::string("http://x/a\0b", 12),
        "http://x/a\x01", " http://x/", "http://x/ ", "",
    };
    for (const std::string& url : bad)
        expect(!urlAcceptable(url.data(), url.size()), "bad URL rejected");
    expect(!urlAcceptable(nullptr, 0) && !urlAcceptable(nullptr), "null URL rejected");

    // Through the production StationManager: a rejected URL never reaches the store.
    Preferences::clear();
    StationManager manager;
    expect(manager.begin(), "record storage");
    const size_t writes = Preferences::writes;
    for (const std::string& url : bad) {
        if (url.empty()) continue;
        std::string visible = url.substr(0, url.find('\0'));
        if (!urlAcceptable(visible.c_str()))
            expect(manager.addStation("Name", visible.c_str()) == 0xFF, "addStation never stores a bad URL");
    }
    expect(manager.addStation("Name", "http://x/a b") == 0xFF, "space in URL refused");
    expect(manager.addStation("Name", "http://x/a\nb") == 0xFF, "LF in URL refused");
    expect(manager.addStation("Name", "http://x/\xc3\xa9") == 0xFF, "non-ASCII in URL refused");
    expect(manager.getStationCount() == 0 && Preferences::writes == writes,
           "refused URLs leave the list and NVS unchanged");
    expect(manager.addStation("Good", "http://x/ok") == 0 && manager.getStationCount() == 1,
           "valid URL unchanged");
    expect(std::string(manager.getStationURL(0)) == "http://x/ok", "URL stored byte-identical");
}

void ingressAndLoad() {
    Preferences::clear();
    StationManager manager;
    expect(manager.begin(), "record storage");

    // addStation / addToFavorites / addToRecent clean the name.
    expect(manager.addStation("A\nB\x01", "http://x/1") == 0, "added");
    expect(std::string(manager.getStationName(0)) == "A B", "addStation cleans the name");
    expect(manager.addToFavorites("F\xff" "av", "http://x/2", STATION_CODEC_MP3), "favorite added");
    expect(std::string(manager.getFavorite(0)->name) == "F?av", "addToFavorites cleans the name");
    manager.addToRecent("\n\n", "http://x/3", STATION_CODEC_MP3);
    expect(std::string(manager.getRecent(0)->name) == "(no name)", "addToRecent placeholder");
    const std::string longName = repeat("\xc3\xa9", 60);
    manager.addToRecent(longName.c_str(), "http://x/4", STATION_CODEC_MP3);
    expect(printableUtf8(manager.getRecent(0)->name) && strlen(manager.getRecent(0)->name) <= 63,
           "recent name cut at a character boundary");

    // NVS load: bad stored names are cleaned in memory and the catalog then encodes.
    Preferences::clear();
    Preferences::seed("stations", "count", "3");
    Preferences::seed("stations", "n0", "Bad\x01Name");
    Preferences::seed("stations", "u0", "http://x/s0");
    Preferences::seed("stations", "n1", "Cut \xc3");
    Preferences::seed("stations", "u1", "http://x/s1");
    Preferences::seed("stations", "n2", "");
    Preferences::seed("stations", "u2", "http://x/s2");
    Preferences::seed("recent", "count", "1");
    Preferences::seed("recent", "n0", "\nrecent\n");
    Preferences::seed("recent", "u0", "http://x/r0");
    Preferences::seed("favorites", "count", "1");
    Preferences::seed("favorites", "n0", "fav\xff");
    Preferences::seed("favorites", "u0", "http://x/f0");
    StationManager loaded;
    expect(loaded.begin(), "record storage");
    const size_t writes = Preferences::writes;
    expect(loaded.loadDefaultStations(), "saved stations load");
    loaded.loadPersistedLists();
    expect(Preferences::writes == writes, "loading writes nothing to NVS");
    expect(Preferences::data["stations"]["n0"] == "Bad\x01Name", "NVS value is left as stored");
    expect(std::string(loaded.getStationName(0)) == "Bad Name", "stored name cleaned in memory");
    expect(std::string(loaded.getStationName(1)) == "Cut ?", "stored broken name cleaned");
    expect(std::string(loaded.getStationName(2)) == "(no name)", "stored empty name gets the placeholder");
    expect(std::string(loaded.getRecent(0)->name) == "recent", "recent record cleaned");
    expect(std::string(loaded.getFavorite(0)->name) == "fav?", "favorite record cleaned");

    // The catalog encoder succeeds on the loaded lists (saved / favorites / recent).
    rtcp_catalog::PageView view{};
    view.collection = rtcp_catalog::Collection::saved;
    view.capacity = MAX_STATIONS;
    view.count = loaded.getStationCount();
    view.item_count = view.count;
    view.revision = 1;
    for (uint8_t i = 0; i < view.count; ++i) view.items[i] = {i, false, loaded.getStation(i)->name};
    uint8_t out[512];
    expect(rtcp_catalog::page(view, 1, out, sizeof out) > 28, "catalog page encodes the loaded saved list");
    // A raw record straight from the stub (never cleaned) also encodes.
    for (uint8_t i = 0; i < view.count; ++i) view.items[i] = {i, false, "Bad\x01Name"};
    expect(rtcp_catalog::page(view, 1, out, sizeof out) > 28, "catalog page encodes a raw bad name");
}

}  // namespace

int main() {
    corpusEncoders();
    cleanNamesAreIdentical();
    expectedBehaviour();
    propertyTests();
    urlRule();
    ingressAndLoad();
    std::cout << "input text sanitize checks passed: " << checks << '\n';
    return 0;
}
