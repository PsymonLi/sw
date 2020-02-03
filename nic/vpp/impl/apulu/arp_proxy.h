//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_APULU_ARP_PROXY_H__
#define __VPP_IMPL_APULU_ARP_PROXY_H__

#include <nic/apollo/api/impl/apulu/nacl_data.h>
#include "p4_cpu_hdr_utils.h"
#include <impl_db.h>
#include <api.h>

always_inline void
pds_arp_proxy_pipeline_init (void)
{
    pds_infra_api_reg_t params = {0};

    params.nacl_id = NACL_DATA_ID_FLOW_MISS_ARP;
    params.node = format(0, "pds-arp-proxy");
    params.frame_queue_index = ~0;
    params.handoff_thread = ~0;
    params.offset = 0;
    params.unreg = 0;

    if (0 != pds_register_nacl_id_to_node(&params)) {
        ASSERT(0);
    }
    return;
}

always_inline void
pds_arp_proxy_add_tx_hdrs_x2 (vlib_buffer_t *b0, vlib_buffer_t *b1,
                              u16 nh_id0, u16 nh_id1)
{
    p4_tx_cpu_hdr_t *tx0, *tx1;

    tx0 = vlib_buffer_get_current(b0);
    tx1 = vlib_buffer_get_current(b1);

    tx0->lif_flags = 0;
    tx0->nexthop_valid = 1;
    tx0->nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    tx0->nexthop_id = clib_host_to_net_u16(nh_id0);
    tx0->lif_flags = clib_host_to_net_u16(tx0->lif_flags);
    tx1->lif_flags = 0;
    tx1->nexthop_valid = 1;
    tx1->nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    tx1->nexthop_id = clib_host_to_net_u16(nh_id1);
    tx1->lif_flags = clib_host_to_net_u16(tx0->lif_flags);
}

always_inline void
pds_arp_proxy_add_tx_hdrs_x1 (vlib_buffer_t *b, u16 nh_id)
{
    p4_tx_cpu_hdr_t *tx;

    tx = vlib_buffer_get_current(b);
    tx->lif_flags = 0;
    tx->nexthop_valid = 1;
    tx->nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    tx->nexthop_id = clib_host_to_net_u16(nh_id);
    tx->lif_flags = clib_host_to_net_u16(tx->lif_flags);
}

always_inline void
arp_proxy_exit_internal_x2 (vlib_buffer_t *p0, vlib_buffer_t *p1)
{
    u16 vnic_id0, vnic_id1;
    pds_impl_db_vnic_entry_t *vnic0, *vnic1;
    u16 nh_hw_id1 = 0, nh_hw_id2 = 0;

    vnic_id0 = vnet_buffer(p0)->pds_arp_data.vnic_id;
    vnic0 = pds_impl_db_vnic_get(vnic_id0);
    vnic_id1 = vnet_buffer(p1)->pds_arp_data.vnic_id;
    vnic1 = pds_impl_db_vnic_get(vnic_id1);
    if (PREDICT_TRUE(vnic0 != NULL)) {
        nh_hw_id1 = vnic0->nh_hw_id;
    }
    if (PREDICT_TRUE(vnic1 != NULL)) {
        nh_hw_id2 = vnic1->nh_hw_id;
    }
    pds_arp_proxy_add_tx_hdrs_x2(p0, p1, nh_hw_id1, nh_hw_id2);
}

always_inline void
arp_proxy_exit_internal_x1 (vlib_buffer_t *p)
{
    u16 vnic_id;
    pds_impl_db_vnic_entry_t *vnic;

    vnic_id = vnet_buffer(p)->pds_arp_data.vnic_id;
    vnic = pds_impl_db_vnic_get(vnic_id);
    pds_arp_proxy_add_tx_hdrs_x1(p, vnic->nh_hw_id);
}

#endif      //__VPP_IMPL_APULU_ARP_PROXY_H__
