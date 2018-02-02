#include "nic/include/hal_lock.hpp"
#include "nic/hal/pd/iris/hal_state_pd.hpp"
#include "nic/hal/pd/iris/copp_pd.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/include/qos_api.hpp"

namespace hal {
namespace pd {

// ----------------------------------------------------------------------------
// Copp Create
// ----------------------------------------------------------------------------
EXTC hal_ret_t
pd_copp_create(pd_copp_create_args_t *args)
{
    hal_ret_t      ret = HAL_RET_OK;;
    pd_copp_t *pd_copp;

    HAL_TRACE_DEBUG("pd-copp::{}: creating pd state for copp: {}",
                    __func__, args->copp->key);

    // Create copp PD
    pd_copp = copp_pd_alloc_init();
    if (pd_copp == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }

    // Link PI & PD
    copp_link_pi_pd(pd_copp, args->copp);

    // Allocate Resources
    ret = copp_pd_alloc_res(pd_copp);
    if (ret != HAL_RET_OK) {
        // No Resources, dont allocate PD
        HAL_TRACE_ERR("pd-copp::{}: Unable to alloc. resources for Copp: {} ret {}",
                      __func__, args->copp->key, ret);
        goto end;
    }

    // Program HW
    ret = copp_pd_program_hw(pd_copp);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("pd-copp::{}: Unable to program hw for Copp: {} ret {}",
                      __func__, args->copp->key, ret);
        goto end;
    }

end:
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("pd-copp::{}: Error in programming hw for Copp: {}: ret: {}",
                      __func__, args->copp->key, ret);
        // unlink_pi_pd(pd_copp, args->copp);
        // copp_pd_free(pd_copp);
        copp_pd_cleanup(pd_copp);
    }

    return ret;
}

#if 0
// ----------------------------------------------------------------------------
// Copp Update
// ----------------------------------------------------------------------------
hal_ret_t
pd_copp_update (pd_copp_update_args_t *pd_copp_upd_args)
{
    hal_ret_t           ret = HAL_RET_OK;

    HAL_TRACE_DEBUG("pd-copp::{}: updating pd state for copp:{}",
                    __func__,
                    pd_copp_upd_args->copp->key);

    return ret;
}
#endif

//-----------------------------------------------------------------------------
// PD Copp Delete
//-----------------------------------------------------------------------------
hal_ret_t
pd_copp_delete (pd_copp_delete_args_t *args)
{
    hal_ret_t ret = HAL_RET_OK;
    pd_copp_t *pd_copp;

    HAL_ASSERT_RETURN((args != NULL), HAL_RET_INVALID_ARG);
    HAL_ASSERT_RETURN((args->copp != NULL), HAL_RET_INVALID_ARG);
    HAL_ASSERT_RETURN((args->copp->pd != NULL), HAL_RET_INVALID_ARG);
    HAL_TRACE_DEBUG("pd-copp:{}:deleting pd state for copp {}",
                    __func__, args->copp->key);
    pd_copp = (pd_copp_t *)args->copp->pd;

    // free up the resource and memory
    ret = copp_pd_cleanup(pd_copp);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("pd-copp:{}:failed pd copp cleanup Copp {}, ret {}",
                      __func__, args->copp->key, ret);
    }

    return ret;
}

//-----------------------------------------------------------------------------
// PD Copp Cleanup
//  - Release all resources
//  - Delink PI <-> PD
//  - Free PD Copp
//  Note:
//      - Just free up whatever PD has.
//      - Dont use this inplace of delete. Delete may result in giving callbacks
//        to others.
//-----------------------------------------------------------------------------
hal_ret_t
copp_pd_cleanup(pd_copp_t *pd_copp)
{
    hal_ret_t       ret = HAL_RET_OK;

    if (!pd_copp) {
        // Nothing to do
        goto end;
    }

    if (pd_copp->hw_policer_id != INVALID_INDEXER_INDEX) {
        // TODO: deprogram hw

    }

    // Releasing resources
    ret = copp_pd_dealloc_res(pd_copp);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("pd-copp:{}: unable to dealloc res for copp: {}",
                      __func__,
                      ((copp_t *)(pd_copp->pi_copp))->key);
        goto end;
    }

    // Delinking PI<->PD
    copp_delink_pi_pd(pd_copp, (copp_t *)pd_copp->pi_copp);

    // Freeing PD
    copp_pd_free(pd_copp);
end:
    return ret;
}


// ----------------------------------------------------------------------------
// Allocate resources for PD Copp
// ----------------------------------------------------------------------------
hal_ret_t
copp_pd_alloc_res (pd_copp_t *pd_copp)
{
    hal_ret_t               ret = HAL_RET_OK;

    // Allocate any hardware resources
    return ret;
}

