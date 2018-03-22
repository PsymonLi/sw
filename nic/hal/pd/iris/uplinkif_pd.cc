#include "nic/include/hal_lock.hpp"
#include "nic/hal/pd/iris/hal_state_pd.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/include/interface_api.hpp"
#include "nic/hal/pd/iris/if_pd.hpp"
#include "nic/hal/pd/iris/uplinkif_pd.hpp"
#include "nic/hal/pd/capri/capri_tm_rw.hpp"
#include "nic/hal/pd/p4pd_api.hpp"

namespace hal {
namespace pd {

// ----------------------------------------------------------------------------
// Uplink If Create
// ----------------------------------------------------------------------------
hal_ret_t 
pd_uplinkif_create(pd_if_create_args_t *args)
{
    hal_ret_t            ret = HAL_RET_OK;; 
    pd_uplinkif_t        *pd_upif;

    HAL_TRACE_DEBUG("{}:creating pd state for if_id: {}", 
                    __FUNCTION__, if_get_if_id(args->intf));

    // Create lif PD
    pd_upif = uplinkif_pd_alloc_init();
    if (pd_upif == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }

    // Link PI & PD
    uplinkif_link_pi_pd(pd_upif, args->intf);

    // Allocate Resources
    ret = uplinkif_pd_alloc_res(pd_upif);
    if (ret != HAL_RET_OK) {
        // No Resources, dont allocate PD
        HAL_TRACE_ERR(":{}:unable to alloc. resources for if_id: {}",
                      __FUNCTION__, if_get_if_id(args->intf));
        goto end;
    }

    // Program HW
    ret = uplinkif_pd_program_hw(pd_upif);

end:
    if (ret != HAL_RET_OK) {
        uplinkif_pd_cleanup(pd_upif);
    }

    return ret;
}

//-----------------------------------------------------------------------------
// PD Uplinkif Update
//-----------------------------------------------------------------------------
hal_ret_t
pd_uplinkif_update (pd_if_update_args_t *args)
{
    // Nothing to do for now
    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// PD Uplinkif Delete
//-----------------------------------------------------------------------------
hal_ret_t
pd_uplinkif_delete (pd_if_delete_args_t *args)
{
    hal_ret_t      ret = HAL_RET_OK;
    pd_uplinkif_t  *uplinkif_pd;

    HAL_ASSERT_RETURN((args != NULL), HAL_RET_INVALID_ARG);
    HAL_ASSERT_RETURN((args->intf != NULL), HAL_RET_INVALID_ARG);
    HAL_ASSERT_RETURN((args->intf->pd_if != NULL), HAL_RET_INVALID_ARG);
    HAL_TRACE_DEBUG("{}:deleting pd state for uplinkif {}",
                    __FUNCTION__, args->intf->if_id);
    uplinkif_pd = (pd_uplinkif_t *)args->intf->pd_if;

    // deprogram HW
    ret = uplinkif_pd_deprogram_hw(uplinkif_pd);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:unable to deprogram hw", __FUNCTION__);
    }

    // pd cleanup
    ret = uplinkif_pd_cleanup(uplinkif_pd);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:failed pd uplinkif delete",
                      __FUNCTION__);
    }

    return ret;
}

//-----------------------------------------------------------------------------
// PD Uplinkif Get
//-----------------------------------------------------------------------------
hal_ret_t
pd_uplinkif_get (pd_if_get_args_t *args)
{
    hal_ret_t               ret = HAL_RET_OK;
    if_t                    *hal_if = args->hal_if;
    pd_uplinkif_t           *uplinkif_pd = (pd_uplinkif_t *)hal_if->pd_if;
    InterfaceGetResponse    *rsp = args->rsp;

    auto up_info = rsp->mutable_status()->mutable_uplink_info();
    up_info->set_uplink_lport_id(uplinkif_pd->upif_lport_id);
    up_info->set_hw_lif_id(uplinkif_pd->hw_lif_id);
    up_info->set_uplink_idx(uplinkif_pd->up_ifpc_id);

    return ret;
}

