//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#include <stddef.h>
#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/tcp/tcp.h>
#include <vnet/session/session.h>
#include <vnet/dpo/load_balance.h>
#include <vnet/plugin/plugin.h>

#ifdef TCP_RTO_MAX
#undef TCP_RTO_MAX
#endif
#ifdef TCP_RTO_MIN
#undef TCP_RTO_MIN
#endif
#ifdef TCP_TIMER_TICK
#undef TCP_TIMER_TICK
#endif
#include "nic/include/tcp_common.h"
#include "nic/utils/tcpcb_pd/tcpcb_pd.h"
#include "tcp_prog_hw.hpp"
#include "nic/include/pds_socket.h"
#include "nic/sdk/third-party/vpp-pkg/include/vnet/tcp/tcp_debug.h"
//#include "nic/sdk/third-party/vpp/src/vnet/tcp/tcp_debug.h"

VLIB_PLUGIN_REGISTER () = {
    .description = "Pensando TCP Plugin",
};

pds_tcp_connection_info_t  *pi_tcp_connection_by_qid; //vpp tcp info

tcp_connection_t *
pds_get_tcp_connection_from_qid (uint32_t hw_qid);

enum {
    TCP_CONN_CLOSED,
    TCP_CONN_OFFLOADED,
    TCP_CONN_CLOSE_REQ,
    TCP_CONN_CLEANED,
    TCP_CONN_MAX_STATES
};

char *tcp_conn_state_string[TCP_CONN_MAX_STATES] =
{
    "CLOSED", // TCP_CONN_CLOSED
    "OFFLOADED", // TCP_CONN_OFFLOADED
    "CLOSE_REQ", // TCP_CONN_CLOSE_REQ
    "CLEANED", // TCP_CONN_CLEANED
};

char *tcp_conn_close_rsn_string[NUM_CLOSE_RSN] =
{
    "CLOSE_REASON_NONE", // CLOSE_REASON_NONE
    "ACTIVE_GRACEFUL", // ACTIVE_GRACEFUL
    "PASSIVE_GRACEFUL", // PASSIVE_GRACEFUL
    "LOCAL_RST", // LOCAL_RST
    "REMOTE_RST", // REMOTE_RST
    "LOCAL_RST_FW2_TO", // LOCAL_RST_FW2_TO
    "LOCAL_RST_KA_TO", // LOCAL_RST_KA_TO
    "LOCAL_RST_RTO", // LOCAL_RST_RTO
};

static void
pds_tcp_fill_tcp_params(tcp_connection_t *con,
                        tcpcb_pd_t *sdk_tcpcb)
{
    sdk_tcpcb->rcv_nxt = con->rcv_nxt;
    sdk_tcpcb->snd_nxt = con->snd_nxt;
    sdk_tcpcb->rcv_wup = con->rcv_nxt;
    sdk_tcpcb->rcv_wnd = con->rcv_wnd;
    sdk_tcpcb->snd_una = con->snd_una;
    sdk_tcpcb->rcv_wscale = con->rcv_wscale;
    sdk_tcpcb->snd_wnd = con->snd_wnd >> con->snd_wscale;
    sdk_tcpcb->smss = con->snd_mss - ((TCP_OPTS_ALIGN - con->snd_mss % TCP_OPTS_ALIGN) % TCP_OPTS_ALIGN) ;
    sdk_tcpcb->rcv_mss = con->mss;
    sdk_tcpcb->snd_cwnd = 10 * sdk_tcpcb->smss;
    // use the following for rfc 6928 compliance
    //sdk_tcpcb->snd_cwnd = min(10*sdk_tcpcb->smss, max(2*sdk_tcpcb->smss, 14600));
    // Use large initial window to do perf testing
    //sdk_tcpcb->snd_cwnd = 10000000;
    sdk_tcpcb->initial_window = 10000000;
    sdk_tcpcb->snd_ssthresh = con->ssthresh;
    sdk_tcpcb->snd_wscale = con->snd_wscale;
    if (sdk_tcpcb->app_type == TCP_APP_TYPE_BYPASS) {
	    sdk_tcpcb->rto =  TCP_RTO_MIN_TICK;         //rto is in 1MS
    } else {      
	    sdk_tcpcb->rto =  con->rto;         //rto is in ms resolution
    }
    sdk_tcpcb->srtt_us = con->srtt * 100;     //srtt_us is in 10us
    sdk_tcpcb->rttvar_us = con->rttvar * 100; //rttvar_us is in 10us units;
    sdk_tcpcb->rtt_seq_tsoffset = tcp_time_now_w_thread (con->c_thread_index) + 1;
    sdk_tcpcb->ts_recent = con->tsval_recent;
}

