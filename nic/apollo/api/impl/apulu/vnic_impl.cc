//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of VNIC
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/vpc.hpp"
#include "nic/apollo/api/subnet.hpp"
#include "nic/apollo/api/vnic.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/api/internal/ipc.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/impl/apulu/route_impl.hpp"
#include "nic/apollo/api/impl/apulu/security_policy_impl.hpp"
#include "nic/apollo/api/impl/apulu/subnet_impl.hpp"
#include "nic/apollo/api/impl/apulu/policer_impl.hpp"
#include "nic/apollo/api/impl/apulu/vnic_impl.hpp"
#include "nic/apollo/api/impl/apulu/vpc_impl.hpp"
#include "nic/apollo/api/impl/apulu/mirror_impl.hpp"
#include "nic/apollo/api/impl/apulu/svc/vnic_svc.hpp"
#include "nic/apollo/api/impl/apulu/svc/svc_utils.hpp"
#include "nic/apollo/p4/include/apulu_table_sizes.h"
#include "nic/apollo/p4/include/apulu_defines.h"
#include "gen/p4gen/p4/include/ftl.h"

#define vnic_tx_stats_action          action_u.vnic_tx_stats_vnic_tx_stats
#define vnic_rx_stats_action          action_u.vnic_rx_stats_vnic_rx_stats
#define meter_stats_action            action_u.meter_stats_meter_stats
#define rxdma_vnic_info               action_u.vnic_info_rxdma_vnic_info_rxdma
#define ing_vnic_info                 action_u.vnic_vnic_info
#define egr_vnic_info                 action_u.rx_vnic_rx_vnic_info
#define nexthop_info                  action_u.nexthop_nexthop_info

// compute next vnic epoch given current epoch
#define PDS_IMPL_VNIC_EPOCH_NEXT(epoch)    ((++(epoch)) & 0xFF)

#define PDS_IMPL_FILL_LIF_VLAN_KEY(key_, encap_)                   \
{                                                                  \
    memset((key_), 0, sizeof(*(key_)));                            \
    if ((encap_)->type == PDS_ENCAP_TYPE_DOT1Q) {                  \
        (key_)->key_metadata_lif = 0;                              \
        (key_)->ctag_1_vid = (encap_)->val.vlan_tag;               \
    } else if ((encap_)->type == PDS_ENCAP_TYPE_QINQ) {            \
        (key_)->key_metadata_lif = (encap_)->val.qinq.s_tag;       \
        (key_)->ctag_1_vid = (encap_)->val.qinq.c_tag;             \
    }                                                              \
}

namespace api {
namespace impl {

/// \defgroup PDS_VNIC_IMPL - VNIC entry datapath implementation
/// \ingroup PDS_VNIC
/// @{

vnic_impl *
vnic_impl::factory(pds_vnic_spec_t *spec) {
    vnic_impl *impl;

    impl = vnic_impl_db()->alloc();
    new (impl) vnic_impl(spec);
    return impl;
}

void
vnic_impl::destroy(vnic_impl *impl) {
    impl->~vnic_impl();
    vnic_impl_db()->free(impl);
}

impl_base *
vnic_impl::clone(void) {
    vnic_impl *cloned_impl;

    cloned_impl = vnic_impl_db()->alloc();
    new (cloned_impl) vnic_impl();
    // deep copy is not needed as we don't store pointers
    *cloned_impl = *this;
    return cloned_impl;
}

sdk_ret_t
vnic_impl::free(vnic_impl *impl) {
    destroy(impl);
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::reserve_resources(api_base *api_obj, api_base *orig_obj,
                             api_obj_ctxt_t *obj_ctxt) {
    uint32_t idx;
    sdk_ret_t ret;
    subnet_entry *subnet;
    mapping_swkey_t mapping_key;
    lif_vlan_swkey_t lif_vlan_key;
    sdk_table_api_params_t tparams;
    device_entry *device = device_find();
    local_mapping_swkey_t local_mapping_key;
    pds_vnic_spec_t *spec = &obj_ctxt->api_params->vnic_spec;

    switch (obj_ctxt->api_op) {
    case API_OP_CREATE:
        // record the fact that resource reservation was attempted
        // NOTE: even if we partially acquire resources and fail eventually,
        //       this will ensure that proper release of resources will happen
        api_obj->set_rsvd_rsc();

        // if this object is restored from persistent storage
        // resources are reserved already
        if (!api_obj->in_restore_list()) {
            // allocate hw id for this vnic
            if ((ret = vnic_impl_db()->vnic_idxr()->alloc(&idx)) !=
                    SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to allocate hw id for vnic %s, err %u",
                              spec->key.str(), ret);
                return ret;
            }
            hw_id_ = idx;

            if ((g_pds_state.device_oper_mode() == PDS_DEV_OPER_MODE_HOST) ||
                (g_pds_state.device_oper_mode() ==
                     PDS_DEV_OPER_MODE_BITW_SMART_SWITCH)) {
                // reserve an entry in the NEXTHOP table for this local vnic
                ret = nexthop_impl_db()->nh_idxr()->alloc(&idx);
                if (ret != SDK_RET_OK) {
                    PDS_TRACE_ERR("Failed to allocate nexthop entry for "
                                  "vnic %s, err %u", spec->key.str(), ret);
                    return ret;
                }
                nh_idx_ = idx;
            }
        }

        // reserve an entry in LIF_VLAN table
        if (spec->vnic_encap.type != PDS_ENCAP_TYPE_NONE) {
            PDS_IMPL_FILL_LIF_VLAN_KEY(&lif_vlan_key, &spec->vnic_encap);
            PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &lif_vlan_key, NULL, NULL,
                                           LIF_VLAN_LIF_VLAN_INFO_ID,
                                           handle_t::null());
            ret = apulu_impl_db()->lif_vlan_tbl()->reserve(&tparams);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to reserve entry in LIF_VLAN for "
                              "vnic %s with encap %s, err %u", spec->key.str(),
                              pds_encap2str(&spec->vnic_encap), ret);
                return ret;
            }
            lif_vlan_hdl_ = tparams.handle;
        }

        if ((g_pds_state.device_oper_mode() == PDS_DEV_OPER_MODE_HOST) ||
            (g_pds_state.device_oper_mode() ==
                 PDS_DEV_OPER_MODE_BITW_SMART_SWITCH)) {
            subnet = subnet_find(&spec->subnet);
            if (unlikely(subnet == NULL)) {
                PDS_TRACE_ERR("Unable to find subnet %s for vnic %s",
                              spec->subnet.str(), spec->key.str());
                return sdk::SDK_RET_INVALID_ARG;
            }
            // reserve an entry in LOCAL_MAPPING table for MAC entry
            memset(&local_mapping_key, 0, sizeof(local_mapping_key));
            local_mapping_key.key_metadata_local_mapping_lkp_type =
                KEY_TYPE_MAC;
            local_mapping_key.key_metadata_local_mapping_lkp_id =
                ((subnet_impl *)subnet->impl())->hw_id();
            sdk::lib::memrev(
                          local_mapping_key.key_metadata_local_mapping_lkp_addr,
                          spec->mac_addr, ETH_ADDR_LEN);
            PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &local_mapping_key,
                                           NULL, NULL, 0,
                                           sdk::table::handle_t::null());
            ret = mapping_impl_db()->local_mapping_tbl()->reserve(&tparams);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to reserve entry in LOCAL_MAPPING "
                              "table for vnic %s, err %u",
                              spec->key.str(), ret);
                goto error;
            }
            local_mapping_hdl_ = tparams.handle;

            // reserve an entry in MAPPING table for MAC entry
            memset(&mapping_key, 0, sizeof(mapping_key));
            mapping_key.p4e_i2e_mapping_lkp_id =
                ((subnet_impl *)subnet->impl())->hw_id();
            mapping_key.p4e_i2e_mapping_lkp_type = KEY_TYPE_MAC;
            sdk::lib::memrev(mapping_key.p4e_i2e_mapping_lkp_addr,
                             spec->mac_addr, ETH_ADDR_LEN);
            PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL, NULL,
                                           0, sdk::table::handle_t::null());
            ret = mapping_impl_db()->mapping_tbl()->reserve(&tparams);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to reserve entry in MAPPING "
                              "table for vnic %s, err %u",
                              spec->key.str(), ret);
                goto error;
            }
            mapping_hdl_ = tparams.handle;

            // reserve an entry in IP_MAC_BINDING table always (table is big
            // enough) and whether to use this or not we will decide when
            // mappings are programmed
            ret =
                mapping_impl_db()->ip_mac_binding_idxr()->alloc(&binding_hw_id_);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to reserve entry in IP_MAC_BINDING table "
                              "for vnic %s, err %u", spec->key.str(), ret);
                goto error;
            }
        }
        break;

    case API_OP_UPDATE:
        // we will use the same h/w resources as the original object
    default:
        break;
    }
    return SDK_RET_OK;

