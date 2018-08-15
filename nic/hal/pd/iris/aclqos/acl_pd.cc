// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
#include "nic/hal/plugins/cfg/nw/vrf.hpp"
#include "nic/hal/pd/iris/nw/vrf_pd.hpp"
#include "nic/include/hal_lock.hpp"
#include "nic/hal/pd/iris/hal_state_pd.hpp"
#include "nic/hal/pd/iris/aclqos/acl_pd.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/include/acl_api.hpp"
#include "nic/build/iris/gen/datapath/p4/include/p4pd.h"
#include "nic/hal/iris/datapath/p4/include/defines.h"
#include "nic/hal/pd/iris/nw/if_pd_utils.hpp"
#include "nic/hal/plugins/cfg/nw/l2segment.hpp"
#include "nic/hal/pd/iris/nw/l2seg_pd.hpp"
#include "nic/hal/plugins/cfg/nw/l2segment_api.hpp"
#include "nic/include/eth.hpp"
#include "nic/hal/pd/iris/internal/copp_pd.hpp"

namespace hal {
namespace pd {
// ----------------------------------------------------------------------------
// Linking PI <-> PD
// ----------------------------------------------------------------------------
static void
acl_pd_link_pi_pd (pd_acl_t *pd_acl, acl_t *pi_acl)
{
    pd_acl->pi_acl = pi_acl;
    pi_acl->pd = pd_acl;
}

// ----------------------------------------------------------------------------
// Un-Linking PI <-> PD
// ----------------------------------------------------------------------------
static void
acl_pd_delink_pi_pd (pd_acl_t *pd_acl, acl_t *pi_acl)
{
    if (pd_acl) {
        pd_acl->pi_acl = NULL;
    }
    if (pi_acl) {
        pi_acl->pd = NULL;
    }
}

// ----------------------------------------------------------------------------
// Allocate resources for PD Acl
// ----------------------------------------------------------------------------
static hal_ret_t
acl_pd_alloc_res (pd_acl_t *pd_acl)
{
    // Allocate any hardware resources
    return HAL_RET_OK;
}

// ----------------------------------------------------------------------------
// Deallocate resources for Acl
// ----------------------------------------------------------------------------
static hal_ret_t
acl_pd_dealloc_res (pd_acl_t *pd_acl)
{
    // Deallocate any hardware resources
    return HAL_RET_OK;
}

static void
populate_ip_common (nacl_swkey_t *key, nacl_swkey_mask_t *mask,
                    acl_ip_match_spec_t *ip_key, acl_ip_match_spec_t *ip_mask)
{
    key->flow_lkp_metadata_lkp_proto = ip_key->ip_proto;
    mask->flow_lkp_metadata_lkp_proto_mask = ip_mask->ip_proto;

    switch(ip_key->ip_proto) {
        case IP_PROTO_ICMP:
        case IP_PROTO_ICMPV6:
            key->flow_lkp_metadata_lkp_dport =
                (ip_key->u.icmp.icmp_type << 8) | ip_key->u.icmp.icmp_code;
            mask->flow_lkp_metadata_lkp_dport_mask =
                (ip_mask->u.icmp.icmp_type << 8) | ip_mask->u.icmp.icmp_code;
            break;

        case IP_PROTO_TCP:
            key->flow_lkp_metadata_lkp_sport = ip_key->u.tcp.sport;
            mask->flow_lkp_metadata_lkp_sport_mask = ip_mask->u.tcp.sport;

            key->flow_lkp_metadata_lkp_dport = ip_key->u.tcp.dport;
            mask->flow_lkp_metadata_lkp_dport_mask = ip_mask->u.tcp.dport;

            key->tcp_flags = ip_key->u.tcp.tcp_flags;
            mask->tcp_flags_mask = ip_mask->u.tcp.tcp_flags;
            break;

        case IP_PROTO_UDP:
            key->flow_lkp_metadata_lkp_sport = ip_key->u.udp.sport;
            mask->flow_lkp_metadata_lkp_sport_mask = ip_mask->u.udp.sport;

            key->flow_lkp_metadata_lkp_dport = ip_key->u.udp.dport;
            mask->flow_lkp_metadata_lkp_dport_mask = ip_mask->u.udp.dport;
            break;

        default:
            break;
    }
}

static void
populate_permit_actions (nacl_actiondata *data, acl_action_spec_t *as)
{
    // TODO Get the index from mirror session
    // handles
    data->nacl_action_u.nacl_nacl_permit.ingress_mirror_en =
        as->ing_mirror_en;
    data->nacl_action_u.nacl_nacl_permit.egress_mirror_en =
        as->egr_mirror_en;
    data->nacl_action_u.nacl_nacl_permit.ingress_mirror_session_id =
        as->ing_mirror_session_handle;
    data->nacl_action_u.nacl_nacl_permit.egress_mirror_session_id =
        as->egr_mirror_session_handle;

    data->nacl_action_u.nacl_nacl_permit.ingress_mirror_session_id =
        as->ing_mirror_session;
    data->nacl_action_u.nacl_nacl_permit.egress_mirror_session_id =
        as->egr_mirror_session;
}

#ifdef ACL_DOL_TEST_ONLY
// Added for internal dol test use only to get the drop reason defines
// TODO: REMOVE

static uint64_t
drop_reason_codes_to_bitmap (drop_reason_codes_t *codes)
{
    uint64_t val = 0;
    if (codes->drop_malformed_pkt) val |= 1ull << DROP_MALFORMED_PKT;
    if (codes->drop_input_mapping) val |= 1ull << DROP_INPUT_MAPPING;
    if (codes->drop_input_mapping_dejavu) val |= 1ull << DROP_INPUT_MAPPING_DEJAVU;
    if (codes->drop_flow_hit) val |= 1ull << DROP_FLOW_HIT;
    if (codes->drop_flow_miss) val |= 1ull << DROP_FLOW_MISS;
    if (codes->drop_nacl) val |= 1ull << DROP_NACL;
    if (codes->drop_ipsg) val |= 1ull << DROP_IPSG;
    if (codes->drop_ip_normalization) val |= 1ull << DROP_IP_NORMALIZATION;
    if (codes->drop_tcp_normalization) val |= 1ull << DROP_TCP_NORMALIZATION;
    if (codes->drop_tcp_rst_with_invalid_ack_num) val |= 1ull << DROP_TCP_RST_WITH_INVALID_ACK_NUM;
    if (codes->drop_tcp_non_syn_first_pkt) val |= 1ull << DROP_TCP_NON_SYN_FIRST_PKT;
    if (codes->drop_icmp_normalization) val |= 1ull << DROP_ICMP_NORMALIZATION;
    if (codes->drop_input_properties_miss) val |= 1ull << DROP_INPUT_PROPERTIES_MISS;
    if (codes->drop_tcp_out_of_window) val |= 1ull << DROP_TCP_OUT_OF_WINDOW;
    if (codes->drop_tcp_split_handshake) val |= 1ull << DROP_TCP_SPLIT_HANDSHAKE;
    if (codes->drop_tcp_win_zero_drop) val |= 1ull << DROP_TCP_WIN_ZERO_DROP;
    if (codes->drop_tcp_data_after_fin) val |= 1ull << DROP_TCP_DATA_AFTER_FIN;
    if (codes->drop_tcp_non_rst_pkt_after_rst) val |= 1ull << DROP_TCP_NON_RST_PKT_AFTER_RST;
    if (codes->drop_tcp_invalid_responder_first_pkt) val |= 1ull << DROP_TCP_INVALID_RESPONDER_FIRST_PKT;
    if (codes->drop_tcp_unexpected_pkt) val |= 1ull << DROP_TCP_UNEXPECTED_PKT;
    if (codes->drop_src_lif_mismatch) val |= 1ull << DROP_SRC_LIF_MISMATCH;
    if (codes->drop_parser_icrc_error) val |= 1ull << DROP_PARSER_ICRC_ERR;
    if (codes->drop_parse_len_error) val |= 1ull << DROP_PARSER_LEN_ERR;
    if (codes->drop_hardware_error) val |= 1ull << DROP_HARDWARE_ERR;
    return val;
}

#endif

static hal_ret_t
acl_pd_pgm_acl_tbl (pd_acl_t *pd_acl, bool update, 
                    bool is_restore, uint32_t restore_tcam_index)
{
    hal_ret_t                              ret = HAL_RET_OK;
    acl_tcam                               *acl_tbl = NULL;
    nacl_swkey_t                           key;
    nacl_swkey_mask_t                      mask;
    nacl_actiondata                        data;
    acl_match_spec_t                       *ms = NULL;
    acl_action_spec_t                      *as = NULL;
    acl_eth_match_spec_t                   *eth_key;
    acl_eth_match_spec_t                   *eth_mask;
    acl_ip_match_spec_t                    *ip_key;
    acl_ip_match_spec_t                    *ip_mask;
    acl_t                                  *pi_acl = pd_acl->pi_acl;
    vrf_t                                  *vrf = NULL;
    l2seg_t                                *l2seg = NULL;
    if_t                                   *redirect_if = NULL;
    uint16_t                               l2seg_mask = 0;
    uint16_t                               ten_mask = 0;
    uint8_t                                ten_shift = 0;
    uint32_t                               qid;
    copp_t                                 *copp;
    uint32_t                               copp_index = 0;

    ms = acl_get_match_spec(pi_acl);
    as = acl_get_action_spec(pi_acl);

    eth_key = &ms->key.eth;
    eth_mask = &ms->mask.eth;
    ip_key = &ms->key.ip;
    ip_mask = &ms->mask.ip;

    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));