static int
pds_tcp_fill_l2_params(tcp_connection_t *con, tcpcb_pd_t *sdk_tcpcb,
                       int *l2_hdr_size)
{
    u8 *buf = sdk_tcpcb->header_template;
    fib_prefix_t pfx = { 0 };
    fib_entry_t *fib_entry;
    const dpo_id_t *dpo;
    ip_adjacency_t *adj;
    vnet_rewrite_header_t *rw;
    load_balance_t *lb;

    fib_prefix_from_ip46_addr(&con->connection.rmt_ip, &pfx);

    fib_node_index_t fei = fib_table_lookup_exact_match(
            con->connection.fib_index, &pfx);
    if (fei == FIB_NODE_INDEX_INVALID) {
        return -1;
    }

    fib_entry = fib_entry_get(fei);
    if (!dpo_id_is_valid(&fib_entry->fe_lb)) {
        /* route not resolved yet */
        return -1;
    }
    dpo = (dpo_id_t *)&fib_entry->fe_lb;
    switch (dpo->dpoi_type) {
        case DPO_LOAD_BALANCE:
            lb = load_balance_get(dpo->dpoi_index);
            dpo = load_balance_get_bucket_i(lb, 0);
        case DPO_ADJACENCY:
            break;
        default:
            return -1;
    }

    adj = adj_get(dpo->dpoi_index);

    rw = &adj->rewrite_header;
    // Skip ethertype
    *l2_hdr_size = rw->data_bytes - 2;
    if (*l2_hdr_size <= 0 || *l2_hdr_size > 16) {
        return -1;
    }

    // Set L2 info in header template
    clib_memcpy(buf, rw->data, *l2_hdr_size);

    return 0;
}

static void
pds_tcp_ip4_proxy_nat(tcp_connection_t *con,
                      uint32_t *lcl_ip, uint32_t *rmt_ip)
{
    ip4_proxy_params_t *proxy = ip4_proxy_get_params();
    if (con->connection.lcl_ip.ip4.as_u32 == proxy->my_ip) {
        if (con->connection.rmt_ip.ip4.as_u32 == proxy->server_ip) {
            *lcl_ip = proxy->client_ip;
        } else {
            *lcl_ip = proxy->server_ip;
        }
    } else { // rmt_ip == my_ip
        if (con->connection.lcl_ip.ip4.as_u32 == proxy->server_ip) {
            *rmt_ip = proxy->client_ip;
        } else {
            *rmt_ip = proxy->server_ip;
        }
    }
}

static void
pds_tcp_get_4_tuple_from_con(tcp_connection_t *con, u8 app_type, u32 *lcl_ip,
                             u32 *rmt_ip, u16 *lcl_port, u16 *rmt_port)
{
    *lcl_ip = con->connection.lcl_ip.ip4.as_u32;
    *rmt_ip = con->connection.rmt_ip.ip4.as_u32;
    if (app_type == TCP_APP_TYPE_BYPASS) {
        pds_tcp_ip4_proxy_nat(con, lcl_ip, rmt_ip);
    }
    *lcl_port = con->connection.lcl_port;
    *rmt_port = con->connection.rmt_port;
}

static void
pds_tcp_fill_ip4_params(tcp_connection_t *con, tcpcb_pd_t *sdk_tcpcb,
                        int l2_hdr_size)
{
    u8 *buf = sdk_tcpcb->header_template;
    u8 i = 0;
    uint32_t lcl_ip, rmt_ip;
    uint16_t lcl_port, rmt_port;

    pds_tcp_get_4_tuple_from_con(con, sdk_tcpcb->app_type, &lcl_ip,
                                 &rmt_ip, &lcl_port, &rmt_port);

    // Set L3 info in header template
    i += l2_hdr_size;

    SET_COMMON_IP_HDR(buf, i);
    clib_memcpy(&buf[i], &lcl_ip, sizeof(u32));
    i += 4;
    clib_memcpy(&buf[i], &rmt_ip, sizeof(u32));
    i += 4;

    sdk_tcpcb->header_len = i;

    // Set L4 port
    sdk_tcpcb->source_port = clib_net_to_host_u16(lcl_port);
    sdk_tcpcb->dest_port = clib_net_to_host_u16(rmt_port);
}