error:

    PDS_TRACE_ERR("Failed to acquire all h/w resources for vnic %s, err %u",
                  spec->key.str(), ret);
    return ret;
}

sdk_ret_t
vnic_impl::release_resources(api_base *api_obj) {
    sdk_ret_t ret;
    subnet_entry *subnet;
    pds_encap_t vnic_encap;
    pds_obj_key_t subnet_key;
    mapping_swkey_t mapping_key;
    lif_vlan_swkey_t lif_vlan_key;
    sdk_table_api_params_t tparams;
    device_entry *device = device_find();
    local_mapping_swkey_t local_mapping_key;
    vnic_entry *vnic = (vnic_entry *)api_obj;

    if (mapping_hdl_.valid()) {
        // lookup the vnic's subnet
        subnet_key = vnic->subnet();
        subnet = subnet_find(&subnet_key);

        // release the reserved LOCAL_MAPPING table entry
        memset(&local_mapping_key, 0, sizeof(local_mapping_key));
        local_mapping_key.key_metadata_local_mapping_lkp_type =
            KEY_TYPE_MAC;
        local_mapping_key.key_metadata_local_mapping_lkp_id =
            ((subnet_impl *)subnet->impl())->hw_id();
        sdk::lib::memrev(
                      local_mapping_key.key_metadata_local_mapping_lkp_addr,
                      vnic->mac(), ETH_ADDR_LEN);
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &local_mapping_key,
                                       NULL, NULL, 0, local_mapping_hdl_);
        ret = mapping_impl_db()->local_mapping_tbl()->release(&tparams);
        SDK_ASSERT(ret == SDK_RET_OK);

        // release the reserved MAPPING table entry
        memset(&mapping_key, 0, sizeof(local_mapping_key));
        mapping_key.p4e_i2e_mapping_lkp_type = KEY_TYPE_MAC;
        mapping_key.p4e_i2e_mapping_lkp_id =
            ((subnet_impl *)subnet->impl())->hw_id();
        sdk::lib::memrev(mapping_key.p4e_i2e_mapping_lkp_addr,
                         vnic->mac(), ETH_ADDR_LEN);
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL, NULL,
                                       0, mapping_hdl_);
        ret = mapping_impl_db()->mapping_tbl()->release(&tparams);
        SDK_ASSERT(ret == SDK_RET_OK);
    }

    // release the NEXTHOP table entry
    if (nh_idx_ != 0xFFFF) {
        nexthop_impl_db()->nh_idxr()->free(nh_idx_);
    }

    // release the vnic h/w id
    if (hw_id_ != 0xFFFF) {
        vnic_impl_db()->vnic_idxr()->free(hw_id_);
    }

    // release LIF_VLAN entry handle
    if (lif_vlan_hdl_.valid()) {
        vnic_encap = vnic->vnic_encap();
        PDS_IMPL_FILL_LIF_VLAN_KEY(&lif_vlan_key, &vnic_encap);
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &lif_vlan_key, NULL, NULL,
                                       LIF_VLAN_LIF_VLAN_INFO_ID,
                                       lif_vlan_hdl_);
        apulu_impl_db()->lif_vlan_tbl()->release(&tparams);
    }

    // free the IP_MAC_BINDING table entries reserved
    if (binding_hw_id_ != PDS_IMPL_RSVD_IP_MAC_BINDING_HW_ID) {
        mapping_impl_db()->ip_mac_binding_idxr()->free(binding_hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::nuke_resources(api_base *api_obj) {
    sdk_ret_t ret;
    subnet_entry *subnet;
    pds_encap_t vnic_encap;
    pds_obj_key_t subnet_key;
    mapping_swkey_t mapping_key;
    lif_vlan_swkey_t lif_vlan_key;
    sdk_table_api_params_t tparams;
    device_entry *device = device_find();
    local_mapping_swkey_t local_mapping_key;
    vnic_entry *vnic = (vnic_entry *)api_obj;

    if (mapping_hdl_.valid()) {
        // lookup the vnic's subnet
        subnet_key = vnic->subnet();
        subnet = subnet_find(&subnet_key);

        // remove entry from LOCAL_MAPPING table
        memset(&local_mapping_key, 0, sizeof(local_mapping_key));
        local_mapping_key.key_metadata_local_mapping_lkp_type =
            KEY_TYPE_MAC;
        local_mapping_key.key_metadata_local_mapping_lkp_id =
            ((subnet_impl *)subnet->impl())->hw_id();
        sdk::lib::memrev(
                     local_mapping_key.key_metadata_local_mapping_lkp_addr,
                     vnic->mac(), ETH_ADDR_LEN);
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &local_mapping_key,
                                       NULL, NULL, 0,
                                       sdk::table::handle_t::null());
        ret = mapping_impl_db()->local_mapping_tbl()->remove(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to remove entry in LOCAL_MAPPING "
                          "table for vnic %s, err %u",
                          vnic->key().str(), ret);
            // fall thru
        }

        // remove entry from MAPPING table
        memset(&mapping_key, 0, sizeof(mapping_key));
        mapping_key.p4e_i2e_mapping_lkp_id =
            ((subnet_impl *)subnet->impl())->hw_id();
        mapping_key.p4e_i2e_mapping_lkp_type = KEY_TYPE_MAC;
        sdk::lib::memrev(mapping_key.p4e_i2e_mapping_lkp_addr,
                         vnic->mac(), ETH_ADDR_LEN);
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL, NULL,
                                       0, sdk::table::handle_t::null());
        ret = mapping_impl_db()->mapping_tbl()->remove(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to remote entry in MAPPING "
                          "table for vnic %s, err %u",
                          vnic->key().str(), ret);
            // fall thru
        }
    }

    // release the NEXTHOP table entry
    if (nh_idx_ != 0xFFFF) {
        nexthop_impl_db()->nh_idxr()->free(nh_idx_);
    }

    // free the vnic hw id
    if (hw_id_ != 0xFFFF) {
        vnic_impl_db()->vnic_idxr()->free(hw_id_);
    }

    // release LIF_VLAN entry handle
    if (lif_vlan_hdl_.valid()) {
        vnic_encap = vnic->vnic_encap();
        PDS_IMPL_FILL_LIF_VLAN_KEY(&lif_vlan_key, &vnic_encap);
        // slhash table requires handle to be passed during remove()
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &lif_vlan_key, NULL, NULL,
                                       LIF_VLAN_LIF_VLAN_INFO_ID,
                                       lif_vlan_hdl_);
        apulu_impl_db()->lif_vlan_tbl()->remove(&tparams);
    }

    // free the IP_MAC_BINDING table entries reserved
    if (binding_hw_id_ != PDS_IMPL_RSVD_IP_MAC_BINDING_HW_ID) {
        mapping_impl_db()->ip_mac_binding_idxr()->free(binding_hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::populate_msg(pds_msg_t *msg, api_base *api_obj,
                        api_obj_ctxt_t *obj_ctxt) {
    lif_impl *lif;

    msg->cfg_msg.vnic.status.hw_id = hw_id_;
    msg->cfg_msg.vnic.status.nh_hw_id = nh_idx_;
    // populate lif id, if vnic has host intf association
    lif = lif_impl_db()->find(&msg->cfg_msg.vnic.spec.host_if);
    if (lif) {
        msg->cfg_msg.vnic.status.host_if_hw_id = lif->id();
    }
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::populate_rxdma_vnic_info_policy_root_(
               vnic_info_rxdma_actiondata_t *vnic_info_data,
               uint32_t idx, mem_addr_t addr) {
    switch (idx) {
    case 0:
        MEM_ADDR_TO_P4_MEM_ADDR(
            vnic_info_data->rxdma_vnic_info.lpm_base1, addr,
            sizeof(vnic_info_data->rxdma_vnic_info.lpm_base1));
        break;
    case 1:
        MEM_ADDR_TO_P4_MEM_ADDR(
            vnic_info_data->rxdma_vnic_info.lpm_base2, addr,
            sizeof(vnic_info_data->rxdma_vnic_info.lpm_base2));
        break;
    case 2:
        MEM_ADDR_TO_P4_MEM_ADDR(
            vnic_info_data->rxdma_vnic_info.lpm_base3, addr,
            sizeof(vnic_info_data->rxdma_vnic_info.lpm_base3));
        break;
    case 3:
        MEM_ADDR_TO_P4_MEM_ADDR(
            vnic_info_data->rxdma_vnic_info.lpm_base4, addr,
            sizeof(vnic_info_data->rxdma_vnic_info.lpm_base4));
        break;
    case 4:
        MEM_ADDR_TO_P4_MEM_ADDR(
            vnic_info_data->rxdma_vnic_info.lpm_base5, addr,
            sizeof(vnic_info_data->rxdma_vnic_info.lpm_base5));
        break;
    case 5:
        MEM_ADDR_TO_P4_MEM_ADDR(
            vnic_info_data->rxdma_vnic_info.lpm_base6, addr,
            sizeof(vnic_info_data->rxdma_vnic_info.lpm_base6));
        break;
    case 6:
        MEM_ADDR_TO_P4_MEM_ADDR(
            vnic_info_data->rxdma_vnic_info.lpm_base7, addr,
            sizeof(vnic_info_data->rxdma_vnic_info.lpm_base7));
        break;
    default:
        PDS_TRACE_ERR("Failed to pack %uth entry in VNIC_INFO_RXDMA table", idx);
        SDK_ASSERT(false);
        return SDK_RET_INVALID_ARG;
    }
    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// VNIC_INFO table in RxDMA is partitioned into two halves - one is used on Tx
// direction and other half in Rx direction. In each direction, two adjacent
// entries are taken per vnic, 1st one for IPv4 and 2nd one IPv6 so overall
// four entries are consumed for each vnic
//------------------------------------------------------------------------------
sdk_ret_t
vnic_impl::program_rxdma_vnic_info_(vnic_entry *vnic, vpc_entry *vpc,
                                    subnet_entry *subnet) {
    uint32_t i;
    sdk_ret_t ret;
    policy *sec_policy;
    route_table *rtable;
    p4pd_error_t p4pd_ret;
    pds_obj_key_t policy_key;
    pds_obj_key_t route_table_key;
    pds_device_oper_mode_t oper_mode;
    policy *ing_v4_policy, *ing_v6_policy;
    policy *egr_v4_policy, *egr_v6_policy;
    vnic_info_rxdma_actiondata_t vnic_info_data;
    mem_addr_t addr, v4_lpm_addr = { 0 }, v6_lpm_addr = { 0 };

    // program IPv4 ingress entry
    memset(&vnic_info_data, 0, sizeof(vnic_info_data));
    vnic_info_data.action_id = VNIC_INFO_RXDMA_VNIC_INFO_RXDMA_ID;

    oper_mode = g_pds_state.device_oper_mode();
    // populate IPv4 route table root address in Rx direction entry
    route_table_key = subnet->v4_route_table();
    if (route_table_key == PDS_ROUTE_TABLE_ID_INVALID) {
        // try the vpc route table
        route_table_key = vpc->v4_route_table();
    }
    if (route_table_key != PDS_ROUTE_TABLE_ID_INVALID) {
        rtable = route_table_find(&route_table_key);
        if (rtable) {
            v4_lpm_addr = ((impl::route_table_impl *)
                               (rtable->impl()))->lpm_root_addr();
            PDS_TRACE_DEBUG("IPv4 lpm root addr 0x%lx", v4_lpm_addr);
            populate_rxdma_vnic_info_policy_root_(&vnic_info_data, 0,
                                                  v4_lpm_addr);
        }
    }

    // if subnet has ingress IPv4 policy, that should be evaluated first in the
    // Rx direction
    // populate egress IPv4 policy roots in the Tx direction entry
    i = 1; // policy roots start at index 1
    for (uint32_t j = 0; j < subnet->num_ing_v4_policy(); j++) {
        policy_key = subnet->ing_v4_policy(j);
        sec_policy = policy_find(&policy_key);
        if (sec_policy) {
            addr = ((impl::security_policy_impl *)
                        (sec_policy->impl()))->security_policy_root_addr();
            PDS_TRACE_DEBUG("IPv4 subnet ing policy root addr 0x%lx", addr);
            populate_rxdma_vnic_info_policy_root_(&vnic_info_data,
                                                  i++, addr);
        }
    }

    // populate ingress IPv4 policy roots in the Rx direction entry now
    for (uint32_t j = 0; j < vnic->num_ing_v4_policy(); j++) {
        policy_key = vnic->ing_v4_policy(j);
        sec_policy = policy_find(&policy_key);
        PDS_TRACE_ERR("Looking for policy %s", policy_key.str());
        addr = ((impl::security_policy_impl *)
                    (sec_policy->impl()))->security_policy_root_addr();
        PDS_TRACE_DEBUG("IPv4 vnic ing policy root addr 0x%lx", addr);
        populate_rxdma_vnic_info_policy_root_(&vnic_info_data, i++, addr);
    }

    // program v4 VNIC_INFO_RXDMA entry for Rx direction at index = (hw_id_*2)
    // NOTE: In the Rx direction, we are not doing route lookups yet, not
    // populating them
    p4pd_ret = p4pd_global_entry_write(P4_P4PLUS_RXDMA_TBL_ID_VNIC_INFO_RXDMA,
                                       (hw_id_ * 2), NULL, NULL,
                                       &vnic_info_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program v4 entry in VNIC_INFO_RXDMA table "
                      "at %u", (hw_id_ * 2));
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    } else {
        PDS_TRACE_VERBOSE("Programmed v4 Rx entry in VNIC_INFO_RXDMA table "
                          "at %u", (hw_id_ * 2));
    }

    // program IPv6 ingress entry
    memset(&vnic_info_data, 0, sizeof(vnic_info_data));
    vnic_info_data.action_id = VNIC_INFO_RXDMA_VNIC_INFO_RXDMA_ID;
    // populate IPv6 route table root address in Rx direction entry
    route_table_key = subnet->v6_route_table();
    if (route_table_key == PDS_ROUTE_TABLE_ID_INVALID) {
        // try the vpc route table
        route_table_key = vpc->v6_route_table();
    }
    if (route_table_key != PDS_ROUTE_TABLE_ID_INVALID) {
        rtable = route_table_find(&route_table_key);
        if (rtable) {
            v6_lpm_addr = ((impl::route_table_impl *)
                               (rtable->impl()))->lpm_root_addr();
            PDS_TRACE_DEBUG("IPv6 lpm root addr 0x%lx", v6_lpm_addr);
            populate_rxdma_vnic_info_policy_root_(&vnic_info_data,
                                                  0, v6_lpm_addr);
        }
    }

    // if subnet has ingress IPv6 policy, that should be evaluated first in the
    // Rx direction
    i = 1; // policy roots start at index 1
    for (uint32_t j = 0; j < subnet->num_ing_v6_policy(); j++) {
        policy_key = subnet->ing_v6_policy(j);
        sec_policy = policy_find(&policy_key);
        if (sec_policy) {
            addr = ((impl::security_policy_impl *)
                        (sec_policy->impl()))->security_policy_root_addr();
            PDS_TRACE_DEBUG("IPv6 subnet ing policy root addr 0x%lx", addr);
            populate_rxdma_vnic_info_policy_root_(&vnic_info_data,
                                                  i++, addr);
        }
    }

    // populate ingress IPv6 policy roots in the Rx direction entry
    for (uint32_t j = 0; j < vnic->num_ing_v6_policy(); j++) {
        policy_key = vnic->ing_v6_policy(j);
        sec_policy = policy_find(&policy_key);
        addr = ((impl::security_policy_impl *)
                    (sec_policy->impl()))->security_policy_root_addr();
        PDS_TRACE_DEBUG("IPv6 vnic ing policy root addr 0x%lx", addr);
        populate_rxdma_vnic_info_policy_root_(&vnic_info_data, i++, addr);
    }

    // program v6 VNIC_INFO_RXDMA entry for Rx direction at
    // index = (hw_id_ * 2) + 1
    // NOTE: In the Rx direction, we are not doing route lookups yet, not
    //       populating them
    p4pd_ret = p4pd_global_entry_write(P4_P4PLUS_RXDMA_TBL_ID_VNIC_INFO_RXDMA,
                                       (hw_id_ * 2) + 1, NULL, NULL,
                                       &vnic_info_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program v6 entry in VNIC_INFO_RXDMA table "
                      "at %u", (hw_id_ * 2) + 1);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    } else {
        PDS_TRACE_VERBOSE("Programmed v6 Rx entry in VNIC_INFO_RXDMA table "
                          "at %u", (hw_id_ * 2) + 1);
    }

    // program IPv4 egress entry
    memset(&vnic_info_data, 0, sizeof(vnic_info_data));
    vnic_info_data.action_id = VNIC_INFO_RXDMA_VNIC_INFO_RXDMA_ID;
    // populate IPv4 route table root address in Tx direction entry
    populate_rxdma_vnic_info_policy_root_(&vnic_info_data, 0, v4_lpm_addr);

    i = 1; // policy roots start at index 1
    // populate egress IPv4 policy roots in the Tx direction entry
    for (uint32_t j = 0; j < vnic->num_egr_v4_policy(); j++) {
        policy_key = vnic->egr_v4_policy(j);
        sec_policy = policy_find(&policy_key);
        addr = ((impl::security_policy_impl *)
                    (sec_policy->impl()))->security_policy_root_addr();
        PDS_TRACE_DEBUG("IPv4 vnic egr policy root addr 0x%lx", addr);
        populate_rxdma_vnic_info_policy_root_(&vnic_info_data, i++, addr);
    }

    // if subnet has egress IPv4 policy, append that policy tree root
    for (uint32_t j = 0; j < subnet->num_egr_v4_policy(); j++) {
        policy_key = subnet->egr_v4_policy(j);
        sec_policy = policy_find(&policy_key);
        if (sec_policy) {
            addr = ((impl::security_policy_impl *)
                        (sec_policy->impl()))->security_policy_root_addr();
            PDS_TRACE_DEBUG("IPv4 subnet egr policy root addr 0x%lx", addr);
            populate_rxdma_vnic_info_policy_root_(&vnic_info_data,
                                                  i++, addr);
        }
    }

    // program v4 VNIC_INFO_RXDMA entry for Tx direction in 2nd half of the
    // table at (VNIC_INFO_TABLE_SIZE*2) + (hw_id_*2) index
    p4pd_ret = p4pd_global_entry_write(P4_P4PLUS_RXDMA_TBL_ID_VNIC_INFO_RXDMA,
                   (VNIC_INFO_TABLE_SIZE * 2) + (hw_id_ * 2),
                   NULL, NULL, &vnic_info_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program VNIC_INFO_RXDMA table at %u",
                      (VNIC_INFO_TABLE_SIZE * 2) + (hw_id_ * 2));
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    } else {
        PDS_TRACE_VERBOSE("Programmed v4 Tx entry in VNIC_INFO_RXDMA table "
                          "at %u", (VNIC_INFO_TABLE_SIZE * 2) + (hw_id_ * 2));
    }

    // program IPv6 egress entry
    memset(&vnic_info_data, 0, sizeof(vnic_info_data));
    vnic_info_data.action_id = VNIC_INFO_RXDMA_VNIC_INFO_RXDMA_ID;
    // populate IPv6 route table root address in Tx direction entry
    populate_rxdma_vnic_info_policy_root_(&vnic_info_data, 0, v6_lpm_addr);

    i = 1; // policy roots start at index 1
    // populate egress IPv6 policy roots in the Tx direction entry
    for (uint32_t j = 0; j < vnic->num_egr_v6_policy(); j++) {
        policy_key =  vnic->egr_v6_policy(j);
        sec_policy = policy_find(&policy_key);
        addr = ((impl::security_policy_impl *)
                    (sec_policy->impl()))->security_policy_root_addr();
        PDS_TRACE_DEBUG("IPv6 vnic egr policy root addr 0x%lx", addr);
        populate_rxdma_vnic_info_policy_root_(&vnic_info_data, i++, addr);
    }

    // if subnet has egress IPv6 policy, append that policy tree root
    for (uint32_t j = 0; j < subnet->num_egr_v6_policy(); j++) {
        policy_key = subnet->egr_v6_policy(j);
        sec_policy = policy_find(&policy_key);
        if (sec_policy) {
            addr = ((impl::security_policy_impl *)
                        (sec_policy->impl()))->security_policy_root_addr();
            PDS_TRACE_DEBUG("IPv6 subnet egr policy root addr 0x%lx", addr);
            populate_rxdma_vnic_info_policy_root_(&vnic_info_data,
                                                  i++, addr);
        }
    }

    // program v6 VNIC_INFO_RXDMA entry for Tx direction in 2nd half of the
    // table at (VNIC_INFO_TABLE_SIZE*2) + (hw_id_*2) + 1 index
    p4pd_ret = p4pd_global_entry_write(P4_P4PLUS_RXDMA_TBL_ID_VNIC_INFO_RXDMA,
                   (VNIC_INFO_TABLE_SIZE * 2) + (hw_id_ * 2) + 1,
                   NULL, NULL, &vnic_info_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program VNIC_INFO_RXDMA table at %u",
                      (VNIC_INFO_TABLE_SIZE*2) + (hw_id_*2) + 1);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    } else {
        PDS_TRACE_VERBOSE("Programmed v6 Tx entry in VNIC_INFO_RXDMA table "
                          "at %u",
                          (VNIC_INFO_TABLE_SIZE * 2) + (hw_id_ * 2) + 1);
    }
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::program_vnic_nh_(pds_device_oper_mode_t oper_mode,
                            pds_vnic_spec_t *spec,
                            nexthop_info_entry_t *nh_data) {
    lif_impl *lif;
    sdk_ret_t ret;

    // program the nexthop table
    switch (oper_mode) {
    case PDS_DEV_OPER_MODE_HOST:
        lif = lif_impl_db()->find(&spec->host_if);
        if (lif) {
            nh_data->set_lif(lif->id());
            nh_data->set_drop(FALSE);
        } else {
            nh_data->set_drop(TRUE);
        }
        nh_data->set_port(TM_PORT_DMA);
        if (spec->vnic_encap.type == PDS_ENCAP_TYPE_DOT1Q) {
            nh_data->set_vlan(spec->vnic_encap.val.vlan_tag);
        }
        nh_data->set_dmaci(MAC_TO_UINT64(spec->mac_addr));
        nh_data->set_rx_vnic_id(hw_id_);
        ret = nh_data->write(nh_idx_);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program NEXTHOP table for vnic %s at "
                          "idx %u", spec->key.str(), nh_idx_);
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
        break;
    case PDS_DEV_OPER_MODE_BITW_SMART_SWITCH:
        // uplink0 is host port, uplink1 is sdn port by default
        // TODO: we can make this configurable
        nh_data->set_port(0);
        if (spec->vnic_encap.type == PDS_ENCAP_TYPE_DOT1Q) {
            nh_data->set_vlan(spec->vnic_encap.val.vlan_tag);
        }
        nh_data->set_dmaci(MAC_TO_UINT64(spec->mac_addr));
        nh_data->set_rx_vnic_id(hw_id_);
        ret = nh_data->write(nh_idx_);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program NEXTHOP table for vnic %s at "
                          "idx %u", spec->key.str(), nh_idx_);
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
        break;
    case PDS_DEV_OPER_MODE_BITW_SMART_SERVICE:
        // in this mode all flows use PDS_IMPL_UPLINK_ECMP_NHGROUP_HW_ID
        // ECMP nexthop
#if 0
        // uplink0 is host port, uplink1 is sdn port by default
        // TODO: we can make this configurable
        nh_data->set_port(0);
        // incoming is QinQ, outgoing .1q (or untagged)
        nh_data->set_vlan(spec->vnic_encap.val.qinq.c_tag);
        nh_data->set_rx_vnic_id(hw_id_);
        ret = nh_data->write(nh_idx_);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program NEXTHOP table for vnic %s at "
                          "idx %u", spec->key.str(), nh_idx_);
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
#endif
        break;
    case PDS_DEV_OPER_MODE_BITW_CLASSIC_SWITCH:
    default:
        PDS_TRACE_ERR("Unsupported device operational mode %u", oper_mode);
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::program_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    vpc_entry *vpc;
    subnet_entry *subnet;
    device_entry *device;
    p4pd_error_t p4pd_ret;
    nexthop_info_entry_t nh_data;
    pds_device_oper_mode_t oper_mode;
    vnic_actiondata_t vnic_data = { 0 };
    pds_obj_key_t vpc_key, tx_policer_key;
    policer_entry *rx_policer, *tx_policer;
    binding_info_entry_t ip_mac_binding_data;
    rx_vnic_actiondata_t rx_vnic_data = { 0 };
    meter_stats_actiondata_t meter_stats_data = { 0 };
    vnic_rx_stats_actiondata_t vnic_rx_stats_data = { 0 };
    vnic_tx_stats_actiondata_t vnic_tx_stats_data = { 0 };
    pds_vnic_spec_t *spec = &obj_ctxt->api_params->vnic_spec;

    device = device_find();
    oper_mode = g_pds_state.device_oper_mode();
    // get the subnet of this vnic
    subnet = subnet_find(&spec->subnet);
    if (subnet == NULL) {
        PDS_TRACE_ERR("Failed to find subnet %s", spec->subnet.str());
        return sdk::SDK_RET_INVALID_ARG;
    }

    // get the vpc of this subnet
    vpc_key = subnet->vpc();
    vpc = vpc_find(&vpc_key);
    if (unlikely(vpc == NULL)) {
        PDS_TRACE_ERR("Failed to find vpc %s", vpc_key.str());
        return sdk::SDK_RET_INVALID_ARG;
    }

    // get the Tx policer, if configured
    if (spec->tx_policer != k_pds_obj_key_invalid) {
        tx_policer = policer_db()->find(&spec->tx_policer);
        if (unlikely(tx_policer == NULL)) {
            PDS_TRACE_ERR("Failed to find vnic %s tx policer %s",
                          spec->key.str(), spec->tx_policer.str());
            return sdk::SDK_RET_INVALID_ARG;
        }
    } else if ((tx_policer_key = device->tx_policer()) !=
                   k_pds_obj_key_invalid) {
        tx_policer = policer_db()->find(&tx_policer_key);
        if (unlikely(tx_policer == NULL)) {
            PDS_TRACE_ERR("Failed to find device tx policer %s",
                          tx_policer_key.str());
            return sdk::SDK_RET_INVALID_ARG;
        }
    } else {
        tx_policer = NULL;
    }

    // get the Rx policer, if configured
    if (spec->rx_policer != k_pds_obj_key_invalid) {
        rx_policer = policer_db()->find(&spec->rx_policer);
        if (unlikely(rx_policer == NULL)) {
            PDS_TRACE_ERR("Failed to find vnic %s rx policer %s",
                          spec->key.str(), spec->rx_policer.str());
            return sdk::SDK_RET_INVALID_ARG;
        }
    } else {
        rx_policer = NULL;
    }

    // initialize tx stats table for this vnic
    vnic_tx_stats_data.action_id = VNIC_TX_STATS_VNIC_TX_STATS_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_VNIC_TX_STATS,
                                       hw_id_, NULL, NULL,
                                       &vnic_tx_stats_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program vnic %s TX_STATS table entry at %u",
                      spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // initialize rx stats tables for this vnic
    vnic_rx_stats_data.action_id = VNIC_RX_STATS_VNIC_RX_STATS_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_VNIC_RX_STATS,
                                       hw_id_, NULL, NULL,
                                       &vnic_rx_stats_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program vnic %s RX_STATS table entry at %u",
                      spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // initialize the meter table entries for this vnic
    // NOTE: each vnic takes two entries in the METER_STATS table, one for Rx
    //       and one for Tx direction traffic
    meter_stats_data.action_id = METER_STATS_METER_STATS_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_METER_STATS,
                                       hw_id_, NULL, NULL,
                                       &meter_stats_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program vnic %s METER_STATS table entry at %u",
                      spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    meter_stats_data.action_id = METER_STATS_METER_STATS_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_METER_STATS,
                                       (METER_TABLE_SIZE >> 1) + hw_id_,
                                       NULL, NULL, &meter_stats_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program vnic %s METER_STATS table entry at %u",
                      spec->key.str(), (METER_TABLE_SIZE >> 1) + hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // program vnic table entry in the ingress pipeline
    vnic_data.action_id = VNIC_VNIC_INFO_ID;
    vnic_data.ing_vnic_info.epoch = epoch_;
    vnic_data.ing_vnic_info.meter_enabled = spec->meter_en;
    vnic_data.ing_vnic_info.rx_mirror_session =
        compute_mirror_bitmap(spec->num_rx_mirror_session,
                              spec->rx_mirror_session);
    vnic_data.ing_vnic_info.tx_mirror_session =
        compute_mirror_bitmap(spec->num_tx_mirror_session,
                              spec->tx_mirror_session);
    vnic_data.ing_vnic_info.binding_check_enabled =
        spec->binding_checks_en ? TRUE : FALSE;
    if (tx_policer) {
         vnic_data.ing_vnic_info.tx_policer_id =
             ((policer_impl *)(tx_policer->impl()))->hw_id();
    } else {
         vnic_data.ing_vnic_info.tx_policer_id = PDS_IMPL_RSVD_POLICER_HW_ID;
    }
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_VNIC, hw_id_,
                                       NULL, NULL, &vnic_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program vnic %s ingress VNIC table "
                      "entry at %u", spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // program vnic table entry in the egress pipeline
    rx_vnic_data.action_id = RX_VNIC_RX_VNIC_INFO_ID;
    if (rx_policer) {
        rx_vnic_data.egr_vnic_info.rx_policer_id =
            ((policer_impl *)(rx_policer->impl()))->hw_id();
    } else {
        rx_vnic_data.egr_vnic_info.rx_policer_id = PDS_IMPL_RSVD_POLICER_HW_ID;
    }
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_RX_VNIC, hw_id_,
                                       NULL, NULL, &rx_vnic_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program vnic %s egres RX_VNIC table "
                      "entry at %u", spec->key.str(), hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // program the IP_MAC_BINDING table and keep it ready
    memset(&ip_mac_binding_data, 0, sizeof(ip_mac_binding_data));
    sdk::lib::memrev(ip_mac_binding_data.addr, spec->mac_addr, ETH_ADDR_LEN);
    ret = ip_mac_binding_data.write(binding_hw_id_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program IP_MAC_BINDING table entry %u for "
                      "vnic %s, err %u", binding_hw_id_, spec->key.str(), ret);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // program the nexthop table
    memset(&nh_data, 0, sizeof(nh_data));
    ret = program_vnic_nh_(oper_mode, spec, &nh_data);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program NEXTHOP table at idx %u for vnic %s",
                      nh_idx_, spec->key.str());
        return ret;
    }

    // program vnic info tables (in rxdma and txdma pipes)
    ret = program_rxdma_vnic_info_((vnic_entry *)api_obj, vpc, subnet);
    return ret;
}

