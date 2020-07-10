//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#include "node.h"
#include "pdsa_uds_hdlr.h"
#include <nic/vpp/infra/ipc/pdsa_vpp_hdlr.h>
#include "sess_api.h"
#include "log.h"
#include <assert.h>
#include <sess.h>
#include <sess_helper.h>
#include <nic/vpp/infra/utils.h>
#include "fixup.h"

typedef struct ipv4_flow_params_s {
    u32 sip;
    u32 dip;
    u16 sport;
    u16 dport;
    u16 proto;
    u16 lkp_id;
    u8 thread_id;
} ipv4_flow_params_t;

typedef struct ipv6_flow_params_s {
    u8 sip[16];
    u8 dip[16];
    u16 sport;
    u16 dport;
    u16 proto;
    u16 lkp_id;
    u8 thread_id;
} ipv6_flow_params_t;

int
pds_get_ipv4_flow_params (pds_flow_hw_ctx_t *session,
                          ipv4_flow_params_t *flow_params)
{
    ftlv4 *table4 = pds_flow_prog_get_table4();
    int thread = vlib_get_thread_index();
    int ret = 0;
    pds_flow_index_t flow_index;

    flow_index.handle = session->iflow.handle;
    if (ftlv4_get_with_handle(table4, flow_index.handle,
                              thread) != 0) {
        ret = -1;
        goto end;
    }

    ftlv4_get_last_read_session_info(&flow_params->sip, &flow_params->dip,
                                     &flow_params->sport, &flow_params->dport,
                                     &flow_params->lkp_id, thread);
    flow_params->thread_id = session->thread_id;
    ret = 0;

end:
    return ret;
}

always_inline bool
is_ipv4_addr_match (u32 addr1, u32 addr2)
{
    if (addr1 == addr2) {
        return true;
    }
    return false;
}

always_inline bool
is_ipv6_addr_match (u8 *addr1, u8 *addr2)
{
    if (!(memcmp(addr1, addr2, IP6_ADDR8_LEN))) {
        return true;
    }
    return false;
}

always_inline int
pds_flow_lookup_id_update (pds_flow_hw_ctx_t *session, bool iflow,
                           u16 lookup_id)
{
    ftlv4 *table4 = (ftlv4 *)pds_flow_get_table4();
    u64 new_handle;
    pds_flow_main_t *fm = &pds_flow_main;
    u32 ses_id = session - fm->session_index_pool + 1;
    u64 handle = iflow ? session->iflow.handle : session->rflow.handle;
    bool l2l = (session->packet_type == PDS_FLOW_L2L_INTER_SUBNET ||
                session->packet_type == PDS_FLOW_L2L_INTRA_SUBNET) ? true : false;
    if (PREDICT_FALSE(ftlv4_insert_with_new_lookup_id(table4, handle,
                                                      &new_handle,
                                                      lookup_id, l2l) != 0)) {
        flow_log_error("%s ftlv4 insert failed", __FUNCTION__);
        return -1;
    }
    pds_session_update_data(ses_id, new_handle, iflow, false);
    return 0;
}

int
pds_flow_delete (pds_flow_hw_ctx_t *session, u64 handle)
{
    int thread = vlib_get_thread_index();

    // we dont need locks here as main thread is blocked during fixup,
    // so no thread accesses session as we are here in worker thread context.
    // there can be i/r flow index update from other threads but they are
    // considered atomic as index memory is always 64 byte aligned.
    if (session->v4) {
        ftlv4 *table4 = (ftlv4 *)pds_flow_get_table4();
        if (PREDICT_FALSE(ftlv4_get_with_handle(table4, handle, thread) !=0)) {
            goto end;
        }
        if (PREDICT_FALSE(ftlv4_remove_cached_entry(table4, thread)) != 0) {
            goto end;
        }
    } else {
        ftl *table = (ftl *)pds_flow_get_table6_or_l2();
        if (PREDICT_FALSE(ftlv6_get_with_handle(table, handle) !=0)) {
            goto end;
        }
        if (PREDICT_FALSE(ftlv6_remove_cached_entry(table)) != 0) {
            goto end;
        }
    }
    return 0;

end:
    return -1;
}

