//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of subnet
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/api/subnet.hpp"
#include "nic/apollo/api/impl/apulu/subnet_impl.hpp"
#include "nic/apollo/api/impl/apulu/vpc_impl.hpp"
#include "nic/apollo/api/impl/apulu/vnic_impl.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"

#define vni_info    action_u.vni_vni_info

#define PDS_IMPL_FILL_SUBNET_VR_IP_MAPPING_DATA(data_, nh_idx_, bd_hw_id_)     \
{                                                                              \
    memset((data_), 0, sizeof(*(data_)));                                      \
    (data_)->is_local = TRUE;                                                  \
    (data_)->nexthop_valid = TRUE;                                             \
    (data_)->nexthop_type = NEXTHOP_TYPE_NEXTHOP;                              \
    (data_)->nexthop_id = (nh_idx_);                                           \
    (data_)->egress_bd_id = (bd_hw_id_);                                       \
}

#define PDS_IMPL_FILL_P4I_BD_DATA(data_, spec_)                                \
{                                                                              \
    memset((data_), 0, sizeof(*data_));                                        \
    (data_)->action_id = P4I_BD_INGRESS_BD_INFO_ID;                            \
    sdk::lib::memrev((data_)->p4i_bd_info.vrmac,                               \
                     (spec_)->vr_mac, ETH_ADDR_LEN);                           \
}

#define PDS_IMPL_FILL_P4E_BD_DATA(data_, spec_)                                \
{                                                                              \
    memset((data_), 0, sizeof(*data_));                                        \
    (data_)->action_id = P4E_BD_EGRESS_BD_INFO_ID;                             \
    (data_)->p4e_bd_info.vni = (spec_)->fabric_encap.val.vnid;                 \
    (data_)->p4e_bd_info.tos = (spec_)->tos;                                   \
    sdk::lib::memrev((data_)->p4e_bd_info.vrmac,                               \
                     (spec_)->vr_mac, ETH_ADDR_LEN);                           \
}

namespace api {
namespace impl {

/// \defgroup PDS_SUBNET_IMPL - subnet entry datapath implementation
/// \ingroup PDS_SUBNET
/// \@{

subnet_impl *
subnet_impl::factory(pds_subnet_spec_t *spec) {
    subnet_impl *impl;
    pds_device_oper_mode_t oper_mode;

    oper_mode = g_pds_state.device_oper_mode();
    if ((oper_mode == PDS_DEV_OPER_MODE_HOST) ||
        (oper_mode == PDS_DEV_OPER_MODE_BITW_SMART_SWITCH)) {
        if (spec->fabric_encap.type != PDS_ENCAP_TYPE_VXLAN) {
            PDS_TRACE_ERR("Unknown fabric encap type %u, value %u - only VxLAN "
                          "fabric encap is supported", spec->fabric_encap.type,
                          spec->fabric_encap.val.value);
            return NULL;
        }
    } else if (oper_mode == PDS_DEV_OPER_MODE_BITW_SMART_SERVICE) {
        if (spec->fabric_encap.type != PDS_ENCAP_TYPE_NONE) {
            PDS_TRACE_ERR("Fabric encap must be PDS_ENCAP_TYPE_NONE in "
                          "BITW_SMART_SERVICE mode");
            return NULL;
        }
    }
    impl = subnet_impl_db()->alloc();
    new (impl) subnet_impl(spec);
    return impl;
}

void
subnet_impl::destroy(subnet_impl *impl) {
    impl->~subnet_impl();
    subnet_impl_db()->free(impl);
}

impl_base *
subnet_impl::clone(void) {
    subnet_impl *cloned_impl;

    cloned_impl = subnet_impl_db()->alloc();
    new (cloned_impl) subnet_impl();
    // NOTE: we don't need to copy the resources at this time, they will be
    //       transferred during the update process
    cloned_impl->key_ = key_;
    return cloned_impl;
}

sdk_ret_t
subnet_impl::free(subnet_impl *impl) {
    destroy(impl);
    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::reserve_vni_entry_(pds_subnet_spec_t *spec) {
    sdk_ret_t ret;
    vni_swkey_t vni_key =  { 0 };
    sdk_table_api_params_t tparams;

    vni_key.vxlan_1_vni = spec->fabric_encap.val.vnid;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &vni_key, NULL, NULL,
                                   VNI_VNI_INFO_ID, handle_t::null());
    ret = vpc_impl_db()->vni_tbl()->reserve(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to reserve entry in VNI table for subnet %s, "
                      "vnid %u, err %u", spec->key.str(),
                      spec->fabric_encap.val.vnid, ret);
        return ret;
    }
    vni_hdl_ = tparams.handle;
    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::reserve_vr_ip_resources_(vpc_impl *vpc, ip_addr_t *vr_ip,
                                      pds_subnet_spec_t *spec) {
    sdk_ret_t ret;
    mapping_swkey_t mapping_key;
    sdk_table_api_params_t tparams;

    PDS_IMPL_FILL_IP_MAPPING_SWKEY(&mapping_key, vpc->hw_id(), vr_ip);
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key,
                                   NULL, NULL, 0, handle_t::null());
    ret = mapping_impl_db()->mapping_tbl()->reserve(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to reserve entry in MAPPING table for "
                      "subnet %s IPv4 VR IP %s", spec->key.str(),
                      ipaddr2str(vr_ip));
        return ret;
    }
    if (vr_ip->af == IP_AF_IPV4) {
        v4_vr_ip_mapping_hdl_ = tparams.handle;
    } else {
        v6_vr_ip_mapping_hdl_ = tparams.handle;
    }
    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::reserve_resources(api_base *api_obj, api_base *orig_obj,
                               api_obj_ctxt_t *obj_ctxt) {
    uint32_t idx;
    sdk_ret_t ret;
    vpc_entry *vpc;
    ip_addr_t vr_ip;
    pds_subnet_spec_t *spec = &obj_ctxt->api_params->subnet_spec;

    // find the VPC of this subnet
    vpc = vpc_find(&spec->vpc);
    if (vpc == NULL) {
        PDS_TRACE_ERR("No vpc %s found for subnet %s",
                      spec->vpc.str(), spec->key.str());
        return SDK_RET_INVALID_ARG;
    }

    switch (obj_ctxt->api_op) {
    case API_OP_CREATE:
        // record the fact that resource reservation was attempted
        // NOTE: even if we partially acquire resources and fail eventually,
        //       this will ensure that proper release of resources will happen
        api_obj->set_rsvd_rsc();
        // reserve a hw id for this subnet
        ret = subnet_impl_db()->subnet_idxr()->alloc(&idx);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to allocate hw id for subnet %s, err %u",
                          spec->key.str(), ret);
            return ret;
        }
        hw_id_ = idx;

