//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// Mock stub init 
//---------------------------------------------------------------

#include "nic/metaswitch/stubs/pds_ms_stubs_init.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hal_init.hpp"

namespace pds_ms {
int init() 
{
    return 1;
};

bool hal_init(void)
{
    return true;
}

void hal_deinit(void)
{
    return;
}

bool hal_hitless_upg_supp()
{
    return false;
}

}
