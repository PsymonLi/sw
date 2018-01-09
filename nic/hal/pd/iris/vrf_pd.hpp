#ifndef __HAL_PD_VRF_HPP__
#define __HAL_PD_VRF_HPP__

#include "nic/include/base.h"
#include "sdk/ht.hpp"
#include "nic/include/pd.hpp"
#include "nic/hal/pd/iris/hal_state_pd.hpp"
#include "nic/hal/pd/iris/l2seg_pd.hpp"

using sdk::lib::ht_ctxt_t;

namespace hal {
namespace pd {

#define HAL_MAX_HW_VRFS         256
#define HAL_PD_L2SEG_MASK       0xFFF
#define HAL_PD_VRF_MASK         0xF000
#define HAL_PD_VRF_SHIFT        12

typedef uint32_t    vrf_hw_id_t;

// vrf pd state
struct pd_vrf_s {
    void               *vrf;              // PI vrf

    // operational state of vrf pd
    uint32_t           gipo_imn_idx[3];      // Input mapping native table idx
    uint32_t           gipo_imt_idx[3];      // Input mapping tunneled table idx
    vrf_hw_id_t        vrf_hw_id;            // hw id for this VRF
    uint32_t           vrf_fl_lkup_id;        // Used by IPSec for flow lookup
    uint32_t           vrf_fromcpu_vlan_id;  // From CPU vlan id
    indexer            *l2seg_hw_id_idxr_;   // indexer for l2segs in this ten

    // Entry used by traffic From CPU traffic
    uint32_t            inp_prop_tbl_cpu_idx;
} __PACK__;

// allocate a vrf pd instance
static inline pd_vrf_t *
vrf_pd_alloc (void)
{
    pd_vrf_t    *vrf_pd;

    vrf_pd = (pd_vrf_t *)g_hal_state_pd->vrf_slab()->alloc();
    if (vrf_pd == NULL) {
        return NULL;
    }

    return vrf_pd;
}

// initialize a vrf pd instance
static inline pd_vrf_t *
vrf_pd_init (pd_vrf_t *vrf_pd)
{
    if (!vrf_pd) {
        return NULL;
    }
    vrf_pd->vrf = NULL;
    vrf_pd->vrf_hw_id = INVALID_INDEXER_INDEX;

    vrf_pd->l2seg_hw_id_idxr_ = 
        sdk::lib::indexer::factory(HAL_MAX_HW_L2SEGMENTS, true, true);
    HAL_ASSERT_RETURN((vrf_pd->l2seg_hw_id_idxr_ != NULL), NULL);

    // Prevention of usage of 0
    vrf_pd->l2seg_hw_id_idxr_->alloc_withid(0);

    for (int i = 0; i < 3; i++) {
        vrf_pd->gipo_imn_idx[i] = INVALID_INDEXER_INDEX;
        vrf_pd->gipo_imt_idx[i] = INVALID_INDEXER_INDEX;
    }

    return vrf_pd;
}

// allocate and initialize a vrf pd instance
static inline pd_vrf_t *
vrf_pd_alloc_init (void)
{
    return vrf_pd_init(vrf_pd_alloc());
}

// freeing vrf pd
static inline hal_ret_t
vrf_pd_free (pd_vrf_t *ten)
{
    ten->l2seg_hw_id_idxr_ ? indexer::destroy(ten->l2seg_hw_id_idxr_) : HAL_NOP;
    hal::pd::delay_delete_to_slab(HAL_SLAB_VRF_PD, ten);
    return HAL_RET_OK;
}

// freeing vrf pd memory
static inline hal_ret_t
vrf_pd_mem_free (pd_vrf_t *ten)
{
    hal::pd::delay_delete_to_slab(HAL_SLAB_VRF_PD, ten);
    return HAL_RET_OK;
}

static inline indexer *
vrf_pd_l2seg_hw_id_indexer(pd_vrf_t *vrf_pd) 
{
    return vrf_pd->l2seg_hw_id_idxr_;

}

hal_ret_t vrf_pd_alloc_res(pd_vrf_t *pd_ten);
hal_ret_t vrf_pd_dealloc_res(pd_vrf_t *vrf_pd);
hal_ret_t vrf_pd_cleanup(pd_vrf_t *vrf_pd);
void link_pi_pd(pd_vrf_t *pd_ten, vrf_t *pi_ten);
void delink_pi_pd(pd_vrf_t *pd_ten, vrf_t *pi_ten);

hal_ret_t vrf_pd_alloc_l2seg_hw_id(pd_vrf_t *vrf_pd, 
                                      uint32_t *l2seg_hw_id);
hal_ret_t vrf_pd_free_l2seg_hw_id(pd_vrf_t *vrf_pd, 
                                     uint32_t l2seg_hw_id);
hal_ret_t vrf_pd_pgm_inp_prop_tbl (pd_vrf_t *vrf_pd);
hal_ret_t vrf_pd_program_hw (pd_vrf_t *vrf_pd);
hal_ret_t vrf_pd_pgm_inp_prop_tbl (pd_vrf_t *vrf_pd);
hal_ret_t vrf_pd_deprogram_hw (pd_vrf_t *vrf_pd);
hal_ret_t vrf_pd_depgm_inp_prop_tbl (pd_vrf_t *vrf_pd);

hal_ret_t vrf_pd_alloc_cpuid(pd_vrf_t *pd_vrf);
hal_ret_t vrf_pd_dealloc_cpuid(pd_vrf_t *vrf_pd);
hal_ret_t vrf_pd_add_to_db (pd_vrf_t *pd_vrf, 
                            hal_handle_t handle);
hal_ret_t vrf_pd_del_from_db (pd_vrf_t *pd_vrf);

}   // namespace pd
}   // namespace hal

#endif    // __HAL_PD_VRF_HPP__