        // reserve an entry in VNI table
        if (spec->fabric_encap.type != PDS_ENCAP_TYPE_NONE) {
            ret = reserve_vni_entry_(spec);
            if (unlikely(ret != SDK_RET_OK)) {
                return ret;
            }
        }

        // reserve an entry in the MAPPING table for the IPv4 VR IP, if needed
        if (spec->v4_vr_ip) {
            memset(&vr_ip, 0, sizeof(vr_ip));
            vr_ip.af = IP_AF_IPV4;
            vr_ip.addr.v4_addr = spec->v4_vr_ip;
            ret = reserve_vr_ip_resources_((vpc_impl *)vpc->impl(),
                                           &vr_ip, spec);
            if (unlikely(ret != SDK_RET_OK)) {
                return ret;
            }
        }

        // reserve an entry in the MAPPING table for the IPv6 VR IP, if needed
        if (!ip_addr_is_zero(&spec->v6_vr_ip)) {
            ret = reserve_vr_ip_resources_((vpc_impl *)vpc->impl(),
                                           &spec->v6_vr_ip, spec);
            if (unlikely(ret != SDK_RET_OK)) {
                return ret;
            }
        }
        break;

    case API_OP_UPDATE:
        // record the fact that resource reservation was attempted
        // NOTE: even if we partially acquire resources and fail eventually,
        //       this will ensure that proper release of resources will happen
        api_obj->set_rsvd_rsc();
        // if vnid is updated, reserve a handle for it in VNI table
        if ((obj_ctxt->upd_bmap & PDS_SUBNET_UPD_FABRIC_ENCAP) &&
            (spec->fabric_encap.type != PDS_ENCAP_TYPE_NONE)) {
            ret = reserve_vni_entry_(spec);
            if (unlikely(ret != SDK_RET_OK)) {
                return ret;
            }
        }

        // reserve resources needed for IPv4 VR IP, if that is updated
        if ((obj_ctxt->upd_bmap & PDS_SUBNET_UPD_V4_VR_IP) && spec->v4_vr_ip) {
            memset(&vr_ip, 0, sizeof(vr_ip));
            vr_ip.af = IP_AF_IPV4;
            vr_ip.addr.v4_addr = spec->v4_vr_ip;
            ret = reserve_vr_ip_resources_((vpc_impl *)vpc->impl(),
                                           &vr_ip, spec);
            if (unlikely(ret != SDK_RET_OK)) {
                return ret;
            }
        }

        // reserve resources needed for IPv6 VR IP, if that is updated
        if ((obj_ctxt->upd_bmap & PDS_SUBNET_UPD_V6_VR_IP) &&
            !ip_addr_is_zero(&spec->v6_vr_ip)) {
            ret = reserve_vr_ip_resources_((vpc_impl *)vpc->impl(),
                                           &spec->v6_vr_ip, spec);
            if (unlikely(ret != SDK_RET_OK)) {
                return ret;
            }
        }
        break;

