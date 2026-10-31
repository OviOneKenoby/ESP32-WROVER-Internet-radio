#pragma once
#include "rtcp_btx.h"

class BluetoothPlaybackAuthority {
public:
    void reset() {
        authoritative_ = false;
        navigation_pending_ = false;
        playback_ = rtcp_btx::Playback::unknown;
        a2dp_ = rtcp_btx::Playback::unknown;
    }
    void beginNavigation() { navigation_pending_ = true; }
    void cancelNavigation() { navigation_pending_ = false; }
    void observeA2dp(rtcp_btx::Playback playback) {
        a2dp_ = playback;
        if (!authoritative_ || navigation_pending_) playback_ = playback;
        if (playback != rtcp_btx::Playback::playing)
            navigation_pending_ = false;
    }
    void observeAvrc(rtcp_btx::Playback playback, bool valid) {
        if (!valid) { reset(); return; }
        authoritative_ = true;
        // Some phones publish a transient AVRCP STOPPED while advancing a
        // track even though their A2DP stream remains active.  Scope the
        // exception to a locally accepted NEXT/PREV operation: direct A2DP
        // pause/stop, a conclusive AVRCP state, disconnect or reset all end it.
        if (navigation_pending_ &&
            playback == rtcp_btx::Playback::stopped &&
            a2dp_ == rtcp_btx::Playback::playing) {
            playback_ = rtcp_btx::Playback::playing;
            return;
        }
        navigation_pending_ = false;
        // Several phones report AVRCP STOPPED for an ordinary Pause while
        // A2DP independently reports REMOTE_SUSPEND. Preserve the direct A2DP
        // pause observation instead of degrading the user-visible state.
        playback_ = playback == rtcp_btx::Playback::stopped &&
                    a2dp_ == rtcp_btx::Playback::paused
            ? rtcp_btx::Playback::paused : playback;
    }
    bool authoritative() const { return authoritative_; }
    bool navigationPending() const { return navigation_pending_; }
    rtcp_btx::Playback playback() const { return playback_; }
private:
    bool authoritative_{false};
    bool navigation_pending_{false};
    rtcp_btx::Playback playback_{rtcp_btx::Playback::unknown};
    rtcp_btx::Playback a2dp_{rtcp_btx::Playback::unknown};
};
