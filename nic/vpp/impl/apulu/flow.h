//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_APULU_FLOW_H__
#define __VPP_IMPL_APULU_FLOW_H__

#include <netinet/in.h>
#include <arpa/inet.h>
#include <gen/p4gen/apulu/include/p4pd.h>
#include <session.h>
#include <api.h>
#include <feature.h>
#include <nic/apollo/api/impl/apulu/nacl_data.h>
#include <vlib/vlib.h>
#include <vnet/vxlan/vxlan_packet.h>
#include <nic/vpp/impl/nat.h>
#include <nic/vpp/impl/pds_table.h>
#include "p4_cpu_hdr_utils.h"
#include "impl_db.h"
#include <netinet/ether.h>
#include "gen/p4gen/p4/include/ftl.h"

#define PDS_FLOW_UPLINK0_LIF_ID     0x0
#define PDS_FLOW_UPLINK1_LIF_ID     0x1

#define PDS_FLOW_NH_ID_BITS            0b1111111111111
#define PDS_FLOW_NH_ID_BIT_POS         0
#define PDS_FLOW_NH_ID_GET(nh_entry)                                    \
    (((nh_entry) >> PDS_FLOW_NH_ID_BIT_POS) & PDS_FLOW_NH_ID_BITS)
#define PDS_FLOW_NH_ID_SET(nh_entry, val)                               \
    (nh_entry = nh_entry | (val << PDS_FLOW_NH_ID_BIT_POS))

#define PDS_FLOW_NH_TYPE_BITS            0b11
#define PDS_FLOW_NH_TYPE_BIT_POS         13
#define PDS_FLOW_NH_TYPE_GET(nh_entry)                                  \
    (((nh_entry) >> PDS_FLOW_NH_TYPE_BIT_POS) & PDS_FLOW_NH_TYPE_BITS)
#define PDS_FLOW_NH_TYPE_SET(nh_entry, val)                             \
    (nh_entry = nh_entry | (val << PDS_FLOW_NH_TYPE_BIT_POS))

#define PDS_FLOW_NH_DROP_BITS            0b1
#define PDS_FLOW_NH_DROP_BIT_POS         15
#define PDS_FLOW_NH_DROP_GET(nh_entry)                                  \
    (((nh_entry) >> PDS_FLOW_NH_DROP_BIT_POS) & PDS_FLOW_NH_DROP_BITS)
#define PDS_FLOW_NH_DROP_SET(nh_entry, val)                             \
    (nh_entry = nh_entry | (val << PDS_FLOW_NH_DROP_BIT_POS))

#define PDS_FLOW_NH_PRIO_BITS            0b111111
#define PDS_FLOW_NH_PRIO_BIT_POS         16
#define PDS_FLOW_NH_PRIO_GET(nh_entry)                                  \
    (((nh_entry) >> PDS_FLOW_NH_PRIO_BIT_POS) & PDS_FLOW_NH_PRIO_BITS)
#define PDS_FLOW_NH_PRIO_SET(nh_entry, val)                             \
    (nh_entry = nh_entry | (val << PDS_FLOW_NH_PRIO_BIT_POS))

#define session_tx_rewrite_flags actiondata.tx_rewrite_flags
#define session_rx_rewrite_flags actiondata.rx_rewrite_flags

typedef CLIB_PACKED(struct pds_vxlan_template_header_s {
    ethernet_header_t eth;
    ip4_header_t ip4;
    udp_header_t udp;
    vxlan_header_t vxlan;
}) pds_vxlan_template_header_t;

always_inline u32
pds_session_get_max (void)
{
    // session table is 2M - 1 as 0 is reserved.
    // so index is 0 to (2 *1024 * 1024) - 2
    return 2097150;
}

extern pds_flow_main_t pds_flow_main;

always_inline u16
pds_get_cpu_flags_from_vnic (pds_impl_db_vnic_entry_t *vnic)
{
    u16 flags = 0;

    if (vnic->encap_type != PDS_ETH_ENCAP_NO_VLAN)
        BIT_SET(flags, VPP_CPU_FLAGS_VLAN_ENCAP);
    if (PREDICT_FALSE(vnic->flow_log_en))
        BIT_SET(flags, VPP_CPU_FLAGS_FLOW_LOG);
    return flags;
}

always_inline u16
pds_get_l2l_cpu_flags_from_vnic (pds_impl_db_vnic_entry_t *vnic)
{
    u16 flags = 0;

    if (vnic->encap_type != PDS_ETH_ENCAP_NO_VLAN)
        BIT_SET(flags, VPP_CPU_FLAGS_L2L_RX_VLAN_ENCAP);
    return flags;
}

always_inline int
pds_session_get_advance_offset (void)
{
    return (APULU_P4_TO_ARM_HDR_SZ - APULU_ARM_TO_P4_HDR_SZ);
}

always_inline int
pds_session_get_nat_drop_next_offset (vlib_buffer_t *p0)
{
    return APULU_P4_TO_ARM_HDR_SZ + vnet_buffer(p0)->l3_hdr_offset -
           vnet_buffer(p0)->l2_hdr_offset;
}

