//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains Athena FTE flow functionality
///
//----------------------------------------------------------------------------
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>

#include <rte_mbuf.h>
#include <rte_malloc.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_icmp.h>
#include <rte_mpls.h>

#include "nic/sdk/lib/thread/thread.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/table.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/sdk/lib/table/ftl/ftl_base.hpp"
#include "nic/sdk/lib/rte_indexer/rte_indexer.hpp"
#include "nic/sdk/lib/bitmap/bitmap.hpp"
#include "fte_athena.hpp"
#include "nic/apollo/p4/include/athena_defines.h"
#include "nic/apollo/api/include/athena/pds_init.h"
#include "nic/apollo/api/include/athena/pds_vnic.h"
#include "nic/apollo/api/include/athena/pds_flow_session_info.h"
#include "nic/apollo/api/include/athena/pds_flow_session_rewrite.h"
#include "nic/apollo/api/include/athena/pds_flow_cache.h"
#include "nic/apollo/api/include/athena/pds_l2_flow_cache.h"
#include "nic/apollo/api/include/athena/pds_flow_age.h"
#include "nic/apollo/api/include/athena/pds_dnat.h"
#include "gen/p4gen/p4/include/ftl.h"
#include "athena_test.hpp"
#include "app_test_utils.hpp"
#include "conntrack_aging.hpp"
#include "json_parser.hpp"

uint32_t num_flows_added = 0;
uint32_t attempted_flows = 0;

