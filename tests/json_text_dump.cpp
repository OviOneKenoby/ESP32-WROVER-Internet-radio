// Helper for test_json_text_strict.py (not a test by itself). For a fixed corpus
// plus a fixed-seed random set it builds JSON with the PRODUCTION json_text
// quoting and the real ArduinoJson serializer, one document per line:
//   <hex of the input bytes> TAB <serialized JSON document>
#include "json_text.h"
#include <ArduinoJson.h>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

static void emit(const std::string& input) {
    std::string quoted;
    json_text::appendQuoted(quoted, input.data(), input.size());
    JsonDocument doc;
    doc["name"] = serialized(quoted);
    doc["url"] = serialized(quoted);
    doc["codec"] = "MP3";
    std::string json;
    serializeJson(doc, json);
    for (unsigned char c : input) std::printf("%02x", c);
    std::printf("\t%s\n", json.c_str());
}

int main() {
    std::vector<std::string> fixed = {
        "", "plain", std::string("\x01", 1), "a\nb", "a\rb", "a\tb", "a\x7f" "b", "q\"uote", "back\\slash",
        "</script><b>x</b>", std::string("nul\0inside", 10), "\x80", "\xc3", "\xe2\x82", "\xc0\xaf", "\xed\xa0\x80",
        "\xf4\x90\x80\x80", "caf\xc3\xa9", "\xe6\x97\xa5", "\xf0\x9f\x8e\xb5", "\x1f\x1e\x1d\x1c\x1b\x1a\x19\x18",
        "\x08\x0c\x0b",
    };
    for (const auto& s : fixed) emit(s);
    std::mt19937 rng(20261107);
    for (unsigned i = 0; i < 100000; ++i) {
        std::string s;
        const unsigned n = rng() % 120;
        for (unsigned k = 0; k < n; ++k) {
            switch (rng() % 8) {
                case 0: s += char(0x20 + rng() % 0x5f); break;
                case 1: s += char(rng() % 0x20); break;
                case 2: s += char(0x80 + rng() % 0x80); break;
                case 3: s += "\xc3\xa9"; break;
                case 4: s += "\xe6\x97\xa5"; break;
                case 5: s += "\xf0\x9f\x8e\xb5"; break;
                case 6: s += (rng() & 1) ? '"' : '\\'; break;
                default: s += char(rng() % 256); break;
            }
        }
        emit(s);
    }
    return 0;
}
