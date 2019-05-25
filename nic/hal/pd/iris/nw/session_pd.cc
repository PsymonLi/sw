// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#include <time.h>
#include "nic/include/base.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/timestamp.hpp"
#include "nic/sdk/include/sdk/lock.hpp"
#include "gen/p4gen/p4/include/p4pd.h"
#include "nic/include/pd_api.hpp"
#include "nic/hal/pd/iris/nw/if_pd_utils.hpp"
#include "nic/hal/pd/iris/nw/session_pd.hpp"
#include "nic/hal/plugins/cfg/nw/interface.hpp"
#include "nic/hal/plugins/cfg/nw/endpoint.hpp"
#include "nic/hal/plugins/cfg/nw/endpoint_api.hpp"
#include "nic/hal/pd/iris/nw/endpoint_pd.hpp"
#include "nic/hal/pd/iris/hal_state_pd.hpp"
#include "nic/hal/plugins/cfg/aclqos/qos_api.hpp"
#include "nic/hal/iris/datapath/p4/include/defines.h"
#include "nic/hal/pd/iris/internal/system_pd.hpp"
#include "nic/hal/pd/capri/capri_hbm.hpp"
#include "platform/capri/capri_tm_rw.hpp"
#include "nic/hal/pd/iris/hal_state_pd.hpp"
#include "nic/hal/pd/iris/internal/p4plus_pd_api.h"
#include <string.h>

namespace hal {
namespace pd {

//------------------------------------------------------------------------------
// program flow stats table entry and return the index at which
// the entry is programmed
//------------------------------------------------------------------------------
hal_ret_t
p4pd_add_flow_stats_table_entry (uint32_t *assoc_hw_idx, uint64_t clock)
{
    hal_ret_t ret;
    sdk_ret_t sdk_ret;
    directmap *stats_table;
    flow_stats_actiondata_t d = { 0 };
    mem_addr_t              stats_mem_addr = 0;
    uint64_t                zero_val = 0;

    SDK_ASSERT(assoc_hw_idx != NULL);
    stats_table = g_hal_state_pd->dm_table(P4TBL_ID_FLOW_STATS);
    SDK_ASSERT(stats_table != NULL);

    d.action_id = FLOW_STATS_FLOW_STATS_ID;
    // P4 has 32 bits so we have to use top 32 bits. We lose the precision by 2^16 ns
    d.action_u.flow_stats_flow_stats.last_seen_timestamp = clock >> 16;
    HAL_TRACE_DEBUG("Setting the last seen timestamp: {} clock {}",
                     d.action_u.flow_stats_flow_stats.last_seen_timestamp,
                     clock);

    // insert the entry
    sdk_ret = stats_table->insert(&d, assoc_hw_idx);
    ret = hal_sdk_ret_to_hal_ret(sdk_ret);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("flow stats table write failure, err : {}", ret);
        return ret;
    }

    ret = hal_pd_stats_addr_get(P4TBL_ID_FLOW_STATS,
                                *assoc_hw_idx, &stats_mem_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Error getting stats address for hw-id {} ret {}",
                      *assoc_hw_idx, ret);
        return ret;
    }

    p4plus_hbm_write(stats_mem_addr, (uint8_t *)&zero_val,
                         sizeof(zero_val), P4PLUS_CACHE_ACTION_NONE);
    p4plus_hbm_write(stats_mem_addr + 8 , (uint8_t *)&zero_val,
                      sizeof(zero_val), P4PLUS_CACHE_ACTION_NONE);
    p4plus_hbm_write(stats_mem_addr + 16, (uint8_t *)&zero_val,
                       sizeof(zero_val), P4PLUS_CACHE_ACTION_NONE);
    p4plus_hbm_write(stats_mem_addr + 24 , (uint8_t *)&zero_val,
                        sizeof(zero_val), P4PLUS_CACHE_ACTION_NONE);

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// delete flow stats table entry at given index
//------------------------------------------------------------------------------
hal_ret_t
p4pd_del_flow_stats_table_entry (uint32_t assoc_hw_idx)
{
    directmap *stats_table;

    // 0th entry is reserved
    if (!assoc_hw_idx) {
        return HAL_RET_INVALID_ARG;
    }

    stats_table = g_hal_state_pd->dm_table(P4TBL_ID_FLOW_STATS);
    SDK_ASSERT(stats_table != NULL);

    return hal_sdk_ret_to_hal_ret(stats_table->remove(assoc_hw_idx));
}

//------------------------------------------------------------------------------
// program flow stats table entries for a given session
//------------------------------------------------------------------------------
hal_ret_t
p4pd_add_flow_stats_table_entries (pd_session_t *session_pd, pd_session_create_args_t *args)
{
    hal_ret_t ret = HAL_RET_OK;

    // program flow_stats table entry for iflow
    if (!session_pd->iflow.assoc_hw_id) {
        ret = p4pd_add_flow_stats_table_entry(&session_pd->iflow.assoc_hw_id, args->clock);
        if (ret != HAL_RET_OK) {
            return ret;
        }
    }

    if (session_pd->iflow_aug.valid && !session_pd->iflow_aug.assoc_hw_id) {
        ret = p4pd_add_flow_stats_table_entry(&session_pd->iflow_aug.assoc_hw_id, args->clock);
        if (ret != HAL_RET_OK) {
            return ret;
        }
    }

    // program flow_stats table entry for rflow
    if (session_pd->rflow.valid && !session_pd->rflow.assoc_hw_id) {
        ret = p4pd_add_flow_stats_table_entry(&session_pd->rflow.assoc_hw_id, args->clock);
        if (ret != HAL_RET_OK) {
            p4pd_del_flow_stats_table_entry(session_pd->iflow.assoc_hw_id);
            return ret;
        }
    }

    if (session_pd->rflow_aug.valid && !session_pd->rflow_aug.assoc_hw_id) {
        ret = p4pd_add_flow_stats_table_entry(&session_pd->rflow_aug.assoc_hw_id, args->clock);
        if (ret != HAL_RET_OK) {
            return ret;
        }
    }
    return ret;
}

//------------------------------------------------------------------------------
// delete flow stats table entries for a given session
//------------------------------------------------------------------------------
hal_ret_t
p4pd_del_flow_stats_table_entries (pd_session_t *session_pd)
{
    hal_ret_t ret = HAL_RET_OK;

    ret = p4pd_del_flow_stats_table_entry(session_pd->iflow.assoc_hw_id);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("iflow flow stats table entry delete failed, err : {}",
                      ret);
    }
    if (session_pd->rflow.valid) {
        SDK_ASSERT(session_pd->rflow.assoc_hw_id);
        ret = p4pd_del_flow_stats_table_entry(session_pd->rflow.assoc_hw_id);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("rflow flow stats table entry delete failed, "
                          "err : {}", ret);
        }
    }

    return ret;
}

