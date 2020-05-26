#include "nic/fte/fte.hpp"
#include "nic/fte/fte_flow.hpp"
#include "nic/include/hal_mem.hpp"
#include "nic/hal/iris/datapath/p4/include/defines.h"
#include "nic/hal/src/utils/utils.hpp"
#include "nic/fte/fte_impl.hpp"
#include <tins/tins.h>

namespace fte {

// Process Session create in fte thread
hal_ret_t
session_create_in_fte (SessionSpec *spec, SessionResponse *rsp)
{
    hal_ret_t        ret = HAL_RET_OK;
    ctx_t            ctx = {};
    flow_t           iflow[ctx_t::MAX_STAGES], rflow[ctx_t::MAX_STAGES];
    uint16_t         num_features;
    size_t           fstate_size = feature_state_size(&num_features);
    feature_state_t *feature_state;

    feature_state = (feature_state_t*)HAL_MALLOC(hal::HAL_MEM_ALLOC_FTE, fstate_size);
    if (!feature_state) {
        ret = HAL_RET_OOM;
        goto end;
    }

    //Init context
    ret = ctx.init(spec, rsp,  iflow, rflow, feature_state, num_features);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("fte: failied to init context, ret={}", ret);
        goto end;
    }

    ret = ctx.process();

 end:
    rsp->set_api_status(hal::hal_prepare_rsp(ret));

    if (feature_state) {
        HAL_FREE(hal::HAL_MEM_ALLOC_FTE, feature_state);
    }

    if (ret == HAL_RET_OK && ctx.session())
        rsp->mutable_status()->set_session_handle(ctx.session()->hal_handle);

    return ret;
}

// Process Session create in fte thread
hal_ret_t
sync_session_in_fte (fte_session_args_t *sess_args)
{
    hal_ret_t        ret = HAL_RET_OK;
    ctx_t            ctx = {};
    flow_t           iflow[ctx_t::MAX_STAGES], rflow[ctx_t::MAX_STAGES];
    uint16_t         num_features;
    size_t           fstate_size = feature_state_size(&num_features);
    feature_state_t *feature_state;
    SessionSpec      spec;
    SessionStatus    status;
    SessionStats     stats;
    uint16_t         sync_sess_cnt = sess_args->sync_msg.sessions_size();
    hal::l2seg_t    *l2seg = hal::find_l2seg_by_id(sess_args->l2seg_id);

    feature_state = (feature_state_t*)HAL_MALLOC(hal::HAL_MEM_ALLOC_FTE, fstate_size);
    if (!feature_state) {
        return HAL_RET_OOM;
    }

    HAL_TRACE_INFO("sync session: cnt:{} l2seg:{} vrf:{}", sync_sess_cnt, sess_args->l2seg_id,
                   sess_args->vrf_handle);
    if (!l2seg) {
        HAL_TRACE_ERR("fte: l2seg not found. Id:{}", sess_args->l2seg_id);
    }

    for (auto i = 0; i < sync_sess_cnt; i++) {
        memset(feature_state, 0, fstate_size);

        spec   = sess_args->sync_msg.sessions(i).spec();
        status = sess_args->sync_msg.sessions(i).status();
        stats  = sess_args->sync_msg.sessions(i).stats();

        hal::proto_msg_dump(spec);

        //Init context
        ret = ctx.init(&spec, &status, &stats, l2seg, sess_args->vrf_handle, iflow, rflow,
                       feature_state, num_features);
        if (ret == HAL_RET_ENTRY_EXISTS) {
            HAL_TRACE_VERBOSE("fte: local session exists");
            updt_ep_to_session_db(ctx.sep(), ctx.dep(), ctx.session(), false);
            continue;
        } else if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("fte: failied to init context, ret={}", ret);
            continue;
        }

        ctx.process();
    }

    if (feature_state) {
        HAL_FREE(hal::HAL_MEM_ALLOC_FTE, feature_state);
    }

    HAL_TRACE_INFO("sync session end");

    return HAL_RET_OK;
}