always_inline void
pds_session_prog_x1 (vlib_buffer_t *b, u32 session_id,
                     u16 thread_id, u16 *next, u32 *counter)
{
    session_track_actiondata_t track_actiondata = {0};
    static struct session_info_entry_t actiondata = {0};
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_rewrite_flags_t *rewrite_flags;
    pds_flow_hw_ctx_t *ctx;
    bool ses_track_en;

    ctx = pds_flow_get_hw_ctx(session_id);
    if (PREDICT_FALSE(!ctx)) {
        next[0] = SESSION_PROG_NEXT_DROP;
        return;
    }
    if (pds_is_flow_napt_en(b)) {
        actiondata.tx_xlate_id =
            vnet_buffer2(b)->pds_nat_data.xlate_idx;
        actiondata.rx_xlate_id =
            vnet_buffer2(b)->pds_nat_data.xlate_idx_rflow;
        actiondata.tx_xlate_id2 = 0;
        actiondata.rx_xlate_id2 = 0;
    } else if (pds_is_flow_svc_map_en(b)) {
        /* Service mapping case (pip, port --> vip, port) */
        actiondata.tx_xlate_id =
            vnet_buffer2(b)->pds_nat_data.xlate_idx;
        actiondata.rx_xlate_id = 0;
        actiondata.tx_xlate_id2 = 0;
        actiondata.rx_xlate_id2 = 0;
    } else if (vnet_buffer2(b)->pds_nat_data.xlate_idx) {
        /* static nat */
        actiondata.tx_xlate_id =
            vnet_buffer2(b)->pds_nat_data.xlate_idx;
        actiondata.rx_xlate_id =
            vnet_buffer2(b)->pds_nat_data.xlate_idx + 1;
        actiondata.tx_xlate_id2 = 0;
        actiondata.rx_xlate_id2 = 0;
    }
    if (vnet_buffer2(b)->pds_nat_data.xlate_idx2) {
        // Twice NAT
        actiondata.tx_xlate_id2 =
            vnet_buffer2(b)->pds_nat_data.xlate_idx2;
        actiondata.rx_xlate_id2 =
            vnet_buffer2(b)->pds_nat_data.xlate_idx2 + 1;
    }
    rewrite_flags = vec_elt_at_index(fm->rewrite_flags,
                                     ctx->packet_type);
    if (PREDICT_FALSE(pds_flow_packet_l2l(ctx->packet_type))) {
        session_tx_rewrite_flags = rewrite_flags->tx_rewrite |
            (pds_is_flow_l2l_tx_vlan(b) ?
                (P4_REWRITE_VLAN_ENCAP << P4_REWRITE_VLAN_START) :
                (P4_REWRITE_VLAN_DECAP << P4_REWRITE_VLAN_START));
        session_rx_rewrite_flags = rewrite_flags->rx_rewrite |
            (pds_is_flow_l2l_rx_vlan(b) ?
                (P4_REWRITE_VLAN_ENCAP << P4_REWRITE_VLAN_START) : 
                (P4_REWRITE_VLAN_DECAP << P4_REWRITE_VLAN_START));
    } else {
        session_tx_rewrite_flags = rewrite_flags->tx_rewrite;
        session_rx_rewrite_flags = rewrite_flags->rx_rewrite |
            (pds_is_flow_rx_vlan(b) ?
                (P4_REWRITE_VLAN_ENCAP << P4_REWRITE_VLAN_START) : 0);
    }
    ses_track_en = fm->con_track_en && (ctx->proto == PDS_FLOW_PROTO_TCP);
    actiondata.session_tracking_en = ses_track_en;
    actiondata.drop = ctx->drop;
    actiondata.qid_en = true;
    // qid starts from 0 and worker thread id from 1
    actiondata.qid = (thread_id - 1);
    if (ses_track_en && !pds_is_flow_session_present(b)) {
        track_actiondata.action_u.session_track_session_track_info.iflow_tcp_state = FLOW_STATE_INIT;
        track_actiondata.action_u.session_track_session_track_info.iflow_tcp_seq_num =
            vnet_buffer(b)->pds_flow_data.flow_hash + 1;
        track_actiondata.action_u.session_track_session_track_info.iflow_tcp_win_size =
            vnet_buffer(b)->pds_flow_data.tcp_win_sz;
        track_actiondata.action_u.session_track_session_track_info.rflow_tcp_state = FLOW_STATE_INIT;
        track_actiondata.action_u.session_track_session_track_info.rflow_tcp_ack_num =
            vnet_buffer(b)->pds_flow_data.flow_hash + 1;
        track_actiondata.action_u.session_track_session_track_info.rflow_tcp_win_size =
            vnet_buffer(b)->pds_flow_data.tcp_win_sz;
        if (PREDICT_FALSE(session_track_program(session_id,
            (void *)&track_actiondata))) {
            if (pds_is_flow_napt_en(b)) {
                next[0] = SESSION_PROG_NEXT_NAT_DROP;
                vlib_buffer_advance(b, pds_session_get_nat_drop_next_offset(b));
            } else {
                next[0] = SESSION_PROG_NEXT_DROP;
            }
            return;
        }
    }

    if (PREDICT_FALSE(pds_session_program(session_id, (void *)&actiondata))) {
        if (pds_is_flow_napt_en(b)) {
            next[0] = SESSION_PROG_NEXT_NAT_DROP;
            vlib_buffer_advance(b, pds_session_get_nat_drop_next_offset(b));
        } else {
            next[0] = SESSION_PROG_NEXT_DROP;
        }
        return;
    }
    next[0] = (pds_flow_age_supported() && !pds_is_flow_defunct(b)) ?
              SESSION_PROG_NEXT_AGE_FLOW : SESSION_PROG_NEXT_FWD_FLOW;
    vlib_buffer_advance(b, pds_session_get_advance_offset());
}

always_inline int
pds_flow_prog_get_next_offset (vlib_buffer_t *p0, u8 is_l2)
{
    if (PREDICT_TRUE(!is_l2)) {
        return -(APULU_P4_TO_ARM_HDR_SZ +
                 (vnet_buffer(p0)->l3_hdr_offset - vnet_buffer(p0)->l2_hdr_offset));
    }

    // It's an L2 flow
    if (BIT_ISSET(vnet_buffer(p0)->pds_flow_data.flags,
                  VPP_CPU_FLAGS_RX_PKT)) {
        // This would be a VXLAN packet
        return -(APULU_P4_TO_ARM_HDR_SZ +
                 (vnet_buffer(p0)->pds_flow_data.l2_inner_offset -
                  vnet_buffer(p0)->l2_hdr_offset));
    }

    return -APULU_P4_TO_ARM_HDR_SZ;
}

always_inline int
pds_flow_prog_get_nat_drop_next_offset (vlib_buffer_t *p0)
{
    return APULU_P4_TO_ARM_HDR_SZ + vnet_buffer(p0)->l3_hdr_offset -
           vnet_buffer(p0)->l2_hdr_offset;
}

always_inline int
pds_flow_prog_get_next_node (void)
{
    return FLOW_PROG_NEXT_SESSION_PROG;
}

always_inline void
pds_flow_create_rx_vxlan_template (void)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_impl_db_device_entry_t *dev;
    pds_vxlan_template_header_t *hdr;
    p4i_device_info_actiondata_t p4i_device_info_data = { 0 };
    int p4pd_ret;

    dev = pds_impl_db_device_get();
    vec_validate_init_empty(fm->rx_vxlan_template,
                            sizeof(pds_vxlan_template_header_t) - 1, 0);
    hdr = (pds_vxlan_template_header_t *) fm->rx_vxlan_template;
    p4pd_ret = pds_table_read(P4TBL_ID_P4I_DEVICE_INFO, 0,
                              (void *)&p4i_device_info_data);
    if (p4pd_ret == 0) {
        u8 *uplink0_mac = p4i_device_info_data.action_u.\
                          p4i_device_info_p4i_device_info.device_mac_addr1;
        hdr->eth.dst_address[0] = uplink0_mac[5];
        hdr->eth.dst_address[1] = uplink0_mac[4];
        hdr->eth.dst_address[2] = uplink0_mac[3];
        hdr->eth.dst_address[3] = uplink0_mac[2];
        hdr->eth.dst_address[4] = uplink0_mac[1];
        hdr->eth.dst_address[5] = uplink0_mac[0];
    } else {
        clib_memcpy(hdr->eth.dst_address, dev->device_mac, ETH_ADDR_LEN);
    }
    hdr->eth.type = clib_host_to_net_u16(ETHERNET_TYPE_IP4);
    hdr->ip4.dst_address.as_u32 = clib_host_to_net_u32(dev->device_ip.ip4.as_u32);
    hdr->ip4.ip_version_and_header_length = 0x45;
    hdr->ip4.length = sizeof(ip4_header_t) + sizeof(udp_header_t) +
                      sizeof(vxlan_header_t);
    hdr->ip4.ttl = 255;
    hdr->ip4.protocol = IP_PROTOCOL_UDP;
    hdr->udp.src_port = clib_host_to_net_u16(50000); // any port
    hdr->udp.dst_port = clib_host_to_net_u16(4789); // vxlan
    hdr->udp.length = sizeof(udp_header_t) + sizeof(vxlan_header_t);
    hdr->vxlan.flags = VXLAN_FLAGS_I; // set flag vni to true
}