// ----------------------------------------------------------------------------
// De-Allocate resources for PD Copp
// ----------------------------------------------------------------------------
hal_ret_t
copp_pd_dealloc_res(pd_copp_t *pd_copp)
{
    hal_ret_t            ret = HAL_RET_OK;

    // Deallocate any hardware resources 
    return ret;
}

#define COPP_ACTION(_arg) d.copp_action_u.copp_execute_copp._arg
static hal_ret_t
copp_pd_program_copp_tbl (pd_copp_t *pd_copp, bool update)
{
    hal_ret_t       ret = HAL_RET_OK;
    sdk_ret_t       sdk_ret;
    copp_t          *pi_copp = (copp_t *)pd_copp->pi_copp;
    directmap       *copp_tbl = NULL;
    copp_actiondata d = {0};

    copp_tbl = g_hal_state_pd->dm_table(P4TBL_ID_COPP);
    HAL_ASSERT_RETURN((copp_tbl != NULL), HAL_RET_ERR);

    d.actionid = COPP_EXECUTE_COPP_ID;
    if (pi_copp->policer.bps_rate == 0) {
        COPP_ACTION(entry_valid) = 0;
    } else {
        COPP_ACTION(entry_valid) = 1;
        COPP_ACTION(pkt_rate) = 0;
        //TODO does this need memrev ?
        memcpy(COPP_ACTION(burst), &pi_copp->policer.burst_size, sizeof(uint32_t));
        // TODO convert the rate into token-rate 
        memcpy(COPP_ACTION(rate), &pi_copp->policer.bps_rate, sizeof(uint32_t));
    }

    // TODO Fixme. Setting entry-valid to 0 until copp is verified and values
    // are determined
    COPP_ACTION(entry_valid) = 0;
    if (update) {
        sdk_ret = copp_tbl->update(pd_copp->hw_policer_id, &d);
    } else {
        sdk_ret = copp_tbl->insert(&d, &pd_copp->hw_policer_id);
    }
    ret = hal_sdk_ret_to_hal_ret(sdk_ret);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("pd-copp:{}: copp policer table write failure, copp {}, ret {}",
                      __FUNCTION__, pi_copp->key, ret);
        return ret;
    }
    HAL_TRACE_DEBUG("pd-copp:{}: copp {} hw_policer_id {} rate {} burst {} programmed",
                    __FUNCTION__, pi_copp->key,
                    pd_copp->hw_policer_id, pi_copp->policer.bps_rate,
                    pi_copp->policer.burst_size);
    return ret;
}
#undef COPP_ACTION

// ----------------------------------------------------------------------------
// Program HW
// ----------------------------------------------------------------------------
hal_ret_t
copp_pd_program_hw(pd_copp_t *pd_copp)
{
    hal_ret_t ret = HAL_RET_OK;
    copp_t    *copp = (copp_t *)pd_copp->pi_copp;

    ret = copp_pd_program_copp_tbl(pd_copp, false);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("pd-copp::{}: Error programming the copp table "
                      "Copp {} ret {}",
                      __func__, copp->key, ret);
        return ret;
    }

    return ret;
}

// ----------------------------------------------------------------------------
// Linking PI <-> PD
// ----------------------------------------------------------------------------
void
copp_link_pi_pd(pd_copp_t *pd_copp, copp_t *pi_copp)
{
    pd_copp->pi_copp = pi_copp;
    pi_copp->pd = pd_copp;
}

// ----------------------------------------------------------------------------
// De-Linking PI <-> PD
// ----------------------------------------------------------------------------
void
copp_delink_pi_pd(pd_copp_t *pd_copp, copp_t *pi_copp)
{
    if (pd_copp) {
        pd_copp->pi_copp = NULL;
    }
    if (pi_copp) {
        pi_copp->pd = NULL;
    }
}

// ----------------------------------------------------------------------------
// Makes a clone
// ----------------------------------------------------------------------------
hal_ret_t
pd_copp_make_clone(copp_t *copp, copp_t *clone)
{
    hal_ret_t ret = HAL_RET_OK;
    pd_copp_t *pd_copp_clone = NULL;

    pd_copp_clone = copp_pd_alloc_init();
    if (pd_copp_clone == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }

    memcpy(pd_copp_clone, copp->pd, sizeof(pd_copp_t));

    copp_link_pi_pd(pd_copp_clone, clone);

end:
    return ret;
}

#if 0
// ----------------------------------------------------------------------------
// Frees PD memory without indexer free.
// ----------------------------------------------------------------------------
hal_ret_t
pd_copp_mem_free(pd_copp_args_t *args)
{
    hal_ret_t      ret = HAL_RET_OK;
    pd_copp_t        *pd_copp;

    pd_copp = (pd_copp_t *)args->copp->pd;
    copp_pd_mem_free(pd_copp);

    return ret;
}
#endif

}    // namespace pd
}    // namespace hal
