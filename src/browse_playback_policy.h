#pragma once
// Decision logic for "pause playback while the REMOTE browses stations".
//
// Pure and RTOS-free so it can be host tested. The caller (main.cpp) performs
// the actions: it pauses when onStart() says so and restarts the saved station
// through the normal deferred play path when onSessionEnd() says so. Nothing
// here ever resumes a paused stream; a stream paused for a long time is dead
// (the server drops it) and resume() cannot reconnect it.
#include <cstddef>
#include <cstdint>
#include <cstring>

class BrowsePlaybackPolicy {
public:
    static constexpr size_t kUrlCapacity = 256;  // == MAX_URL_LENGTH in config.h

    // Player as seen by the policy. "other" covers stopped, buffering, no source
    // and every non-radio source (Bluetooth).
    enum class Player : uint8_t { playing, paused, other };
    enum class StartResult : uint8_t {
        pause,         // station remembered: the caller must pause now
        session_kept,  // START inside a session that already paused playback
        not_playing    // nothing remembered, nothing to change
    };
    enum class EndResult : uint8_t {
        nothing_saved,
        playback_changed,  // something else changed playback: never override it
        restart            // restart url()/codec() through the normal play path
    };

    StartResult onStart(Player player, const char* url, uint8_t codec) {
        // A saved station is valid only while the player is still exactly as this
        // policy left it (paused on the same stream); anything else means another
        // party took over, so the old session state is stale.
        if (saved_ && !stillPausedOnSaved(player, url)) saved_ = false;
        if (saved_) return StartResult::session_kept;
        if (player == Player::playing && url && url[0] &&
            std::strlen(url) < kUrlCapacity) {
            std::strcpy(url_, url);
            codec_ = codec;
            saved_ = true;
            return StartResult::pause;
        }
        return StartResult::not_playing;
    }

    // The user picked a station from Browse: the new station plays as usual and
    // the saved one is discarded. Returns true when a station had been saved.
    bool onStationChosen() {
        const bool had = saved_;
        saved_ = false;
        return had;
    }

    // Cancel command or RTCP close. After a "restart" result url() and codec()
    // stay readable until the next onStart().
    EndResult onSessionEnd(Player player, const char* url) {
        if (!saved_) return EndResult::nothing_saved;
        const bool unchanged = stillPausedOnSaved(player, url);
        saved_ = false;
        return unchanged ? EndResult::restart : EndResult::playback_changed;
    }

    bool saved() const { return saved_; }
    const char* url() const { return url_; }
    uint8_t codec() const { return codec_; }

private:
    bool stillPausedOnSaved(Player player, const char* url) const {
        return player == Player::paused && url && std::strcmp(url, url_) == 0;
    }

    char url_[kUrlCapacity]{};
    uint8_t codec_ = 0;
    bool saved_ = false;
};
