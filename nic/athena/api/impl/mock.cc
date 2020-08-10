#include <stdio.h>
#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/apollo/core/core.hpp"
#include <nic/sdk/include/sdk/table.hpp>


namespace pds_ms {

void *
pds_ms_thread_init (void *ctxt)
{
    sdk::lib::thread::find(core::PDS_THREAD_ID_ROUTING)->set_ready(true);
    while (1) {
        sleep(2);
    }
    return NULL;
}

sdk_ret_t 
pds_ms_upg_hitless_init (void)
{
    return SDK_RET_OK;
}

}
