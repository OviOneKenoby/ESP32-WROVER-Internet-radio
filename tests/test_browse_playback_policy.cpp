#include "browse_playback_policy.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

using P = BrowsePlaybackPolicy;
using Player = P::Player;
using Start = P::StartResult;
using End = P::EndResult;
using Pick = P::PickResult;

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
    // remember nothing, and nothing is ever started afterwards.
    for (Player state : {Player::paused, Player::other}) {
        P p;
        assert(p.onStart(state, kA, 0) == Start::not_playing);
        assert(!p.saved());
        assert(!p.onPickAdmitted(5));  // nothing saved: no pick is tracked
        assert(p.onPickResolved(5, false, Player::paused, kA) == Pick::nothing_saved);
        assert(p.onSessionEnd(Player::paused, kA) == End::nothing_saved);
        assert(p.onSessionEnd(Player::other, "") == End::nothing_saved);
    }
    // START without a usable stream URL while "playing", and an over-long URL.
    {
        P p;
        assert(p.onStart(Player::playing, "", 0) == Start::not_playing);
        assert(p.onStart(Player::playing, nullptr, 0) == Start::not_playing);
        char longUrl[P::kUrlCapacity + 8];
        std::memset(longUrl, 'x', sizeof longUrl - 1);
        longUrl[sizeof longUrl - 1] = 0;
        assert(p.onStart(Player::playing, longUrl, 0) == Start::not_playing);
        assert(!p.saved());
    }
    // The Browse jobs never restore anything; a repeated START inside the session
    // keeps the saved station (the player now reads "paused").
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onStart(Player::paused, kA, 0) == Start::session_kept);
        assert(p.saved() && std::strcmp(p.url(), kA) == 0);
    }
    // PICK SUCCEEDED: the saved station is kept while the pick is pending and is
    // discarded only when the pick plays. Nothing is restarted.
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onPickAdmitted(7));
        assert(p.saved() && p.pickPending(7));
        assert(p.onPickResolved(7, true, Player::playing, kB) == Pick::station_chosen);
        assert(!p.saved() && !p.pickPending(7));
        assert(p.onSessionEnd(Player::playing, kB) == End::nothing_saved);
    }
    // PICK FAILED (the station cannot open): the player is still paused on the
    // saved stream, so the previous station is restarted - exactly once.
    for (int cancelled = 0; cancelled < 2; ++cancelled) {  // failed and cancelled behave alike
        P p;
        assert(p.onStart(Player::playing, kA, 1) == Start::pause);
        assert(p.onPickAdmitted(9));
        assert(p.onPickResolved(9, false, Player::paused, kA) == Pick::restart);
        assert(std::strcmp(p.url(), kA) == 0 && p.codec() == 1);  // readable now
        assert(!p.saved());
        assert(p.onPickResolved(9, false, Player::paused, kA) == Pick::nothing_saved);
        assert(p.onSessionEnd(Player::paused, kA) == End::nothing_saved);
    }
    // PICK FAILED but someone changed playback meanwhile: never override it.
    {
        const struct { Player player; const char* url; } changed[] = {
            {Player::playing, kA},   // resumed
            {Player::playing, kB},   // another station started
            {Player::other, ""},     // physical stop
            {Player::paused, kB}};   // paused on a different stream
        for (const auto& c : changed) {
            P p;
            assert(p.onStart(Player::playing, kA, 0) == Start::pause);
            assert(p.onPickAdmitted(3));
            assert(p.onPickResolved(3, false, c.player, c.url) == Pick::playback_changed);
            assert(!p.saved());
        }
    }
    // SESSION ENDS WHILE THE PICK IS PENDING (cancel command or RTCP close): no
    // restart is queued; the saved station is kept and the pick decides.
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onPickAdmitted(11));
        assert(p.onSessionEnd(Player::paused, kA) == End::pick_pending);
        assert(p.saved());
        assert(p.onSessionEnd(Player::paused, kA) == End::pick_pending);  // repeated close
        assert(p.onPickResolved(11, false, Player::paused, kA) == Pick::restart);
        assert(!p.saved());
    }
    {
        P p;  // same, but the pick plays
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onPickAdmitted(12));
        assert(p.onSessionEnd(Player::paused, kA) == End::pick_pending);
        assert(p.onPickResolved(12, true, Player::playing, kB) == Pick::station_chosen);
        assert(!p.saved());
    }
    // A DIFFERENT request resolving (a REMOTE play that was rejected or finished
    // elsewhere) must not touch the tracked pick.
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onPickAdmitted(20));
        assert(p.onPickResolved(21, true, Player::playing, kB) == Pick::nothing_saved);
        assert(p.saved() && p.pickPending(20));
        assert(p.onPickResolved(20, false, Player::paused, kA) == Pick::restart);
    }
    // START while a pick is pending keeps the saved station and does not pause again.
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onPickAdmitted(30));
        assert(p.onStart(Player::paused, kA, 0) == Start::session_kept);
        assert(p.pickPending(30));
    }
    // Cancel/close with the player still as we left it and NO pick: restart.
    for (int i = 0; i < 2; ++i) {
        P p;
        assert(p.onStart(Player::playing, kA, 1) == Start::pause);
        assert(p.onSessionEnd(Player::paused, kA) == End::restart);
        assert(!p.saved());
        assert(std::strcmp(p.url(), kA) == 0 && p.codec() == 1);
        assert(p.onSessionEnd(Player::paused, kA) == End::nothing_saved);
    }
    // Playback changed by someone else during a pick-less session: never override.
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
    // reached the policy, a pick was pending): the next START drops it.
    {
        P p;
        assert(p.onStart(Player::playing, kA, 0) == Start::pause);
        assert(p.onPickAdmitted(40));
        assert(p.onStart(Player::other, "", 0) == Start::not_playing);
        assert(!p.saved() && !p.pickPending(40));
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