// ----------------------------------------------------------------------------
// Allocate resources for PD Uplink if
// ----------------------------------------------------------------------------
hal_ret_t 
uplinkif_pd_alloc_res(pd_uplinkif_t *pd_upif)
{
    hal_ret_t            ret = HAL_RET_OK;
    indexer::status      rs = indexer::SUCCESS;

    // Allocate lif hwid
#if 0
    rs = g_hal_state_pd->lif_hwid_idxr()->
        alloc((uint32_t *)&pd_upif->hw_lif_id);
    if (rs != indexer::SUCCESS) {
        HAL_TRACE_ERR("{}:failed to alloc hw_lif_id err: {}", 
                      __FUNCTION__, rs);
        pd_upif->hw_lif_id = INVALID_INDEXER_INDEX;
        return HAL_RET_NO_RESOURCE;
    }
#endif
    pd_upif->hw_lif_id = if_allocate_hwlif_id();
    if (pd_upif->hw_lif_id == INVALID_INDEXER_INDEX) {
        HAL_TRACE_ERR("{}:failed to alloc hw_lif_id err: {}", 
                      __FUNCTION__, rs);
        return HAL_RET_NO_RESOURCE;
    }
    HAL_TRACE_DEBUG("{}:if_id:{} allocated hw_lif_id: {}", 
                    __FUNCTION__, 
                    if_get_if_id((if_t *)pd_upif->pi_if),
                    pd_upif->hw_lif_id);

    // Allocate ifpc id
    rs = g_hal_state_pd->uplinkifpc_hwid_idxr()->
        alloc((uint32_t *)&pd_upif->up_ifpc_id);
    if (rs != indexer::SUCCESS) {
        HAL_TRACE_ERR("{}:failed to alloc uplink_ifpc_id err: {}", 
                      __FUNCTION__, rs);
        pd_upif->up_ifpc_id = INVALID_INDEXER_INDEX;
        return HAL_RET_NO_RESOURCE;
    }
    HAL_TRACE_DEBUG("{}:if_id:{} allocated uplink_ifpc_id: {}", 
                    __FUNCTION__, 
                    if_get_if_id((if_t *)pd_upif->pi_if),
                    pd_upif->up_ifpc_id);

    // Allocate lport
    rs = g_hal_state_pd->lport_idxr()->alloc((uint32_t *)&pd_upif->
            upif_lport_id);
    if (rs != indexer::SUCCESS) {
        HAL_TRACE_ERR("{}:failed to alloc uplink_ifpc_id err: {}", 
                      __FUNCTION__, rs);
        pd_upif->upif_lport_id = INVALID_INDEXER_INDEX;
        return HAL_RET_NO_RESOURCE;
    }
    HAL_TRACE_DEBUG("{}:if_id:{} allocated lport_id:{}", 
                    __FUNCTION__, 
                    if_get_if_id((if_t *)pd_upif->pi_if),
                    pd_upif->upif_lport_id);
    return ret;
}

//-----------------------------------------------------------------------------
// De-Allocate resources. 
//-----------------------------------------------------------------------------
hal_ret_t
uplinkif_pd_dealloc_res(pd_uplinkif_t *upif_pd)
{
    hal_ret_t           ret = HAL_RET_OK;
    indexer::status     rs;

    if (upif_pd->hw_lif_id != INVALID_INDEXER_INDEX) {
#if 0
        rs = g_hal_state_pd->lif_hwid_idxr()->free(upif_pd->hw_lif_id);
        if (rs != indexer::SUCCESS) {
            HAL_TRACE_ERR("{}:failed to free hw_lif_id err: {}", 
                          __FUNCTION__, upif_pd->hw_lif_id);
            ret = HAL_RET_INVALID_OP;
            goto end;
        }
#endif
        // TODO: Have to free up the index from lif manager

        HAL_TRACE_DEBUG("{}:freed hw_lif_id: {}", 
                        __FUNCTION__, upif_pd->hw_lif_id);
    }

    if (upif_pd->up_ifpc_id != INVALID_INDEXER_INDEX) {
        rs = g_hal_state_pd->uplinkifpc_hwid_idxr()->free(upif_pd->up_ifpc_id);
        if (rs != indexer::SUCCESS) {
            HAL_TRACE_ERR("{}:failed to free uplink_ifpc_id err: {}", 
                          __FUNCTION__, upif_pd->up_ifpc_id);
            ret = HAL_RET_INVALID_OP;
            goto end;
        }

        HAL_TRACE_DEBUG("{}:freed uplink_ifpc_id: {}", 
                        __FUNCTION__, upif_pd->up_ifpc_id);
    }

    if (upif_pd->upif_lport_id != INVALID_INDEXER_INDEX) {
        rs = g_hal_state_pd->lport_idxr()->free(upif_pd->upif_lport_id);
        if (rs != indexer::SUCCESS) {
            HAL_TRACE_ERR("{}:failed to free lport_id err: {}", 
                          __FUNCTION__, upif_pd->upif_lport_id);
            ret = HAL_RET_INVALID_OP;
            goto end;
        }

        HAL_TRACE_DEBUG("{}:freed lport_id: {}", 
                        __FUNCTION__, upif_pd->upif_lport_id);
    }

end:
    return ret;
}