void
pds_tcp_alloc_store_hw_offload_info (tcp_connection_t *vpp_tconn,
                                     pds_sockopt_hw_offload_t *info)
{
    pds_sockopt_hw_offload_t *linfo = NULL;
    pds_tcp_connection_info_t *tinfo;

    if (!vpp_tconn->hw_offload) {
        linfo = (pds_sockopt_hw_offload_t *)clib_mem_alloc(sizeof(pds_sockopt_hw_offload_t));
        if (linfo) {
            clib_memcpy(linfo, info, sizeof(pds_sockopt_hw_offload_t));

            tinfo = &pi_tcp_connection_by_qid[info->hw_qid];
            tinfo->c_index = vpp_tconn->c_c_index;
            tinfo->thread_index = vpp_tconn->c_thread_index;
            tinfo->state = TCP_CONN_OFFLOADED;
            tinfo->app_type = info->app_type;
            vpp_tconn->hw_offload = linfo;
        }
    } else {
        TCP_DBG ("hw_offload already stored ! %p", vpp_tconn->hw_offload);
    }
    return;
}

tcp_connection_t *
pds_get_tcp_connection_from_qid (uint32_t hw_qid) {

    pds_tcp_connection_info_t *tinfo;
    tcp_connection_t *tc;

    tinfo = &pi_tcp_connection_by_qid[hw_qid];
    tc = tcp_connection_get(tinfo->c_index, tinfo->thread_index);
    return(tc);
}

void
pds_tcp_free_hw_offload_info (tcp_connection_t *vpp_tconn)
{
    pds_sockopt_hw_offload_t *linfo;
    uint32_t hw_qid;

    if (vpp_tconn->hw_offload) {
        linfo = (pds_sockopt_hw_offload_t *)vpp_tconn->hw_offload;
        hw_qid = linfo->hw_qid;

        pi_tcp_connection_by_qid[hw_qid].state = TCP_CONN_CLOSED;

        clib_mem_free(vpp_tconn->hw_offload);
        vpp_tconn->hw_offload = NULL;

        if (pi_tcp_connection_by_qid[hw_qid].app_type !=
              TCP_APP_TYPE_ARM_SOCKET) {
            // Connection close complete. CLEANED --> CLOSED. Delete vppconn
            tcp_connection_del(vpp_tconn);
        }
    }
    return;
}

int
pds_tcp_hw_offload_handler (tcp_connection_t *con, u64 option,
                            void *data, u32 data_len)
{
    tcpcb_pd_t sdk_tcpcb = { 0 };
    int l2_hdr_size = 0;
    int ret;
    uint32_t lcl_ip, rmt_ip;
    uint16_t lcl_port, rmt_port;

    if (data_len != sizeof(pds_sockopt_hw_offload_t)) {
        TCP_DBG ("data_len %d expect %d", data_len, sizeof(pds_sockopt_hw_offload_t));
        return 1;
    }

    pds_tcp_fill_def_params(&sdk_tcpcb);

    pds_tcp_fill_app_params(&sdk_tcpcb, data, data_len);

    if (pds_tcp_fill_l2_params(con, &sdk_tcpcb, &l2_hdr_size) != 0) {
        TCP_DBG ("pds_tcp_fill_l2_params error");
        return -1;
    }

    pds_tcp_fill_tcp_params(con, &sdk_tcpcb);

    if (con->connection.is_ip4) {
        pds_tcp_fill_ip4_params(con, &sdk_tcpcb, l2_hdr_size);
    }

    ret = pds_tcp_prog_tcpcb(&sdk_tcpcb);
    if (ret != 0) {
        TCP_DBG ("pds_tcp_prog_tcpcb error %d", ret);
        return -1;
    }

    pds_tcp_get_4_tuple_from_con(con, sdk_tcpcb.app_type, &lcl_ip,
                                 &rmt_ip, &lcl_port, &rmt_port);
    pds_tcp_prog_ip4_flow(sdk_tcpcb.qid, true,
                          lcl_ip, rmt_ip, lcl_port, rmt_port,
                          sdk_tcpcb.app_type == TCP_APP_TYPE_BYPASS); // proxy

    pds_tcp_fill_app_return_params(&sdk_tcpcb, data, data_len);
    if ((sdk_tcpcb.app_type == TCP_APP_TYPE_BYPASS) || (sdk_tcpcb.app_type == TCP_APP_TYPE_ARM_SOCKET)) {
        pds_tcp_fill_proxy_return_params(&sdk_tcpcb, data, data_len);
    }
    pds_tcp_alloc_store_hw_offload_info(con, data);
    return ret;
}