//------------------------------------------------------------------------------
// program flow state table entry and return the index at which
// the entry is programmed
//------------------------------------------------------------------------------
hal_ret_t
p4pd_add_session_state_table_entry (pd_session_t *session_pd,
                                    session_state_t *session_state,
                                    nwsec_profile_t *nwsec_profile)
{
    hal_ret_t                ret;
    sdk_ret_t                sdk_ret;
    directmap                *session_state_table;
    flow_t                   *iflow, *rflow;
    session_state_actiondata_t d = { 0 };
    session_t                *session = (session_t *)session_pd->session;

    SDK_ASSERT(session_pd != NULL);
    iflow = session->iflow;
    rflow = session->rflow;
    SDK_ASSERT((iflow != NULL) && (rflow != NULL));

    session_state_table = g_hal_state_pd->dm_table(P4TBL_ID_SESSION_STATE);
    SDK_ASSERT(session_state_table != NULL);

    // populate the action information
    d.action_id = SESSION_STATE_TCP_SESSION_STATE_INFO_ID;

    if (session_state) {
        // session specific information
        d.action_u.session_state_tcp_session_state_info.tcp_ts_option_negotiated =
            session_state->tcp_ts_option;
        d.action_u.session_state_tcp_session_state_info.tcp_sack_perm_option_negotiated =
            session_state->tcp_sack_perm_option;

        // initiator flow specific information
        d.action_u.session_state_tcp_session_state_info.iflow_tcp_state =
            session_state->iflow_state.state;
        d.action_u.session_state_tcp_session_state_info.iflow_tcp_seq_num =
            session_state->iflow_state.tcp_seq_num;
        d.action_u.session_state_tcp_session_state_info.iflow_tcp_ack_num =
            session_state->iflow_state.tcp_ack_num;
        d.action_u.session_state_tcp_session_state_info.iflow_tcp_win_sz =
            session_state->iflow_state.tcp_win_sz;
        d.action_u.session_state_tcp_session_state_info.iflow_tcp_win_scale =
            session_state->iflow_state.tcp_win_scale;
        d.action_u.session_state_tcp_session_state_info.iflow_tcp_mss =
            session_state->iflow_state.tcp_mss;
        d.action_u.session_state_tcp_session_state_info.iflow_tcp_ws_option_sent =
            session_state->iflow_state.tcp_ws_option_sent;
        d.action_u.session_state_tcp_session_state_info.iflow_tcp_ts_option_sent =
            session_state->iflow_state.tcp_ts_option_sent;
        d.action_u.session_state_tcp_session_state_info.iflow_tcp_sack_perm_option_sent =
            session_state->iflow_state.tcp_sack_perm_option_sent;
        d.action_u.session_state_tcp_session_state_info.iflow_exceptions_seen =
            session_state->iflow_state.exception_bmap;

        // responder flow specific information
        d.action_u.session_state_tcp_session_state_info.rflow_tcp_state =
            session_state->rflow_state.state;
        d.action_u.session_state_tcp_session_state_info.rflow_tcp_seq_num =
            session_state->rflow_state.tcp_seq_num;
        d.action_u.session_state_tcp_session_state_info.rflow_tcp_ack_num =
            session_state->rflow_state.tcp_ack_num;
        d.action_u.session_state_tcp_session_state_info.rflow_tcp_win_sz =
            session_state->rflow_state.tcp_win_sz;
        d.action_u.session_state_tcp_session_state_info.rflow_tcp_win_scale =
            session_state->rflow_state.tcp_win_scale;
        d.action_u.session_state_tcp_session_state_info.rflow_tcp_mss =
            session_state->rflow_state.tcp_mss;
        d.action_u.session_state_tcp_session_state_info.rflow_exceptions_seen =
            session_state->rflow_state.exception_bmap;


        d.action_u.session_state_tcp_session_state_info.syn_cookie_delta =
            session_state->iflow_state.syn_ack_delta;
    }

    d.action_u.session_state_tcp_session_state_info.flow_rtt_seq_check_enabled =
        nwsec_profile ?  nwsec_profile->tcp_rtt_estimate_en : FALSE;

    // insert the entry
    sdk_ret = session_state_table->insert(&d, &session_pd->session_state_idx);
    ret = hal_sdk_ret_to_hal_ret(sdk_ret);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("session state table write failure, err : {}", ret);
        return ret;
    }

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// delete flow state table entry at given index
//------------------------------------------------------------------------------
hal_ret_t
p4pd_del_session_state_table_entry (uint32_t session_state_idx)
{
    directmap *session_state_table;

    // 0th entry is reserved
    if (!session_state_idx) {
        return HAL_RET_INVALID_ARG;
    }

    session_state_table = g_hal_state_pd->dm_table(P4TBL_ID_SESSION_STATE);
    SDK_ASSERT(session_state_table != NULL);

    return hal_sdk_ret_to_hal_ret(session_state_table->remove(session_state_idx));
}
//------------------------------------------------------------------------------
// program flow info table entry at either given index or if given index is 0
// allocate an index and return that
// TODO:
// 1. flow_ttl is set to DEF_TTL (64) currently ... if we have packet in hand we
//    should take it from there or else default is fine
// 2. twice nat not supported
//-----------------------------------------------------------------------------
hal_ret_t
p4pd_add_upd_flow_info_table_entry (session_t *session, pd_flow_t *flow_pd,
                                    flow_role_t role, bool aug, uint64_t clock)
{
    hal_ret_t ret;
    sdk_ret_t sdk_ret;
    directmap *info_table;
    flow_info_actiondata_t d = { 0};
    flow_pgm_attrs_t *flow_attrs = NULL;
    flow_cfg_t *flow_cfg = NULL;
    pd_session_t *sess_pd = NULL;
    bool entry_exists = false;

    sess_pd = session->pd;

    info_table = g_hal_state_pd->dm_table(P4TBL_ID_FLOW_INFO);
    SDK_ASSERT(info_table != NULL);

    sdk_ret = info_table->retrieve(flow_pd->assoc_hw_id, &d);
    ret = hal_sdk_ret_to_hal_ret(sdk_ret);
    if (ret == HAL_RET_OK) {
        entry_exists = true;
    }

    if (role == FLOW_ROLE_INITIATOR) {
        flow_attrs = aug ? &session->iflow->assoc_flow->pgm_attrs :
                &session->iflow->pgm_attrs;
    } else {
        flow_attrs = aug ? &session->rflow->assoc_flow->pgm_attrs :
                &session->rflow->pgm_attrs;
    }
    if (flow_attrs->drop) {
        d.action_id = FLOW_INFO_FLOW_HIT_DROP_ID;
        d.action_u.flow_info_flow_hit_drop.start_timestamp = (clock>>16);
        HAL_TRACE_DEBUG("Action being set to drop");

    } else {
        d.action_id = FLOW_INFO_FLOW_INFO_ID;
        // get the flow attributes
        if (role == FLOW_ROLE_INITIATOR) {
            flow_cfg = &session->iflow->config;
            if (aug) {
                d.action_u.flow_info_flow_info.ingress_mirror_session_id =
                                     session->iflow->assoc_flow->config.ing_mirror_session;
                d.action_u.flow_info_flow_info.egress_mirror_session_id =
                                     session->iflow->assoc_flow->config.eg_mirror_session;
            } else {
                d.action_u.flow_info_flow_info.ingress_mirror_session_id =
                                     flow_cfg->ing_mirror_session;
                d.action_u.flow_info_flow_info.egress_mirror_session_id =
                                     flow_cfg->eg_mirror_session;
            }
        } else {
            flow_cfg = &session->rflow->config;
            if (aug) {
                d.action_u.flow_info_flow_info.ingress_mirror_session_id =
                                     session->rflow->assoc_flow->config.ing_mirror_session;
                d.action_u.flow_info_flow_info.egress_mirror_session_id =
                                     session->rflow->assoc_flow->config.eg_mirror_session;
            } else {
                d.action_u.flow_info_flow_info.ingress_mirror_session_id =
                                     flow_cfg->ing_mirror_session;
                d.action_u.flow_info_flow_info.egress_mirror_session_id =
                                     flow_cfg->eg_mirror_session;
            }
        }

        if (flow_attrs->export_en) {
            d.action_u.flow_info_flow_info.export_id1 =
                                                 flow_attrs->export_id1;
            d.action_u.flow_info_flow_info.export_id2 =
                                                 flow_attrs->export_id2;
            d.action_u.flow_info_flow_info.export_id3 =
                                                 flow_attrs->export_id3;
            d.action_u.flow_info_flow_info.export_id4 =
                                                 flow_attrs->export_id4;
        }

        d.action_u.flow_info_flow_info.expected_src_lif_check_en =
            flow_attrs->expected_src_lif_en;
        d.action_u.flow_info_flow_info.expected_src_lif =
            flow_attrs->expected_src_lif;
        d.action_u.flow_info_flow_info.dst_lport = flow_attrs->lport;
        d.action_u.flow_info_flow_info.multicast_en = flow_attrs->mcast_en;
        d.action_u.flow_info_flow_info.multicast_ptr = flow_attrs->mcast_ptr;

        // Set the tunnel originate flag
        d.action_u.flow_info_flow_info.tunnel_originate =
                                                        flow_attrs->tunnel_orig;
        // L4 LB (NAT) Info
        d.action_u.flow_info_flow_info.nat_l4_port = flow_attrs->nat_l4_port;
        memcpy(d.action_u.flow_info_flow_info.nat_ip, &flow_attrs->nat_ip.addr,
               sizeof(ipvx_addr_t));
        if (flow_cfg->key.flow_type == FLOW_TYPE_V6) {
            memrev(d.action_u.flow_info_flow_info.nat_ip,
                   sizeof(d.action_u.flow_info_flow_info.nat_ip));
        }
        d.action_u.flow_info_flow_info.twice_nat_idx = flow_attrs->twice_nat_idx;

        // QOS Info
        d.action_u.flow_info_flow_info.qos_class_en = flow_attrs->qos_class_en;
        d.action_u.flow_info_flow_info.qos_class_id = flow_attrs->qos_class_id;

        // TBD: check class NIC mode and set this
        d.action_u.flow_info_flow_info.qid_en = flow_attrs->qid_en;
        if (flow_attrs->qid_en) {
            d.action_u.flow_info_flow_info.qtype = flow_attrs->qtype;
            d.action_u.flow_info_flow_info.tunnel_vnid = flow_attrs->qid;
        } else {
            d.action_u.flow_info_flow_info.tunnel_vnid = flow_attrs->tnnl_vnid;
        }

        /*
         * For packets to host, the vlan id will be derived from output_mapping, but
         * the decision to do vlan encap or not is coming from here.
         */
        d.action_u.flow_info_flow_info.tunnel_rewrite_index = flow_attrs->tnnl_rw_idx;

        // TBD: check analytics policy and set this
        d.action_u.flow_info_flow_info.log_en = FALSE;

        d.action_u.flow_info_flow_info.rewrite_flags =
            (flow_attrs->mac_sa_rewrite ? REWRITE_FLAGS_MAC_SA : 0) |
            (flow_attrs->mac_da_rewrite ? REWRITE_FLAGS_MAC_DA : 0) |
            (flow_attrs->ttl_dec ? REWRITE_FLAGS_TTL_DEC : 0);

        if (flow_attrs->rw_act != REWRITE_NOP_ID) {
            d.action_u.flow_info_flow_info.rewrite_index = flow_attrs->rw_idx;
        }
    #if 0
        // TODO: if we are doing routing, then set ttl_dec to TRUE
        if ((role == FLOW_ROLE_INITIATOR && !aug) ||
            (role == FLOW_ROLE_RESPONDER && aug)) {
            // Assuming aug flows are either send to uplink or receive from uplink
            d.action_u.flow_info_flow_info.flow_conn_track = true;
        } else {
            d.action_u.flow_info_flow_info.flow_conn_track = session->config.conn_track_en;
        }
    #endif
        d.action_u.flow_info_flow_info.flow_conn_track = session->conn_track_en;
        d.action_u.flow_info_flow_info.flow_ttl = 64;
        d.action_u.flow_info_flow_info.flow_role = flow_attrs->role;
        d.action_u.flow_info_flow_info.session_state_index =
        d.action_u.flow_info_flow_info.flow_conn_track ?
                 sess_pd->session_state_idx : 0;
        d.action_u.flow_info_flow_info.start_timestamp = (clock>>16);
    } // End updateion flow allow action

    if (entry_exists) {
       // Update the entry
       sdk_ret = info_table->update(flow_pd->assoc_hw_id, &d);
       ret = hal_sdk_ret_to_hal_ret(sdk_ret);
       if (ret != HAL_RET_OK) {
           HAL_TRACE_ERR("flow info table update failure, idx : {}, err : {}",
                         flow_pd->assoc_hw_id, ret);
           return ret;
       }
    } else {
       // insert the entry
       sdk_ret = info_table->insert_withid(&d, flow_pd->assoc_hw_id);
       ret = hal_sdk_ret_to_hal_ret(sdk_ret);
       if (ret != HAL_RET_OK) {
           HAL_TRACE_ERR("flow info table write failure, idx : {}, err : {}",
                             flow_pd->assoc_hw_id, ret);
           return ret;
       }
    }

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// delete flow info table entry at given index
//------------------------------------------------------------------------------
hal_ret_t
p4pd_del_flow_info_table_entry (uint32_t index)
{
    directmap *info_table;

    // 0th entry is reserved
    if (!index) {
        return HAL_RET_INVALID_ARG;
    }

    info_table = g_hal_state_pd->dm_table(P4TBL_ID_FLOW_INFO);
    SDK_ASSERT(info_table != NULL);

    return hal_sdk_ret_to_hal_ret(info_table->remove(index));
}

//------------------------------------------------------------------------------
// delete flow info table entries for a given session
//------------------------------------------------------------------------------
hal_ret_t
p4pd_del_flow_info_table_entries (pd_session_t *session_pd)
{
    hal_ret_t    ret;

    ret = p4pd_del_flow_info_table_entry(session_pd->iflow.assoc_hw_id);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("iflow flow info table entry delete failed, "
                      "idx : {}, err : {}",
                      session_pd->iflow.assoc_hw_id, ret);
    }
    if (session_pd->rflow.valid) {
        SDK_ASSERT(session_pd->rflow.assoc_hw_id);
        ret = p4pd_del_flow_info_table_entry(session_pd->rflow.assoc_hw_id);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("rflow flow info table entry delete failed, "
                          "idx : {}, err : {}",
                          session_pd->rflow.assoc_hw_id, ret);
         }
    }

    return ret;
}