    copp = find_copp_by_handle(as->copp_handle);
    if (copp) {
        copp_index = copp->pd->hw_policer_id;
    }

    // Populate the data
    switch(as->action) {
        case acl::ACL_ACTION_LOG:
            if (!copp) {
                HAL_TRACE_ERR("Acl {} copp not specified for log", pi_acl->key);
                return HAL_RET_INVALID_ARG;
            }

            data.actionid = NACL_NACL_PERMIT_ID;
            data.nacl_action_u.nacl_nacl_permit.log_en = 1;
            qid = types::CPUCB_ID_NACL_LOG;
#ifdef ACL_DOL_TEST_ONLY
            if (as->int_as.qid_en) {
                data.nacl_action_u.nacl_nacl_permit.qid = as->int_as.qid;
            }
#endif
            data.nacl_action_u.nacl_nacl_permit.qid_en = 1;
            data.nacl_action_u.nacl_nacl_permit.qid = qid;

            data.nacl_action_u.nacl_nacl_permit.policer_index = copp_index;

            populate_permit_actions(&data, as);
            break;
        case acl::ACL_ACTION_REDIRECT:
            data.actionid = NACL_NACL_PERMIT_ID;

            redirect_if = find_if_by_handle(as->redirect_if_handle);
            data.nacl_action_u.nacl_nacl_permit.dst_lport_en = 1;
            data.nacl_action_u.nacl_nacl_permit.dst_lport = if_get_lport_id(redirect_if);

            if (if_is_cpu_if(redirect_if)) {
                // If going to CPU, do not do any rewrites on packet. So set
                // the rewrite indexes to 0 (nop)
                if (!copp) {
                    HAL_TRACE_ERR("Acl {} copp not specified for redirect to cpu",
                                  pi_acl->key);
                    return HAL_RET_INVALID_ARG;
                }

                qid = types::CPUCB_ID_NACL_REDIRECT;
#ifdef ACL_DOL_TEST_ONLY
                if (as->int_as.qid_en) {
                    qid = as->int_as.qid;
                }
#endif
                data.nacl_action_u.nacl_nacl_permit.qid_en = 1;
                data.nacl_action_u.nacl_nacl_permit.qid = qid;
                data.nacl_action_u.nacl_nacl_permit.force_flow_hit = 1;
                data.nacl_action_u.nacl_nacl_permit.discard_drop = 1;
                data.nacl_action_u.nacl_nacl_permit.rewrite_en = 1;
                data.nacl_action_u.nacl_nacl_permit.rewrite_index = 0;
                data.nacl_action_u.nacl_nacl_permit.rewrite_flags = 0;
                data.nacl_action_u.nacl_nacl_permit.tunnel_rewrite_en = 1;
                data.nacl_action_u.nacl_nacl_permit.tunnel_rewrite_index = 0;
                data.nacl_action_u.nacl_nacl_permit.tunnel_vnid = 0;
                data.nacl_action_u.nacl_nacl_permit.tunnel_originate = 0;

                data.nacl_action_u.nacl_nacl_permit.policer_index = copp_index;
            } else {
                // TODO: Figure out how to get these values
                data.nacl_action_u.nacl_nacl_permit.force_flow_hit = 0;
                data.nacl_action_u.nacl_nacl_permit.discard_drop = 0;
                data.nacl_action_u.nacl_nacl_permit.rewrite_en = 0;
                data.nacl_action_u.nacl_nacl_permit.rewrite_index = 0;
                data.nacl_action_u.nacl_nacl_permit.rewrite_flags = 0;
                data.nacl_action_u.nacl_nacl_permit.tunnel_rewrite_en = 0;
                data.nacl_action_u.nacl_nacl_permit.tunnel_rewrite_index = 0;
                data.nacl_action_u.nacl_nacl_permit.tunnel_vnid = 0;
                data.nacl_action_u.nacl_nacl_permit.tunnel_originate = 0;
#ifdef ACL_DOL_TEST_ONLY
                data.nacl_action_u.nacl_nacl_permit.force_flow_hit = 1;
                data.nacl_action_u.nacl_nacl_permit.discard_drop = 1;
                data.nacl_action_u.nacl_nacl_permit.rewrite_en = 1;
                data.nacl_action_u.nacl_nacl_permit.rewrite_index =
                    as->int_as.rw_idx;
                data.nacl_action_u.nacl_nacl_permit.rewrite_flags =
                    (as->int_as.mac_sa_rewrite ? REWRITE_FLAGS_MAC_SA : 0) |
                    (as->int_as.mac_da_rewrite ? REWRITE_FLAGS_MAC_DA : 0) |
                    (as->int_as.ttl_dec ? REWRITE_FLAGS_TTL_DEC : 0);

                data.nacl_action_u.nacl_nacl_permit.tunnel_rewrite_en = 1;
                if (if_is_tunnel_if(redirect_if)) {
                    data.nacl_action_u.nacl_nacl_permit.tunnel_rewrite_index =
                        (tunnelif_get_rw_idx((pd_tunnelif_t *)redirect_if->pd_if));
                    data.nacl_action_u.nacl_nacl_permit.tunnel_vnid =
                        as->int_as.tnnl_vnid;
                    data.nacl_action_u.nacl_nacl_permit.tunnel_originate = 1;
                } else {
                    // support only non-native segments for DOL testing
                    data.nacl_action_u.nacl_nacl_permit.tunnel_vnid =
                        as->int_as.tnnl_vnid;
                    data.nacl_action_u.nacl_nacl_permit.tunnel_rewrite_index =
                        g_hal_state_pd->tnnl_rwr_tbl_encap_vlan_idx();
                }
#endif
            }

            populate_permit_actions(&data, as);
            break;
        case acl::ACL_ACTION_PERMIT:
            data.actionid = NACL_NACL_PERMIT_ID;
            populate_permit_actions(&data, as);
            break;
        case acl::ACL_ACTION_DENY:
            data.actionid = NACL_NACL_DENY_ID;
            break;
        default:
            data.actionid = NACL_NOP_ID;
            break;
    }