int
pds_hw_pre_offload_test_handler (tcp_connection_t *con)
{
    int ret = 0;
    pds_sockopt_hw_offload_t req = {0};

    req.app_type = TCP_APP_TYPE_ARM_SOCKET;
    ret = pds_tcp_hw_offload_handler(con, 0, (void *)&req, 
                                     sizeof(pds_sockopt_hw_offload_t));
    return ret;
}


int
pds_tcp_close_handler (tcp_connection_t *con)
{
    int ret = 0;

    /*
     * Do the relevant cleanups.
     */

    pds_tcp_free_hw_offload_info(con);
    return ret;
}

int
pds_hw_pre_offload_handler (tcp_connection_t *con)
{
    int ret = 0;

    return ret;
}

int
pds_tcp_window_scale_handler (void)
{
    int wscale;

    /*
     * Max congestion window can grow to is carried as a window scale number
     * in SYN/SYN-ACK packet. Its (2 ** 16) * (2 ** wscale). 
     *
     * Since our SERQ ring is 1K(2 ** 10) in size and with 8K(2 ** 13) byte
     * page size, we are looking at (2 ** 10) * (2 ** 13) max window size i.e
     * (2 ** 23) window size. 
     */
    wscale = (23 - 16);
    return wscale;
}

void pds_tcp_cleanup_proc(uint32_t hw_qid, uint8_t reason)
{
    tcp_connection_t    *con;
    int ret;
    u32 lcl_ip, rmt_ip;
    u16 lcl_port, rmt_port;
    pds_tcp_connection_info_t *tinfo = &pi_tcp_connection_by_qid[hw_qid];

    // Sanity check for the expected connection close flow
    if (!((tinfo->app_type != TCP_APP_TYPE_ARM_SOCKET &&
          tinfo->state == TCP_CONN_OFFLOADED) ||
          (tinfo->app_type == TCP_APP_TYPE_ARM_SOCKET &&
          tinfo->state == TCP_CONN_CLOSED))) {
        TCP_DBG ("Error: unexpected state hw_qid %d app_type %d state %d",
              hw_qid, tinfo->app_type, tinfo->state);
        return;
    }

    con = pds_get_tcp_connection_from_qid(hw_qid);
    if (con) {
        pds_tcp_get_4_tuple_from_con(con, tinfo->app_type, &lcl_ip,
                                     &rmt_ip, &lcl_port, &rmt_port);
        // TEMPORARY: DO NOT DELETE FLOW UNTIL FLOW DELETE FIXED
#if 0
        pds_tcp_del_ip4_flow(hw_qid, true,
                             lcl_ip, rmt_ip, lcl_port, rmt_port);
#endif
    } else {
        // Assert condition. MUST not happen
        TCP_DBG ("Error: VPP Conn NULL %d", hw_qid);
        return;
    }

    ret = pds_tcp_del_tcpcb(hw_qid);
    if (ret) {
        TCP_DBG ("Error: pds_tcp_del_tcpcb failed hw_id %d", hw_qid);
        return;
    }

    pi_tcp_connection_by_qid[hw_qid].state = TCP_CONN_CLEANED;
    pi_tcp_connection_by_qid[hw_qid].close_reason = reason;

    if (tinfo->app_type != TCP_APP_TYPE_ARM_SOCKET) {
        /*
         * Notify VPP about the close, so it changes its state for App
         * to be notified error on epoll
         */
        TCP_DBG ("Calling session_transport_reset_notify con %p hw_id %d", con, hw_qid);
        session_transport_reset_notify(&con->connection);
        //session_transport_closing_notify(&con->connection);
    } else {
        // Connection close is complete. CLOSED --> CLEANED. Delete vppconn
        tcp_connection_del(con);
    }
}

