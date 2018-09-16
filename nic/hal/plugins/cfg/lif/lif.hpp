//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __LIF_HPP__
#define __LIF_HPP__

#include "nic/include/base.hpp"
#include "nic/include/eth.hpp"
#include "nic/include/ip.hpp"
#include "sdk/list.hpp"
#include "sdk/ht.hpp"
#include "nic/include/bitmap.hpp"
#include "nic/hal/plugins/cfg/nw/vrf.hpp"
#include "nic/gen/proto/hal/interface.pb.h"
#include "nic/hal/plugins/cfg/aclqos/qos.hpp"
#include "nic/hal/plugins/cfg/lif/lif_manager.hpp"

#define NUM_MAX_COSES 16
// Size of RSS seed in bytes
#define ETH_RSS_KEY_LENGTH          40
// Number of RSS indirection entries
#define ETH_RSS_INDIR_LENGTH        128

using intf::LifSpec;
using intf::LifResponse;
using intf::LifDeleteRequest;
using intf::LifDeleteResponse;
using intf::LifGetRequest;
using intf::LifGetResponse;
using intf::LifGetResponseMsg;

using sdk::lib::ht_ctxt_t;
using sdk::lib::dllist_ctxt_t;
using hal::utils::bitmap;

namespace hal {

// typedef struct if_s if_t;

// PKtFilter structure
typedef struct pkt_filter_s {
    bool    receive_broadcast;     // Receive Broadcast
    bool    receive_all_multicast; // Receive all Multicast
    bool    receive_promiscuous;   // Receive Unknown Unicast, Broadcast, Multicast. Not Known Unicast
} __PACK__ pkt_filter_t;

typedef struct lif_cos_info_s {
    uint16_t            cos_bmp;         // bitmap of COS values for Tx traffic supported on this LIF.
    uint8_t             coses;           // cos values supported by LIF. cosA - Bits 0-4, cosB - Bits 4-7.
    hal_handle_t        rx_qos_class_handle; // qos-class-hdl for Rx traffic on this LIF.
    hal_handle_t        tx_qos_class_handle; // qos-class-hdl for Tx traffic on this LIF.
    policer_t           rx_policer;      // Rx policer
    policer_t           tx_policer;      // Tx policer
} __PACK__ lif_qos_info_t;

// Lif RSS config structure
typedef struct lif_rss_info_s {
    uint32_t    type;
    uint8_t     key[ETH_RSS_KEY_LENGTH];
    uint8_t     indir[ETH_RSS_INDIR_LENGTH];
} __PACK__ lif_rss_info_t;

// Lif queue info structure
typedef struct lif_queue_info_s {
    uint8_t     type;        // hardware queue type of queue
    uint16_t    size;        // size of qstate
    uint16_t    num_queues;  // number of queues
} __PACK__ lif_queue_info_t;

// LIF structure
typedef struct lif_s {
    hal_spinlock_t      slock;           // lock to protect this structure
    lif_id_t            lif_id;          // lif id assigned
    intf::IfStatus      admin_status;    // admin status
    bool                vlan_strip_en;   // vlan strip enable
    bool                vlan_insert_en;  // if en, ingress vlan is in p4plus_to_p4 dr
    bool                is_management;   // set for MNICs and management NICs
    hal_handle_t        pinned_uplink;   // uplink this LIF is pinned to
    bool                enable_rdma;     // enable rdma on this LIF
    uint32_t            rdma_max_keys;
    uint32_t            rdma_max_ahs;
    uint32_t            rdma_max_pt_entries;
    lif_queue_info_t    qinfo[intf::LifQPurpose_MAX+1]; // purpose to qtype mapping
    lif_qos_info_t      qos_info;
    bool                qstate_init_done;// qstate map init status.
    pkt_filter_t        packet_filters;  // Packet Filter Modes
    lif_rss_info_t      rss;             // rss configuration

    // operational state of interface
    hal_handle_t        hal_handle;      // HAL allocated handle

    // back references to enic ifs
    dllist_ctxt_t       if_list_head;    // interfaces (enics) behind this lif
    block_list          *filter_list;    // filter list

    void                *pd_lif;
} __PACK__ lif_t;

typedef struct lif_hal_info_s {
    bool        with_hw_lif_id;
    uint32_t    hw_lif_id;
    bool        dont_zero_qstate_mem;
} lif_hal_info_t;

typedef struct lif_create_app_ctxt_s {
    lif_hal_info_t      *lif_info;
    LifSpec             *spec;
    LifResponse         *rsp;
    uint32_t            hw_lif_id;
} __PACK__ lif_create_app_ctxt_t;

typedef struct lif_update_app_ctxt_s {
    LifSpec      *spec;
    LifResponse  *rsp;
    bool         vlan_strip_en;
    bool         qstate_map_init_set;
    bool         vlan_insert_en_changed:1;
    uint64_t     pkt_filter_prom_changed:1;
    uint64_t     pkt_filter_bcast_changed:1;
    uint64_t     pkt_filter_allmc_changed:1;
    uint64_t     receive_promiscous:1;
    uint64_t     receive_broadcast:1;
    uint64_t     receive_all_multicast:1;
    uint64_t     vlan_strip_en_changed:1;
    uint64_t     pinned_uplink_changed:1;
    uint64_t     rx_policer_changed:1;
    uint64_t     tx_policer_changed:1;
    uint64_t     vlan_insert_en;
    hal_handle_t new_pinned_uplink;
    bool         rss_config_changed;
} __PACK__ lif_update_app_ctxt_t;

#define HAL_MAX_LIFS                                 1024

static inline void
lif_lock (lif_t *lif, const char *fname,
          int lineno, const char *fxname)
{
    HAL_TRACE_DEBUG("{}:operlock:locking lif:{} from {}:{}:{}",
                    __FUNCTION__, lif->lif_id,
                    fname, lineno, fxname);
    HAL_SPINLOCK_LOCK(&lif->slock);
}

static inline void
lif_unlock (lif_t *lif, const char *fname,
            int lineno, const char *fxname)
{
    HAL_TRACE_DEBUG("{}:operlock:unlocking lif:{} from {}:{}:{}",
                    __FUNCTION__, lif->lif_id,
                    fname, lineno, fxname);
    HAL_SPINLOCK_UNLOCK(&lif->slock);
}

extern LIFManager *lif_manager();

extern void *lif_id_get_key_func(void *entry);
extern uint32_t lif_id_compute_hash_func(void *key, uint32_t ht_size);
extern bool lif_id_compare_key_func(void *key1, void *key2);
extern lif_t *find_lif_by_id(lif_id_t lif_id);
extern lif_t *find_lif_by_handle(hal_handle_t handle);
void lif_print_ifs(lif_t *lif);
void lif_print(lif_t *lif);
hal_ret_t lif_update_trigger_if(lif_t *lif,
                                bool vlan_strip_en_changed,
                                bool vlan_strip_en,
                                bool vlan_insert_en_changed,
                                bool vlan_insert_en,
                                bool pinned_uplink_changed,
                                hal_handle_t pinned_uplink);
void LifGetQState(const intf::QStateGetReq &req, intf::QStateGetResp *resp);
void LifSetQState(const intf::QStateSetReq &req, intf::QStateSetResp *resp);
void lif_spec_dump (LifSpec& spec);
const char *lif_spec_keyhandle_to_str(const kh::LifKeyHandle& key_handle);
const char *lif_keyhandle_to_str(lif_t *lif);
lif_t *lif_lookup_key_or_handle(const kh::LifKeyHandle& kh);

// SVC APIs
hal_ret_t lif_create(LifSpec& spec, LifResponse *rsp,
                     lif_hal_info_t *lif_hal_info);
hal_ret_t lif_update(LifSpec& spec, LifResponse *rsp);
hal_ret_t lif_delete(LifDeleteRequest& req,
                     LifDeleteResponse *rsp);
hal_ret_t lif_get(LifGetRequest& req, LifGetResponseMsg *rsp);

}    // namespace hal

#endif    // __LIF_HPP__