namespace fte_ath {

static pds_flow_expiry_fn_t aging_expiry_dflt_fn;

#define IPV4_ADDR_LEN 4
#define IPV6_ADDR_LEN 16

// Flow direction bitmask
#define HOST_TO_SWITCH 0x1
#define SWITCH_TO_HOST 0x2

static rte_indexer *g_conntrack_indexer;
static rte_indexer *g_session_indexer;
uint32_t g_session_rewrite_index = 1;

// H2S specific fields
uint32_t g_h2s_vlan = 0x0002;
uint16_t g_h2s_vnic_id = 0x0001;
uint8_t  idx = 0;

// H2S Session info rewrite
mac_addr_t substrate_smac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
mac_addr_t substrate_dmac = {0x00, 0x06, 0x07, 0x08, 0x09, 0x0a};
uint16_t substrate_vlan = 0x02;
uint32_t substrate_sip = 0x04030201;
uint32_t substrate_dip = 0x01020304;
uint8_t substrate_ip_ttl = 64;
uint16_t substrate_udp_sport = 0xabcd;
uint16_t substrate_udp_dport = 0x1234;
uint32_t mpls1_label = 0x12345;
uint32_t mpls2_label = 0x6789a;

// S2H specific fields
uint32_t g_s2h_mpls1_label = 0x12345;
uint32_t g_s2h_mpls2_label = 0x6789a;

// S2H Session info rewrite
mac_addr_t ep_smac = {0x00, 0x00, 0xF1, 0xD0, 0xD1, 0xD0};
mac_addr_t ep_dmac = {0x00, 0x00, 0x00, 0x40, 0x08, 0x01};
uint16_t  vnic_vlan = 0x01;

// Headers used for Packet Rewrite
// Hardcoded based on rewrite table entries
uint8_t h2s_l2vlan_encap_hdr[] = {
    0x00, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02,
    0x08, 0x00
};

// Note: Total length & Checksum to be updated accordingly
uint8_t h2s_ip_encap_hdr[] = {
    0x45, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x11, 0xA2, 0xA0, 0x04, 0x03, 0x02, 0x01,
    0x01, 0x02, 0x03, 0x04
};

// Note: Length to be updated accordingly
uint8_t h2s_udp_encap_hdr[] = {
    0xE4, 0xE7, 0x19, 0xEB, 0x00, 0x60, 0x00, 0x00
};

// Two MPLS headers
uint8_t h2s_mpls_encap_hdrs[] = {
    0x12, 0x34, 0x50, 0x00,
    0x67, 0x89, 0xA1, 0x00
};

// Geneve header
uint8_t h2s_geneve_encap_hdr[] = {
    0x04, 0x00, 0x65, 0x58, 0xAA, 0xBB, 0xCC, 0x00
};

// Geneve Options: src_slot_id & dst_slot_id
uint8_t h2s_geneve_options[] = {
    0x00, 0x00, 0x21, 0x01, 0x00, 0x11, 0x22, 0x33,
    0x00, 0x00, 0x22, 0x01, 0x00, 0x01, 0x23, 0x4A
};

uint8_t s2h_l2vlan_encap_hdr[] = {
    0x00, 0x00, 0x00, 0x40, 0x08, 0x01, 0x00, 0x00,
    0xF1, 0xD0, 0xD1, 0xD0, 0x81, 0x00, 0x00, 0x01,
    0x08, 0x00
};

sdk_ret_t
fte_conntrack_indexer_init (void)
{
    g_conntrack_indexer = rte_indexer::factory(PDS_CONNTRACK_ID_MAX,
                                               true, true);
    if (g_conntrack_indexer == NULL) {
        PDS_TRACE_DEBUG("g_conntrack_indexer init failed.\n");
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

void
fte_conntrack_indexer_destroy (void)
{
    rte_indexer::destroy(g_conntrack_indexer);
}

sdk_ret_t
fte_conntrack_index_alloc (uint32_t *conntrack_id)
{
    return g_conntrack_indexer->alloc(conntrack_id);
}

sdk_ret_t
fte_conntrack_index_free (uint32_t conntrack_id)
{
    return g_conntrack_indexer->free(conntrack_id);
}

static inline uint8_t
fte_is_conntrack_enabled (uint16_t vnic_id)
{
    return (g_flow_cache_policy[vnic_id].conntrack);
}
static inline bool
fte_skip_flow_log (uint16_t vnic_id) 
{
    return g_flow_cache_policy[vnic_id].skip_flow_log;
}

static pds_egress_action_t
fte_get_egress_action (uint16_t vnic_id, uint8_t dir)
{
    uint8_t             json_egress_action  = 0;
    pds_egress_action_t egress_action; 

    if (dir == SWITCH_TO_HOST) {
        json_egress_action = g_flow_cache_policy[vnic_id].to_host.egress_action;
        egress_action = EGRESS_ACTION_TX_TO_HOST; 
    } else if (dir == HOST_TO_SWITCH) {
        json_egress_action = g_flow_cache_policy[vnic_id].to_switch.egress_action;
        egress_action = EGRESS_ACTION_TX_TO_SWITCH; 
    } else {
        //Asummming to Host as default
        egress_action = EGRESS_ACTION_TX_TO_HOST;
    }

    if (json_egress_action == EGR_ACTION_DROP) {
        return EGRESS_ACTION_DROP;
    } else if (json_egress_action == EGR_ACTION_DROP_BY_SL) {
        return EGRESS_ACTION_DROP_BY_SL;
    }

    return egress_action;
} 

sdk_ret_t
fte_session_indexer_init (void)
{
    g_session_indexer = rte_indexer::factory(PDS_FLOW_SESSION_INFO_ID_MAX,
                                             true, true);
    if (g_session_indexer == NULL) {
        PDS_TRACE_DEBUG("g_session_indexer init failed.\n");
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

void
fte_session_indexer_destroy (void)
{
    rte_indexer::destroy(g_session_indexer);
}

static inline uint8_t
fte_is_vnic_type_l2 (uint16_t vnic_id)
{
    return (g_flow_cache_policy[vnic_id].vnic_type == VNIC_L2);
}

void
fte_l2_flow_range_bmp_destroy (void)
{
    l2_flows_range_info_t *flow_range;
    uint16_t vnic_id, i;

    for (i = 0; i < g_num_policies; i++) {
        vnic_id = g_vnic_id_list[i];
        if (!fte_is_vnic_type_l2(vnic_id)) {
            continue;
        }

        flow_range = &(g_flow_cache_policy[vnic_id].l2_flows_range);
        rte_bitmap_free(flow_range->h2s_bmp);
        rte_free((void *)flow_range->h2s_bmp);
        rte_bitmap_free(flow_range->s2h_bmp);
        rte_free((void *)flow_range->s2h_bmp);
    }

    return;
}

sdk_ret_t
fte_session_index_alloc (uint32_t *sess_id)
{
    return g_session_indexer->alloc(sess_id);
}

sdk_ret_t
fte_session_index_free (uint32_t sess_id)
{
    return g_session_indexer->free(sess_id);
}

// NAT MAP TABLE APIs
static sdk_ret_t
fte_nmt_get_local_ip (uint16_t vnic_id, uint32_t nat_ip, uint32_t *local_ip)
{
    uint8_t num_nat_mappings;
    nat_map_tbl_t *nat_map_tbl;
    int i = 0;

    num_nat_mappings = g_flow_cache_policy[vnic_id].num_nat_mappings;
    nat_map_tbl = g_flow_cache_policy[vnic_id].nat_map_tbl;

    while (i < num_nat_mappings) {
        if (nat_ip == nat_map_tbl[i].nat_ip) {
            *local_ip = nat_map_tbl[i].local_ip;
            return SDK_RET_OK;
        }
        i++;
    }

    return SDK_RET_ENTRY_NOT_FOUND;
}

static sdk_ret_t
fte_nmt_get_nat_ip (uint16_t vnic_id, uint32_t local_ip, uint32_t *nat_ip)
{
    uint8_t num_nat_mappings;
    nat_map_tbl_t *nat_map_tbl;
    int i = 0;

    num_nat_mappings = g_flow_cache_policy[vnic_id].num_nat_mappings;
    nat_map_tbl = g_flow_cache_policy[vnic_id].nat_map_tbl;

    while (i < num_nat_mappings) {
        if (local_ip == nat_map_tbl[i].local_ip) {
            *nat_ip = nat_map_tbl[i].nat_ip;
            return SDK_RET_OK;
        }
        i++;
    }

    return SDK_RET_ENTRY_NOT_FOUND;
}

static sdk_ret_t
fte_nmt_get_rewrite_id (uint16_t vnic_id, uint32_t local_ip,
                        uint32_t *h2s_rewrite_id,
                        uint32_t *s2h_rewrite_id)
{
    uint8_t num_nat_mappings;
    nat_map_tbl_t *nat_map_tbl;
    int i = 0;

    num_nat_mappings = g_flow_cache_policy[vnic_id].num_nat_mappings;
    nat_map_tbl = g_flow_cache_policy[vnic_id].nat_map_tbl;

    while (i < num_nat_mappings) {
        if (local_ip == nat_map_tbl[i].local_ip) {
            *h2s_rewrite_id = nat_map_tbl[i].h2s_rewrite_id;
            *s2h_rewrite_id = nat_map_tbl[i].s2h_rewrite_id;
            return SDK_RET_OK;
        }
        i++;
    }

    return SDK_RET_ENTRY_NOT_FOUND;
}

static void
fte_nat_csum_adj_h2s_v4 (struct ipv4_hdr *iph, uint32_t new_ipaddr)
{
    uint8_t *oip, *nip, *ipcsum, *l4csum;
    uint16_t oip_sum, nip_sum;
    int32_t tmp_ipcsum = 0, tmp_l4csum = 0;
    uint8_t ip_hlen = 0, l4csum_adj = 0;
    uint8_t ip_proto;
    struct tcp_hdr *tcph;
    struct udp_hdr *udph;
    uint8_t i;

    ipcsum = (uint8_t *) &(iph->hdr_checksum);
    tmp_ipcsum = ((ipcsum[0] << 8) + ipcsum[1]);
    tmp_ipcsum = ((~tmp_ipcsum) & 0xffff);

    // oip is in BE form
    oip = (uint8_t *) &(iph->src_addr);
    for (i = 0; i < 2; i++) {
        oip_sum = ((oip[0] << 8) + oip[1]);
        oip += 2;

        tmp_ipcsum -= oip_sum;
        if (tmp_ipcsum <= 0) {
            tmp_ipcsum--;
            tmp_ipcsum &= 0xffff;
        }
    }

    // nip is in LE form
    nip = (uint8_t *) &new_ipaddr; 
    for (i = 0; i < 2; i++) {
        nip_sum = ((nip[1] << 8) + nip[0]);
        nip += 2;

        tmp_ipcsum += nip_sum;
        if (tmp_ipcsum & 0x10000) {
            tmp_ipcsum++;
            tmp_ipcsum &= 0xffff;
        }
    }

    tmp_ipcsum = (~tmp_ipcsum & 0xffff);
    ipcsum[0] = (tmp_ipcsum >> 8);
    ipcsum[1] = (tmp_ipcsum & 0xff);

    ip_proto = iph->next_proto_id;
    if ((ip_proto != IP_PROTO_TCP) &&
        (ip_proto != IP_PROTO_UDP)) {
        return;
    }

    ip_hlen = ((iph->version_ihl & IPV4_HDR_IHL_MASK) *
               IPV4_IHL_MULTIPLIER);
    tcph = (struct tcp_hdr *)((uint8_t *)iph + ip_hlen);
    if (ip_proto == IP_PROTO_TCP) {
        l4csum = (uint8_t *) &(tcph->cksum);
        l4csum_adj = 1;
    } else {
        udph = (struct udp_hdr *)tcph;
        if (udph->dgram_cksum) {
            l4csum = (uint8_t *) &(udph->dgram_cksum);
            l4csum_adj = 1;
        }
    }

    // TCP/UDP csum adjustment
    if (l4csum_adj) {
        tmp_l4csum = ((l4csum[0] << 8) + l4csum[1]);
        tmp_l4csum = ((~tmp_l4csum) & 0xffff);

        // oip is in BE form
        oip = (uint8_t *) &(iph->src_addr);
        for (i = 0; i < 2; i++) {
            oip_sum = ((oip[0] << 8) + oip[1]);
            oip += 2;

            tmp_l4csum -= oip_sum;
            if (tmp_l4csum <= 0) {
                tmp_l4csum--;
                tmp_l4csum &= 0xffff;
            }
        }

        // nip is in LE form
        nip = (uint8_t *) &new_ipaddr;
        for (i = 0; i < 2; i++) {
            nip_sum = ((nip[1] << 8) + nip[0]);
            nip += 2;

            tmp_l4csum += nip_sum;
            if (tmp_l4csum & 0x10000) {
                tmp_l4csum++;
                tmp_l4csum &= 0xffff;
            }
        }

        tmp_l4csum = (~tmp_l4csum & 0xffff);
        l4csum[0] = (tmp_l4csum >> 8);
        l4csum[1] = (tmp_l4csum & 0xff);
    }

    return;        
}

static sdk_ret_t
fte_flow_extract_prog_args (struct rte_mbuf *m, pds_flow_spec_t *spec,
                            uint8_t *dir, uint16_t *ip_off,
                            uint16_t *vnic_id, uint8_t *tcp_flags)
{
    struct ether_hdr *eth0;
    struct ipv4_hdr *ip40;
    struct tcp_hdr *tcp0;
    struct udp_hdr *udp0;
    struct mpls_hdr *mpls_dst;
    struct geneve_hdr *genh;
    struct geneve_options *gen_opt;
    uint16_t ip0_offset = 0;
    uint16_t vlan_id = 0;
    uint32_t mpls_label = 0;
    uint8_t protocol = 0;
    uint16_t sport = 0, dport = 0;
    uint8_t mplsoudp = 0, geneve = 0;
    pds_flow_key_t *key = &(spec->key);

    // mbuf data starts at eth header
    eth0 = rte_pktmbuf_mtod(m, struct ether_hdr *);
    ip0_offset += sizeof(struct ether_hdr);

    if ((rte_be_to_cpu_16(eth0->ether_type) != ETHER_TYPE_VLAN) &&
        (rte_be_to_cpu_16(eth0->ether_type) != ETHER_TYPE_IPv4) &&
        (rte_be_to_cpu_16(eth0->ether_type) != ETHER_TYPE_IPv6)) {
        PDS_TRACE_DEBUG("Unsupported ether_type:0x%x \n",
                        rte_be_to_cpu_16(eth0->ether_type));
        return SDK_RET_INVALID_OP;
    }

    if ((rte_be_to_cpu_16(eth0->ether_type) == ETHER_TYPE_VLAN)) {
        struct vlan_hdr *vh = (struct vlan_hdr *)(eth0 + 1);

        if ((rte_be_to_cpu_16(vh->eth_proto) != ETHER_TYPE_IPv4) &&
            (rte_be_to_cpu_16(vh->eth_proto) != ETHER_TYPE_IPv6)) {
            PDS_TRACE_DEBUG("Unsupported VLAN eth_proto:0x%x \n",
                            rte_be_to_cpu_16(vh->eth_proto));
            return SDK_RET_INVALID_OP;
        }

        ip0_offset += (sizeof(struct vlan_hdr));
        vlan_id = (rte_be_to_cpu_16(vh->vlan_tci) & 0x0fff);
    }

    ip40 = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, ip0_offset);
    udp0 = (struct udp_hdr *)(ip40 + 1);

    if (((ip40->version_ihl >> 4) == 4) &&
        (ip40->next_proto_id == IP_PROTO_UDP)) {
        dport = rte_be_to_cpu_16(udp0->dst_port);
        if ((mplsoudp = (dport == 0x19EB)) ||
            (geneve = (dport == 0x17C1))) {
            ip0_offset += (sizeof(struct ipv4_hdr));
            ip0_offset += (sizeof(struct udp_hdr));
        }
    }

    if (mplsoudp) {
        mpls_dst = (struct mpls_hdr *)(udp0 + 1); /* 1st label */
        ip0_offset += (sizeof(struct mpls_hdr));

        if (mpls_dst->bs == 0) { 
            mpls_dst = (struct mpls_hdr *) (mpls_dst + 1);  /* 2nd label */
            ip0_offset += (sizeof(struct mpls_hdr));

            if (mpls_dst->bs == 0) {
                struct mpls_hdr *mpls3;

                mpls3 = (struct mpls_hdr *) (mpls_dst + 1);  /* 3rd label */
                ip0_offset += (sizeof(struct mpls_hdr));

                if (mpls3->bs == 0) {
                    PDS_TRACE_DEBUG("Unsupported: MPLS lables > 3.");
                    return SDK_RET_INVALID_OP;
                }
            }
        }
        mpls_label = ((rte_be_to_cpu_16(mpls_dst->tag_msb) << 4) |
                            mpls_dst->tag_lsb);
    } else if (geneve) {
        genh = (struct geneve_hdr *)(udp0 + 1);
        ip0_offset += (sizeof(struct geneve_hdr));

        // FIXME: Only src_slot_id & dst_slot_id options should be present
        gen_opt = (struct geneve_options *)(genh + 1);
        gen_opt += 1;
        ip0_offset += (sizeof(struct geneve_options));

        mpls_label = rte_be_to_cpu_32(gen_opt->opt_data);
        ip0_offset += (sizeof(struct geneve_options));

        // L2 encapsulation
        if (rte_be_to_cpu_16(genh->proto_type) == 0x6558) {
            ip0_offset += (sizeof(struct ether_hdr));
        }
    }

    *ip_off = ip0_offset;
    if (mpls_label) {
        *dir = SWITCH_TO_HOST;
        *vnic_id = g_mpls_label_to_vnic[mpls_label];
    } else {
        *dir = HOST_TO_SWITCH;
        *vnic_id = g_vlan_to_vnic[vlan_id];
    }

    if (*vnic_id == 0) {
        PDS_TRACE_DEBUG("vnic_id lookup failed.\n");
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    key->vnic_id = *vnic_id;

    ip40 = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, ip0_offset);
    if ((ip40->version_ihl >> 4) == 4) {
        uint32_t src_ip, dst_ip;
        uint32_t dnat_local_ip;

        protocol = ip40->next_proto_id;
        if ((protocol != IP_PROTO_TCP) &&
            (protocol != IP_PROTO_UDP) && 
            (protocol != IP_PROTO_ICMP)) {
            PDS_TRACE_DEBUG("Unsupported IP Proto:%u\n", protocol);
            return SDK_RET_INVALID_OP;
        }
        
        src_ip = rte_be_to_cpu_32(ip40->src_addr);
        dst_ip = rte_be_to_cpu_32(ip40->dst_addr);
        key->key_type = KEY_TYPE_IPV4;
        if (*dir == HOST_TO_SWITCH) {
            memcpy(key->ip_saddr, &src_ip, IPV4_ADDR_LEN);
            memcpy(key->ip_daddr, &dst_ip, IPV4_ADDR_LEN);
        }
        else {
            if (g_flow_cache_policy[*vnic_id].nat_enabled) {
                if (fte_nmt_get_local_ip(*vnic_id, dst_ip,
                                         &dnat_local_ip) == SDK_RET_OK) {
                    dst_ip = dnat_local_ip;
                }
            }
            memcpy(key->ip_saddr, &dst_ip, IPV4_ADDR_LEN);
            memcpy(key->ip_daddr, &src_ip, IPV4_ADDR_LEN);
        }

        tcp0 = (struct tcp_hdr *) (((uint8_t *) ip40) +
                ((ip40->version_ihl & IPV4_HDR_IHL_MASK) *
                IPV4_IHL_MULTIPLIER));
    } else {
        struct ipv6_hdr *ip60 = (struct ipv6_hdr *)ip40;

        protocol = ip60->proto;
        if ((protocol != IP_PROTO_TCP) &&
            (protocol != IP_PROTO_UDP) && 
            (protocol != IP_PROTO_ICMPV6)) {
            PDS_TRACE_DEBUG("Unsupported IPV6 Proto:%u\n", protocol);
            return SDK_RET_INVALID_OP;
        }

        key->key_type = KEY_TYPE_IPV6;
        if (*dir == HOST_TO_SWITCH) {
            sdk::lib::memrev(key->ip_saddr, ip60->src_addr, IPV6_ADDR_LEN);
            sdk::lib::memrev(key->ip_daddr, ip60->dst_addr, IPV6_ADDR_LEN);
        } else {
            sdk::lib::memrev(key->ip_saddr, ip60->dst_addr, IPV6_ADDR_LEN);
            sdk::lib::memrev(key->ip_daddr, ip60->src_addr, IPV6_ADDR_LEN);
        }

        tcp0 = (struct tcp_hdr *) (((uint8_t *) ip60) +
                    IPV6_BASE_HDR_SIZE);
    }

    key->ip_proto = protocol;
    if ((protocol == IP_PROTO_ICMP) ||
        (protocol == IP_PROTO_ICMPV6)) {
        struct icmp_hdr *icmph = ((struct icmp_hdr *)tcp0);

        key->l4.icmp.type = icmph->icmp_type;
        key->l4.icmp.code = icmph->icmp_code;
        key->l4.icmp.identifier = rte_be_to_cpu_16(icmph->icmp_ident);
    } else {
        sport = rte_be_to_cpu_16(tcp0->src_port);
        dport = rte_be_to_cpu_16(tcp0->dst_port);
        if (*dir == HOST_TO_SWITCH) {
            key->l4.tcp_udp.sport = sport;
            key->l4.tcp_udp.dport = dport;
        } else {
            key->l4.tcp_udp.sport = dport;
            key->l4.tcp_udp.dport = sport;
        }
        if (protocol == IP_PROTO_TCP) {
            *tcp_flags = tcp0->tcp_flags;
        }
    }

    return SDK_RET_OK;
}

static sdk_ret_t
fte_conntrack_state_create (uint32_t conntrack_index,
                            pds_flow_spec_t *flow_spec,
                            uint8_t flow_dir, uint8_t tcp_flags)
{
    sdk_ret_t ret = SDK_RET_OK;
    pds_conntrack_spec_t conn_spec;

    conn_spec.key.conntrack_id = conntrack_index;

    if (flow_spec->key.ip_proto == IP_PROTO_TCP) {
        conn_spec.data.flow_type = PDS_FLOW_TYPE_TCP;
        if (tcp_flags & TCP_FLAG_SYN) {
            if (flow_dir == HOST_TO_SWITCH) {
                conn_spec.data.flow_state = PDS_FLOW_STATE_SYN_SENT;
            } else {
                conn_spec.data.flow_state = PDS_FLOW_STATE_SYN_RECV;
            }
        } else {
            // TCP connection pickup not supported.
            return SDK_RET_INVALID_OP;
        }
    } else {
        if (flow_spec->key.ip_proto == IP_PROTO_UDP) {
            conn_spec.data.flow_type = PDS_FLOW_TYPE_UDP;
        } else if ((flow_spec->key.ip_proto == IP_PROTO_ICMP) ||
                   (flow_spec->key.ip_proto == IP_PROTO_ICMPV6)) {
            conn_spec.data.flow_type = PDS_FLOW_TYPE_ICMP;
        } else {
            conn_spec.data.flow_type = PDS_FLOW_TYPE_OTHERS;
        }

        if (flow_dir == HOST_TO_SWITCH) {
            conn_spec.data.flow_state = OPEN_CONN_SENT;
        } else {
            conn_spec.data.flow_state = OPEN_CONN_RECV;
        }
    }

    ret = (sdk_ret_t)pds_conntrack_state_create(&conn_spec);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_DEBUG("pds_conntrack_state_create failed. \n");
        return ret;
    }

    return SDK_RET_OK;
}

static sdk_ret_t
fte_conntrack_state_delete (uint32_t conntrack_index)
{
    pds_conntrack_key_t conn_key;

    memset(&conn_key, 0, sizeof(conn_key));
    conn_key.conntrack_id = conntrack_index;
    return (sdk_ret_t)pds_conntrack_state_delete(&conn_key);
}

static inline void
fte_conntrack_free (uint16_t vnic_id, uint32_t conntrack_index)
{
    if (fte_is_conntrack_enabled(vnic_id)) {
        fte_conntrack_state_delete(conntrack_index);
        fte_conntrack_index_free(conntrack_index);
    }
}

static void
fte_flow_h2s_rewrite_geneve (struct rte_mbuf *m,
                             uint16_t ip_offset, uint16_t vnic_id,
                             struct ether_hdr **l2_flow_eth_hdr)
{
    uint16_t mbuf_prepend_len;
    uint8_t *pkt_start;
    struct ether_hdr *etherh;
    struct vlan_hdr *vlanh;
    struct ipv4_hdr *ip4h;
    struct udp_hdr *udph;
    struct geneve_hdr *genh;
    struct geneve_options *gen_opt;
    uint16_t ip_tot_len, udp_len;
    rewrite_underlay_info_t *rewrite_underlay;
    uint32_t nat_ip;

    // SNAT
    if (g_flow_cache_policy[vnic_id].nat_enabled) {
        ip4h = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, ip_offset);
        if (fte_nmt_get_nat_ip(vnic_id, rte_be_to_cpu_32(ip4h->src_addr),
                               &nat_ip) == SDK_RET_OK) {
            fte_nat_csum_adj_h2s_v4(ip4h, nat_ip);
            ip4h->src_addr = rte_cpu_to_be_32(nat_ip);
        }
    }

    // Strip VLAN tag
    etherh = rte_pktmbuf_mtod(m, struct ether_hdr *);
    memmove(rte_pktmbuf_adj(m, sizeof(struct vlan_hdr)), etherh,
                            (2 * ETHER_ADDR_LEN));
    *l2_flow_eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);

    rewrite_underlay = &(g_flow_cache_policy[vnic_id].rewrite_underlay);
    mbuf_prepend_len = (sizeof(h2s_l2vlan_encap_hdr) +
                        sizeof(h2s_ip_encap_hdr) +
                        sizeof(h2s_udp_encap_hdr) +
                        sizeof(h2s_geneve_encap_hdr) +
                        sizeof(h2s_geneve_options));

    pkt_start = (uint8_t *)rte_pktmbuf_prepend(m, mbuf_prepend_len);
    etherh = (struct ether_hdr *)pkt_start;
    vlanh = (struct vlan_hdr *)(etherh + 1);

    memcpy(pkt_start, h2s_l2vlan_encap_hdr,
           sizeof(h2s_l2vlan_encap_hdr));
    memcpy(etherh->d_addr.addr_bytes,
           rewrite_underlay->substrate_dmac, ETHER_ADDR_LEN);
    memcpy(etherh->s_addr.addr_bytes,
           rewrite_underlay->substrate_smac, ETHER_ADDR_LEN);
    vlanh->vlan_tci = (rte_be_to_cpu_16(vlanh->vlan_tci) & 0xf000);
    vlanh->vlan_tci |= (rewrite_underlay->substrate_vlan & 0x0fff);
    vlanh->vlan_tci = rte_cpu_to_be_16(vlanh->vlan_tci);
    pkt_start += sizeof(h2s_l2vlan_encap_hdr);

    memcpy(pkt_start, h2s_ip_encap_hdr, sizeof(h2s_ip_encap_hdr));
    ip4h = (struct ipv4_hdr *)pkt_start;
    ip_tot_len = (m->pkt_len - sizeof(h2s_l2vlan_encap_hdr));
    ip4h->total_length = rte_cpu_to_be_16(ip_tot_len);
    ip4h->src_addr = rte_cpu_to_be_32(rewrite_underlay->substrate_sip);
    ip4h->dst_addr = rte_cpu_to_be_32(rewrite_underlay->substrate_dip);
    pkt_start += sizeof(h2s_ip_encap_hdr);

    memcpy(pkt_start, h2s_udp_encap_hdr, sizeof(h2s_udp_encap_hdr));
    udph = (struct udp_hdr *)pkt_start;
    udph->src_port = idx++;
    udph->dst_port = rte_cpu_to_be_16(0x17C1);
    udp_len = (m->pkt_len - (sizeof(h2s_l2vlan_encap_hdr) +
               sizeof(h2s_ip_encap_hdr)));
    udph->dgram_len = rte_cpu_to_be_16(udp_len);
    pkt_start += sizeof(h2s_udp_encap_hdr);

    memcpy(pkt_start, h2s_geneve_encap_hdr,
           sizeof(h2s_geneve_encap_hdr));
    genh = (struct geneve_hdr *)pkt_start;
    genh->vni_rsvd =
        (rte_cpu_to_be_32(rewrite_underlay->u.geneve.vni << 8));
    pkt_start += sizeof(h2s_geneve_encap_hdr);

    memcpy(pkt_start, h2s_geneve_options,
           sizeof(h2s_geneve_options));
    gen_opt = (struct geneve_options *)pkt_start;
    gen_opt->opt_data =
        rte_cpu_to_be_32(g_flow_cache_policy[vnic_id].src_slot_id);
    gen_opt += 1;
    gen_opt->opt_data =
        rte_cpu_to_be_32(rewrite_underlay->u.geneve.dst_slot_id);    

    return;
}

static void
fte_flow_h2s_rewrite_mplsoudp (struct rte_mbuf *m, uint16_t ip_offset, uint16_t vnic_id)
{
    uint16_t total_encap_len;
    uint16_t mbuf_prepend_len;
    uint8_t *pkt_start;
    struct ether_hdr *etherh;
    struct vlan_hdr *vlanh;
    struct ipv4_hdr *ip4h;
    struct udp_hdr *udph;
    struct mpls_hdr *mplsh;
    uint16_t ip_tot_len, udp_len;
    rewrite_underlay_info_t *rewrite_underlay;
    uint32_t nat_ip;

    // SNAT
    if (g_flow_cache_policy[vnic_id].nat_enabled) {
        ip4h = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, ip_offset);
        if (fte_nmt_get_nat_ip(vnic_id, rte_be_to_cpu_32(ip4h->src_addr),
                               &nat_ip) == SDK_RET_OK) {
            fte_nat_csum_adj_h2s_v4(ip4h, nat_ip);
            ip4h->src_addr = rte_cpu_to_be_32(nat_ip);
        }
    }

