//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// mfg mode related API
///
//----------------------------------------------------------------------------


#include "nic/apollo/api/impl/lif_impl.hpp"
#include "nic/apollo/api/include/athena/pds_init.h"
#include "nic/sdk/lib/catalog/catalog.hpp"
#include "gen/p4gen/athena/include/p4pd.h"
#include "nic/apollo/api/impl/athena/pds_impl_state.hpp"
#include "nic/apollo/api/impl/athena/athena_impl.hpp"

using api::impl::lif_impl;
using api::impl::lif_impl_db;

typedef struct lif_datapath_mnic_ctx_s {
    lif_impl **lif_0;
    lif_impl **lif_1;
    lif_type_t type;
} __PACK__ lif_datapath_mnic_ctx_t;

static bool
lif_datapath_mnic_cb_(void *api_obj, void *ctxt) {
    lif_impl *lif = (lif_impl *)api_obj;
    lif_datapath_mnic_ctx_t *cb_ctx = (lif_datapath_mnic_ctx_t *)ctxt;

    PDS_TRACE_DEBUG("lif cb called for lif type %u id %u ifidx %u", lif->type(),
                    lif->id(), lif->pinned_ifindex());
    if (lif->type() == cb_ctx->type) {
        if (*cb_ctx->lif_0 == NULL) {
            *cb_ctx->lif_0 = lif;
        } else {
            *cb_ctx->lif_1 = lif;
            return true;
        }
    }
    return false;
}

sdk_ret_t
athena_program_mfg_mode_nacls(pds_mfg_mode_params_t *params) {
    
    sdk_ret_t ret = SDK_RET_OK;
    lif_datapath_mnic_ctx_t cb_ctx = {0};
    lif_impl *dp_mnic0 = NULL, *dp_mnic1 = NULL;
    pds_lif_id_t mnic0_id, mnic1_id;
    if_index_t pinned_if_idx_mnic0, pinned_if_idx_mnic1;
    nacl_swkey_t           key = { 0 };
    nacl_swkey_mask_t      mask = { 0 };
    nacl_actiondata_t      data =  { 0 };
    sdk_table_api_params_t tparams;
    std::array<uint16_t, PDS_MFG_TEST_NUM_VLANS> vlans;
    uint32_t hw_lif_a, hw_lif_b, hw_oport_a, hw_oport_b;

    if(params == NULL) {
        PDS_TRACE_ERR("params arg is null");
        return SDK_RET_INVALID_ARG;
    }
    
    for(uint16_t idx = 0; idx < PDS_MFG_TEST_NUM_VLANS; ++idx) {
        vlans[idx] = params->vlans[idx];
    }

    cb_ctx.type = sdk::platform::LIF_TYPE_MNIC_CPU;
    cb_ctx.lif_0 = &dp_mnic0;
    cb_ctx.lif_1 = &dp_mnic1;

    if(lif_impl_db() == NULL) {
        PDS_TRACE_ERR("lif_impl_db not initialized");
        return SDK_RET_ENTRY_NOT_FOUND; 
    }

    lif_impl_db()->walk(lif_datapath_mnic_cb_, &cb_ctx);
   
    if(dp_mnic0 == NULL || dp_mnic1 == NULL) {
        PDS_TRACE_ERR("Failed to fetch dp mnic info: dp_mnic0 0x%p, dp_mnic1 0x%p", 
                        dp_mnic0, dp_mnic1);
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    mnic0_id = dp_mnic0->id();
    pinned_if_idx_mnic0 = dp_mnic0->pinned_ifindex();
    mnic1_id = dp_mnic1->id();
    pinned_if_idx_mnic1 = dp_mnic1->pinned_ifindex();

    PDS_TRACE_DEBUG("data path mnic lif info: dp_mnic0 lif %u uplink 0x%x, "
                    "dp_mnic1 lif %u uplink 0x%x", mnic0_id, 
                    pinned_if_idx_mnic0, mnic1_id, 
                    pinned_if_idx_mnic1);
    
    //NACLs to redirect traffic with specific vlans to ARM
    hw_lif_a = sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_mnic0);
    hw_lif_b = sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_mnic1);
   
    key.capri_intrinsic_lif = hw_lif_a;
    mask.capri_intrinsic_lif_mask = 0xFFFF;
    
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.action_u.nacl_nacl_redirect.redir_type = PACKET_ACTION_REDIR_RXDMA;
    data.action_u.nacl_nacl_redirect.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.action_u.nacl_nacl_redirect.oport = TM_PORT_DMA;
    data.action_u.nacl_nacl_redirect.lif = mnic0_id;
    
    for (int idx = 0; idx != vlans.size(); ++idx) {      
        key.vlan = vlans[idx];
        mask.vlan_mask = 0xFFFF;

        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                       NACL_NACL_REDIRECT_ID,
                                       sdk::table::handle_t::null());
        ret = api::impl::athena_impl_db()->nacl_tbl()->insert(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program vlan redirect NACL for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_a, 
                    mnic0_id, vlans[idx], ret);
            return ret;

        } else {
            PDS_TRACE_DEBUG("Vlan redirect nacl programmed for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_a, 
                    mnic0_id, vlans[idx], ret);
        }
    }

    key.capri_intrinsic_lif = hw_lif_b;
    mask.capri_intrinsic_lif_mask = 0xFFFF;
    
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.action_u.nacl_nacl_redirect.redir_type = PACKET_ACTION_REDIR_RXDMA;
    data.action_u.nacl_nacl_redirect.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.action_u.nacl_nacl_redirect.oport = TM_PORT_DMA;
    data.action_u.nacl_nacl_redirect.lif = mnic1_id;
    
    for (int idx = 0; idx != vlans.size(); ++idx) {      
        key.vlan = vlans[idx];
        mask.vlan_mask = 0xFFFF;

        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                       NACL_NACL_REDIRECT_ID,
                                       sdk::table::handle_t::null());
        ret = api::impl::athena_impl_db()->nacl_tbl()->insert(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program vlan redirect NACL for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_b, 
                    mnic1_id, vlans[idx], ret);
            return ret;

        } else {
            PDS_TRACE_DEBUG("Vlan redirect nacl programmed for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_b, 
                    mnic1_id, vlans[idx], ret);
        }
    }

    //NACLs to redirect traffic between uplinks
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));

    hw_oport_a = 
        api::g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_mnic0);
    hw_oport_b = 
        api::g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_mnic1);
    
    key.capri_intrinsic_lif = hw_lif_a;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.action_u.nacl_nacl_redirect.redir_type = PACKET_ACTION_REDIR_UPLINK;
    data.action_u.nacl_nacl_redirect.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.action_u.nacl_nacl_redirect.oport = hw_oport_b;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = api::impl::athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program redirect nacl from uplink lif %u "
                      "to uplink lif %u, err 0x%x\n", hw_lif_a, hw_lif_b, ret);
        return ret;
    }
    else {
        PDS_TRACE_DEBUG("Nacl programmed to redirect traffic from uplink "
                "lif %u to uplink lif %u\n", hw_lif_a, hw_lif_b);
    }

    key.capri_intrinsic_lif = hw_lif_b;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.action_u.nacl_nacl_redirect.redir_type = PACKET_ACTION_REDIR_UPLINK;
    data.action_u.nacl_nacl_redirect.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.action_u.nacl_nacl_redirect.oport = hw_oport_a;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = api::impl::athena_impl_db()->nacl_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program redirect nacl from uplink lif %u "
                      "to uplink lif %u, err 0x%x\n", hw_lif_b, hw_lif_a, ret);
        return ret;
    }
    else {
        PDS_TRACE_DEBUG("Nacl programmed to redirect traffic from uplink "
                "lif %u to uplink lif %u\n", hw_lif_b, hw_lif_a);
    }
    
    return SDK_RET_OK;
}