always_inline void
pds_flow_add_vxlan_template (vlib_buffer_t *b0)
{
    pds_flow_main_t *fm = &pds_flow_main;
    u16 vxlan_hdr_len, orig_len;
    pds_vxlan_template_header_t *hdr;
    u32 vnid = 0;
    pds_impl_db_subnet_entry_t *subnet;

    subnet = pds_impl_db_subnet_get(vnet_buffer(b0)->pds_flow_data.egress_lkp_id);
    if (PREDICT_TRUE(subnet != NULL)) {
        vnid = subnet->vnid;
    }

    if (PREDICT_FALSE(!fm->rx_vxlan_template)) {
        pds_flow_create_rx_vxlan_template();
    }
    vxlan_hdr_len = vec_len(fm->rx_vxlan_template);
    orig_len = b0->current_length - APULU_ARM_TO_P4_HDR_SZ;
    hdr = (pds_vxlan_template_header_t *) (((u8 *) vlib_buffer_push_uninit(b0,
          vxlan_hdr_len)) + APULU_ARM_TO_P4_HDR_SZ);
    clib_memcpy(hdr, fm->rx_vxlan_template, vxlan_hdr_len);
    hdr->vxlan.vni_reserved = vnid; // vnid store in network byte order
    hdr->ip4.length += orig_len;
    hdr->ip4.length = clib_host_to_net_u16(hdr->ip4.length);
    hdr->udp.length += orig_len;
    hdr->udp.length = clib_host_to_net_u16(hdr->udp.length);
    return;
}

always_inline void
pds_flow_add_tx_hdrs_x2 (vlib_buffer_t *b0, vlib_buffer_t *b1)
{
    p4_tx_cpu_hdr_t *tx0, *tx1;
    u32 lif0, lif1;
    u32 ses_id0, ses_id1;
    pds_flow_hw_ctx_t *ses0, *ses1;

    if (pds_is_flow_l2l(b0)) {
        pds_flow_add_vxlan_template(b0);
        lif0 = PDS_FLOW_UPLINK0_LIF_ID;
    } else {
        lif0 = vnet_buffer(b0)->pds_flow_data.lif;
    }
    if (pds_is_flow_l2l(b1)) {
        pds_flow_add_vxlan_template(b1);
        lif1 = PDS_FLOW_UPLINK0_LIF_ID;
    } else {
        lif1 = vnet_buffer(b1)->pds_flow_data.lif;
    }
    tx0 = vlib_buffer_get_current(b0);
    tx1 = vlib_buffer_get_current(b1);

    tx0->lif_flags = 0;
    tx1->lif_flags = 0;
    tx0->lif_sbit0_ebit7 = lif0 & 0xff;
    tx1->lif_sbit0_ebit7 = lif0 & 0xff;
    tx0->lif_sbit8_ebit10 = lif1 >> 0x8;
    tx1->lif_sbit8_ebit10 = lif1 >> 0x8;

    ses_id0 = vnet_buffer(b0)->pds_flow_data.ses_id;
    ses0 = pds_flow_get_hw_ctx(ses_id0);
    if (ses0 && ses0->ingress_bd) {
        tx0->flow_lkp_id_override = 1;
        tx0->flow_lkp_id =
            clib_host_to_net_u16(ses0->ingress_bd);
    }
    ses_id1 = vnet_buffer(b1)->pds_flow_data.ses_id;
    ses1 = pds_flow_get_hw_ctx(ses_id1);
    if (ses1 && ses1->ingress_bd) {
        tx1->flow_lkp_id_override = 1;
        tx1->flow_lkp_id =
            clib_host_to_net_u16(ses1->ingress_bd);
    }
    tx0->skip_stats_update = vnet_buffer(b0)->pds_flow_data.skip_session_stats_update;
    tx1->skip_stats_update = vnet_buffer(b1)->pds_flow_data.skip_session_stats_update;

    tx0->lif_flags = clib_host_to_net_u16(tx0->lif_flags);
    tx1->lif_flags = clib_host_to_net_u16(tx1->lif_flags);
}

always_inline void
pds_flow_add_tx_hdrs_x1 (vlib_buffer_t *b0)
{
    p4_tx_cpu_hdr_t *tx0;
    u32 lif = 0;
    u32 ses_id;
    pds_flow_hw_ctx_t *ses;

    if (pds_is_flow_l2l(b0)) {
        pds_flow_add_vxlan_template(b0);
        lif = PDS_FLOW_UPLINK0_LIF_ID;
    } else {
        lif = vnet_buffer(b0)->pds_flow_data.lif;
    }
    tx0 = vlib_buffer_get_current(b0);
    tx0->lif_flags = 0;
    tx0->lif_sbit0_ebit7 = lif & 0xff;
    tx0->lif_sbit8_ebit10 = lif >> 0x8;
    ses_id = vnet_buffer(b0)->pds_flow_data.ses_id;
    ses = pds_flow_get_hw_ctx(ses_id);
    if (ses && ses->ingress_bd) {
        tx0->flow_lkp_id_override = 1;
        tx0->flow_lkp_id =
            clib_host_to_net_u16(ses->ingress_bd);
    }
    tx0->skip_stats_update = vnet_buffer(b0)->pds_flow_data.skip_session_stats_update;
    tx0->lif_flags = clib_host_to_net_u16(tx0->lif_flags);
}

static char *
pds_flow4_key2str (void *key)
{
    static char str[256] = {0};
    flow_swkey_t *k = (flow_swkey_t *)key;
    char srcstr[INET_ADDRSTRLEN + 1];
    char dststr[INET_ADDRSTRLEN + 1];

    inet_ntop(AF_INET, k->key_metadata_src, srcstr, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, k->key_metadata_dst, dststr, INET_ADDRSTRLEN);
    sprintf(str, "Src:%s Dst:%s Dport:%u Sport:%u Proto:%u VNIC:%u",
            srcstr, dststr,
            k->key_metadata_dport, k->key_metadata_sport,
            k->key_metadata_proto, k->key_metadata_flow_lkp_id);
    return str;
}

static char *
pds_flow_key2str (void *key)
{
    static char str[256] = {0};
    flow_swkey_t *k = (flow_swkey_t *)key;
    char srcstr[INET6_ADDRSTRLEN + 1];
    char dststr[INET6_ADDRSTRLEN + 1];

    if (k->key_metadata_ktype == KEY_TYPE_IPV6) {
        inet_ntop(AF_INET6, k->key_metadata_src, srcstr, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, k->key_metadata_dst, dststr, INET6_ADDRSTRLEN);
        sprintf(str, "Src:%s Dst:%s Dport:%u Sport:%u Proto:%u VNIC:%u",
                srcstr, dststr,
                k->key_metadata_dport, k->key_metadata_sport,
                k->key_metadata_proto, k->key_metadata_flow_lkp_id);
    } else {
        // Key type MAC - L2 flow
        sprintf(str, "Src:%s Dst:%s EtherType:%u VNIC:%u",
                ether_ntoa((struct ether_addr *)k->key_metadata_src),
                ether_ntoa((struct ether_addr *)k->key_metadata_dst),
                k->key_metadata_dport, k->key_metadata_flow_lkp_id);
    }
    return str;
}

static char *
pds_flow_appdata2str (void *appdata)
{
    static char str[512] = {0};
    flow_appdata_t *d = (flow_appdata_t *)appdata;
    sprintf(str, "session_index:%d flow_role:%d",
            d->session_index, d->flow_role);
    return str;
}

always_inline bool
pds_is_flow_drop (vlib_buffer_t *p0)
{
    if (PDS_FLOW_NH_DROP_GET(vnet_buffer(p0)->pds_flow_data.nexthop)) {
        return true;
    }
    return false;
}