    rewrite_underlay = &(g_flow_cache_policy[vnic_id].rewrite_underlay);
    total_encap_len = (sizeof(h2s_l2vlan_encap_hdr) +
                        sizeof(h2s_ip_encap_hdr) + 
                        sizeof(h2s_udp_encap_hdr) +
                        sizeof(h2s_mpls_encap_hdrs));

    mbuf_prepend_len = (total_encap_len - ip_offset);
    pkt_start = (uint8_t *)rte_pktmbuf_prepend(m, mbuf_prepend_len);
    etherh = (struct ether_hdr *)pkt_start;
    vlanh = (struct vlan_hdr *)(etherh + 1);

    memcpy(pkt_start, h2s_l2vlan_encap_hdr,
           sizeof(h2s_l2vlan_encap_hdr));
    memcpy(etherh->d_addr.addr_bytes, rewrite_underlay->substrate_dmac, ETHER_ADDR_LEN);
    memcpy(etherh->s_addr.addr_bytes, rewrite_underlay->substrate_smac, ETHER_ADDR_LEN);
    vlanh->vlan_tci = (rte_be_to_cpu_16(vlanh->vlan_tci) & 0xf000);
    vlanh->vlan_tci |= (rewrite_underlay->substrate_vlan & 0x0fff);
    vlanh->vlan_tci = rte_cpu_to_be_16(vlanh->vlan_tci);
    pkt_start += sizeof(h2s_l2vlan_encap_hdr);

    memcpy(pkt_start, h2s_ip_encap_hdr, sizeof(h2s_ip_encap_hdr));
    ip4h = (struct ipv4_hdr *)pkt_start;
    ip_tot_len = (m->pkt_len - sizeof(h2s_l2vlan_encap_hdr));
    ip4h->total_length = rte_cpu_to_be_16(ip_tot_len);
    ip4h->src_addr = rte_cpu_to_be_32(rewrite_underlay->substrate_sip);
    ip4h->dst_addr = rte_cpu_to_be_32(rewrite_underlay->substrate_dip);
    pkt_start += sizeof(h2s_ip_encap_hdr);
  
    /*Temporary fix to change values of UDP SPORT */ 
    h2s_udp_encap_hdr[1] = idx++;

    memcpy(pkt_start, h2s_udp_encap_hdr, sizeof(h2s_udp_encap_hdr));
    udph = (struct udp_hdr *)pkt_start;
    udp_len = (m->pkt_len - (sizeof(h2s_l2vlan_encap_hdr) +
               sizeof(h2s_ip_encap_hdr)));
    udph->dgram_len = rte_cpu_to_be_16(udp_len); 
    pkt_start += sizeof(h2s_udp_encap_hdr);

    memcpy(pkt_start, h2s_mpls_encap_hdrs,
           sizeof(h2s_mpls_encap_hdrs));
    mplsh = (struct mpls_hdr *)pkt_start;
    mplsh->tag_msb = rte_cpu_to_be_16((rewrite_underlay->u.mplsoudp.mpls_label1 >> 4) & 0xffff);
    mplsh->tag_lsb = (rewrite_underlay->u.mplsoudp.mpls_label1 & 0xf);
    mplsh += 1;
    mplsh->tag_msb = rte_cpu_to_be_16((rewrite_underlay->u.mplsoudp.mpls_label2 >> 4) & 0xffff);
    mplsh->tag_lsb = (rewrite_underlay->u.mplsoudp.mpls_label2 & 0xf);

    return;
}

static void
fte_flow_s2h_rewrite (struct rte_mbuf *m, uint16_t ip_offset, uint16_t vnic_id)
{
    uint16_t mbuf_adj_len;
    uint8_t *pkt_start;
    struct ether_hdr *etherh;
    struct vlan_hdr *vlanh;
    struct ipv4_hdr *ipv4h;
    rewrite_host_info_t *rewrite_host;
    uint32_t local_ip;

    // DNAT
    if (g_flow_cache_policy[vnic_id].nat_enabled) {
        ipv4h = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, ip_offset);
        if (fte_nmt_get_local_ip(vnic_id, rte_be_to_cpu_32(ipv4h->dst_addr),
                               &local_ip) == SDK_RET_OK) {
            ipv4h->dst_addr = rte_cpu_to_be_32(local_ip);
        }
    }

    rewrite_host = &(g_flow_cache_policy[vnic_id].rewrite_host);

    mbuf_adj_len = (ip_offset - sizeof(s2h_l2vlan_encap_hdr));
    pkt_start = (uint8_t *)rte_pktmbuf_adj(m, mbuf_adj_len);
    etherh = (struct ether_hdr *)pkt_start;
    vlanh = (struct vlan_hdr *)(etherh + 1);

    memcpy(pkt_start, s2h_l2vlan_encap_hdr,
           sizeof(s2h_l2vlan_encap_hdr));
    memcpy(etherh->d_addr.addr_bytes, rewrite_host->ep_dmac, ETHER_ADDR_LEN);
    memcpy(etherh->s_addr.addr_bytes, rewrite_host->ep_smac, ETHER_ADDR_LEN);
    vlanh->vlan_tci = (rte_be_to_cpu_16(vlanh->vlan_tci) & 0xf000);
    vlanh->vlan_tci |= (g_flow_cache_policy[vnic_id].vlan_id & 0x0fff);
    vlanh->vlan_tci = rte_cpu_to_be_16(vlanh->vlan_tci);

    ipv4h = (struct ipv4_hdr *)(vlanh + 1); 
    if ((ipv4h->version_ihl >> 4) != 4) {
        vlanh->eth_proto = rte_cpu_to_be_16(ETHER_TYPE_IPv6);
    }

    return;
}

