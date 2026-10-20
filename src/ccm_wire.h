#pragma once
#include <cstddef>
#include <cstdint>

namespace ccm_wire {
enum class Kind : uint8_t { identity_request=1, identity_response=2, capability_request=3,
    capability_response=4, snapshot_request=5, snapshot_response=6,
    volume_set=7, command_result=8, playback_command=9,
    now_playing_request=10, now_playing_response=11 };
struct VolumeSet { uint64_t request=0; uint32_t numerator=0, denominator=0; };
enum class PlaybackOperation : uint8_t { play=1, pause=2, previous=3, next=4 };
struct PlaybackCommand {
    uint64_t request=0;
    uint8_t operation=0;
    uint8_t expected_state=0;
    uint64_t media_generation=0;
};
struct NowPlaying {
    uint8_t flags=0;
    uint8_t context=0;
    const char* station=nullptr;
    uint8_t station_size=0;
    const char* artist=nullptr;
    uint8_t artist_size=0;
    const char* title=nullptr;
    uint8_t title_size=0;
    const char* combined=nullptr;
    uint16_t combined_size=0;
    uint64_t sequence=0;
    uint64_t media_generation=0;
    uint64_t metadata_revision=0;
};
bool parseVolumeSet(const uint8_t*, size_t, VolumeSet&);
bool volumePercent(const VolumeSet&, uint8_t&);
bool parsePlaybackCommand(const uint8_t*, size_t, PlaybackCommand&);
size_t identity(const char* firmware, uint8_t*, size_t);
size_t capabilities(uint8_t*, size_t);
size_t snapshot(uint8_t volume, uint8_t playback, uint64_t sequence,
                uint64_t media_generation, uint8_t*, size_t);
size_t result(uint64_t request, bool accepted, uint8_t error, uint8_t*, size_t);
size_t nowPlaying(const NowPlaying&, uint8_t*, size_t);
}