// Process grpc session_create
hal_ret_t
session_create (SessionSpec& spec, SessionResponse *rsp)
{
    struct fn_ctx_t {
        SessionSpec&     spec;
        SessionResponse *rsp;
        hal_ret_t ret;
    } fn_ctx = { spec, rsp, HAL_RET_OK };

    hal::hal_api_trace(" API Begin: Session create ");
    hal::proto_msg_dump(spec);

    fte_execute(0, [](void *data) {
            fn_ctx_t *fn_ctx = (fn_ctx_t *)data;
            fn_ctx->ret = session_create_in_fte(&fn_ctx->spec, fn_ctx->rsp);
        }, &fn_ctx);


    HAL_TRACE_VERBOSE("----------------------- API End ------------------------");

    return fn_ctx.ret;
}

// Process Sync session create or update
hal_ret_t
sync_session (fte_session_args_t sess_args)
{
    struct fn_ctx_t {
        fte_session_args_t   args;
        hal_ret_t            ret;
    } fn_ctx = { sess_args, HAL_RET_OK };

    fte_execute(0, [](void *data) {
            fn_ctx_t *fn_ctx = (fn_ctx_t *)data;
            fn_ctx->ret = sync_session_in_fte(&fn_ctx->args);
        }, &fn_ctx);

    return fn_ctx.ret;
}

// Process Session delete in fte thread
hal_ret_t
session_delete_in_fte (hal_handle_t session_handle, bool force_delete)
{
    hal_ret_t ret;
    ctx_t ctx = {};
    uint16_t num_features;
    size_t fstate_size = feature_state_size(&num_features);
    feature_state_t *feature_state = NULL;
    flow_t iflow[ctx_t::MAX_STAGES], rflow[ctx_t::MAX_STAGES];
    hal::session_t *session;

    session = hal::find_session_by_handle(session_handle);
    if (session == NULL) {
        HAL_TRACE_VERBOSE("Invalid session handle {}", session_handle);
        return  HAL_RET_HANDLE_INVALID;
    }
    session->deleting = 1;

    HAL_TRACE_DEBUG("fte:: Received session Delete for session id {} force_delete: {}",
                    session->hal_handle, force_delete);

    HAL_TRACE_VERBOSE("num features: {} feature state size: {}", num_features, fstate_size);

    feature_state = (feature_state_t*)HAL_MALLOC(hal::HAL_MEM_ALLOC_FTE, fstate_size);
    if (!feature_state) {
        ret = HAL_RET_OOM;
        goto end;
    }

    // Process pkt with db open
    fte::impl::cfg_db_open();
    
    //Init context
    ret = ctx.init(session, iflow, rflow, feature_state, num_features);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("fte: failied to init context, ret={}", ret);
        goto end;
    }
    ctx.set_force_delete(force_delete);
    ctx.set_pipeline_event(FTE_SESSION_DELETE);

    ret = ctx.process();

    // close the config db
    fte::impl::cfg_db_close();

end:


    if (feature_state) {
        HAL_FREE(hal::HAL_MEM_ALLOC_FTE, feature_state);
    }
    return ret;
}

hal_ret_t
session_delete_async (hal::session_t *session, bool force_delete)
{
    struct fn_ctx_t {
        hal_handle_t session_handle;
        bool force_delete;
        hal_ret_t ret;
    };
    fn_ctx_t *fn_ctx = (fn_ctx_t *)HAL_MALLOC(hal::HAL_MEM_ALLOC_SESS_DEL_DATA, (sizeof(fn_ctx_t)));

    fn_ctx->session_handle  = session->hal_handle;
    fn_ctx->force_delete = force_delete;
    fn_ctx->ret = HAL_RET_OK;

    fte_softq_enqueue(session->fte_id, [](void *data) {
            fn_ctx_t *fn_ctx = (fn_ctx_t *) data;
            fn_ctx->ret = session_delete_in_fte(fn_ctx->session_handle, fn_ctx->force_delete);
            if (fn_ctx->ret != HAL_RET_OK) {
                HAL_TRACE_VERBOSE("session delete in fte failed for handle: {}", fn_ctx->session_handle);
            }
        HAL_FREE(hal::HAL_MEM_ALLOC_SESS_DEL_DATA, fn_ctx);
        }, fn_ctx);

    return fn_ctx->ret;
}

