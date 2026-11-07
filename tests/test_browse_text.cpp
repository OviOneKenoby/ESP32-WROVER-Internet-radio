// browse_text::sanitize must turn ANY name into one that the PRODUCTION RTCP
// browser encoder (rtcp_browser::state) accepts. The production validator is
// exercised through state() itself: it returns 0 when a name is rejected.
#include "browse_text.h"
#include "browse_text_fixtures.h"
#include "rtcp_browser.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

// True when the production encoder accepts `name` as the only item of a page.
static bool productionAccepts(const char* name) {
    rtcp_browser::StateView view{};
    view.stage = rtcp_browser::Stage::results;
    view.status = rtcp_browser::Status::ready;
    view.error = rtcp_browser::success;
    view.count = 1;
    view.start = 0;
    view.item_count = 1;
    view.generation = 1;
    view.revision = 1;
    view.items[0].token = 0;
    view.items[0].name = name;
    uint8_t out[256];
    return rtcp_browser::state(view, 1, out, sizeof out) != 0;
}

static std::string clean(const std::string& in, size_t cap, const char* placeholder = "(no name)") {
    std::vector<char> out(cap + 1, 'X');
    const size_t n = browse_text::sanitize(in.data(), in.size(), out.data(), cap, placeholder);
    assert(n < cap);
    assert(out[n] == 0);
    return std::string(out.data(), n);
}

static void checkInvariants(const std::string& out, size_t cap) {
    assert(out.size() <= cap - 1);
    assert(productionAccepts(out.c_str()));
    for (unsigned char c : out) assert(c >= 0x20 && c != 0x7f);
    assert(out.empty() == false);
    assert(out.front() != ' ' && out.back() != ' ');
    assert(out.find("  ") == std::string::npos);
}

