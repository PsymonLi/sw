//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "platform/src/lib/nicmgr/include/nicmgr_init.hpp"
#include "nic/apollo/api/impl/athena/athena_devapi_impl.hpp"

rdmamgr *
rdma_manager_init (mpartition *mp, lif_mgr *lm)
{
    return NULL;
}

devapi *
devapi_init(void)
{
    return api::impl::athena_devapi_impl::factory();
}
