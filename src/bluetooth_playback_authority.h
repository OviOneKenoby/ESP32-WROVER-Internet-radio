#pragma once
#include "rtcp_btx.h"

class BluetoothPlaybackAuthority {
public:
    void reset() {
        authoritative_ = false;
        playback_ = rtcp_btx::Playback::unknown;
        a2dp_ = rtcp_btx::Playback::unknown;
    }
    void observeA2dp(rtcp_btx::Playback playback) {
        a2dp_ = playback;
        if (!authoritative_) playback_ = playback;
    }
    void observeAvrc(rtcp_btx::Playback playback, bool valid) {
        if (!valid) { reset(); return; }
        authoritative_ = true;
        // Several phones report AVRCP STOPPED for an ordinary Pause while
        // A2DP independently reports REMOTE_SUSPEND. Preserve the direct A2DP
        // pause observation instead of degrading the user-visible state.
        playback_ = playback == rtcp_btx::Playback::stopped &&
                    a2dp_ == rtcp_btx::Playback::paused
            ? rtcp_btx::Playback::paused : playback;
    }
    bool authoritative() const { return authoritative_; }
    rtcp_btx::Playback playback() const { return playback_; }
private:
    bool authoritative_{false};
    rtcp_btx::Playback playback_{rtcp_btx::Playback::unknown};
    rtcp_btx::Playback a2dp_{rtcp_btx::Playback::unknown};
};