    // Populate the Key and Mask
    key.entry_inactive_nacl = 0;
    mask.entry_inactive_nacl_mask = 0x1;

    // Match only when from_cpu is 0
    key.control_metadata_from_cpu = 0;
    mask.control_metadata_from_cpu_mask = 1;

    pd_get_l2seg_ten_masks(&l2seg_mask, &ten_mask, &ten_shift);
    if (ms->vrf_match) {
        vrf = vrf_lookup_by_handle(ms->vrf_handle);
        key.flow_lkp_metadata_lkp_vrf =
            ((pd_vrf_t *)(vrf->pd))->vrf_hw_id << ten_shift;
        mask.flow_lkp_metadata_lkp_vrf_mask = ten_mask;
    } else if (ms->l2seg_match) {
        l2seg = l2seg_lookup_by_handle(ms->l2seg_handle);
        key.flow_lkp_metadata_lkp_vrf =
            ((pd_l2seg_t *)(l2seg->pd))->l2seg_fl_lkup_id;
        mask.flow_lkp_metadata_lkp_vrf_mask = ten_mask | l2seg_mask;
    }

    if (ms->src_if_match) {
        key.control_metadata_src_lport =
            if_get_lport_id(find_if_by_handle(ms->src_if_handle));
        mask.control_metadata_src_lport_mask =
            ~(mask.control_metadata_src_lport_mask & 0);
    }