always_inline void
pds_l2l_flow_extract_nexthop_info (vlib_buffer_t *p0,
                                   u16 *nhid,
                                   u8 *nh_type,
                                   u8 *nh_valid,
                                   u8 *nh_priority,
                                   u8 *drop)
{
    u32 nexthop = 0;
    pds_flow_main_t *fm = &pds_flow_main;

    nexthop = vnet_buffer(p0)->pds_flow_data.nexthop;
    *nh_priority = PDS_FLOW_NH_PRIO_GET(nexthop);
    if (PREDICT_FALSE(PDS_FLOW_NH_DROP_GET(nexthop))) {
        *drop = 1;
        *nh_valid = 1;
        *nh_type = NEXTHOP_TYPE_NEXTHOP;
        *nhid = fm->drop_nexthop;
        return;
    }
    *drop = 0;
    *nh_type = PDS_FLOW_NH_TYPE_GET(nexthop);
    if (0 != nexthop) {
        *nh_valid = 1;
        *nhid = PDS_FLOW_NH_ID_GET(nexthop);
        return;
    }
    *nh_valid = 0;
    *nhid = 0;
    return;
}

always_inline void
pds_flow_extract_nexthop_info(vlib_buffer_t *p0,
                              u8 is_ip4, u8 iflow,
                              u16 thread_index)
{
    u32 nexthop = 0;
    pds_impl_db_vnic_entry_t *vnic0;
    pds_flow_main_t *fm = &pds_flow_main;

    // check if drop bit is set and program nh as drop
    if (PREDICT_FALSE(PDS_FLOW_NH_DROP_GET(vnet_buffer(p0)->pds_flow_data.nexthop))) {
        nexthop = vnet_buffer(p0)->pds_flow_data.nexthop;
        if (is_ip4) {
            ftlv4_cache_set_nexthop(fm->drop_nexthop, NEXTHOP_TYPE_NEXTHOP, 1,
                                    PDS_FLOW_NH_PRIO_GET(nexthop), thread_index);
        } else {
            ftlv6_cache_set_nexthop(fm->drop_nexthop, NEXTHOP_TYPE_NEXTHOP, 1,
                                    PDS_FLOW_NH_PRIO_GET(nexthop));
        }
        return;
    }

    u8 rx_pak = BIT_ISSET(vnet_buffer(p0)->pds_flow_data.flags,
                          VPP_CPU_FLAGS_RX_PKT) ? 1 : 0;

    if (iflow) {
        // nexthop is for iflow if tx packet
        if (!rx_pak) {
            nexthop = vnet_buffer(p0)->pds_flow_data.nexthop;
        }
    } else {
        if (rx_pak) {
            // nexthop is for rflow if rx packet
            nexthop = vnet_buffer(p0)->pds_flow_data.nexthop;
        } else {
            // use vnic nexthop for rflow as there may be cases (for example
            // NAT) where mapping is not hit for reverse flow.
            if (pds_is_flow_napt_en(p0)) {
                vnic0 = pds_impl_db_vnic_get(vnet_buffer2(p0)->pds_nat_data.vnic_id);
                if (vnic0) {
                    PDS_FLOW_NH_ID_SET(nexthop, vnic0->nh_hw_id);
                    PDS_FLOW_NH_TYPE_SET(nexthop, NEXTHOP_TYPE_NEXTHOP);
                }
            }
        }
    }

    if (is_ip4) {
        if (PDS_FLOW_NH_ID_GET(nexthop)) {
            ftlv4_cache_set_nexthop(PDS_FLOW_NH_ID_GET(nexthop),
                                    PDS_FLOW_NH_TYPE_GET(nexthop),
                                    1, PDS_FLOW_NH_PRIO_GET(nexthop),
                                    thread_index);
        } else {
            ftlv4_cache_set_nexthop(0, 0, 0, 0, thread_index);
        }
    } else {
        if (nexthop & 0xffff) {
            ftlv6_cache_set_nexthop(PDS_FLOW_NH_ID_GET(nexthop),
                                    PDS_FLOW_NH_TYPE_GET(nexthop),
                                    1, PDS_FLOW_NH_PRIO_GET(nexthop));
        } else {
            ftlv6_cache_set_nexthop(0, 0, 0, 0);
        }
    }
}

always_inline int
pds_flow_classify_get_advance_offset (vlib_buffer_t *b, u8 p4_flags)
{
    if (PREDICT_TRUE(BIT_ISSET(p4_flags, VPP_CPU_FLAGS_IP_VALID))) {
        return (VPP_P4_TO_ARM_HDR_SZ +
                (vnet_buffer(b)->l3_hdr_offset - vnet_buffer (b)->l2_hdr_offset));
    }

    // It is an L2 flow
    if (BIT_ISSET(vnet_buffer(b)->pds_flow_data.flags, VPP_CPU_FLAGS_RX_PKT)) {
        // This would be a VXLAN packet
        return (VPP_P4_TO_ARM_HDR_SZ +
                (vnet_buffer(b)->pds_flow_data.l2_inner_offset -
                 vnet_buffer(b)->l2_hdr_offset));
    }

    return VPP_P4_TO_ARM_HDR_SZ;
}

always_inline bool
pds_flow_mapping_over_route_check (p4_rx_cpu_hdr_t *hdr,
                                   u8 is_rx)
{
    static pds_impl_db_device_entry_t *device = NULL; 

    if (PREDICT_FALSE(NULL == device)) {
        device = pds_impl_db_device_get();
    }

    // no route, then mapping wins always
    if (hdr->drop || (0 == hdr->nexthop_id)) {
        return true;
    }

    // packet from host
    if (!is_rx) {
        if (!hdr->mapping_hit) {
            // only route present
            return false;
        }
        goto priority_check;
    }
    // packet from uplink
    if (!hdr->src_mapping_hit) {
        // only route present
        return false;
    }

priority_check:
    // both route and mapping are hit, whichever has lesser
    // value of priority that wins.
    if (PREDICT_FALSE(hdr->route_priority < device->mapping_prio)) {
        return false;
    }
    // in case of tie between mapping and route, mapping wins.
    return true;
}