sdk_ret_t
vnic_impl::update_hw(api_base *orig_obj, api_base *curr_obj,
                     api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    vpc_entry *vpc;
    device_entry *device;
    subnet_entry *subnet;
    pds_obj_key_t lif_key;
    pds_obj_key_t vpc_key;
    pds_vnic_spec_t *spec;
    p4pd_error_t p4pd_ret;
    vnic_actiondata_t vnic_data;
    pds_obj_key_t tx_policer_key;
    nexthop_info_entry_t nh_data;
    rx_vnic_actiondata_t rx_vnic_data;
    policer_entry *rx_policer, *tx_policer;
    vnic_entry *vnic = (vnic_entry *)curr_obj;

    memset(&nh_data, 0, nh_data.entry_size());
    spec = &obj_ctxt->api_params->vnic_spec;
    // we don't need to reset the VNIC_TX_STATS and VNIC_RX_STATS
    // table entries because of udpate
    // update the nexthop table entry, if needed
    if ((obj_ctxt->upd_bmap & PDS_VNIC_UPD_HOST_IFINDEX) ||
        (obj_ctxt->upd_bmap & PDS_VNIC_UPD_VNIC_ENCAP)) {
        device = device_find();
        ret = nh_data.read(nh_idx_);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to read nexthop table for vnic %s at "
                          "idx %u", spec->key.str(), nh_idx_);
            return sdk::SDK_RET_HW_READ_ERR;
        }
        ret = program_vnic_nh_(device->oper_mode(), spec, &nh_data);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program NEXTHOP table at idx %u for vnic %s",
                          nh_idx_, spec->key.str());
            return ret;
        }
    }

    // if there is a change to route tables and/or policy tables this vnic is
    // pointing and/or if the vnic itself is modified to point to different
    // route/policy tables, update the vnic info programming
    if ((obj_ctxt->upd_bmap & PDS_VNIC_UPD_POLICY) ||
        (obj_ctxt->upd_bmap & PDS_VNIC_UPD_ROUTE_TABLE)) {
        subnet = subnet_find(&spec->subnet);
        vpc_key = subnet->vpc();
        vpc = vpc_find(&vpc_key);
        ret = program_rxdma_vnic_info_((vnic_entry *)curr_obj, vpc, subnet);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to update rxdma VNIC_INFO table for "
                          "vnic %s, err %u", spec->key.str(), ret);
            return ret;
        }
    }

    // if mirror sessions or metering enable/disable config changed, update
    // ingress VNIC table entry
    if ((obj_ctxt->upd_bmap & PDS_VNIC_UPD_METER_EN)          ||
        (obj_ctxt->upd_bmap & PDS_VNIC_UPD_TX_POLICER)        ||
        (obj_ctxt->upd_bmap & PDS_VNIC_UPD_BINDING_CHECKS)    ||
        (obj_ctxt->upd_bmap & PDS_VNIC_UPD_TX_MIRROR_SESSION) ||
        (obj_ctxt->upd_bmap & PDS_VNIC_UPD_RX_MIRROR_SESSION)) {
        // do read-modify-update of the VNIC table entry
        p4pd_global_entry_read(P4TBL_ID_VNIC, hw_id_, NULL, NULL, &vnic_data);
        // take care of meter update
        if (obj_ctxt->upd_bmap & PDS_VNIC_UPD_METER_EN) {
            vnic_data.ing_vnic_info.meter_enabled = spec->meter_en;
        }
        // take care of tx policer update
        if (obj_ctxt->upd_bmap & PDS_VNIC_UPD_TX_POLICER) {
            if (spec->tx_policer != k_pds_obj_key_invalid) {
                tx_policer = policer_db()->find(&spec->tx_policer);
                if (unlikely(tx_policer == NULL)) {
                    PDS_TRACE_ERR("Failed to find vnic %s tx policer %s",
                                  spec->key.str(), spec->tx_policer.str());
                    return sdk::SDK_RET_INVALID_ARG;
                }
            } else if ((tx_policer_key = device_find()->tx_policer()) !=
                           k_pds_obj_key_invalid) {
                tx_policer = policer_db()->find(&tx_policer_key);
                if (unlikely(tx_policer == NULL)) {
                    PDS_TRACE_ERR("Failed to find device tx policer %s",
                                  tx_policer_key.str());
                    return sdk::SDK_RET_INVALID_ARG;
                }
            } else {
                tx_policer = NULL;
            }
            if (tx_policer) {
                vnic_data.ing_vnic_info.tx_policer_id =
                    ((policer_impl *)(tx_policer->impl()))->hw_id();
            } else {
                vnic_data.ing_vnic_info.tx_policer_id =
                    PDS_IMPL_RSVD_POLICER_HW_ID;
            }
        }
        // handle binding check config change
        if (obj_ctxt->upd_bmap & PDS_VNIC_UPD_BINDING_CHECKS) {
            vnic_data.ing_vnic_info.binding_check_enabled =
                spec->binding_checks_en ? TRUE : FALSE;
        }
        // handle Rx mirror session update, if any
        if (obj_ctxt->upd_bmap & PDS_VNIC_UPD_RX_MIRROR_SESSION) {
            vnic_data.ing_vnic_info.rx_mirror_session =
                compute_mirror_bitmap(spec->num_rx_mirror_session,
                                      spec->rx_mirror_session);
        }
        // handle Tx mirror session update, if any
        if (obj_ctxt->upd_bmap & PDS_VNIC_UPD_TX_MIRROR_SESSION) {
            vnic_data.ing_vnic_info.tx_mirror_session =
                compute_mirror_bitmap(spec->num_tx_mirror_session,
                                      spec->tx_mirror_session);
        }
        p4pd_ret = p4pd_global_entry_write(P4TBL_ID_VNIC, hw_id_,
                                           NULL, NULL, &vnic_data);
        if (p4pd_ret != P4PD_SUCCESS) {
            PDS_TRACE_ERR("Failed to program vnic %s ingress VNIC table "
                          "entry at %u", spec->key.str(), hw_id_);
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
    }
    // update RX_VNIC table in egress pipe if rx policer changed
    if (obj_ctxt->upd_bmap & PDS_VNIC_UPD_RX_POLICER) {
        p4pd_global_entry_read(P4TBL_ID_VNIC, hw_id_,
                               NULL, NULL, &rx_vnic_data);
        if (spec->rx_policer != k_pds_obj_key_invalid) {
            rx_policer = policer_db()->find(&spec->rx_policer);
            if (unlikely(rx_policer == NULL)) {
                PDS_TRACE_ERR("Failed to find vnic %s rx policer %s",
                              spec->key.str(), spec->rx_policer.str());
                return sdk::SDK_RET_INVALID_ARG;
            }
            rx_vnic_data.egr_vnic_info.rx_policer_id =
                ((policer_impl *)(rx_policer->impl()))->hw_id();
        } else {
            rx_vnic_data.egr_vnic_info.rx_policer_id =
                PDS_IMPL_RSVD_POLICER_HW_ID;
        }
        p4pd_ret = p4pd_global_entry_write(P4TBL_ID_RX_VNIC, hw_id_,
                                           NULL, NULL, &rx_vnic_data);
        if (p4pd_ret != P4PD_SUCCESS) {
            PDS_TRACE_ERR("Failed to program vnic %s egres RX_VNIC table "
                          "entry at %u", spec->key.str(), hw_id_);
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::add_local_mapping_entry_(pds_epoch_t epoch, vpc_entry *vpc,
                                    subnet_entry *subnet, vnic_entry *vnic,
                                    pds_vnic_spec_t *spec) {
    sdk_ret_t ret;
    sdk_table_api_params_t tparams;
    local_mapping_swkey_t local_mapping_key;
    local_mapping_appdata_t local_mapping_data;

    // fill the key
    PDS_IMPL_FILL_LOCAL_L2_MAPPING_SWKEY(&local_mapping_key,
        ((subnet_impl *)subnet->impl())->hw_id(),
        spec->mac_addr);
    // fill the data
    PDS_IMPL_FILL_LOCAL_MAPPING_APPDATA(&local_mapping_data, hw_id_,
                                        PDS_IMPL_RSVD_NAT_HW_ID,
                                        PDS_IMPL_RSVD_TAG_HW_ID,
                                        PDS_IMPL_RSVD_IP_MAC_BINDING_HW_ID,
                                        PDS_IMPL_RSVD_IP_MAC_BINDING_HW_ID,
                                        vnic->tagged(),
                                        MAPPING_TYPE_OVERLAY);

    // program LOCAL_MAPPING entry
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &local_mapping_key, NULL,
                                   &local_mapping_data,
                                   LOCAL_MAPPING_LOCAL_MAPPING_INFO_ID,
                                   local_mapping_hdl_);
    ret = mapping_impl_db()->local_mapping_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program LOCAL_MAPPING entry for vnic %s, "
                      "(subnet %s, mac %s)", spec->key.str(),
                      spec->subnet.str(), macaddr2str(spec->mac_addr));
        goto error;
    }
    return SDK_RET_OK;

error:
    return ret;
}

