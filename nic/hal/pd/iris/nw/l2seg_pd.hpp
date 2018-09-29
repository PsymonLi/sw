//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef __HAL_PD_L2SEG_HPP__
#define __HAL_PD_L2SEG_HPP__

#include "nic/sdk/include/sdk/ht.hpp"
#include "nic/include/pd.hpp"
#include "nic/include/base.hpp"
#include "nic/hal/pd/pd_api.hpp"
#include "nic/hal/pd/iris/hal_state_pd.hpp"

using sdk::lib::ht_ctxt_t;

namespace hal {
namespace pd {

#define HAL_MAX_HW_L2SEGMENTS                        2048

// l2seg pd state
typedef struct pd_l2seg_s {
    l2seg_hw_id_t   l2seg_hw_id;         // hw id for this segment
    uint32_t        l2seg_fl_lkup_id;    // used in data plane as vrf
    uint32_t        cpu_l2seg_id;        // traffic from CPU
    // [Uplink ifpc_id] -> Input Properties(Hash Index).
    // If L2Seg is native on an uplink, it will have two entries.
    // (Vlan_v: 1, Vlan: 0; Vlan_v: 0, Vlan: 0);
    uint32_t        inp_prop_tbl_idx[HAL_MAX_UPLINK_IF_PCS];
    uint32_t        inp_prop_tbl_idx_pri[HAL_MAX_UPLINK_IF_PCS];
    uint32_t        inp_prop_tbl_cpu_idx;   // traffic from CPU

    // Valid only for classic mode
    uint32_t        num_prom_lifs;       // Prom lifs in l2seg.
    hal_handle_t    prom_if_handle;      // Enic if handle for prom_lif.
                                         // Valid only if num_prom_lifs is 1.
    uint32_t        prom_if_dest_lport;  // Prom IF's dlport

    void            *l2seg;              // PI L2 segment
} __PACK__ pd_l2seg_t;

// allocate a l2seg pd instance
static inline pd_l2seg_t *
l2seg_pd_alloc (void)
{
    pd_l2seg_t    *l2seg_pd;

    l2seg_pd = (pd_l2seg_t *)g_hal_state_pd->l2seg_slab()->alloc();
    if (l2seg_pd == NULL) {
        return NULL;
    }

    return l2seg_pd;
}

// initialize a l2seg pd instance
static inline pd_l2seg_t *
l2seg_pd_init (pd_l2seg_t *l2seg_pd)
{
    if (!l2seg_pd) {
        return NULL;
    }
    l2seg_pd->l2seg                = NULL;
    l2seg_pd->l2seg_hw_id          = INVALID_INDEXER_INDEX;
    l2seg_pd->l2seg_fl_lkup_id     = INVALID_INDEXER_INDEX;
    l2seg_pd->cpu_l2seg_id         = INVALID_INDEXER_INDEX;
    l2seg_pd->inp_prop_tbl_cpu_idx = INVALID_INDEXER_INDEX;
    l2seg_pd->num_prom_lifs        = 0;
    l2seg_pd->prom_if_handle       = HAL_HANDLE_INVALID;
    l2seg_pd->prom_if_dest_lport   = INVALID_INDEXER_INDEX;

    for (int i = 0; i < HAL_MAX_UPLINK_IF_PCS; i++) {
        l2seg_pd->inp_prop_tbl_idx[i]     = INVALID_INDEXER_INDEX;
        l2seg_pd->inp_prop_tbl_idx_pri[i] = INVALID_INDEXER_INDEX;
    }

    return l2seg_pd;
}

// allocate and initialize a l2seg pd instance
static inline pd_l2seg_t *
l2seg_pd_alloc_init (void)
{
    return l2seg_pd_init(l2seg_pd_alloc());
}

// free l2seg pd instance
static inline hal_ret_t
l2seg_pd_free (pd_l2seg_t *l2seg_pd)
{
    hal::pd::delay_delete_to_slab(HAL_SLAB_L2SEG_PD, l2seg_pd);
    return HAL_RET_OK;
}

// free l2seg pd instance. Just freeing as it will be used during
// update to just memory free.
static inline hal_ret_t
l2seg_pd_mem_free (pd_l2seg_t *l2seg_pd)
{
    hal::pd::delay_delete_to_slab(HAL_SLAB_L2SEG_PD, l2seg_pd);
    return HAL_RET_OK;
}

extern void *flow_lkupid_get_hw_key_func(void *entry);
extern uint32_t flow_lkupid_compute_hw_hash_func(void *key, uint32_t ht_size);
extern bool flow_lkupid_compare_hw_key_func(void *key1, void *key2);
hal_ret_t pd_l2seg_update_prom_lifs(pd_l2seg_t *pd_l2seg,
                                    if_t *prom_enic_if,
                                    bool inc, bool skip_hw_pgm);
hal_ret_t l2seg_pd_pgm_mbr_ifs (block_list *if_list, l2seg_t *l2seg,
                                bool is_upgrade);
hal_ret_t l2seg_pd_depgm_mbr_ifs (block_list *if_list, l2seg_t *l2seg);
hal_ret_t l2seg_uplink_depgm_input_properties_tbl (l2seg_t *l2seg,
                                                   if_t *hal_if);
hal_ret_t l2seg_uplink_pgm_input_properties_tbl(l2seg_t *l2seg,
                                                if_t *hal_if,
                                                bool is_upgrade = false);
}   // namespace pd
}   // namespace hal

#endif    // __HAL_PD_L2SEG_HPP__