static void
pds_flow_fixup_rflow (pds_flow_fixup_data_t *data,
                      u32 ses_id, u16 thread_id)
{
    int ret;
    u64 handle;
    u16 bd_id, tx_rewrite, rx_rewrite;
    bool l2l;
    ftlv4 *table4 = pds_flow_prog_get_table4();
    pds_flow_hw_ctx_t *session = pds_flow_get_session(ses_id);

    if (PREDICT_FALSE(!session)) {
        return;
    }

    switch (session->packet_type) {
    case PDS_FLOW_L2L_INTER_SUBNET:
    case PDS_FLOW_L2R_INTER_SUBNET:
        // Update software session data
        if (session->packet_type == PDS_FLOW_L2L_INTER_SUBNET) {
            session->packet_type = PDS_FLOW_L2R_INTER_SUBNET;
            // In this case, get bd_id from iflow
            ret = ftlv4_get_with_handle(table4,
                                        session->iflow.handle,
                                        thread_id);
            if (PREDICT_FALSE(ret != 0)) {
                return;
            }
            ftlv4_get_last_read_entry_lookup_id(&bd_id, thread_id);
        } else {
            session->packet_type = PDS_FLOW_L2L_INTER_SUBNET;
            bd_id = data->bd_id;
        }

        // We need to insert the rflow with the new bd_id and then delete
        // the old flow
        handle = session->rflow.handle;
        if (PREDICT_FALSE(pds_flow_lookup_id_update(session, false,
                                                    bd_id) != 0)) {
            return;
        }
        if (PREDICT_FALSE(pds_flow_delete(session, handle) != 0)) {
            return;
        }
        break;
    case PDS_FLOW_L2L_INTRA_SUBNET:
    case PDS_FLOW_L2R_INTRA_SUBNET:
        // Update session packet type
        if (session->packet_type == PDS_FLOW_L2L_INTRA_SUBNET) {
            session->packet_type = PDS_FLOW_L2R_INTRA_SUBNET;
            l2l = false;
        } else {
            session->packet_type = PDS_FLOW_L2L_INTRA_SUBNET;
            l2l = true;
        }

        // In this case, we only need to update l2l flag in the rflow.
        ret = ftlv4_get_with_handle(table4,
                                    session->rflow.handle,
                                    thread_id);
        if (PREDICT_FALSE(ret != 0)) {
            return;
        }

        ftlv4_set_last_read_entry_l2l(l2l, thread_id);
        if (PREDICT_FALSE(ftlv4_update_cached_entry(table4,
                                                    thread_id) != 0)) {
            return;
        }
        break;
    case PDS_FLOW_R2L_INTRA_SUBNET:
    case PDS_FLOW_R2L_INTER_SUBNET:
        // In this case, we are moving destination from local to remote, so delete the
        // session
        pds_flow_delete_session(ses_id);
        return;
    default:
        assert(0);
    }

    // In all cases, update the iflow as well
    if (pds_flow_packet_l2l(session->packet_type)) {
        l2l = true;
        // Also set session data
        session->dst_vnic_id = data->vnic_id;
    } else {
        l2l = false;
    }

    ret = ftlv4_get_with_handle(table4,
                                session->iflow.handle,
                                thread_id);
    if (PREDICT_FALSE(ret != 0)) {
        return;
    }

    ftlv4_set_last_read_entry_l2l(l2l, thread_id);
    if (PREDICT_FALSE(ftlv4_update_cached_entry(table4,
                                                thread_id) != 0)) {
        return;
    }

    // Also, update the session flags
    pds_session_get_rewrite_flags(ses_id, session->packet_type,
                                  &tx_rewrite, &rx_rewrite);
    pds_session_update_rewrite_flags(ses_id, tx_rewrite, rx_rewrite);
    return;
}

