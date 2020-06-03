/*
 *  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
 */

#include <stdio.h>
#include <nic/sdk/include/sdk/table.hpp>

namespace pds_ms {

int pds_ms_init(void) __attribute__ ((weak));
void * pds_ms_thread_init(void *ctxt) __attribute__ ((weak));
sdk_ret_t pds_ms_upg_hitless_init (void) __attribute__ ((weak));

int
pds_ms_init (void)
{
    return 0;
}

void *
pds_ms_thread_init (void *ctxt)
{
    return nullptr;
}

sdk_ret_t 
pds_ms_upg_hitless_init (void)
{
    return  SDK_RET_OK;
}
}