hal_ret_t
session_update_in_fte (hal_handle_t session_handle, uint64_t featureid_bitmap)
{
    hal_ret_t ret;
    ctx_t ctx = {};
    uint16_t num_features;
    size_t fstate_size = feature_state_size(&num_features);
    feature_state_t *feature_state = NULL;
    flow_t iflow[ctx_t::MAX_STAGES], rflow[ctx_t::MAX_STAGES];
    hal::session_t *session;

    session = hal::find_session_by_handle(session_handle);
    if (session == NULL) {
        HAL_TRACE_VERBOSE("Invalid session handle {}", session_handle);
        return  HAL_RET_HANDLE_INVALID;
    }

    // Bail if its already getting deleted
    if (session->deleting) {
        return HAL_RET_OK;
    }

    HAL_TRACE_DEBUG("fte:: Received session update for session id {}",
                    session->hal_handle);

    HAL_TRACE_VERBOSE("feature_bitmap: {} num features: {} feature state size: {}", featureid_bitmap, num_features, fstate_size);

    feature_state = (feature_state_t*)HAL_MALLOC(hal::HAL_MEM_ALLOC_FTE, fstate_size);
    if (!feature_state) {
        ret = HAL_RET_OOM;
        goto end;
    }

    //Init context
    ret = ctx.init(session, iflow, rflow, feature_state, num_features);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("fte: failed to init context, ret={}", ret);
        goto end;
    }
    ctx.set_pipeline_event(FTE_SESSION_UPDATE);
    ctx.set_featureid_bitmap(featureid_bitmap);

    ret = ctx.process();

end:
    if (feature_state) {
        HAL_FREE(hal::HAL_MEM_ALLOC_FTE, feature_state);
    }
    return ret;
}

hal_ret_t
session_update_async (hal::session_t *session, uint64_t featureid_bitmap)
{
    struct fn_ctx_t {
        hal_handle_t session_handle;
        uint64_t     featureid_bitmap;
        hal_ret_t ret;
    };
    fn_ctx_t *fn_ctx = (fn_ctx_t *)HAL_MALLOC(hal::HAL_MEM_ALLOC_SESS_UPD_DATA, (sizeof(fn_ctx_t)));

    fn_ctx->session_handle  = session->hal_handle;
    fn_ctx->featureid_bitmap = featureid_bitmap;
    fn_ctx->ret = HAL_RET_OK;
    
    HAL_TRACE_VERBOSE("Feature id bitmap: {}", fn_ctx->featureid_bitmap);
    fte_softq_enqueue(session->fte_id, [](void *data) {
            fn_ctx_t *fn_ctx = (fn_ctx_t *) data;
            fn_ctx->ret = session_update_in_fte(fn_ctx->session_handle, fn_ctx->featureid_bitmap);
            if (fn_ctx->ret != HAL_RET_OK) {
                HAL_TRACE_ERR("session update in fte failed for handle: {}", fn_ctx->session_handle);
            }
            HAL_FREE(hal::HAL_MEM_ALLOC_SESS_UPD_DATA, fn_ctx);
        }, fn_ctx);

    return fn_ctx->ret;
}

//------------------------------------------------------------------------
// Delete the sepecified session
// Should be called from non-fte thread
//------------------------------------------------------------------------
hal_ret_t
session_delete (hal::session_t *session, bool force_delete)
{
    struct fn_ctx_t {
        hal_handle_t session_handle;
        bool force_delete;
        hal_ret_t ret;
    } fn_ctx = { session->hal_handle, force_delete, HAL_RET_OK};

    fte_execute(session->fte_id, [](void *data) {
            fn_ctx_t *fn_ctx = (fn_ctx_t *) data;
            fn_ctx->ret = session_delete_in_fte(fn_ctx->session_handle, fn_ctx->force_delete);
        }, &fn_ctx);

    return fn_ctx.ret;
}

