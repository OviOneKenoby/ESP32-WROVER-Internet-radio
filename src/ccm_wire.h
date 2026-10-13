#pragma once
#include <cstddef>
#include <cstdint>

namespace ccm_wire {
enum class Kind : uint8_t { identity_request=1, identity_response=2, capability_request=3,
    capability_response=4, snapshot_request=5, snapshot_response=6,
    volume_set=7, command_result=8 };
struct VolumeSet { uint64_t request=0; uint32_t numerator=0, denominator=0; };
bool parseVolumeSet(const uint8_t*, size_t, VolumeSet&);
bool volumePercent(const VolumeSet&, uint8_t&);
size_t identity(const char* firmware, uint8_t*, size_t);
size_t capabilities(uint8_t*, size_t);
size_t snapshot(uint8_t volume, uint8_t playback, uint64_t sequence,
                uint64_t media_generation, uint8_t*, size_t);
size_t result(uint64_t request, bool accepted, uint8_t error, uint8_t*, size_t);
}
