#include "browse_playback_policy.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

using P = BrowsePlaybackPolicy;
using Player = P::Player;
using Start = P::StartResult;
using End = P::EndResult;

static const char* kA = "http://live.example:8000/a";
static const char* kB = "http://other.example/b";

int main() {
    // START while PLAYING: remember the station and ask for a pause.
    {
        P p;
        assert(!p.saved());
        assert(p.onStart(Player::playing, kA, 1) == Start::pause);
        assert(p.saved() && std::strcmp(p.url(), kA) == 0 && p.codec() == 1);
    }
    // START while PAUSED or other (stopped, buffering, Bluetooth, no source):
    // remember nothing, and a later end never starts playback.
    for (Player state : {Player::paused, Player::other}) {
        P p;
        assert(p.onStart(state, kA, 0) == Start::not_playing);
        assert(!p.saved());
        assert(p.onSessionEnd(Player::paused, kA) == End::nothing_saved);
        assert(p.onSessionEnd(Player::other, "") == End::nothing_saved);
    }
    // START without a usable stream URL while "playing": nothing to restart.
    {
        P p;
        assert(p.onStart(Player::playing, "", 0) == Start::not_playing);
        assert(p.onStart(Player::playing, nullptr, 0) == Start::not_playing);
        assert(!p.saved());
    }
    // Over-long URL is refused, not truncated.
    {
        P p;
        char longUrl[P::kUrlCapacity + 8];
        std::memset(longUrl, 'x', sizeof longUrl - 1);
        longUrl[sizeof longUrl - 1] = 0;
        assert(p.onStart(Player::playing, longUrl, 0) == Start::not_playing);
        assert(!p.saved());
    }
    // The three Browse jobs (START, letter, genre) never restore anything: only
    // session end and station choice do. A repeated START keeps the saved
    // station (the policy had paused playback, so the player now reads "paused").
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onStart(Player::paused, kA, 0) == Start::session_kept);
        assert(p.saved() && std::strcmp(p.url(), kA) == 0);
    }
    // Station chosen: saved state discarded, nothing restored afterwards.
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onStationChosen());
        assert(!p.saved());
        assert(p.onSessionEnd(Player::playing, kB) == End::nothing_saved);
        assert(!p.onStationChosen());  // nothing saved any more
    }
    // Cancel command or RTCP close with the player still as we left it: restart.
    for (int i = 0; i < 2; ++i) {
        P p;
        assert(p.onStart(Player::playing, kA, 1) == Start::pause);
        assert(p.onSessionEnd(Player::paused, kA) == End::restart);
        assert(!p.saved());  // consumed: a second close restarts nothing
        assert(std::strcmp(p.url(), kA) == 0 && p.codec() == 1);  // readable now
        assert(p.onSessionEnd(Player::paused, kA) == End::nothing_saved);
    }
    // Playback changed by someone else during the session: never override it.
    //   - resumed or a new station started (PLAYING)
    //   - physical navigation stopped it (other, empty URL)
    //   - paused again but on another stream
    {
        const struct { Player player; const char* url; } changed[] = {
            {Player::playing, kA}, {Player::playing, kB}, {Player::other, ""},
            {Player::paused, kB}, {Player::paused, ""}};
        for (const auto& c : changed) {
            P p;
            assert(p.onStart(Player::playing, kA, 0) == Start::pause);
            assert(p.onSessionEnd(c.player, c.url) == End::playback_changed);
            assert(!p.saved());
            assert(p.onSessionEnd(Player::paused, kA) == End::nothing_saved);
        }
    }
    // Stale saved state (physical navigation stopped playback, no end event
    // reached the policy): the next START sees the player changed, drops the
    // stale state and decides afresh.
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onStart(Player::other, "", 0) == Start::not_playing);
        assert(!p.saved());
        assert(p.onStart(Player::playing, kB, 2) == Start::pause);
        assert(std::strcmp(p.url(), kB) == 0 && p.codec() == 2);
    }
    // A new session after a finished one starts clean.
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onSessionEnd(Player::paused, kA) == End::restart);
        assert(p.onStart(Player::playing, kB, 1) == Start::pause);
        assert(std::strcmp(p.url(), kB) == 0);
    }
    std::printf("browse_playback_policy: PASS\n");
    return 0;
}