//------------------------------------------------------------------------------
// program flow info table entries for a given session
//------------------------------------------------------------------------------
hal_ret_t
p4pd_add_flow_info_table_entries (pd_session_create_args_t *args)
{
    hal_ret_t       ret;
    pd_session_t    *session_pd = (pd_session_t *)args->session->pd;

    HAL_TRACE_DEBUG("Adding flow info table entries");
    // program flow_info table entry for rflow
    if (session_pd->rflow.valid) {
        ret = p4pd_add_upd_flow_info_table_entry(args->session, &session_pd->rflow,
                                                 FLOW_ROLE_RESPONDER, false, args->clock);
        if (ret != HAL_RET_OK) {
            p4pd_del_flow_info_table_entry(session_pd->iflow.assoc_hw_id);
            return ret;
        }
        if (session_pd->rflow_aug.valid) {
            ret = p4pd_add_upd_flow_info_table_entry(args->session, &session_pd->rflow_aug,
                                                     FLOW_ROLE_RESPONDER, true, args->clock);
            if (ret != HAL_RET_OK) {
                return ret;
            }
        }
    }

    ret = p4pd_add_upd_flow_info_table_entry(args->session, &session_pd->iflow,
                                             FLOW_ROLE_INITIATOR, false, args->clock);
    if (ret != HAL_RET_OK) {
        return ret;
    }

    if (session_pd->iflow_aug.valid) {
        ret = p4pd_add_upd_flow_info_table_entry(args->session, &session_pd->iflow_aug,
                                                 FLOW_ROLE_INITIATOR, true, args->clock);
        if (ret != HAL_RET_OK) {
            return ret;
        }
    }

    return ret;
}