    default:
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::release_resources(api_base *api_obj) {
    sdk_table_api_params_t tparams = { 0 };

    if (hw_id_ != 0xFFFF) {
        subnet_impl_db()->subnet_idxr()->free(hw_id_);
    }

    if (vni_hdl_.valid()) {
        tparams.handle = vni_hdl_;
        vpc_impl_db()->vni_tbl()->release(&tparams);
    }

    if (v4_vr_ip_mapping_hdl_.valid()) {
        tparams.handle = v4_vr_ip_mapping_hdl_;
        mapping_impl_db()->mapping_tbl()->release(&tparams);
    }

    if (v6_vr_ip_mapping_hdl_.valid()) {
        tparams.handle = v6_vr_ip_mapping_hdl_;
        mapping_impl_db()->mapping_tbl()->release(&tparams);
    }
    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::nuke_resources(api_base *api_obj) {
    sdk_ret_t ret;
    vpc_entry *vpc;
    ip_addr_t vr_ip;
    pds_obj_key_t vpc_key;
    mapping_swkey_t mapping_key;
    vni_swkey_t vni_key =  { 0 };
    sdk_table_api_params_t tparams = { 0 };
    subnet_entry *subnet = (subnet_entry *)api_obj;

    if (hw_id_ != 0xFFFF) {
        subnet_impl_db()->subnet_idxr()->free(hw_id_);
    }

    if (vni_hdl_.valid()) {
        vni_key.vxlan_1_vni = subnet->fabric_encap().val.vnid;
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &vni_key, NULL, NULL,
                                       VNI_VNI_INFO_ID, vni_hdl_);
        ret = vpc_impl_db()->vni_tbl()->remove(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to remove VNI table entry for subnet %s, "
                          "vni %u", subnet->key().str(), vni_key.vxlan_1_vni);
        }
    }

    // find the VPC of this subnet
    vpc_key = subnet->vpc();
    vpc = vpc_find(&vpc_key);
    if (vpc == NULL) {
        PDS_TRACE_ERR("No vpc %s found for subnet %s",
                      vpc_key.str(), subnet->key().str());
        return SDK_RET_INVALID_ARG;
    }

