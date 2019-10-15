//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#pragma once

#include "nic/include/fte.hpp"
#include "nic/hal/plugins/sfw/alg_utils/core.hpp"
#include "nic/hal/plugins/sfw/core.hpp"

namespace hal {
namespace plugins {
namespace alg_rpc {

using namespace hal::plugins::alg_utils;
using namespace hal::plugins::sfw;

#define RPC_DEFAULT_TIMEOUT (3600 * 1000) // 60 mins in msecs

/*
 * Externs
 */
extern alg_state_t *g_rpc_state;
extern tcp_buffer_slab_t g_rpc_tcp_buffer_slabs;

/*
 * Constants
 */
const std::string FTE_FEATURE_ALG_RPC("pensando.io/sfw:alg_rpc");
#define MAX_ALG_RPC_PKT_SZ  3000
#define WORD_BYTES     4
#define SHORT_BYTES    2
#define UUID_SZ        16

/*
 * Function prototypes
 */

// plugin.cc
fte::pipeline_action_t alg_rpc_exec(fte::ctx_t &ctx);
void rpcinfo_cleanup_hdlr(l4_alg_status_t *l4_sess);
typedef size_t (*rpc_cb_t)(void *ctx, uint8_t *pkt, size_t pkt_len);
hal_ret_t expected_flow_handler(fte::ctx_t &ctx, expected_flow_t *wentry);
void insert_rpc_expflow(fte::ctx_t& ctx, l4_alg_status_t *l4_sess,
                        rpc_cb_t cb, uint32_t timeout=RPC_DEFAULT_TIMEOUT);
hal_ret_t alg_msrpc_exec(fte::ctx_t& ctx, sfw_info_t *sfw_info, l4_alg_status_t *l4_sess);
hal_ret_t alg_sunrpc_exec(fte::ctx_t& ctx, sfw_info_t *sfw_info, l4_alg_status_t *l4_sess);
uint8_t *alloc_rpc_pkt(void);
fte::pipeline_action_t alg_rpc_session_delete_cb(fte::ctx_t &ctx);
fte::pipeline_action_t alg_rpc_session_get_cb(fte::ctx_t &ctx);

hal_ret_t alg_rpc_init(hal_cfg_t *hal_cfg);
void alg_rpc_exit();

/*
 * Data Structures
 */
typedef struct rpc_info_ {
    /*
     * Ctrl Session Expected Flow Info
     */
    uint8_t           *pkt;
    uint32_t           pkt_len;
    uint32_t           payload_offset;
    bool               skip_sfw;
    rpc_cb_t           callback;
    uint8_t            rpc_frag_cont;
    uint8_t            addr_family;
    ipvx_addr_t        ip;
    uint32_t           prot;
    uint32_t           dport;
    uint32_t           vers;
    uint8_t            pkt_type;
    uint8_t            pgmid_sz;
    void              *pgm_ids;
    union {
        struct { /* SUNRPC Info */
            uint32_t   xid;
            uint32_t   prog_num;
            uint8_t    rpcvers;
        } __PACK__;
        struct { /* MSRPC Info */
            uint8_t    data_rep;
            uint32_t   call_id;
            uint8_t    act_id[16];
            uint8_t    uuid[UUID_SZ];
            uint8_t    msrpc_64bit;
            uint8_t    msrpc_ctxt_id[256];
            uint8_t    num_msrpc_ctxt;
        } __PACK__;
    } __PACK__;
    /*
     * L4 Ctrl session oper status
     */
    uint32_t data_sess;
    uint32_t parse_errors;
    uint32_t maxpkt_sz_exceeded;
    uint32_t num_exp_flows;
} rpc_info_t;

void incr_parse_error(rpc_info_t *info);
void incr_data_sess(rpc_info_t *info);
void incr_max_pkt_sz(rpc_info_t  *info);
void incr_num_exp_flows(rpc_info_t *info);
void copy_sfw_info(sfw_info_t *sfw_info, l4_alg_status_t *l4_sess);

}  // namespace alg_rpc
}  // namespace plugins
}  // namespace hal
