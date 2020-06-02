//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// Main entry point for the Pensando Distributed Services Agent (PDSA)
//---------------------------------------------------------------

#include "nic/metaswitch/stubs/common/pds_ms_state_init.hpp"
#include "nic/metaswitch/stubs/hals/pds_ms_hal_init.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_mgmt_init.hpp"
#include "nic/metaswitch/stubs/pds_ms_stubs_init.hpp"
#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/sdk/include/sdk/base.hpp"

namespace pds_ms {

int
pds_ms_init (void)
{
    if (!pds_ms::state_init()) {
        return -1;
    }
    if (!pds_ms_mgmt_init()) {
        goto error;
    }

    return 0;

error:
    pds_ms::state_destroy();
    return -1;
}

static void
pds_ms_thread_cleanup (void *arg)
{
    pds_ms_terminate();
}

void *
pds_ms_thread_init (void *ctxt)
{
    // opting for graceful termination
    SDK_THREAD_INIT(ctxt);
    pthread_cleanup_push(pds_ms_thread_cleanup, NULL);

    if (pds_ms_init() < 0) {
        SDK_ASSERT("pds_ms_init failed!");
    }
    pthread_cleanup_pop(1);
    return NULL;
}

}