hal_ret_t
p4pd_fill_flow_hash_key (flow_key_t *flow_key, uint32_t lkp_vrf,
                         uint8_t lkp_inst, flow_hash_swkey_t &key)
{
    // initialize all the key fields of flow
    if (flow_key->flow_type == FLOW_TYPE_V4 || flow_key->flow_type == FLOW_TYPE_V6) {
        memcpy(key.flow_lkp_metadata_lkp_src, flow_key->sip.v6_addr.addr8,
                IP6_ADDR8_LEN);
        memcpy(key.flow_lkp_metadata_lkp_dst, flow_key->dip.v6_addr.addr8,
                IP6_ADDR8_LEN);
    } else {
        memcpy(key.flow_lkp_metadata_lkp_src, flow_key->smac, sizeof(flow_key->smac));
        memcpy(key.flow_lkp_metadata_lkp_dst, flow_key->dmac, sizeof(flow_key->dmac));
    }

    if (flow_key->flow_type == FLOW_TYPE_V6) {
        memrev(key.flow_lkp_metadata_lkp_src, sizeof(key.flow_lkp_metadata_lkp_src));
        memrev(key.flow_lkp_metadata_lkp_dst, sizeof(key.flow_lkp_metadata_lkp_dst));
    } else if (flow_key->flow_type == FLOW_TYPE_L2) {
        memrev(key.flow_lkp_metadata_lkp_src, 6);
        memrev(key.flow_lkp_metadata_lkp_dst, 6);
    }

    if (flow_key->flow_type == FLOW_TYPE_V4 || flow_key->flow_type == FLOW_TYPE_V6) {
        if ((flow_key->proto == IP_PROTO_TCP) || (flow_key->proto == IP_PROTO_UDP)) {
            key.flow_lkp_metadata_lkp_sport = flow_key->sport;
            key.flow_lkp_metadata_lkp_dport = flow_key->dport;
        } else if ((flow_key->proto == IP_PROTO_ICMP) ||
                (flow_key->proto == IP_PROTO_ICMPV6)) {
            // Revisit: Swapped sport and dport. This is what matches what
            //          is coded in P4.
            key.flow_lkp_metadata_lkp_sport = flow_key->icmp_id;
            key.flow_lkp_metadata_lkp_dport =
                ((flow_key->icmp_type << 8) | flow_key->icmp_code);
        } else if (flow_key->proto == IPPROTO_ESP) {
            key.flow_lkp_metadata_lkp_sport = flow_key->spi >> 16 & 0xFFFF;
            key.flow_lkp_metadata_lkp_dport = flow_key->spi & 0xFFFF;
        }
    } else {
        // For FLOW_TYPE_L2
        key.flow_lkp_metadata_lkp_dport = flow_key->ether_type;
    }
    if (flow_key->flow_type == FLOW_TYPE_L2) {
        key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_MAC;
    } else if (flow_key->flow_type == FLOW_TYPE_V4) {
        key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_IPV4;
    } else if (flow_key->flow_type == FLOW_TYPE_V6) {
        key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_IPV6;
    } else {
        // TODO: for now !!
        key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_NONE;
    }
    key.flow_lkp_metadata_lkp_vrf = lkp_vrf;
    key.flow_lkp_metadata_lkp_proto = flow_key->proto;
    key.flow_lkp_metadata_lkp_inst = lkp_inst;
    key.flow_lkp_metadata_lkp_dir = flow_key->dir;

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// program flow hash table entry for a given flow
//------------------------------------------------------------------------------
hal_ret_t
p4pd_add_flow_hash_table_entry (flow_key_t *flow_key, uint32_t lkp_vrf,
                                uint8_t lkp_inst, pd_flow_t *flow_pd,
                                uint32_t hash_val, uint8_t export_en,
                                uint32_t *flow_hash_p)
{
    hal_ret_t ret = HAL_RET_OK;
    flow_hash_swkey_t key = { 0 };
    flow_hash_appdata_t appdata = { 0 };

    if (flow_pd->installed == true) {
        return HAL_RET_OK;
    }

    // initialize all the key fields of flow
    if (flow_key->flow_type == FLOW_TYPE_V4 || flow_key->flow_type == FLOW_TYPE_V6) {
        memcpy(key.flow_lkp_metadata_lkp_src, flow_key->sip.v6_addr.addr8,
                IP6_ADDR8_LEN);
        memcpy(key.flow_lkp_metadata_lkp_dst, flow_key->dip.v6_addr.addr8,
                IP6_ADDR8_LEN);
    } else {
        memcpy(key.flow_lkp_metadata_lkp_src, flow_key->smac, sizeof(flow_key->smac));
        memcpy(key.flow_lkp_metadata_lkp_dst, flow_key->dmac, sizeof(flow_key->dmac));
    }

    if (flow_key->flow_type == FLOW_TYPE_V6) {
        memrev(key.flow_lkp_metadata_lkp_src, sizeof(key.flow_lkp_metadata_lkp_src));
        memrev(key.flow_lkp_metadata_lkp_dst, sizeof(key.flow_lkp_metadata_lkp_dst));
    } else if (flow_key->flow_type == FLOW_TYPE_L2) {
        memrev(key.flow_lkp_metadata_lkp_src, 6);
        memrev(key.flow_lkp_metadata_lkp_dst, 6);
    }

    if (flow_key->flow_type == FLOW_TYPE_V4 || flow_key->flow_type == FLOW_TYPE_V6) {
        if ((flow_key->proto == IP_PROTO_TCP) || (flow_key->proto == IP_PROTO_UDP)) {
            key.flow_lkp_metadata_lkp_sport = flow_key->sport;
            key.flow_lkp_metadata_lkp_dport = flow_key->dport;
        } else if ((flow_key->proto == IP_PROTO_ICMP) ||
                (flow_key->proto == IP_PROTO_ICMPV6)) {
            // Revisit: Swapped sport and dport. This is what matches what
            //          is coded in P4.
            key.flow_lkp_metadata_lkp_sport = flow_key->icmp_id;
            key.flow_lkp_metadata_lkp_dport =
                ((flow_key->icmp_type << 8) | flow_key->icmp_code);
        } else if (flow_key->proto == IPPROTO_ESP) {
            key.flow_lkp_metadata_lkp_sport = flow_key->spi >> 16 & 0xFFFF;
            key.flow_lkp_metadata_lkp_dport = flow_key->spi & 0xFFFF;
        }
    } else {
        // For FLOW_TYPE_L2
        key.flow_lkp_metadata_lkp_dport = flow_key->ether_type;
    }
    if (flow_key->flow_type == FLOW_TYPE_L2) {
        key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_MAC;
    } else if (flow_key->flow_type == FLOW_TYPE_V4) {
        key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_IPV4;
    } else if (flow_key->flow_type == FLOW_TYPE_V6) {
        key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_IPV6;
    } else {
        // TODO: for now !!
        key.flow_lkp_metadata_lkp_type = FLOW_KEY_LOOKUP_TYPE_NONE;
    }
    key.flow_lkp_metadata_lkp_vrf = lkp_vrf;
    key.flow_lkp_metadata_lkp_proto = flow_key->proto;
    key.flow_lkp_metadata_lkp_inst = lkp_inst;
    key.flow_lkp_metadata_lkp_dir = flow_key->dir;

    appdata.flow_index = flow_pd->assoc_hw_id;
    appdata.export_en = export_en;
    if (hal::utils::hal_trace_level() >= ::utils::trace_verbose) {
        fmt::MemoryWriter src_buf, dst_buf;
        for (uint32_t i = 0; i < 16; i++) {
            src_buf.write("{:#x} ", key.flow_lkp_metadata_lkp_src[i]);
        }
        HAL_TRACE_DEBUG("Src:");
        HAL_TRACE_DEBUG("{}", src_buf.c_str());
        for (uint32_t i = 0; i < 16; i++) {
            dst_buf.write("{:#x} ", key.flow_lkp_metadata_lkp_dst[i]);
        }
        HAL_TRACE_DEBUG("Dst:");
        HAL_TRACE_DEBUG("{}", dst_buf.c_str());
    }

    if (hash_val) {
        ret = g_hal_state_pd->flow_table_pd_get()->insert(&key, &appdata,
                                                          &hash_val, true);
    } else {
        ret = g_hal_state_pd->flow_table_pd_get()->insert(&key, &appdata,
                                                          &hash_val, false);
    }

    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("flow table insert failed, err : {}", ret);
        return ret;
    }

    *flow_hash_p = hash_val;
    flow_pd->installed = true;

    return ret;
}

