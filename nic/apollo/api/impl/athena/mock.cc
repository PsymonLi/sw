#include <stdio.h>
#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/apollo/core/core.hpp"

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

}