    if (ms->dest_if_match) {
        key.control_metadata_dst_lport =
            if_get_lport_id(find_if_by_handle(ms->dest_if_handle));
        mask.control_metadata_dst_lport_mask =
            ~(mask.control_metadata_dst_lport_mask & 0);
    }

    switch(ms->acl_type) {
        case ACL_TYPE_NONE:
            // Nothing to do
            break;
        case ACL_TYPE_ETH:
            // Mac address needs to be in little-endian format while
            // passing to p4pd. So convert to uint64 and do a memcpy
            key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_MAC;
            mask.flow_lkp_metadata_lkp_type_mask =
                ~(mask.flow_lkp_metadata_lkp_type_mask & 0);

            memcpy(key.flow_lkp_metadata_lkp_src, eth_key->mac_sa, sizeof(mac_addr_t));
            memrev(key.flow_lkp_metadata_lkp_src, sizeof(mac_addr_t));
            memcpy(mask.flow_lkp_metadata_lkp_src_mask, eth_mask->mac_sa, sizeof(mac_addr_t));
            memrev(mask.flow_lkp_metadata_lkp_src_mask, sizeof(mac_addr_t));

            memcpy(key.flow_lkp_metadata_lkp_dst, eth_key->mac_da, sizeof(mac_addr_t));
            memrev(key.flow_lkp_metadata_lkp_dst, sizeof(mac_addr_t));
            memcpy(mask.flow_lkp_metadata_lkp_dst_mask, eth_mask->mac_da, sizeof(mac_addr_t));
            memrev(mask.flow_lkp_metadata_lkp_dst_mask, sizeof(mac_addr_t));

            key.flow_lkp_metadata_lkp_dport = eth_key->ether_type;
            mask.flow_lkp_metadata_lkp_dport_mask = eth_mask->ether_type;
            break;
        case ACL_TYPE_IP:
            key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_IPV4 &
                FLOW_KEY_LOOKUP_TYPE_IPV6;
            mask.flow_lkp_metadata_lkp_type_mask = ~(FLOW_KEY_LOOKUP_TYPE_IPV4 ^
                                                     FLOW_KEY_LOOKUP_TYPE_IPV6);
            populate_ip_common(&key, &mask, ip_key, ip_mask);
            break;
        case ACL_TYPE_IPv4:
            key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_IPV4;
            mask.flow_lkp_metadata_lkp_type_mask =
                ~(mask.flow_lkp_metadata_lkp_type_mask & 0);

            memcpy(key.flow_lkp_metadata_lkp_src,
                   &ip_key->sip.addr.v4_addr,
                   sizeof(ipv4_addr_t));
            memcpy(mask.flow_lkp_metadata_lkp_src_mask,
                   &ip_mask->sip.addr.v4_addr,
                   sizeof(ipv4_addr_t));

            memcpy(key.flow_lkp_metadata_lkp_dst,
                   &ip_key->dip.addr.v4_addr,
                   sizeof(ipv4_addr_t));
            memcpy(mask.flow_lkp_metadata_lkp_dst_mask,
                   &ip_mask->dip.addr.v4_addr,
                   sizeof(ipv4_addr_t));

            populate_ip_common(&key, &mask, ip_key, ip_mask);
            break;
        case ACL_TYPE_IPv6:
            key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_IPV6;
            mask.flow_lkp_metadata_lkp_type_mask =
                ~(mask.flow_lkp_metadata_lkp_type_mask & 0);

            memcpy(key.flow_lkp_metadata_lkp_src,
                   ip_key->sip.addr.v6_addr.addr8,
                   IP6_ADDR8_LEN);
            memrev(key.flow_lkp_metadata_lkp_src, sizeof(key.flow_lkp_metadata_lkp_src));
            memcpy(mask.flow_lkp_metadata_lkp_src_mask,
                   ip_mask->sip.addr.v6_addr.addr8,
                   IP6_ADDR8_LEN);
            memrev(mask.flow_lkp_metadata_lkp_src_mask, sizeof(mask.flow_lkp_metadata_lkp_src_mask));

            memcpy(key.flow_lkp_metadata_lkp_dst,
                   ip_key->dip.addr.v6_addr.addr8,
                   IP6_ADDR8_LEN);
            memrev(key.flow_lkp_metadata_lkp_dst, sizeof(key.flow_lkp_metadata_lkp_dst));
            memcpy(mask.flow_lkp_metadata_lkp_dst_mask,
                   ip_mask->dip.addr.v6_addr.addr8,
                   IP6_ADDR8_LEN);
            memrev(mask.flow_lkp_metadata_lkp_dst_mask, sizeof(mask.flow_lkp_metadata_lkp_dst_mask));

            populate_ip_common(&key, &mask, ip_key, ip_mask);
            break;
        case ACL_TYPE_INVALID:
            return HAL_RET_ERR;
    }
    // TODO:Populate the below values
    key.control_metadata_flow_miss_ingress = 0;
    mask.control_metadata_flow_miss_ingress_mask = 0;
    key.ethernet_dstAddr[0] = 0;
    mask.ethernet_dstAddr_mask[0] = 0;
    key.l3_metadata_ip_option_seen = 0;
    mask.l3_metadata_ip_option_seen_mask = 0;
    key.l3_metadata_ip_frag = 0;
    mask.l3_metadata_ip_frag_mask = 0;
    memset(key.control_metadata_drop_reason, 0,
           sizeof(key.control_metadata_drop_reason));
    memset(mask.control_metadata_drop_reason_mask, 0,
           sizeof(mask.control_metadata_drop_reason_mask));
    key.flow_lkp_metadata_lkp_dir = 0;
    mask.flow_lkp_metadata_lkp_dir_mask = 0;

#ifdef ACL_DOL_TEST_ONLY
    // Key of internal fields for use only with DOL/testing infra
    // For production builds this needs to be removed
    // TODO: REMOVE
    key.control_metadata_flow_miss_ingress = ms->int_key.flow_miss;
    mask.control_metadata_flow_miss_ingress_mask = ms->int_mask.flow_miss;
    memcpy(key.ethernet_dstAddr, ms->int_key.outer_mac_da, sizeof(mac_addr_t));
    memrev(key.ethernet_dstAddr, sizeof(mac_addr_t));
    memcpy(mask.ethernet_dstAddr_mask, ms->int_mask.outer_mac_da, sizeof(mac_addr_t));
    memrev(mask.ethernet_dstAddr_mask, sizeof(mac_addr_t));
    key.l3_metadata_ip_option_seen = ms->int_key.ip_options;
    mask.l3_metadata_ip_option_seen_mask = ms->int_mask.ip_options;
    key.l3_metadata_ip_frag = ms->int_key.ip_frag;
    mask.l3_metadata_ip_frag_mask = ms->int_mask.ip_frag;