always_inline void
pds_flow_packet_type_derive (vlib_buffer_t *p, p4_rx_cpu_hdr_t *hdr,
                             u16 flags, u16 *next, u32 *counter,
                             pds_flow_hw_ctx_t *ctx)
{
    u16 xlate_id;
    u8 intra_subnet = 0;
    u8 pkt_type;
    pds_impl_db_device_entry_t *dev = pds_impl_db_device_get();
    bool mapping;

    vnet_buffer(p)->sw_if_index[VLIB_TX] = hdr->ingress_bd_id;

    if (!hdr->rx_packet) {
        if (hdr->ingress_bd_id == hdr->egress_bd_id) {
            intra_subnet = 1;
        }
        mapping = pds_flow_mapping_over_route_check(hdr, 0);
        if (mapping) {
            // assume l2l traffic is handled by vswitch on host
            if (PREDICT_FALSE(hdr->is_local)) {
                vnet_buffer(p)->pds_flow_data.egress_lkp_id = hdr->egress_bd_id;
                if (intra_subnet) {
                    pkt_type = PDS_FLOW_L2L_INTRA_SUBNET;
                } else {
                    pkt_type = PDS_FLOW_L2L_INTER_SUBNET;
                }
            } else {
                // since mapping hit, remote side will send vnid of local mapping
                vnet_buffer(p)->pds_flow_data.egress_lkp_id = hdr->ingress_bd_id;
                if (intra_subnet) {
                    pkt_type = PDS_FLOW_L2R_INTRA_SUBNET;
                } else {
                    pkt_type = PDS_FLOW_L2R_INTER_SUBNET;
                }
            }
        } else {
            // route hit case
            if (dev->overlay_routing_en) {
                // remote end knows subnet VNI
                vnet_buffer(p)->pds_flow_data.egress_lkp_id =
                        hdr->ingress_bd_id;
                pkt_type = PDS_FLOW_L2N_OVERLAY_ROUTE_EN;
            } else {
                // remote end knows VNI of VPC, not subnet
                pds_impl_db_vpc_entry_t *vpc = pds_impl_db_vpc_get(hdr->vpc_id);
                if (PREDICT_FALSE(!vpc)) {
                    *next = FLOW_CLASSIFY_NEXT_DROP;
                    counter[FLOW_CLASSIFY_COUNTER_VPC_NOT_FOUND] += 1;
                    return;
                }
                if (PREDICT_FALSE(vpc->flags & PDS_VPP_VPC_FLAGS_CONTROL_VPC &&
                    hdr->snat_type == ROUTE_RESULT_SNAT_TYPE_NONE)) {
                    pkt_type = PDS_FLOW_L2N_INTRA_VCN_ROUTE;
                } else {
                    pkt_type = PDS_FLOW_L2N_OVERLAY_ROUTE_DIS;
                }
                vnet_buffer(p)->pds_flow_data.egress_lkp_id = vpc->hw_bd_id;
            }
            if (hdr->snat_type != ROUTE_RESULT_SNAT_TYPE_NONE) {
                /* only from host supported for now */
                /*
                 * Try service NAPT first
                 */
                if (hdr->snat_type == ROUTE_RESULT_SNAT_TYPE_NAPT_SVC) {
                    if (BIT_ISSET(flags, VPP_CPU_FLAGS_IPV4_1_VALID)) {
                        BIT_SET(vnet_buffer(p)->pds_flow_data.flags,
                                (VPP_CPU_FLAGS_NAPT | VPP_CPU_FLAGS_NAPT_SVC));
                        *next = FLOW_CLASSIFY_NEXT_IP4_NAT;
                        counter[FLOW_CLASSIFY_COUNTER_IP4_NAT] += 1;
                    } else {
                        *next = FLOW_CLASSIFY_NEXT_DROP;
                        counter[FLOW_CLASSIFY_COUNTER_UNKOWN] += 1;
                        return;
                    }
                    if (hdr->dnat_en) {
                        // For now only supported in overlay_routing_dis case
                        u32 ip;
                        u16 port;
                        xlate_id = hdr->dnat_id * 2;
                        pds_dnat_tbl_read_ip4(xlate_id, &ip, &port);
                        vnet_buffer2(p)->pds_nat_data.xlate_idx2 = xlate_id;
                        vnet_buffer2(p)->pds_nat_data.xlate_addr2 = ip;
                        pkt_type = PDS_FLOW_L2N_OVERLAY_ROUTE_DIS_TWICE_NAT;
                    } else if (dev->overlay_routing_en) {
                        pkt_type = PDS_FLOW_L2N_OVERLAY_ROUTE_EN_NAPT;
                    } else {
                        pkt_type = PDS_FLOW_L2N_OVERLAY_ROUTE_DIS_NAPT;
                    }
                }
                /* Try static NAT */
                else if (hdr->mapping_xlate_id != 0) {
                    u32 ip;
                    u16 port;
                    xlate_id = hdr->mapping_xlate_id;
                    pds_snat_tbl_read_ip4(xlate_id, &ip, &port);
                    vnet_buffer2(p)->pds_nat_data.xlate_idx = xlate_id;
                    vnet_buffer2(p)->pds_nat_data.xlate_addr = ip;
                    if (dev->overlay_routing_en) {
                        pkt_type = PDS_FLOW_L2N_OVERLAY_ROUTE_EN_NAT;
                    } else {
                        pkt_type = PDS_FLOW_L2N_OVERLAY_ROUTE_DIS_NAT;
                    }
                } else {
                    /* nat44 */
                    if (BIT_ISSET(flags, VPP_CPU_FLAGS_IPV4_1_VALID)) {
                        BIT_SET(vnet_buffer(p)->pds_flow_data.flags,
                                VPP_CPU_FLAGS_NAPT);
                        *next = FLOW_CLASSIFY_NEXT_IP4_NAT;
                        counter[FLOW_CLASSIFY_COUNTER_IP4_NAT] += 1;
                    } else {
                        *next = FLOW_CLASSIFY_NEXT_DROP;
                        counter[FLOW_CLASSIFY_COUNTER_UNKOWN] += 1;
                        return;
                    }
                    if (dev->overlay_routing_en) {
                        pkt_type = PDS_FLOW_L2N_OVERLAY_ROUTE_EN_NAPT;
                    } else {
                        pkt_type = PDS_FLOW_L2N_OVERLAY_ROUTE_DIS_NAPT;
                    }
                }
            }
        }
    } else {
        mapping = pds_flow_mapping_over_route_check(hdr, 1);
        vnet_buffer(p)->pds_flow_data.egress_lkp_id = hdr->egress_bd_id;
        if (!mapping) {
            pds_impl_db_vpc_entry_t *vpc = pds_impl_db_vpc_get(hdr->vpc_id);
            if (PREDICT_FALSE(!vpc)) {
                *next = FLOW_CLASSIFY_NEXT_DROP;
                counter[FLOW_CLASSIFY_COUNTER_VPC_NOT_FOUND] += 1;
                return;
            }
            if (PREDICT_FALSE(vpc->flags & PDS_VPP_VPC_FLAGS_CONTROL_VPC)) {
                pkt_type = PDS_FLOW_N2L_INTRA_VCN_ROUTE;
            } else {
                // rflow is not mapping hit, its route hit.
                // ingress bd id will be VPC bd id
                if (dev->overlay_routing_en) {
                    pkt_type = PDS_FLOW_N2L_OVERLAY_ROUTE_EN;
                } else {
                    pkt_type = PDS_FLOW_N2L_OVERLAY_ROUTE_DIS;
                }
                if (hdr->service_xlate_id) {
                    /* Service mapping case (pip, port --> vip, port) */
                    BIT_SET(vnet_buffer(p)->pds_flow_data.flags,
                            VPP_CPU_FLAGS_NAT_SVC_MAP);
                    vnet_buffer2(p)->pds_nat_data.xlate_idx = hdr->service_xlate_id;
                    if (dev->overlay_routing_en) {
                        pkt_type = PDS_FLOW_N2L_OVERLAY_ROUTE_EN_SVC_NAT;
                    } else {
                        pkt_type = PDS_FLOW_N2L_OVERLAY_ROUTE_DIS_SVC_NAT;
                    }
                } else if (hdr->snat_type != ROUTE_RESULT_SNAT_TYPE_NONE) {
                    /* Only static nat should be valid here */
                    if (hdr->mapping_xlate_id != 0) {
                        u32 ip;
                        u16 port;
                        xlate_id = hdr->mapping_xlate_id - 1;
                        pds_snat_tbl_read_ip4(xlate_id + 1, &ip, &port);
                        vnet_buffer2(p)->pds_nat_data.xlate_idx = xlate_id;
                        vnet_buffer2(p)->pds_nat_data.xlate_addr = ip;
                        if (dev->overlay_routing_en) {
                            pkt_type = PDS_FLOW_N2L_OVERLAY_ROUTE_EN_NAT;
                        } else {
                            pkt_type = PDS_FLOW_N2L_OVERLAY_ROUTE_DIS_NAT;
                        }
                    } else {
                        /*
                         * We should not reach here since in NAT invalid case,
                         * vnic is not found and this function is not called.
                         * But still increment the NAT invalidate counter
                         */
                        *next = FLOW_CLASSIFY_NEXT_DROP;
                        counter[FLOW_CLASSIFY_COUNTER_IP4_NAT_INVAL] += 1;
                        return;
                    }
                }
            }
        } else {
            if ((hdr->is_l3_vnid) ||
                (hdr->egress_bd_id != hdr->src_bd_id)) {
                pkt_type = PDS_FLOW_R2L_INTER_SUBNET;
            } else {
                pkt_type = PDS_FLOW_R2L_INTRA_SUBNET;
            }
        }
    }

    // if existing session then retain packet type last known
    if (PREDICT_FALSE(NULL != ctx)) {
        pkt_type = ctx->packet_type;
    }
    vnet_buffer(p)->pds_flow_data.packet_type = pkt_type;
    return;
}