    // free up IPv4 VR IP entry
    if (v4_vr_ip_mapping_hdl_.valid()) {
        memset(&vr_ip, 0, sizeof(vr_ip));
        vr_ip.af = IP_AF_IPV4;
        vr_ip.addr.v4_addr = subnet->v4_vr_ip();
        PDS_IMPL_FILL_IP_MAPPING_SWKEY(&mapping_key,
                                       ((vpc_impl *)vpc->impl())->hw_id(),
                                       &vr_ip);
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL, NULL,
                                       0, handle_t::null());
        ret = mapping_impl_db()->mapping_tbl()->remove(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to remove entry in MAPPING table for "
                          "subnet %s IPv4 VR IP %s", subnet->key().str(),
                          ipv4addr2str(subnet->v4_vr_ip()));
            return ret;
        }
    }

    // free up IPv6 VR IP entry
    if (v6_vr_ip_mapping_hdl_.valid()) {
        vr_ip = subnet->v6_vr_ip();
        PDS_IMPL_FILL_IP_MAPPING_SWKEY(&mapping_key,
                                       ((vpc_impl *)vpc->impl())->hw_id(),
                                       &vr_ip);
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL, NULL,
                                       0, handle_t::null());
        ret = mapping_impl_db()->mapping_tbl()->remove(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to remove entry in MAPPING table for "
                          "subnet %s IPv6 VR IP %s", subnet->key().str(),
                          ipaddr2str(&vr_ip));
            return ret;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::populate_msg(pds_msg_t *msg, api_base *api_obj,
                          api_obj_ctxt_t *obj_ctxt) {
    msg->cfg_msg.subnet.status.hw_id = hw_id_;
    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::add_vr_ip_mapping_entries_(uint16_t bd_hw_id,
                                        pds_subnet_spec_t *spec) {
    sdk_ret_t ret;
    lif_impl *lif;
    vpc_entry *vpc;
    ip_addr_t vr_ip;
    mapping_swkey_t mapping_key;
    mapping_appdata_t mapping_data;
    sdk_table_api_params_t tparams;

    vpc = vpc_find(&spec->vpc);
    lif = lif_impl_db()->find(sdk::platform::LIF_TYPE_MNIC_CPU);

    // fill the MAPPING table entry data
    PDS_IMPL_FILL_SUBNET_VR_IP_MAPPING_DATA(&mapping_data,
                                            lif->nh_idx(), bd_hw_id);
    // program the IPv4 VR IP in the MAPPING table
    if (v4_vr_ip_mapping_hdl_.valid()) {
        // fill the IPv4 key
        memset(&vr_ip, 0, sizeof(vr_ip));
        vr_ip.af = IP_AF_IPV4;
        vr_ip.addr.v4_addr = spec->v4_vr_ip;
        PDS_IMPL_FILL_IP_MAPPING_SWKEY(&mapping_key,
                                       ((vpc_impl *)vpc->impl())->hw_id(),
                                       &vr_ip);
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL,
                                       &mapping_data, MAPPING_MAPPING_INFO_ID,
                                       v4_vr_ip_mapping_hdl_);
        ret = mapping_impl_db()->mapping_tbl()->insert(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program MAPPING table entry for "
                          "VR IP %s of subnet %s",
                          ipv4addr2str(spec->v4_vr_ip), spec->key.str());
            return ret;
        }
    }

    // program the IPv6 VR IP in the MAPPING table
    if (v6_vr_ip_mapping_hdl_.valid()) {
        // fill the IPv6 key
        PDS_IMPL_FILL_IP_MAPPING_SWKEY(&mapping_key,
                                       ((vpc_impl *)vpc->impl())->hw_id(),
                                       &spec->v6_vr_ip);
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL,
                                       &mapping_data,
                                       MAPPING_MAPPING_INFO_ID,
                                       v6_vr_ip_mapping_hdl_);
        ret = mapping_impl_db()->mapping_tbl()->insert(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program MAPPING table entry for "
                          "VR IP %s of subnet %s",
                          ipaddr2str(&spec->v6_vr_ip), spec->key.str());
            return ret;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::program_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    p4pd_error_t p4pd_ret;
    p4i_bd_actiondata_t p4i_bd_data;
    p4e_bd_actiondata_t p4e_bd_data;
    pds_subnet_spec_t *spec = &obj_ctxt->api_params->subnet_spec;

    // install MAPPING table entries for VR IPs pointing to datapath lif
    if (v4_vr_ip_mapping_hdl_.valid() || v6_vr_ip_mapping_hdl_.valid()) {
        ret = add_vr_ip_mapping_entries_(hw_id_, spec);
        if (unlikely(ret != SDK_RET_OK)) {
            return ret;
        }
    }

    // program BD table in the ingress pipe
    PDS_IMPL_FILL_P4I_BD_DATA(&p4i_bd_data, spec);
    PDS_TRACE_VERBOSE("Programming P4I_BD table at %u", hw_id_);
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4I_BD, hw_id_,
                                       NULL, NULL, &p4i_bd_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program P4I_BD table at index %u", hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // program BD table in the egress pipe
    PDS_IMPL_FILL_P4E_BD_DATA(&p4e_bd_data, spec);
    PDS_TRACE_VERBOSE("Programming P4E_BD table at %u with vni %u for subnet %s",
                      hw_id_, p4e_bd_data.p4e_bd_info.vni, spec->key.str());
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4E_BD, hw_id_,
                                       NULL, NULL, &p4e_bd_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program BD table at index %u", hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::cleanup_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    vpc_entry *vpc;
    ip_addr_t vr_ip;
    pds_obj_key_t vpc_key;
    p4pd_error_t p4pd_ret;
    mapping_swkey_t mapping_key;
    sdk_table_api_params_t tparams;
    p4i_bd_actiondata_t p4i_bd_data = { 0 };
    p4e_bd_actiondata_t p4e_bd_data = { 0 };
    mapping_appdata_t mapping_data;
    subnet_entry *subnet = (subnet_entry *)api_obj;

    // install MAPPING table entries for VR IPs pointing to datapath lif
    if (v4_vr_ip_mapping_hdl_.valid() || v6_vr_ip_mapping_hdl_.valid()) {
        vpc_key = subnet->vpc();
        vpc = vpc_find(&vpc_key);
        // fill the MAPPING table entry data
        memset(&mapping_data, 0, sizeof(mapping_data));
        mapping_data.nexthop_valid = FALSE;
        mapping_data.nexthop_type = NEXTHOP_TYPE_NEXTHOP;
        mapping_data.nexthop_id = PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID;
        mapping_data.egress_bd_id = 0;
        if (v4_vr_ip_mapping_hdl_.valid()) {
            // fill the IPv4 key
            memset(&vr_ip, 0, sizeof(vr_ip));
            vr_ip.af = IP_AF_IPV4;
            vr_ip.addr.v4_addr = subnet->v4_vr_ip();
            PDS_IMPL_FILL_IP_MAPPING_SWKEY(&mapping_key,
                                           ((vpc_impl *)vpc->impl())->hw_id(),
                                           &vr_ip);
            PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL,
                                           &mapping_data,
                                           MAPPING_MAPPING_INFO_ID,
                                           sdk::table::handle_t::null());
            ret = mapping_impl_db()->mapping_tbl()->update(&tparams);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to cleanup entry in MAPPING table for "
                              "subnet %s IPv4 VR IP %s", subnet->key().str(),
                              ipaddr2str(&vr_ip));
                return ret;
            }
        }
        if (v6_vr_ip_mapping_hdl_.valid()) {
            vr_ip = subnet->v6_vr_ip();
            PDS_IMPL_FILL_IP_MAPPING_SWKEY(&mapping_key,
                                           ((vpc_impl *)vpc->impl())->hw_id(),
                                           &vr_ip);
            PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL,
                                           &mapping_data,
                                           MAPPING_MAPPING_INFO_ID,
                                           sdk::table::handle_t::null());
            ret = mapping_impl_db()->mapping_tbl()->update(&tparams);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to cleanup entry in MAPPING table for "
                              "subnet %s IPv6 VR IP %s", subnet->key().str(),
                              ipaddr2str(&vr_ip));
                return ret;
            }
        }
    }

    // program BD table in the ingress pipe
    p4i_bd_data.action_id = P4I_BD_INGRESS_BD_INFO_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4I_BD, hw_id_,
                                       NULL, NULL, &p4i_bd_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to cleanup P4I_BD table at index %u", hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // program BD table in the egress pipe
    p4e_bd_data.action_id = P4E_BD_EGRESS_BD_INFO_ID;
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4E_BD, hw_id_,
                                       NULL, NULL, &p4e_bd_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to cleanup P4E_BD table at index %u", hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::update_hw(api_base *orig_obj, api_base *curr_obj,
                       api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    p4pd_error_t p4pd_ret;
    subnet_impl *orig_impl;
    pds_subnet_spec_t *spec;
    p4i_bd_actiondata_t p4i_bd_data;
    p4e_bd_actiondata_t p4e_bd_data;

    spec = &obj_ctxt->api_params->subnet_spec;
    orig_impl = (subnet_impl *)(((subnet_entry *)orig_obj)->impl());

    // install MAPPING table entries for VR IPs pointing to datapath lif
    if (v4_vr_ip_mapping_hdl_.valid() || v6_vr_ip_mapping_hdl_.valid()) {
        ret = add_vr_ip_mapping_entries_(orig_impl->hw_id(), spec);
        if (unlikely(ret != SDK_RET_OK)) {
            return ret;
        }
    }

    // program BD table in the ingress pipe
    PDS_IMPL_FILL_P4I_BD_DATA(&p4i_bd_data, spec);
    PDS_TRACE_VERBOSE("Programming P4I_BD table at %u", orig_impl->hw_id_);
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4I_BD, orig_impl->hw_id_,
                                       NULL, NULL, &p4i_bd_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program P4I_BD table at index %u",
                      orig_impl->hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    // program BD table in the egress pipe
    PDS_IMPL_FILL_P4E_BD_DATA(&p4e_bd_data, spec);
    PDS_TRACE_VERBOSE("Programming P4E_BD table at %u with vni %u for subnet %s",
                      orig_impl->hw_id_, p4e_bd_data.p4e_bd_info.vni,
                      spec->key.str());
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4E_BD, orig_impl->hw_id_,
                                       NULL, NULL, &p4e_bd_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program P4E_BD table at index %u",
                      orig_impl->hw_id_);
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }

    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::activate_create_(pds_epoch_t epoch, subnet_entry *subnet,
                              pds_subnet_spec_t *spec) {
    lif_impl *lif;
    vpc_entry *vpc;
    device_entry *device;
    sdk_ret_t ret = SDK_RET_OK;
    vni_swkey_t vni_key = { 0 };
    sdk_table_api_params_t tparams;
    vni_actiondata_t vni_data = { 0 };

    PDS_TRACE_DEBUG("Activating subnet %s, hw id %u, create, num host if %u, "
                    "host if %s", spec->key.str(), hw_id_, spec->num_host_if,
                    spec->host_if[0].str());
    vpc = vpc_find(&spec->vpc);
    if (vpc == NULL) {
        PDS_TRACE_ERR("No vpc %s found to program subnet %s",
                      spec->vpc.str(), spec->key.str());
        return SDK_RET_INVALID_ARG;
    }

    if (vni_hdl_.valid()) {
        // fill the key
        vni_key.vxlan_1_vni = spec->fabric_encap.val.vnid;
        // fill the data
        vni_data.action_id = VNI_VNI_INFO_ID;
        vni_data.vni_info.bd_id = hw_id_;
        vni_data.vni_info.vpc_id = ((vpc_impl *)vpc->impl())->hw_id();
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &vni_key, NULL, &vni_data,
                                       VNI_VNI_INFO_ID, vni_hdl_);
        // program the VNI table
        ret = vpc_impl_db()->vni_tbl()->insert(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Programming of VNI table failed for subnet %s, err %u",
                          spec->key.str(), ret);
            return ret;
        }
    }

    // if the subnet is enabled on host interface, update the lif table with
    // subnet and vpc ids appropriately
    if (spec->num_host_if) {
        for (uint8_t i = 0; i < spec->num_host_if; i++) {
            lif = lif_impl_db()->find(&spec->host_if[i]);
            ret = program_lif_table(lif->id(), P4_LIF_TYPE_HOST,
                                    ((vpc_impl *)vpc->impl())->hw_id(),
                                    hw_id_, lif->vnic_hw_id(),
                                    subnet->vr_mac(),
                                    device_find()->learning_enabled(), false,
                                    P4_LIF_DIR_HOST);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to update lif# %u, %s on subnet %s "
                              "create, err %u", i, spec->host_if[i].str(),
                              spec->key.str(), ret);
            }
        }
    }
    subnet_impl_db()->insert(hw_id_, this);
    return ret;
}

