//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_APULU_SESSION_HELPER_H__
#define __VPP_IMPL_APULU_SESSION_HELPER_H__

#include <nic/vpp/flow/node.h>
#include "impl_db.h"
#include <nic/apollo/p4/include/apulu_defines.h>

always_inline bool
pds_flow_from_host (u32 ses_id, u8 flow_role)
{
    pds_flow_hw_ctx_t *session = pds_flow_get_hw_ctx(ses_id);
    if (pds_flow_packet_l2l(session->packet_type)) {
        //both flows are from host
        return true;
    }

    // If iflow_rx is true, then iflow is from uplink and rflow is from host
    // and vice versa
    if (flow_role == TCP_FLOW_INITIATOR) {
        return !session->iflow_rx;
    }
    return session->iflow_rx;
}

always_inline void
pds_session_get_rewrite_flags (u32 ses_id, u8 pkt_type,
                               u16 *tx_rewrite, u16 *rx_rewrite)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_rewrite_flags_t *rewrite_flags;
    pds_impl_db_vnic_entry_t *vnic;
    pds_flow_hw_ctx_t *session = pds_flow_get_hw_ctx(ses_id);
    bool tx_vlan, rx_vlan;

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, pkt_type);
    if (PREDICT_FALSE(pds_flow_packet_l2l(pkt_type))) {
        vnic = pds_impl_db_vnic_get(session->dst_vnic_id);
        tx_vlan = vnic && vnic->encap_type != PDS_ETH_ENCAP_NO_VLAN;
        *tx_rewrite = rewrite_flags->tx_rewrite |
            (tx_vlan ?
                (P4_REWRITE_VLAN_ENCAP << P4_REWRITE_VLAN_START) :
                (P4_REWRITE_VLAN_DECAP << P4_REWRITE_VLAN_START));

        vnic = pds_impl_db_vnic_get(session->src_vnic_id);
        rx_vlan = vnic && vnic->encap_type != PDS_ETH_ENCAP_NO_VLAN;
        *rx_rewrite = rewrite_flags->rx_rewrite |
            (rx_vlan ?
                (P4_REWRITE_VLAN_ENCAP << P4_REWRITE_VLAN_START) :
                (P4_REWRITE_VLAN_DECAP << P4_REWRITE_VLAN_START));
    } else {
        *tx_rewrite = rewrite_flags->tx_rewrite;
        vnic = pds_impl_db_vnic_get(session->src_vnic_id);
        rx_vlan =  vnic && vnic->encap_type != PDS_ETH_ENCAP_NO_VLAN;
        *rx_rewrite = rewrite_flags->rx_rewrite |
            (rx_vlan ?
                (P4_REWRITE_VLAN_ENCAP << P4_REWRITE_VLAN_START) : 0);
    }
}

#endif    // __VPP_IMPL_APULU_SESSION_HELPER_H__