static void
fte_flow_s2h_rewrite_l2 (struct rte_mbuf *m,
                         uint16_t ip_offset, uint16_t vnic_id,
                         struct ether_hdr **l2_flow_eth_hdr)
{
    uint16_t mbuf_adj_len;
    uint8_t *pkt_start;
    struct ether_hdr *etherh, *new_etherh;
    struct vlan_hdr *vlanh;
    struct ipv4_hdr *ipv4h;
    uint32_t local_ip;

    // DNAT
    if (g_flow_cache_policy[vnic_id].nat_enabled) {
        ipv4h = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, ip_offset);
        if (fte_nmt_get_local_ip(vnic_id, rte_be_to_cpu_32(ipv4h->dst_addr),
                               &local_ip) == SDK_RET_OK) {
            ipv4h->dst_addr = rte_cpu_to_be_32(local_ip);
        }
    }

    mbuf_adj_len = (ip_offset - sizeof(struct ether_hdr));
    pkt_start = (uint8_t *)rte_pktmbuf_adj(m, mbuf_adj_len);
    etherh = (struct ether_hdr *)pkt_start;

    // Insert VLAN tag
    new_etherh = (struct ether_hdr *)rte_pktmbuf_prepend(m,
                                        sizeof(struct vlan_hdr));
    memmove(new_etherh, etherh, 2 * ETHER_ADDR_LEN);
    new_etherh->ether_type = rte_cpu_to_be_16(ETHER_TYPE_VLAN);

    vlanh = (struct vlan_hdr *)(new_etherh + 1);
    memset(vlanh, 0, sizeof(struct vlan_hdr));
    vlanh->vlan_tci |= (g_flow_cache_policy[vnic_id].vlan_id & 0x0fff);
    vlanh->vlan_tci = rte_cpu_to_be_16(vlanh->vlan_tci);

    ipv4h = (struct ipv4_hdr *)(vlanh + 1); 
    if ((ipv4h->version_ihl >> 4) != 4) {
        vlanh->eth_proto = rte_cpu_to_be_16(ETHER_TYPE_IPv6);
    } else {
        vlanh->eth_proto = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
    }

    *l2_flow_eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);

    return;
}

static void
fte_flow_pkt_rewrite (struct rte_mbuf *m, uint8_t dir,
                      uint16_t ip_offset, uint16_t vnic_id,
                      struct ether_hdr **l2_flow_eth_hdr)
{
    if (dir == HOST_TO_SWITCH) {
        if (fte_is_vnic_type_l2(vnic_id)) {
            fte_flow_h2s_rewrite_geneve(m, ip_offset, vnic_id,
                                        l2_flow_eth_hdr);
            return;
        }
        fte_flow_h2s_rewrite_mplsoudp(m, ip_offset, vnic_id);
    } else {
        if (fte_is_vnic_type_l2(vnic_id)) {
            fte_flow_s2h_rewrite_l2(m, ip_offset, vnic_id,
                                    l2_flow_eth_hdr);
            return;
        }
        fte_flow_s2h_rewrite(m, ip_offset, vnic_id);
    }

    return;
}

static sdk_ret_t
fte_get_session_rewrite_id (uint16_t vnic_id, uint32_t local_ip,
                            uint32_t *h2s_rewrite_id,
                            uint32_t *s2h_rewrite_id)
{
    if (g_flow_cache_policy[vnic_id].nat_enabled) {
        if (fte_nmt_get_rewrite_id(vnic_id, local_ip,
                                   h2s_rewrite_id,
                                   s2h_rewrite_id) == SDK_RET_OK) {
            return SDK_RET_OK;
        }
    }

    *h2s_rewrite_id = 
        g_flow_cache_policy[vnic_id].rewrite_underlay.rewrite_id;
    *s2h_rewrite_id =
        g_flow_cache_policy[vnic_id].rewrite_host.rewrite_id;

    return SDK_RET_OK;
}

static sdk_ret_t
fte_session_info_create (uint32_t session_index,
                         uint32_t conntrack_index,
                         uint16_t h2s_allowed_flow_state_bitmask,
                         uint16_t s2h_allowed_flow_state_bitmask,
                         uint32_t h2s_rewrite_id,
                         uint32_t s2h_rewrite_id,
                         bool     skip_flow_log,
                         pds_egress_action_t  h2s_egress_action,
                         pds_egress_action_t  s2h_egress_action)
{
    pds_flow_session_spec_t spec;

    memset(&spec, 0, sizeof(spec));
    spec.key.session_info_id = session_index;
    spec.key.direction = (SWITCH_TO_HOST | HOST_TO_SWITCH);

    spec.data.conntrack_id = conntrack_index;
    spec.data.skip_flow_log = skip_flow_log;

    spec.data.host_to_switch_flow_info.rewrite_id = h2s_rewrite_id;
    spec.data.host_to_switch_flow_info.allowed_flow_state_bitmask =
                                    h2s_allowed_flow_state_bitmask;
    spec.data.host_to_switch_flow_info.egress_action = h2s_egress_action;

    spec.data.switch_to_host_flow_info.rewrite_id = s2h_rewrite_id;
    spec.data.switch_to_host_flow_info.allowed_flow_state_bitmask =
                                    s2h_allowed_flow_state_bitmask;
    spec.data.switch_to_host_flow_info.egress_action = s2h_egress_action;

    return (sdk_ret_t)pds_flow_session_info_create(&spec);
}

static sdk_ret_t
fte_session_info_delete (uint32_t session_index)
{
    pds_flow_session_key_t session_key;

    memset(&session_key, 0, sizeof(session_key));
    session_key.session_info_id = session_index;
    session_key.direction = (SWITCH_TO_HOST | HOST_TO_SWITCH);
    return (sdk_ret_t)pds_flow_session_info_delete(&session_key);
}

static sdk_ret_t
fte_l2_flow_check_macaddr (uint16_t vnic_id, uint8_t flow_dir,
                           l2_flows_range_info_t *flow_range,
                           struct ether_hdr *eth_hdr,
                           uint32_t *h2s_bmp_posn,
                           uint32_t *s2h_bmp_posn)
{
    uint64_t h2s_mac = 0, s2h_mac = 0;

    if (flow_dir == HOST_TO_SWITCH) {
        h2s_mac = MAC_TO_UINT64(eth_hdr->d_addr.addr_bytes);
        if ((h2s_mac < flow_range->h2s_mac_lo) ||
            (h2s_mac > flow_range->h2s_mac_hi)) {
            PDS_TRACE_DEBUG("H2S: VNIC:%u: DMAC is out of range.\n",
                            vnic_id);
            return SDK_RET_OOB;
        }

        s2h_mac = MAC_TO_UINT64(eth_hdr->s_addr.addr_bytes);
        if ((s2h_mac < flow_range->s2h_mac_lo) ||
            (s2h_mac > flow_range->s2h_mac_hi)) {
            PDS_TRACE_DEBUG("H2S: VNIC:%u: SMAC is out of range.\n",
                            vnic_id);
            return SDK_RET_OOB;
        }
    } else {
        s2h_mac = MAC_TO_UINT64(eth_hdr->d_addr.addr_bytes);
        if ((s2h_mac < flow_range->s2h_mac_lo) ||
            (s2h_mac > flow_range->s2h_mac_hi)) {
            PDS_TRACE_DEBUG("S2H: VNIC:%u: DMAC is out of range.\n",
                            vnic_id);
            return SDK_RET_OOB;
        }

        h2s_mac = MAC_TO_UINT64(eth_hdr->s_addr.addr_bytes);
        if ((h2s_mac < flow_range->h2s_mac_lo) ||
            (h2s_mac > flow_range->h2s_mac_hi)) {
            PDS_TRACE_DEBUG("S2H: VNIC:%u: SMAC is out of range.\n",
                            vnic_id);
            return SDK_RET_OOB;
        }
    }

    *h2s_bmp_posn = (h2s_mac - flow_range->h2s_mac_lo);
    *s2h_bmp_posn = (s2h_mac - flow_range->s2h_mac_lo);

    return SDK_RET_OK;
}
    
static sdk_ret_t
fte_l2_flow_cache_entry_create (uint16_t vnic_id, uint8_t flow_dir,
                                struct ether_hdr *eth_hdr,
                                uint32_t h2s_rewrite_id,
                                uint32_t s2h_rewrite_id)
{
    sdk_ret_t ret = SDK_RET_OK;
    pds_l2_flow_spec_t spec_h2s;
    pds_l2_flow_spec_t spec_s2h;
    l2_flows_range_info_t *flow_range;
    uint32_t h2s_bmp_posn = 0;
    uint32_t s2h_bmp_posn = 0;

    flow_range = &(g_flow_cache_policy[vnic_id].l2_flows_range);
    ret = fte_l2_flow_check_macaddr(vnic_id, flow_dir,
                                    flow_range, eth_hdr,
                                    &h2s_bmp_posn, &s2h_bmp_posn);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_DEBUG("fte_l2_flow_check_macaddr failed.\n");
        return ret;
    }

    memset(&spec_h2s, 0, sizeof(pds_l2_flow_spec_t));
    memset(&spec_s2h, 0, sizeof(pds_l2_flow_spec_t));

    spec_h2s.key.vnic_id = vnic_id;
    spec_s2h.key.vnic_id = vnic_id;

    if (flow_dir == HOST_TO_SWITCH) {
        sdk::lib::memrev(spec_h2s.key.dmac,
                         eth_hdr->d_addr.addr_bytes, ETH_ADDR_LEN);
        sdk::lib::memrev(spec_s2h.key.dmac,
                         eth_hdr->s_addr.addr_bytes, ETH_ADDR_LEN);
    } else {
        sdk::lib::memrev(spec_s2h.key.dmac,
                         eth_hdr->d_addr.addr_bytes, ETH_ADDR_LEN);
        sdk::lib::memrev(spec_h2s.key.dmac,
                         eth_hdr->s_addr.addr_bytes, ETH_ADDR_LEN);
    }

    spec_h2s.data.index = h2s_rewrite_id;
    spec_s2h.data.index = s2h_rewrite_id;

    rte_spinlock_lock(&(flow_range->h2s_bmp_slock));
    if (!(rte_bitmap_get(flow_range->h2s_bmp, h2s_bmp_posn))) {
        rte_bitmap_set(flow_range->h2s_bmp, h2s_bmp_posn);
        rte_spinlock_unlock(&(flow_range->h2s_bmp_slock));

        ret = (sdk_ret_t)pds_l2_flow_cache_entry_create(&spec_h2s);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_DEBUG("pds_l2_flow_cache_entry_create "
                            "H2S failed. \n");
            rte_bitmap_clear(flow_range->h2s_bmp, h2s_bmp_posn);
            return ret;
        }
    } else {
        rte_spinlock_unlock(&(flow_range->h2s_bmp_slock));
    }

    rte_spinlock_lock(&(flow_range->s2h_bmp_slock));
    if (!(rte_bitmap_get(flow_range->s2h_bmp, s2h_bmp_posn))) {
        rte_bitmap_set(flow_range->s2h_bmp, s2h_bmp_posn);
        rte_spinlock_unlock(&(flow_range->s2h_bmp_slock));

        ret = (sdk_ret_t)pds_l2_flow_cache_entry_create(&spec_s2h);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_DEBUG("pds_l2_flow_cache_entry_create "
                            "S2H failed. \n");
            rte_bitmap_clear(flow_range->s2h_bmp, s2h_bmp_posn);
            return ret;
        }
    } else {
        rte_spinlock_unlock(&(flow_range->s2h_bmp_slock));
    }

    return SDK_RET_OK;
}

