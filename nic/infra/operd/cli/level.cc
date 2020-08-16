//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file handles operdctl level CLI
///
//----------------------------------------------------------------------------

#include <stdint.h>
#include <memory>
#include "nic/sdk/lib/operd/region.hpp"
#include "nic/infra/operd/cli/operdctl.hpp"

int
level (int argc, const char *argv[])
{
    sdk::operd::region_ptr region;
    uint8_t new_level;
    
    if (argc != 2) {
        printf("Usage: dump <region> <level>\n"
               "<region> is the region name\n"
               "<level> is a number between 0-5\n");
        return -1;
    }

    new_level = (uint8_t)atoi(argv[1]);

    region = std::make_shared<sdk::operd::region>(argv[0]);

    region->set_severity(new_level);

    printf("Region %s level set to %i\n", argv[0], new_level);
    
    return 0;
}
