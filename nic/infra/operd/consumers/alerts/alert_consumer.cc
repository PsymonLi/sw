//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file consumes events from operd daemon
///
//----------------------------------------------------------------------------

#include <arpa/inet.h>
#include <inttypes.h>
#include "nic/sdk/lib/operd/operd.hpp"
#include "gen/proto/operd/events.pb.h"

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

}    // extern "C"
