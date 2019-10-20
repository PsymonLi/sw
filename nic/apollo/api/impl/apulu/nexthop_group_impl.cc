//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of nexthop group
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/nexthop_group.hpp"
#include "nic/apollo/api/impl/apulu/nexthop_group_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"

namespace api {
namespace impl {

/// \defgroup PDS_NEXTHOP_GROUP_IMPL - nexthop group datapath implementation
/// \ingroup PDS_NEXTHOP
/// \@{

nexthop_group_impl *
nexthop_group_impl::factory(pds_nexthop_group_spec_t *spec) {
    nexthop_group_impl *impl;

    // TODO: move to slab later
    impl = (nexthop_group_impl *)
               SDK_CALLOC(SDK_MEM_ALLOC_PDS_NEXTHOP_GROUP_IMPL,
                          sizeof(nexthop_group_impl));
    new (impl) nexthop_group_impl();
    return impl;
}

void
nexthop_group_impl::destroy(nexthop_group_impl *impl) {
    impl->~nexthop_group_impl();
    SDK_FREE(SDK_MEM_ALLOC_PDS_NEXTHOP_GROUP_IMPL, impl);
}

sdk_ret_t
nexthop_group_impl::reserve_resources(api_base *orig_obj,
                                      obj_ctxt_t *obj_ctxt) {
    uint32_t idx;
    sdk_ret_t ret;
    pds_nexthop_group_spec_t *spec;

    spec = &obj_ctxt->api_params->nexthop_group_spec;
    // reserve an entry in NEXTHOP_GROUP table
    ret = nexthop_group_impl_db()->nhgroup_idxr()->alloc(&idx);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to reserve an entry in ECMP table, ",
                      "for nexthop group %u, err %u", spec->key.id, ret);
        return ret;
    }
    hw_id_ = idx;
    if (spec->type == PDS_NHGROUP_TYPE_UNDERLAY_ECMP) {
        if (spec->num_nexthops) {
            ret = nexthop_impl_db()->nh_idxr()->alloc(&idx, spec->num_nexthops);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Failed to reserve %u entries in "
                              "NEXTHOP table for nexthop group %u, "
                              "err %u", spec->num_nexthops,
                              spec->key.id, ret);
                goto error;
            }
            nh_base_hw_id_ = idx;
        }
    }
    return SDK_RET_OK;

error:

    if (hw_id_ != 0xFFFF) {
        nexthop_group_impl_db()->nhgroup_idxr()->free(hw_id_);
        hw_id_ = 0xFFFF;
    }
    return ret;
}

sdk_ret_t
nexthop_group_impl::release_resources(api_base *api_obj) {
    nexthop_group *nhgroup = (nexthop_group *)api_obj;

    if (hw_id_ != 0xFFFF) {
        nexthop_group_impl_db()->nhgroup_idxr()->free(hw_id_);
    }
    if (nh_base_hw_id_ != 0xFFFF) {
        nexthop_impl_db()->nh_idxr()->free(nh_base_hw_id_,
                                           nhgroup->num_nexthops());
    }
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_group_impl::nuke_resources(api_base *api_obj) {
    // for indexers, release_resources() does the job
    return this->release_resources(api_obj);
}

sdk_ret_t
nexthop_group_impl::activate_create_(pds_epoch_t epoch,
                                     nexthop_group *nh_group,
                                     pds_nexthop_group_spec_t *spec) {
    return SDK_RET_ERR;
}

sdk_ret_t
nexthop_group_impl::activate_delete_(pds_epoch_t epoch,
                                     nexthop_group *nh_group) {
    return SDK_RET_ERR;
}

sdk_ret_t
nexthop_group_impl::activate_hw(api_base *api_obj, pds_epoch_t epoch,
                                api_op_t api_op, obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_nexthop_group_spec_t *spec;

    switch (api_op) {
    case api::API_OP_CREATE:
        spec = &obj_ctxt->api_params->nexthop_group_spec;
        ret = activate_create_(epoch, (nexthop_group *)api_obj, spec);
        break;

    case api::API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (nexthop_group *)api_obj);
        break;

    case api::API_OP_UPDATE:
    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

sdk_ret_t
nexthop_group_impl::reprogram_hw(api_base *api_obj, api_op_t api_op) {
    return SDK_RET_INVALID_OP;
}

sdk_ret_t
nexthop_group_impl::cleanup_hw(api_base *api_obj, obj_ctxt_t *obj_ctxt) {
    return SDK_RET_OK;
}

sdk_ret_t
nexthop_group_impl::update_hw(api_base *orig_obj, api_base *curr_obj,
                              obj_ctxt_t *obj_ctxt) {
    return sdk::SDK_RET_INVALID_OP;
}

sdk_ret_t
nexthop_group_impl::reactivate_hw(api_base *api_obj, pds_epoch_t epoch,
                                  api_op_t api_op) {
    return SDK_RET_ERR;
}

void
nexthop_group_impl::fill_status_(pds_nexthop_group_status_t *status) {
    status->hw_id = hw_id_;
}

sdk_ret_t
nexthop_group_impl::fill_spec_(pds_nexthop_group_spec_t *spec) {
    return SDK_RET_ERR;
}

sdk_ret_t
nexthop_group_impl::read_hw(api_base *api_obj, obj_key_t *key,
                            obj_info_t *info) {
    return SDK_RET_ERR;
}

/// \@}    // end of PDS_NEXTHOP_GROUP_IMPL

}    // namespace impl
}    // namespace api
