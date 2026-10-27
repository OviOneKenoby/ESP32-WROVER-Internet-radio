#pragma once
#include "rtcp_btx.h"

class BluetoothPlaybackAuthority {
public:
    void reset() { authoritative_ = false; playback_ = rtcp_btx::Playback::unknown; }
    void observeA2dp(rtcp_btx::Playback playback) {
        if (!authoritative_) playback_ = playback;
    }
    void observeAvrc(rtcp_btx::Playback playback, bool valid) {
        if (!valid) { reset(); return; }
        authoritative_ = true;
        playback_ = playback;
    }
    bool authoritative() const { return authoritative_; }
    rtcp_btx::Playback playback() const { return playback_; }
private:
    bool authoritative_{false};
    rtcp_btx::Playback playback_{rtcp_btx::Playback::unknown};
};