always_inline int
pds_flow_vr_ip_ping (p4_rx_cpu_hdr_t *hdr, vlib_buffer_t *vlib, u16 nh_hw_id,
                     u8 is_ip4, u16 *next, u32 *counter)
{
    uint16_t bd_id;
    uint32_t vrip = 0;
    uint8_t *vrmac;
    icmp46_header_t *icmp0;

    if (is_ip4) {
        ip4_header_t *ip40;
        u32 dst_ip;
        u8 protocol;

        ip40 = vlib_buffer_get_current(vlib);
        dst_ip = clib_net_to_host_u32(ip40->dst_address.as_u32);
        protocol = ip40->protocol;
        icmp0 = (icmp46_header_t *) (((u8 *) ip40) +
                (vnet_buffer (vlib)->l4_hdr_offset -
                 vnet_buffer (vlib)->l3_hdr_offset));

        bd_id = ((p4_rx_cpu_hdr_t *)hdr)->egress_bd_id;
        pds_impl_db_vr_ip_mac_get(bd_id, &vrip, &vrmac);
        if (dst_ip == vrip) {
            if (protocol == IP_PROTOCOL_ICMP) {
                if (PREDICT_TRUE(icmp0->type == ICMP4_echo_request)) {
                    vnet_buffer(vlib)->pds_tx_data.vnic_id = hdr->vnic_id;
                    vnet_buffer(vlib)->pds_tx_data.nh_id = nh_hw_id;
                    vnet_buffer(vlib)->pds_tx_data.ether_type = 0;
                    vnet_buffer(vlib)->sw_if_index[VLIB_TX] =
                        vnet_buffer(vlib)->sw_if_index[VLIB_RX];
                    *next = FLOW_CLASSIFY_NEXT_ICMP_VRIP;
                    counter[FLOW_CLASSIFY_COUNTER_ICMP_VRIP] += 1;
                    return 0;
                }
            }
            *next = FLOW_CLASSIFY_NEXT_DROP;
            counter[FLOW_CLASSIFY_COUNTER_VRIP_DROP] += 1;
            return 0;
        }
    } else {
        // IPv6 not supported
        return -1;
    }
    return -1;
}

always_inline void
pds_flow_classify_no_vnic (vlib_buffer_t *p,
                           p4_rx_cpu_hdr_t *hdr,
                           u16 *next,
                           u32 *counter)
{
    if (hdr->rx_packet && (hdr->flags & VPP_CPU_FLAGS_IPV4_2_VALID) &&
        !hdr->flow_hit) {
        /*
         * When vnic lookup fails in P4, P4/P4+ cannot do a route lookup on
         * the source ip to determine route hit with NAT rule enabled. We
         * assume such packets are invalid NAT flows (sent to server not
         * containing the NAT port block matching the dest ip)
         */
        vnet_buffer(p)->pds_flow_data.flags |= hdr->flags;
        vnet_buffer2(p)->pds_nat_data.vpc_id = hdr->vpc_id;
        /*
         * set l3_hdr_offset to offset to outer header from current
         * location. set l4_hdr_offset to offset to inner l4 header from
         * current location
         */
        vnet_buffer(p)->l3_hdr_offset = hdr->l3_offset -
                hdr->l3_inner_offset;
        vnet_buffer(p)->l4_hdr_offset = hdr->l4_inner_offset -
                hdr->l3_inner_offset;
        vlib_buffer_advance(p, VPP_P4_TO_ARM_HDR_SZ +
                hdr->l3_inner_offset - hdr->l2_offset);
        *next = FLOW_CLASSIFY_NEXT_IP4_NAT_INVAL;
        counter[FLOW_CLASSIFY_COUNTER_IP4_NAT_INVAL] += 1;
    } else {
        *next = FLOW_CLASSIFY_NEXT_DROP;
        counter[FLOW_CLASSIFY_COUNTER_VNIC_NOT_FOUND] += 1;
    }
    return;
}

always_inline void
pds_flow_l2l_packet_process (vlib_buffer_t *p,
                             p4_rx_cpu_hdr_t *hdr,
                             pds_flow_hw_ctx_t *ctx,
                             u8 flags,
                             u16 *next,
                             u32 *counter)
{
    pds_impl_db_vnic_entry_t *vnic;

    if (hdr->rx_packet) {
        if (!BIT_ISSET(flags, VPP_CPU_FLAGS_IPV4_2_VALID)) {
            *next = FLOW_CLASSIFY_NEXT_DROP;
            counter[FLOW_CLASSIFY_COUNTER_VNIC_NOT_FOUND] += 1;
            return;
        }
        vnic = pds_impl_db_vnic_get(ctx->vnic_id);
        if (PREDICT_FALSE(!vnic)) {
            *next = FLOW_CLASSIFY_NEXT_DROP;
            counter[FLOW_CLASSIFY_COUNTER_VNIC_NOT_FOUND] += 1;
            return;
        }
        BIT_SET(vnet_buffer(p)->pds_flow_data.flags,
                pds_get_l2l_cpu_flags_from_vnic(vnic));
        vnet_buffer(p)->pds_flow_data.lif = vnic->host_lif_hw_id;
        vnet_buffer(p)->pds_flow_data.l2_inner_offset =
                hdr->l2_inner_offset;
        vlib_buffer_advance(p, pds_flow_classify_get_advance_offset(p,
                            hdr->flags));
        *next = FLOW_CLASSIFY_NEXT_IP4_L2L_FLOW_PROG;
    } else {
        vnet_buffer(p)->pds_flow_data.egress_lkp_id = hdr->egress_bd_id;
        vlib_buffer_advance(p, pds_flow_classify_get_advance_offset(p,
                            hdr->flags));
        *next = FLOW_CLASSIFY_NEXT_IP4_L2L_DEF_FLOW_PROG;
    }
    counter[FLOW_CLASSIFY_COUNTER_IP4_L2L_FLOW] += 1;
    return;
}