sdk_ret_t
fte_flow_create(uint16_t vnic_id, ipv4_addr_t v4_addr_sip, ipv4_addr_t v4_addr_dip,
        uint8_t proto, uint16_t sport, uint16_t dport,
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
    spec.key.l4.tcp_udp.sport = sport;
    spec.key.l4.tcp_udp.dport = dport;

    spec.data.index_type = index_type;
    spec.data.index = index;

    return (sdk_ret_t)pds_flow_cache_entry_create(&spec);
}

sdk_ret_t
fte_flow_create_icmp(uint16_t vnic_id,
        ipv4_addr_t v4_addr_sip, ipv4_addr_t v4_addr_dip,
        uint8_t proto, uint8_t type, uint8_t code, uint16_t identifier,
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
    spec.key.l4.icmp.type = type;
    spec.key.l4.icmp.code = code;
    spec.key.l4.icmp.identifier = identifier;

    spec.data.index_type = index_type;
    spec.data.index = index;

    return (sdk_ret_t)pds_flow_cache_entry_create(&spec);
}

sdk_ret_t
fte_flow_create_v6(uint16_t vnic_id, ipv6_addr_t *v6_addr_sip,
        ipv6_addr_t *v6_addr_dip,
        uint8_t proto, uint16_t sport, uint16_t dport,
        pds_flow_spec_index_type_t index_type, uint32_t index)
{
    pds_flow_spec_t             spec;


    spec.key.vnic_id = vnic_id;
    spec.key.key_type = KEY_TYPE_IPV6;
    sdk::lib::memrev(spec.key.ip_saddr, (uint8_t *)v6_addr_sip,
            sizeof(ipv6_addr_t));
    sdk::lib::memrev(spec.key.ip_daddr, (uint8_t*)v6_addr_dip,
            sizeof(ipv6_addr_t));
    spec.key.ip_proto = proto;
    spec.key.l4.tcp_udp.sport = sport;
    spec.key.l4.tcp_udp.dport = dport;

    spec.data.index_type = index_type;
    spec.data.index = index;

    return (sdk_ret_t)pds_flow_cache_entry_create(&spec);
}

sdk_ret_t
fte_flow_create_v6_icmp(uint16_t vnic_id, ipv6_addr_t *v6_addr_sip,
        ipv6_addr_t *v6_addr_dip,
        uint8_t proto, uint8_t type, uint8_t code, uint16_t identifier,
        pds_flow_spec_index_type_t index_type, uint32_t index)
{
    pds_flow_spec_t             spec;


    spec.key.vnic_id = vnic_id;
    spec.key.key_type = KEY_TYPE_IPV6;
    sdk::lib::memrev(spec.key.ip_saddr, (uint8_t *)v6_addr_sip,
            sizeof(ipv6_addr_t));
    sdk::lib::memrev(spec.key.ip_daddr, (uint8_t*)v6_addr_dip,
            sizeof(ipv6_addr_t));
    spec.key.ip_proto = proto;
    spec.key.l4.icmp.type = type;
    spec.key.l4.icmp.code = code;
    spec.key.l4.icmp.identifier = identifier;

    spec.data.index_type = index_type;
    spec.data.index = index;

    return (sdk_ret_t)pds_flow_cache_entry_create(&spec);
}

sdk_ret_t
fte_session_info_create_all(uint32_t session_id, uint32_t conntrack_id,
                uint8_t skip_flow_log, mac_addr_t *host_mac,
                uint16_t h2s_epoch_vnic, uint32_t h2s_epoch_vnic_id,
                uint16_t h2s_epoch_mapping, uint32_t h2s_epoch_mapping_id,
                uint16_t h2s_policer_bw1_id, uint16_t h2s_policer_bw2_id,
                uint16_t h2s_vnic_stats_id, uint8_t *h2s_vnic_stats_mask,
                uint16_t h2s_vnic_histogram_latency_id, uint16_t h2s_vnic_histogram_packet_len_id,
                uint8_t h2s_tcp_flags_bitmap,
                uint32_t h2s_session_rewrite_id,
                uint16_t h2s_allowed_flow_state_bitmask,
                pds_egress_action_t h2s_egress_action,

                uint16_t s2h_epoch_vnic, uint32_t s2h_epoch_vnic_id,
                uint16_t s2h_epoch_mapping, uint32_t s2h_epoch_mapping_id,
                uint16_t s2h_policer_bw1_id, uint16_t s2h_policer_bw2_id,
                uint16_t s2h_vnic_stats_id, uint8_t *s2h_vnic_stats_mask,
                uint16_t s2h_vnic_histogram_latency_id, uint16_t s2h_vnic_histogram_packet_len_id,
                uint8_t s2h_tcp_flags_bitmap,
                uint32_t s2h_session_rewrite_id,
                uint16_t s2h_allowed_flow_state_bitmask,
                pds_egress_action_t s2h_egress_action)
{
    pds_ret_t                               ret = PDS_RET_OK;
    pds_flow_session_spec_t                 spec;

    memset(&spec, 0, sizeof(spec));
    spec.key.session_info_id = session_id;
    spec.key.direction = (SWITCH_TO_HOST | HOST_TO_SWITCH);

    spec.data.conntrack_id = conntrack_id;
    spec.data.skip_flow_log = skip_flow_log;
    sdk::lib::memrev(spec.data.host_mac, (uint8_t*)host_mac, sizeof(mac_addr_t));

    /* Host-to-switch */
    spec.data.host_to_switch_flow_info.epoch_vnic = h2s_epoch_vnic;
    spec.data.host_to_switch_flow_info.epoch_vnic_id = h2s_epoch_vnic_id;
    spec.data.host_to_switch_flow_info.epoch_mapping = h2s_epoch_mapping;
    spec.data.host_to_switch_flow_info.policer_bw1_id = h2s_policer_bw1_id;
    spec.data.host_to_switch_flow_info.policer_bw2_id = h2s_policer_bw2_id;
    spec.data.host_to_switch_flow_info.vnic_stats_id = h2s_vnic_stats_id;
    sdk::lib::memrev(spec.data.host_to_switch_flow_info.vnic_stats_mask,
            h2s_vnic_stats_mask, PDS_FLOW_STATS_MASK_LEN);
    spec.data.host_to_switch_flow_info.vnic_histogram_latency_id = h2s_vnic_histogram_latency_id;
    spec.data.host_to_switch_flow_info.vnic_histogram_packet_len_id = h2s_vnic_histogram_packet_len_id;
    spec.data.host_to_switch_flow_info.tcp_flags_bitmap = h2s_tcp_flags_bitmap;
    spec.data.host_to_switch_flow_info.rewrite_id = h2s_session_rewrite_id;
    spec.data.host_to_switch_flow_info.allowed_flow_state_bitmask = h2s_allowed_flow_state_bitmask;
    spec.data.host_to_switch_flow_info.egress_action = h2s_egress_action;

    /* Switch-to-host */
    spec.data.switch_to_host_flow_info.epoch_vnic = s2h_epoch_vnic;
    spec.data.switch_to_host_flow_info.epoch_vnic_id = s2h_epoch_vnic_id;
    spec.data.switch_to_host_flow_info.epoch_mapping = s2h_epoch_mapping;
    spec.data.switch_to_host_flow_info.policer_bw1_id = s2h_policer_bw1_id;
    spec.data.switch_to_host_flow_info.policer_bw2_id = s2h_policer_bw2_id;
    spec.data.switch_to_host_flow_info.vnic_stats_id = s2h_vnic_stats_id;
    sdk::lib::memrev(spec.data.switch_to_host_flow_info.vnic_stats_mask,
            s2h_vnic_stats_mask, PDS_FLOW_STATS_MASK_LEN);
    spec.data.switch_to_host_flow_info.vnic_histogram_latency_id = s2h_vnic_histogram_latency_id;
    spec.data.switch_to_host_flow_info.vnic_histogram_packet_len_id = s2h_vnic_histogram_packet_len_id;
    spec.data.switch_to_host_flow_info.tcp_flags_bitmap = s2h_tcp_flags_bitmap;
    spec.data.switch_to_host_flow_info.rewrite_id = s2h_session_rewrite_id;
    spec.data.switch_to_host_flow_info.allowed_flow_state_bitmask = s2h_allowed_flow_state_bitmask;
    spec.data.switch_to_host_flow_info.egress_action = s2h_egress_action;

    ret = pds_flow_session_info_create(&spec);
    if (ret != PDS_RET_OK) {
        PDS_TRACE_ERR("Failed to program session s2h info : %u\n", ret);
    }
    return (sdk_ret_t)ret;
}

sdk_ret_t
fte_flow_prog (struct rte_mbuf *m)
{
    sdk_ret_t ret = SDK_RET_OK;
    pds_flow_spec_t flow_spec;
    uint8_t flow_dir;
    uint16_t ip_offset;
    uint16_t vnic_id = 0;
    uint8_t tcp_flags = 0;
    uint32_t h2s_rewrite_id;
    uint32_t s2h_rewrite_id;
    uint32_t conntrack_index = 0;
    uint32_t session_index;
    uint16_t h2s_flow_state_bmp = 0;
    uint16_t s2h_flow_state_bmp = 0;
    struct ether_hdr *l2_flow_eth_hdr = NULL;
    bool     skip_flow_log = false;
    pds_egress_action_t h2s_egress_action = EGRESS_ACTION_TX_TO_SWITCH;
    pds_egress_action_t s2h_egress_action = EGRESS_ACTION_TX_TO_HOST;

    memset(&flow_spec, 0, sizeof(pds_flow_spec_t));
    ret = fte_flow_extract_prog_args(m, &flow_spec, &flow_dir,
                                &ip_offset, &vnic_id, &tcp_flags);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_DEBUG("fte_flow_extract_prog_args failed. \n");
        return ret;
    }

    // PKT Rewrite
    fte_flow_pkt_rewrite(m, flow_dir, ip_offset, vnic_id,
                         &l2_flow_eth_hdr);

    if (fte_is_conntrack_enabled(vnic_id)) {
        ret = fte_conntrack_index_alloc(&conntrack_index);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_DEBUG("fte_conntrack_index_alloc failed. \n");
            return ret;
        }
        ret = fte_conntrack_state_create(conntrack_index, &flow_spec,
                                         flow_dir, tcp_flags);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_DEBUG("fte_conntrack_state_create failed. \n");
            return ret;
        }
        if (flow_spec.key.ip_proto == IP_PROTO_TCP) {
            // allowed_flow_state_bitmask
            if (flow_dir == HOST_TO_SWITCH) {
                // Applicable SYN states - HOST side initiated flows
                // SYN_SENT & SYNACK_RECV
                h2s_flow_state_bmp = 0x3F3;
                s2h_flow_state_bmp = 0x3F3;
            } else {
                // Applicable SYN states - SWITCH side initiated flows
                // SYN_RECV & SYNACK_SENT
                s2h_flow_state_bmp = 0x3ED;
                h2s_flow_state_bmp = 0x3ED;
            }
        }
    }

    flow_spec.data.index_type = PDS_FLOW_SPEC_INDEX_SESSION;
    ret = fte_session_index_alloc(&session_index);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_DEBUG("fte_session_index_alloc failed. \n");
        fte_conntrack_free(vnic_id, conntrack_index);
        return ret;
    }
    flow_spec.data.index = session_index;

    ret = fte_get_session_rewrite_id(vnic_id,
                            (*(uint32_t *)flow_spec.key.ip_saddr),
                            &h2s_rewrite_id, &s2h_rewrite_id);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_DEBUG("fte_get_session_rewrite_id failed. \n");
        fte_session_index_free(session_index);
        fte_conntrack_free(vnic_id, conntrack_index);
        return ret;
    }

    skip_flow_log = fte_skip_flow_log(vnic_id);
    h2s_egress_action = fte_get_egress_action(vnic_id, HOST_TO_SWITCH);
    s2h_egress_action = fte_get_egress_action(vnic_id, SWITCH_TO_HOST);

    ret = fte_session_info_create(session_index, conntrack_index,
                                  h2s_flow_state_bmp,
                                  s2h_flow_state_bmp,
                                  h2s_rewrite_id, s2h_rewrite_id,
                                  skip_flow_log, 
                                  h2s_egress_action, 
                                  s2h_egress_action);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_DEBUG("fte_session_info_create failed. \n");
        fte_session_index_free(session_index);
        fte_conntrack_free(vnic_id, conntrack_index);
        return ret;
    }

    ret = (sdk_ret_t)pds_flow_cache_entry_create(&flow_spec);
    if (ret != SDK_RET_OK) {
        fte_session_info_delete(session_index);
        fte_session_index_free(session_index);
        fte_conntrack_free(vnic_id, conntrack_index);
        if (ret != SDK_RET_ENTRY_EXISTS) {
            PDS_TRACE_DEBUG("pds_flow_cache_entry_create failed. \n");
            return ret;
        }
    }

    if (fte_is_vnic_type_l2(vnic_id)) {
        ret = fte_l2_flow_cache_entry_create(vnic_id, flow_dir,
                    l2_flow_eth_hdr, h2s_rewrite_id, s2h_rewrite_id);
        if (ret != SDK_RET_OK) {
            pds_flow_data_t flow_data;
            PDS_TRACE_DEBUG("fte_l2_flow_cache_entry_create "
                            "failed. \n");
            memset(&flow_data, 0, sizeof(flow_data));
            flow_data.index = session_index;
            flow_data.index_type = PDS_FLOW_SPEC_INDEX_SESSION;
            pds_flow_cache_entry_delete_by_flow_info(&flow_data);
            fte_session_info_delete(session_index);
            fte_session_index_free(session_index);
            fte_conntrack_free(vnic_id, conntrack_index);
            return ret;
        }
    }
         
    return SDK_RET_OK;    
}