hal_ret_t
session_delete (SessionDeleteRequest& spec, SessionDeleteResponse *rsp)
{
    hal_ret_t        ret;
    hal::session_t  *session = NULL;

    hal::hal_api_trace(" API Begin: Session delete ");
    hal::proto_msg_dump(spec);

    session = hal::find_session_by_handle(spec.session_handle());
    if (session == NULL) {
        ret = HAL_RET_HANDLE_INVALID;
        goto end;
    }

    ret = session_delete(session, true);

end:
    rsp->set_api_status(hal::hal_prepare_rsp(ret));

    HAL_TRACE_DEBUG("----------------------- API End ------------------------");
    return ret;
}

hal_ret_t
session_get (hal::session_t *session, SessionGetResponse *response)
{
    hal_ret_t        ret;
    ctx_t            ctx = {};
    uint16_t         num_features;
    size_t           fstate_size = feature_state_size(&num_features);
    feature_state_t *feature_state = NULL;
    flow_t           iflow[ctx_t::MAX_STAGES], rflow[ctx_t::MAX_STAGES];

    HAL_TRACE_VERBOSE("--------------------- API Start ------------------------");
    HAL_TRACE_VERBOSE("fte:: Session handle {} Get",
                     session->hal_handle);

    feature_state = (feature_state_t*)HAL_MALLOC(hal::HAL_MEM_ALLOC_FTE, fstate_size);
    if (!feature_state) {
        ret = HAL_RET_OOM;
        goto end;
    }

    //Init context
    ret = ctx.init(session, iflow, rflow, feature_state, num_features);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("fte: failed to init context, ret={}", ret);
        goto end;
    }
    ctx.set_pipeline_event(FTE_SESSION_GET);
    ctx.set_sess_get_resp(response);

    // Execute pipeline without any changes to the session
    ctx.set_ignore_session_create(true);

    ret = ctx.process();

end:
    response->set_api_status(hal::hal_prepare_rsp(ret));

    if (feature_state) {
        HAL_FREE(hal::HAL_MEM_ALLOC_FTE, feature_state);
    }

    HAL_TRACE_VERBOSE("----------------------- API End ------------------------");
    return ret;
}

bool
session_is_feature_enabled(hal::session_t *session, const char *feature)
{
    sdk::lib::dllist_ctxt_t   *entry = NULL;
    uint16_t                   num_features;

    if (session == NULL || feature == NULL) {
        HAL_TRACE_ERR("Invalid argumet session: {:p} feature: {:p}",
                       (void *)session, (void *)feature);
        goto end;
    }

    feature_state_size(&num_features);
    dllist_for_each(entry, &session->feature_list_head) {
        feature_session_state_t *state =
            dllist_entry(entry, feature_session_state_t, session_feature_lentry);
        uint16_t id = feature_id(state->feature_name);
        // Look for the feature name in session. If it is
        // present then the assumtion is feature is enabled
        // for the session
        if (id <= num_features &&
            (strstr(state->feature_name, feature) != NULL)) {
            return true;
        }
    }
end:
    return false;
}

hal_ret_t 
fte_inject_pkt (cpu_rxhdr_t *cpu_rxhdr,
                std::vector<uint8_t *> &pkts, size_t pkt_len, bool copied_pkt_arg)
{
    ctx_t ctx;

    struct fn_ctx_t {
        ctx_t *ctx;
        cpu_rxhdr_t *cpu_rxhdr;
        std::vector<uint8_t *> &pkts;
        size_t pkt_len;
        bool   copied_pkt;
        hal_ret_t ret;
    } fn_ctx = { &ctx, cpu_rxhdr, pkts, pkt_len, copied_pkt_arg, HAL_RET_OK };

    fte::fte_execute(0, [](void *data) {
        fn_ctx_t *fn_ctx = (fn_ctx_t *)data;
        auto pkt = fn_ctx->pkts[0];
        fte::ctx_t *ctx = fn_ctx->ctx;

        fte::flow_t iflow[fte::ctx_t::MAX_STAGES], rflow[fte::ctx_t::MAX_STAGES];
        uint16_t num_features;
        size_t fstate_size = fte::feature_state_size(&num_features);
        fte::feature_state_t *feature_state = (fte::feature_state_t*)HAL_MALLOC(hal::HAL_MEM_ALLOC_FTE, fstate_size);

        for (int i=0; i<fte::ctx_t::MAX_STAGES; i++) {
            iflow[i]  =  {};
            rflow[i]  =  {};
        }

        for (uint32_t i=0; i<fn_ctx->pkts.size(); i++) {
            hal::hal_cfg_db_open(hal::CFG_OP_READ);
            fn_ctx->ret = ctx->init(fn_ctx->cpu_rxhdr, pkt, fn_ctx->pkt_len, fn_ctx->copied_pkt,
                                    iflow, rflow, feature_state, num_features);
            if (fn_ctx->ret == HAL_RET_OK) {
                fn_ctx->ret = ctx->process();
            }
            SDK_ASSERT(fn_ctx->ret == HAL_RET_OK);
            hal::hal_cfg_db_close();
        }
        HAL_FREE(hal::HAL_MEM_ALLOC_FTE, feature_state);
    }, &fn_ctx );

    return HAL_RET_OK;
}