//------------------------------------------------------------------------------
// delete flow hash table entry at given index
//------------------------------------------------------------------------------
hal_ret_t
p4pd_del_flow_hash_table_entry (flow_key_t *flow_key, uint32_t lkp_vrf,
                                uint8_t lkp_inst, pd_flow_t *flow_pd)
{
    hal_ret_t ret = HAL_RET_OK;
    flow_hash_swkey_t key = { 0 };

    if (flow_pd->installed == false) {
        return HAL_RET_OK;
    }

    p4pd_fill_flow_hash_key(flow_key, lkp_vrf, lkp_inst, key);

    ret = g_hal_state_pd->flow_table_pd_get()->remove(&key);
    if (ret == HAL_RET_OK) {
        flow_pd->installed = false;
    }
    return ret;
}

//------------------------------------------------------------------------------
// Remove all valid flow hash table entries with given data (flow index)
//------------------------------------------------------------------------------
hal_ret_t
p4pd_del_flow_hash_table_entries (pd_session_t *session_pd)
{
    hal_ret_t ret = HAL_RET_OK;
    hal_ret_t final_ret = HAL_RET_OK;
    session_t *session = (session_t *)session_pd->session;

    if (session_pd->iflow.valid) {
        ret = p4pd_del_flow_hash_table_entry(&session->iflow->config.key,
                                             session->iflow->pgm_attrs.vrf_hwid,
                                             session->iflow->pgm_attrs.lkp_inst,
                                             &session_pd->iflow);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("iflow flow info table entry delete failed, "
                          "session idx : {}, err : {}",
                          session_pd->session_state_idx, ret);
            final_ret = ret;
        }
    }

    if (session_pd->iflow_aug.valid) {
        ret = p4pd_del_flow_hash_table_entry(&session->iflow->assoc_flow->config.key,
                                             session->iflow->assoc_flow->pgm_attrs.vrf_hwid,
                                             session->iflow->assoc_flow->pgm_attrs.lkp_inst,
                                             &session_pd->iflow_aug);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("iflow aug flow info table entry delete failed, "
                          "session idx : {}, err : {}",
                          session_pd->session_state_idx, ret);
            final_ret = ret;
        }
    }

    if (session_pd->rflow.valid) {
        ret = p4pd_del_flow_hash_table_entry(&session->rflow->config.key,
                                             session->rflow->pgm_attrs.vrf_hwid,
                                             session->rflow->pgm_attrs.lkp_inst,
                                             &session_pd->rflow);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("rflow flow info table entry delete failed, "
                          "session idx : {}, err : {}",
                          session_pd->session_state_idx, ret);
            final_ret = ret;
        }
    }

    if (session_pd->rflow_aug.valid) {
        ret = p4pd_del_flow_hash_table_entry(&session->rflow->assoc_flow->config.key,
                                             session->rflow->assoc_flow->pgm_attrs.vrf_hwid,
                                             session->rflow->assoc_flow->pgm_attrs.lkp_inst,
                                             &session_pd->rflow_aug);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("rflow aug flow info table entry delete failed, "
                          "session idx : {}, err : {}",
                          session_pd->session_state_idx, ret);
            final_ret = ret;
        }

    }

    return final_ret;
}

//------------------------------------------------------------------------------
// program flow hash table entry with given data (flow index)
//------------------------------------------------------------------------------
hal_ret_t
p4pd_add_flow_hash_table_entries (pd_session_t *session_pd,
                                  pd_session_create_args_t *args)
{
    hal_ret_t               ret = HAL_RET_OK;
    session_t               *session = (session_t *)session_pd->session;
    uint32_t                flow_hash = 0;

    if (session_pd->rflow.valid && !session_pd->rflow.installed) {
        ret = p4pd_add_flow_hash_table_entry(&session->rflow->config.key,
                                             session->rflow->pgm_attrs.vrf_hwid,
                                             session->rflow->pgm_attrs.lkp_inst,
                                             &session_pd->rflow,
                                             0,
                                             session->rflow->pgm_attrs.export_en,
                                             &flow_hash);

        if (args->rsp) {
            args->rsp->mutable_status()->mutable_rflow_status()->set_flow_hash(flow_hash);
        }
        if (ret != HAL_RET_OK) {
            goto p4pd_add_flow_hash_table_entries_return;
        }

        if (session_pd->rflow_aug.valid && !session_pd->rflow_aug.installed) {
            // TODO: key has to involve service done? populate in flow_attrs
            ret = p4pd_add_flow_hash_table_entry(&session->rflow->assoc_flow->config.key,
                                                 session->rflow->assoc_flow->pgm_attrs.vrf_hwid,
                                                 session->rflow->assoc_flow->pgm_attrs.lkp_inst,
                                                 &session_pd->rflow_aug,
                                                 0,
                                                 session->rflow->assoc_flow->pgm_attrs.export_en,
                                                 &flow_hash);
            if (args->rsp) {
                args->rsp->mutable_status()->mutable_rflow_status()->set_flow_hash(flow_hash);
            }
            if (ret != HAL_RET_OK) {
                goto p4pd_add_flow_hash_table_entries_return;
            }
        }
    }

    if (!session_pd->iflow.installed && args->update_iflow) {
        ret = p4pd_add_flow_hash_table_entry(&session->iflow->config.key,
                                             session->iflow->pgm_attrs.vrf_hwid,
                                             session->iflow->pgm_attrs.lkp_inst,
                                             &session_pd->iflow,
                                             args->iflow_hash,
                                             session->iflow->pgm_attrs.export_en,
                                             &flow_hash);
        if (args->rsp) {
            args->rsp->mutable_status()->mutable_iflow_status()->set_flow_hash(flow_hash);
        }

        if (ret != HAL_RET_OK) {
            goto p4pd_add_flow_hash_table_entries_return;
        }
    }

    if (args->update_iflow && session_pd->iflow_aug.valid &&\
        !session_pd->iflow_aug.installed) {
        ret = p4pd_add_flow_hash_table_entry(&session->iflow->assoc_flow->config.key,
                                             session->iflow->assoc_flow->pgm_attrs.vrf_hwid,
                                             session->iflow->assoc_flow->pgm_attrs.lkp_inst,
                                             &session_pd->iflow_aug,
                                             0,
                                             session->iflow->assoc_flow->pgm_attrs.export_en,
                                             &flow_hash);
        if (args->rsp) {
            args->rsp->mutable_status()->mutable_iflow_status()->set_flow_hash(flow_hash);
        }

        if (ret != HAL_RET_OK) {
            goto p4pd_add_flow_hash_table_entries_return;
        }
    }

p4pd_add_flow_hash_table_entries_return:
    if (ret != HAL_RET_OK) {
        SDK_ASSERT(p4pd_del_flow_hash_table_entries(session_pd) == HAL_RET_OK);
    }

    return ret;
}

