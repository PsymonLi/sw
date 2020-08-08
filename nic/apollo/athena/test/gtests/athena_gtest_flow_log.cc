#ifndef P4_14
#include <stdint.h>
#include <vector>
#include "nic/include/base.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/apollo/athena/api/include/pds_vnic.h"
#include "nic/apollo/athena/api/include/pds_flow_cache.h"
#include "nic/apollo/athena/api/include/pds_flow_log.h"
#include "nic/apollo/athena/api/include/pds_l2_flow_cache.h"
#include "nic/apollo/athena/api/include/pds_flow_session_info.h"
#include "nic/apollo/athena/api/include/pds_flow_session_rewrite.h"
#include <gtest/gtest.h>
#include "athena_gtest.hpp"

#define PKT_CNT 2 

static mac_addr_t  ep_smac = {0x00, 0x00, 0xde, 0xad, 0xde, 0xad};
static mac_addr_t  ep_dmac = {0x00, 0x00, 0xbe, 0xef, 0xbe, 0xef};
static mac_addr_t  substrate_smac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
static mac_addr_t  substrate_dmac = {0x00, 0x06, 0x07, 0x08, 0x09, 0x0a};
static uint16_t    substrate_vlan = 0x02;
static uint32_t    substrate_sip = 0x04030201;
static uint32_t    substrate_dip = 0x01020304;
static uint32_t    mpls1_label = 0x12345;
static uint32_t    mpls2_label = MPLS_LABEL_FLOW_LOG_L3;
static uint32_t    g_lrn_flow_cnt = 0;

static uint32_t    g_h2s_sip = 0x23362611;
static uint32_t    g_h2s_dip = 0x36232611;
static uint8_t     g_h2s_proto = 0x11;
static uint16_t    g_h2s_sport = 0x03e8;
static uint16_t    g_h2s_dport = 0x2710;

sdk_ret_t
setup_vnic_mapping (void)
{
    pds_vnic_type_t vnic_type = VNIC_TYPE_L3;
    sdk_ret_t   ret =  SDK_RET_OK;
    /* Program L3 VNIC */
    ret = (sdk_ret_t )vlan_to_vnic_map(VLAN_ID_FLOW_LOG_L3, 
                           VNIC_ID_FLOW_LOG_L3, vnic_type);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = (sdk_ret_t )mpls_label_to_vnic_map(MPLS_LABEL_FLOW_LOG_L3, 
                                 VNIC_ID_FLOW_LOG_L3, vnic_type);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    vnic_type = VNIC_TYPE_L2;
    /* Program L2 VNIC */
    ret = (sdk_ret_t )vlan_to_vnic_map(VLAN_ID_FLOW_LOG_L2, 
                           VNIC_ID_FLOW_LOG_L2, vnic_type);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = (sdk_ret_t )mpls_label_to_vnic_map(GENEVE_DST_SLOT_ID_FLOW_LOG_L2, 
                                 VNIC_ID_FLOW_LOG_L2, vnic_type);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    return SDK_RET_OK;
}