#define lif_vlan_info        action_u.lif_vlan_lif_vlan_info
sdk_ret_t
vnic_impl::add_lif_vlan_entry_(pds_epoch_t epoch, vpc_entry *vpc,
                               subnet_entry *subnet, vnic_entry *vnic,
                               pds_vnic_spec_t *spec) {
    sdk_ret_t ret;
    lif_vlan_swkey_t lif_vlan_key;
    sdk_table_api_params_t tparams;
    lif_vlan_actiondata_t lif_vlan_data;

    // program the LIF_VLAN table
    PDS_IMPL_FILL_LIF_VLAN_KEY(&lif_vlan_key, &spec->vnic_encap);
    memset(&lif_vlan_data, 0, sizeof(lif_vlan_data));
    lif_vlan_data.action_id = LIF_VLAN_LIF_VLAN_INFO_ID;
    lif_vlan_data.lif_vlan_info.vnic_id = hw_id_;
    lif_vlan_data.lif_vlan_info.bd_id =
        ((subnet_impl *)subnet->impl())->hw_id();
    lif_vlan_data.lif_vlan_info.vpc_id = ((vpc_impl *)vpc->impl())->hw_id();
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &lif_vlan_key, NULL,
                                   &lif_vlan_data, LIF_VLAN_LIF_VLAN_INFO_ID,
                                   lif_vlan_hdl_);
    ret = apulu_impl_db()->lif_vlan_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program LIF_VLAN entry for vnic %s, "
                      "encap %s, err %u", spec->key.str(),
                      pds_encap2str(&spec->vnic_encap), ret);
        return ret;
    }
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::add_mapping_entry_(pds_epoch_t epoch, vpc_entry *vpc,
                              subnet_entry *subnet, vnic_entry *vnic,
                              pds_vnic_spec_t *spec) {
    sdk_ret_t ret;
    sdk_table_api_params_t tparams;
    mapping_swkey_t mapping_key = { 0 };
    mapping_appdata_t mapping_data = { 0 };

    // fill the key
    mapping_key.p4e_i2e_mapping_lkp_id =
        ((subnet_impl *)subnet->impl())->hw_id();
    mapping_key.p4e_i2e_mapping_lkp_type = KEY_TYPE_MAC;
    sdk::lib::memrev(mapping_key.p4e_i2e_mapping_lkp_addr,
                     spec->mac_addr, ETH_ADDR_LEN);

    // fill the data
    mapping_data.is_local = TRUE;
    mapping_data.nexthop_valid = TRUE;
    mapping_data.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
    mapping_data.nexthop_id = nh_idx_;
    mapping_data.egress_bd_id =
        ((subnet_impl *)subnet->impl())->hw_id();
    sdk::lib::memrev(mapping_data.dmaci, spec->mac_addr, ETH_ADDR_LEN);
    mapping_data.rx_vnic_id = hw_id_;

    // program MAPPING table entry
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL, &mapping_data,
                                   MAPPING_MAPPING_INFO_ID,
                                   mapping_hdl_);
    ret = mapping_impl_db()->mapping_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program MAPPING entry for vnic %s, "
                      "(subnet %s, mac %s)", spec->key.str(),
                      spec->subnet.str(), macaddr2str(spec->mac_addr));
        goto error;
    }
    return SDK_RET_OK;

