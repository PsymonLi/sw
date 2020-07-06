//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// PDS Metaswitch stub Subnet Spec store used by Mgmt
//---------------------------------------------------------------

#ifndef __PDS_MS_HITLESS_UPG_HPP__
#define __PDS_MS_HITLESS_UPG_HPP__

#include "include/sdk/base.hpp"

namespace pds_ms {

sdk_ret_t
pds_ms_upg_hitless_init (void);

sdk_ret_t
pds_ms_upg_hitless_start_test (void);

sdk_ret_t
pds_ms_upg_hitless_sync_test (void);

void
upg_ipc_init (void);

}

#endif
