//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// vport entry handling
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/msg.h"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/core/msg.h"
#include "nic/apollo/api/vport.hpp"
#include "nic/apollo/api/pds_state.hpp"

namespace api {

/// \defgroup PDS_VPORT_ENTRY - vport functionality
/// \ingroup PDS_VPORT
/// @{

vport_entry::vport_entry() {
    ht_ctxt_.reset();
    impl_ = NULL;
}

vport_entry *
vport_entry::factory(pds_vport_spec_t *spec) {
    vport_entry *vport;

    // create vport entry with defaults, if any
    vport = vport_db()->alloc();
    if (vport) {
        new (vport) vport_entry();
        vport->impl_ = impl_base::factory(impl::IMPL_OBJ_ID_VPORT, spec);
        if (vport->impl_ == NULL) {
            vport_entry::destroy(vport);
            return NULL;
        }
    }
    return vport;
}

vport_entry::~vport_entry() {
}

void
vport_entry::destroy(vport_entry *vport) {
    vport->nuke_resources_();
    if (vport->impl_) {
        impl_base::destroy(impl::IMPL_OBJ_ID_VPORT, vport->impl_);
    }
    vport->~vport_entry();
    vport_db()->free(vport);
}

api_base *
vport_entry::clone(api_ctxt_t *api_ctxt) {
    vport_entry *cloned_vport;

    cloned_vport = vport_db()->alloc();
    if (cloned_vport) {
        new (cloned_vport) vport_entry();
        if (cloned_vport->init_config(api_ctxt) != SDK_RET_OK) {
            goto error;
        }
        cloned_vport->impl_ = impl_->clone();
        if (unlikely(cloned_vport->impl_ == NULL)) {
            PDS_TRACE_ERR("Failed to clone vport %s impl", key_.str());
            goto error;
        }
    }
    return cloned_vport;

error:

    cloned_vport->~vport_entry();
    vport_db()->free(cloned_vport);
    return NULL;
}

sdk_ret_t
vport_entry::free(vport_entry *vport) {
    if (vport->impl_) {
        impl_base::free(impl::IMPL_OBJ_ID_VPORT, vport->impl_);
    }
    vport->~vport_entry();
    vport_db()->free(vport);
    return SDK_RET_OK;
}

sdk_ret_t
vport_entry::reserve_resources(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    return impl_->reserve_resources(this, orig_obj, obj_ctxt);
}

sdk_ret_t
vport_entry::release_resources(void) {
    return impl_->release_resources(this);
}

sdk_ret_t
vport_entry::nuke_resources_(void) {
    return impl_->nuke_resources(this);
}

sdk_ret_t
vport_entry::init_config(api_ctxt_t *api_ctxt) {
    pds_vport_spec_t *spec = &api_ctxt->api_params->vport_spec;

    memcpy(&key_, &spec->key, sizeof(pds_obj_key_t));
    encap_ = spec->encap;
    if (unlikely((encap_.type != PDS_ENCAP_TYPE_NONE) &&
                 (encap_.type != PDS_ENCAP_TYPE_QINQ))) {
        PDS_TRACE_ERR("Invalid encap type %u on vport %s",
                      encap_.type, key_.str());
        return SDK_RET_INVALID_ARG;

    }
    num_ing_v4_policy_ = spec->num_ing_v4_policy;
    for (uint8_t i = 0; i < num_ing_v4_policy_; i++) {
        ing_v4_policy_[i] = spec->ing_v4_policy[i];
    }
    num_ing_v6_policy_ = spec->num_ing_v6_policy;
    for (uint8_t i = 0; i < num_ing_v6_policy_; i++) {
        ing_v6_policy_[i] = spec->ing_v6_policy[i];
    }
    num_egr_v4_policy_ = spec->num_egr_v4_policy;
    for (uint8_t i = 0; i < num_egr_v4_policy_; i++) {
        egr_v4_policy_[i] = spec->egr_v4_policy[i];
    }
    num_egr_v6_policy_ = spec->num_egr_v6_policy;
    for (uint8_t i = 0; i < num_egr_v6_policy_; i++) {
        egr_v6_policy_[i] = spec->egr_v6_policy[i];
    }
    return SDK_RET_OK;
}

sdk_ret_t
vport_entry::populate_msg(pds_msg_t *msg, api_obj_ctxt_t *obj_ctxt) {
    msg->cfg_msg.op = obj_ctxt->api_op;
    msg->cfg_msg.obj_id = OBJ_ID_VPORT;
    if (obj_ctxt->api_op == API_OP_DELETE) {
        msg->cfg_msg.vport.key = obj_ctxt->api_params->key;
    } else {
        msg->cfg_msg.vport.spec = obj_ctxt->api_params->vport_spec;
        impl_->populate_msg(msg, this, obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
vport_entry::program_create(api_obj_ctxt_t *obj_ctxt) {
    pds_vport_spec_t *spec = &obj_ctxt->api_params->vport_spec;

    PDS_TRACE_DEBUG("Programming vport %s, encap %s", key_.str(),
                    pds_encap2str(&spec->encap));
    return impl_->program_hw(this, obj_ctxt);
}

sdk_ret_t
vport_entry::cleanup_config(api_obj_ctxt_t *obj_ctxt) {
    return impl_->cleanup_hw(this, obj_ctxt);
}

sdk_ret_t
vport_entry::compute_update(api_obj_ctxt_t *obj_ctxt) {
    pds_vport_spec_t *spec = &obj_ctxt->api_params->vport_spec;

    if ((encap_.type != spec->encap.type) ||
        (encap_.val.value != spec->encap.val.value)) {
        PDS_TRACE_ERR("Attempt to modify immutable attr \"encap\" "
                      "from %s to %s on vport %s",
                      pds_encap2str(&encap_),
                      pds_encap2str(&spec->encap), key_.str());
        return SDK_RET_INVALID_ARG;
    }
    if ((num_ing_v4_policy_ != spec->num_ing_v4_policy)          ||
        (num_ing_v6_policy_ != spec->num_ing_v6_policy)          ||
        (num_egr_v4_policy_ != spec->num_egr_v4_policy)          ||
        (num_egr_v6_policy_ != spec->num_egr_v6_policy)          ||
        (memcmp(ing_v4_policy_, spec->ing_v4_policy,
                num_ing_v4_policy_ * sizeof(ing_v4_policy_[0]))) ||
        (memcmp(ing_v6_policy_, spec->ing_v6_policy,
                num_ing_v6_policy_ * sizeof(ing_v6_policy_[0]))) ||
        (memcmp(egr_v4_policy_, spec->egr_v4_policy,
                num_egr_v4_policy_ * sizeof(egr_v4_policy_[0]))) ||
        (memcmp(egr_v6_policy_, spec->egr_v6_policy,
                num_egr_v6_policy_ * sizeof(egr_v6_policy_[0])))) {
        obj_ctxt->upd_bmap |= PDS_VPORT_UPD_POLICY;
    }
    PDS_TRACE_DEBUG("vport %s upd bmap 0x%lx", key_.str(), obj_ctxt->upd_bmap);
    return SDK_RET_OK;
}

sdk_ret_t
vport_entry::program_update(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Updating vport %s", key_.str());
    return impl_->update_hw(orig_obj, this, obj_ctxt);
}

sdk_ret_t
vport_entry::activate_config(pds_epoch_t epoch, api_op_t api_op,
                            api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Activating vport %s config", key_.str());
    return impl_->activate_hw(this, orig_obj, epoch, api_op, obj_ctxt);
}

sdk_ret_t
vport_entry::reprogram_config(api_obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Reprogramming vport %s, encap %s, upd bmap 0x%lx",
                    key_.str(), pds_encap2str(&encap_), obj_ctxt->upd_bmap);
    return impl_->reprogram_hw(this, obj_ctxt);
}

sdk_ret_t
vport_entry::reactivate_config(pds_epoch_t epoch, api_obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Reactivating vport %s, encap %s",
                    key_.str(), pds_encap2str(&encap_));
    return impl_->reactivate_hw(this, epoch, obj_ctxt);
}

sdk_ret_t
vport_entry::fill_spec_(pds_vport_spec_t *spec) {
    uint8_t i;

    memcpy(&spec->key, &key_, sizeof(pds_obj_key_t));
    spec->encap = encap_;
    spec->num_ing_v4_policy = num_ing_v4_policy_;
    for (i = 0; i < num_ing_v4_policy_; i++) {
        spec->ing_v4_policy[i] = ing_v4_policy_[i];
    }
    spec->num_ing_v6_policy = num_ing_v6_policy_;
    for (i = 0; i < num_ing_v6_policy_; i++) {
        spec->ing_v6_policy[i] = ing_v6_policy_[i];
    }
    spec->num_egr_v4_policy = num_egr_v4_policy_;
    for (i = 0; i < num_egr_v4_policy_; i++) {
        spec->egr_v4_policy[i] = egr_v4_policy_[i];
    }
    spec->num_egr_v6_policy = num_egr_v6_policy_;
    for (i = 0; i < num_egr_v6_policy_; i++) {
        spec->egr_v6_policy[i] = egr_v6_policy_[i];
    }
    return SDK_RET_OK;
}

sdk_ret_t
vport_entry::read(pds_vport_info_t *info) {
    sdk_ret_t ret;

    ret = fill_spec_(&info->spec);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    return impl_->read_hw(this, (impl::obj_key_t *)(&info->spec.key),
                          (impl::obj_info_t *)info);
}

sdk_ret_t
vport_entry::add_to_db(void) {
    PDS_TRACE_VERBOSE("Adding vport %s to db", key_.str());
    return vport_db()->insert(this);
}

sdk_ret_t
vport_entry::del_from_db(void) {
    if (vport_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
vport_entry::update_db(api_base *orig_obj, api_obj_ctxt_t *obj_ctxt) {
    if (vport_db()->remove((vport_entry *)orig_obj)) {
        return vport_db()->insert(this);
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
vport_entry::delay_delete(void) {
    return delay_delete_to_slab(PDS_SLAB_ID_VPORT, this);
}

/// @}     // end of PDS_VPORT_ENTRY

}    // namespace api