sdk_ret_t
athena_unprogram_mfg_mode_nacls(pds_mfg_mode_params_t *params) {

    sdk_ret_t ret = SDK_RET_OK;
    lif_datapath_mnic_ctx_t cb_ctx = {0};
    lif_impl *dp_mnic0 = NULL, *dp_mnic1 = NULL;
    pds_lif_id_t mnic0_id, mnic1_id;
    if_index_t pinned_if_idx_mnic0, pinned_if_idx_mnic1;
    nacl_swkey_t           key = { 0 };
    nacl_swkey_mask_t      mask = { 0 };
    nacl_actiondata_t      data =  { 0 };
    sdk_table_api_params_t tparams;
    std::array<uint16_t, PDS_MFG_TEST_NUM_VLANS> vlans;
    uint32_t hw_lif_a, hw_lif_b, hw_oport_a, hw_oport_b;

    if(params == NULL) {
        PDS_TRACE_ERR("params arg is null");
        return SDK_RET_INVALID_ARG;
    }
    
    for(uint16_t idx = 0; idx < PDS_MFG_TEST_NUM_VLANS; ++idx) {
        vlans[idx] = params->vlans[idx];
    }

    cb_ctx.type = sdk::platform::LIF_TYPE_MNIC_CPU;
    cb_ctx.lif_0 = &dp_mnic0;
    cb_ctx.lif_1 = &dp_mnic1;

    if(lif_impl_db() == NULL) {
        PDS_TRACE_ERR("lif_impl_db not initialized");
        return SDK_RET_ENTRY_NOT_FOUND; 
    }

    lif_impl_db()->walk(lif_datapath_mnic_cb_, &cb_ctx);
   
    if(dp_mnic0 == NULL || dp_mnic1 == NULL) {
        PDS_TRACE_ERR("Failed to fetch dp mnic info: dp_mnic0 0x%p, dp_mnic1 0x%p", 
                        dp_mnic0, dp_mnic1);
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    mnic0_id = dp_mnic0->id();
    pinned_if_idx_mnic0 = dp_mnic0->pinned_ifindex();
    mnic1_id = dp_mnic1->id();
    pinned_if_idx_mnic1 = dp_mnic1->pinned_ifindex();

    PDS_TRACE_DEBUG("data path mnic lif info: dp_mnic0 lif %u uplink 0x%x, "
                    "dp_mnic1 lif %u uplink 0x%x", mnic0_id, 
                    pinned_if_idx_mnic0, mnic1_id, 
                    pinned_if_idx_mnic1);
    
    //Remove NACLs that redirect traffic with specific vlans to ARM
    hw_lif_a = sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_mnic0);
    hw_lif_b = sdk::lib::catalog::ifindex_to_logical_port(pinned_if_idx_mnic1);
   
    key.capri_intrinsic_lif = hw_lif_a;
    mask.capri_intrinsic_lif_mask = 0xFFFF;
    
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.action_u.nacl_nacl_redirect.redir_type = PACKET_ACTION_REDIR_RXDMA;
    data.action_u.nacl_nacl_redirect.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.action_u.nacl_nacl_redirect.oport = TM_PORT_DMA;
    data.action_u.nacl_nacl_redirect.lif = mnic0_id;
    
    for (int idx = 0; idx != vlans.size(); ++idx) {      
        key.vlan = vlans[idx];
        mask.vlan_mask = 0xFFFF;

        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                       NACL_NACL_REDIRECT_ID,
                                       sdk::table::handle_t::null());
        ret = api::impl::athena_impl_db()->nacl_tbl()->remove(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to remove vlan redirect NACL for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_a, 
                    mnic0_id, vlans[idx], ret);
            return ret;

        } else {
            PDS_TRACE_DEBUG("Vlan redirect nacl removed for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_a, 
                    mnic0_id, vlans[idx], ret);
        }
    }

    key.capri_intrinsic_lif = hw_lif_b;
    mask.capri_intrinsic_lif_mask = 0xFFFF;
    
    data.action_id = NACL_NACL_REDIRECT_ID;
    data.action_u.nacl_nacl_redirect.redir_type = PACKET_ACTION_REDIR_RXDMA;
    data.action_u.nacl_nacl_redirect.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.action_u.nacl_nacl_redirect.oport = TM_PORT_DMA;
    data.action_u.nacl_nacl_redirect.lif = mnic1_id;
    
    for (int idx = 0; idx != vlans.size(); ++idx) {      
        key.vlan = vlans[idx];
        mask.vlan_mask = 0xFFFF;

        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                       NACL_NACL_REDIRECT_ID,
                                       sdk::table::handle_t::null());
        ret = api::impl::athena_impl_db()->nacl_tbl()->remove(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to remove vlan redirect NACL for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_b, 
                    mnic1_id, vlans[idx], ret);
            return ret;

        } else {
            PDS_TRACE_DEBUG("Vlan redirect nacl removed for uplink "
                    "lif %u to mnic lif %u vlan %u, err 0x%x\n", hw_lif_b, 
                    mnic1_id, vlans[idx], ret);
        }
    }

    //Remove NACLs that redirect traffic between uplinks
    memset(&key, 0, sizeof(key));
    memset(&mask, 0, sizeof(mask));
    memset(&data, 0, sizeof(data));

    hw_oport_a = 
        api::g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_mnic0);
    hw_oport_b = 
        api::g_pds_state.catalogue()->ifindex_to_tm_port(pinned_if_idx_mnic1);
    
    key.capri_intrinsic_lif = hw_lif_a;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.action_u.nacl_nacl_redirect.redir_type = PACKET_ACTION_REDIR_UPLINK;
    data.action_u.nacl_nacl_redirect.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.action_u.nacl_nacl_redirect.oport = hw_oport_b;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = api::impl::athena_impl_db()->nacl_tbl()->remove(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to remove redirect nacl from uplink lif %u "
                      "to uplink lif %u, err 0x%x\n", hw_lif_a, hw_lif_b, ret);
        return ret;
    }
    else {
        PDS_TRACE_DEBUG("Nacl to redirect traffic from uplink "
                "lif %u to uplink lif %u removed\n", hw_lif_a, hw_lif_b);
    }

    key.capri_intrinsic_lif = hw_lif_b;
    mask.capri_intrinsic_lif_mask = 0xFFFF;

    data.action_id = NACL_NACL_REDIRECT_ID;
    data.action_u.nacl_nacl_redirect.redir_type = PACKET_ACTION_REDIR_UPLINK;
    data.action_u.nacl_nacl_redirect.app_id = P4PLUS_APPTYPE_CLASSIC_NIC;
    data.action_u.nacl_nacl_redirect.oport = hw_oport_a;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &key, &mask, &data,
                                   NACL_NACL_REDIRECT_ID,
                                   sdk::table::handle_t::null());
    ret = api::impl::athena_impl_db()->nacl_tbl()->remove(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to remove redirect nacl from uplink lif %u "
                      "to uplink lif %u, err 0x%x\n", hw_lif_b, hw_lif_a, ret);
        return ret;
    }
    else {
        PDS_TRACE_DEBUG("Nacl to redirect traffic from uplink "
                "lif %u to uplink lif %u removed\n", hw_lif_b, hw_lif_a);
    }
    
    return SDK_RET_OK;
}
