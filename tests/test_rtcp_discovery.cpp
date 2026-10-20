#include "../src/rtcp_discovery.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace rtcp;
    DiscoveryAdvertisementModel m;
    assert(m.update(false, true) == DiscoveryAction::none);
    assert(m.update(true, false) == DiscoveryAction::none);
    assert(m.update(true, true) == DiscoveryAction::start);
    m.started(false);
    assert(!m.active && m.failures == 1);
    assert(m.update(true, true) == DiscoveryAction::start);
    m.started(true);
    assert(m.active && m.starts == 1);
    assert(m.update(true, true) == DiscoveryAction::none);
    assert(m.update(false, true) == DiscoveryAction::stop);
    m.stopped();
    assert(!m.active && m.stops == 1);
    std::puts("RTCP DISCOVERY ADVERTISEMENT 9/9 PASS");
}
