#include "../src/volume_target.h"
#include <cassert>
#include <iostream>

int main() {
    unsigned checks = 0;
    assert(VolumeTarget::afterDetents(50, 1) == 55); ++checks;
    assert(VolumeTarget::afterDetents(50, -1) == 45); ++checks;
    assert(VolumeTarget::afterDetents(25, 15) == 100); ++checks;
    assert(VolumeTarget::afterDetents(95, 3 - 2) == 100); ++checks;
    assert(VolumeTarget::afterDetents(95, -3 + 2) == 90); ++checks;
    assert(VolumeTarget::afterDetents(95, 15) == 100); ++checks;
    assert(VolumeTarget::afterDetents(5, -15) == 0); ++checks;
    assert(VolumeTarget::afterDetents(100, 1) == 100); ++checks;
    assert(VolumeTarget::afterDetents(0, -1) == 0); ++checks;
    assert(VolumeTarget::afterDetents(50, 0) == 50); ++checks;
    std::cout << "TOTAL " << checks << "/" << checks << " PASS\n";
}