static void
pds_flow_fixup_iflow (pds_flow_fixup_data_t *data,
                      u32 ses_id, u16 thread_id)
{
    int ret;
    u64 handle;
    u16 bd_id, tx_rewrite, rx_rewrite;
    bool l2l;
    ftlv4 *table4 = pds_flow_prog_get_table4();
    pds_flow_hw_ctx_t *session = pds_flow_get_session(ses_id);

    if (PREDICT_FALSE(!session)) {
        return;
    }

    switch (session->packet_type) {
    case PDS_FLOW_L2L_INTER_SUBNET:
    case PDS_FLOW_R2L_INTER_SUBNET:
        // Update session packet type
        if (session->packet_type == PDS_FLOW_L2L_INTER_SUBNET) {
            session->packet_type = PDS_FLOW_R2L_INTER_SUBNET;
            // In this case, get bd_id from rflow
            ret = ftlv4_get_with_handle(table4,
                                        session->rflow.handle,
                                        thread_id);
            if (PREDICT_FALSE(ret != 0)) {
                return;
            }
            ftlv4_get_last_read_entry_lookup_id(&bd_id, thread_id);
        } else {
            session->packet_type = PDS_FLOW_L2L_INTER_SUBNET;
            bd_id = data->bd_id;
        }

        // We need to insert the iflow with the new bd_id and then delete
        // the old flow
        handle = session->iflow.handle;
        if (PREDICT_FALSE(pds_flow_lookup_id_update(session, true,
                                                    bd_id) != 0)) {
            return;
        }
        if (PREDICT_FALSE(pds_flow_delete(session, handle) != 0)) {
            return;
        }
        break;
    case PDS_FLOW_L2L_INTRA_SUBNET:
    case PDS_FLOW_R2L_INTRA_SUBNET:
        // Update session packet type
        if (session->packet_type == PDS_FLOW_L2L_INTRA_SUBNET) {
            session->packet_type = PDS_FLOW_R2L_INTRA_SUBNET;
            l2l = false;
        } else {
            session->packet_type = PDS_FLOW_L2L_INTRA_SUBNET;
            l2l = true;
        }

        // In this case, we only need to update l2l flag in the iflow.
        ret = ftlv4_get_with_handle(table4,
                                    session->iflow.handle,
                                    thread_id);
        if (PREDICT_FALSE(ret != 0)) {
            return;
        }

        ftlv4_set_last_read_entry_l2l(l2l, thread_id);
        if (PREDICT_FALSE(ftlv4_update_cached_entry(table4,
                                                    thread_id) != 0)) {
            return;
        }
        break;
    case PDS_FLOW_L2R_INTRA_SUBNET:
    case PDS_FLOW_L2R_INTER_SUBNET:
        // In this case, we are moving source from local to remote, so delete the
        // session
        pds_flow_delete_session(ses_id);
        return;
    default:
        assert(0);
    }

    // In all cases, update the rflow as well
    if (pds_flow_packet_l2l(session->packet_type)) {
        l2l = true;
        // Also update software session data
        session->src_vnic_id = data->vnic_id;
        session->iflow_rx = false;
        session->ingress_bd = data->bd_id;
    } else {
        l2l = false;
    }

    ret = ftlv4_get_with_handle(table4,
                                session->rflow.handle,
                                thread_id);
    if (PREDICT_FALSE(ret != 0)) {
        return;
    }

    ftlv4_set_last_read_entry_l2l(l2l, thread_id);
    if (PREDICT_FALSE(ftlv4_update_cached_entry(table4,
                                                thread_id) != 0)) {
        return;
    }

    // Also, update the session flags
    // Also, update the session flags
    pds_session_get_rewrite_flags(ses_id, session->packet_type,
                                  &tx_rewrite, &rx_rewrite);
    pds_session_update_rewrite_flags(ses_id, tx_rewrite, rx_rewrite);
    return;
}

static void
pds_flow_fixup_internal_cb (vlib_main_t * vm)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_fixup_data_t *data = &fm->fixup_data;
    pds_flow_move_thread_t *fixup;

    fixup = &data->move_sess[vm->thread_index];
    for (int i = 0; i < fixup->cur_len; i++) {
        if (fixup->data[i].iflow) {
            // This means iflow needs to be fixed up
            pds_flow_fixup_iflow(data,
                                 fixup->data[i].session_id,
                                 vm->thread_index);
        } else {
            pds_flow_fixup_rflow(data,
                                 fixup->data[i].session_id,
                                 vm->thread_index);
        }
    }
    fixup->cur_len = 0;
    fixup = &data->delete_sess[vm->thread_index];
    for (int i = 0; i < fixup->cur_len; i++) {
        pds_flow_delete_session(fixup->data[i].session_id);
    }
    fixup->cur_len = 0;

    clib_callback_enable_disable
        (vm->worker_thread_main_loop_callbacks,
         vm->worker_thread_main_loop_callback_tmp,
         vm->worker_thread_main_loop_callback_lock,
         (void *) pds_flow_fixup_internal_cb, 0 /* disable */ );
}

always_inline void
pds_flow_fixup_data_set (pds_flow_fixup_data_t *data,
                         u16 bd_id, u16 vnic_id)
{
    data->bd_id = bd_id;
    data->vnic_id = vnic_id;
}

always_inline void
pds_flow_fixup_internal (int thread_id)
{
    clib_callback_enable_disable
        (vlib_mains[thread_id]->worker_thread_main_loop_callbacks,
         vlib_mains[thread_id]->worker_thread_main_loop_callback_tmp,
         vlib_mains[thread_id]->worker_thread_main_loop_callback_lock,
         (void *) pds_flow_fixup_internal_cb, 1 /* enable */ );
    while (clib_callback_is_set
           (vlib_mains[thread_id]->worker_thread_main_loop_callbacks,
            vlib_mains[thread_id]->worker_thread_main_loop_callback_lock,
            (void *) pds_flow_fixup_internal_cb)) {
        usleep(1);
    }
    return;
}