void
fte_thread_init (unsigned int core_id)
{
    PDS_TRACE_DEBUG("Thread init on core#:%u\n", core_id);
    pds_thread_init(core_id);
}

sdk_ret_t
fte_vlan_to_vnic_map (uint16_t vlan_id, uint16_t vnic_id,
                      uint8_t vnic_type)
{
    pds_vlan_to_vnic_map_spec_t spec;

    spec.key.vlan_id = vlan_id;
    spec.data.vnic_type =
        ((vnic_type == VNIC_L3) ? VNIC_TYPE_L3 : VNIC_TYPE_L2);
    spec.data.vnic_id = vnic_id;

    return (sdk_ret_t)pds_vlan_to_vnic_map_create(&spec);
}

sdk_ret_t
fte_mpls_label_to_vnic_map (uint32_t mpls_label, uint16_t vnic_id,
                            uint8_t vnic_type)
{
    pds_mpls_label_to_vnic_map_spec_t spec;

    spec.key.mpls_label = mpls_label;
    spec.data.vnic_type =
        ((vnic_type == VNIC_L3) ? VNIC_TYPE_L3 : VNIC_TYPE_L2);
    spec.data.vnic_id = vnic_id;

    return (sdk_ret_t)pds_mpls_label_to_vnic_map_create(&spec);
}

sdk_ret_t
fte_h2s_v4_session_rewrite_mplsoudp (uint32_t session_rewrite_id,
                                     mac_addr_t *substrate_dmac,
                                     mac_addr_t *substrate_smac,
                                     uint16_t substrate_vlan,
                                     uint32_t substrate_sip,
                                     uint32_t substrate_dip,
                                     uint32_t mpls1_label,
                                     uint32_t mpls2_label)
{
    pds_flow_session_rewrite_spec_t spec;

    memset(&spec, 0, sizeof(spec));
    spec.key.session_rewrite_id = session_rewrite_id;

    spec.data.strip_l2_header = TRUE;
    spec.data.strip_vlan_tag = TRUE;

    spec.data.nat_info.nat_type = REWRITE_NAT_TYPE_NONE;

    spec.data.encap_type = ENCAP_TYPE_MPLSOUDP;
    sdk::lib::memrev(spec.data.u.mplsoudp_encap.l2_encap.dmac,
                     (uint8_t*)substrate_dmac, sizeof(mac_addr_t));
    sdk::lib::memrev(spec.data.u.mplsoudp_encap.l2_encap.smac,
                     (uint8_t*)substrate_smac, sizeof(mac_addr_t));
    spec.data.u.mplsoudp_encap.l2_encap.insert_vlan_tag = TRUE;
    spec.data.u.mplsoudp_encap.l2_encap.vlan_id = substrate_vlan;

    spec.data.u.mplsoudp_encap.ip_encap.ip_saddr = substrate_sip;
    spec.data.u.mplsoudp_encap.ip_encap.ip_daddr = substrate_dip;

    spec.data.u.mplsoudp_encap.mpls1_label = mpls1_label;
    spec.data.u.mplsoudp_encap.mpls2_label = mpls2_label;

    return (sdk_ret_t)pds_flow_session_rewrite_create(&spec);
}

sdk_ret_t
fte_h2s_v4_session_rewrite_geneve (uint32_t session_rewrite_id,
                                   mac_addr_t *substrate_dmac,
                                   mac_addr_t *substrate_smac,
                                   uint16_t substrate_vlan,
                                   uint32_t substrate_sip,
                                   uint32_t substrate_dip,
                                   uint32_t vni, uint32_t src_slot_id,
                                   uint32_t dst_slot_id,
                                   uint16_t sg_id1, uint16_t sg_id2,
                                   uint16_t sg_id3, uint16_t sg_id4,
                                   uint16_t sg_id5, uint16_t sg_id6,
                                   uint32_t orig_phy_ip)
{
    pds_flow_session_rewrite_spec_t spec;

    memset(&spec, 0, sizeof(spec));
    spec.key.session_rewrite_id = session_rewrite_id;

    spec.data.strip_l2_header = FALSE;
    spec.data.strip_vlan_tag = TRUE;

    spec.data.nat_info.nat_type = REWRITE_NAT_TYPE_NONE;

    spec.data.encap_type = ENCAP_TYPE_GENEVE;
    sdk::lib::memrev(spec.data.u.geneve_encap.l2_encap.dmac,
                     (uint8_t*)substrate_dmac, sizeof(mac_addr_t));
    sdk::lib::memrev(spec.data.u.geneve_encap.l2_encap.smac,
                     (uint8_t*)substrate_smac, sizeof(mac_addr_t));
    spec.data.u.geneve_encap.l2_encap.insert_vlan_tag = TRUE;
    spec.data.u.geneve_encap.l2_encap.vlan_id = substrate_vlan;

    spec.data.u.geneve_encap.ip_encap.ip_saddr = substrate_sip;
    spec.data.u.geneve_encap.ip_encap.ip_daddr = substrate_dip;

    spec.data.u.geneve_encap.vni = vni;
    spec.data.u.geneve_encap.source_slot_id = src_slot_id;
    spec.data.u.geneve_encap.destination_slot_id = dst_slot_id;
    spec.data.u.geneve_encap.sg_id1 = sg_id1;
    spec.data.u.geneve_encap.sg_id2 = sg_id2;
    spec.data.u.geneve_encap.sg_id3 = sg_id3;
    spec.data.u.geneve_encap.sg_id4 = sg_id4;
    spec.data.u.geneve_encap.sg_id5 = sg_id5;
    spec.data.u.geneve_encap.sg_id6 = sg_id6;
    spec.data.u.geneve_encap.originator_physical_ip = orig_phy_ip;

    return (sdk_ret_t)pds_flow_session_rewrite_create(&spec);
}

sdk_ret_t
fte_h2s_nat_v4_session_rewrite_mplsoudp (uint32_t session_rewrite_id,
                                     mac_addr_t *substrate_dmac,
                                     mac_addr_t *substrate_smac,
                                     uint16_t substrate_vlan,
                                     uint32_t substrate_sip,
                                     uint32_t substrate_dip,
                                     uint32_t mpls1_label,
                                     uint32_t mpls2_label,
                                     pds_flow_session_rewrite_nat_type_t nat_type,
                                     ipv4_addr_t ipv4_addr)
{
    pds_flow_session_rewrite_spec_t spec;

    memset(&spec, 0, sizeof(spec));
    spec.key.session_rewrite_id = session_rewrite_id;

    spec.data.strip_l2_header = TRUE;
    spec.data.strip_vlan_tag = TRUE;

    spec.data.nat_info.nat_type = nat_type;
    spec.data.nat_info.u.ipv4_addr = (uint32_t) ipv4_addr;

    spec.data.encap_type = ENCAP_TYPE_MPLSOUDP;
    sdk::lib::memrev(spec.data.u.mplsoudp_encap.l2_encap.dmac,
                     (uint8_t*)substrate_dmac, sizeof(mac_addr_t));
    sdk::lib::memrev(spec.data.u.mplsoudp_encap.l2_encap.smac,
                     (uint8_t*)substrate_smac, sizeof(mac_addr_t));
    spec.data.u.mplsoudp_encap.l2_encap.insert_vlan_tag = TRUE;
    spec.data.u.mplsoudp_encap.l2_encap.vlan_id = substrate_vlan;

    spec.data.u.mplsoudp_encap.ip_encap.ip_saddr = substrate_sip;
    spec.data.u.mplsoudp_encap.ip_encap.ip_daddr = substrate_dip;

    spec.data.u.mplsoudp_encap.mpls1_label = mpls1_label;
    spec.data.u.mplsoudp_encap.mpls2_label = mpls2_label;

    return (sdk_ret_t)pds_flow_session_rewrite_create(&spec);
}

sdk_ret_t
fte_h2s_nat_v4_session_rewrite_geneve (uint32_t session_rewrite_id,
                                   mac_addr_t *substrate_dmac,
                                   mac_addr_t *substrate_smac,
                                   uint16_t substrate_vlan,
                                   uint32_t substrate_sip,
                                   uint32_t substrate_dip,
                                   uint32_t vni, uint32_t src_slot_id,
                                   uint32_t dst_slot_id,
                                   uint16_t sg_id1, uint16_t sg_id2,
                                   uint16_t sg_id3, uint16_t sg_id4,
                                   uint16_t sg_id5, uint16_t sg_id6,
                                   uint32_t orig_phy_ip,
                                   pds_flow_session_rewrite_nat_type_t nat_type,
                                   ipv4_addr_t ipv4_addr)
{
    pds_flow_session_rewrite_spec_t spec;

    memset(&spec, 0, sizeof(spec));
    spec.key.session_rewrite_id = session_rewrite_id;

    spec.data.strip_l2_header = FALSE;
    spec.data.strip_vlan_tag = TRUE;

    spec.data.nat_info.nat_type = nat_type;
    spec.data.nat_info.u.ipv4_addr = (uint32_t) ipv4_addr;

    spec.data.encap_type = ENCAP_TYPE_GENEVE;
    sdk::lib::memrev(spec.data.u.geneve_encap.l2_encap.dmac,
                     (uint8_t*)substrate_dmac, sizeof(mac_addr_t));
    sdk::lib::memrev(spec.data.u.geneve_encap.l2_encap.smac,
                     (uint8_t*)substrate_smac, sizeof(mac_addr_t));
    spec.data.u.geneve_encap.l2_encap.insert_vlan_tag = TRUE;
    spec.data.u.geneve_encap.l2_encap.vlan_id = substrate_vlan;

    spec.data.u.geneve_encap.ip_encap.ip_saddr = substrate_sip;
    spec.data.u.geneve_encap.ip_encap.ip_daddr = substrate_dip;

    spec.data.u.geneve_encap.vni = vni;
    spec.data.u.geneve_encap.source_slot_id = src_slot_id;
    spec.data.u.geneve_encap.destination_slot_id = dst_slot_id;
    spec.data.u.geneve_encap.sg_id1 = sg_id1;
    spec.data.u.geneve_encap.sg_id2 = sg_id2;
    spec.data.u.geneve_encap.sg_id3 = sg_id3;
    spec.data.u.geneve_encap.sg_id4 = sg_id4;
    spec.data.u.geneve_encap.sg_id5 = sg_id5;
    spec.data.u.geneve_encap.sg_id6 = sg_id6;
    spec.data.u.geneve_encap.originator_physical_ip = orig_phy_ip;

    return (sdk_ret_t)pds_flow_session_rewrite_create(&spec);
}

sdk_ret_t
fte_create_dnat_map_ipv4(uint16_t vnic_id, ipv4_addr_t v4_nat_dip, 
        ipv4_addr_t v4_orig_dip, uint16_t dnat_epoch)
{
    pds_dnat_mapping_spec_t         spec;

    memset(&spec, 0, sizeof(spec));

    spec.key.vnic_id = vnic_id;
    spec.key.key_type = IP_AF_IPV4;
    memcpy(spec.key.addr, &v4_nat_dip, sizeof(ipv4_addr_t));

    spec.data.addr_type = IP_AF_IPV4;
    memcpy(spec.data.addr, &v4_orig_dip, sizeof(ipv4_addr_t));
    spec.data.epoch = dnat_epoch;

    return (sdk_ret_t)pds_dnat_map_entry_create(&spec);
}