hal_ret_t
fte_inject_eth_pkt (const lifqid_t &lifq,
                    hal_handle_t src_ifh, hal_handle_t src_l2segh,
                    std::vector<Tins::EthernetII> &pkts, bool add_padding)
{
    hal::pd::pd_if_get_hw_lif_id_args_t lif_args;
    hal::if_t *sif = hal::find_if_by_handle(src_ifh);
    hal::l2seg_t *l2seg = hal::l2seg_lookup_by_handle(src_l2segh);

    // use first pkt to build cpu header
    Tins::EthernetII eth = pkts[0];

    uint8_t vlan_valid;
    uint16_t vlan_id;
    hal::if_l2seg_get_encap(sif, l2seg, &vlan_valid, &vlan_id);

    hal::pd::pd_l2seg_get_flow_lkupid_args_t args;
    hal::pd::pd_func_args_t pd_func_args = {0};
    args.l2seg = l2seg;
    pd_func_args.pd_l2seg_get_flow_lkupid = &args;
    hal::pd::hal_pd_call(hal::pd::PD_FUNC_ID_L2SEG_GET_FLOW_LKPID, &pd_func_args);
    hal::pd::l2seg_hw_id_t hwid = args.hwid;

    if (sif->if_type == intf::IF_TYPE_ENIC) {
        hal::pd::pd_func_args_t pd_func_args = {0};
        lif_args.pi_if = sif;
        pd_func_args.pd_if_get_hw_lif_id = &lif_args;
        hal::pd::hal_pd_call(hal::pd::PD_FUNC_ID_IF_GET_HW_LIF_ID, &pd_func_args);
    }

    fte::cpu_rxhdr_t cpu_rxhdr = {};
    cpu_rxhdr.src_lif = (sif->if_type == intf::IF_TYPE_ENIC)?lif_args.hw_lif_id:sif->if_id;
    cpu_rxhdr.lif = lifq.lif;
    cpu_rxhdr.qtype = lifq.qtype;
    cpu_rxhdr.qid = lifq.qid;
    cpu_rxhdr.lkp_vrf = hwid;
    cpu_rxhdr.lkp_dir = sif->if_type == intf::IF_TYPE_ENIC ? FLOW_DIR_FROM_DMA :
        FLOW_DIR_FROM_UPLINK;
    cpu_rxhdr.lkp_inst = 0;
    cpu_rxhdr.lkp_type = FLOW_KEY_LOOKUP_TYPE_IPV4;
    cpu_rxhdr.l2_offset = 0;
  
    cpu_rxhdr.l3_offset = eth.header_size() + eth.find_pdu<Tins::Dot1Q>()->header_size();
    cpu_rxhdr.l4_offset = cpu_rxhdr.l3_offset + eth.find_pdu<Tins::IP>()->header_size();
    cpu_rxhdr.payload_offset = cpu_rxhdr.l4_offset;

    auto l4pdu = eth.find_pdu<Tins::IP>()->inner_pdu();
    if (l4pdu) {
        cpu_rxhdr.payload_offset +=  l4pdu->header_size();
    }

    cpu_rxhdr.flags = 0;
    Tins::TCP *tcp = eth.find_pdu<Tins::TCP>();
    if (tcp) {
        cpu_rxhdr.tcp_seq_num = ntohl(tcp->seq());
        cpu_rxhdr.tcp_flags = ((tcp->get_flag(Tins::TCP::SYN) << 1) |
                               (tcp->get_flag(Tins::TCP::ACK) << 4));
    }

    std::vector<uint8_t *>buffs;
    size_t buff_size;
    bool copied_pkt = true;

    for (auto pkt: pkts) {
        vector<uint8_t> buffer = pkt.serialize();
        if (add_padding) {
            buff_size += 4;
            buffer.resize(buff_size, 0);
        }
        buffs.push_back(&buffer[0]);
    }

    return fte_inject_pkt(&cpu_rxhdr, buffs, buff_size, copied_pkt);
}

