// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#include "nic/sdk/include/sdk/lock.hpp"
#include "nic/hal/pd/pd_api.hpp"
#include "nic/hal/plugins/cfg/nw/interface_api.hpp"
#include "nic/hal/plugins/cfg/nw/l2segment_api.hpp"
#include "gen/proto/interface.pb.h"
#include "nic/hal/pd/gft/if_pd.hpp"
#include "nic/hal/pd/gft/lif_pd.hpp"
#include "nic/hal/pd/gft/enicif_pd.hpp"
#include "nic/p4/gft/include/defines.h"
#include "nic/hal/pd/gft/pd_utils.hpp"
#include "lib/table/tcam/tcam.hpp"

namespace hal {
namespace pd {

pd_lif_t *pd_enicif_get_pd_lif(pd_enicif_t *pd_enicif);

//-----------------------------------------------------------------------------
// allocate resources for PD EnicIf
//-----------------------------------------------------------------------------
static hal_ret_t 
pd_enicif_alloc_res (pd_enicif_t *pd_enicif)
{
    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// free enic interface resources
//-----------------------------------------------------------------------------
static hal_ret_t
pd_enicif_dealloc_res (pd_enicif_t *pd_enicif)
{
    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// PD enic if cleanup 
//  - Release all resources
//  - Delink PI <-> PD
//  - Free PD If 
//  Note:
//      - Just free up whatever PD has. 
//      - Dont use this inplace of delete. Delete may result in giving callbacks
//        to others.
//-----------------------------------------------------------------------------
static hal_ret_t
pd_enicif_cleanup (pd_enicif_t *pd_enicif)
{
    hal_ret_t       ret = HAL_RET_OK;

    if (!pd_enicif) {
        // Nothing to do
        goto end;
    }

    // Releasing resources
    ret = pd_enicif_dealloc_res(pd_enicif);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("unable to dealloc res for enicif: {}", 
                      ((if_t *)(pd_enicif->pi_if))->if_id);
        goto end;
    }

    // Delinking PI<->PD
    pd_enicif_delink_pi_pd(pd_enicif, (if_t *)pd_enicif->pi_if);

    // Freeing PD
    pd_enicif_free(pd_enicif);

end:

    return ret;
}

static hal_ret_t
pd_enicif_pgm_tx_vport(pd_enicif_t *pd_enicif, table_oper_t oper)
{
    hal_ret_t               ret = HAL_RET_OK;
    sdk_ret_t               sdk_ret;
    tx_vport_swkey_t        key;
    tx_vport_swkey_mask_t   mask;
    tx_vport_actiondata_t     data;
    mac_addr_t              *mac = NULL;
    tcam                    *tx_vport_tbl = NULL;

    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));

    tx_vport_tbl = g_hal_state_pd->tcam_table(P4TBL_ID_TX_VPORT);
    SDK_ASSERT_RETURN((tx_vport_tbl != NULL), HAL_RET_ERR);

    // key
    mac = if_get_mac_addr((if_t*)pd_enicif->pi_if);
    memcpy(key.flow_action_metadata_tx_ethernet_dst, *mac, 
           ETHER_ADDR_LEN);
    memrev(key.flow_action_metadata_tx_ethernet_dst, ETHER_ADDR_LEN);

    // mask
    memset(mask.flow_action_metadata_tx_ethernet_dst_mask, 0xFF, 6);


    // data
    data.action_u.tx_vport_tx_vport.port = TM_PORT_INGRESS;

    if (oper == TABLE_OPER_INSERT) {
        sdk_ret = tx_vport_tbl->insert(&key, &mask, &data,
                                       &pd_enicif->tx_vport_idx);
        ret = hal_sdk_ret_to_hal_ret(sdk_ret);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("unable to program tx_vport tbl. ret: {}",
                          ret);
            goto end;
        } else {
            HAL_TRACE_DEBUG("programmed tx_vport tbl at: {}",
                            pd_enicif->tx_vport_idx);
        }
    } else {
        sdk_ret = tx_vport_tbl->update(pd_enicif->tx_vport_idx, 
                                       &data);
        ret = hal_sdk_ret_to_hal_ret(sdk_ret);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("unable to program tx_vport tbl. ret: {}",
                          ret);
            goto end;
        } else {
            HAL_TRACE_DEBUG("programmed tx_vport tbl at: {}",
                            pd_enicif->tx_vport_idx);
        }
    }

