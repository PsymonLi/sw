//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_APOLLO_SESS_HELPER_H__
#define __VPP_IMPL_APOLLO_SESS_HELPER_H__

#include <nic/vpp/flow/node.h>
#include <nic/apollo/p4/include/defines.h>

always_inline bool
pds_flow_from_host (u32 ses_id, u8 flow_role)
{
    return false;
}

always_inline void
pds_session_get_rewrite_flags (u32 ses_id, u8 pkt_type,
                               u16 *tx_rewrite, u16 *rx_rewrite)
{
    *tx_rewrite = 0;
    *rx_rewrite = 0;
}

#endif    // __VPP_IMPL_APOLLO_SESS_HELPER_H__