//------------------------------------------------------------------------------
// program all PD tables for given session and maintain meta data in PD state
//------------------------------------------------------------------------------
hal_ret_t
pd_session_create (pd_func_args_t *pd_func_args)
{
    pd_conv_sw_clock_to_hw_clock_args_t     clock_args;
    hal_ret_t                               ret;
    pd_session_create_args_t               *args = pd_func_args->pd_session_create;
    pd_session_t                           *session_pd;
    session_t                              *session = args->session;
    uint64_t                                sw_ns = 0, clock=0;
    timespec_t                              ts;
    pd_func_args_t                          pd_clock_fn_args = {0};

    HAL_TRACE_DEBUG("Creating pd state for session");

    session_pd = session_pd_alloc_init();
    if (session_pd == NULL) {
        return HAL_RET_OOM;
    }
    session_pd->session = args->session;
    args->session->pd = session_pd;

    if (session->rflow) {
        session_pd->rflow.valid = TRUE;
        if (session->rflow->assoc_flow) {
            session_pd->rflow_aug.valid = TRUE;
        }
    } else {
        session_pd->rflow.valid = FALSE;
    }

    session_pd->iflow.valid = true;
    if (session->iflow->assoc_flow) {
        session_pd->iflow_aug.valid = TRUE;
    }

    // Get the hw clock
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sdk::timestamp_to_nsecs(&ts, &sw_ns);
    clock_args.hw_tick = &clock;
    clock_args.sw_ns = sw_ns;
    pd_clock_fn_args.pd_conv_sw_clock_to_hw_clock = &clock_args;
    pd_conv_sw_clock_to_hw_clock(&pd_clock_fn_args);

    args->clock = clock;

    // add flow stats entries first
    ret = p4pd_add_flow_stats_table_entries(session_pd, args);
    if (ret != HAL_RET_OK) {
        return ret;
    }

    // HACK: Only for TCP proxy we are disabling connection tracking.
    //       Have to disable once we make TCP proxy work with connection tracking
    if (session_pd->iflow_aug.valid || session_pd->rflow_aug.valid) {
        args->session->conn_track_en = 0;
    }

    // if connection tracking is on, add flow state entry for this session
    if (args->session->conn_track_en) {
        session_pd->conn_track_en = 1;
        ret = p4pd_add_session_state_table_entry(session_pd, args->session_state, args->nwsec_prof);
        if (ret != HAL_RET_OK) {
            goto cleanup;
        }
    } else {
        session_pd->conn_track_en = 0;
    }

    // add flow info table entries
    ret = p4pd_add_flow_info_table_entries(args);
    if (ret != HAL_RET_OK) {
        goto cleanup;
    }

    ret = p4pd_add_flow_hash_table_entries(session_pd, args);
    if (ret != HAL_RET_OK) {
        goto cleanup;
    }

    return HAL_RET_OK;

cleanup:

    if (session_pd) {
        p4pd_del_flow_stats_table_entries(session_pd);
        p4pd_del_session_state_table_entry(session_pd->session_state_idx);
        p4pd_del_flow_info_table_entries(session_pd);
        session_pd_free(session_pd);
        args->session->pd = NULL;
    }
    return ret;
}

//-----------------------------------------
// update PD tables for given session
//------------------------------------------
hal_ret_t
pd_session_update (pd_func_args_t *pd_func_args)
{
    timespec_t                           ts;
    pd_conv_sw_clock_to_hw_clock_args_t  clock_args;
    pd_func_args_t                       pd_clock_fn_args = {0};
    hal_ret_t                            ret;
    pd_session_update_args_t            *args = pd_func_args->pd_session_update;
    pd_session_t                        *session_pd = NULL;
    session_t                           *session = args->session;
    uint64_t                             sw_ns = 0, clock=0;

    //HAL_TRACE_DEBUG("Updating pd state for session");

    session_pd = session->pd;

    SDK_ASSERT(session_pd != NULL);

    if (session->rflow) {
        //HAL_TRACE_DEBUG("Programming Rflow");
        session_pd->rflow.valid = TRUE;
        if (session->rflow->assoc_flow) {
            session_pd->rflow_aug.valid = TRUE;
        }
    } else {
        session_pd->rflow.valid = FALSE;
    }

    if (session->iflow->assoc_flow) {
        session_pd->iflow_aug.valid = TRUE;
    }

    // Get the hw clock
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sdk::timestamp_to_nsecs(&ts, &sw_ns);
    clock_args.hw_tick = &clock;
    clock_args.sw_ns = sw_ns;
    pd_clock_fn_args.pd_conv_sw_clock_to_hw_clock = &clock_args;
    pd_conv_sw_clock_to_hw_clock(&pd_clock_fn_args);

    args->clock = clock;

    // update/add flow stats entries first
    ret = p4pd_add_flow_stats_table_entries(session_pd, (pd_session_create_args_t *)args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Flow stats table entry upd failure");
        return ret;
    }

    // update/add flow info table entries
    ret = p4pd_add_flow_info_table_entries((pd_session_create_args_t *)args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Flow info table entry upd failure");
        goto cleanup;
    }

    ret = p4pd_add_flow_hash_table_entries(session_pd,
                                           (pd_session_create_args_t *)args);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Flow hash table entry upd failure");
        goto cleanup;
    }

    return HAL_RET_OK;

cleanup:

    if (session_pd) {
        p4pd_del_flow_stats_table_entries(session_pd);
        p4pd_del_session_state_table_entry(session_pd->session_state_idx);
        p4pd_del_flow_info_table_entries(session_pd);
        session_pd_free(session_pd);
        args->session->pd = NULL;
    }

    return ret;
}

//-----------------------------------------
// Delete PD tables for given session
//------------------------------------------
hal_ret_t
pd_session_delete (pd_func_args_t *pd_func_args)
{
    hal_ret_t          ret = HAL_RET_OK;
    pd_session_delete_args_t *args = pd_func_args->pd_session_delete;
    pd_session_t       *session_pd = NULL;

    HAL_TRACE_DEBUG("Deleting pd state for session");

    session_pd = args->session->pd;

    SDK_ASSERT(session_pd != NULL);

    // del flow stats entries first
    p4pd_del_flow_stats_table_entries(session_pd);

    // Del session state
    p4pd_del_session_state_table_entry(session_pd->session_state_idx);

    // del flow info table entries
    p4pd_del_flow_info_table_entries(session_pd);

    // del flow hash table entries
    p4pd_del_flow_hash_table_entries(session_pd);

    session_pd_free(session_pd);
    args->session->pd = NULL;

    return ret;
}

