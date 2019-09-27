//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of vpc
///
//----------------------------------------------------------------------------

#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/vpc.hpp"
#include "nic/apollo/api/impl/apulu/vpc_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/utils/utils.hpp"

namespace api {
namespace impl {

/// \defgroup PDS_VPC_IMPL - vpc entry datapath implementation
/// \ingroup PDS_VPC
/// \@{

vpc_impl *
vpc_impl::factory(pds_vpc_spec_t *spec) {
    vpc_impl *impl;

    // TODO: move to slab later
    if (spec->fabric_encap.type != PDS_ENCAP_TYPE_VXLAN) {
        PDS_TRACE_ERR("Unknown fabric encap type %u, value %u - only VxLAN "
                      "fabric encap is supported", spec->fabric_encap.type,
                      spec->fabric_encap.val);
        return NULL;
    }
    impl = (vpc_impl *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_VPC_IMPL,
                                   sizeof(vpc_impl));
    new (impl) vpc_impl();
    return impl;
}

void
vpc_impl::destroy(vpc_impl *impl) {
    impl->~vpc_impl();
    SDK_FREE(SDK_MEM_ALLOC_PDS_VPC_IMPL, impl);
}

sdk_ret_t
vpc_impl::reserve_resources(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    vni_swkey_t vni_key =  { 0 };
    sdk_table_api_params_t tparams;
    pds_vpc_spec_t *spec = &obj_ctxt->api_params->vpc_spec;

    // reserve an entry in VNI table
    vni_key.vxlan_1_vni = spec->fabric_encap.val.vnid;
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &vni_key, NULL, NULL,
                                   VNI_VNI_INFO_ID,
                                   handle_t::null());
    ret = vpc_impl_db()->vni_tbl()->reserve(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to reserve entry in VNI table for vpc %u, err %u",
                      spec->key.id, ret);
        return ret;
    }
    vni_hdl_ = tparams.handle;
    return SDK_RET_OK;
}

sdk_ret_t
vpc_impl::release_resources(api_base *api_obj) {
    sdk_table_api_params_t tparams = { 0 };

    if (vni_hdl_.valid()) {
        tparams.handle = vni_hdl_;
        vpc_impl_db()->vni_tbl()->release(&tparams);
    }
    return SDK_RET_OK;
}

sdk_ret_t
vpc_impl::nuke_resources(api_base *api_obj) {
    sdk_table_api_params_t tparams = { 0 };
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
vpc_impl::reprogram_hw(api_base *api_obj, api_op_t api_op) {
    return SDK_RET_ERR;
}

sdk_ret_t
vpc_impl::update_hw(api_base *orig_obj, api_base *curr_obj,
                    obj_ctxt_t *obj_ctxt) {
    return sdk::SDK_RET_INVALID_OP;
}

#define vni_info    action_u.vni_vni_info
#define vpc_info    action_u.vpc_vpc_info
sdk_ret_t
vpc_impl::activate_vpc_create_(pds_epoch_t epoch, vpc_entry *vpc,
                               pds_vpc_spec_t *spec) {
    sdk_ret_t ret;
    p4pd_error_t p4pd_ret;
    vni_swkey_t vni_key = { 0 };
    vni_actiondata_t vni_data = { 0 };
    vpc_actiondata_t vpc_data { 0 };
    sdk_table_api_params_t tparams = { 0 };

    PDS_TRACE_DEBUG("Activating vpc %u, type %u, fabric encap (%u, %u)",
                    spec->key.id, spec->type, spec->fabric_encap.type,
                    spec->fabric_encap.val.vnid);
    // fill the key
    vni_key.vxlan_1_vni = spec->fabric_encap.val.vnid;
    // fill the data
    vni_data.vni_info.bd_id = vpc->hw_id();    // bd hw id = vpc hw id for a vpc
    vni_data.vni_info.vpc_id = vpc->hw_id();
    PDS_IMPL_FILL_TABLE_API_PARAMS(&tparams, &vni_key, NULL, &vni_data,
                                   VNI_VNI_INFO_ID, vni_hdl_);
    // program the VNI table
    ret = vpc_impl_db()->vni_tbl()->insert(&tparams);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Programming of VNI table failed for vpc %u, err %u",
                      spec->key.id, ret);
        return ret;
    }

    // program VPC table in the egress pipe
    vpc_data.action_id = VPC_VPC_INFO_ID;
    vpc_data.vpc_info.vni = spec->fabric_encap.val.vnid;
    memcpy(vpc_data.vpc_info.vrmac, spec->vr_mac, ETH_ADDR_LEN);
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_VPC, vpc->hw_id(),
                                       NULL, NULL, &vpc_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program VPC table at index %u", vpc->hw_id());
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return ret;
}

sdk_ret_t
vpc_impl::activate_vpc_delete_(pds_epoch_t epoch, vpc_entry *vpc) {
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
vpc_impl::activate_hw(api_base *api_obj, pds_epoch_t epoch,
                      api_op_t api_op, obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_vpc_spec_t *spec;

    switch (api_op) {
    case api::API_OP_CREATE:
        spec = &obj_ctxt->api_params->vpc_spec;
        ret = activate_vpc_create_(epoch, (vpc_entry *)api_obj, spec);
        break;

    case api::API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_vpc_delete_(epoch, (vpc_entry *)api_obj);
        break;

    case api::API_OP_UPDATE:
    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

sdk_ret_t
vpc_impl::reactivate_hw(api_base *api_obj, pds_epoch_t epoch,
                        api_op_t api_op) {
    return SDK_RET_ERR;
}

sdk_ret_t
vpc_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    return SDK_RET_INVALID_OP;
}

/// \@}    // end of PDS_VPC_IMPL

}    // namespace impl
}    // namespace api