error:

    return ret;
}

sdk_ret_t
vnic_impl::activate_create_(pds_epoch_t epoch, vnic_entry *vnic,
                            pds_vnic_spec_t *spec) {
    sdk_ret_t ret;
    vpc_entry *vpc;
    subnet_entry *subnet;
    pds_obj_key_t vpc_key;

    // fetch the subnet of this vnic
    subnet = subnet_find(&spec->subnet);
    // fetch the vpc of this vnic
    vpc_key = subnet->vpc();
    vpc = vpc_find(&vpc_key);
    if (unlikely(vpc == NULL)) {
        return SDK_RET_INVALID_ARG;
    }

    // program LIF_VLAN table entry
    if (lif_vlan_hdl_.valid()) {
        ret = add_lif_vlan_entry_(epoch, vpc, subnet, vnic, spec);
        if (unlikely(ret != SDK_RET_OK)) {
            goto error;
        }
    }

    // program mapping entries
    if (mapping_hdl_.valid()) {
        ret = add_local_mapping_entry_(epoch, vpc, subnet, vnic, spec);
        if (unlikely(ret != SDK_RET_OK)) {
            goto error;
        }

        ret = add_mapping_entry_(epoch, vpc, subnet, vnic, spec);
        if (unlikely(ret != SDK_RET_OK)) {
            goto error;
        }
    }
    vnic_impl_db()->insert(hw_id_, this);
    return SDK_RET_OK;

error:

    return ret;
}

