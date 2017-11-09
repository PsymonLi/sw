#include "nic/hal/plugins/network/net_plugin.hpp"
#include "nic/hal/src/session.hpp"
#include "nic/p4/nw/include/defines.h"
#include "nic/hal/src/nwsec_group.hpp"
#include "nic/hal/src/nwsec.hpp"
#include "nic/hal/plugins/network/net_dfw.hpp"

namespace hal {
namespace net {

#define DEFAULT_MSS 536 // RFC6691
#define DEFAULT_WINDOW_SIZE 512

hal_ret_t
net_dfw_match_rules(fte::ctx_t                  &ctx,
                    nwsec_policy_rules_t        *nwsec_plcy_rules,
                    fte::net_dfw_match_result_t *match_rslt)
{
    flow_key_t          flow_key;
    nwsec_policy_svc_t  *nwsec_plcy_svc = NULL;
    dllist_ctxt_t       *lnode = NULL;
    flow_role_t          role;

    role = ctx.role();
    flow_key = ctx.get_key(role);
    HAL_TRACE_DEBUG(" proto {} dport {}", flow_key.proto, ctx.get_key(role).dport);
    dllist_for_each(lnode, &nwsec_plcy_rules->fw_svc_list_head) {
        nwsec_plcy_svc = dllist_entry(lnode, nwsec_policy_svc_t, lentry);
        if (nwsec_plcy_svc != NULL) {
            //ToDo (lseshan) - identify the wildcard better way
            if (nwsec_plcy_svc->ipproto == 0) {
                // Match all
                match_rslt->valid  = 1;
                match_rslt->alg    = nwsec_plcy_svc->alg;
                match_rslt->action = (session::FlowAction)nwsec_plcy_rules->action;
                match_rslt->log    = nwsec_plcy_rules->log;
                HAL_TRACE_DEBUG("Wild card Rule match {} {}", match_rslt->action, match_rslt->log);
                return HAL_RET_OK;
                
            }
            //Compare proto
            if (nwsec_plcy_svc->ipproto == flow_key.proto) {
                if (nwsec_plcy_svc->ipproto == IPPROTO_ICMP ||
                    nwsec_plcy_svc->ipproto == IPPROTO_ICMPV6) {
                    //Check the icmp message type
                    if (nwsec_plcy_svc->icmp_msg_type == flow_key.icmp_type)
                    {
                        match_rslt->valid    = 1;
                        match_rslt->alg      = nwsec_plcy_svc->alg;
                        match_rslt->action   = (session::FlowAction) nwsec_plcy_rules->action;
                        match_rslt->log      = nwsec_plcy_rules->log;
                        return HAL_RET_OK;
                    }
                } else {
                    //Check the port match
                    if (nwsec_plcy_svc->dst_port == ctx.get_key(role).dport) {
                        //Fill the match actions
                        match_rslt->valid    = 1;
                        match_rslt->alg      = nwsec_plcy_svc->alg;
                        match_rslt->action   = (session::FlowAction) nwsec_plcy_rules->action;
                        match_rslt->log      = nwsec_plcy_rules->log;
                        return HAL_RET_OK;
                    }
                }
            }
        }
    }

    return HAL_RET_FTE_RULE_NO_MATCH;
}

hal_ret_t
net_dfw_check_policy_pair(fte::ctx_t                    &ctx,
                          uint32_t                      src_sg,
                          uint32_t                      dst_sg,
                          fte::net_dfw_match_result_t   *match_rslt)
{
    nwsec_policy_cfg_t    *nwsec_plcy_cfg = NULL;
    dllist_ctxt_t         *lnode = NULL;
    nwsec_policy_rules_t  *nwsec_plcy_rules = NULL;
    hal_ret_t             ret = HAL_RET_OK;

    nwsec_policy_key_t plcy_key;

    plcy_key.sg_id = src_sg;
    plcy_key.peer_sg_id = dst_sg;

    HAL_TRACE_DEBUG("fte::Lookup Policy for SG Pair: {} {}", src_sg, dst_sg);

    nwsec_plcy_cfg = nwsec_policy_cfg_lookup_by_key(plcy_key);
    if (nwsec_plcy_cfg == NULL) {
        HAL_TRACE_ERR("Failed to get the src_sg: {} peer_sg {}", src_sg, dst_sg);
        return HAL_RET_SG_NOT_FOUND;
    }

    dllist_for_each(lnode, &nwsec_plcy_cfg->rules_head) {
        nwsec_plcy_rules = dllist_entry(lnode, nwsec_policy_rules_t, lentry);
        ret = net_dfw_match_rules(ctx, nwsec_plcy_rules, match_rslt);
        if (ret == HAL_RET_OK && match_rslt->valid) {
            return ret;
        }
    }
    return ret;
}

hal_ret_t
net_dfw_pol_check_sg_policy(fte::ctx_t                  &ctx,
                            fte::net_dfw_match_result_t *match_rslt)
{
    ep_t        *sep = NULL;
    ep_t        *dep = NULL;
    hal_ret_t    ret;


    if (ctx.protobuf_request()) {
        match_rslt->valid  = 1;
        match_rslt->action = ctx.sess_spec()->initiator_flow().flow_data().flow_info().flow_action();
        return HAL_RET_OK;
    }

    if (ctx.drop_flow()) {
        match_rslt->valid = 1;
        match_rslt->action = session::FLOW_ACTION_DROP;
        return HAL_RET_OK;
    }

    //ToDo (lseshan) - For now if a sep or dep is not found allow
    // Eventually use prefix to find the sg
    if (ctx.sep() == NULL ||  (ctx.dep() == NULL)) {
        match_rslt->valid = 1;
        match_rslt->action = session::FLOW_ACTION_ALLOW;
        return HAL_RET_OK;
    }

    //HACK:Todo(lseshan) for sun rpc - until figure out to use ctx.get_key(role) or ctx.key()
    if (ctx.key().dport == 111) {
        match_rslt->alg   =   nwsec::APP_SVC_SUN_RPC;
        match_rslt->valid = 1;
        match_rslt->action = session::FLOW_ACTION_ALLOW;
        return HAL_RET_OK;
    }

    sep  = ctx.sep();
    dep  = ctx.dep();
    for (int i = 0; i < sep->sgs.sg_id_cnt; i++) {
        for (int j = 0; j < dep->sgs.sg_id_cnt; j++) {
            ret = net_dfw_check_policy_pair(ctx,
                                            sep->sgs.arr_sg_id[i],
                                            dep->sgs.arr_sg_id[j],
                                            match_rslt);            if (ret == HAL_RET_OK) {
                if (match_rslt->valid) {
                    return ret;
                }
            } else {
                // Handle error case
            }
        }
    }

    // ToDo (lseshan) Handle SP miss condition
    // For now hardcoding to ALLOW but we have read default action and act
    // accordingly
    match_rslt->valid = 1;
    match_rslt->action = session::FLOW_ACTION_ALLOW;

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// extract all the TCP related state from session spec
//------------------------------------------------------------------------------
static hal_ret_t
net_dfw_extract_session_state_from_spec (fte::flow_state_t *flow_state,
                                 const session::FlowData& flow_data)
{
    auto conn_track_info = flow_data.conn_track_info();
    flow_state->state = flow_data.flow_info().tcp_state();
    flow_state->tcp_seq_num = conn_track_info.tcp_seq_num();
    flow_state->tcp_ack_num = conn_track_info.tcp_ack_num();
    flow_state->tcp_win_sz = conn_track_info.tcp_win_sz();
    flow_state->tcp_win_scale = conn_track_info.tcp_win_scale();
    flow_state->tcp_mss = conn_track_info.tcp_mss();
    flow_state->create_ts = conn_track_info.flow_create_ts();
    flow_state->last_pkt_ts = flow_state->create_ts;
    flow_state->packets = conn_track_info.flow_packets();
    flow_state->bytes = conn_track_info.flow_bytes();
    flow_state->exception_bmap = conn_track_info.exception_bits();

    return HAL_RET_OK;
}


static inline bool
net_dfw_conn_tracking_configured(fte::ctx_t &ctx)
{

    if (ctx.protobuf_request()) {
        return ctx.sess_spec()->conn_track_en();
    }

    if (ctx.key().proto != types::IPPROTO_TCP) {
        return false;
    }

    // lookup Security profile
    if (ctx.tenant()->nwsec_profile_handle  != HAL_HANDLE_INVALID) {
        hal::nwsec_profile_t  *nwsec_prof =
            find_nwsec_profile_by_handle(ctx.tenant()->nwsec_profile_handle);
        if (nwsec_prof != NULL) {
            return nwsec_prof->cnxn_tracking_en;
        }
    }
    return false;
}

fte::pipeline_action_t
dfw_exec(fte::ctx_t& ctx)
{
    hal_ret_t                      ret;
    fte::net_dfw_match_result_t    match_rslt;

    // security policy action
    fte::flow_update_t flowupd = {type: fte::FLOWUPD_ACTION};

    // ToDo (lseshan) - for now handling only ingress rules
    // Need to select SPs based on the flow direction
    // 
    if (ctx.role() == hal::FLOW_ROLE_INITIATOR) {
        ret = net_dfw_pol_check_sg_policy(ctx, &match_rslt);
        if (ret == HAL_RET_OK) {
            if (match_rslt.valid) {
                flowupd.action  = match_rslt.action;
                ctx.set_alg_proto(match_rslt.alg);
                //ctx.log         = match_rslt.log;
            } else {
                // ToDo ret value was ok but match_rslt.valid is 0
                // to handle the case if it happens
            }
        }
    } else {
        //Responder Role: Not checking explicitly
        if (ctx.drop_flow()) {
            flowupd.action = session::FLOW_ACTION_DROP;
        } else {
            flowupd.action = session::FLOW_ACTION_ALLOW;
        }
    }

    ret = ctx.update_flow(flowupd);
    if (ret != HAL_RET_OK) {
        ctx.set_feature_status(ret);
        return fte::PIPELINE_END;
    }

    //Todo(lseshan) Move connection tracking to different function
    // connection tracking
    if (!net_dfw_conn_tracking_configured(ctx)) {
        return fte::PIPELINE_CONTINUE;
    }

    //iflow
    flowupd = {type: fte::FLOWUPD_FLOW_STATE};

    if (ctx.role() == hal::FLOW_ROLE_INITIATOR) {
        if (ctx.protobuf_request()) {
            net_dfw_extract_session_state_from_spec(&flowupd.flow_state,
                                            ctx.sess_spec()->initiator_flow().flow_data());
            flowupd.flow_state.syn_ack_delta = ctx.sess_spec()->iflow_syn_ack_delta();
        } else {
            const fte::cpu_rxhdr_t *rxhdr = ctx.cpu_rxhdr();
            flowupd.flow_state.state = session::FLOW_TCP_STATE_INIT;
            // Expectation is to program the next expected seq num.
            flowupd.flow_state.tcp_seq_num = ntohl(rxhdr->tcp_seq_num) + 1;
            flowupd.flow_state.tcp_ack_num = ntohl(rxhdr->tcp_ack_num);
            flowupd.flow_state.tcp_win_sz = ntohs(rxhdr->tcp_window);

            // TCP Options
            if (rxhdr->flags & CPU_FLAGS_TCP_OPTIONS_PRESENT) {
                // MSS Option
                flowupd.flow_state.tcp_mss = DEFAULT_MSS;
                if (rxhdr->tcp_options & CPU_TCP_OPTIONS_MSS) {
                    flowupd.flow_state.tcp_mss = ntohs(rxhdr->tcp_mss);
                }

                // Window Scale option
                flowupd.flow_state.tcp_ws_option_sent = 0;
                if (rxhdr->tcp_options & CPU_TCP_OPTIONS_WINDOW_SCALE) {
                    flowupd.flow_state.tcp_ws_option_sent = 1;
                    flowupd.flow_state.tcp_win_scale = rxhdr->tcp_ws;
                }

                // timestamp option
                flowupd.flow_state.tcp_ts_option_sent = 0;
                if (rxhdr->tcp_options & CPU_TCP_OPTIONS_TIMESTAMP) {
                    flowupd.flow_state.tcp_ts_option_sent = 1;
                }
            }
        }
    } else { /* rflow */
        if (ctx.protobuf_request()) {
            net_dfw_extract_session_state_from_spec(&flowupd.flow_state,
                                            ctx.sess_spec()->responder_flow().flow_data());
        } else {
            flowupd.flow_state.state = session::FLOW_TCP_STATE_INIT;
            flowupd.flow_state.tcp_mss = DEFAULT_MSS;
            flowupd.flow_state.tcp_win_sz = DEFAULT_WINDOW_SIZE; //This is to allow the SYN Packet to go through.
        }
    }

    ret = ctx.update_flow(flowupd);
    if (ret != HAL_RET_OK) {
        ctx.set_feature_status(ret);
        return fte::PIPELINE_END;
    }

    return fte::PIPELINE_CONTINUE;
}

} // namespace net
} // namespace hal