sdk_ret_t
setup_l3_flow_tables (void)
{
    sdk_ret_t       ret = SDK_RET_OK;
    mac_addr_t      host_mac;
    uint8_t         vnic_stats_mask[PDS_FLOW_STATS_MASK_LEN];
    uint32_t        s2h_session_rewrite_id = g_session_rewrite_index++;
    uint32_t        h2s_session_rewrite_id = g_session_rewrite_index++;
    uint16_t        sport = g_h2s_sport;
    uint16_t        dport = g_h2s_dport;
    pds_flow_spec_t spec;


    ret = create_s2h_session_rewrite(s2h_session_rewrite_id,
                                     (mac_addr_t*)ep_dmac, 
                                     (mac_addr_t*)ep_smac, 
                                     VLAN_ID_FLOW_LOG_L3);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    ret = create_h2s_session_rewrite_mplsoudp(h2s_session_rewrite_id,
                                              &substrate_dmac, 
                                              &substrate_smac,
                                              substrate_vlan,
                                              substrate_sip, 
                                              substrate_dip,
                                              mpls1_label, mpls2_label);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    spec.key.vnic_id = VNIC_ID_FLOW_LOG_L3;
    spec.key.key_type = KEY_TYPE_IPV4;
    memset(spec.key.ip_saddr, 0, sizeof(spec.key.ip_saddr));
    memcpy(spec.key.ip_saddr, &g_h2s_sip, sizeof(ipv4_addr_t));
    memset(spec.key.ip_daddr, 0, sizeof(spec.key.ip_daddr));
    memcpy(spec.key.ip_daddr, &g_h2s_dip, sizeof(ipv4_addr_t));
    spec.key.ip_proto = g_h2s_proto;
    spec.data.index_type = PDS_FLOW_SPEC_INDEX_SESSION;

    for (int i = 0; i < PKT_CNT; i++) {
        if (i == 0) {
            sport += 2271;
            dport +=2271;
        } else {
            sport = g_h2s_sport;
            dport = g_h2s_dport;
            sport += 2896;
            dport +=2896;
        }
        memset(&host_mac, 0, sizeof(host_mac));
        ret = create_session_info_all(g_session_index, /*conntrack_id*/0,
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

        spec.data.index = g_session_index++;
        spec.key.l4.tcp_udp.sport = sport++;
        spec.key.l4.tcp_udp.dport = dport++;
        printf("i is %d, sport %d, dport %d \n", i, spec.key.l4.tcp_udp.sport, spec.key.l4.tcp_udp.dport);
        ret =  (sdk_ret_t)pds_flow_cache_entry_create(&spec);
        if (ret != SDK_RET_OK) {
            printf("pds_flow_cache_entry_create returned error :"
                   "for i %d, err: 0x%x", i, ret);
            return ret;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
setup_flow_log_selector (uint8_t tableid)
{
    pds_flow_log_table_selector_spec_t   spec;

    if (tableid == 0) {
        spec.flow_log_table = PDS_FLOW_LOG_TABLE0;
    } else {
        spec.flow_log_table = PDS_FLOW_LOG_TABLE1;
    }

    return (sdk_ret_t) pds_flow_log_table_selector_update(&spec);
}

sdk_ret_t
athena_gtest_setup_flow_log (void)
{
    sdk_ret_t       ret = SDK_RET_OK;

    // Setup VNIC Mappings
    ret = setup_vnic_mapping();
    if (ret != SDK_RET_OK) {
        return ret;
    }
    // Setup Flow table and session info for a FLow on L3 VNIC
    ret = setup_l3_flow_tables();
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ret = setup_flow_log_selector(0);

    return ret;
}

void
flow_log_cb_fn (pds_flow_log_table_iter_cb_arg_t *arg)
{

    pds_flow_log_entry_key_t *key = NULL;
    pds_flow_log_entry_data_t *data = NULL;
    if (arg == NULL) {
        return;
    }

    printf(" Flow log index %d", arg->key.entry_idx);

    key = &arg->info.key;
    data = &arg->info.data;

    printf("SrcIP:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x "
           "DstIP:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x "
           "Dport:%u Sport:%u Proto:%u "
           "Ktype:%u VNICID:%u , disposition:%u, pkts_from_host %u"
           "pkts_from_switch:%u, start_time: %lu end_time %lu, cnt %u\n\n",
           key->ip_saddr[0], key->ip_saddr[1], key->ip_saddr[2], key->ip_saddr[3],
           key->ip_saddr[4], key->ip_saddr[5], key->ip_saddr[6], key->ip_saddr[7],
           key->ip_saddr[8], key->ip_saddr[9], key->ip_saddr[10], key->ip_saddr[11],
           key->ip_saddr[12], key->ip_saddr[13], key->ip_saddr[14], key->ip_saddr[15],
           key->ip_daddr[0], key->ip_daddr[1], key->ip_daddr[2], key->ip_daddr[3],
           key->ip_daddr[4], key->ip_daddr[5], key->ip_daddr[6], key->ip_daddr[7],
           key->ip_daddr[8], key->ip_daddr[9], key->ip_daddr[10], key->ip_daddr[11],
           key->ip_daddr[12], key->ip_daddr[13], key->ip_daddr[14], key->ip_daddr[15],
           key->dport, key->sport,
           key->proto, (uint8_t)key->key_type, key->vnic_id, key->disposition,
           data->pkt_from_host, data->pkt_from_switch, data->start_time, 
           data->last_time, ++g_lrn_flow_cnt);

    if (key->sport == g_h2s_sport+2271) {
        if (data->pkt_from_switch != 1 || data->pkt_from_host !=1) {
            ASSERT_TRUE(1==0);
        }
    } else if (key->sport == g_h2s_sport+2896) {
        if (data->pkt_from_host !=1) {
            ASSERT_TRUE(1==0);
        }
    }
     
    return;
}

sdk_ret_t
validate_flow_table (uint8_t tableid, uint32_t exp_count)
{
    pds_flow_log_table_iter_cb_arg_t  arg = {0};
    g_lrn_flow_cnt  = 0;

    printf("Inside Validate_flow_table, tableid %d \n", tableid);
    pds_flow_log_table_entry_iterate(flow_log_cb_fn, (pds_flow_log_table_t) tableid, &arg);

    if (g_lrn_flow_cnt != exp_count) {
        return SDK_RET_INVALID_ARG;
    }

    return SDK_RET_OK;
}

sdk_ret_t
clear_validate_flow_table (uint8_t table_id)
{

    if(pds_flow_log_table_reset((pds_flow_log_table_t)table_id) != PDS_RET_OK) {
        return SDK_RET_HW_PROGRAM_ERR;
    }

    return(validate_flow_table(table_id, 0));
}

/*
 * Host to Switch UDP flow: Packet to be sent
 */
static uint8_t g_snd_pkt_ipv4_udp_h2s[] = {
    0x00, 0x00, 0xde, 0xad, 0xde, 0xad, 0x00, 0x00,
    0xbe, 0xef, 0xbe, 0xef, 0x81, 0x00, 0x00, 0x10,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x11, 0xd5, 0x21, 0x23, 0x36,
    0x26, 0x11, 0x36, 0x23, 0x26, 0x11, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x3C, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79
};

static uint8_t g_rcv_pkt_ipv4_udp_h2s[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x74, 0x00, 0x00,
    0x40, 0x00, 0xff, 0x11, 0x71, 0x6f, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0xfc, 0xeb,
    0x19, 0xeb, 0x00, 0x60, 0x00, 0x00, 0x12, 0x34,
    0x50, 0x40, 0x67, 0x8a, 0x11, 0x40, 0x45, 0x00,
    0x00, 0x50, 0x00, 0x01, 0x00, 0x00, 0x40, 0x11,
    0xd5, 0x21, 0x23, 0x36, 0x26, 0x11, 0x36, 0x23,
    0x26, 0x11, 0x03, 0xe8, 0x27, 0x10, 0x00, 0x3c,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6a, 0x6c, 0x6b, 0x6d, 0x6e,
    0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7a, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6c, 0x6b,
    0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7a, 0x78, 0x79
};
static uint8_t g_snd_pkt_ipv4_udp_s2h[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x74, 0x00, 0x00,
    0x00, 0x00, 0x40, 0x11, 0x70, 0x70, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0xff, 0xcb,
    0x19, 0xeb, 0x00, 0x60, 0x00, 0x00, 0x12, 0x34,
    0x50, 0x40, 0x67, 0x8a, 0x11, 0x40, 0x45, 0x00,
    0x00, 0x50, 0x00, 0x01, 0x00, 0x00, 0x40, 0x11,
    0xd5, 0x21, 0x36, 0x23, 0x26, 0x11, 0x23, 0x36,
    0x26, 0x11, 0x27, 0x10, 0x03, 0xe8, 0x00, 0x3c,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6a, 0x6c, 0x6b, 0x6d, 0x6e,
    0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7a, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6c, 0x6b,
    0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7a, 0x78, 0x79
};

static uint8_t g_rcv_pkt_ipv4_udp_s2h[] = {
    0x00, 0x00, 0xbe, 0xef, 0xbe, 0xef, 0x00, 0x00,
    0xde, 0xad, 0xde, 0xad, 0x81, 0x00, 0x00, 0x10,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x11, 0xd5, 0x21, 0x36, 0x23,
    0x26, 0x11, 0x23, 0x36, 0x26, 0x11, 0x27, 0x10,
    0x03, 0xe8, 0x00, 0x3C, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79
};

static uint8_t g_snd_pkt_ipv4_fmiss_h2s[] = {
    0x00, 0x00, 0xde, 0xad, 0xde, 0xad, 0x00, 0x00,
    0xbe, 0xef, 0xbe, 0xef, 0x81, 0x00, 0x00, 0x10,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x11, 0xfa, 0xab, 0x13, 0x16,
    0x26, 0x10, 0x16, 0x13, 0x26, 0x10, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x3C, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79
};

static uint8_t g_snd_pkt_ipv4_udp_h2s_colli[] = {
    0x00, 0x00, 0xde, 0xad, 0xde, 0xad, 0x00, 0x00,
    0xbe, 0xef, 0xbe, 0xef, 0x81, 0x00, 0x00, 0x10,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x50, 0x00, 0x01,
    0x00, 0x00, 0x40, 0x11, 0xd5, 0x21, 0x23, 0x36,
    0x26, 0x11, 0x36, 0x23, 0x26, 0x11, 0x03, 0xE8,
    0x27, 0x10, 0x00, 0x3C, 0x00, 0x00, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
    0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x7A, 0x78, 0x79,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x6C, 0x6B, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x7A,
    0x78, 0x79
};

static uint8_t g_rcv_pkt_ipv4_udp_h2s_colli[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00, 0x45, 0x00, 0x00, 0x74, 0x00, 0x00,
    0x40, 0x00, 0xff, 0x11, 0x71, 0x6f, 0x04, 0x03,
    0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0xf5, 0xce,
    0x19, 0xeb, 0x00, 0x60, 0x00, 0x00, 0x12, 0x34,
    0x50, 0x40, 0x67, 0x8a, 0x11, 0x40, 0x45, 0x00,
    0x00, 0x50, 0x00, 0x01, 0x00, 0x00, 0x40, 0x11,
    0xd5, 0x21, 0x23, 0x36, 0x26, 0x11, 0x36, 0x23,
    0x26, 0x11, 0x03, 0xe8, 0x27, 0x10, 0x00, 0x3c,
    0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6a, 0x6c, 0x6b, 0x6d, 0x6e,
    0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7a, 0x78, 0x79, 0x61, 0x62, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6c, 0x6b,
    0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x7a, 0x78, 0x79
};


sdk_ret_t
athena_gtest_test_flow_log (uint8_t tc)
{
    sdk_ret_t ret = SDK_RET_OK;

    uint16_t  sport = g_h2s_sport+2271;
    uint16_t  dport = g_h2s_dport+2271;

    //Send traffic. 
    g_snd_pkt_ipv4_udp_h2s[39] = (sport)%256;
    g_snd_pkt_ipv4_udp_h2s[38] = (sport>>8);
    g_snd_pkt_ipv4_udp_h2s[41] = (dport)%256;
    g_snd_pkt_ipv4_udp_h2s[40] = (dport>>8);
    
    g_rcv_pkt_ipv4_udp_h2s[75] = (sport)%256;
    g_rcv_pkt_ipv4_udp_h2s[74] = (sport>>8);
    g_rcv_pkt_ipv4_udp_h2s[77] = (dport)%256;
    g_rcv_pkt_ipv4_udp_h2s[76] = (dport>>8);

    ret = send_packet("UDP-IPv4: h2s pkt", g_snd_pkt_ipv4_udp_h2s,
            sizeof(g_snd_pkt_ipv4_udp_h2s), g_h_port,
            g_rcv_pkt_ipv4_udp_h2s, 
            sizeof(g_rcv_pkt_ipv4_udp_h2s), g_s_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // Send packet in reverse direction. Same entry should be hit.
    g_snd_pkt_ipv4_udp_s2h[77] = (sport)%256;
    g_snd_pkt_ipv4_udp_s2h[76] = (sport>>8);
    g_snd_pkt_ipv4_udp_s2h[75] = (dport)%256;
    g_snd_pkt_ipv4_udp_s2h[74] = (dport>>8);
    
    g_rcv_pkt_ipv4_udp_s2h[41] = (sport)%256;
    g_rcv_pkt_ipv4_udp_s2h[40] = (sport>>8);
    g_rcv_pkt_ipv4_udp_s2h[39] = (dport)%256;
    g_rcv_pkt_ipv4_udp_s2h[38] = (dport>>8);
    ret = send_packet("UDP-IPv4: s2h pkt", g_snd_pkt_ipv4_udp_s2h,
            sizeof(g_snd_pkt_ipv4_udp_s2h), g_s_port,
            g_rcv_pkt_ipv4_udp_s2h, 
            sizeof(g_rcv_pkt_ipv4_udp_s2h), g_h_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    //Send a packet which would hit same entry in first pass.
    sport = g_h2s_sport + 2896;
    dport =g_h2s_dport + 2896;

    g_snd_pkt_ipv4_udp_h2s_colli[39] = (sport)%256;
    g_snd_pkt_ipv4_udp_h2s_colli[38] = (sport>>8);
    g_snd_pkt_ipv4_udp_h2s_colli[41] = (dport)%256;
    g_snd_pkt_ipv4_udp_h2s_colli[40] = (dport>>8);
    
    g_rcv_pkt_ipv4_udp_h2s_colli[75] = (sport)%256;
    g_rcv_pkt_ipv4_udp_h2s_colli[74] = (sport>>8);
    g_rcv_pkt_ipv4_udp_h2s_colli[77] = (dport)%256;
    g_rcv_pkt_ipv4_udp_h2s_colli[76] = (dport>>8);
 
    ret = send_packet("UDP-IPv4: h2s pkt", g_snd_pkt_ipv4_udp_h2s_colli,
            sizeof(g_snd_pkt_ipv4_udp_h2s_colli), g_h_port,
            g_rcv_pkt_ipv4_udp_h2s_colli, 
            sizeof(g_rcv_pkt_ipv4_udp_h2s_colli), g_s_port);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    //Send traffic. Flow Miss. Should not be accounted for
    ret = send_packet("UDP-IPv4: h2s flow miss pkt", g_snd_pkt_ipv4_fmiss_h2s,
            sizeof(g_snd_pkt_ipv4_fmiss_h2s), g_h_port,
            NULL, 0, 0);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    //Switch Active Table
    ret = setup_flow_log_selector(!tc);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // Read Inactive Flow table.
    ret = validate_flow_table(tc, 2);
    if (ret != SDK_RET_OK) {
        return ret;
    }

#ifdef FL_CLEAR_SUP
    ret = clear_validate_flow_table(tc);
    if (ret != SDK_RET_OK) {
        return ret;
    }
#endif
    return ret;
}
#endif