sdk_ret_t
vnic_impl::activate_delete_(pds_epoch_t epoch, vnic_entry *vnic) {
    sdk_ret_t ret;
    subnet_entry *subnet;
    p4pd_error_t p4pd_ret;
    pds_encap_t vnic_encap;
    pds_obj_key_t subnet_key;
    mapping_swkey_t mapping_key;
    lif_vlan_swkey_t lif_vlan_key;
    sdk_table_api_params_t tparams;
    mapping_appdata_t mapping_data;
    lif_vlan_actiondata_t lif_vlan_data;
    local_mapping_swkey_t local_mapping_key;
    local_mapping_appdata_t local_mapping_data;

    if (lif_vlan_hdl_.valid()) {
        // invalidate the entry in LIF_VLAN table
        vnic_encap = vnic->vnic_encap();
        PDS_IMPL_FILL_LIF_VLAN_KEY(&lif_vlan_key, &vnic_encap);
        memset(&lif_vlan_data, 0, sizeof(lif_vlan_data));
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &lif_vlan_key, NULL,
                                       &lif_vlan_data,
                                       LIF_VLAN_LIF_VLAN_INFO_ID,
                                       lif_vlan_hdl_);
        ret = apulu_impl_db()->lif_vlan_tbl()->update(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to clear LIF_VLAN entry for vnic %s, "
                          "encap %s, err %u", vnic->key2str().c_str(),
                          pds_encap2str(&vnic_encap), ret);
            return ret;
        }
    }

    if (mapping_hdl_.valid()) {
        subnet_key = vnic->subnet();
        subnet = subnet_find(&subnet_key);

        // invalidate LOCAL_MAPPING table entry of the MAC entry
        memset(&local_mapping_key, 0, sizeof(local_mapping_key));
        local_mapping_key.key_metadata_local_mapping_lkp_type = KEY_TYPE_MAC;
        local_mapping_key.key_metadata_local_mapping_lkp_id =
            ((subnet_impl *)subnet->impl())->hw_id();
        sdk::lib::memrev(local_mapping_key.key_metadata_local_mapping_lkp_addr,
                         vnic->mac(), ETH_ADDR_LEN);
        memset(&local_mapping_data, 0, sizeof(local_mapping_data));
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &local_mapping_key, NULL,
                                       &local_mapping_data,
                                       LOCAL_MAPPING_LOCAL_MAPPING_INFO_ID,
                                       sdk::table::handle_t::null());
        ret = mapping_impl_db()->local_mapping_tbl()->update(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to update LOCAL_MAPPING entry for vnic %s, "
                          "(subnet %s, mac %s), err %u", vnic->key().str(),
                          subnet_key.str(), macaddr2str(vnic->mac()), ret);
            return ret;
        }

        // invalidate MAPPING table entry of the MAC entry
        memset(&mapping_key, 0, sizeof(mapping_key));
        mapping_key.p4e_i2e_mapping_lkp_id =
            ((subnet_impl *)subnet->impl())->hw_id();
        mapping_key.p4e_i2e_mapping_lkp_type = KEY_TYPE_MAC;
        sdk::lib::memrev(mapping_key.p4e_i2e_mapping_lkp_addr,
                         vnic->mac(), ETH_ADDR_LEN);
        memset(&mapping_data, 0, sizeof(mapping_data));
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL,
                                       &mapping_data, MAPPING_MAPPING_INFO_ID,
                                       sdk::table::handle_t::null());
        ret = mapping_impl_db()->mapping_tbl()->update(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to update MAPPING entry for vnic %s, "
                          "(subnet %s, mac %s), ret %u", vnic->key().str(),
                          subnet_key.str(), macaddr2str(vnic->mac()), ret);
        }
    }
    vnic_impl_db()->remove(hw_id_);
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::upd_dhcp_binding_(vnic_entry *vnic, vnic_entry *orig_vnic,
                             subnet_entry *subnet, pds_vnic_spec_t *spec) {

    sdk_ret_t ret = SDK_RET_OK;

    if (orig_vnic->primary() && !vnic->primary()) {
        // vnic was marked as primary and but it is marked as non-primary
        // now, if we had hostname attribute set on this vnic earlier, we
        // must reset it in the DHCP bindings
        if (orig_vnic->hostname()[0] != '\0') {
            // reset the hostname in the DHCP bindings
            ret = mapping_impl_db()->update_dhcp_binding(vnic, subnet);
        }
    } else if (!orig_vnic->primary() && vnic->primary()) {
        // vnic was marked as non-primary and but it is marked as primary
        // now, if we have hostname attribute set on this vnic now, we must
        // update the DHCP binding
        if (spec->hostname[0] != '\0') {
            // update DHCP binding with the hostname
            ret = mapping_impl_db()->update_dhcp_binding(vnic, subnet);
        }
    } else if (vnic->primary() && (spec->hostname[0] != '\0')) {
        // primary attribute has not changed, but hostname could have changed
        if (memcmp(spec->hostname, orig_vnic->hostname(),
                   PDS_MAX_HOST_NAME_LEN)) {
            // update DHCP binding with the hostname
            ret = mapping_impl_db()->update_dhcp_binding(vnic, subnet);
        }
    }
    return ret;
}

