#include "allocation_failure_records.h"
#include <cstdlib>
#include <iostream>

static int checks = 0;
void expect(bool value, const char* message) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
int main() {
    AllocationFailureRecords records;
    expect(records.total == 0 && records.last.sequence == 0, "empty snapshot");
    AllocationFailureRecord event;
    event.size = 1622; event.caps = 0x80c;
    event.task[0] = 'w'; event.pcs[0] = 0x40083f98; event.frameCount = 1;
    records.append(event);
    event.task[0] = 'x'; event.pcs[0] = 0;
    expect(records.first[0].task[0] == 'w' && records.first[0].pcs[0] == 0x40083f98,
        "records own names and addresses after caller changes");
    for (unsigned i = 2; i <= 10; ++i) { event.size = i; records.append(event); }
    expect(records.total == 10 && records.last.sequence == 10 && records.last.size == 10,
        "latest event retained when capacity exceeded");
    expect(records.first[0].sequence == 1 && records.first[0].size == 1622,
        "first failure survives subsequent failures");
    for (unsigned i = 1; i < 4; ++i)
        expect(records.first[i].sequence == i + 1 && records.first[i].size == i + 1,
            "first four events preserve original order");
    expect(records.first[0].caps == 0x80c, "capability mask preserved");
    records.total = UINT32_MAX;
    records.append(event);
    expect(records.total == UINT32_MAX && records.last.sequence == UINT32_MAX &&
        records.first[0].size == 1622, "counter saturation never indexes outside storage");
    std::cout << "PASS: " << checks << " failure-record retention checks\n";
}