    uint64_t drop_reason = 0;
    uint64_t drop_reason_mask = 0;


    if (ms->int_key.no_drop) {
        drop_reason = 0;
        drop_reason_mask = ~0;
    } else {
        drop_reason = drop_reason_codes_to_bitmap(&ms->int_key.drop_reasons);
        drop_reason_mask = drop_reason_codes_to_bitmap(&ms->int_mask.drop_reasons);
    }
    memcpy(key.control_metadata_drop_reason, &drop_reason,
           sizeof(key.control_metadata_drop_reason));
    memcpy(mask.control_metadata_drop_reason_mask, &drop_reason_mask,
           sizeof(mask.control_metadata_drop_reason_mask));
    key.flow_lkp_metadata_lkp_dir = ms->int_key.direction;
    mask.flow_lkp_metadata_lkp_dir_mask = ms->int_mask.direction;
    if (ms->int_mask.from_cpu) {
        key.control_metadata_from_cpu = ms->int_key.from_cpu;
        mask.control_metadata_from_cpu_mask = ms->int_mask.from_cpu;
    }
#endif

    acl_tbl = g_hal_state_pd->acl_table();
    HAL_ASSERT_RETURN((acl_tbl != NULL), HAL_RET_ERR);

    // Insert the entry
    if (is_restore) {
        ret = acl_tbl->insert_withid(&key, &mask, &data, acl_get_priority(pi_acl), 
                                     restore_tcam_index, &pd_acl->handle);
    } else if (update) {
        ret = acl_tbl->update(pd_acl->handle, &key, &mask, &data,
                              acl_get_priority(pi_acl));
    } else {
        ret = acl_tbl->insert(&key, &mask, &data, acl_get_priority(pi_acl), &pd_acl->handle);
    }
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Unable to program for nacl: {}, ret: {}",
                      pd_acl->pi_acl->key, ret);
    } else {
        HAL_TRACE_DEBUG("Programmed for nacl: {}, ret: {}",
                        pd_acl->pi_acl->key, ret);
    }

    return ret;
}

