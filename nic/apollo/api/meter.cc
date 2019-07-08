//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// meter entry handling
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/api/meter.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/framework/api_ctxt.hpp"
#include "nic/apollo/framework/api_engine.hpp"

namespace api {

/// \@defgroup PDS_METER_ENTRY - meter functionality
/// \@ingroup PDS_METER
/// \@{

typedef struct meter_update_ctxt_s {
    meter_entry *meter;
    obj_ctxt_t *obj_ctxt;
} __PACK__ meter_update_ctxt_t;

meter_entry::meter_entry() {
    //SDK_SPINLOCK_INIT(&slock_, PTHREAD_PROCESS_PRIVATE);
    ht_ctxt_.reset();
}

meter_entry *
meter_entry::factory(pds_meter_spec_t *spec) {
    meter_entry    *meter;

    // create meter instance with defaults, if any
    meter = meter_db()->alloc();
    if (meter) {
        new (meter) meter_entry();
        meter->impl_ = impl_base::factory(impl::IMPL_OBJ_ID_METER, spec);
        if (meter->impl_ == NULL) {
            meter_entry::destroy(meter);
            return NULL;
        }
    }
    return meter;
}

meter_entry::~meter_entry() {
    // TODO: fix me
    //SDK_SPINLOCK_DESTROY(&slock_);
}

void
meter_entry::destroy(meter_entry *meter) {
    meter->nuke_resources_();
    if (meter->impl_) {
        impl_base::destroy(impl::IMPL_OBJ_ID_METER, meter->impl_);
    }
    meter->~meter_entry();
    meter_db()->free(meter);
}

sdk_ret_t
meter_entry::init_config(api_ctxt_t *api_ctxt) {
    pds_meter_spec_t    *spec;

    spec = &api_ctxt->api_params->meter_spec;
    memcpy(&this->key_, &spec->key, sizeof(pds_meter_key_t));
    this->af_ = spec->af;
    return SDK_RET_OK;
}

sdk_ret_t
meter_entry::reserve_resources(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    return impl_->reserve_resources(this, obj_ctxt);
}

sdk_ret_t
meter_entry::program_config(obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Programming meter %u", key_.id);
    return impl_->program_hw(this, obj_ctxt);
}

sdk_ret_t
meter_entry::nuke_resources_(void) {
    return impl_->nuke_resources(this);
}

sdk_ret_t
meter_entry::release_resources(void) {
    return impl_->release_resources(this);
}

sdk_ret_t
meter_entry::update_config(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    return impl_->program_hw(this, obj_ctxt);
}

sdk_ret_t
meter_entry::activate_config(pds_epoch_t epoch, api_op_t api_op,
                            obj_ctxt_t *obj_ctxt) {
    return impl_->activate_hw(this, epoch, api_op, obj_ctxt);
}

sdk_ret_t
meter_entry::update_db(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    return sdk::SDK_RET_INVALID_OP;
}

sdk_ret_t
meter_entry::add_to_db(void) {
    PDS_TRACE_VERBOSE("Adding meter %u to db", key_.id);
    return meter_db()->insert(this);
}

sdk_ret_t
meter_entry::del_from_db(void) {
    if (meter_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
meter_entry::delay_delete(void) {
    return delay_delete_to_slab(PDS_SLAB_ID_METER, this);
}

static bool
vnic_upd_walk_cb_(void *api_obj, void *ctxt) {
    vnic_entry *vnic = (vnic_entry *)api_obj;
    meter_update_ctxt_t *upd_ctxt = (meter_update_ctxt_t *)ctxt;

    if ((vnic->meter(IP_AF_IPV4).id == upd_ctxt->meter->key().id) ||
        (vnic->meter(IP_AF_IPV4).id == upd_ctxt->meter->key().id)) {
        upd_ctxt->obj_ctxt->add_deps(vnic, API_OP_UPDATE);
    }
    return false;
}

sdk_ret_t
meter_entry::add_deps(obj_ctxt_t *obj_ctxt) {
    meter_update_ctxt_t upd_ctxt = { 0 };

    upd_ctxt.meter = this;
    upd_ctxt.obj_ctxt = obj_ctxt;
    return vnic_db()->walk(vnic_upd_walk_cb_, &upd_ctxt);
}

sdk_ret_t
meter_entry::read(pds_meter_key_t *key, pds_meter_info_t *info) {
    if (impl_) {
        return impl_->read_hw(this, (impl::obj_key_t *)key,
                          (impl::obj_info_t *)info);
    }
}

/// \@}    // end of PDS_METER_ENTRY

}    // namespace api
