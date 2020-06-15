//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of vport
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
#include "nic/apollo/api/vport.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/impl/apulu/vport_impl.hpp"
#include "nic/apollo/api/impl/apulu/security_policy_impl.hpp"
#include "nic/apollo/p4/include/apulu_defines.h"
#include "gen/p4gen/p4/include/ftl.h"

// compute next vport epoch given current epoch
#define PDS_IMPL_VPORT_EPOCH_NEXT(epoch)    ((++(epoch)) & 0xFF)

namespace api {
namespace impl {

/// \defgroup PDS_VPORT_IMPL - vport entry datapath implementation
/// \ingroup PDS_VPORT
/// @{

vport_impl *
vport_impl::factory(pds_vport_spec_t *spec) {
    vport_impl *impl;

    impl = vport_impl_db()->alloc();
    new (impl) vport_impl(spec);
    return impl;
}

void
vport_impl::destroy(vport_impl *impl) {
    impl->~vport_impl();
    vport_impl_db()->free(impl);
}

impl_base *
vport_impl::clone(void) {
    vport_impl *cloned_impl;

    cloned_impl = vport_impl_db()->alloc();
    new (cloned_impl) vport_impl();
    // deep copy is not needed as we don't store pointers
    *cloned_impl = *this;
    return cloned_impl;
}

sdk_ret_t
vport_impl::free(vport_impl *impl) {
    destroy(impl);
    return SDK_RET_OK;
}

sdk_ret_t
vport_impl::reserve_resources(api_base *api_obj, api_base *orig_obj,
                              api_obj_ctxt_t *obj_ctxt) {
    uint32_t idx;
    sdk_ret_t ret;
    pds_vport_spec_t *spec = &obj_ctxt->api_params->vport_spec;

    switch (obj_ctxt->api_op) {
    case API_OP_CREATE:
    case API_OP_UPDATE:
        // record the fact that resource reservation was attempted
        // NOTE: even if we partially acquire resources and fail eventually,
        //       this will ensure that proper release of resources will happen
        api_obj->set_rsvd_rsc();
        // reserve an entry in the NEXTHOP table for this vport
        ret = nexthop_impl_db()->nh_idxr()->alloc(&idx);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to allocate nexthop entry for vport %s, "
                          "err %u", spec->key.str(), ret);
            return ret;
        }
        nh_idx_ = idx;

        // allocate hw id for this vport
        if ((ret = vnic_impl_db()->vnic_idxr()->alloc(&idx)) !=
                SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to allocate hw id for vport %s, err %u",
                          spec->key.str(), ret);
            goto error;
        }
        break;

    default:
        break;
    }
    return SDK_RET_OK;

error:

    PDS_TRACE_ERR("Failed to acquire all h/w resources for vport %s, err %u",
                  spec->key.str(), ret);
    return ret;
}

sdk_ret_t
vport_impl::release_resources(api_base *api_obj) {
    sdk_ret_t ret;
    sdk_table_api_params_t tparams;

    // release the NEXTHOP table entry
    if (nh_idx_ != 0xFFFF) {
        nexthop_impl_db()->nh_idxr()->free(nh_idx_);
    }

    // release the vport h/w id
    if (hw_id_ != 0xFFFF) {
        vnic_impl_db()->vnic_idxr()->free(hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
vport_impl::nuke_resources(api_base *api_obj) {
    sdk_ret_t ret;
    sdk_table_api_params_t tparams;

    // release the NEXTHOP table entry
    if (nh_idx_ != 0xFFFF) {
        nexthop_impl_db()->nh_idxr()->free(nh_idx_);
    }

    // free the vport h/w id
    if (hw_id_ != 0xFFFF) {
        vnic_impl_db()->vnic_idxr()->free(hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
vport_impl::populate_msg(pds_msg_t *msg, api_base *api_obj,
                        api_obj_ctxt_t *obj_ctxt) {
    msg->cfg_msg.vport.status.hw_id = hw_id_;
    msg->cfg_msg.vport.status.nh_hw_id = nh_idx_;
    return SDK_RET_OK;
}

sdk_ret_t
vport_impl::program_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    return SDK_RET_ERR;
}

sdk_ret_t
vport_impl::cleanup_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    return SDK_RET_OK;
}

sdk_ret_t
vport_impl::update_hw(api_base *orig_obj, api_base *curr_obj,
                     api_obj_ctxt_t *obj_ctxt) {
    return SDK_RET_ERR;
}

#define vlan_info    action_u.vlan_vlan_info
sdk_ret_t
vport_impl::add_vlan_entry_(pds_epoch_t epoch, vport_entry *vport,
                            pds_vport_spec_t *spec) {
    return SDK_RET_ERR;
}

sdk_ret_t
vport_impl::activate_create_(pds_epoch_t epoch, vport_entry *vport,
                            pds_vport_spec_t *spec) {
    return SDK_RET_ERR;
}

sdk_ret_t
vport_impl::activate_delete_(pds_epoch_t epoch, vport_entry *vport) {
    return SDK_RET_ERR;
}

sdk_ret_t
vport_impl::activate_update_(pds_epoch_t epoch, vport_entry *vport,
                             vport_entry *orig_vport,
                             api_obj_ctxt_t *obj_ctxt) {
    return SDK_RET_ERR;
}

sdk_ret_t
vport_impl::activate_hw(api_base *api_obj, api_base *orig_obj,
                        pds_epoch_t epoch,
                        api_op_t api_op, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_vport_spec_t *spec;

    switch (api_op) {
    case API_OP_CREATE:
        spec = &obj_ctxt->api_params->vport_spec;
        ret = activate_create_(epoch, (vport_entry *)api_obj, spec);
        break;

    case API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (vport_entry *)api_obj);
        break;

    case API_OP_UPDATE:
        ret = activate_update_(epoch, (vport_entry *)api_obj,
                               (vport_entry *)orig_obj, obj_ctxt);
        break;

    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

sdk_ret_t
vport_impl::reprogram_hw(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    return SDK_RET_ERR;
}

void
vport_impl::fill_status_(pds_vport_status_t *status) {
    status->hw_id = hw_id_;
    status->nh_hw_id = nh_idx_;
}

sdk_ret_t
vport_impl::fill_stats_(pds_vport_stats_t *stats) {
    return SDK_RET_ERR;
}

sdk_ret_t
vport_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    sdk_ret_t rv;
    pds_vport_info_t *vport_info = (pds_vport_info_t *)info;

    rv = fill_stats_(&vport_info->stats);
    if (unlikely(rv != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failed to read hardware stats tables for vport %s",
                      api_obj->key2str().c_str());
        return rv;
    }
    fill_status_(&vport_info->status);
    return SDK_RET_OK;
}

/// \@}

}    // namespace impl
}    // namespace api