int main() {
    // Sanity: the production validator rejects what it rejected on hardware.
    assert(!productionAccepts("\nRADIO MARIAM ARABIC\n"));
    assert(!productionAccepts("BEAUTIFUL INSTRUMENTALS\r"));
    assert(!productionAccepts(""));
    assert(productionAccepts("RADIO MARIAM ARABIC"));

    // a) real fixtures
    assert(clean("\nRADIO MARIAM ARABIC\n", 64) == "RADIO MARIAM ARABIC");
    assert(clean("BEAUTIFUL INSTRUMENTALS\r", 64) == "BEAUTIFUL INSTRUMENTALS");
    int controlNames = 0, longNames = 0;
    for (size_t i = 0; i < kBadNameCount; ++i) {
        const std::string in(reinterpret_cast<const char*>(kBadNames[i].bytes), kBadNames[i].size);
        // the raw string as copied before the fix (strlcpy into char[64]) is rejected
        std::string raw = in.substr(0, 63);
        if (productionAccepts(raw.c_str()) == false) ++controlNames;
        if (in.size() > 63) ++longNames;
        for (size_t cap : {size_t(64), size_t(40), size_t(32)}) {
            const std::string out = clean(in, cap);
            checkInvariants(out, cap);
        }
    }
    assert(kBadNameCount == 14 && controlNames == 14 && longNames == 5);

    // b) edge cases
    assert(clean("", 64) == "(no name)");
    assert(clean(std::string(5, char(1)) + char(0x7f) + char(31), 64) == "(no name)");
    assert(clean("     ", 64) == "(no name)");
    assert(clean(std::string("A") + char(0) + "B", 64) == "A B");  // NUL inside a counted string
    assert(clean("  a \t\n b  ", 64) == "a b");
    {   // exactly capacity-1 ASCII bytes pass unchanged, one more is cut
        std::string s63(63, 'a'), s64(64, 'a');
        assert(clean(s63, 64) == s63);
        assert(clean(s64, 64) == s63);
    }
    {   // 2-byte character straddling the boundary is dropped whole, not split
        std::string s = std::string(62, 'a') + "\xc4\x83";  // 62 + 2 = 64 bytes
        assert(clean(s, 64) == std::string(62, 'a'));
        std::string t = std::string(61, 'a') + "\xc4\x83";  // 63 bytes: fits
        assert(clean(t, 64) == t);
    }
    {   // 4-byte emoji at the boundary
        std::string s = std::string(61, 'a') + "\xf0\x9f\x8e\xb5";  // 65 bytes
        assert(clean(s, 64) == std::string(61, 'a'));
        std::string t = std::string(59, 'a') + "\xf0\x9f\x8e\xb5";  // 63 bytes
        assert(clean(t, 64) == t);
    }
    // invalid UTF-8: lone continuation, truncated sequence, overlong, bad lead
    assert(clean("a\x80z", 64) == "a?z");
    assert(clean("a\xe2\x82", 64) == "a??");            // truncated 3-byte at the end
    assert(clean("a\xc0\x80z", 64) == "a??z");          // overlong NUL
    assert(clean("a\xed\xa0\x80z", 64) == "a???z");     // surrogate
    assert(clean("a\xff z", 64) == "a? z");
    assert(clean("\xf4\x90\x80\x80", 64) == "????");    // > U+10FFFF
    assert(clean("\x80", 64) == "?");
    // placeholder cut to a tiny capacity
    assert(clean("", 5) == "(no");
    // maximum-length valid name of a multi-byte script (21 x 3 bytes = 63)
    {
        std::string cjk;
        for (int i = 0; i < 21; ++i) cjk += "\xe9\xa6\x99";
        assert(cjk.size() == 63 && clean(cjk, 64) == cjk);
        assert(clean(cjk + "\xe9\xa6\x99", 64) == cjk);
    }
    // capacity 0 writes nothing
    { char c = 'Z'; assert(browse_text::sanitize("abc", 3, &c, 0, "x") == 0 && c == 'Z'); }

    // d) clean names pass through byte-identical
    const char* cleanNames[] = {
        "West City Radio", "Kiss FM", "Digi FM", "Radio Rom\xc3\xa2nia Actualit\xc4\x83\xc8\x9bi",
        "\xe9\xa6\x99\xe6\xb8\xaf\xe7\x94\xb5\xe5\x8f\xb0\xe7\xac\xac\xe4\xb8\x80\xe5\x8f\xb0",
        "\xd0\xa0\xd0\xb0\xd0\xb4\xd0\xb8\xd0\xbe \xd0\xa0\xd0\xbe\xd1\x81\xd1\x81\xd0\xb8\xd1\x8f",
        "Radio \xf0\x9f\x8e\xb5 Hits", "La Sonora 88.1 Sogamoso", "ABF (HTTPS validation)",
        "Rock & Roll - 100.3 FM", "\xc3\x84mbient Sp\xc3\xa5" "ce"};
    for (const char* n : cleanNames) {
        assert(strlen(n) <= 63);
        assert(clean(n, 64) == n);
        assert(productionAccepts(n));
    }
    // idempotence
    for (size_t i = 0; i < kBadNameCount; ++i) {
        const std::string in(reinterpret_cast<const char*>(kBadNames[i].bytes), kBadNames[i].size);
        const std::string once = clean(in, 64);
        assert(clean(once, 64) == once);
    }

    // c) property test: fixed seed, 100,000 random byte strings
    std::mt19937 rng(20261107u);
    static const char* snippets[] = {"\xc3\xa9", "\xe9\xa6\x99", "\xf0\x9f\x8e\xb5", "\xd0\x96", "\xe2\x80\x94"};
    const size_t caps[] = {64, 40, 32, 5, 2};
    long accepted = 0;
    for (int iter = 0; iter < 100000; ++iter) {
        std::string s;
        const int len = int(rng() % 121);
        while (int(s.size()) < len) {
            const unsigned kind = rng() % 6;
            if (kind == 0) s += char(0x20 + rng() % 0x5f);          // printable ASCII
            else if (kind == 1) s += char(rng() % 0x20);            // control (incl. NUL)
            else if (kind == 2) s += snippets[rng() % 5];           // valid multi-byte
            else if (kind == 3) s += char(0x80 + rng() % 0x80);     // garbage high byte
            else if (kind == 4) s += ' ';
            else s += char(rng() % 256);                            // anything
        }
        const size_t cap = caps[iter % 5];
        const std::string out = clean(s, cap);
        checkInvariants(out, cap);
        ++accepted;
    }
    assert(accepted == 100000);

    std::printf("browse_text: PASS\n");
    return 0;
}