end:

    return ret;
}



static hal_ret_t
pd_enicif_pgm_rx_vport (pd_enicif_t *pd_enicif, table_oper_t oper)
{
    hal_ret_t               ret = HAL_RET_OK;
    sdk_ret_t               sdk_ret;
    rx_vport_swkey_t        key;
    rx_vport_swkey_mask_t   mask;
    rx_vport_actiondata_t     data;
    mac_addr_t              *mac = NULL;
    tcam                    *rx_vport_tbl = NULL;
    pd_lif_t                *pd_lif             = NULL;
    uint32_t                hw_lif_id = 0;

    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));

    rx_vport_tbl = g_hal_state_pd->tcam_table(P4TBL_ID_RX_VPORT);
    SDK_ASSERT_RETURN((rx_vport_tbl != NULL), HAL_RET_ERR);

    pd_lif = pd_enicif_get_pd_lif(pd_enicif);

    // key
    key.ethernet_1_valid = 0xFF;
    mac = if_get_mac_addr((if_t*)pd_enicif->pi_if);
    memcpy(key.ethernet_1_dstAddr, *mac, ETHER_ADDR_LEN);
    memrev(key.ethernet_1_dstAddr, ETHER_ADDR_LEN);

    // mask
    mask.ethernet_1_valid_mask = 0xFF;
    memset(mask.ethernet_1_dstAddr_mask, 0xFF, 6);

    // data
    ret = if_get_hw_lif_id((if_t*)pd_enicif->pi_if, &hw_lif_id);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("unable to get hw_lif_id ret: {}", ret);
        goto end;
    }
    data.action_u.rx_vport_rx_vport.vport = hw_lif_id;
    data.action_u.rx_vport_rx_vport.tm_oport = TM_PORT_DMA;
    data.action_u.rx_vport_rx_vport.rdma_enabled =
         pd_lif ? lif_get_enable_rdma((lif_t *)pd_lif->pi_lif) : false;

    if (oper == TABLE_OPER_INSERT) {
        sdk_ret = rx_vport_tbl->insert(&key, &mask, &data,
                                       &pd_enicif->rx_vport_idx);
        ret = hal_sdk_ret_to_hal_ret(sdk_ret);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("unable to program rx_vport tbl. ret: {}",
                          ret);
            goto end;
        } else {
            HAL_TRACE_DEBUG("programmed rx_vport tbl at: {}",
                            pd_enicif->rx_vport_idx);
        }
    } else {
        sdk_ret = rx_vport_tbl->update(pd_enicif->rx_vport_idx, 
                                       &data);
        ret = hal_sdk_ret_to_hal_ret(sdk_ret);
        if (ret != HAL_RET_OK) {
            HAL_TRACE_ERR("unable to program rx_vport tbl. ret: {}",
                          ret);
            goto end;
        } else {
            HAL_TRACE_DEBUG("programmed rx_vport tbl at: {}",
                            pd_enicif->rx_vport_idx);
        }
    }

end:

    return ret;
}

//-----------------------------------------------------------------------------
// program h/w
//-----------------------------------------------------------------------------
static hal_ret_t
pd_enicif_program_hw (pd_enicif_t *pd_enicif)
{
    hal_ret_t   ret     = HAL_RET_OK;
#if 0
    if_t        *hal_if = (if_t *)pd_enicif->pi_if;

    if (hal_if->enic_type != intf::IF_ENIC_TYPE_GFT) {
        HAL_TRACE_ERR("invalid enicif type {} in GFT", hal_if->enic_type);
        goto end;
    }
#endif

    ret = pd_enicif_pgm_rx_vport(pd_enicif, TABLE_OPER_INSERT);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("failed to program rx_vport ret: {}", ret);
        goto end;
    }

    // TODO: Temporary change for latency measurements
    ret = pd_enicif_pgm_tx_vport(pd_enicif, TABLE_OPER_INSERT);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("failed to program rx_vport ret: {}", ret);
        goto end;
    }

end:

    return ret;
}

