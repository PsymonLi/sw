#ifndef P4_14
#include <stdint.h>
#include <vector>
#include "nic/include/base.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/apollo/api/include/athena/pds_vnic.h"
#include "nic/apollo/api/include/athena/pds_flow_cache.h"
#include "nic/apollo/api/include/athena/pds_l2_flow_cache.h"
#include "nic/apollo/api/include/athena/pds_flow_session_info.h"
#include "nic/apollo/api/include/athena//pds_flow_session_rewrite.h"

#include "athena_gtest.hpp"

sdk_ret_t
create_flow_v4_others (uint16_t vnic_id, ipv4_addr_t v4_addr_sip, ipv4_addr_t v4_addr_dip,
        uint8_t proto, 
        pds_flow_spec_index_type_t index_type, uint32_t index)
{
    pds_flow_spec_t             spec;


    spec.key.vnic_id = vnic_id;
    spec.key.key_type = KEY_TYPE_IPV4;
    memset(spec.key.ip_saddr, 0, sizeof(spec.key.ip_saddr));
    memcpy(spec.key.ip_saddr, &v4_addr_sip, sizeof(ipv4_addr_t));
    memset(spec.key.ip_daddr, 0, sizeof(spec.key.ip_daddr));
    memcpy(spec.key.ip_daddr, &v4_addr_dip, sizeof(ipv4_addr_t));
    spec.key.ip_proto = proto;

    spec.data.index_type = index_type;
    spec.data.index = index;

    return (sdk_ret_t)pds_flow_cache_entry_create(&spec);
}


static uint16_t    g_tcp_vnic_id = VNIC_ID_L2_CONN_TCP;
static uint32_t    g_h2s_tcp_vlan = VLAN_ID_L2_CONN_TCP;

/*
 * Normalized key for TCP flow
 */
static uint32_t    g_h2s_sip = 0x02000001;
static uint32_t    g_h2s_dip = 0xc0000201;
static uint8_t     g_h2s_proto = 0x6;
static uint16_t    g_h2s_sport = 0x03e8;
static uint16_t    g_h2s_dport = 0x2710;

/*
 * Normalized key for ICMP flow
 */
static uint8_t      g_h2s_icmp_type = 0x08;
static uint8_t      g_h2s_icmp_code = 0x0;
static uint16_t     g_h2s_icmp_identifier = 0x1234;

/*
 * Normalized IPv6 key for TCP flow
 */
static ipv6_addr_t  g_h2s_sipv6 = 
                                {
                                    0x02, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x01,
                                };
static ipv6_addr_t  g_h2s_dipv6 = 
                                {
                                    0x0c, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x02, 0x01,
                                };

/*
 * Session info rewrite - S2H
 */
static mac_addr_t  ep_smac = {0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0};
static mac_addr_t  ep_dmac = {0x00, 0x00, 0x00, 0x40, 0x08, 0x01};

/*
 * Session into rewrite - H2S
 */
static mac_addr_t  substrate_smac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
static mac_addr_t  substrate_dmac = {0x00, 0x06, 0x07, 0x08, 0x09, 0x0a};
static uint16_t    substrate_vlan = 0x02;
static uint32_t    substrate_sip = 0x04030201;
static uint32_t    substrate_dip = 0x01020304;

static uint32_t    vni = 0xa0b0c0;
static uint32_t    source_slot_id = 0x102030;
static uint32_t    destination_slot_id = GENEVE_DST_SLOT_ID_CONN_TCP;
static uint32_t    sg_id1 = 0x0;
static uint32_t    sg_id2 = 0x0;
static uint32_t    sg_id3 = 0x0;
static uint32_t    sg_id4 = 0x0;
static uint32_t    sg_id5 = 0x0;
static uint32_t    sg_id6 = 0x0;
static uint32_t    originator_physical_ip = 0x0;