//-----------------------------------------------------------------------------
// PD Uplinkif Cleanup
//  - Release all resources
//  - Delink PI <-> PD
//  - Free PD If 
//  Note:
//      - Just free up whatever PD has. 
//      - Dont use this inplace of delete. Delete may result in giving callbacks
//        to others.
//-----------------------------------------------------------------------------
hal_ret_t
uplinkif_pd_cleanup(pd_uplinkif_t *upif_pd)
{
    hal_ret_t       ret = HAL_RET_OK;

    if (!upif_pd) {
        // Nothing to do
        goto end;
    }

    // Releasing resources
    ret = uplinkif_pd_dealloc_res(upif_pd);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}: unable to dealloc res for uplinkif: {}", 
                      __FUNCTION__, 
                      ((if_t *)(upif_pd->pi_if))->if_id);
        goto end;
    }

    // Delinking PI<->PD
    uplinkif_delink_pi_pd(upif_pd, (if_t *)upif_pd->pi_if);

    // Freeing PD
    uplinkif_pd_free(upif_pd);
end:
    return ret;
}

// ----------------------------------------------------------------------------
// DeProgram HW
// ----------------------------------------------------------------------------
hal_ret_t
uplinkif_pd_deprogram_hw (pd_uplinkif_t *pd_upif)
{
    hal_ret_t            ret = HAL_RET_OK;

    // De program TM register
    ret = uplinkif_pd_depgm_tm_register(pd_upif);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:unable to deprogram hw", __FUNCTION__);
    }

    // De-Program Output Mapping Table
    ret = uplinkif_pd_depgm_output_mapping_tbl(pd_upif);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:unable to deprogram hw", __FUNCTION__);
    }
    return ret;
}

// ----------------------------------------------------------------------------
// De-Program TM Register
// ----------------------------------------------------------------------------
hal_ret_t
uplinkif_pd_depgm_tm_register(pd_uplinkif_t *pd_upif)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint8_t                     tm_oport = 0;

    tm_oport = uplinkif_get_port_num((if_t *)(pd_upif->pi_if)); 

    ret = capri_tm_uplink_lif_set(tm_oport, 0);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:unable to deprogram for if_id: {}",
                __FUNCTION__, if_get_if_id((if_t *)pd_upif->pi_if));
    } else {
        HAL_TRACE_DEBUG("{}:deprogrammed for if_id: {} "
                        "iport:{} => hwlif: {}",
                        __FUNCTION__, if_get_if_id((if_t *)pd_upif->pi_if),
                        tm_oport, 0);
    }

    return ret;
}

// ----------------------------------------------------------------------------
// DeProgram output mapping table for cpu tx traffic
// ----------------------------------------------------------------------------
hal_ret_t
uplinkif_pd_depgm_output_mapping_tbl (pd_uplinkif_t *pd_upif)
{
    hal_ret_t                   ret = HAL_RET_OK;
    sdk_ret_t                   sdk_ret;
    directmap                   *dm_omap = NULL;

    dm_omap = g_hal_state_pd->dm_table(P4TBL_ID_OUTPUT_MAPPING);
    HAL_ASSERT_RETURN((dm_omap != NULL), HAL_RET_ERR);
    
    sdk_ret = dm_omap->remove(pd_upif->upif_lport_id);
    ret = hal_sdk_ret_to_hal_ret(sdk_ret);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:unable to deprogram omapping table",
                __FUNCTION__, pd_upif->upif_lport_id);
    } else {
        HAL_TRACE_ERR("{}:deprogrammed omapping table",
                __FUNCTION__, pd_upif->upif_lport_id);
    }

    return ret;
}

// ----------------------------------------------------------------------------
// Program HW
// ----------------------------------------------------------------------------
hal_ret_t
uplinkif_pd_program_hw(pd_uplinkif_t *pd_upif)
{
    hal_ret_t            ret;

    // TODO: Program TM table port_num -> lif_hw_id
    ret = uplinkif_pd_pgm_tm_register(pd_upif);
    HAL_ASSERT_RETURN(ret == HAL_RET_OK, ret);

    // Program Output Mapping Table
    ret = uplinkif_pd_pgm_output_mapping_tbl(pd_upif);
    HAL_ASSERT_RETURN(ret == HAL_RET_OK, ret);

    return ret;
}