static hal_ret_t
acl_pd_cleanup_acl_tbl (pd_acl_t *pd_acl)
{
    hal_ret_t ret = HAL_RET_OK;
    acl_tcam  *acl_tbl = NULL;

    acl_tbl = g_hal_state_pd->acl_table();
    HAL_ASSERT_RETURN((acl_tbl != NULL), HAL_RET_ERR);

    ret = acl_tbl->remove(pd_acl->handle);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Unable to cleanup for acl: {}",
                      pd_acl->pi_acl->key);
    } else {
        HAL_TRACE_DEBUG("Programmed cleanup acl: {}",
                        pd_acl->pi_acl->key);
    }

    return ret;
}

static hal_ret_t
acl_pd_deprogram_hw (pd_acl_t *pd_acl)
{
    hal_ret_t   ret;
    ret = acl_pd_cleanup_acl_tbl(pd_acl);

    return ret;
}

// ----------------------------------------------------------------------------
// Program HW
// ----------------------------------------------------------------------------
static hal_ret_t
acl_pd_program_hw (pd_acl_t *pd_acl, bool update,
                   bool is_restore = false, uint32_t restore_tcam_index = 0)
{
    hal_ret_t   ret;

    ret = acl_pd_pgm_acl_tbl(pd_acl, update, is_restore, restore_tcam_index);
    return ret;
}

