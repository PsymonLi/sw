#ifndef __HAL_RW_PD_HPP__
#define __HAL_RW_PD_HPP__

#include "nic/include/base.hpp"
#include "lib/ht/ht.hpp"
#include "nic/hal/pd/iris/hal_state_pd.hpp"

using sdk::lib::ht_ctxt_t;

namespace hal {
namespace pd {

#define HAL_MAX_RW_TBL_ENTRIES                        4 * 1024    // size of rw table in P4. 

typedef struct pd_rw_entry_info_s {
	bool 		with_id;
	uint32_t 	rw_idx;
} pd_rw_entry_info_t;

typedef struct pd_rw_entry_key_s {
    mac_addr_t          mac_sa;
    mac_addr_t          mac_da;
    rewrite_actions_en  rw_act;
} __PACK__ pd_rw_entry_key_t;

// rw table entry state
typedef struct pd_rw_entry_s {
    pd_rw_entry_key_t   rw_key;
    uint32_t            rw_idx;          
    uint32_t            ref_cnt;

    ht_ctxt_t           ht_ctxt; 
} __PACK__ pd_rw_entry_t;

// allocate a rw entry instance
static inline pd_rw_entry_t*
rw_entry_pd_alloc (void)
{
    pd_rw_entry_t       *rwe;

    rwe = (pd_rw_entry_t *)g_hal_state_pd->rw_entry_slab()->alloc();
    if (rwe == NULL) {
        return NULL;
    }

    return rwe;
}

// initialize a rwe instance
static inline pd_rw_entry_t *
rw_entry_pd_init (pd_rw_entry_t *rwe)
{
    if (!rwe) {
        return NULL;
    }
    memset(&rwe->rw_key, 0, sizeof(pd_rw_entry_key_t));
    rwe->rw_idx = 0;
    rwe->ref_cnt = 0;

    // initialize meta information
    rwe->ht_ctxt.reset();

    return rwe;
}

// allocate and initialize a rw entry instance
static inline pd_rw_entry_t *
rw_entry_pd_alloc_init (void)
{
    return rw_entry_pd_init(rw_entry_pd_alloc());
}

// free rw entry instance
static inline hal_ret_t
rw_entry_pd_free (pd_rw_entry_t *rwe)
{
    hal::pd::delay_delete_to_slab(HAL_SLAB_RW_PD, rwe);
    return HAL_RET_OK;
}

// insert rw entry state in all meta data structures
static inline hal_ret_t
add_rw_entry_pd_to_db (pd_rw_entry_t *rwe)
{
    g_hal_state_pd->rw_table_ht()->insert(rwe, &rwe->ht_ctxt);
    return HAL_RET_OK;
}

static inline hal_ret_t
del_rw_entry_pd_from_db(pd_rw_entry_t *rwe)
{
    g_hal_state_pd->rw_table_ht()->remove_entry(rwe, 
                                                &rwe->ht_ctxt);
    return HAL_RET_OK;
}

// find a ipseccb pd instance given its hw id
static inline pd_rw_entry_t *
find_rw_entry_by_key (pd_rw_entry_key_t *key) 
{
    return (pd_rw_entry_t *)g_hal_state_pd->rw_table_ht()->lookup(key);
}

extern void *rw_entry_pd_get_key_func(void *entry);
extern uint32_t rw_entry_pd_key_size(void);

hal_ret_t rw_pd_pgm_rw_tbl(pd_rw_entry_t *rwe);
hal_ret_t rw_pd_depgm_rw_tbl(pd_rw_entry_t *rwe);
hal_ret_t rw_entry_find(pd_rw_entry_key_t *rw_key, pd_rw_entry_t **rwe);
hal_ret_t rw_entry_alloc(pd_rw_entry_key_t *rw_key, 
                         pd_rw_entry_info_t *rw_info, 
                         uint32_t *rw_idx);
hal_ret_t rw_entry_find_or_alloc(pd_rw_entry_key_t *rw_key, 
                                 uint32_t *rw_idx);
hal_ret_t rw_entry_delete(pd_rw_entry_key_t *rw_key);

}   // namespace pd
}   // namespace hal

#endif    // __HAL_RW_PD_HPP__