// ----------------------------------------------------------------------------
// Program TM Register
// ----------------------------------------------------------------------------
hal_ret_t
uplinkif_pd_pgm_tm_register(pd_uplinkif_t *pd_upif)
{
    hal_ret_t                   ret = HAL_RET_OK;
    uint8_t                     tm_oport = 0;

    tm_oport = uplinkif_get_port_num((if_t *)(pd_upif->pi_if)); 

    ret = capri_tm_uplink_lif_set(tm_oport, pd_upif->hw_lif_id);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:unable to program for if_id: {}",
                __FUNCTION__, if_get_if_id((if_t *)pd_upif->pi_if));
    } else {
        HAL_TRACE_DEBUG("{}:programmed for if_id: {} "
                        "iport:{} => hwlif: {}",
                        __FUNCTION__, if_get_if_id((if_t *)pd_upif->pi_if),
                        tm_oport, pd_upif->hw_lif_id);
    }

    return ret;
}

// ----------------------------------------------------------------------------
// Program Output Mapping Table
// ----------------------------------------------------------------------------
#define om_tmoport data.output_mapping_action_u.output_mapping_set_tm_oport
hal_ret_t
uplinkif_pd_pgm_output_mapping_tbl(pd_uplinkif_t *pd_upif)
{
    hal_ret_t                   ret = HAL_RET_OK;
    sdk_ret_t                   sdk_ret;
    uint8_t                     tm_oport = 0;
    output_mapping_actiondata   data;
    directmap                   *dm_omap = NULL;

    memset(&data, 0, sizeof(data));

    tm_oport = uplinkif_get_port_num((if_t *)(pd_upif->pi_if)); 

    data.actionid = OUTPUT_MAPPING_SET_TM_OPORT_ID;
    om_tmoport.nports = 1;
    om_tmoport.egress_mirror_en = 1;
    om_tmoport.egress_port1 = tm_oport;
    om_tmoport.dst_lif = pd_upif->hw_lif_id;

    // Program OutputMapping table
    //  - Get tmoport from PI
    //  - Get vlan_tagid_in_skb from the fwding mode:
    //      - Classic: TRUE
    //      - Switch : FALSE

    dm_omap = g_hal_state_pd->dm_table(P4TBL_ID_OUTPUT_MAPPING);
    HAL_ASSERT_RETURN((dm_omap != NULL), HAL_RET_ERR);

    sdk_ret = dm_omap->insert_withid(&data, pd_upif->upif_lport_id);
    ret = hal_sdk_ret_to_hal_ret(sdk_ret);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("{}:unable to program for if_id: {}",
                __FUNCTION__, if_get_if_id((if_t *)pd_upif->pi_if));
    } else {
        HAL_TRACE_DEBUG("{}:programmed for if_id: {} at {}",
                __FUNCTION__, if_get_if_id((if_t *)pd_upif->pi_if),
                pd_upif->upif_lport_id);
    }
    return ret;
}

// ----------------------------------------------------------------------------
// Makes a clone
// ----------------------------------------------------------------------------
hal_ret_t
pd_uplinkif_make_clone(pd_if_make_clone_args_t *args)
{
    hal_ret_t           ret = HAL_RET_OK;
    pd_uplinkif_t       *pd_uplinkif_clone = NULL;
    if_t *hal_if = args->hal_if;
    if_t *clone = args->clone;

    pd_uplinkif_clone = uplinkif_pd_alloc_init();
    if (pd_uplinkif_clone == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }

    memcpy(pd_uplinkif_clone, hal_if->pd_if, sizeof(pd_uplinkif_t));

    uplinkif_link_pi_pd(pd_uplinkif_clone, clone);

end:
    return ret;
}

// ----------------------------------------------------------------------------
// Frees PD memory without indexer free.
// ----------------------------------------------------------------------------
hal_ret_t
pd_uplinkif_mem_free(pd_if_mem_free_args_t *args)
{
    hal_ret_t      ret = HAL_RET_OK;
    pd_uplinkif_t  *upif_pd;

    upif_pd = (pd_uplinkif_t *)args->intf->pd_if;
    uplinkif_pd_mem_free(upif_pd);

    return ret;
}


}    // namespace pd
}    // namespace hal
