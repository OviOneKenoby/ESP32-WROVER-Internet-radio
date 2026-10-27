#include "bluetooth_playback_authority.h"
#include <cassert>
#include <cstdio>

int main() {
    BluetoothPlaybackAuthority state;
    state.observeA2dp(rtcp_btx::Playback::playing);
    assert(!state.authoritative() && state.playback() == rtcp_btx::Playback::playing);
    state.observeAvrc(rtcp_btx::Playback::paused, true);
    assert(state.authoritative() && state.playback() == rtcp_btx::Playback::paused);
    state.observeA2dp(rtcp_btx::Playback::playing);
    assert(state.playback() == rtcp_btx::Playback::paused);
    state.observeAvrc(rtcp_btx::Playback::playing, true);
    assert(state.playback() == rtcp_btx::Playback::playing);
    state.observeAvrc(rtcp_btx::Playback::stopped, true);
    assert(state.playback() == rtcp_btx::Playback::stopped);
    state.reset();
    state.observeA2dp(rtcp_btx::Playback::paused);
    assert(!state.authoritative() && state.playback() == rtcp_btx::Playback::paused);
    state.observeAvrc(rtcp_btx::Playback::unknown, false);
    assert(!state.authoritative() && state.playback() == rtcp_btx::Playback::unknown);
    std::puts("Bluetooth playback authority: PASS");
}
