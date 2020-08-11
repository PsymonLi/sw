#include <arpa/inet.h>
#include <inttypes.h>

#include "gen/proto/operd/events.pb.h"
#include "lib/operd/operd.hpp"

extern "C" {

void
handler(sdk::operd::log_ptr entry)
{
    operd::Event event;
    bool result = event.ParseFromArray(entry->data(), entry->data_length());
    assert(result == true);

    printf("%u %u %s %s\n", event.type(), event.category(),
           event.description().c_str(), event.message().c_str());
}

} // extern "C"
