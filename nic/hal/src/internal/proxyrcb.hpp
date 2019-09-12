//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __PROXYRCB_HPP__
#define __PROXYRCB_HPP__

#include "nic/include/base.hpp"
#include "nic/sdk/include/sdk/encap.hpp"
#include "lib/list/list.hpp"
#include "lib/ht/ht.hpp"
#include "gen/proto/internal.pb.h"
#include "nic/include/pd.hpp"
#include "nic/include/app_redir_shared.h"

using sdk::lib::ht_ctxt_t;
using sdk::lib::dllist_ctxt_t;

using internal::ProxyrCbSpec;
using internal::ProxyrCbStatus;
using internal::ProxyrCbResponse;
using internal::ProxyrCbKeyHandle;
using internal::ProxyrCbRequestMsg;
using internal::ProxyrCbResponseMsg;
using internal::ProxyrCbDeleteRequestMsg;
using internal::ProxyrCbDeleteResponseMsg;
using internal::ProxyrCbGetRequest;
using internal::ProxyrCbGetRequestMsg;
using internal::ProxyrCbGetResponse;
using internal::ProxyrCbGetResponseMsg;

namespace hal {

typedef struct proxyrcb_s {
    sdk_spinlock_t        slock;                   // lock to protect this structure
    proxyrcb_id_t         cb_id;

    /*
     * Note that ordering of fields below does not matter;
     * data will get written to HBM according to P4+ table entry's
     * ordering defined in proxyr_rxdma_p4plus_ingress.h
     * see hal/pd/iris/proxyrcb_pd.cc)
     */
    uint16_t              pi;                       // if more PI/CI's are added,
    uint16_t              ci;                       // be sure to adjust HAL_PROXYRCB_NUM_RINGS_MAX below
    uint16_t              proxyrcb_flags;
    uint8_t               my_txq_ring_size_shift;
    uint8_t               my_txq_entry_size_shift;
    uint64_t              my_txq_base;

    uint64_t              chain_rxq_base;           // next service chain RxQ base
    uint64_t              chain_rxq_ring_indices_addr;
    uint8_t               chain_rxq_ring_size_shift;
    uint8_t               chain_rxq_entry_size_shift;
    uint8_t               chain_rxq_ring_index_select;

    uint8_t               redir_span;

    /*
     * Flow keys for generating pen_proxy_redir_header_v1_t
     */
    ipvx_addr_t           ip_sa;
    ipvx_addr_t           ip_da;
    uint16_t              sport;
    uint16_t              dport;
    vrf_id_t              vrf;
    uint8_t               af;
    uint8_t               ip_proto;

    /*
     * 64-bit statistic counters
     */
    uint64_t              stat_pkts_redir;
    uint64_t              stat_pkts_discard;

    /*
     * 32-bit saturating statistic counters
     */
    uint32_t              stat_cb_not_ready;
    uint32_t              stat_null_ring_indices_addr;
    uint32_t              stat_aol_err;
    uint32_t              stat_rxq_full;
    uint32_t              stat_txq_empty;
    uint32_t              stat_sem_alloc_full;
    uint32_t              stat_sem_free_full;

    hal_handle_t          hal_handle;               // HAL allocated handle

    // PD state
    void                  *pd;                      // all PD specific state

    ht_ctxt_t             ht_ctxt;                  // id based hash table ctxt
    ht_ctxt_t             hal_handle_ht_ctxt;       // hal handle based hash table ctxt
    proxyrcb_id_t         rev_cb_id;                // cb_id of reverse direction
    uint8_t               dir;
    uint8_t               role;
} proxyrcb_t;

#define HAL_MAX_PROXYRCB_HT_SIZE          1024      // hash table size

/*
 * Number of PI/CI pairs defined in proxyrcb_t above;
 * find a change the corresponding #define as needed.
 */
#define HAL_NUM_PROXYRCB_RINGS_MAX        APP_REDIR_PROXYR_RINGS_MAX

extern void *proxyrcb_get_key_func(void *entry);
extern uint32_t proxyrcb_key_size(void);
extern proxyrcb_t *find_proxyrcb_by_id(proxyrcb_id_t proxyrcb_id);
extern proxyrcb_t *find_proxyrcb_by_handle(hal_handle_t handle);

extern void *proxyrcb_get_handle_key_func(void *entry);
extern uint32_t proxyrcb_handle_key_size(void);

}    // namespace hal

#endif    // __PROXYRCB_HPP__