hal_ret_t
session_update_ep_in_fte (hal_handle_t session_handle)
{
    hal_ret_t ret = HAL_RET_OK;
    ctx_t ctx = {};
    uint16_t num_features;
    size_t fstate_size = feature_state_size(&num_features);
    feature_state_t *feature_state = NULL;
    flow_t iflow[ctx_t::MAX_STAGES], rflow[ctx_t::MAX_STAGES];
    hal::session_t *session;

    session = hal::find_session_by_handle(session_handle);
    if (session == NULL) {
        HAL_TRACE_VERBOSE("Invalid session handle {}", session_handle);
        return  HAL_RET_HANDLE_INVALID;
    }

    // Bail if its already getting deleted
    if (session->deleting) {
        return HAL_RET_OK;
    }

    HAL_TRACE_DEBUG("fte:: Received session update for session id {}",
                    session->hal_handle);

    HAL_TRACE_VERBOSE("num features: {} feature state size: {}", num_features, fstate_size);

    feature_state = (feature_state_t*)HAL_MALLOC(hal::HAL_MEM_ALLOC_FTE, fstate_size);
    if (!feature_state) {
        ret = HAL_RET_OOM;
        goto end;
    }

    //Init context
    ret = ctx.init(session, iflow, rflow, feature_state, num_features);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("fte: failed to init context, ret={}", ret);
        goto end;
    }
    ctx.set_pipeline_event(FTE_SESSION_UPDATE);
    // Set the logger instance
    if (!ctx.ipc_logging_disable()) {
        ctx.set_ipc_logger(get_current_ipc_logger_inst());
    } 

    HAL_TRACE_VERBOSE("Sep: {} Dep: {} Sep Dir: {} Dep Dir: {}", 
                      ctx.sep_handle(), ctx.dep_handle(), 
                      ((ctx.sep() && (ctx.sep()->ep_flags & EP_FLAGS_LOCAL)) ?FLOW_DIR_FROM_DMA : FLOW_DIR_FROM_UPLINK),
                      ((ctx.dep() && (ctx.dep()->ep_flags & EP_FLAGS_LOCAL)) ?FLOW_DIR_FROM_DMA : FLOW_DIR_FROM_UPLINK));
    session->sep_handle  = ctx.sep_handle();
    session->dep_handle  = ctx.dep_handle(); 
    session->iflow->config.dir = (ctx.sep() && (ctx.sep()->ep_flags & EP_FLAGS_LOCAL)) ?
            FLOW_DIR_FROM_DMA : FLOW_DIR_FROM_UPLINK;
    session->rflow->config.dir = (ctx.dep() && (ctx.dep()->ep_flags & EP_FLAGS_LOCAL)) ?
            FLOW_DIR_FROM_DMA : FLOW_DIR_FROM_UPLINK;
   
    ctx.flow_log()->sfw_action = (nwsec::SecurityAction)ctx.session()->sfw_action;
    ctx.flow_log()->rule_id = ctx.session()->sfw_rule_id;
    ctx.flow_log()->alg = (nwsec::ALGName)ctx.session()->alg; 
    ctx.add_flow_logging(ctx.key(), session->hal_handle, ctx.flow_log(), 
                         (hal::flow_direction_t)session->iflow->config.dir);

end:
    return ret;
}

} // namespace fte