always_inline void
pds_flow_classify_x1 (vlib_buffer_t *p, u16 *next, u32 *counter)
{
    p4_rx_cpu_hdr_t *hdr = vlib_buffer_get_current(p);
    u8 flag_orig;
    u32 nexthop;
    pds_impl_db_vnic_entry_t *vnic;
    pds_flow_hw_ctx_t *ctx = NULL;

    *next = FLOW_CLASSIFY_N_NEXT;
    flag_orig = hdr->flags;
    u8 flags = BIT_ISSET(flag_orig, VPP_CPU_FLAGS_IP_VALID);
    vnic = pds_impl_db_vnic_get(hdr->vnic_id);
    vnet_buffer(p)->pds_flow_data.ses_id = hdr->session_id;
    vnet_buffer(p)->pds_flow_data.flow_hash = hdr->flow_hash;
    BIT_SET(vnet_buffer(p)->pds_flow_data.flags,
            pds_get_cpu_flags_from_hdr(hdr));
    if (vnic) {
        BIT_SET(vnet_buffer(p)->pds_flow_data.flags,
                pds_get_cpu_flags_from_vnic(vnic));
    }
    vnet_buffer(p)->pds_flow_data.tcp_flags = hdr->tcp_flags;
    vnet_buffer(p)->pds_flow_data.epoch = hdr->epoch;
    nexthop = hdr->nexthop_id;
    // program nexthop always if there is one with priority.
    // p4 will decide if to take mapping or route.
    if (nexthop) {
        PDS_FLOW_NH_TYPE_SET(nexthop, hdr->nexthop_type);
        PDS_FLOW_NH_PRIO_SET(nexthop, hdr->route_priority);
    }
    PDS_FLOW_NH_DROP_SET(nexthop, hdr->drop);
    vnet_buffer(p)->pds_flow_data.nexthop = nexthop;

    vnet_buffer(p)->l2_hdr_offset = hdr->l2_offset;
    if (BIT_ISSET(hdr->flags, VPP_CPU_FLAGS_IPV4_2_VALID)) {
        vnet_buffer(p)->l3_hdr_offset =
                hdr->l3_inner_offset ? hdr->l3_inner_offset : hdr->l3_offset;
        vnet_buffer(p)->l4_hdr_offset =
                hdr->l4_inner_offset ? hdr->l4_inner_offset : hdr->l4_offset;
    } else {
        vnet_buffer(p)->l3_hdr_offset = hdr->l3_offset;
        vnet_buffer(p)->l4_hdr_offset = hdr->l4_offset;
    }
    vnet_buffer(p)->pds_flow_data.lif = hdr->lif;
    vnet_buffer2(p)->pds_nat_data.vpc_id = hdr->vpc_id;
    vnet_buffer2(p)->pds_nat_data.vnic_id = hdr->vnic_id;

    if (PREDICT_FALSE(pds_is_flow_session_present(p))) {
        ctx = pds_flow_get_hw_ctx(
              vnet_buffer(p)->pds_flow_data.ses_id);
        if (PREDICT_FALSE(NULL == ctx)) {
            *next = FLOW_CLASSIFY_NEXT_DROP;
            counter[FLOW_CLASSIFY_COUNTER_SES_NOT_FOUND] += 1;
            return;
        }
        if (pds_flow_packet_l2l(ctx->packet_type)) {
            pds_flow_l2l_packet_process(p, hdr, ctx, flags, next, counter);
            goto end;
        }
    }

    // drop rflow defunct packets
    // ideally these should be dropped in p4, extra check here
    if (PREDICT_FALSE(hdr->defunct_flow && hdr->flow_role)) {
        *next = FLOW_CLASSIFY_NEXT_DROP;
        counter[FLOW_CLASSIFY_COUNTER_DEFUNCT_RFLOW] += 1;
        return;
    }

    // If it's a TCP SYN packet, it should always go to FLOW_PROG node
    if (!hdr->defunct_flow && hdr->tcp_flags &&
        !(hdr->tcp_flags & TCP_FLAG_SYN)) {
        // IF flow ageing is not supported or session is not present, all other TCP
        // packets can be dropped
        if (PREDICT_FALSE(!pds_flow_age_supported() ||
                          !pds_is_flow_session_present(p))) {
            *next = FLOW_CLASSIFY_NEXT_DROP;
            counter[FLOW_CLASSIFY_COUNTER_TCP_PKT_NO_SES] += 1;
            goto end;
        }
        // If ageing is supported, all TCP packets except SYN should go to AGE_FLOW
        // node
        vlib_buffer_advance(p, (APULU_P4_TO_ARM_HDR_SZ -
                                APULU_ARM_TO_P4_HDR_SZ));
        *next = FLOW_CLASSIFY_NEXT_AGE_FLOW;
        counter[FLOW_CLASSIFY_COUNTER_TCP_PKT] += 1;
        goto end;
    }
    if (PREDICT_FALSE(!vnic)) {
        pds_flow_classify_no_vnic(p, hdr, next, counter);
        return;
    }

    vlib_buffer_advance(p, pds_flow_classify_get_advance_offset(p, flag_orig));
    if (PREDICT_FALSE(vnic->max_sessions &&
        (vnic->active_ses_count >= vnic->max_sessions))) {
        *next = FLOW_CLASSIFY_NEXT_DROP;
        counter[FLOW_CLASSIFY_COUNTER_MAX_EXCEEDED] += 1;
        goto end;
    }
    vnic->active_ses_count++;

    pds_flow_packet_type_derive(p, hdr, flags, next, counter, ctx);
    if (FLOW_CLASSIFY_N_NEXT != *next) {
        goto end;
    }

    if ((flags == VPP_CPU_FLAGS_IPV4_1_VALID)) {
        if (pds_flow_vr_ip_ping(hdr, p, vnic->nh_hw_id,
                                true, next, counter)) {
            *next = FLOW_CLASSIFY_NEXT_IP4_FLOW_PROG;
            counter[FLOW_CLASSIFY_COUNTER_IP4_FLOW] += 1;
        }
    } else if (BIT_ISSET(flags, VPP_CPU_FLAGS_IPV4_2_VALID)) {
        *next = FLOW_CLASSIFY_NEXT_IP4_TUN_FLOW_PROG;
        counter[FLOW_CLASSIFY_COUNTER_IP4_TUN_FLOW] += 1;
    } else if (PREDICT_FALSE(BIT_ISSET(flags,
               VPP_CPU_FLAGS_IPV6_VALID))) {
        *next = FLOW_CLASSIFY_NEXT_DROP;
        counter[FLOW_CLASSIFY_COUNTER_IP6_FLOW] += 1;
    } else {
        *next = FLOW_CLASSIFY_NEXT_DROP;
        counter[FLOW_CLASSIFY_COUNTER_L2_FLOW] += 1;
    }

end:
    return;
}

always_inline void
pds_flow_classify_x2 (vlib_buffer_t *p0, vlib_buffer_t *p1,
                      u16 *next0, u16 *next1, u32 *counter)
{
    pds_flow_classify_x1(p0, next0, counter);
    pds_flow_classify_x1(p1, next1, counter);
}

always_inline void
pds_flow_handle_l2l (vlib_buffer_t *p0, u8 flow_exists,
                     u8 *miss_hit, pds_flow_hw_ctx_t *ses)
{
    if (flow_exists) {
        *miss_hit = 0;
    } else {
        *miss_hit = 1;
        // store ingress bd id as in second pass we will not get this
        ses->ingress_bd =
                vnet_buffer(p0)->sw_if_index[VLIB_TX];
    }
}