sdk_ret_t
fte_s2h_v4_session_rewrite (uint32_t session_rewrite_id,
                            mac_addr_t *ep_dmac, mac_addr_t *ep_smac,
                            uint16_t vnic_vlan)
{
    pds_flow_session_rewrite_spec_t spec;

    memset(&spec, 0, sizeof(spec));
    spec.key.session_rewrite_id = session_rewrite_id;

    spec.data.strip_encap_header = TRUE;
    spec.data.strip_l2_header = TRUE;
    spec.data.strip_vlan_tag = TRUE;

    spec.data.nat_info.nat_type = REWRITE_NAT_TYPE_NONE;

    spec.data.encap_type = ENCAP_TYPE_L2;
    sdk::lib::memrev(spec.data.u.l2_encap.dmac, (uint8_t*)ep_dmac,
                     sizeof(mac_addr_t));
    sdk::lib::memrev(spec.data.u.l2_encap.smac, (uint8_t*)ep_smac,
                     sizeof(mac_addr_t));
    spec.data.u.l2_encap.insert_vlan_tag = TRUE;
    spec.data.u.l2_encap.vlan_id = vnic_vlan;

    return (sdk_ret_t)pds_flow_session_rewrite_create(&spec);
}

sdk_ret_t
fte_s2h_v4_session_rewrite_l2 (uint32_t session_rewrite_id,
                               uint16_t vnic_vlan)
{
    pds_flow_session_rewrite_spec_t spec;

    memset(&spec, 0, sizeof(spec));
    spec.key.session_rewrite_id = session_rewrite_id;

    spec.data.strip_encap_header = TRUE;
    spec.data.strip_l2_header = TRUE;
    spec.data.strip_vlan_tag = TRUE;

    spec.data.nat_info.nat_type = REWRITE_NAT_TYPE_NONE;

    spec.data.encap_type = ENCAP_TYPE_INSERT_CTAG;
    spec.data.u.insert_ctag.vlan_id = vnic_vlan;

    return (sdk_ret_t)pds_flow_session_rewrite_create(&spec);
}

sdk_ret_t
fte_s2h_nat_v4_session_rewrite (uint32_t session_rewrite_id,
                            mac_addr_t *ep_dmac, mac_addr_t *ep_smac,
                            uint16_t vnic_vlan,
                            pds_flow_session_rewrite_nat_type_t nat_type,
                            ipv4_addr_t ipv4_addr)
{
    pds_flow_session_rewrite_spec_t spec;

    memset(&spec, 0, sizeof(spec));
    spec.key.session_rewrite_id = session_rewrite_id;

    spec.data.strip_encap_header = TRUE;
    spec.data.strip_l2_header = TRUE;
    spec.data.strip_vlan_tag = TRUE;

    spec.data.nat_info.nat_type = nat_type;
    spec.data.nat_info.u.ipv4_addr = (uint32_t) ipv4_addr;

    spec.data.encap_type = ENCAP_TYPE_L2;
    sdk::lib::memrev(spec.data.u.l2_encap.dmac, (uint8_t*)ep_dmac,
                     sizeof(mac_addr_t));
    sdk::lib::memrev(spec.data.u.l2_encap.smac, (uint8_t*)ep_smac,
                     sizeof(mac_addr_t));
    spec.data.u.l2_encap.insert_vlan_tag = TRUE;
    spec.data.u.l2_encap.vlan_id = vnic_vlan;

    return (sdk_ret_t)pds_flow_session_rewrite_create(&spec);
}

sdk_ret_t
fte_s2h_nat_v4_session_rewrite_l2 (uint32_t session_rewrite_id,
                               uint16_t vnic_vlan,
                               pds_flow_session_rewrite_nat_type_t nat_type,
                               ipv4_addr_t ipv4_addr)
{
    pds_flow_session_rewrite_spec_t spec;

    memset(&spec, 0, sizeof(spec));
    spec.key.session_rewrite_id = session_rewrite_id;

    spec.data.strip_encap_header = TRUE;
    spec.data.strip_l2_header = TRUE;
    spec.data.strip_vlan_tag = TRUE;

    spec.data.nat_info.nat_type = nat_type;
    spec.data.nat_info.u.ipv4_addr = (uint32_t) ipv4_addr;

    spec.data.encap_type = ENCAP_TYPE_INSERT_CTAG;
    spec.data.u.insert_ctag.vlan_id = vnic_vlan;

    return (sdk_ret_t)pds_flow_session_rewrite_create(&spec);
}

static sdk_ret_t 
fte_setup_dnat_flow (flow_cache_policy_info_t *policy)
{
    sdk_ret_t ret = SDK_RET_OK;
    rewrite_underlay_info_t *rewrite_underlay;
    rewrite_host_info_t *rewrite_host;
    nat_info_t *nat_info;
    nat_map_tbl_t *nat_map_tbl;
    uint32_t local_ip, nat_ip;
    uint8_t map_cnt = 0;

    rewrite_underlay = &(policy->rewrite_underlay);
    rewrite_host = &(policy->rewrite_host);
    nat_info = &(policy->nat_info);
    nat_map_tbl = policy->nat_map_tbl;

    local_ip = nat_info->local_ip_lo;
    nat_ip = nat_info->nat_ip_lo;

    while (local_ip <= nat_info->local_ip_hi) {
        /* DNAT mapping */
        ret = fte_create_dnat_map_ipv4(policy->vnic_id, nat_ip,
                                       local_ip, 0);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_DEBUG("fte_create_dnat_map_ipv4 failed. \n");
            return ret;
        }

        nat_map_tbl[map_cnt].local_ip = local_ip;
        nat_map_tbl[map_cnt].nat_ip = nat_ip;

        nat_map_tbl[map_cnt].h2s_rewrite_id = g_session_rewrite_index++;
        if (rewrite_underlay->encap_type == ENCAP_MPLSOUDP) {
            ret = fte_h2s_nat_v4_session_rewrite_mplsoudp(
                        nat_map_tbl[map_cnt].h2s_rewrite_id,
                        (mac_addr_t *)rewrite_underlay->substrate_dmac,
                        (mac_addr_t *)rewrite_underlay->substrate_smac,
                        rewrite_underlay->substrate_vlan,
                        rewrite_underlay->substrate_sip,
                        rewrite_underlay->substrate_dip,
                        rewrite_underlay->u.mplsoudp.mpls_label1,
                        rewrite_underlay->u.mplsoudp.mpls_label2,
                        REWRITE_NAT_TYPE_IPV4_SNAT,
                        (ipv4_addr_t)nat_ip);
                            
            if (ret != SDK_RET_OK) {
                PDS_TRACE_DEBUG("fte_h2s_nat_v4_session_rewrite_mplsoudp "
                                "failed.\n");
                return ret;
            }
        } else if (rewrite_underlay->encap_type == ENCAP_GENEVE) {
            ret = fte_h2s_nat_v4_session_rewrite_geneve(
                        nat_map_tbl[map_cnt].h2s_rewrite_id,
                        (mac_addr_t *)rewrite_underlay->substrate_dmac,
                        (mac_addr_t *)rewrite_underlay->substrate_smac,
                        rewrite_underlay->substrate_vlan,
                        rewrite_underlay->substrate_sip,
                        rewrite_underlay->substrate_dip,
                        rewrite_underlay->u.geneve.vni,
                        policy->src_slot_id,
                        rewrite_underlay->u.geneve.dst_slot_id,
                        rewrite_underlay->u.geneve.sg_id1,
                        rewrite_underlay->u.geneve.sg_id2,
                        rewrite_underlay->u.geneve.sg_id3,
                        rewrite_underlay->u.geneve.sg_id4,
                        rewrite_underlay->u.geneve.sg_id5,
                        rewrite_underlay->u.geneve.sg_id6,
                        rewrite_underlay->u.geneve.orig_phy_ip,
                        REWRITE_NAT_TYPE_IPV4_SNAT,
                        (ipv4_addr_t)nat_ip);

            if (ret != SDK_RET_OK) {
                PDS_TRACE_DEBUG("fte_h2s_nat_v4_session_rewrite_geneve "
                                "failed.\n");
                return ret;
            }
        } else {
            PDS_TRACE_DEBUG("Unsupported encap_type:%u \n",
                            rewrite_underlay->encap_type);
            return SDK_RET_INVALID_OP;
        }

        nat_map_tbl[map_cnt].s2h_rewrite_id = g_session_rewrite_index++;
        if (policy->vnic_type == VNIC_L2) {
            ret = fte_s2h_nat_v4_session_rewrite_l2(
                    nat_map_tbl[map_cnt].s2h_rewrite_id,
                    policy->vlan_id,
                    REWRITE_NAT_TYPE_IPV4_DNAT,
                    (ipv4_addr_t)local_ip);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_DEBUG("fte_s2h_nat_v4_session_rewrite_l2 "
                                "failed.\n");
                return ret;
            }
        } else {
            ret = fte_s2h_nat_v4_session_rewrite(
                    nat_map_tbl[map_cnt].s2h_rewrite_id,
                    (mac_addr_t *)rewrite_host->ep_dmac,
                    (mac_addr_t *)rewrite_host->ep_smac,
                    policy->vlan_id,
                    REWRITE_NAT_TYPE_IPV4_DNAT,
                    (ipv4_addr_t)local_ip);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_DEBUG("fte_s2h_nat_v4_session_rewrite failed.\n");
                return ret;
            }
        }

        local_ip++;
        nat_ip++;
        map_cnt++;
    }

    policy->num_nat_mappings = map_cnt;
    return SDK_RET_OK;
}

sdk_ret_t 
fte_setup_v4_flows_json (void)
{
    sdk_ret_t ret;
    uint8_t v4_flows_cnt = 0;
    v4_flows_info_t *v4_flows;
    uint16_t vnic_id;
    uint32_t sip, dip;
    uint8_t proto;
    uint16_t sport, dport;
    uint32_t session_index;
    uint32_t h2s_rewrite_id;
    uint32_t s2h_rewrite_id;

    while (v4_flows_cnt < g_num_v4_flows) {
        v4_flows = &g_v4_flows[v4_flows_cnt];
        vnic_id = v4_flows->vnic_lo;
        proto = v4_flows->proto;
        PDS_TRACE_DEBUG("v4_flows[%u] VNIC: Lo-%u Hi-%u, SIP: Lo-0x%x Hi-0x%x "
               "DIP: Lo-0x%x Hi-0x%x SPORT: Lo-%u Hi-%u, DPORT: Lo-%u Hi-%u\n PROTO: %u\n",
               v4_flows_cnt, v4_flows->vnic_lo, v4_flows->vnic_hi,
               v4_flows->sip_lo, v4_flows->sip_hi, v4_flows->dip_lo, v4_flows->dip_hi,
               v4_flows->sport_lo, v4_flows->sport_hi, v4_flows->dport_lo, v4_flows->dport_hi,
               v4_flows->proto);
        while (vnic_id <= v4_flows->vnic_hi) {
            if (g_flow_cache_policy[vnic_id].vnic_id == 0) {
                vnic_id++;
                continue;
            }
            sip = v4_flows->sip_lo;
            while (sip <= v4_flows->sip_hi) {
                dip = v4_flows->dip_lo;
                while (dip <= v4_flows->dip_hi) {
                    sport = v4_flows->sport_lo;
                    while (sport <= v4_flows->sport_hi) {
                        dport = v4_flows->dport_lo;
                        while (dport <= v4_flows->dport_hi) {
                            ret = fte_get_session_rewrite_id(
                                    vnic_id, sip,
                                    &h2s_rewrite_id, &s2h_rewrite_id);
                            if (ret != SDK_RET_OK) {
                                PDS_TRACE_DEBUG(
                                    "fte_get_session_rewrite_id failed.\n");
                                return ret;
                            }

                            ret = fte_session_index_alloc(
                                    &session_index);
                            if (ret != SDK_RET_OK) {
                                PDS_TRACE_DEBUG(
                                    "fte_session_index_alloc failed.\n");
                                return ret;
                            }

                            ret = fte_session_info_create(
                                    session_index,
                                    0 /* conntrack_index */,
                                    0 /* h2s_flow_state_bmp */,
                                    0 /* s2h_flow_state_bmp */,
                                    h2s_rewrite_id, s2h_rewrite_id, 0, 
                                    EGRESS_ACTION_TX_TO_SWITCH, EGRESS_ACTION_TX_TO_HOST);
                            if (ret != SDK_RET_OK) {
                                PDS_TRACE_DEBUG(
                                    "fte_session_info_create failed.\n");
                                return ret;
                            }

                            ret = fte_flow_create(vnic_id, sip, dip,
                                    proto, sport, dport,
                                    PDS_FLOW_SPEC_INDEX_SESSION,
                                    session_index);
                            attempted_flows++;
                            if (ret != SDK_RET_OK) {
                                PDS_TRACE_DEBUG("Flow Create Fail: SrcIP:0x%x DstIP:0x%x "
                                    "Dport:%u Sport:%u Proto:%u "
                                    "VNICID:%u index:%u\n",
                                    sip, dip, dport, sport, proto,
                                    vnic_id, session_index);
                                // Even on collision/flow insert fail,
                                // continue the flow creation
                                // return ret;
                                dport++;
                                continue;
                            } else {
                                //PDS_TRACE_DEBUG("Created: SrcIP:0x%x DstIP:0x%x "
                                //    "Dport:%u Sport:%u Proto:%u "
                                //    "VNICID:%u index:%u\n",
                                //    sip, dip, dport, sport, proto,
                                //    vnic_id, session_index);
                            }
                            num_flows_added++;
                            dport++;
                        }
                        sport++;
                    }
                    dip++;
                }
                sip++;
            }
            vnic_id++;
        }
        v4_flows_cnt++;
    }

    PDS_TRACE_DEBUG("fte_setup_v4_flows_json: num_flows_added:%u,"
                    " attempted_flows:%u\n", num_flows_added, attempted_flows);
    return SDK_RET_OK;
}