sdk_ret_t
vnic_impl::activate_update_(pds_epoch_t epoch, vnic_entry *vnic,
                            vnic_entry *orig_vnic, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    vpc_entry *vpc;
    subnet_entry *subnet;
    pds_vnic_spec_t *spec;
    p4pd_error_t p4pd_ret;
    pds_obj_key_t vpc_key;
    lif_vlan_actiondata_t lif_vlan_data;

    spec = &obj_ctxt->api_params->vnic_spec;
    subnet = subnet_find(&spec->subnet);
#if 0
    if (obj_ctxt->upd_bmap & PDS_VNIC_UPD_VNIC_ENCAP) {
        if (orig_vnic->vnic_encap().type != PDS_ENCAP_TYPE_NONE) {
            // we need to cleanup old vlan table entry
            memset(&vlan_data, 0, sizeof(vlan_data));
            p4pd_ret = p4pd_global_entry_write(P4TBL_ID_VLAN,
                           orig_vnic->vnic_encap().val.vlan_tag,
                           NULL, NULL, &vlan_data);
            if (p4pd_ret != P4PD_SUCCESS) {
                PDS_TRACE_ERR("Failed to clear VLAN entry %u for vnic %u",
                              orig_vnic->vnic_encap().val.vlan_tag,
                              spec->vnic_encap.val.vlan_tag);
                return sdk::SDK_RET_HW_PROGRAM_ERR;
            }
        }

        if (spec->vnic_encap.type != PDS_ENCAP_TYPE_NONE) {
            // fetch the subnet of this vnic
            if (unlikely(subnet == NULL)) {
                return SDK_RET_INVALID_ARG;
            }

            // fetch the vpc of this vnic
            vpc_key = subnet->vpc();
            vpc = vpc_find(&vpc_key);
            if (unlikely(vpc == NULL)) {
                return SDK_RET_INVALID_ARG;
            }
            ret = add_vlan_entry_(epoch, vpc, subnet, vnic, spec);
            if (unlikely(ret != SDK_RET_OK)) {
                goto end;
            }
        }
    }
#endif

    // update DHCP binding, if needed
    ret = upd_dhcp_binding_(vnic, orig_vnic, subnet, spec);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);

    // delete old impl and insert cloned impl into ht
    vnic_impl_db()->remove(hw_id_);
    vnic_impl_db()->insert(hw_id_, this);
    return SDK_RET_OK;

#if 0
end:

    return ret;
#endif
}

