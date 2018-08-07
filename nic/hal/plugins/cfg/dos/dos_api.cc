//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/hal/plugins/cfg/dos/dos.hpp"
#include "nic/include/dos_api.hpp"

namespace hal {

void
dos_set_pd_dos (dos_policy_t *pi_nw, void *pd_nw)
{
    pi_nw->pd = pd_nw;

}

}    // namespace hal
