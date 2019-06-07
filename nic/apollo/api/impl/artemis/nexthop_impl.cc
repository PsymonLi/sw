//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of nexthop
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/nexthop.hpp"
#include "nic/apollo/api/impl/artemis/nexthop_impl.hpp"
#include "nic/apollo/api/impl/artemis/pds_impl_state.hpp"
#include "nic/apollo/api/pds_state.hpp"

namespace api {
namespace impl {

/// \defgroup PDS_NEXTHOP_IMPL - nexthop datapath implementation
/// \ingroup PDS_NEXTHOP
/// \@{

nexthop_impl *
nexthop_impl::factory(pds_nexthop_spec_t *spec) {
    nexthop_impl *impl;

    // TODO: move to slab later
    impl = (nexthop_impl *)SDK_CALLOC(SDK_MEM_ALLOC_PDS_NEXTHOP_IMPL,
                                   sizeof(nexthop_impl));
    new (impl) nexthop_impl();
    return impl;
}

void
nexthop_impl::destroy(nexthop_impl *impl) {
    impl->~nexthop_impl();
    SDK_FREE(SDK_MEM_ALLOC_PDS_NEXTHOP_IMPL, impl);
}

sdk_ret_t
nexthop_impl::reserve_resources(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_nexthop_spec_t *spec;

    spec = &obj_ctxt->api_params->nexthop_spec;
    // for blackhole nexthop we can use PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID
    if (spec->type != PDS_NH_TYPE_BLACKHOLE) {
        // reserve an entry in NEXTHOP table
        ret = nexthop_impl_db()->nh_tbl()->reserve(&hw_id_);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to reserve entry in nexthop table, err %u", ret);
            return ret;
        }
    } else {
        hw_id_ = PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID;
    }
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_impl::release_resources(api_base *api_obj) {
    if ((hw_id_ != PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID) &&
        (hw_id_ != 0xFFFFFFFF)) {
        return nexthop_impl_db()->nh_tbl()->release(hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_impl::nuke_resources(api_base *api_obj) {
    if ((hw_id_ != PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID) &&
        (hw_id_ != 0xFFFFFFFF)) {
        return nexthop_impl_db()->nh_tbl()->remove(hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_impl::program_hw(api_base *api_obj, obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_nexthop_spec_t *spec;
    nexthop_actiondata_t nh_data = { 0 };

    spec = &obj_ctxt->api_params->nexthop_spec;
    nh_data.action_id = NEXTHOP_NEXTHOP_INFO_ID;

    switch (spec->type) {
    case PDS_NH_TYPE_BLACKHOLE:
        // nothing to program for system-wide blackhole nexthop
        break;

    case PDS_NH_TYPE_IP:
        // DIPo, DMACo are no-op for this native IP type nexthop
        nh_data.action_u.nexthop_nexthop_info.vni = spec->vlan;
        if (is_mac_set(spec->mac)) {
            sdk::lib::memrev(nh_data.action_u.nexthop_nexthop_info.dmaci,
                             spec->mac, ETH_ADDR_LEN);
        } else {
            // we should do nexthop IP based lookup in the neighbour db
            // and get the MAC, if mac is not provided
            // TODO: currently MAC must be specified in all cases
            SDK_ASSERT(0);
        }
        break;
    default:
        ret = SDK_RET_INVALID_ARG;
        PDS_TRACE_ERR("Failed to program NEXTHOP table at %u, err %u",
                      hw_id_, ret);
        goto error;
        break;
    }
    ret = nexthop_impl_db()->nh_tbl()->insert_atid(&nh_data, hw_id_);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to program NEXTHOP table at %u, err %u",
                      hw_id_, ret);
        goto error;
    }
    return SDK_RET_OK;

error:
    return ret;
}

sdk_ret_t
nexthop_impl::reprogram_hw(api_base *api_obj, api_op_t api_op) {
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
nexthop_impl::cleanup_hw(api_base *api_obj, obj_ctxt_t *obj_ctxt) {
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_impl::update_hw(api_base *orig_obj, api_base *curr_obj,
                     obj_ctxt_t *obj_ctxt) {
    return sdk::SDK_RET_INVALID_OP;
}

sdk_ret_t
nexthop_impl::reactivate_hw(api_base *api_obj, pds_epoch_t epoch,
                            api_op_t api_op) {
    return SDK_RET_ERR;
}

void
nexthop_impl::fill_status_(pds_nexthop_status_t *status) {
    status->hw_id = hw_id_;
}

void
nexthop_impl::fill_spec_(nexthop_actiondata_t *data,
                         pds_nexthop_spec_t *spec) {
    if (PDS_IMPL_SYSTEM_DROP_NEXTHOP_HW_ID == hw_id_) {
        spec->type = PDS_NH_TYPE_BLACKHOLE;
        return;
    }
    // TODO: read DIPo, DIPi, other types
    spec->type = PDS_NH_TYPE_IP;
    spec->vlan = data->action_u.nexthop_nexthop_info.vni;
    sdk::lib::memrev(spec->mac, data->action_u.nexthop_nexthop_info.dmaci,
                     ETH_ADDR_LEN);
}

sdk_ret_t
nexthop_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    nexthop_actiondata_t nh_data = { 0 };
    pds_nexthop_info_t *nh_info = (pds_nexthop_info_t *)info;

    if (nexthop_impl_db()->nh_tbl()->retrieve(hw_id_,
                                              &nh_data) != SDK_RET_OK) {
        return SDK_RET_ENTRY_NOT_FOUND;
    }
    fill_spec_(&nh_data, &nh_info->spec);
    fill_status_(&nh_info->status);

    return SDK_RET_OK;
}

/// \@}    // end of PDS_NEXTHOP_IMPL

}    // namespace impl
}    // namespace api