sdk_ret_t
subnet_impl::activate_delete_(pds_epoch_t epoch, subnet_entry *subnet) {
    sdk_ret_t ret = SDK_RET_OK;
    vni_swkey_t vni_key = { 0 };
    vni_actiondata_t vni_data = { 0 };
    sdk_table_api_params_t tparams = { 0 };
    lif_impl *lif;
    pds_obj_key_t host_if;

    PDS_TRACE_DEBUG("Activating subnet %s delete, fabric encap (%u, %u)",
                    subnet->key().str(), subnet->fabric_encap().type,
                    subnet->fabric_encap().val.vnid);

    if (vni_hdl_.valid()) {
        // fill the key
        vni_key.vxlan_1_vni = subnet->fabric_encap().val.vnid;
        // fill the data
        vni_data.vni_info.bd_id = PDS_IMPL_RSVD_BD_HW_ID;
        vni_data.vni_info.vpc_id = PDS_IMPL_RSVD_VPC_HW_ID;
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &vni_key, NULL, &vni_data,
                                       VNI_VNI_INFO_ID,
                                       sdk::table::handle_t::null());
        // program the VNI table
        ret = vpc_impl_db()->vni_tbl()->update(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Programming of VNI table failed for subnet %s, err %u",
                          subnet->key().str(), ret);
        }
    }
    // reset the lif entry
    for (uint8_t i = 0; i < subnet->num_host_if(); i++) {
        host_if = subnet->host_if(i);
        lif = lif_impl_db()->find(&host_if);
        if (lif) {
            ret = program_lif_table(lif->id(), P4_LIF_TYPE_HOST,
                                    PDS_IMPL_RSVD_VPC_HW_ID,
                                    PDS_IMPL_RSVD_BD_HW_ID,
                                    lif->vnic_hw_id(), g_zero_mac,
                                    false, false, P4_LIF_DIR_HOST);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to update lif %s on subnet %s"
                              " delete, err %u", host_if.str(),
                              subnet->key().str(), ret);
            } else {
                PDS_TRACE_VERBOSE("Updated lif %s on subnet %s delete",
                                  host_if.str(), subnet->key().str());
            }
        }
    }
    subnet_impl_db()->remove(hw_id_);
    return ret;
}

