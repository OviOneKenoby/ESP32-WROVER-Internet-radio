#pragma once
// Decision logic for "pause playback while the REMOTE browses stations".
//
// Pure and RTOS-free so it can be host tested. The caller (main.cpp) performs
// the actions: it pauses when onStart() says so and restarts the saved station
// through the normal deferred play path when a result says "restart". Nothing
// here ever resumes a paused stream; a stream paused for a long time is dead
// (the server drops it) and resume() cannot reconnect it.
//
// A station picked in Browse does not discard the saved station immediately:
// the pick may fail to open (unreachable or dead station), and then the
// previous station must come back. The saved station is kept until the pick's
// deferred play resolves.
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
        restart,           // restart url()/codec() through the normal play path
        pick_pending       // a pick is in flight: its resolution decides
    };
    enum class PickResult : uint8_t {
        nothing_saved,
        station_chosen,    // the picked station plays: saved station discarded
        playback_changed,  // something else changed playback: never override it
        restart            // the pick did not play: restart url()/codec()
    };

    StartResult onStart(Player player, const char* url, uint8_t codec) {
        // A saved station is valid only while the player is still exactly as this
        // policy left it (paused on the same stream); anything else means another
        // party took over, so the old session state is stale.
        if (saved_ && !stillPausedOnSaved(player, url)) clearSaved();
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

    // A pick was admitted by the deferred play path (request id). The saved
    // station is kept until onPickResolved(). Returns true when a pick is now
    // being tracked (a station was saved).
    bool onPickAdmitted(uint64_t request) {
        if (!saved_) return false;
        pick_pending_ = true;
        pick_request_ = request;
        return true;
    }

    bool pickPending(uint64_t request) const {
        return pick_pending_ && request == pick_request_;
    }

    // The pick's deferred play finished. "succeeded" true: the new station plays
    // and the saved one is discarded. Otherwise (failed to open, or cancelled
    // before it played) the previous station is restarted if the player is still
    // paused on it.
    PickResult onPickResolved(uint64_t request, bool succeeded, Player player,
                              const char* url) {
        if (!pickPending(request)) return PickResult::nothing_saved;
        pick_pending_ = false;
        if (!saved_) return PickResult::nothing_saved;
        if (succeeded) {
            saved_ = false;
            return PickResult::station_chosen;
        }
        const bool unchanged = stillPausedOnSaved(player, url);
        saved_ = false;
        return unchanged ? PickResult::restart : PickResult::playback_changed;
    }

    // Cancel command or RTCP close. After a "restart" result url() and codec()
    // stay readable until the next onStart(). While a pick is pending nothing is
    // queued here (the deferred play slot is busy); the pick resolution decides.
    EndResult onSessionEnd(Player player, const char* url) {
        if (!saved_) return EndResult::nothing_saved;
        if (pick_pending_) return EndResult::pick_pending;
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
    void clearSaved() {
        saved_ = false;
        pick_pending_ = false;
    }

    char url_[kUrlCapacity]{};
    uint64_t pick_request_ = 0;
    uint8_t codec_ = 0;
    bool saved_ = false;
    bool pick_pending_ = false;
};
