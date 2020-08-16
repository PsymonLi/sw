//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// sample consumer for operd
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/operd/operd.hpp"

extern "C" {

void
handler (sdk::operd::log_ptr entry)
{
    printf("%s\n", entry->data());
}

}
