//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// route table handling
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/api/route.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/framework/api_ctxt.hpp"
#include "nic/apollo/framework/api_engine.hpp"

namespace api {

/// \@defgroup PDS_ROUTE_TABLE - route table functionality
/// \@ingroup PDS_ROUTE
/// \@{

typedef struct route_table_update_ctxt_s {
    route_table *rtable;
    obj_ctxt_t *obj_ctxt;
} __PACK__ route_table_update_ctxt_t;

route_table::route_table() {
    //SDK_SPINLOCK_INIT(&slock_, PTHREAD_PROCESS_PRIVATE);
    ht_ctxt_.reset();
}

route_table *
route_table::factory(pds_route_table_spec_t *spec) {
    route_table    *rtable;

    // create route table instance with defaults, if any
    rtable = route_table_db()->alloc();
    if (rtable) {
        new (rtable) route_table();
        rtable->impl_ = impl_base::factory(impl::IMPL_OBJ_ID_ROUTE_TABLE, spec);
        if (rtable->impl_ == NULL) {
            route_table::destroy(rtable);
            return NULL;
        }
    }
    return rtable;
}

route_table::~route_table() {
    // TODO: fix me
    //SDK_SPINLOCK_DESTROY(&slock_);
}

void
route_table::destroy(route_table *rtable) {
    rtable->nuke_resources_();
    if (rtable->impl_) {
        impl_base::destroy(impl::IMPL_OBJ_ID_ROUTE_TABLE, rtable->impl_);
    }
    rtable->~route_table();
    route_table_db()->free(rtable);
}

sdk_ret_t
route_table::init_config(api_ctxt_t *api_ctxt) {
    pds_route_table_spec_t    *spec;

    spec = &api_ctxt->api_params->route_table_spec;
    memcpy(&this->key_, &spec->key, sizeof(pds_route_table_key_t));
    this->af_ = spec->af;
    return SDK_RET_OK;
}

sdk_ret_t
route_table::reserve_resources(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    return impl_->reserve_resources(this, obj_ctxt);
}

sdk_ret_t
route_table::program_config(obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Programming route table %u", key_.id);
    return impl_->program_hw(this, obj_ctxt);
}

sdk_ret_t
route_table::nuke_resources_(void) {
    return impl_->nuke_resources(this);
}

sdk_ret_t
route_table::release_resources(void) {
    return impl_->release_resources(this);
}

sdk_ret_t
route_table::update_config(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    return impl_->program_hw(this, obj_ctxt);
}

sdk_ret_t
route_table::activate_config(pds_epoch_t epoch, api_op_t api_op,
                            obj_ctxt_t *obj_ctxt) {
    PDS_TRACE_DEBUG("Activating route table %u config", key_.id);
    return impl_->activate_hw(this, epoch, api_op, obj_ctxt);
}

sdk_ret_t
route_table::update_db(api_base *orig_obj, obj_ctxt_t *obj_ctxt) {
    return sdk::SDK_RET_INVALID_OP;
}

sdk_ret_t
route_table::add_to_db(void) {
    PDS_TRACE_VERBOSE("Adding route table %u to db", key_.id);
    return route_table_db()->insert(this);
}

sdk_ret_t
route_table::del_from_db(void) {
    if (route_table_db()->remove(this)) {
        return SDK_RET_OK;
    }
    return SDK_RET_ENTRY_NOT_FOUND;
}

sdk_ret_t
route_table::delay_delete(void) {
    return delay_delete_to_slab(PDS_SLAB_ID_ROUTE_TABLE, this);
}

static bool
subnet_upd_walk_cb_(void *api_obj, void *ctxt) {
    subnet_entry *subnet = (subnet_entry *)api_obj;
    route_table_update_ctxt_t *upd_ctxt = (route_table_update_ctxt_t *)ctxt;

    if ((subnet->v4_route_table().id == upd_ctxt->rtable->key().id) ||
        (subnet->v6_route_table().id == upd_ctxt->rtable->key().id)) {
        upd_ctxt->obj_ctxt->add_deps(subnet, API_OP_UPDATE);
    }
    return false;
}

sdk_ret_t
route_table::add_deps(obj_ctxt_t *obj_ctxt) {
    route_table_update_ctxt_t upd_ctxt = { 0 };

    upd_ctxt.rtable = this;
    upd_ctxt.obj_ctxt = obj_ctxt;
    return subnet_db()->walk(subnet_upd_walk_cb_, &upd_ctxt);
}

/// \@}    // end of PDS_ROUTE_TABLE

}    // namespace api