static sdk_ret_t
fte_setup_flow (void)
{
    sdk_ret_t ret = SDK_RET_OK;
    flow_cache_policy_info_t *policy;
    rewrite_underlay_info_t *rewrite_underlay;
    rewrite_host_info_t *rewrite_host;
    uint32_t h2s_bmp_size = 0, s2h_bmp_size = 0;
    uint32_t bmp_size = 0;
    void *bmp_mem;
    uint16_t vnic_id;
    uint16_t i;

    if (g_flow_age_normal_tmo_set) {
        ret = (sdk_ret_t)pds_flow_age_normal_timeouts_set(&g_flow_age_normal_tmo);
        if (ret != SDK_RET_OK) {
            printf("pds_flow_age_normal_timeouts_set failed. \n");
        }
    }

    if (g_flow_age_accel_tmo_set) {
        ret = (sdk_ret_t)pds_flow_age_accel_timeouts_set(&g_flow_age_accel_tmo);
        if (ret != SDK_RET_OK) {
            printf("pds_flow_age_accel_timeouts_set failed. \n");
        }
    }

    for (i = 0; i < g_num_policies; i++) {
        vnic_id = g_vnic_id_list[i];
        policy = &(g_flow_cache_policy[vnic_id]);
        // Setup VNIC Mappings
        ret = fte_vlan_to_vnic_map(policy->vlan_id, vnic_id,
                                   policy->vnic_type);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_DEBUG("fte_vlan_to_vnic_map failed.\n");
            return ret;
        }

        // Setup VNIC Mappings
        ret = fte_mpls_label_to_vnic_map(policy->src_slot_id, vnic_id,
                                         policy->vnic_type);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_DEBUG("fte_mpls_label_to_vnic_map failed.\n");
            return ret;
        }

        if (policy->vnic_type == VNIC_L2) {
            h2s_bmp_size = ((policy->l2_flows_range.h2s_mac_hi -
                            policy->l2_flows_range.h2s_mac_lo) + 1);
            bmp_size = rte_bitmap_get_memory_footprint(h2s_bmp_size);
            bmp_mem = rte_zmalloc("l2_flows_range_bmp", bmp_size,
                                  RTE_CACHE_LINE_SIZE);
            if (bmp_mem == NULL) {
                PDS_TRACE_DEBUG("VNIC:%u l2_flows_range h2s bmp_mem "
                                "alloc failed. \n", vnic_id);
                return SDK_RET_ERR;
            }

            policy->l2_flows_range.h2s_bmp =
                rte_bitmap_init(h2s_bmp_size, (uint8_t *) bmp_mem,
                                bmp_size);
            if (policy->l2_flows_range.h2s_bmp == NULL) {
                PDS_TRACE_DEBUG("VNIC:%u l2_flows_range.h2s_bmp "
                               "init failed. \n", vnic_id);
                return SDK_RET_ERR;
            }
            rte_spinlock_init(&(policy->l2_flows_range.h2s_bmp_slock));

            bmp_size = 0;
            bmp_mem = NULL;
            s2h_bmp_size = ((policy->l2_flows_range.s2h_mac_hi -
                            policy->l2_flows_range.s2h_mac_lo) + 1);
            bmp_size = rte_bitmap_get_memory_footprint(s2h_bmp_size);
            bmp_mem = rte_zmalloc("l2_flows_range_bmp", bmp_size,
                                  RTE_CACHE_LINE_SIZE);
            if (bmp_mem == NULL) {
                PDS_TRACE_DEBUG("VNIC:%u l2_flows_range s2h bmp_mem "
                                "alloc failed. \n", vnic_id);
                return SDK_RET_ERR;
            }
            policy->l2_flows_range.s2h_bmp =
                rte_bitmap_init(s2h_bmp_size, (uint8_t *) bmp_mem,
                                bmp_size);
            if (policy->l2_flows_range.s2h_bmp == NULL) {
                PDS_TRACE_DEBUG("VNIC:%u l2_flows_range.s2h_bmp "
                                "init failed. \n", vnic_id);
                return SDK_RET_ERR;
            }
            rte_spinlock_init(&(policy->l2_flows_range.s2h_bmp_slock));
        }

        if (policy->nat_enabled) {
            ret = fte_setup_dnat_flow(policy);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_DEBUG("fte_setup_dnat_flow failed.\n");
                return ret;
           }
        }

        rewrite_underlay = &(policy->rewrite_underlay);
        rewrite_underlay->rewrite_id = g_session_rewrite_index++;
        if (rewrite_underlay->encap_type == ENCAP_MPLSOUDP) {
            ret = fte_h2s_v4_session_rewrite_mplsoudp(
                            rewrite_underlay->rewrite_id,
                            (mac_addr_t *)rewrite_underlay->substrate_dmac,
                            (mac_addr_t *)rewrite_underlay->substrate_smac,
                            rewrite_underlay->substrate_vlan,
                            rewrite_underlay->substrate_sip,
                            rewrite_underlay->substrate_dip,
                            rewrite_underlay->u.mplsoudp.mpls_label1,
                            rewrite_underlay->u.mplsoudp.mpls_label2);
                            
            if (ret != SDK_RET_OK) {
                PDS_TRACE_DEBUG("fte_h2s_v4_session_rewrite_mplsoudp "
                                "failed.\n");
                return ret;
            }
        } else if (rewrite_underlay->encap_type == ENCAP_GENEVE) {
            ret = fte_h2s_v4_session_rewrite_geneve(
                            rewrite_underlay->rewrite_id,
                            (mac_addr_t *)rewrite_underlay->substrate_dmac,
                            (mac_addr_t *)rewrite_underlay->substrate_smac,
                            rewrite_underlay->substrate_vlan,
                            rewrite_underlay->substrate_sip,
                            rewrite_underlay->substrate_dip,
                            rewrite_underlay->u.geneve.vni,
                            policy->src_slot_id,
                            rewrite_underlay->u.geneve.dst_slot_id,
                            rewrite_underlay->u.geneve.sg_id1,
                            rewrite_underlay->u.geneve.sg_id2,
                            rewrite_underlay->u.geneve.sg_id3,
                            rewrite_underlay->u.geneve.sg_id4,
                            rewrite_underlay->u.geneve.sg_id5,
                            rewrite_underlay->u.geneve.sg_id6,
                            rewrite_underlay->u.geneve.orig_phy_ip);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_DEBUG("fte_h2s_v4_session_rewrite_geneve "
                                "failed.\n");
                return ret;
            }
        } else {
            PDS_TRACE_DEBUG("Unsupported encap_type:%u \n",
                            rewrite_underlay->encap_type);
            return SDK_RET_INVALID_OP;
        }

        rewrite_host = &(policy->rewrite_host);
        rewrite_host->rewrite_id = g_session_rewrite_index++;

        if (policy->vnic_type == VNIC_L2) {
            ret = fte_s2h_v4_session_rewrite_l2(
                            rewrite_host->rewrite_id,
                            policy->vlan_id);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_DEBUG("fte_s2h_v4_session_rewrite_l2 "
                                "failed.\n");
                return ret;
            }
        } else {
            ret = fte_s2h_v4_session_rewrite(
                            rewrite_host->rewrite_id,
                            (mac_addr_t *)rewrite_host->ep_dmac,
                            (mac_addr_t *)rewrite_host->ep_smac,
                            policy->vlan_id);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_DEBUG("fte_s2h_v4_session_rewrite failed.\n");
                return ret;
            }
        }
    }

    ret = fte_setup_v4_flows_json();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_DEBUG("fte_setup_v4_flows_json failed.\n");
        return ret;
    }

    return ret;
}

pds_ret_t
fte_flows_aging_expiry_fn(uint32_t expiry_id,
                          pds_flow_age_expiry_type_t expiry_type,
                          void *user_ctx,
                          uint32_t *ret_handle)
{
    uint32_t    handle = 0;
    pds_ret_t   ret = PDS_RET_OK;

    switch (expiry_type) {

    case EXPIRY_TYPE_SESSION:
        ret = (*aging_expiry_dflt_fn)(expiry_id, expiry_type,
                                      user_ctx, &handle);

        /*
         * Let aging scanners find the entry again if retry needed.
         */
        if (ret != PDS_RET_RETRY) {
            fte_session_index_free(expiry_id);
        }
        break;

    case EXPIRY_TYPE_CONNTRACK:
        ret = (*aging_expiry_dflt_fn)(expiry_id, expiry_type,
                                      user_ctx, &handle);
        if (ret != PDS_RET_RETRY) {
            if (handle) {
                fte_session_index_free(handle);
            }

            /*
             * If FTE also maintains a bitmap of allocated conntrack IDs,
             * the expiry_id should be released here.
             */
            fte_conntrack_index_free(expiry_id);
        }
        break;

    default:
        ret = PDS_RET_INVALID_ARG;
        break;
    }
    return ret;
}

sdk_ret_t
fte_flows_init ()
{
    sdk_ret_t sdk_ret = SDK_RET_OK;

    sdk_ret = fte_conntrack_indexer_init();
    if (sdk_ret != SDK_RET_OK) {
        PDS_TRACE_DEBUG("fte_conntrack_indexer_init failed.\n");
        return sdk_ret;
    }

    sdk_ret = fte_session_indexer_init();
    if (sdk_ret != SDK_RET_OK) {
        PDS_TRACE_DEBUG("fte_session_indexer_init failed.\n");
        return sdk_ret;
    }

    /*
     * Override the default aging callback so session indices can be managed
     */
    sdk_ret = (sdk_ret_t)pds_flow_age_sw_pollers_expiry_fn_dflt(&aging_expiry_dflt_fn);
    if (sdk_ret == SDK_RET_OK) {
        sdk_ret = (sdk_ret_t)pds_flow_age_sw_pollers_poll_control(false,
                                                     fte_flows_aging_expiry_fn);
    }
    if (sdk_ret != SDK_RET_OK) {
        PDS_TRACE_ERR("fte aging callback override failed.\n");
        return sdk_ret;
    }

    if (!skip_fte_flow_prog()) {
        if ((sdk_ret = fte_setup_flow()) != SDK_RET_OK) {
            PDS_TRACE_DEBUG("fte_setup_flow failed.\n");
            return sdk_ret;
        } else {
            PDS_TRACE_DEBUG("fte_setup_flow success.\n");
        }
    }

    return sdk_ret;
}

} // namespace fte
