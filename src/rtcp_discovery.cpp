#include "rtcp_discovery.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <cstdio>

namespace rtcp {

void DiscoveryAdvertisement::poll(bool service_ready, std::uint32_t now_ms) {
    const bool retry_due = static_cast<std::int32_t>(now_ms - retry_at_ms_) >= 0;
    switch (model_.update(service_ready, retry_due)) {
        case DiscoveryAction::none:
            return;
        case DiscoveryAction::stop:
            MDNS.end();
            model_.stopped();
            retry_at_ms_ = now_ms;
            return;
        case DiscoveryAction::start:
            break;
    }

    char hostname[24]{};
    const std::uint32_t suffix = static_cast<std::uint32_t>(ESP.getEfuseMac());
    std::snprintf(hostname, sizeof hostname, "tuner-%06lx",
                  static_cast<unsigned long>(suffix & 0xFFFFFFUL));
    const bool ok = MDNS.begin(hostname) && MDNS.addService("rtcp", "tcp", 46411);
    if (!ok) MDNS.end();
    model_.started(ok);
    retry_at_ms_ = now_ms + retry_ms;
}

}  // namespace rtcp