always_inline void
pds_flow_rewrite_flags_init (void)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_rewrite_flags_t *rewrite_flags;

    vec_validate(fm->rewrite_flags, PDS_FLOW_PKT_TYPE_MAX);
    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2L_INTRA_SUBNET);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START);
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2L_INTER_SUBNET);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2R_INTRA_SUBNET);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START);
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2R_INTER_SUBNET);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2N_OVERLAY_ROUTE_EN);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is bridging not routing as it will come with VNI of subnet
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2N_OVERLAY_ROUTE_EN_NAPT);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_SIP_FROM_NAT << P4_REWRITE_SIP_START) |
            (P4_REWRITE_SPORT_FROM_NAT << P4_REWRITE_SPORT_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is bridging not routing as it will come with VNI of subnet
    // rewrite dmaci from nexthop, since there will be no mapping
    // lookup in rx direction
    rewrite_flags->rx_rewrite =
             (P4_REWRITE_DIP_FROM_NAT << P4_REWRITE_DIP_START) |
             (P4_REWRITE_DPORT_FROM_NAT << P4_REWRITE_DPORT_START) |
             (P4_REWRITE_DMAC_FROM_NEXTHOP << P4_REWRITE_DMAC_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2N_OVERLAY_ROUTE_EN_NAT);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_SIP_FROM_NAT << P4_REWRITE_SIP_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is bridging not routing as it will come with VNI of subnet
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DIP_FROM_NAT << P4_REWRITE_DIP_START) |
            (P4_REWRITE_DMAC_FROM_NEXTHOP << P4_REWRITE_DMAC_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2N_OVERLAY_ROUTE_DIS);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is routing as it will come with VNI of VPC
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2N_OVERLAY_ROUTE_DIS_NAPT);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_SIP_FROM_NAT << P4_REWRITE_SIP_START) |
            (P4_REWRITE_SPORT_FROM_NAT << P4_REWRITE_SPORT_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is routing as it will come with VNI of VPC.
    // rewrite dmaci from nexthop, since there will be no mapping
    // lookup in rx direction
    rewrite_flags->rx_rewrite =
             (P4_REWRITE_DIP_FROM_NAT << P4_REWRITE_DIP_START) |
             (P4_REWRITE_DPORT_FROM_NAT << P4_REWRITE_DPORT_START) |
             (P4_REWRITE_DMAC_FROM_NEXTHOP << P4_REWRITE_DMAC_START) |
             (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
             (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2N_OVERLAY_ROUTE_DIS_NAT);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_SIP_FROM_NAT << P4_REWRITE_SIP_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is routing as it will come with VNI of VPC
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DIP_FROM_NAT << P4_REWRITE_DIP_START) |
            (P4_REWRITE_DMAC_FROM_NEXTHOP << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2N_OVERLAY_ROUTE_DIS_TWICE_NAT);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_SIP_FROM_NAT << P4_REWRITE_SIP_START) |
            (P4_REWRITE_SPORT_FROM_NAT << P4_REWRITE_SPORT_START) |
            (P4_REWRITE_DIP_FROM_NAT << P4_REWRITE_DIP_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is routing as it will come with VNI of VPC
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DIP_FROM_NAT << P4_REWRITE_DIP_START) |
            (P4_REWRITE_DPORT_FROM_NAT << P4_REWRITE_DPORT_START) |
            (P4_REWRITE_SIP_FROM_NAT << P4_REWRITE_SIP_START) |
            (P4_REWRITE_DMAC_FROM_NEXTHOP << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);

    // For intra VCN traffic, don't do routing rewrites
    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_L2N_INTRA_VCN_ROUTE);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START);
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_R2L_INTRA_SUBNET);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START);
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_R2L_INTER_SUBNET);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_N2L_OVERLAY_ROUTE_EN);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is bridging not routing as it will come with VNI of subnet
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_N2L_OVERLAY_ROUTE_EN_NAT);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_SIP_FROM_NAT << P4_REWRITE_SIP_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is bridging not routing as it will come with VNI of subnet
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DIP_FROM_NAT << P4_REWRITE_DIP_START) |
            (P4_REWRITE_DMAC_FROM_NEXTHOP << P4_REWRITE_DMAC_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_N2L_OVERLAY_ROUTE_EN_SVC_NAT);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is routing as it will come with VNI of VPC
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DIP_FROM_NAT << P4_REWRITE_DIP_START) |
            (P4_REWRITE_DPORT_FROM_NAT << P4_REWRITE_DPORT_START) |
            (P4_REWRITE_DMAC_FROM_NEXTHOP << P4_REWRITE_DMAC_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_N2L_OVERLAY_ROUTE_DIS);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is routing as it will come with VNI of VPC
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_N2L_OVERLAY_ROUTE_DIS_NAT);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_SIP_FROM_NAT << P4_REWRITE_SIP_START) |
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is routing as it will come with VNI of VPC
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DIP_FROM_NAT << P4_REWRITE_DIP_START) |
            (P4_REWRITE_DMAC_FROM_NEXTHOP << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);

    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_N2L_OVERLAY_ROUTE_DIS_SVC_NAT);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_DMAC_FROM_TUNNEL << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);
    // return traffic is routing as it will come with VNI of VPC
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DIP_FROM_NAT << P4_REWRITE_DIP_START) |
            (P4_REWRITE_DPORT_FROM_NAT << P4_REWRITE_DPORT_START) |
            (P4_REWRITE_DMAC_FROM_NEXTHOP << P4_REWRITE_DMAC_START) |
            (P4_REWRITE_SMAC_FROM_VRMAC << P4_REWRITE_SMAC_START) |
            (P4_REWRITE_TTL_DEC << P4_REWRITE_TTL_START);

    // For intra VCN traffic, don't do routing rewrites
    rewrite_flags = vec_elt_at_index(fm->rewrite_flags, PDS_FLOW_N2L_INTRA_VCN_ROUTE);
    rewrite_flags->tx_rewrite =
            (P4_REWRITE_ENCAP_VXLAN << P4_REWRITE_ENCAP_START);
    rewrite_flags->rx_rewrite =
            (P4_REWRITE_DMAC_FROM_MAPPING << P4_REWRITE_DMAC_START);

    return;
}

always_inline void
pds_flow_pipeline_init (vlib_main_t *vm)
{
    pds_infra_api_reg_t params = {0};
    //vlib_node_t *flow;

    params.nacl_id = NACL_DATA_ID_FLOW_MISS_IP4_IP6;
    params.node = format(0, "pds-flow-classify");
    //flow = vlib_get_node_by_name(vlib_get_main(), (u8 *) "pds-flow-classify");
    //params.frame_queue_index = vlib_frame_queue_main_init (flow->index, 0);
    //params.handoff_thread = 0;
    params.frame_queue_index = ~0;
    params.handoff_thread = ~0;
    params.offset = 0;
    params.unreg = 0;

    if (0 != pds_register_nacl_id_to_node(&params)) {
        ASSERT(0);
    }

    icmp_echo_request_register_next_node(vm, (u8 *) "pds-vnic-l2-rewrite");

    return;
}

#endif    // __VPP_IMPL_APULU_FLOW_H__