//-----------------------------------------------------------------------------
// deprogram h/w
//-----------------------------------------------------------------------------
static hal_ret_t
pd_enicif_deprogram_hw (pd_enicif_t *pd_enicif)
{
    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// enic if crate
//-----------------------------------------------------------------------------
hal_ret_t 
pd_enicif_create (pd_if_create_args_t *args)
{
    hal_ret_t            ret = HAL_RET_OK;; 
    pd_enicif_t          *pd_enicif;

    HAL_TRACE_DEBUG("creating pd state for enicif: {}", 
                    if_get_if_id(args->intf));

    // Create Enic If PD
    pd_enicif = pd_enicif_alloc_init();
    if (pd_enicif == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }

    // Link PI & PD
    pd_enicif_link_pi_pd(pd_enicif, args->intf);

    // Allocate Resources
    ret = pd_enicif_alloc_res(pd_enicif);
    if (ret != HAL_RET_OK) {
        // No Resources, dont allocate PD
        HAL_TRACE_ERR("unable to alloc. resources for enicif: {}",
                      if_get_if_id(args->intf));
        goto end;
    }

    // Program HW
    ret = pd_enicif_program_hw(pd_enicif);

end:

    if (ret != HAL_RET_OK) {
        pd_enicif_cleanup(pd_enicif);
    }

    return ret;
}

//-----------------------------------------------------------------------------
// PD EnicIf Update
//-----------------------------------------------------------------------------
hal_ret_t
pd_enicif_update (pd_if_update_args_t *args)
{
    return HAL_RET_OK;
}

//-----------------------------------------------------------------------------
// PD EnicIf Delete
//-----------------------------------------------------------------------------
hal_ret_t
pd_enicif_delete (pd_if_delete_args_t *args)
{
    hal_ret_t      ret = HAL_RET_OK;
    pd_enicif_t    *enicif_pd;

    SDK_ASSERT_RETURN((args != NULL), HAL_RET_INVALID_ARG);
    SDK_ASSERT_RETURN((args->intf != NULL), HAL_RET_INVALID_ARG);
    SDK_ASSERT_RETURN((args->intf->pd_if != NULL), HAL_RET_INVALID_ARG);
    HAL_TRACE_DEBUG("deleting pd state for enicif: {}",
                    args->intf->if_id);
    enicif_pd = (pd_enicif_t *)args->intf->pd_if;

    // deprogram HW
    ret = pd_enicif_deprogram_hw(enicif_pd);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("unable to deprogram hw. ret: {}", ret);
    }

    ret = pd_enicif_cleanup(enicif_pd);
    if (ret != HAL_RET_OK) {
        HAL_TRACE_ERR("failed pd enicif delete. ret: {}", ret);
    }

    return ret;
}

//-----------------------------------------------------------------------------
// makes a clone
//-----------------------------------------------------------------------------
hal_ret_t
pd_enicif_make_clone (pd_if_make_clone_args_t *args)
{
    hal_ret_t           ret = HAL_RET_OK;
    pd_enicif_t       *pd_enicif_clone = NULL;
    if_t *hal_if = args->hal_if;
    if_t *clone = args->clone;

    pd_enicif_clone = pd_enicif_alloc_init();
    if (pd_enicif_clone == NULL) {
        ret = HAL_RET_OOM;
        goto end;
    }

    memcpy(pd_enicif_clone, hal_if->pd_if, sizeof(pd_enicif_t));
    pd_enicif_link_pi_pd(pd_enicif_clone, clone);

end:

    return ret;
}

//-----------------------------------------------------------------------------
// frees PD memory without indexer free.
//-----------------------------------------------------------------------------
hal_ret_t
pd_enicif_mem_free (pd_if_mem_free_args_t *args)
{
    hal_ret_t      ret = HAL_RET_OK;
    pd_enicif_t  *upif_pd;

    upif_pd = (pd_enicif_t *)args->intf->pd_if;
    pd_enicif_free(upif_pd);

    return ret;
}

pd_lif_t *
pd_enicif_get_pd_lif(pd_enicif_t *pd_enicif)
{
    if_t        *pi_if = NULL;
    pd_lif_t    *pd_lif = NULL;
    lif_t       *pi_lif = NULL;

    pi_if = (if_t *)pd_enicif->pi_if;
    SDK_ASSERT_RETURN(pi_if != NULL, 0);

    pi_lif = if_get_lif(pi_if);
    SDK_ASSERT(pi_lif != NULL);

    pd_lif = (pd_lif_t *)lif_get_pd_lif(pi_lif);
    SDK_ASSERT(pi_lif != NULL);

    return pd_lif;
}
}    // namespace pd
}    // namespace hal