//-----------------------------------------------------------------------------
// PD Acl Cleanup
//  - Release all resources
//  - Delink PI <-> PD
//  - Free PD Acl
//  Note:
//      - Just free up whatever PD has.
//      - Dont use this inplace of delete. Delete may result in giving callbacks
//        to others.
//-----------------------------------------------------------------------------
static hal_ret_t
acl_pd_cleanup (pd_acl_t *pd_acl)
{
    hal_ret_t       ret = HAL_RET_OK;

    if (!pd_acl) {
        // Nothing to do
        goto end;
    }

    if (pd_acl->handle != ACL_TCAM_ENTRY_INVALID_HANDLE) {
        ret = acl_pd_deprogram_hw(pd_acl);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("unable to deprogram hw for acl: {}",
                          pd_acl->pi_acl->key);
            goto end;
        }
    }

    // Releasing resources
    ret = acl_pd_dealloc_res(pd_acl);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("unable to dealloc res for acl: {}",
                      pd_acl->pi_acl->key);
        goto end;
    }

    // Delinking PI<->PD
    acl_pd_delink_pi_pd(pd_acl, pd_acl->pi_acl);

    // Freeing PD
    acl_pd_free(&pd_acl);
end:
    return ret;
}


// ----------------------------------------------------------------------------
//  Acl Update
// ----------------------------------------------------------------------------
hal_ret_t
pd_acl_update (pd_func_args_t *pd_func_args)
{
    hal_ret_t ret = HAL_RET_OK;
    pd_acl_update_args_t *args = pd_func_args->pd_acl_update;
    pd_acl_t  *pd_acl;

    HAL_ASSERT_RETURN((args != NULL), HAL_RET_INVALID_ARG);
    HAL_ASSERT_RETURN((args->acl != NULL), HAL_RET_INVALID_ARG);
    HAL_ASSERT_RETURN((args->acl->pd != NULL), HAL_RET_INVALID_ARG);

    HAL_TRACE_DEBUG("updating pd state for acl:{}",
                    args->acl->key);

    pd_acl = (pd_acl_t *)args->acl->pd;

    ret = acl_pd_program_hw(pd_acl, true);
    return ret;
}

// ----------------------------------------------------------------------------
// Delete a PD ACL and remove from hardware
// ----------------------------------------------------------------------------
hal_ret_t
pd_acl_delete (pd_func_args_t *pd_func_args)
{
    hal_ret_t ret = HAL_RET_OK;
    pd_acl_delete_args_t *args = pd_func_args->pd_acl_delete;
    pd_acl_t *pd_acl;

    HAL_ASSERT_RETURN((args != NULL), HAL_RET_INVALID_ARG);
    HAL_ASSERT_RETURN((args->acl != NULL), HAL_RET_INVALID_ARG);
    HAL_ASSERT_RETURN((args->acl->pd != NULL), HAL_RET_INVALID_ARG);
    HAL_TRACE_DEBUG("deleting pd state for acl {}",
                    args->acl->key);
    pd_acl = (pd_acl_t *)args->acl->pd;

    // free up the resource and memory
    ret = acl_pd_cleanup(pd_acl);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("failed pd acl cleanup {}, ret {}",
                      args->acl->key, ret);
    }

    return ret;
}

// ----------------------------------------------------------------------------
// Create a PD ACL and add it to hardware
// ----------------------------------------------------------------------------
hal_ret_t
pd_acl_create (pd_func_args_t *pd_func_args)
{
    hal_ret_t ret = HAL_RET_OK;
    pd_acl_create_args_t *args = pd_func_args->pd_acl_create;
    pd_acl_t  *pd_acl;

    HAL_TRACE_DEBUG("Creating pd state for acl: {}",
                    args->acl->key);

    // Create acl PD
    pd_acl = acl_pd_alloc_init();
    if (pd_acl == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }

    // Link PI & PD
    acl_pd_link_pi_pd(pd_acl, args->acl);

    // Allocate Resources
    ret = acl_pd_alloc_res(pd_acl);
    if (ret != HAL_RET_OK) {
        // No Resources, dont allocate PD
        HAL_TRACE_ERR("Unable to alloc. resources for acl: {}",
                      args->acl->key);
        goto end;
    }

    // Program HW
    ret = acl_pd_program_hw(pd_acl, false);

end:
    if (ret != HAL_RET_OK) {
        acl_pd_cleanup(pd_acl);
    }
    return ret;
}