static clib_error_t *
pds_tcp_init (vlib_main_t * vm)
{
    clib_error_t *error = NULL;
    tcp_set_sock_option_cb_data cb_data = {0};
    int num_tcp_flows;

    if ((error = vlib_call_init_function (vm, ip_main_init)))
        goto done;

    if ((error = vlib_call_init_function (vm, ip4_lookup_init)))
        goto done;

    if ((error = vlib_call_init_function (vm, ip6_lookup_init)))
        goto done;

    if ((error = vlib_call_init_function (vm, tcp_init)))
        goto done;

    cb_data.opt_handler = pds_tcp_hw_offload_handler;
    (void)tcp_register_setsockopt_handler(SESSION_OPTIONS_HW_OFFLOAD, &cb_data);

    (void)tcp_register_close_handler(pds_tcp_close_handler);
    (void)tcp_register_hw_pre_offload_handler(pds_hw_pre_offload_handler);
    (void)tcp_register_hw_pre_offload_test_handler(pds_hw_pre_offload_test_handler);
    (void)tcp_register_window_scale_handler(pds_tcp_window_scale_handler);

    (void)pds_init();

    num_tcp_flows = pds_get_max_num_tcp_flows();
    pi_tcp_connection_by_qid = clib_mem_alloc(sizeof(pds_tcp_connection_info_t) * num_tcp_flows);
    if (!pi_tcp_connection_by_qid) {
        //Err
    }
done:
    return error;
}

VLIB_INIT_FUNCTION (pds_tcp_init);

static inline int tcp_state_active(pds_tcp_connection_info_t *tinfo)
{
    if ((tinfo->app_type == TCP_APP_TYPE_ARM_SOCKET &&
          tinfo->state != TCP_CONN_CLEANED) ||
          (tinfo->app_type != TCP_APP_TYPE_ARM_SOCKET &&
          tinfo->state != TCP_CONN_CLOSED))
        return 1;

    return 0;
}

static inline int tcp_state_internal_error(pds_tcp_connection_info_t *tinfo)
{
  if (!tcp_state_active(tinfo) && (tinfo->close_reason >= INTERNAL_ERROR_START))
    return 1;

  return 0;
}

static clib_error_t *
show_tcp_plugin_session_info_cmd_fn(vlib_main_t * vm,
          unformat_input_t * input, vlib_cli_command_t * cmd_arg)
{
  int qid, active_conns = 0, error_conns = 0;
  int max_tcp_flows = pds_get_max_num_tcp_flows();
  pds_tcp_connection_info_t *tinfo;

  if (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
    if (unformat (input, "error")) {
      for (qid = 0; qid < max_tcp_flows; qid++) {
        tinfo = &pi_tcp_connection_by_qid[qid];
        if (tcp_state_internal_error(tinfo)) {
          error_conns++;
          vlib_cli_output(vm, "hw_qid: %u state: %s close_rsn %s c_idx: %d t_idx: %d\n", qid,
          tcp_conn_state_string[tinfo->state], tcp_conn_close_rsn_string[tinfo->close_reason],
             tinfo->c_index, tinfo->thread_index);
        }
      }
      vlib_cli_output(vm, "%d error connection/s\n", error_conns);
    }
  } else {
    for (qid = 0; qid < max_tcp_flows; qid++) {
      tinfo = &pi_tcp_connection_by_qid[qid];
      if (tcp_state_active(tinfo)) {
        active_conns++;
        vlib_cli_output(vm, "hw_qid: %u state: %s c_idx: %d t_idx: %d\n", qid,
        tcp_conn_state_string[tinfo->state], tinfo->c_index,
           tinfo->thread_index);
      }
    }
    vlib_cli_output(vm, "%d active connection/s\n", active_conns);
  }
  return 0;
}

VLIB_CLI_COMMAND (tcp_plugin_session_cmd, static) =
{
    .path = "show tcp-plugin session",
    .short_help = "show tcp-plugin session info",
    .function = show_tcp_plugin_session_info_cmd_fn,
};
