//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Testapp to generate a syslog
///
//----------------------------------------------------------------------------

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nic/sdk/lib/operd/decoder.h"
#include "nic/sdk/lib/operd/logger.hpp"
#include "nic/sdk/lib/operd/operd.hpp"

int
main (int argc, const char *argv[])
{
    sdk::operd::producer_ptr producer = sdk::operd::producer::create("syslog");

    assert(argc == 2);
    
    producer->write(OPERD_DECODER_PLAIN_TEXT, sdk::operd::INFO, argv[1],
                    strlen(argv[1]) + 1);

    return 0;
}