//-----------------------------------------
// get all session related information
//------------------------------------------
hal_ret_t
pd_session_get (pd_func_args_t *pd_func_args)
{
    hal_ret_t ret = HAL_RET_OK;
    pd_session_get_args_t *args = pd_func_args->pd_session_get;
    pd_func_args_t pd_func_args1 = {0};
    sdk_ret_t sdk_ret;
    session_state_actiondata_t d = {0};
    directmap *session_state_table = NULL;
    session_state_t *ss = args->session_state;
    pd_flow_get_args_t flow_get_args;
    pd_session_t *pd_session;
    session_state_tcp_session_state_info_t info;

    if (args->session == NULL || args->session->pd == NULL) {
        return HAL_RET_INVALID_ARG;
    }
    pd_session = args->session->pd;

    if (args->session->conn_track_en) {
        session_state_table = g_hal_state_pd->dm_table(P4TBL_ID_SESSION_STATE);
        SDK_ASSERT(session_state_table != NULL);

        sdk_ret = session_state_table->retrieve(pd_session->session_state_idx, &d);
        ret = hal_sdk_ret_to_hal_ret(sdk_ret);
        if (ret == HAL_RET_OK) {
            info = d.action_u.session_state_tcp_session_state_info;
            ss->tcp_ts_option = info.tcp_ts_option_negotiated;
            ss->tcp_sack_perm_option = info.tcp_sack_perm_option_negotiated;

            // Initiator flow specific information
            ss->iflow_state.state = (session::FlowTCPState)info.iflow_tcp_state;
            ss->iflow_state.tcp_seq_num = info.iflow_tcp_seq_num;
            ss->iflow_state.tcp_ack_num = info.iflow_tcp_ack_num;
            ss->iflow_state.tcp_win_sz = info.iflow_tcp_win_sz;
            ss->iflow_state.syn_ack_delta = info.syn_cookie_delta;
            ss->iflow_state.tcp_mss = info.iflow_tcp_mss;
            ss->iflow_state.tcp_win_scale = info.iflow_tcp_win_scale;
            ss->iflow_state.tcp_ws_option_sent = info.iflow_tcp_ws_option_sent;
            ss->iflow_state.tcp_ts_option_sent = info.iflow_tcp_ts_option_sent;
            ss->iflow_state.tcp_sack_perm_option_sent =
                                          info.iflow_tcp_sack_perm_option_sent;
            ss->iflow_state.exception_bmap = info.iflow_exceptions_seen;

            // Responder flow specific information
            ss->rflow_state.state =
              (session::FlowTCPState)info.rflow_tcp_state;
            ss->rflow_state.tcp_seq_num = info.rflow_tcp_seq_num;
            ss->rflow_state.tcp_ack_num = info.rflow_tcp_ack_num;
            ss->rflow_state.tcp_win_sz = info.rflow_tcp_win_sz;
            ss->rflow_state.tcp_mss = info.rflow_tcp_mss;
            ss->rflow_state.tcp_win_scale = info.rflow_tcp_win_scale;
            ss->rflow_state.exception_bmap = info.rflow_exceptions_seen;
        }
    }

    flow_get_args.pd_session = pd_session;
    flow_get_args.role = FLOW_ROLE_INITIATOR;
    flow_get_args.flow_state = &ss->iflow_state;
    flow_get_args.aug = false;
    pd_func_args1.pd_flow_get = &flow_get_args;
    ret = pd_flow_get(&pd_func_args1);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Initiator Flow get failed session {}",
                      args->session->hal_handle);
        return ret;
    }

    flow_get_args.pd_session = pd_session;
    flow_get_args.role = FLOW_ROLE_RESPONDER;
    flow_get_args.flow_state = &ss->rflow_state;
    flow_get_args.aug = false;
    pd_func_args1.pd_flow_get = &flow_get_args;
    ret = pd_flow_get(&pd_func_args1);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Responder Flow get failed session {}",
                      args->session->hal_handle);
        return ret;
    }

    if (pd_session->iflow_aug.valid) {
        flow_get_args.pd_session = pd_session;
        flow_get_args.role = FLOW_ROLE_INITIATOR;
        flow_get_args.flow_state = &ss->iflow_aug_state;
        flow_get_args.aug = true;
        pd_func_args1.pd_flow_get = &flow_get_args;
        ret = pd_flow_get(&pd_func_args1);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("Initiator Aug Flow get failed session {}",
                          args->session->hal_handle);
            return ret;
        }
    }

    if (pd_session->rflow_aug.valid) {
        flow_get_args.pd_session = pd_session;
        flow_get_args.role = FLOW_ROLE_RESPONDER;
        flow_get_args.flow_state = &ss->rflow_aug_state;
        flow_get_args.aug = true;
        pd_func_args1.pd_flow_get = &flow_get_args;
        ret = pd_flow_get(&pd_func_args1);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("Responder Aug Flow get failed session {}",
                          args->session->hal_handle);
            return ret;
        }
    }

    return HAL_RET_OK;
}

typedef struct flow_atomic_stats_ {
    uint64_t permit_bytes:64;
    uint64_t permit_packets:64;
    uint64_t drop_bytes:64;
    uint64_t drop_packets:64;
} __PACK__ flow_atomic_stats_t;

//------------------------------------------------------------------------------
// get all flow related information
//------------------------------------------------------------------------------
hal_ret_t
pd_flow_get (pd_func_args_t *pd_func_args)
{
    hal_ret_t ret = HAL_RET_OK;
    pd_conv_hw_clock_to_sw_clock_args_t clock_args = {0};
    pd_flow_get_args_t *args = pd_func_args->pd_flow_get;
    sdk_ret_t sdk_ret;
    mem_addr_t              stats_addr = 0;
    flow_atomic_stats_t stats_0 = {0};
    flow_atomic_stats_t stats_1 = {0};
    flow_stats_actiondata_t d = {0};
    flow_info_actiondata_t f = {0};
    directmap *info_table = NULL;
    directmap *stats_table = NULL;
    pd_flow_t pd_flow;
    session_t *session = NULL;

    if (args->pd_session == NULL ||
        ((pd_session_t *)args->pd_session)->session == NULL) {
        return HAL_RET_INVALID_ARG;
    }

    session = (session_t *)((pd_session_t *)args->pd_session)->session;
    stats_table = g_hal_state_pd->dm_table(P4TBL_ID_FLOW_STATS);
    SDK_ASSERT(stats_table != NULL);

    if (args->aug == false) {
        if (args->role == FLOW_ROLE_INITIATOR) {
            pd_flow = args->pd_session->iflow;
        } else {
            pd_flow = args->pd_session->rflow;
        }
    } else {
        if (args->role == FLOW_ROLE_INITIATOR) {
            pd_flow = args->pd_session->iflow_aug;
        } else {
            pd_flow = args->pd_session->rflow_aug;
        }
    }

    ret = hal_pd_stats_addr_get(P4TBL_ID_FLOW_STATS,
                                pd_flow.assoc_hw_id, &stats_addr);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Error getting stats address for flow {} hw-id {} ret {}",
                      session->hal_handle, pd_flow.assoc_hw_id, ret);
        return ret;
    }

    sdk_ret = sdk::asic::asic_mem_read(stats_addr, (uint8_t *)&stats_0,
                                       sizeof(stats_0));
    if (sdk_ret != SDK_RET_OK) {
        HAL_TRACE_ERR("Error reading stats for flow {} hw-id {} ret {}",
                      session->hal_handle, pd_flow.assoc_hw_id, ret);
        return ret;
    }

    // read the d-vector
    sdk_ret = stats_table->retrieve(pd_flow.assoc_hw_id, &d);
    ret = hal_sdk_ret_to_hal_ret(sdk_ret);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Error reading stats action entry flow {} hw-id {} ret {}",
                      session->hal_handle, pd_flow.assoc_hw_id, ret);
        return ret;
    }

    sdk_ret = sdk::asic::asic_mem_read(stats_addr, (uint8_t *)&stats_1,
                                       sizeof(stats_1));
    if (sdk_ret != SDK_RET_OK) {
        HAL_TRACE_ERR("Error reading stats for flow {} hw-id {} ret {}",
                      session->hal_handle, pd_flow.assoc_hw_id, ret);
        return ret;
    }