// 1.1.1.1 (belonging to bd 1) and 2.2.2.2 (belonging to bd 2)
//1.L2L inter subnet - > R2L inter
//  iflow - 1.1.1.1 -> 2.2.2.2 -> 1 ==> 1.1.1.1 -> 2.2.2.2 -> 2 -> Delete and
//  Add new as key is changing.
//  rflow - 2.2.2.2 -> 1.1.1.1 -> 2 ==> 2.2.2.2 -> 1.1.1.1 -> 2 -> no change.
//2.L2L inter -> L2R inter
//  iflow - 1.1.1.1 -> 2.2.2.2 -> 1 ==> 1.1.1.1 -> 2.2.2.2 -> 1 -> no change
//  rflow - 2.2.2.2 -> 1.1.1.1 -> 2 ==> 2.2.2.2 -> 1.1.1.1 -> 1 -> Delete and
//  Add new as key is changing
//3.R2L inter subnet - > L2L inter
//  iflow - 1.1.1.1 -> 2.2.2.2 -> 2 ==> 1.1.1.1 -> 2.2.2.2 -> 1 -> Delete and
//  Add new as key is changing
//  rflow - 2.2.2.2 -> 1.1.1.1 -> 2 ==> 2.2.2.2 -> 1.1.1.1 -> 2 -> no change
//4.L2R inter subnet -> L2L
//  iflow - 1.1.1.1 -> 2.2.2.2 -> 1 ==> 1.1.1.1 -> 2.2.2.2 -> 1 -> no change
//  rflow - 2.2.2.2 -> 1.1.1.1 -> 1 ==> 2.2.2.2 -> 1.1.1.1 -> 2 Delete and
//  Add new as key is changing.
//5. intra subnet cases(except R2L_INTRA_SUBNET) - only session flags need to be
//updated
always_inline bool
pds_each_flow_fixup (u32 event_id, u32 addr,
                     u32 ses_id, u16 bd_id,
                     u16 vnic_id, bool del)
{
    pds_flow_main_t *fm = &pds_flow_main;
    pds_flow_hw_ctx_t *session = pds_flow_get_session_and_lock(ses_id);
    bool ret = true, iflow;
    pds_flow_pkt_type pkt_type;
    pds_flow_move_thread_t *fixup;
    int max_retry = 25;

    if (PREDICT_FALSE(session == NULL)) {
        return ret;
    }
    while (!session->prog_done) {
        usleep(1);
        if (0 == --max_retry) {
            // this flow is not ready for fixup yet, so return
            goto done;
        }
    }
    pkt_type = session->packet_type;

    if (session->v4) {
        ipv4_flow_params_t flow_params;
        int ret_val;

        ret_val = pds_get_ipv4_flow_params(session, &flow_params);
        if (PREDICT_FALSE(ret_val == -1)) {
            ret = false;
            goto done;
        }

        // if we are changing from L2L to R2L or vice versa iflow needs
        // to be changed.
        // if we are changing from L2R to L2L or vice versa rflow needs
        // to be changed.
        if (addr == flow_params.sip) {
            iflow = true;
        } else if (addr == flow_params.dip) {
            iflow = false;
        } else {
            goto done;
        }

        if (del || (pkt_type >= PDS_FLOW_L2N_ASYMMETRIC_ROUTE)) {
            fixup = &fm->fixup_data.delete_sess[flow_params.thread_id];
        } else {
            fixup = &fm->fixup_data.move_sess[flow_params.thread_id];
        }
        if (fixup->cur_len >= PDS_FLOW_FIXUP_BATCH_SIZE) {
            pds_flow_fixup_internal(flow_params.thread_id);
        }
        fixup->data[fixup->cur_len].iflow = iflow;
        fixup->data[fixup->cur_len++].session_id = ses_id;
    } else {
        // TODO Handle ipv6
    }
done:
    pds_flow_session_unlock(ses_id);
    return ret;
}

void
pds_ip_flow_fixup (u32 event_id, u32 ip_addr,
                   u16 bd_id, u16 vnic_id, bool del)
{
    pds_flow_main_t *fm = &pds_flow_main;
    u32 ses_id, thread_id;

    pds_flow_fixup_data_set(&fm->fixup_data, bd_id, vnic_id);
    for (ses_id = 1; ses_id <= fm->max_sessions; ses_id++) {
        if (!pds_each_flow_fixup(event_id, ip_addr, ses_id,
                                 bd_id, vnic_id, del)) {
            flow_log_error("Failed to fixup session %u", ses_id);
        }
    }
    for (thread_id = 1; thread_id < fm->no_threads; thread_id++) {
        pds_flow_fixup_internal(thread_id);
    }
    return;
}

static clib_error_t *
pds_flow_fixup_init (vlib_main_t *vm)
{
    pds_vpp_learn_subscribe();
    return 0;
}

VLIB_INIT_FUNCTION (pds_flow_fixup_init) =
{
    .runs_after = VLIB_INITS("pds_infra_init"),
};