sdk_ret_t
vnic_impl::activate_hw(api_base *api_obj, api_base *orig_obj, pds_epoch_t epoch,
                       api_op_t api_op, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_vnic_spec_t *spec;

    switch (api_op) {
    case API_OP_CREATE:
        spec = &obj_ctxt->api_params->vnic_spec;
        ret = activate_create_(epoch, (vnic_entry *)api_obj, spec);
        break;

    case API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (vnic_entry *)api_obj);
        break;

    case API_OP_UPDATE:
        ret = activate_update_(epoch, (vnic_entry *)api_obj,
                               (vnic_entry *)orig_obj, obj_ctxt);
        break;

    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

sdk_ret_t
vnic_impl::reprogram_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    vpc_entry *vpc;
    vnic_entry *vnic;
    subnet_entry *subnet;
    p4pd_error_t p4pd_ret;
    pds_obj_key_t vpc_key;
    pds_obj_key_t subnet_key;
    vnic_actiondata_t vnic_data;

    // the only programming that we need to do when vnic is in the dependent
    // list is to update the vnic info table in rxdma for policy and route
    // table updates
    if (obj_ctxt->upd_bmap & (PDS_VNIC_UPD_POLICY | PDS_VNIC_UPD_ROUTE_TABLE)) {
        vnic = (vnic_entry *)api_obj;
        subnet_key = vnic->subnet();
        subnet = subnet_find(&subnet_key);
        vpc_key = subnet->vpc();
        vpc = vpc_find(&vpc_key);
        ret = program_rxdma_vnic_info_((vnic_entry *)api_obj, vpc, subnet);
        SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);

        // update the epoch
        // do read-modify-update of the VNIC table entry
        p4pd_ret = p4pd_global_entry_read(P4TBL_ID_VNIC, hw_id_,
                                          NULL, NULL, &vnic_data);
        vnic_data.ing_vnic_info.epoch = PDS_IMPL_VNIC_EPOCH_NEXT(epoch_);
        p4pd_ret = p4pd_global_entry_write(P4TBL_ID_VNIC, hw_id_,
                                           NULL, NULL, &vnic_data);
        if (p4pd_ret != P4PD_SUCCESS) {
            PDS_TRACE_ERR("Failed to program vnic %s ingress VNIC table "
                          "entry at %u", vnic->key2str().c_str(), hw_id_);
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
        return SDK_RET_OK;
    }
    // not expecting any other recursive updates at this point
    SDK_ASSERT(FALSE);
    return SDK_RET_OK;
}

void
vnic_impl::fill_status_(pds_vnic_status_t *status) {
    status->hw_id = hw_id_;
    status->nh_hw_id = nh_idx_;
    status->epoch = epoch_;
}

sdk_ret_t
vnic_impl::fill_stats_(pds_vnic_stats_t *stats) {
    sdk_ret_t ret;
    p4pd_error_t p4pd_ret;
    vnic_tx_stats_actiondata_t tx_stats;
    vnic_rx_stats_actiondata_t rx_stats;
    meter_stats_actiondata_t meter_stats;

    // read P4TBL_ID_VNIC_TX_STATS table
    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_VNIC_TX_STATS, hw_id_, NULL,
                                      NULL, &tx_stats);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to read VNIC_TX_STATS table at index %u", hw_id_);
        return sdk::SDK_RET_HW_READ_ERR;
    }
    stats->tx_pkts  = *(uint64_t *)tx_stats.vnic_tx_stats_action.out_packets;
    stats->tx_bytes = *(uint64_t *)tx_stats.vnic_tx_stats_action.out_bytes;

    // read P4TBL_ID_VNIC_RX_STATS table
    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_VNIC_RX_STATS, hw_id_, NULL,
                                      NULL, &rx_stats);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to read VNIC_RX_STATS table at index %u", hw_id_);
        return sdk::SDK_RET_HW_READ_ERR;
    }
    stats->rx_pkts  = *(uint64_t *)rx_stats.vnic_rx_stats_action.in_packets;
    stats->rx_bytes = *(uint64_t *)rx_stats.vnic_rx_stats_action.in_bytes;

    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_METER_STATS, hw_id_, NULL, NULL,
                                      &meter_stats);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to read METER_STATS table entry at index %u",
                      hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    stats->meter_tx_pkts =
        *(uint64_t *)meter_stats.meter_stats_action.in_packets;
    stats->meter_tx_bytes =
        *(uint64_t *)meter_stats.meter_stats_action.in_bytes;

    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_METER_STATS,
                                      (METER_TABLE_SIZE >> 1) + hw_id_,
                                      NULL, NULL, &meter_stats);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to read METER_STATS table entry at index %u",
                      (METER_TABLE_SIZE >> 1) + hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    stats->meter_rx_pkts =
        *(uint64_t *)meter_stats.meter_stats_action.in_packets;
    stats->meter_rx_bytes =
        *(uint64_t *)meter_stats.meter_stats_action.in_bytes;
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::fill_spec_(pds_vnic_spec_t *spec) {
    sdk_ret_t ret;
    nexthop_info_entry_t nh_data;

    if (nh_idx_ != 0xFFFF) {
        // read the nexthop table
        ret = nh_data.read(nh_idx_);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to read NEXTHOP table for vnic %s at idx %u",
                          spec->key.str(), nh_idx_);
            return sdk::SDK_RET_HW_READ_ERR;
        }
        MAC_UINT64_TO_ADDR(spec->mac_addr, nh_data.get_dmaci());
    }
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    sdk_ret_t rv;
    pds_vnic_info_t *vnic_info = (pds_vnic_info_t *)info;

    rv = fill_spec_(&vnic_info->spec);
    if (unlikely(rv != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to read hardware spec tables for vnic %s",
                      api_obj->key2str().c_str());
        return rv;
    }

    rv = fill_stats_(&vnic_info->stats);
    if (unlikely(rv != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to read hardware stats tables for vnic %s",
                      api_obj->key2str().c_str());
        return rv;
    }
    fill_status_(&vnic_info->status);
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::reset_stats(void) {
    p4pd_error_t p4pd_ret;
    vnic_rx_stats_actiondata_t vnic_rx_stats_data = { 0 };
    vnic_tx_stats_actiondata_t vnic_tx_stats_data = { 0 };

    // reset tx stats table for this vnic
    vnic_tx_stats_data.action_id = VNIC_TX_STATS_VNIC_TX_STATS_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_VNIC_TX_STATS,
                                       hw_id_, NULL, NULL,
                                       &vnic_tx_stats_data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);

    // initialize rx stats tables for this vnic
    vnic_rx_stats_data.action_id = VNIC_RX_STATS_VNIC_RX_STATS_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_VNIC_RX_STATS,
                                       hw_id_, NULL, NULL,
                                       &vnic_rx_stats_data);
    SDK_ASSERT(p4pd_ret == P4PD_SUCCESS);
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::backup(obj_info_t *info, upg_obj_info_t *upg_info) {
    sdk_ret_t ret;
    upg_obj_tlv_t *tlv;
    pds_vnic_info_t *vnic_info;
    pds::VnicGetResponse proto_msg;

    tlv = (upg_obj_tlv_t *)upg_info->mem;
    vnic_info = (pds_vnic_info_t *)info;

    ret = fill_spec_(&vnic_info->spec);
    if (unlikely(ret != SDK_RET_OK)) {
        return ret;
    }
    fill_status_(&vnic_info->status);
    // convert api info to proto
    pds_vnic_api_info_to_proto(vnic_info, (void *)&proto_msg);
    ret = pds_svc_serialize_proto_msg(upg_info, tlv, &proto_msg);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to serialize vnic %s, err %u",
                      vnic_info->spec.key.str(), ret);
    }
    return ret;
}

sdk_ret_t
vnic_impl::restore_resources(obj_info_t *info) {
    sdk_ret_t ret;
    pds_vnic_spec_t *spec;
    pds_vnic_status_t *status;
    pds_vnic_info_t *vnic_info;

    vnic_info = (pds_vnic_info_t *)info;
    spec = &vnic_info->spec;
    status = &vnic_info->status;
    // restore an entry in the NEXTHOP table for this local vnic
    ret = nexthop_impl_db()->nh_idxr()->alloc(status->nh_hw_id);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore nexthop entry for vnic %s, "
                      "err %u, hw id %u", spec->key.str(), ret,
                      status->nh_hw_id);
        return ret;
    }
    nh_idx_ = status->nh_hw_id;

    // allocate hw id for this vnic
    ret = vnic_impl_db()->vnic_idxr()->alloc(status->hw_id);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore hw id for vnic %s, err %u,"
                      " hw id %u", spec->key.str(), ret,
                      status->hw_id);
        return ret;
    }
    hw_id_ = status->hw_id;
    return SDK_RET_OK;
}

sdk_ret_t
vnic_impl::restore(obj_info_t *info, upg_obj_info_t *upg_info) {
    sdk_ret_t ret;
    upg_obj_tlv_t *tlv;
    pds_vnic_info_t *vnic_info;
    uint32_t obj_size, meta_size;
    pds::VnicGetResponse proto_msg;

    tlv = (upg_obj_tlv_t *)upg_info->mem;
    vnic_info = (pds_vnic_info_t *)info;
    obj_size = tlv->len;
    meta_size = sizeof(upg_obj_tlv_t);
    // fill up the size, even if it fails later. to try and restore next obj
    upg_info->size = obj_size + meta_size;
    // de-serialize proto msg
    if (proto_msg.ParseFromArray(tlv->obj, tlv->len) == false) {
        PDS_TRACE_ERR("Failed to de-serialize vnic");
        return SDK_RET_OOM;
    }
    // convert proto msg to vnic info
    ret = pds_vnic_proto_to_api_info(vnic_info, &proto_msg);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to convert vnic proto msg to info, err %u", ret);
        return ret;
    }
    // now restore hw resources
    ret = restore_resources((obj_info_t *)vnic_info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to restore hw resources for vnic %s, err %u",
                      vnic_info->spec.key.str(), ret);
    }
    return ret;
}

/// \@}

}    // namespace impl
}    // namespace api