#if STATS_DEBUG
    HAL_TRACE_DEBUG("Flow stats for session {} stats_addr: {:#x} stats_0: permit_packets {},"
                    "permit_bytes: {}, drop_packets: {}, drop_bytes: {} "
                    "stats_1: permit_packets {}, permit_bytes: {}, drop_packets: {},"
                    "drop_bytes: {} P4 table read: permit_packets {}, permit_bytes: {},"
                    "drop_packets: {}, drop_bytes: {} ", session->hal_handle, stats_addr,
                    stats_0.permit_packets, stats_0.permit_bytes, stats_0.drop_packets,
                    stats_0.drop_bytes, stats_1.permit_packets, stats_1.permit_bytes,
                    stats_1.drop_packets, stats_1.drop_bytes, d.action_u.flow_stats_flow_stats.permit_packets,
                    d.action_u.flow_stats_flow_stats.permit_bytes, d.action_u.flow_stats_flow_stats.drop_packets,
                    d.action_u.flow_stats_flow_stats.drop_bytes);
#endif

    if (stats_0.permit_packets == stats_1.permit_packets) {
        stats_1.permit_packets += d.action_u.flow_stats_flow_stats.permit_packets;
        stats_1.permit_bytes += d.action_u.flow_stats_flow_stats.permit_bytes;
    }

    if (stats_0.drop_packets == stats_1.drop_packets) {
        stats_1.drop_packets += d.action_u.flow_stats_flow_stats.drop_packets;
        stats_1.drop_bytes += d.action_u.flow_stats_flow_stats.drop_bytes;
    }

    args->flow_state->packets = stats_1.permit_packets;
    args->flow_state->bytes = stats_1.permit_bytes;
    args->flow_state->drop_packets = stats_1.drop_packets;
    args->flow_state->drop_bytes = stats_1.drop_bytes;

    clock_args.hw_tick = d.action_u.flow_stats_flow_stats.last_seen_timestamp;
    clock_args.hw_tick = (clock_args.hw_tick << 16);
    clock_args.sw_ns = &args->flow_state->last_pkt_ts;
    pd_func_args->pd_conv_hw_clock_to_sw_clock = &clock_args;
    pd_conv_hw_clock_to_sw_clock(pd_func_args);

    info_table = g_hal_state_pd->dm_table(P4TBL_ID_FLOW_INFO);
    SDK_ASSERT(info_table != NULL);

    sdk_ret = info_table->retrieve(pd_flow.assoc_hw_id, &f);
    ret = hal_sdk_ret_to_hal_ret(sdk_ret);
    if (ret == HAL_RET_OK) {
        if (f.action_id == FLOW_INFO_FLOW_HIT_DROP_ID) {
            clock_args.hw_tick = f.action_u.flow_info_flow_hit_drop.start_timestamp;

        } else {
            clock_args.hw_tick = f.action_u.flow_info_flow_info.start_timestamp;
        }
        clock_args.hw_tick = (clock_args.hw_tick << 16);
        clock_args.sw_ns = &args->flow_state->create_ts;
        pd_func_args->pd_conv_hw_clock_to_sw_clock = &clock_args;
        pd_conv_hw_clock_to_sw_clock(pd_func_args);
    }

    return ret;
}

//------------------------------------------------------------------------------------
// Add a bypass flow info entry
//------------------------------------------------------------------------------------
hal_ret_t
pd_add_cpu_bypass_flow_info (uint32_t *flow_info_hwid)
{
    timespec_t                           ts;
    pd_conv_sw_clock_to_hw_clock_args_t  clock_args;
    pd_func_args_t                       pd_clock_fn_args = {0};
    flow_info_actiondata_t                 d = { 0};
    hal_ret_t                            ret = HAL_RET_OK;
    sdk_ret_t                            sdk_ret;
    directmap                           *info_table;
    uint64_t                             sw_ns = 0, clock = 0;

    // Get the hw clock
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sdk::timestamp_to_nsecs(&ts, &sw_ns);
    clock_args.hw_tick = &clock;
    clock_args.sw_ns = sw_ns;
    pd_clock_fn_args.pd_conv_sw_clock_to_hw_clock = &clock_args;
    pd_conv_sw_clock_to_hw_clock(&pd_clock_fn_args);

    ret = p4pd_add_flow_stats_table_entry(flow_info_hwid, clock);
    if (ret != HAL_RET_OK) {
        return ret;
    }

    info_table = g_hal_state_pd->dm_table(P4TBL_ID_FLOW_INFO);
    SDK_ASSERT(info_table != NULL);

    d.action_id = FLOW_INFO_FLOW_INFO_FROM_CPU_ID;

    // insert the entry
    sdk_ret = info_table->insert_withid(&d, *flow_info_hwid);
    ret = hal_sdk_ret_to_hal_ret(sdk_ret);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("flow info table write failure, idx : {}, err : {}",
                       *flow_info_hwid, ret);
        return ret;
    }

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------------
// Get bypass flow info entry
//------------------------------------------------------------------------------------
hal_ret_t
pd_get_cpu_bypass_flowid (pd_func_args_t *pd_func_args)
{
    uint32_t                             flow_info_hwid = 0;
    pd_get_cpu_bypass_flowid_args_t     *args = pd_func_args->pd_get_cpu_bypass_flowid;

    if (!g_hal_state_pd->cpu_bypass_flowid()) {
        pd_add_cpu_bypass_flow_info(&flow_info_hwid);
        g_hal_state_pd->set_cpu_bypass_flowid(flow_info_hwid);
    }

    args->hw_flowid = g_hal_state_pd->cpu_bypass_flowid();

    return HAL_RET_OK;
}

// 
// Read the flow hash table given flow key
//
hal_ret_t
pd_flow_hash_get (pd_func_args_t *pd_func_args) {
    pd_flow_hash_get_args_t *args = pd_func_args->pd_flow_hash_get;
    hal_ret_t ret = HAL_RET_OK;
    flow_hash_swkey_t key = { 0 };

    p4pd_fill_flow_hash_key(&args->key,
            args->hw_vrf_id, args->lkp_inst, key);

    ret = g_hal_state_pd->flow_table_pd_get()->get(&key, args->rsp);
    if (ret != HAL_RET_OK) {
        return HAL_RET_OK;
    }

    return ret;
}

}    // namespace pd
}    // namespace hal
