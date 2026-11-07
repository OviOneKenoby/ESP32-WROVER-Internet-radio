// icy_metadata::consume must, for ANY metadata block, consume exactly the declared
// number of bytes, call the title callback at most once with a NUL-terminated
// string that fits the working buffer, and return the expected title for ordinary
// blocks. (Memory-safety under AddressSanitizer is checked separately with the
// harness described in the TASK 044 evidence; this test runs on the plain host
// compiler.)
#include "../src/icy_metadata.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed) : s(seed) {}
    uint32_t below(uint32_t n) { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return uint32_t(s % n); }
};

struct Stream {
    std::vector<uint8_t> data;
    size_t at = 0;
    Rng* rng = nullptr;
    int read(uint8_t* dst, size_t n) {
        size_t left = data.size() - at;
        size_t give = n < left ? n : left;
        if (rng && give) {
            if (rng->below(8) == 0) return 0;
            give = 1 + rng->below(uint32_t(give));
        }
        std::memcpy(dst, data.data() + at, give);
        at += give;
        return int(give);
    }
};

struct Result { int calls = 0; std::string title; bool ok = false; size_t consumed = 0; };

Result run(Stream& s, int size) {
    Result r;
    const size_t start = s.at;
    r.ok = icy_metadata::consume(s, size,
        [&](const char* t) {
            ++r.calls;
            size_t n = 0;
            while (n < 400 && t[n]) ++n;
            assert(n < 273);  // NUL found inside the working buffer
            r.title.assign(t, n);
        },
        [] {});
    r.consumed = s.at - start;
    return r;
}

std::string pad16(std::string b) { while (b.size() % 16) b += '\0'; return b; }

std::vector<uint8_t> withTail(const std::string& b) {
    std::vector<uint8_t> d(b.begin(), b.end());
    for (int i = 0; i < 8; ++i) d.push_back(0xEE);
    return d;
}

unsigned checks = 0;
#define EXPECT(c) do { ++checks; if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); return 1; } } while (0)

}  // namespace

int main() {
    // Ordinary blocks give the exact title, whole or split reads.
    const char* titles[] = {"Artist - Song", "", "A", "Caf\xc3\xa9 - \xe6\x97\xa5\xe6\x9c\xac", "x y  z", "Don't Stop"};
    for (const char* title : titles) {
        const std::string forms[] = {
            std::string("StreamTitle='") + title + "';",
            std::string("StreamTitle='") + title + "';StreamUrl='';",
            std::string("StreamTitle=\"") + title + "\";",
            std::string("a=1;StreamTitle='") + title + "';",
        };
        for (const std::string& f : forms) {
            const std::string blk = pad16(f);
            for (int split = 0; split < 20; ++split) {
                Stream s; s.data = withTail(blk);
                Rng rng(uint64_t(split) + 5);
                if (split) s.rng = &rng;
                Result r = run(s, int(blk.size()));
                EXPECT(r.ok && r.calls == 1 && r.consumed == blk.size() && s.data[s.at] == 0xEE);
                if (std::strchr(title, '"') == nullptr) EXPECT(r.title == title);
            }
        }
    }

    // Systematic shapes at the boundary sizes: always consumed, at most one callback.
    const int sizes[] = {1, 15, 16, 240, 255, 256, 271, 272, 273, 274, 500, 1000, 4080};
    for (int size : sizes) {
        const std::string shapes[] = {
            "StreamTitle='" + std::string(size_t(size), 'A'),
            "StreamTitle=" + std::string(size_t(size), 'B'),
            "StreamTitle=" + std::string(size_t(size), ';'),
            std::string(size_t(size), 'z'),
            std::string(size_t(size), '\0'),
            "StreamTitle='ab" + std::string(1, '\0') + "cd';" + std::string(size_t(size), 'D'),
            "StreamTitle='" + std::string(size_t(size), '\xc3'),
        };
        for (const std::string& shape : shapes) {
            std::string blk = shape;
            blk.resize(size_t(size), 'q');
            for (int mode = 0; mode < 2; ++mode) {
                Stream s; s.data = withTail(blk);
                Rng rng(uint64_t(size) * 3 + 1);
                if (mode) s.rng = &rng;
                Result r = run(s, size);
                EXPECT(r.ok && r.consumed == size_t(size) && s.data[s.at] == 0xEE && r.calls <= 1);
            }
        }
    }

    // Random blocks, fixed seed.
    Rng rng(20261107);
    for (int i = 0; i < 100000; ++i) {
        const int size = 1 + int(rng.below(4080));
        std::string blk(size_t(size), '\0');
        const uint32_t kind = rng.below(4);
        for (char& c : blk) {
            c = kind == 0 ? char(rng.below(256)) : kind == 1 ? ";'\"A\0=S"[rng.below(7)] : kind == 2 ? char('a' + rng.below(26)) : (rng.below(4) ? 'x' : '\0');
        }
        if (rng.below(2) && size >= 13) std::memcpy(&blk[rng.below(uint32_t(size - 12))], "StreamTitle=", 12);
        Stream s; s.data = withTail(blk);
        Rng sr(rng.below(1000000) + 1);
        if (rng.below(2)) s.rng = &sr;
        Result r = run(s, size);
        EXPECT(r.ok && r.consumed == size_t(size) && s.data[s.at] == 0xEE && r.calls <= 1);
    }

    // Two blocks back to back keep the stream aligned; a stream error is reported.
    {
        const std::string b1 = pad16("StreamTitle='One';"), b2 = pad16("StreamTitle='Two';");
        Stream s; s.data = std::vector<uint8_t>();
        for (char c : b1 + "AB" + b2 + "CD") s.data.push_back(uint8_t(c));
        Result r1 = run(s, int(b1.size())); s.at += 2;
        Result r2 = run(s, int(b2.size())); s.at += 2;
        EXPECT(r1.title == "One" && r2.title == "Two" && s.at == s.data.size());
    }
    {
        struct Failing { int n = 0; int read(uint8_t*, size_t) { return ++n == 1 ? 40 : -1; } } f;
        int calls = 0;
        const bool ok = icy_metadata::consume(f, 200, [&](const char*) { ++calls; }, [] {});
        EXPECT(!ok);
    }
    std::printf("icy metadata reader: %u checks passed\n", checks);
    return 0;
}
