#include "rtcp_catalog.h"

#include <cassert>
#include <cstdint>
#include <cstdio>

using namespace rtcp_catalog;

int main() {
    const uint8_t query_bytes[] = {kQueryRequest, uint8_t(Collection::favorites),
                                   0, 5, 5};
    Query query{};
    assert(parseQuery(query_bytes, sizeof query_bytes, query));
    assert(query.collection == Collection::favorites && query.offset == 5 &&
           query.limit == 5);
    uint8_t bad_query[sizeof query_bytes]{};
    for (size_t i = 0; i < sizeof query_bytes; ++i) bad_query[i] = query_bytes[i];
    bad_query[4] = 6;
    assert(!parseQuery(bad_query, sizeof bad_query, query));

    PageView view{};
    view.collection = Collection::favorites;
    view.flags = full;
    view.capacity = 10;
    view.count = 7;
    view.start = 5;
    view.item_count = 2;
    view.revision = 12;
    view.items[0] = {5, false, "Alpha"};
    view.items[1] = {6, true, "B\xc3\xa9ta"};
    uint8_t output[288]{};
    const size_t page_size = page(view, 19, output, sizeof output);
    assert(page_size == 28 + 9 + 9);
    assert(output[0] == kPageResponse && output[1] == kMajor);
    assert(output[2] == uint8_t(Collection::favorites) && output[3] == full);
    assert(output[8] == 2 && output[9] == 10);
    assert(output[29] == 5 && output[30] == 0 && output[31] == 5);
    assert(output[38] == 6 && output[39] == 1 && output[40] == 5);

    view.items[1].name = "bad\x01text";
    assert(page(view, 19, output, sizeof output) == 0);
    view.items[1].name = "B\xc3\xa9ta";
    view.items[1].token = 4;
    assert(page(view, 19, output, sizeof output) == 0);

    uint8_t command_bytes[21]{};
    command_bytes[0] = kCommandRequest;
    command_bytes[8] = 9;
    command_bytes[9] = uint8_t(Operation::remove);
    command_bytes[10] = uint8_t(Collection::favorites);
    command_bytes[12] = 2;
    command_bytes[20] = 12;
    Command command{};
    assert(parseCommand(command_bytes, sizeof command_bytes, command));
    assert(command.request == 9 && command.token == 2 &&
           command.expected_revision == 12);
    assert(admit(command, 12, 7, false).error == success);
    assert(admit(command, 11, 7, false).error == stale);
    assert(admit(command, 12, 7, true).error == busy);
    command.token = 7;
    assert(admit(command, 12, 7, false).error == unavailable);
    command.token = 2;
    command.collection = Collection::recent;
    assert(admit(command, 12, 7, false).error == unsupported);
    command.operation = Operation::add_favorite;
    assert(admit(command, 12, 7, false).error == success);
    command.collection = Collection::saved;
    assert(admit(command, 12, 7, false).error == unsupported);

    command.collection = Collection::favorites;
    command.operation = Operation::play;
    Mailbox mailbox;
    assert(mailbox.put(command));
    assert(!mailbox.put(command));
    Command taken{};
    assert(mailbox.take(taken));
    assert(taken.request == command.request && taken.token == command.token);
    assert(!mailbox.take(taken));
    std::puts("RTCP catalog model: PASS");
}
