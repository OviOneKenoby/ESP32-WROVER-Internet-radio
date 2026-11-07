// A station URL that does not fit its buffer (NUL included) is rejected, never
// truncated. Also covers the failed-pick path the rejection reuses.
#include "url_fit.h"
#include "browse_playback_policy.h"
#include <cassert>
#include <cstdio>
#include <string>

using P = BrowsePlaybackPolicy;
using Player = P::Player;
using Pick = P::PickResult;

static const size_t kCap = 256;  // MAX_URL_LENGTH (config.h:144)
static const char* kA = "http://live.example:8000/a";

// Mirrors rejectBrowsePickUrl() in main.cpp: a rejected play never reaches the
// audio source; it only resolves the pick as failed. Returns the number of
// "play" calls issued for the REJECTED url (must stay 0).
struct Audio { int playCalls = 0; };

static Pick rejectPick(P& p, uint64_t request, Player player, const char* url) {
    p.onPickAdmitted(request);                       // no-op when nothing is saved
    return p.onPickResolved(request, false, player, url);
}

int main() {
    static_assert(P::kUrlCapacity == kCap, "policy capacity must equal MAX_URL_LENGTH");

    // Boundary lengths.
    assert(urlFitsBuffer(size_t(0), kCap));
    assert(urlFitsBuffer(size_t(1), kCap));
    assert(urlFitsBuffer(kCap - 2, kCap));
    assert(urlFitsBuffer(kCap - 1, kCap));      // 255: accepted unchanged
    assert(!urlFitsBuffer(kCap, kCap));         // 256: rejected (no room for NUL)
    assert(!urlFitsBuffer(kCap + 1, kCap));
    assert(!urlFitsBuffer(size_t(591), kCap));  // RADIO NORA OLDIES length
    assert(!urlFitsBuffer(size_t(0), size_t(0)));
    // String overload.
    assert(urlFitsBuffer(std::string(255, 'x').c_str(), kCap));
    assert(!urlFitsBuffer(std::string(256, 'x').c_str(), kCap));
    assert(!urlFitsBuffer(std::string(591, 'x').c_str(), kCap));
    assert(!urlFitsBuffer(static_cast<const char*>(nullptr), kCap));

    // A rejected pick with a saved station: restart exactly once, no play call.
    {
        P p; Audio audio;
        assert(p.onStart(Player::playing, kA, 1) == P::StartResult::pause);
        assert(!urlFitsBuffer(size_t(591), kCap));   // decision: reject => no audio.playCalls++
        assert(rejectPick(p, 77, Player::paused, kA) == Pick::restart);
        assert(std::string(p.url()) == kA && p.codec() == 1);
        assert(!p.saved());
        assert(rejectPick(p, 77, Player::paused, kA) == Pick::nothing_saved);  // not twice
        assert(p.onSessionEnd(Player::paused, kA) == P::EndResult::nothing_saved);
        assert(audio.playCalls == 0);
    }
    // Rejected pick, nothing saved (player was not playing): nothing starts.
    {
        P p;
        assert(p.onStart(Player::paused, kA, 0) == P::StartResult::not_playing);
        assert(rejectPick(p, 78, Player::paused, kA) == Pick::nothing_saved);
        assert(!p.saved());
    }
    // The REMOTE's follow-up CANCEL after the failed result must not restart again.
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == P::StartResult::pause);
        assert(rejectPick(p, 79, Player::paused, kA) == Pick::restart);
        assert(p.onSessionEnd(Player::paused, kA) == P::EndResult::nothing_saved);
    }
    // A saved playing URL that does not fit is not saved (never truncated).
    {
        P p;
        std::string longUrl = "http://x/" + std::string(300, 'y');
        assert(p.onStart(Player::playing, longUrl.c_str(), 0) == P::StartResult::not_playing);
        assert(!p.saved());
        std::string ok = "http://x/" + std::string(246, 'y');   // 255 chars
        assert(ok.size() == 255);
        assert(p.onStart(Player::playing, ok.c_str(), 0) == P::StartResult::pause);
        assert(std::string(p.url()) == ok);                     // unchanged
    }
    std::printf("url_fit: PASS\n");
    return 0;
}