static sdk_ret_t
setup_l2_flow_v4_conntrack_tcp(void)
{
    sdk_ret_t       ret = SDK_RET_OK;
    mac_addr_t      host_mac;
    uint8_t         vnic_stats_mask[PDS_FLOW_STATS_MASK_LEN];
    uint32_t        s2h_session_rewrite_id;
    uint32_t        h2s_session_rewrite_id;
    pds_vnic_type_t vnic_type = VNIC_TYPE_L2;
    uint32_t        s2h_session_rewrite_encap_id;
    uint32_t        h2s_session_rewrite_encap_id;
    pds_flow_type_t flow_type = PDS_FLOW_TYPE_TCP;
    pds_flow_state_t flow_state = UNESTABLISHED;


    // Setup VNIC Mappings
    ret = vlan_to_vnic_map(g_h2s_tcp_vlan, g_tcp_vnic_id, vnic_type);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // Setup VNIC Mappings
    ret = mpls_label_to_vnic_map(destination_slot_id, g_tcp_vnic_id, vnic_type);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    s2h_session_rewrite_id = g_session_rewrite_index++;
    s2h_session_rewrite_encap_id = s2h_session_rewrite_id;

 
   ret = create_s2h_session_rewrite_insert_ctag(s2h_session_rewrite_id,  g_h2s_tcp_vlan);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    h2s_session_rewrite_id = g_session_rewrite_index++;
    h2s_session_rewrite_encap_id = h2s_session_rewrite_id;

    ret = create_h2s_session_rewrite_geneve(h2s_session_rewrite_id,
        &substrate_dmac, &substrate_smac,
        substrate_vlan,
        substrate_sip, substrate_dip,
	vni,
        source_slot_id, destination_slot_id,
	sg_id1, sg_id2,
	sg_id3, sg_id4,
	sg_id5, sg_id6,
        originator_physical_ip);
    if (ret != SDK_RET_OK) {
        return ret;
    }


    memset(&host_mac, 0, sizeof(host_mac));
    g_session_id_v4_tcp = g_session_index;
    ret = create_session_info_all(g_session_index, /*conntrack_id*/g_conntrack_index,
                /*skip_flow_log*/ FALSE, /*host_mac*/ &host_mac,

                /*h2s_epoch_vnic*/ 0, /*h2s_epoch_vnic_id*/ 0,
                /*h2s_epoch_mapping*/0, /*h2s_epoch_mapping_id*/0,
                /*h2s_policer_bw1_id*/0, /*h2s_policer_bw2_id*/0,
                /*h2s_vnic_stats_id*/0, /*h2s_vnic_stats_mask*/ vnic_stats_mask,
                /*h2s_vnic_histogram_latency_id*/0, /*h2s_vnic_histogram_packet_len_id*/0,
                /*h2s_tcp_flags_bitmap*/0,
                /*h2s_session_rewrite_id*/ h2s_session_rewrite_id,
                /*h2s_allowed_flow_state_bitmask*/0x3FF,
                /*h2s_egress_action*/EGRESS_ACTION_NONE,

                /*s2h_epoch_vnic*/ 0, /*s2h_epoch_vnic_id*/ 0,
                /*s2h_epoch_mapping*/0, /*s2h_epoch_mapping_id*/0,
                /*s2h_policer_bw1_id*/0, /*s2h_policer_bw2_id*/0,
                /*s2h_vnic_stats_id*/0, /*s2h_vnic_stats_mask*/ vnic_stats_mask,
                /*s2h_vnic_histogram_latency_id*/0, /*s2h_vnic_histogram_packet_len_id*/0,
                /*s2h_tcp_flags_bitmap*/0,
                /*s2h_session_rewrite_id*/ s2h_session_rewrite_id,
                /*s2h_allowed_flow_state_bitmask*/0x3FF,
                /*s2h_egress_action*/EGRESS_ACTION_NONE
                );
    if (ret != SDK_RET_OK) {
        return ret;
    }

    //create conntrack entry
    g_conntrack_id_v4_tcp = g_conntrack_index;
    ret = create_conntrack_all(g_conntrack_index, flow_type, flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // Setup Normalized Flow entry
    ret = create_flow_v4_tcp_udp(g_tcp_vnic_id, g_h2s_sip, g_h2s_dip,
            g_h2s_proto, g_h2s_sport, g_h2s_dport,
            PDS_FLOW_SPEC_INDEX_SESSION, g_session_index);
    g_session_index++;
    g_conntrack_index++;
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* IPV6 flow and associated session */
    ret = create_session_info_all(g_session_index, /*conntrack_id*/g_conntrack_index,
                /*skip_flow_log*/ FALSE, /*host_mac*/ &host_mac,

                /*h2s_epoch_vnic*/ 0, /*h2s_epoch_vnic_id*/ 0,
                /*h2s_epoch_mapping*/0, /*h2s_epoch_mapping_id*/0,
                /*h2s_policer_bw1_id*/0, /*h2s_policer_bw2_id*/0,
                /*h2s_vnic_stats_id*/0, /*h2s_vnic_stats_mask*/ vnic_stats_mask,
                /*h2s_vnic_histogram_latency_id*/0, /*h2s_vnic_histogram_packet_len_id*/0,
                /*h2s_tcp_flags_bitmap*/0,
                /*h2s_session_rewrite_id*/ h2s_session_rewrite_id,
                /*h2s_allowed_flow_state_bitmask*/0,
                /*h2s_egress_action*/EGRESS_ACTION_NONE,

                /*s2h_epoch_vnic*/ 0, /*s2h_epoch_vnic_id*/ 0,
                /*s2h_epoch_mapping*/0, /*s2h_epoch_mapping_id*/0,
                /*s2h_policer_bw1_id*/0, /*s2h_policer_bw2_id*/0,
                /*s2h_vnic_stats_id*/0, /*s2h_vnic_stats_mask*/ vnic_stats_mask,
                /*s2h_vnic_histogram_latency_id*/0, /*s2h_vnic_histogram_packet_len_id*/0,
                /*s2h_tcp_flags_bitmap*/0,
                /*s2h_session_rewrite_id*/ s2h_session_rewrite_id,
                /*s2h_allowed_flow_state_bitmask*/0,
                /*s2h_egress_action*/EGRESS_ACTION_NONE
                );
    if (ret != SDK_RET_OK) {
        return ret;
    }

    //create conntrack entry
    g_conntrack_id_v6_tcp = g_conntrack_index;
    ret = create_conntrack_all(g_conntrack_index, flow_type, flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }


    ret = create_flow_v6_tcp_udp (g_tcp_vnic_id, &g_h2s_sipv6, &g_h2s_dipv6,
            g_h2s_proto, g_h2s_sport, g_h2s_dport,
            PDS_FLOW_SPEC_INDEX_CONNTRACK, g_conntrack_index);
    g_session_index++;
    g_conntrack_index++;
    if (ret != SDK_RET_OK) {
        return ret;
    }


    /* setup udp flow */

    flow_type = PDS_FLOW_TYPE_UDP;

    memset(&host_mac, 0, sizeof(host_mac));
    ret = create_session_info_all(g_session_index, /*conntrack_id*/g_conntrack_index,
                /*skip_flow_log*/ FALSE, /*host_mac*/ &host_mac,

                /*h2s_epoch_vnic*/ 0, /*h2s_epoch_vnic_id*/ 0,
                /*h2s_epoch_mapping*/0, /*h2s_epoch_mapping_id*/0,
                /*h2s_policer_bw1_id*/0, /*h2s_policer_bw2_id*/0,
                /*h2s_vnic_stats_id*/0, /*h2s_vnic_stats_mask*/ vnic_stats_mask,
                /*h2s_vnic_histogram_latency_id*/0, /*h2s_vnic_histogram_packet_len_id*/0,
                /*h2s_tcp_flags_bitmap*/0,
                /*h2s_session_rewrite_id*/ h2s_session_rewrite_id,
                /*h2s_allowed_flow_state_bitmask*/0,
                /*h2s_egress_action*/EGRESS_ACTION_NONE,

                /*s2h_epoch_vnic*/ 0, /*s2h_epoch_vnic_id*/ 0,
                /*s2h_epoch_mapping*/0, /*s2h_epoch_mapping_id*/0,
                /*s2h_policer_bw1_id*/0, /*s2h_policer_bw2_id*/0,
                /*s2h_vnic_stats_id*/0, /*s2h_vnic_stats_mask*/ vnic_stats_mask,
                /*s2h_vnic_histogram_latency_id*/0, /*s2h_vnic_histogram_packet_len_id*/0,
                /*s2h_tcp_flags_bitmap*/0,
                /*s2h_session_rewrite_id*/ s2h_session_rewrite_id,
                /*s2h_allowed_flow_state_bitmask*/0,
                /*s2h_egress_action*/EGRESS_ACTION_NONE
                );
    if (ret != SDK_RET_OK) {
        return ret;
    }

    //create conntrack entry
    g_conntrack_id_v4_udp = g_conntrack_index;
    ret = create_conntrack_all(g_conntrack_index, flow_type, flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    uint16_t   h2s_sport_udp = g_h2s_sport+1;
    uint8_t    h2s_proto_udp = 0x11;

    // Setup Normalized Flow entry
    ret = create_flow_v4_tcp_udp(g_tcp_vnic_id, g_h2s_sip, g_h2s_dip,
            h2s_proto_udp, h2s_sport_udp, g_h2s_dport,
            PDS_FLOW_SPEC_INDEX_SESSION, g_session_index);
    g_session_index++;
    g_conntrack_index++;
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* setup icmp flow */
    flow_type = PDS_FLOW_TYPE_ICMP;
    memset(&host_mac, 0, sizeof(host_mac));
    ret = create_session_info_all(g_session_index, /*conntrack_id*/g_conntrack_index,
                /*skip_flow_log*/ FALSE, /*host_mac*/ &host_mac,

                /*h2s_epoch_vnic*/ 0, /*h2s_epoch_vnic_id*/ 0,
                /*h2s_epoch_mapping*/0, /*h2s_epoch_mapping_id*/0,
                /*h2s_policer_bw1_id*/0, /*h2s_policer_bw2_id*/0,
                /*h2s_vnic_stats_id*/0, /*h2s_vnic_stats_mask*/ vnic_stats_mask,
                /*h2s_vnic_histogram_latency_id*/0, /*h2s_vnic_histogram_packet_len_id*/0,
                /*h2s_tcp_flags_bitmap*/0,
                /*h2s_session_rewrite_id*/ h2s_session_rewrite_id,
                /*h2s_allowed_flow_state_bitmask*/0,
                /*h2s_egress_action*/EGRESS_ACTION_NONE,

                /*s2h_epoch_vnic*/ 0, /*s2h_epoch_vnic_id*/ 0,
                /*s2h_epoch_mapping*/0, /*s2h_epoch_mapping_id*/0,
                /*s2h_policer_bw1_id*/0, /*s2h_policer_bw2_id*/0,
                /*s2h_vnic_stats_id*/0, /*s2h_vnic_stats_mask*/ vnic_stats_mask,
                /*s2h_vnic_histogram_latency_id*/0, /*s2h_vnic_histogram_packet_len_id*/0,
                /*s2h_tcp_flags_bitmap*/0,
                /*s2h_session_rewrite_id*/ s2h_session_rewrite_id,
                /*s2h_allowed_flow_state_bitmask*/0,
                /*s2h_egress_action*/EGRESS_ACTION_NONE
                );
    if (ret != SDK_RET_OK) {
        return ret;
    }

    //create conntrack entry
    g_conntrack_id_v4_icmp = g_conntrack_index;
    ret = create_conntrack_all(g_conntrack_index, flow_type, flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    uint8_t    h2s_proto_icmp = 0x1;

    // Setup Normalized Flow entry
    ret = create_flow_v4_icmp(g_tcp_vnic_id, g_h2s_sip, g_h2s_dip,
            h2s_proto_icmp, g_h2s_icmp_type, g_h2s_icmp_code, g_h2s_icmp_identifier,
            PDS_FLOW_SPEC_INDEX_SESSION, g_session_index);
    g_session_index++;
    g_conntrack_index++;
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* setup others flow */
    flow_type = PDS_FLOW_TYPE_OTHERS;
    memset(&host_mac, 0, sizeof(host_mac));
    ret = create_session_info_all(g_session_index, /*conntrack_id*/g_conntrack_index,
                /*skip_flow_log*/ FALSE, /*host_mac*/ &host_mac,

                /*h2s_epoch_vnic*/ 0, /*h2s_epoch_vnic_id*/ 0,
                /*h2s_epoch_mapping*/0, /*h2s_epoch_mapping_id*/0,
                /*h2s_policer_bw1_id*/0, /*h2s_policer_bw2_id*/0,
                /*h2s_vnic_stats_id*/0, /*h2s_vnic_stats_mask*/ vnic_stats_mask,
                /*h2s_vnic_histogram_latency_id*/0, /*h2s_vnic_histogram_packet_len_id*/0,
                /*h2s_tcp_flags_bitmap*/0,
                /*h2s_session_rewrite_id*/ h2s_session_rewrite_id,
                /*h2s_allowed_flow_state_bitmask*/0,
                /*h2s_egress_action*/EGRESS_ACTION_NONE,

                /*s2h_epoch_vnic*/ 0, /*s2h_epoch_vnic_id*/ 0,
                /*s2h_epoch_mapping*/0, /*s2h_epoch_mapping_id*/0,
                /*s2h_policer_bw1_id*/0, /*s2h_policer_bw2_id*/0,
                /*s2h_vnic_stats_id*/0, /*s2h_vnic_stats_mask*/ vnic_stats_mask,
                /*s2h_vnic_histogram_latency_id*/0, /*s2h_vnic_histogram_packet_len_id*/0,
                /*s2h_tcp_flags_bitmap*/0,
                /*s2h_session_rewrite_id*/ s2h_session_rewrite_id,
                /*s2h_allowed_flow_state_bitmask*/0,
                /*s2h_egress_action*/EGRESS_ACTION_NONE
                );
    if (ret != SDK_RET_OK) {
        return ret;
    }

    //create conntrack entry
    g_conntrack_id_v4_others = g_conntrack_index;
    ret = create_conntrack_all(g_conntrack_index, flow_type, flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    uint8_t    h2s_proto_others = 0x33;

    // Setup Normalized Flow entry
    ret = create_flow_v4_others(g_tcp_vnic_id, g_h2s_sip, g_h2s_dip,
            h2s_proto_others, 
            PDS_FLOW_SPEC_INDEX_SESSION, g_session_index);
    g_session_index++;
    g_conntrack_index++;
    if (ret != SDK_RET_OK) {
        return ret;
    }

   ret = create_l2_flow (g_tcp_vnic_id, ep_dmac, s2h_session_rewrite_encap_id);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = create_l2_flow (g_tcp_vnic_id, ep_smac, h2s_session_rewrite_encap_id);
    if (ret != SDK_RET_OK) {
        return ret;
    }

 
    return ret;
}

sdk_ret_t
athena_gtest_setup_l2_flows_conntrack_tcp(void)
{
    sdk_ret_t       ret = SDK_RET_OK;

    ret = setup_l2_flow_v4_conntrack_tcp();

    if (ret != SDK_RET_OK) {
        return ret;
    }

    return ret;

}

/*
 * Host to Switch TCP flow S: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_tcp_h2s_s[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0x83, 0x7D,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch TCP flow S: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_tcp_h2s_s[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x9E, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x46, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x8A, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00, 0x00, 0x40,
    0x08, 0x01, 0x08, 0x00, 0x45, 0x00, 0x00, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A,
    0x02, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x02, 0x01,
    0x03, 0xE8, 0x27, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x20, 0x00,
    0x83, 0x7D, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host TCP flow SA: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_tcp_s2h_sa[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x9E, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x46, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x8A, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x08, 0x00, 0x45, 0x00, 0x00, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A,
    0xC0, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x01,
    0x27, 0x10, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x50, 0x12, 0x20, 0x00,
    0x83, 0x6D, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host TCP flow SA: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_tcp_s2h_sa[] = {
    0x00, 0x00, 0x00, 0x40, 0x08, 0x01, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0xC0, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x01, 0x27, 0x10,
    0x03, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x12, 0x20, 0x00, 0x83, 0x6D,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host TCP flow R: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_tcp_s2h_r[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x9E, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x46, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x8A, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x08, 0x00, 0x45, 0x00, 0x00, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A,
    0xC0, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x01,
    0x27, 0x10, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x50, 0x04, 0x20, 0x00,
    0x83, 0x7B, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host TCP flow R: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_tcp_s2h_r[] = {
    0x00, 0x00, 0x00, 0x40, 0x08, 0x01, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0xC0, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x01, 0x27, 0x10,
    0x03, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x04, 0x20, 0x00, 0x83, 0x7B,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch TCP flow R: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_tcp_h2s_r[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x04, 0x20, 0x00, 0x83, 0x7B,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch TCP flow R: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_tcp_h2s_r[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x9E, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x46, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x8A, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00, 0x00, 0x40,
    0x08, 0x01, 0x08, 0x00, 0x45, 0x00, 0x00, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A,
    0x02, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x02, 0x01,
    0x03, 0xE8, 0x27, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x50, 0x04, 0x20, 0x00,
    0x83, 0x7B, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch TCP flow F: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_tcp_h2s_f[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x01, 0x20, 0x00, 0x83, 0x7E,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch TCP flow F: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_tcp_h2s_f[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x9E, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x46, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x8A, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00, 0x00, 0x40,
    0x08, 0x01, 0x08, 0x00, 0x45, 0x00, 0x00, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A,
    0x02, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x02, 0x01,
    0x03, 0xE8, 0x27, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x50, 0x01, 0x20, 0x00,
    0x83, 0x7E, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host TCP flow F: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_tcp_s2h_f[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x9E, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x46, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x8A, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x08, 0x00, 0x45, 0x00, 0x00, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A,
    0xC0, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x01,
    0x27, 0x10, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x50, 0x01, 0x20, 0x00,
    0x83, 0x7E, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host TCP flow F: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_tcp_s2h_f[] = {
    0x00, 0x00, 0x00, 0x40, 0x08, 0x01, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0xC0, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x01, 0x27, 0x10,
    0x03, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x01, 0x20, 0x00, 0x83, 0x7E,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch TCP flow SA: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_tcp_h2s_sa[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x12, 0x20, 0x00, 0x83, 0x6D,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch TCP flow SA: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_tcp_h2s_sa[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x9E, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x46, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x8A, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00, 0x00, 0x40,
    0x08, 0x01, 0x08, 0x00, 0x45, 0x00, 0x00, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A,
    0x02, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x02, 0x01,
    0x03, 0xE8, 0x27, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x50, 0x12, 0x20, 0x00,
    0x83, 0x6D, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch TCP flow A: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_tcp_h2s_a[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x10, 0x20, 0x00, 0x83, 0x6F,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch TCP flow A: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_tcp_h2s_a[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x9E, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x46, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x8A, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00, 0x00, 0x40,
    0x08, 0x01, 0x08, 0x00, 0x45, 0x00, 0x00, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A,
    0x02, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x02, 0x01,
    0x03, 0xE8, 0x27, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0x20, 0x00,
    0x83, 0x6F, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host TCP flow A: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_tcp_s2h_a[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x9E, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x46, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x8A, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x08, 0x00, 0x45, 0x00, 0x00, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A,
    0xC0, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x01,
    0x27, 0x10, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x50, 0x10, 0x20, 0x00,
    0x83, 0x6F, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host TCP flow A: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_tcp_s2h_a[] = {
    0x00, 0x00, 0x00, 0x40, 0x08, 0x01, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0xC0, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x01, 0x27, 0x10,
    0x03, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x10, 0x20, 0x00, 0x83, 0x6F,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host TCP flow S: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_tcp_s2h_s[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x9E, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x46, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x8A, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x08, 0x00, 0x45, 0x00, 0x00, 0x5C,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A,
    0xC0, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x01,
    0x27, 0x10, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x20, 0x00,
    0x83, 0x7D, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host TCP flow S: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_tcp_s2h_s[] = { 
    0x00, 0x00, 0x00, 0x40, 0x08, 0x01, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x5C, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x06, 0xB6, 0x9A, 0xC0, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x01, 0x27, 0x10,
    0x03, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x50, 0x02, 0x20, 0x00, 0x83, 0x7D,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B,
    0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
};


/*
 * Switch to Host IPV6 TCP flow SA: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv6_tcp_s2h_sa[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0xB2, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x32, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x9E, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x86, 0xDD, 0x60, 0x00, 0x00, 0x00,
    0x00, 0x48, 0x06, 0x40, 0x0C, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x27, 0x10, 0x03, 0xE8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0x12, 0x20, 0x00, 0x37, 0x6E, 0x00, 0x00,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host IPV6 TCP flow SA: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv6_tcp_s2h_sa[] = { 
    0x00, 0x00, 0x00, 0x40, 0x08, 0x01, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x81, 0x00, 0x00, 0x20,
    0x86, 0xDD, 0x60, 0x00, 0x00, 0x00, 0x00, 0x48,
    0x06, 0x40, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x27, 0x10, 0x03, 0xE8, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x12,
    0x20, 0x00, 0x37, 0x6E, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79,
};

/*
 * Switch to Host IPV6 TCP flow R: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv6_tcp_s2h_r[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0xB2, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x32, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x9E, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x86, 0xDD, 0x60, 0x00, 0x00, 0x00,
    0x00, 0x48, 0x06, 0x40, 0x0C, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x27, 0x10, 0x03, 0xE8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0x04, 0x20, 0x00, 0x37, 0x7C, 0x00, 0x00,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch IPV6 TCP flow R: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv6_tcp_h2s_r[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x20,
    0x86, 0xDD, 0x60, 0x00, 0x00, 0x00, 0x00, 0x48,
    0x06, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x01, 0x03, 0xE8, 0x27, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x04,
    0x20, 0x00, 0x37, 0x7C, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79,
};

/*
 * Switch to Host IPV6 TCP flow F: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv6_tcp_s2h_f[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0xB2, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x32, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x9E, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x86, 0xDD, 0x60, 0x00, 0x00, 0x00,
    0x00, 0x48, 0x06, 0x40, 0x0C, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x27, 0x10, 0x03, 0xE8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0x01, 0x20, 0x00, 0x37, 0x7F, 0x00, 0x00,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79,
};

/*
 * Host to Switch IPV6 TCP flow F: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv6_tcp_h2s_f[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x20,
    0x86, 0xDD, 0x60, 0x00, 0x00, 0x00, 0x00, 0x48,
    0x06, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x01, 0x03, 0xE8, 0x27, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x01,
    0x20, 0x00, 0x37, 0x7F, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79,
};


/*
 * Host to Switch UDP flow: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_udp_h2s[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0xB6, 0x9B, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x03, 0xE9,
    0x27, 0x10, 0x00, 0x3C, 0xF3, 0x43, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79,
};

/*
 * Host to Switch UDP flow: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_udp_h2s[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x92, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x52, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x7E, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00, 0x00, 0x40,
    0x08, 0x01, 0x08, 0x00, 0x45, 0x00, 0x00, 0x50,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0xB6, 0x9B,
    0x02, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x02, 0x01,
    0x03, 0xE9, 0x27, 0x10, 0x00, 0x3C, 0xF3, 0x43,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host UDP flow: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_udp_s2h[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x92, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x52, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x7E, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x08, 0x00, 0x45, 0x00, 0x00, 0x50,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0xB6, 0x9B,
    0xC0, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x01,
    0x27, 0x10, 0x03, 0xE9, 0x00, 0x3C, 0xF3, 0x43,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host UDP flow: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_udp_s2h[] = {
    0x00, 0x00, 0x00, 0x40, 0x08, 0x01, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0xB6, 0x9B, 0xC0, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x01, 0x27, 0x10,
    0x03, 0xE9, 0x00, 0x3C, 0xF3, 0x43, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79,
};


/*
 * Host to Switch ICMP flow: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_icmp_h2s[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x01, 0xB6, 0xAB, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x08, 0x00,
    0xC8, 0x94, 0x12, 0x34, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79,
};

/*
 * Host to Switch ICMP flow: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_icmp_h2s[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x92, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x52, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x7E, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00, 0x00, 0x40,
    0x08, 0x01, 0x08, 0x00, 0x45, 0x00, 0x00, 0x50,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0xB6, 0xAB,
    0x02, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x02, 0x01,
    0x08, 0x00, 0xC8, 0x94, 0x12, 0x34, 0x00, 0x00,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host ICMP flow: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_icmp_s2h[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x92, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x52, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x7E, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x08, 0x00, 0x45, 0x00, 0x00, 0x50,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0xB6, 0xAB,
    0xC0, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x01,
    0x08, 0x00, 0xC8, 0x94, 0x12, 0x34, 0x00, 0x00,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host ICMP flow: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_icmp_s2h[] = {
    0x00, 0x00, 0x00, 0x40, 0x08, 0x01, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x01, 0xB6, 0xAB, 0xC0, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x01, 0x08, 0x00,
    0xC8, 0x94, 0x12, 0x34, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79,
};

/*
 * Host to Switch OTHERS flow: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_others_h2s[] = {
    0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x48, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x33, 0xB6, 0x81, 0x02, 0x00,
    0x00, 0x01, 0xC0, 0x00, 0x02, 0x01, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79,
};

/*
 * Host to Switch OTHERS flow: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_others_h2s[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x8A, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x5A, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x76, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x00, 0x00, 0x00, 0x40,
    0x08, 0x01, 0x08, 0x00, 0x45, 0x00, 0x00, 0x48,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x33, 0xB6, 0x81,
    0x02, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x02, 0x01,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host OTHERS flow: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_others_s2h[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x8A, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x5A, 0x01, 0x02,
    0x03, 0x04, 0x04, 0x03, 0x02, 0x01, 0x00, 0x00,
    0x17, 0xC1, 0x00, 0x76, 0x00, 0x00, 0x04, 0x00,
    0x65, 0x58, 0xA0, 0xB0, 0xC0, 0x00, 0x00, 0x00,
    0x21, 0x01, 0x00, 0x10, 0x20, 0x30, 0x00, 0x00,
    0x22, 0x01, 0x00, 0x01, 0x23, 0x60, 0x00, 0x00,
    0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0xF1, 0xD0,
    0xD1, 0xD0, 0x08, 0x00, 0x45, 0x00, 0x00, 0x48,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x33, 0xB6, 0x81,
    0xC0, 0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x01,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7A, 0x78, 0x79,
};

/*
 * Switch to Host OTHERS flow: Expected Packet
 */
static uint8_t g_rcv_pkt_ipv4_others_s2h[] = {
    0x00, 0x00, 0x00, 0x40, 0x08, 0x01, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x81, 0x00, 0x00, 0x20,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x48, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x33, 0xB6, 0x81, 0xC0, 0x00,
    0x02, 0x01, 0x02, 0x00, 0x00, 0x01, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79,
};

sdk_ret_t
athena_gtest_test_l2_flows_conntrack_tcp(void)
{
    sdk_ret_t           ret = SDK_RET_OK;
    uint32_t             mask_start_pos = 38;
    pds_flow_type_t      p_flow_type;
    pds_flow_state_t     p_flow_state;
    pds_flow_type_t      e_flow_type;
    pds_flow_state_t     e_flow_state; 
    uint16_t h2s_allowed_flow_state = 0x3FF;
    uint16_t s2h_allowed_flow_state = 0x3FF;
    p_flow_type = PDS_FLOW_TYPE_TCP;
    p_flow_state = UNESTABLISHED;
    e_flow_type = PDS_FLOW_TYPE_TCP;
    e_flow_state = UNESTABLISHED;

    /* flow_state unestablished, H2S flag='S', no change expected */
    ret = send_packet_wmask("TCP-L2-IPv4: h2s pkt", g_snd_pkt_ipv4_tcp_h2s_s,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_s), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_s, sizeof(g_rcv_pkt_ipv4_tcp_h2s_s), g_s_port, mask_start_pos);

    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

   /*  REMOVED STATE  */
   /* flow_state REMOVED, H2S flag='S', expected PDS_FLOW_STATE_SYN_SENT */
    p_flow_state = REMOVED;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    /*
    ret = send_packet_wmask("TCP-L2-IPv4: h2s pkt", g_snd_pkt_ipv4_tcp_h2s_s,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_s), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_s, sizeof(g_rcv_pkt_ipv4_tcp_h2s_s), g_s_port, mask_start_pos);
    */
    ret = send_packet_wmask("TCP-L2-IPv4: REMOVED h2s pkt", g_snd_pkt_ipv4_tcp_h2s_s,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_s), g_h_port,
			    NULL, 0, 0, mask_start_pos);

    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

   /* program flow state REMOVED, S2H flag='S', expected PDS_FLOW_STATE_SYN_RECV */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*
    ret = send_packet("TCP-L2-IPv4: s2h S pkt", g_snd_pkt_ipv4_tcp_s2h_s,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_s), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_s, sizeof(g_rcv_pkt_ipv4_tcp_s2h_s), g_h_port);
    */

    ret = send_packet("TCP-L2-IPv4: REMOVED s2h S pkt", g_snd_pkt_ipv4_tcp_s2h_s,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_s), g_s_port,
			    NULL, 0, 0);

    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYN_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
 
    /*  SYN_SENT STATE  */
    /* program flow state SYN_SENT, S2H flag='SA', expected SYNACK_RECV */
    p_flow_state = PDS_FLOW_STATE_SYN_SENT;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYN_SENT s2h SA pkt", g_snd_pkt_ipv4_tcp_s2h_sa,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_sa), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_sa, sizeof(g_rcv_pkt_ipv4_tcp_s2h_sa), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYNACK_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_SENT, S2H flag='R', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYN_SENT s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_SENT, H2S flag='R', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: SYN_SENT h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /* program flow state SYN_SENT, H2S flag='F', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: SYN_SENT h2s F pkt", g_snd_pkt_ipv4_tcp_h2s_f,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_f), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_f, sizeof(g_rcv_pkt_ipv4_tcp_h2s_f), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_SENT, S2H flag='F', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYN_SENT s2h F pkt", g_snd_pkt_ipv4_tcp_s2h_f,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_f), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_f, sizeof(g_rcv_pkt_ipv4_tcp_s2h_f), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

 
    /*  SYN_RECV STATE  */
    /* program flow state SYN_RECV, S2H flag='SA', expected SYNACK_RECV */
    p_flow_state = PDS_FLOW_STATE_SYN_RECV;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: SYN_RECV h2s SA pkt", g_snd_pkt_ipv4_tcp_h2s_sa,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_sa), g_h_port,
    g_rcv_pkt_ipv4_tcp_h2s_sa, sizeof(g_rcv_pkt_ipv4_tcp_h2s_sa), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYNACK_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_RECV, S2H flag='R', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYN_RECV s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_RECV, H2S flag='R', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: SYN_RECV h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /* program flow state SYN_RECV, H2S flag='F', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: SYN_RECV h2s F pkt", g_snd_pkt_ipv4_tcp_h2s_f,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_f), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_f, sizeof(g_rcv_pkt_ipv4_tcp_h2s_f), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_RECV, S2H flag='F', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYN_RECV s2h F pkt", g_snd_pkt_ipv4_tcp_s2h_f,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_f), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_f, sizeof(g_rcv_pkt_ipv4_tcp_s2h_f), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  SYNACK_SENT STATE  */
    /* program flow state SYNACK_SENT, S2H flag='A', expected ESTABLISHED */
    p_flow_state = PDS_FLOW_STATE_SYNACK_SENT;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYNACK_SENT s2h A pkt", g_snd_pkt_ipv4_tcp_s2h_a,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_a), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_a, sizeof(g_rcv_pkt_ipv4_tcp_s2h_a), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYNACK_SENT, S2H flag='R', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYNACK_SENT s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYNACK_SENT, H2S flag='R', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: SYNACK_SENT h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /* program flow state SYNACK_SENT, H2S flag='F', expected FIN_SENT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: SYNACK_SENT h2s F pkt", g_snd_pkt_ipv4_tcp_h2s_f,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_f), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_f, sizeof(g_rcv_pkt_ipv4_tcp_h2s_f), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYNACK_SENT, S2H flag='F', expected FIN_RECV */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYNACK_SENT s2h F pkt", g_snd_pkt_ipv4_tcp_s2h_f,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_f), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_f, sizeof(g_rcv_pkt_ipv4_tcp_s2h_f), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /*  SYNACK_RECV STATE  */
    /* program flow state SYNACK_RECV, H2S flag='A', expected ESTABLISHED */
    p_flow_state = PDS_FLOW_STATE_SYNACK_RECV;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: SYNACK_RECV h2s A pkt", g_snd_pkt_ipv4_tcp_h2s_a,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_a), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_a, sizeof(g_rcv_pkt_ipv4_tcp_h2s_a), g_s_port, mask_start_pos);

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYNACK_RECV, S2H flag='R', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYNACK_RECV s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYNACK_RECV, H2S flag='R', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: SYNACK_RECV h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /* program flow state SYNACK_RECV, H2S flag='F', expected FIN_SENT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: SYNACK_RECV h2s F pkt", g_snd_pkt_ipv4_tcp_h2s_f,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_f), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_f, sizeof(g_rcv_pkt_ipv4_tcp_h2s_f), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYNACK_RECV, S2H flag='F', expected FIN_RECV */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYNACK_RECV s2h F pkt", g_snd_pkt_ipv4_tcp_s2h_f,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_f), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_f, sizeof(g_rcv_pkt_ipv4_tcp_s2h_f), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /*  ESTABLISHED STATE  */
    /* program flow state ESTABLISHED, S2H flag='R', expected RST_CLOSE */
    p_flow_state = ESTABLISHED;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: ESTABLISHED s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state ESTABLISHED, H2S flag='R', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: ESTABLISHED h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /* program flow state ESTABLISHED, H2S flag='F', expected FIN_SENT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: ESTABLISHED h2s F pkt", g_snd_pkt_ipv4_tcp_h2s_f,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_f), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_f, sizeof(g_rcv_pkt_ipv4_tcp_h2s_f), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state ESTABLISHED, S2H flag='F', expected FIN_RECV */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: ESTABLISHED s2h F pkt", g_snd_pkt_ipv4_tcp_s2h_f,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_f), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_f, sizeof(g_rcv_pkt_ipv4_tcp_s2h_f), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  FIN_SENT STATE  */
    /* program flow state FIN_SENT, S2H flag='R', expected RST_CLOSE */
    p_flow_state = FIN_SENT;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: FIN_SENT s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state FIN_SENT, H2S flag='R', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: FIN_SENT h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    

    /* program flow state FIN_SENT, S2H flag='F', expected TIME_WAIT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: FIN_SENT s2h F pkt", g_snd_pkt_ipv4_tcp_s2h_f,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_f), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_f, sizeof(g_rcv_pkt_ipv4_tcp_s2h_f), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = TIME_WAIT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /*  FIN_RECV STATE  */
    /* program flow state FIN_RECV, S2H flag='R', expected RST_CLOSE */
    p_flow_state = FIN_RECV;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: FIN_RECV s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state FIN_RECV, H2S flag='R', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: FIN_RECV h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
 

    /* program flow state FIN_SENT, H2S flag='F', expected TIME_WAIT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: FIN_SENT h2s F pkt", g_snd_pkt_ipv4_tcp_h2s_f,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_f), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_f, sizeof(g_rcv_pkt_ipv4_tcp_h2s_f), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = TIME_WAIT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  TIME_WAIT STATE  */
    /* program flow state TIME_WAIT, S2H flag='R', expected REMOVED */
    p_flow_state = TIME_WAIT;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: TIME_WAIT s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state TIME_WAIT, H2S flag='R', expected REMOVED */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: TIME_WAIT h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

   /* flow_state TIME_WAIT, H2S flag='S', expected PDS_FLOW_STATE_SYN_SENT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: TIME_WAIT h2s S pkt", g_snd_pkt_ipv4_tcp_h2s_s,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_s), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_s, sizeof(g_rcv_pkt_ipv4_tcp_h2s_s), g_s_port, mask_start_pos);

    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

   /* program flow state TIME_WAIT, S2H flag='S', expected PDS_FLOW_STATE_SYN_SENT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: TIME_WAIT s2h S pkt", g_snd_pkt_ipv4_tcp_s2h_s,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_s), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_s, sizeof(g_rcv_pkt_ipv4_tcp_s2h_s), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYN_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  RST_CLOSE STATE  */
   /* flow_state RST_CLOSE, H2S flag='S', expected PDS_FLOW_STATE_SYN_SENT */
    p_flow_state = RST_CLOSE;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: RST_CLOSE h2s pkt", g_snd_pkt_ipv4_tcp_h2s_s,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_s), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_s, sizeof(g_rcv_pkt_ipv4_tcp_h2s_s), g_s_port, mask_start_pos);

    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

   /* program flow state RST_CLOSE, S2H flag='S', expected PDS_FLOW_STATE_SYN_RECV */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: RST_CLOSE s2h S pkt", g_snd_pkt_ipv4_tcp_s2h_s,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_s), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_s, sizeof(g_rcv_pkt_ipv4_tcp_s2h_s), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYN_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

   /*  OPEN_CONN_SENT STATE  */
   /* program flow state OPEN_CONN_SENT, S2H flag='R', expected RST_CLOSE */
    p_flow_state = OPEN_CONN_SENT;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: OPEN_CONN_SENT s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state OPEN_CONN_SENT, H2S flag='R', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: OPEN_CONN_SENT h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /* program flow state OPEN_CONN_SENT, H2S flag='F', expected FIN_SENT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: OPEN_CONN_SENT h2s F pkt", g_snd_pkt_ipv4_tcp_h2s_f,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_f), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_f, sizeof(g_rcv_pkt_ipv4_tcp_h2s_f), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state OPEN_CONN_SENT, S2H flag='F', expected FIN_RECV */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: OPEN_CONN_SENT s2h F pkt", g_snd_pkt_ipv4_tcp_s2h_f,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_f), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_f, sizeof(g_rcv_pkt_ipv4_tcp_s2h_f), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state OPEN_CONN_SENT, S2H flag='A', expected ESTABLISHED */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: OPEN_CONN_SENT s2h A pkt", g_snd_pkt_ipv4_tcp_s2h_a,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_a), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_a, sizeof(g_rcv_pkt_ipv4_tcp_s2h_a), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

   /*  OPEN_CONN_RECV STATE  */
    /* program flow state OPEN_CONN_RECV, S2H flag='R', expected RST_CLOSE */
    p_flow_state = OPEN_CONN_RECV;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: OPEN_CONN_RECV s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state OPEN_CONN_RECV, H2S flag='R', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: OPEN_CONN_RECV h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /* program flow state OPEN_CONN_RECV, H2S flag='F', expected FIN_SENT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: OPEN_CONN_RECV h2s F pkt", g_snd_pkt_ipv4_tcp_h2s_f,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_f), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_f, sizeof(g_rcv_pkt_ipv4_tcp_h2s_f), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state OPEN_CONN_RECV, S2H flag='F', expected FIN_RECV */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: OPEN_CONN_RECV s2h F pkt", g_snd_pkt_ipv4_tcp_s2h_f,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_f), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_f, sizeof(g_rcv_pkt_ipv4_tcp_s2h_f), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state OPEN_CONN_RECV, H2S flag='A', expected ESTABLISHED */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: OPEN_CONN_RECV h2s A pkt", g_snd_pkt_ipv4_tcp_h2s_a,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_a), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_a, sizeof(g_rcv_pkt_ipv4_tcp_h2s_a), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* TEST  allowed_flow_state_bitmap */

    /*  SYN_SENT STATE  */
    /* program flow state SYN_SENT, allowed_flow_state, S2H flag='SA', expected PDS_FLOW_STATE_SYNACK_RECV */
    p_flow_state = PDS_FLOW_STATE_SYN_SENT;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    h2s_allowed_flow_state = 0x3FF & (~(1 << PDS_FLOW_STATE_SYN_SENT));
    s2h_allowed_flow_state = (1 << PDS_FLOW_STATE_SYN_SENT);

    ret = update_session_info_conntrack(g_session_id_v4_tcp, g_conntrack_id_v4_tcp, 
					h2s_allowed_flow_state, s2h_allowed_flow_state);

    ret = send_packet("TCP-L2-IPv4: ALLOWED, SYN_SENT s2h SA pkt", g_snd_pkt_ipv4_tcp_s2h_sa,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_sa), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_sa, sizeof(g_rcv_pkt_ipv4_tcp_s2h_sa), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYNACK_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_SENT, !allowed_flow_state,  H2S flag='R', expected  SYN_SENT*/
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: NOT ALLOWED, SYN_SENT h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    NULL, 0, 0, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /*  SYN_RECV STATE  */
    /* program flow state SYN_RECV, !allowed, h2s  flag='SA', expected SYN_RECV */
    p_flow_state = PDS_FLOW_STATE_SYN_RECV;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    h2s_allowed_flow_state = 0x3FF & (~(1 << PDS_FLOW_STATE_SYN_RECV));
    s2h_allowed_flow_state = (1 << PDS_FLOW_STATE_SYN_RECV);

    ret = update_session_info_conntrack(g_session_id_v4_tcp, g_conntrack_id_v4_tcp, 
					h2s_allowed_flow_state, s2h_allowed_flow_state);

    ret = send_packet_wmask("TCP-L2-IPv4: !ALLOWED, SYN_RECV h2s SA pkt", g_snd_pkt_ipv4_tcp_h2s_sa,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_sa), g_h_port,
    NULL, 0, 0, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYN_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_RECV, allowed, S2H flag='R', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYN_RECV, ALLOWED, s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
			    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  SYNACK_SENT STATE  */
    /* program flow state SYNACK_SENT, !allowed, S2H flag='A', expected SYNACK_SENT */
    p_flow_state = PDS_FLOW_STATE_SYNACK_SENT;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    h2s_allowed_flow_state = (1 << PDS_FLOW_STATE_SYNACK_SENT);;
    s2h_allowed_flow_state = 0x3FF & (~(1 << PDS_FLOW_STATE_SYNACK_SENT));

    ret = update_session_info_conntrack(g_session_id_v4_tcp, g_conntrack_id_v4_tcp, 
					h2s_allowed_flow_state, s2h_allowed_flow_state);

    ret = send_packet("TCP-L2-IPv4: !ALLOWED, SYNACK_SENT s2h A pkt", g_snd_pkt_ipv4_tcp_s2h_a,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_a), g_s_port,
    NULL, 0, 0);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYNACK_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYNACK_SENT, ALLOWED H2S flag='R', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: ALLOWED SYNACK_SENT h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_r, sizeof(g_rcv_pkt_ipv4_tcp_h2s_r), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /*  SYNACK_RECV STATE  */
    /* program flow state SYNACK_RECV, allowed H2S flag='A', expected ESTABLISHED */
    p_flow_state = PDS_FLOW_STATE_SYNACK_RECV;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    h2s_allowed_flow_state = (1 << PDS_FLOW_STATE_SYNACK_RECV);
    s2h_allowed_flow_state = 0x3FF & (~(1 << PDS_FLOW_STATE_SYNACK_RECV));

    ret = update_session_info_conntrack(g_session_id_v4_tcp, g_conntrack_id_v4_tcp, 
					h2s_allowed_flow_state, s2h_allowed_flow_state);

    ret = send_packet_wmask("TCP-L2-IPv4: ALLOWED SYNACK_RECV h2s A pkt", g_snd_pkt_ipv4_tcp_h2s_a,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_a), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_a, sizeof(g_rcv_pkt_ipv4_tcp_h2s_a), g_s_port, mask_start_pos);

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYNACK_RECV, !ALLOWED, S2H flag='R', expected SYNACK_RECV */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: SYNACK_RECV !ALLOWED, s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    NULL, 0, g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYNACK_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
 
    /*  ESTABLISHED STATE  */
    /* program flow state ESTABLISHED, ALLOWED S2H flag='R', expected RST_CLOSE */
    p_flow_state = ESTABLISHED;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    h2s_allowed_flow_state = 0x3FF & (~(1 << ESTABLISHED));
    s2h_allowed_flow_state = (1 << ESTABLISHED);

    ret = update_session_info_conntrack(g_session_id_v4_tcp, g_conntrack_id_v4_tcp, 
					h2s_allowed_flow_state, s2h_allowed_flow_state);

    ret = send_packet("TCP-L2-IPv4: ESTABLISHED ALLOWED s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state ESTABLISHED, !ALLOWED, H2S flag='R', expected ESTABLISHED */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: ESTABLISHED !ALLOWED, h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    NULL, 0, 0, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  FIN_SENT STATE  */
    /* program flow state FIN_SENT, ALLOWED S2H flag='R', expected RST_CLOSE */
    p_flow_state = FIN_SENT;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    h2s_allowed_flow_state = 0x3FF & (~(1 << FIN_SENT));
    s2h_allowed_flow_state = (1 << FIN_SENT);

    ret = update_session_info_conntrack(g_session_id_v4_tcp, g_conntrack_id_v4_tcp, 
					h2s_allowed_flow_state, s2h_allowed_flow_state);

    ret = send_packet("TCP-L2-IPv4: FIN_SENT ALLOWED s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state FIN_SENT, !ALLOWED H2S flag='R', expected FIN_SENT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: !ALLOWED, FIN_SENT h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    NULL, 0, g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  FIN_RECV STATE  */
    /* program flow state FIN_RECV, ALLOWED S2H flag='R', expected RST_CLOSE */
    p_flow_state = FIN_RECV;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    h2s_allowed_flow_state = 0x3FF & (~(1 << FIN_RECV));
    s2h_allowed_flow_state = (1 << FIN_RECV);

    ret = update_session_info_conntrack(g_session_id_v4_tcp, g_conntrack_id_v4_tcp, 
					h2s_allowed_flow_state, s2h_allowed_flow_state);

    ret = send_packet("TCP-L2-IPv4: FIN_RECV ALLOWED s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state FIN_RECV, !ALLOWED,  H2S flag='R', expected FIN_RECV */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: FIN_RECV !ALLOWED, h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    NULL, 0, g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = FIN_RECV;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  TIME_WAIT STATE  */
    /* program flow state TIME_WAIT, ALLOWED S2H flag='R', expected REMOVED */
    p_flow_state = TIME_WAIT;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    h2s_allowed_flow_state = 0x3FF & (~(1 << TIME_WAIT));
    s2h_allowed_flow_state = (1 << TIME_WAIT);

    ret = update_session_info_conntrack(g_session_id_v4_tcp, g_conntrack_id_v4_tcp, 
					h2s_allowed_flow_state, s2h_allowed_flow_state);

    ret = send_packet("TCP-L2-IPv4: TIME_WAIT ALLOWED s2h R pkt", g_snd_pkt_ipv4_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_r), g_s_port,
    g_rcv_pkt_ipv4_tcp_s2h_r, sizeof(g_rcv_pkt_ipv4_tcp_s2h_r), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state TIME_WAIT, !ALLLOWED H2S flag='R', expected TIME_WAIT */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv4: TIME_WAIT !ALLOWED h2s R pkt", g_snd_pkt_ipv4_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_r), g_h_port,
			    NULL, 0, g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = TIME_WAIT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  RST_CLOSE STATE  */
   /* flow_state RST_CLOSE, H2S ALLOWED flag='S', expected PDS_FLOW_STATE_SYN_SENT */
    p_flow_state = RST_CLOSE;
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    h2s_allowed_flow_state = (1 << RST_CLOSE);
    s2h_allowed_flow_state = 0x3FF & (~(1 << RST_CLOSE));

    ret = update_session_info_conntrack(g_session_id_v4_tcp, g_conntrack_id_v4_tcp, 
					h2s_allowed_flow_state, s2h_allowed_flow_state);

    ret = send_packet_wmask("TCP-L2-IPv4: RST_CLOSE ALLOWED h2s pkt", g_snd_pkt_ipv4_tcp_h2s_s,
            sizeof(g_snd_pkt_ipv4_tcp_h2s_s), g_h_port,
			    g_rcv_pkt_ipv4_tcp_h2s_s, sizeof(g_rcv_pkt_ipv4_tcp_h2s_s), g_s_port, mask_start_pos);

    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYN_SENT;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

   /* program flow state RST_CLOSE, !ALLOWED, S2H flag='S', expected RST_CLOSE */
    ret = create_conntrack_all(g_conntrack_id_v4_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv4: RST_CLOSE !ALLOWED s2h S pkt", g_snd_pkt_ipv4_tcp_s2h_s,
            sizeof(g_snd_pkt_ipv4_tcp_s2h_s), g_s_port,
			    NULL, 0, g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = RST_CLOSE;
    ret = compare_conntrack(g_conntrack_id_v4_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }


    /* IPV6 CONNTRACK ONLY */
    /*  SYN_SENT STATE  */
    /* program flow state SYN_SENT, S2H flag='SA', expected SYNACK_RECV */
    p_flow_state = PDS_FLOW_STATE_SYN_SENT;
    ret = create_conntrack_all(g_conntrack_id_v6_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv6: s2h SA pkt", g_snd_pkt_ipv6_tcp_s2h_sa,
            sizeof(g_snd_pkt_ipv6_tcp_s2h_sa), g_s_port,
			    NULL, 0, 0);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = PDS_FLOW_STATE_SYNACK_RECV;
    ret = compare_conntrack(g_conntrack_id_v6_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_SENT, S2H flag='R', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v6_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv6: s2h R pkt", g_snd_pkt_ipv6_tcp_s2h_r,
            sizeof(g_snd_pkt_ipv6_tcp_s2h_r), g_s_port,
			    NULL, 0, 0);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v6_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_SENT, H2S flag='R', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v6_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv6: h2s R pkt", g_snd_pkt_ipv6_tcp_h2s_r,
            sizeof(g_snd_pkt_ipv6_tcp_h2s_r), g_h_port,
			    NULL, 0, 0, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v6_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    
    /* program flow state SYN_SENT, H2S flag='F', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v6_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("TCP-L2-IPv6: h2s F pkt", g_snd_pkt_ipv6_tcp_h2s_f,
            sizeof(g_snd_pkt_ipv6_tcp_h2s_f), g_h_port,
			    NULL, 0, 0, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v6_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /* program flow state SYN_SENT, S2H flag='F', expected REMOVE */
    ret = create_conntrack_all(g_conntrack_id_v6_tcp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("TCP-L2-IPv6: s2h F pkt", g_snd_pkt_ipv6_tcp_s2h_f,
            sizeof(g_snd_pkt_ipv6_tcp_s2h_f), g_s_port,
			    NULL, 0, 0);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = REMOVED;
    ret = compare_conntrack(g_conntrack_id_v6_tcp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  UDP IPV4 */
    /*  OPEN_CONN_SENT  */
    /* program flow state OPEN_CONN_SENT, S2H, expected ESTABLISHED */
    p_flow_type = PDS_FLOW_TYPE_UDP;
    e_flow_type = PDS_FLOW_TYPE_UDP;
    p_flow_state = OPEN_CONN_SENT;
    ret = create_conntrack_all(g_conntrack_id_v4_udp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("UDP-L2-IPv4: s2h pkt", g_snd_pkt_ipv4_udp_s2h,
            sizeof(g_snd_pkt_ipv4_udp_s2h), g_s_port,
    g_rcv_pkt_ipv4_udp_s2h, sizeof(g_rcv_pkt_ipv4_udp_s2h), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_udp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  OPEN_CONN_RECV  */
    /* program flow state OPEN_CONN_RECV, H2S, expected ESTABLISHED */
    p_flow_state = OPEN_CONN_RECV;
    ret = create_conntrack_all(g_conntrack_id_v4_udp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("UDP-L2-IPv4: h2s pkt", g_snd_pkt_ipv4_udp_h2s,
            sizeof(g_snd_pkt_ipv4_udp_h2s), g_h_port,
			    g_rcv_pkt_ipv4_udp_h2s, sizeof(g_rcv_pkt_ipv4_udp_h2s), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_udp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
 
   /*  ICMP IPV4 */
    /*  OPEN_CONN_SENT  */
    /* program flow state OPEN_CONN_SENT, S2H, expected ESTABLISHED */
    p_flow_type = PDS_FLOW_TYPE_ICMP;
    e_flow_type = PDS_FLOW_TYPE_ICMP;
    p_flow_state = OPEN_CONN_SENT;
    ret = create_conntrack_all(g_conntrack_id_v4_icmp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("ICMP-L2-IPv4: s2h pkt", g_snd_pkt_ipv4_icmp_s2h,
            sizeof(g_snd_pkt_ipv4_icmp_s2h), g_s_port,
    g_rcv_pkt_ipv4_icmp_s2h, sizeof(g_rcv_pkt_ipv4_icmp_s2h), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_icmp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    /*  OPEN_CONN_RECV  */
    /* program flow state OPEN_CONN_RECV, H2S, expected ESTABLISHED */
    p_flow_state = OPEN_CONN_RECV;
    ret = create_conntrack_all(g_conntrack_id_v4_icmp, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("ICMP-L2-IPv4: h2s pkt", g_snd_pkt_ipv4_icmp_h2s,
            sizeof(g_snd_pkt_ipv4_icmp_h2s), g_h_port,
			    g_rcv_pkt_ipv4_icmp_h2s, sizeof(g_rcv_pkt_ipv4_icmp_h2s), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_icmp, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
 
   /*  OTHERS IPV4 */
    /*  OPEN_CONN_SENT  */
    /* program flow state OPEN_CONN_SENT, S2H, expected ESTABLISHED */
    p_flow_type = PDS_FLOW_TYPE_OTHERS;
    e_flow_type = PDS_FLOW_TYPE_OTHERS;
    p_flow_state = OPEN_CONN_SENT;
    ret = create_conntrack_all(g_conntrack_id_v4_others, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet("OTHERS-L2-IPv4: s2h pkt", g_snd_pkt_ipv4_others_s2h,
            sizeof(g_snd_pkt_ipv4_others_s2h), g_s_port,
    g_rcv_pkt_ipv4_others_s2h, sizeof(g_rcv_pkt_ipv4_others_s2h), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_others, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

 
    /*  OPEN_CONN_RECV  */
    /* program flow state OPEN_CONN_RECV, H2S, expected ESTABLISHED */
    p_flow_state = OPEN_CONN_RECV;
    ret = create_conntrack_all(g_conntrack_id_v4_others, p_flow_type, p_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = send_packet_wmask("OTHERS-L2-IPv4: h2s pkt", g_snd_pkt_ipv4_others_h2s,
            sizeof(g_snd_pkt_ipv4_others_h2s), g_h_port,
			    g_rcv_pkt_ipv4_others_h2s, sizeof(g_rcv_pkt_ipv4_others_h2s), g_s_port, mask_start_pos);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    e_flow_state = ESTABLISHED;
    ret = compare_conntrack(g_conntrack_id_v4_others, e_flow_type, e_flow_state);
    if (ret != SDK_RET_OK) {
        return ret;
    }
 

    
    return ret;
}
#endif