sdk_ret_t
subnet_impl::activate_update_(pds_epoch_t epoch, subnet_entry *subnet,
                              subnet_entry *orig_subnet,
                              api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    lif_impl *lif;
    vpc_entry *vpc;
    pds_obj_key_t lif_key;
    pds_subnet_spec_t *spec;
    vni_swkey_t vni_key = { 0 };
    sdk_table_api_params_t tparams;
    vni_actiondata_t vni_data = { 0 };
    subnet_impl *orig_impl = (subnet_impl *)orig_subnet->impl();

    spec = &obj_ctxt->api_params->subnet_spec;
    PDS_TRACE_DEBUG("Activating subnet %s, update host if %s, upd bmap 0x%lx",
                    spec->key.str(), spec->host_if[0].str(),
                    obj_ctxt->upd_bmap);
    vpc = vpc_find(&spec->vpc);
    if (vpc == NULL) {
        PDS_TRACE_ERR("No vpc %s found to program subnet %s",
                      spec->vpc.str(), spec->key.str());
        return SDK_RET_INVALID_ARG;
    }

    // xfer resources from original object to the cloned object
    hw_id_ = orig_impl->hw_id_;

    if (obj_ctxt->upd_bmap & PDS_SUBNET_UPD_FABRIC_ENCAP) {
        // fill the key
        vni_key.vxlan_1_vni = spec->fabric_encap.val.vnid;
        // fill the data
        vni_data.action_id = VNI_VNI_INFO_ID;
        vni_data.vni_info.bd_id = hw_id_;
        vni_data.vni_info.vpc_id = ((vpc_impl *)vpc->impl())->hw_id();
        PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &vni_key, NULL, &vni_data,
                                       VNI_VNI_INFO_ID, vni_hdl_);
        // insert new entry in the VNI table
        ret = vpc_impl_db()->vni_tbl()->insert(&tparams);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("New VNI table entry insert failed, "
                          "subnet %s, err %u", spec->key.str(), ret);
            return ret;
        }
    } else {
        // transfer the resource
        vni_hdl_ = orig_impl->vni_hdl_;
    }

    if (obj_ctxt->upd_bmap & PDS_SUBNET_UPD_HOST_IFINDEX) {
        // NOTE: because of the sequence of operations below it is possible that
        //       a subnet and lif association that is not touched by the user
        //       can be cleaned up and get re-established, so traffic from other
        //       lifs that were there in old config and in new config may see
        //       momentary drop !!! we can optimize this later by doing mxn walk
        // cleanup previous lif to this subnet association
        for (uint8_t i = 0; i < orig_subnet->num_host_if(); i++) {
            lif_key = orig_subnet->host_if(i);
            lif = lif_impl_db()->find(&lif_key);
            ret = program_lif_table(lif->id(), P4_LIF_TYPE_HOST,
                                    PDS_IMPL_RSVD_VPC_HW_ID,
                                    PDS_IMPL_RSVD_BD_HW_ID, lif->vnic_hw_id(),
                                    g_zero_mac, false, false, P4_LIF_DIR_HOST);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to reset lif %s on subnet %s update, "
                              "err %u", lif_key.str(), spec->key.str(), ret);
                return ret;
            }
        }
        // reprogram new associations
        for (uint8_t i = 0; i < spec->num_host_if; i++) {
            lif = lif_impl_db()->find(&spec->host_if[i]);
            ret = program_lif_table(lif->id(), P4_LIF_TYPE_HOST,
                                    ((vpc_impl *)vpc->impl())->hw_id(),
                                    hw_id_, lif->vnic_hw_id(),
                                    subnet->vr_mac(),
                                    device_find()->learning_enabled(), false,
                                    P4_LIF_DIR_HOST);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to update lif %s table entry on "
                              "subnet %s update, err %u",
                              spec->host_if[i].str(), spec->key.str(), ret);
                return ret;
            }
        }
    }
    // delete old impl and insert cloned impl into ht
    ret = subnet_impl_db()->update(hw_id_, this);
    SDK_ASSERT_RETURN((ret == SDK_RET_OK), ret);

    // relinquish the ownership of resources from old object
    orig_impl->hw_id_ = 0xFFFF;
    if (!(obj_ctxt->upd_bmap & PDS_SUBNET_UPD_FABRIC_ENCAP)) {
        // take over the existing VNI entry as there is no vnid change
        orig_impl->vni_hdl_ = handle_t::null();
    }
    if (!(obj_ctxt->upd_bmap & PDS_SUBNET_UPD_V4_VR_IP)) {
        // take over the existing V4 VR IP entry as there is no change
        v4_vr_ip_mapping_hdl_ = orig_impl->v4_vr_ip_mapping_hdl_;
        orig_impl->v4_vr_ip_mapping_hdl_ = handle_t::null();
    }
    if (!(obj_ctxt->upd_bmap & PDS_SUBNET_UPD_V6_VR_IP)) {
        // take over the existing V6 VR IP entry as there is no change
        v6_vr_ip_mapping_hdl_ = orig_impl->v6_vr_ip_mapping_hdl_;
        orig_impl->v6_vr_ip_mapping_hdl_ = handle_t::null();
    }

    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::activate_hw(api_base *api_obj, api_base *orig_obj,
                         pds_epoch_t epoch, api_op_t api_op,
                         api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_subnet_spec_t *spec;

    switch (api_op) {
    case API_OP_CREATE:
        spec = &obj_ctxt->api_params->subnet_spec;
        ret = activate_create_(epoch, (subnet_entry *)api_obj, spec);
        break;

    case API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (subnet_entry *)api_obj);
        break;

    case API_OP_UPDATE:
        ret = activate_update_(epoch, (subnet_entry *)api_obj,
                               (subnet_entry *)orig_obj, obj_ctxt);
        break;

    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

static bool
local_mapping_dhcp_binding_upd_cb_ (sdk_table_api_params_t *params)
{
    sdk_ret_t ret;
    vpc_entry *vpc;
    vnic_impl *vnic_impl;
    subnet_entry *subnet;
    ip_addr_t mapping_ip;
    pds_obj_key_t vpc_key;
    uint16_t vpc_hw_id, bd_id;
    local_mapping_swkey_t *key;
    mapping_swkey_t mapping_key;
    subnet_impl *subnet_impl_obj;
    pds_mapping_spec_t spec = {0};
    local_mapping_appdata_t *data;
    mapping_appdata_t mapping_data;
    sdk_table_api_params_t tparams;

    subnet = (subnet_entry *)api_framework_obj((api_base *)(params->cbdata));
    SDK_ASSERT(subnet != NULL);
    bd_id = ((subnet_impl *)subnet->impl())->hw_id();
    key = (local_mapping_swkey_t *)(params->key);
    data = (local_mapping_appdata_t *)(params->appdata);

    vpc_key = subnet->vpc();
    vpc = vpc_find(&vpc_key);
    vpc_hw_id = ((vpc_impl *)vpc->impl())->hw_id();

    if (key->key_metadata_local_mapping_lkp_type != KEY_TYPE_IPV4) {
        // not interested in non-IPv4 entries
        return false;
    }
    if (key->key_metadata_local_mapping_lkp_id != vpc_hw_id) {
        // this mapping doesn't belong to this vpc
        return false;
    }
    data = (local_mapping_appdata_t *)(params->appdata);
    if (data->ip_type != MAPPING_TYPE_OVERLAY) {
        // not interested in public IPs
        return false;
    }

    // get the subnet of this local overlay mapping entry
    vnic_impl = vnic_impl_db()->find(data->vnic_id);
    SDK_ASSERT(vnic_impl != NULL);
    memset(&mapping_ip, 0, sizeof(mapping_ip));
    mapping_ip.af = IP_AF_IPV4;
    mapping_ip.addr.v4_addr = *(uint32_t *)key->key_metadata_local_mapping_lkp_addr;
    PDS_IMPL_FILL_IP_MAPPING_SWKEY(&mapping_key, vpc_hw_id, &mapping_ip);
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &mapping_key, NULL, &mapping_data,
                                   0, handle_t::null());
    ret = mapping_impl_db()->mapping_tbl()->get(&tparams);
    SDK_ASSERT(ret == SDK_RET_OK);

    // and check if this is subnet of interest
    if (mapping_data.egress_bd_id != bd_id) {
        return false;
    }

    spec.skey.type = PDS_MAPPING_TYPE_L3;
    spec.skey.vpc = subnet->vpc();
    spec.skey.ip_addr.af = IP_AF_IPV4;
    spec.skey.ip_addr.addr.v4_addr = mapping_ip.addr.v4_addr;
    spec.subnet = subnet->key();
    spec.vnic = *(vnic_impl->key());
    sdk::lib::memrev((uint8_t *)&spec.overlay_mac, mapping_data.dmaci, sizeof(mac_addr_t));

    mapping_impl_db()->remove_dhcp_binding(&spec.skey);
    mapping_impl_db()->insert_dhcp_binding(&spec);

    return false;
}