// ----------------------------------------------------------------------------
// Makes a clone
// ----------------------------------------------------------------------------
hal_ret_t
pd_acl_make_clone (pd_func_args_t *pd_func_args)
{
    hal_ret_t ret = HAL_RET_OK;
    pd_acl_make_clone_args_t *args = pd_func_args->pd_acl_make_clone;
    pd_acl_t *pd_acl_clone = NULL;
    acl_t *acl = args->acl;
    acl_t *clone = args->clone;

    pd_acl_clone = acl_pd_alloc_init();
    if (pd_acl_clone == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }

    memcpy(pd_acl_clone, acl->pd, sizeof(pd_acl_t));

    acl_pd_link_pi_pd(pd_acl_clone, clone);

end:
    return ret;
}

// ----------------------------------------------------------------------------
// Frees PD memory without indexer free.
// ----------------------------------------------------------------------------
hal_ret_t
pd_acl_mem_free (pd_func_args_t *pd_func_args)
{
    pd_acl_t        *pd_acl;
    pd_acl_mem_free_args_t *args = pd_func_args->pd_acl_mem_free;

    pd_acl = (pd_acl_t *)args->acl->pd;
    acl_pd_mem_free(pd_acl);

    return HAL_RET_OK;
}

// ----------------------------------------------------------------------------
// pd acl get
// ----------------------------------------------------------------------------
hal_ret_t
pd_acl_get (pd_func_args_t *pd_func_args)
{
    hal_ret_t         ret = HAL_RET_OK;
    acl_tcam          *acl_tbl = NULL;
    pd_acl_get_args_t *args = pd_func_args->pd_acl_get;
    acl_t             *acl = args->acl;
    pd_acl_t          *pd_acl = (pd_acl_t *)acl->pd;
    AclGetResponse    *rsp = args->rsp;
    uint32_t          hw_tcam_idx;

    auto acl_info = rsp->mutable_status()->mutable_epd_status();

    acl_tbl = g_hal_state_pd->acl_table();
    HAL_ASSERT_RETURN((acl_tbl != NULL), HAL_RET_ERR);

    ret = acl_tbl->get_index(pd_acl->handle, &hw_tcam_idx);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Unable to get tcam index for nacl: {} ret: {}",
                      pd_acl->pi_acl->key, ret);
        return ret;
    }

    acl_info->set_hw_tcam_idx(hw_tcam_idx);
    return ret;
}

// ----------------------------------------------------------------------------
// pd acl restore from response
// ----------------------------------------------------------------------------
static hal_ret_t
acl_pd_restore_data (pd_acl_restore_args_t *args)
{
    hal_ret_t      ret = HAL_RET_OK;
#if 0
    acl_t    *acl = args->acl;
    pd_acl_t *acl_pd = (pd_acl_t *)acl->pd;

    auto acl_info = args->acl_status->epd_status();
#endif

    return ret;
}

//-----------------------------------------------------------------------------
// pd acl restore
//-----------------------------------------------------------------------------
hal_ret_t
pd_acl_restore (pd_func_args_t *pd_func_args)
{
    hal_ret_t      ret;
    pd_acl_restore_args_t *args = pd_func_args->pd_acl_restore;
    pd_acl_t *acl_pd;

    HAL_ASSERT_RETURN((args != NULL), HAL_RET_INVALID_ARG);
    HAL_TRACE_DEBUG("Restoring pd state for acl {}", args->acl->key);

    // allocate PD acl state
    acl_pd = acl_pd_alloc_init();
    if (acl_pd == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }

    // link pi & pd
    acl_pd_link_pi_pd(acl_pd, args->acl);

    ret = acl_pd_restore_data(args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Unable to restore pd data for acl: {}, err: {}",
                      args->acl->key, ret);
        goto end;
    }

    // This call will just populate table libs and calls to HW will be
    // a NOOP in p4pd code
    ret = acl_pd_program_hw(acl_pd, false, 
                            true, args->acl_status->epd_status().hw_tcam_idx());
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:failed to program hw", __FUNCTION__);
        goto end;
    }

end:

    if (ret != HAL_RET_OK) {
        acl_pd_cleanup(acl_pd);
    }

    return ret;
}

}    // namespace pd
}    // namespace hal
