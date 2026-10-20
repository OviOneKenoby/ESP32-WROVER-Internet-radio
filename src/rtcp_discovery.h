#pragma once

#include <cstdint>

namespace rtcp {

enum class DiscoveryAction : std::uint8_t { none, start, stop };

struct DiscoveryAdvertisementModel {
    bool active{false};
    std::uint32_t starts{0};
    std::uint32_t stops{0};
    std::uint32_t failures{0};

    DiscoveryAction update(bool service_ready, bool retry_due) const {
        if (!service_ready && active) return DiscoveryAction::stop;
        if (service_ready && !active && retry_due) return DiscoveryAction::start;
        return DiscoveryAction::none;
    }
    void started(bool ok) {
        if (ok) { active = true; if (starts != UINT32_MAX) ++starts; }
        else if (failures != UINT32_MAX) ++failures;
    }
    void stopped() {
        active = false;
        if (stops != UINT32_MAX) ++stops;
    }
};

class DiscoveryAdvertisement {
public:
    void poll(bool service_ready, std::uint32_t now_ms);
    const DiscoveryAdvertisementModel& diagnostics() const { return model_; }

private:
    static constexpr std::uint32_t retry_ms = 5000;
    DiscoveryAdvertisementModel model_{};
    std::uint32_t retry_at_ms_{0};
};

}  // namespace rtcp