sdk_ret_t
subnet_impl::reactivate_hw(api_base *api_obj, pds_epoch_t epoch,
                           api_obj_ctxt_t *obj_ctxt) {
    sdk_table_api_params_t api_params = { 0 };

    if (obj_ctxt->upd_bmap & PDS_SUBNET_UPD_DHCP_PROXY_POLICY) {
        api_params.itercb = local_mapping_dhcp_binding_upd_cb_;
        api_params.cbdata = api_obj;
        mapping_impl_db()->local_mapping_tbl()->iterate(&api_params);
    }
    return SDK_RET_OK;
}

void
subnet_impl::fill_status_(pds_subnet_status_t *status) {
    status->hw_id = hw_id_;
}

sdk_ret_t
subnet_impl::fill_spec_(pds_subnet_spec_t *spec) {
    sdk_ret_t ret;
    p4pd_error_t p4pd_ret;
    vni_actiondata_t vni_data;
    p4e_bd_actiondata_t bd_data;
    vni_swkey_t vni_key = { 0 };
    sdk_table_api_params_t tparams;

    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_P4E_BD, hw_id_,
                                      NULL, NULL, &bd_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to read BD table at index %u", hw_id_);
        return sdk::SDK_RET_HW_READ_ERR;
    }
    spec->fabric_encap.val.vnid = bd_data.p4e_bd_info.vni;
    spec->fabric_encap.type = PDS_ENCAP_TYPE_VXLAN;
    sdk::lib::memrev(spec->vr_mac, bd_data.p4e_bd_info.vrmac, ETH_ADDR_LEN);
    spec->tos = bd_data.p4e_bd_info.tos;
    vni_key.vxlan_1_vni = spec->fabric_encap.val.vnid;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &vni_key, NULL, &vni_data,
                                   VNI_VNI_INFO_ID, handle_t::null());
    // read the VNI table
    ret = vpc_impl_db()->vni_tbl()->get(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to read VNI table for vnid %u, err %u",
                      bd_data.p4e_bd_info.vni, ret);
        return sdk::SDK_RET_HW_READ_ERR;
    }
    return SDK_RET_OK;
}

sdk_ret_t
subnet_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    sdk_ret_t ret;
    pds_subnet_info_t *subnet_info = (pds_subnet_info_t *)info;

    if ((ret = fill_spec_(&subnet_info->spec)) != SDK_RET_OK) {
        return ret;
    }

    fill_status_(&subnet_info->status);
    return SDK_RET_OK;
}

/// \@}    // end of PDS_SUBNET_IMPL

}    // namespace impl
}    // namespace api
