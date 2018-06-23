//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __TCPCB_HPP__
#define __TCPCB_HPP__

#include "nic/include/base.h"
#include "nic/include/encap.hpp"
#include "sdk/list.hpp"
#include "sdk/ht.hpp"
#include "nic/gen/proto/hal/tcp_proxy_cb.pb.h"
#include "nic/include/pd.hpp"
#include "nic/include/hal_state.hpp"

using sdk::lib::ht_ctxt_t;
using sdk::lib::dllist_ctxt_t;


using tcpcb::TcpCbSpec;
using tcpcb::TcpCbStatus;
using tcpcb::TcpCbResponse;
using tcpcb::TcpCbKeyHandle;
using tcpcb::TcpCbRequestMsg;
using tcpcb::TcpCbResponseMsg;
using tcpcb::TcpCbDeleteRequestMsg;
using tcpcb::TcpCbDeleteResponseMsg;
using tcpcb::TcpCbGetRequest;
using tcpcb::TcpCbGetRequestMsg;
using tcpcb::TcpCbGetResponse;
using tcpcb::TcpCbGetResponseMsg;

#define INVALID_HEADER_TEMPLATE_LEN ((uint32_t)-1)

namespace hal {

typedef struct tcpcb_s {
    hal_spinlock_t        slock;                   // lock to protect this structure
    tcpcb_id_t            cb_id;                   // TCP CB id
    uint32_t              rcv_nxt;
    uint32_t              snd_nxt;
    uint32_t              snd_una;
    uint32_t              rcv_tsval;
    uint32_t              ts_recent;
    uint64_t              rx_ts;
    uint64_t              serq_base;
    uint32_t              debug_dol;
    uint32_t              sesq_pi;
    uint32_t              sesq_ci;
    uint64_t              sesq_base;
    uint32_t              asesq_pi;
    uint32_t              asesq_ci;
    uint64_t              asesq_base;
    uint32_t              snd_wnd;
    uint32_t              snd_cwnd;
    uint32_t              rcv_mss;
    uint16_t              source_port;
    uint16_t              dest_port;
    uint8_t               header_template[64];
    uint32_t              state;
    uint16_t              source_lif;
    uint32_t              debug_dol_tx;
    uint32_t              header_len;
    uint32_t              pending_ack_send;
    types::AppRedirType   l7_proxy_type;
    uint32_t              sesq_retx_ci;            // for testing, check in DOL
    uint32_t              retx_snd_una;            // for testing, check in DOL
    uint32_t              rto;
    uint32_t              snd_cwnd_cnt;
    uint32_t              serq_pi;
    uint32_t              pred_flags;
    // operational state of TCP Proxy CB
    hal_handle_t          hal_handle;              // HAL allocated handle

    // rx stats
    uint32_t              debug_stage0_7_thread;
    uint64_t              bytes_rcvd;
    uint64_t              pkts_rcvd;
    uint64_t              pages_alloced;
    uint64_t              desc_alloced;
    uint64_t              debug_num_phv_to_mem;
    uint64_t              debug_num_pkt_to_mem;

    uint64_t              debug_atomic_delta;
    uint64_t              debug_atomic0_incr1247;
    uint64_t              debug_atomic1_incr247;
    uint64_t              debug_atomic2_incr47;
    uint64_t              debug_atomic3_incr47;
    uint64_t              debug_atomic4_incr7;
    uint64_t              debug_atomic5_incr7;
    uint64_t              debug_atomic6_incr7;

    uint64_t              bytes_acked;
    uint64_t              slow_path_cnt;
    uint64_t              serq_full_cnt;
    uint64_t              ooo_cnt;

    uint8_t               debug_dol_tblsetaddr;

    // tx stats
    uint64_t              bytes_sent;
    uint64_t              pkts_sent;
    uint64_t              debug_num_phv_to_pkt;
    uint64_t              debug_num_mem_to_pkt;

    // PD state
    void                  *pd;                     // all PD specific state

    ht_ctxt_t             ht_ctxt;                 // id based hash table ctxt
    ht_ctxt_t             hal_handle_ht_ctxt;      // hal handle based hash table ctxt
    uint16_t              other_qid;
} __PACK__ tcpcb_t;

// max. number of TCP CBs supported  (TODO: we can take this from cfg file)
#define HAL_MAX_TCPCB                           2048

// allocate a tcpcbment instance
static inline tcpcb_t *
tcpcb_alloc (void)
{
    tcpcb_t    *tcpcb;

    tcpcb = (tcpcb_t *)g_hal_state->tcpcb_slab()->alloc();
    if (tcpcb == NULL) {
        return NULL;
    }
    return tcpcb;
}

// initialize a tcpcbment instance
static inline tcpcb_t *
tcpcb_init (tcpcb_t *tcpcb)
{
    if (!tcpcb) {
        return NULL;
    }
    HAL_SPINLOCK_INIT(&tcpcb->slock, PTHREAD_PROCESS_PRIVATE);

    // initialize the operational state
    tcpcb->pd = NULL;

    // initialize meta information
    tcpcb->ht_ctxt.reset();
    tcpcb->hal_handle_ht_ctxt.reset();

    return tcpcb;
}

// allocate and initialize a TCPCB instance
static inline tcpcb_t *
tcpcb_alloc_init (void)
{
    return tcpcb_init(tcpcb_alloc());
}

static inline hal_ret_t
tcpcb_free (tcpcb_t *tcpcb)
{
    HAL_SPINLOCK_DESTROY(&tcpcb->slock);
    hal::delay_delete_to_slab(HAL_SLAB_TCPCB, tcpcb);
    return HAL_RET_OK;
}

static inline tcpcb_t *
find_tcpcb_by_id (tcpcb_id_t tcpcb_id)
{
    return (tcpcb_t *)g_hal_state->tcpcb_id_ht()->lookup(&tcpcb_id);
}

extern void *tcpcb_get_key_func(void *entry);
extern uint32_t tcpcb_compute_hash_func(void *key, uint32_t ht_size);
extern bool tcpcb_compare_key_func(void *key1, void *key2);

extern void *tcpcb_get_handle_key_func(void *entry);
extern uint32_t tcpcb_compute_handle_hash_func(void *key, uint32_t ht_size);
extern bool tcpcb_compare_handle_key_func(void *key1, void *key2);

hal_ret_t tcpcb_create(tcpcb::TcpCbSpec& spec,
                       tcpcb::TcpCbResponse *rsp);

hal_ret_t tcpcb_update(tcpcb::TcpCbSpec& spec,
                       tcpcb::TcpCbResponse *rsp);

hal_ret_t tcpcb_delete(tcpcb::TcpCbDeleteRequest& req,
                       tcpcb::TcpCbDeleteResponseMsg *rsp);

hal_ret_t tcpcb_get(tcpcb::TcpCbGetRequest& req,
                    tcpcb::TcpCbGetResponseMsg *rsp);
}    // namespace hal

#endif    // __TCPCB_HPP__

