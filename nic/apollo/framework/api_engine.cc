//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// API engine functionality
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/event_thread/event_thread.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_msg.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/framework/impl_base.hpp"
#include "nic/apollo/framework/api_ipc.hpp"
#include "nic/apollo/core/core.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/core/event.hpp"
#include "nic/apollo/core/msg.h"
#include "nic/apollo/core/msg.hpp"
#include "nic/apollo/include/globals.hpp"

#define PDS_API_PREPROCESS_COUNTER_INC(cntr_, val)           \
            counters_.preprocess.cntr_ += (val)
#define PDS_API_PREPROCESS_CREATE_COUNTER_INC(cntr_, val)    \
            counters_.preprocess.create.cntr_ += (val)
#define PDS_API_PREPROCESS_DELETE_COUNTER_INC(cntr_, val)    \
            counters_.preprocess.del.cntr_ += (val)
#define PDS_API_PREPROCESS_UPDATE_COUNTER_INC(cntr_, val)    \
            counters_.preprocess.upd.cntr_ += (val)

#define PDS_API_RSV_RSC_COUNTER_INC(cntr_, val)              \
            counters_.rsv_rsc.cntr_ += (val)
#define PDS_API_RSV_RSC_CREATE_COUNTER_INC(cntr_, val)       \
            counters_.rsv_rsc.create.cntr_ += (val)
#define PDS_API_RSV_RSC_DELETE_COUNTER_INC(cntr_, val)       \
            counters_.rsv_rsc.del.cntr_ += (val)
#define PDS_API_RSV_RSC_UPDATE_COUNTER_INC(cntr_, val)       \
            counters_.rsv_rsc.upd.cntr_ += (val)

#define PDS_API_OBJ_DEP_UPDATE_COUNTER_INC(cntr_, val)       \
            counters_.obj_dep.upd.cntr_ += (val)

#define PDS_API_PGMCFG_COUNTER_INC(cntr_, val)               \
            counters_.pgm_cfg.cntr_ += (val)
#define PDS_API_PGMCFG_CREATE_COUNTER_INC(cntr_, val)        \
            counters_.pgm_cfg.create.cntr_ += (val)
#define PDS_API_PGMCFG_DELETE_COUNTER_INC(cntr_, val)        \
            counters_.pgm_cfg.del.cntr_ += (val)
#define PDS_API_PGMCFG_UPDATE_COUNTER_INC(cntr_, val)        \
            counters_.pgm_cfg.upd.cntr_ += (val)

#define PDS_API_ACTCFG_COUNTER_INC(cntr_, val)               \
            counters_.act_cfg.cntr_ += (val)
#define PDS_API_ACTCFG_CREATE_COUNTER_INC(cntr_, val)        \
            counters_.act_cfg.create.cntr_ += (val)
#define PDS_API_ACTCFG_DELETE_COUNTER_INC(cntr_, val)        \
            counters_.act_cfg.del.cntr_ += (val)
#define PDS_API_ACTCFG_UPDATE_COUNTER_INC(cntr_, val)        \
            counters_.act_cfg.upd.cntr_ += (val)

#define PDS_API_REPGMCFG_UPDATE_COUNTER_INC(cntr_, val)      \
            counters_.re_pgm_cfg.upd.cntr_ += (val)

#define PDS_API_RE_ACTCFG_UPDATE_COUNTER_INC(cntr_, val)     \
            counters_.re_act_cfg.upd.cntr_ += (val)

#define PDS_API_COMMIT_COUNTER_INC(cntr_, val)               \
            counters_.commit.cntr_ += (val)

#define PDS_API_ABORT_COUNTER_INC(cntr_, val)                \
            counters_.abort.cntr_ += (val)

namespace api {

/// \defgroup PDS_API_ENGINE Framework for processing APIs
/// @{

static slab *g_api_ctxt_slab_ = NULL;
static slab *g_api_msg_slab_ = NULL;
static pds_epoch_t g_current_epoch_ = PDS_EPOCH_INVALID;
static api_engine g_api_engine;

api_engine *
api_engine_get (void) {
    return &g_api_engine;
}

slab *
api_ctxt_slab (void)
{
    return g_api_ctxt_slab_;
}

slab *
api_msg_slab (void)
{
    return g_api_msg_slab_;
}

pds_epoch_t
get_current_epoch (void)
{
    return g_current_epoch_;
}

api_base *
api_obj_find_clone (api_base *api_obj) {
    api_base *clone;

    clone = g_api_engine.cloned_obj(api_obj);
    if (clone) {
        return clone;
    }
    return api_obj;
}

api_base *
api_framework_obj (api_base *api_obj) {
    return g_api_engine.framework_obj(api_obj);
}

sdk_ret_t
api_obj_add_to_deps (api_op_t api_op,
                     obj_id_t obj_id_a, api_base *api_obj_a,
                     obj_id_t obj_id_b, api_base *api_obj_b,
                     uint64_t upd_bmap) {
    api_obj_ctxt_t *octxt;

    PDS_TRACE_VERBOSE("Adding %s to AoL, api op %u, upd bmap 0x%lx",
                      api_obj_b->key2str().c_str(), api_op, upd_bmap);
    octxt = g_api_engine.add_to_deps_list(api_op,
                                          obj_id_a, api_obj_a,
                                          obj_id_b, api_obj_b,
                                          upd_bmap);
    if (octxt) {
        PDS_TRACE_VERBOSE("Triggering recursive update on %s, upd bmap 0x%lx",
                          api_obj_b->key2str().c_str(), octxt->upd_bmap);
        api_obj_b->add_deps(octxt);
    }
    return SDK_RET_OK;
}

api_op_t
api_engine::api_op_(api_op_t old_op, api_op_t new_op) {
    return dedup_api_op_[old_op][new_op];
}

sdk_ret_t
api_engine::pre_process_create_(api_ctxt_t *api_ctxt) {
    sdk_ret_t ret;
    api_op_t api_op;
    api_base *api_obj;
    api_obj_ctxt_t *octxt;

    api_obj = api_base::find_obj(api_ctxt);
    if (api_obj == NULL) {
        // instantiate a new object
        api_obj = api_base::factory(api_ctxt);
        if (unlikely(api_obj == NULL)) {
            PDS_API_PREPROCESS_CREATE_COUNTER_INC(oom_err, 1);
            return SDK_RET_ERR;
        }
        PDS_TRACE_VERBOSE("Allocated api obj %p, api op %u, obj id %u",
                          api_obj, api_ctxt->api_op, api_ctxt->obj_id);
        // add it to dirty object list
        octxt = api_obj_ctxt_alloc_();
        octxt->api_op = API_OP_CREATE;
        octxt->obj_id = api_ctxt->obj_id;
        octxt->api_params = api_ctxt->api_params;
        add_to_dirty_list_(api_obj, octxt);
        // initialize the object with the given config
        ret = api_obj->init_config(api_ctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_API_PREPROCESS_CREATE_COUNTER_INC(init_cfg_err, 1);
            PDS_TRACE_ERR("Config initialization failed for obj id %u, "
                          "api op %u, err %u", api_ctxt->obj_id,
                          api_ctxt->api_op, ret);
            return ret;
        }
        ret = api_obj->add_to_db();
        SDK_ASSERT(ret == SDK_RET_OK);
    } else if (api_obj->in_restore_list()) {
        // it is a restored object which is already in db
        // reset this flag only when config is accepted
        // assert in case if this obj was already in dirty list
        // as we are not expecting any modification in same batch
        SDK_ASSERT(api_obj->in_dirty_list() == false);
        octxt = api_obj_ctxt_alloc_();
        octxt->api_op = API_OP_CREATE;
        octxt->obj_id = api_ctxt->obj_id;
        octxt->api_params = api_ctxt->api_params;
        // add it to dirty object list
        add_to_dirty_list_(api_obj, octxt);
        // initialize the object with the given config
        ret = api_obj->init_config(api_ctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_API_PREPROCESS_CREATE_COUNTER_INC(init_cfg_err, 1);
            PDS_TRACE_ERR("Config initialization failed for restored obj id %u, "
                          "api op %u, err %u",
                          api_ctxt->obj_id, api_ctxt->api_op, ret);
            return ret;
        }
        PDS_TRACE_VERBOSE("restored api obj %p, api op %u, obj id %u",
                          api_obj, api_ctxt->api_op, api_ctxt->obj_id);
    } else {
        // this could be XXX-DEL-ADD/ADD-XXX-DEL-XXX-ADD kind of scenario in
        // same batch, as we don't expect to see ADD-XXX-ADD (unless we want
        // to support idempotency)
        if (unlikely(api_obj->in_dirty_list() == false)) {
            // attemping a CREATE on already created object
            PDS_TRACE_ERR("Creating an obj that exists already, obj %s",
                          api_obj->key2str().c_str());
            PDS_API_PREPROCESS_CREATE_COUNTER_INC(obj_exists_err, 1);
            return SDK_RET_ENTRY_EXISTS;
        }
        api_obj_ctxt_t *octxt = batch_ctxt_.dom.find(api_obj)->second;
        if (unlikely((octxt->api_op != API_OP_NONE) &&
                     (octxt->api_op != API_OP_DELETE))) {
            // only API_OP_NONE and API_OP_DELETE are expected in current state
            // in the dirty obj map
            PDS_TRACE_ERR("Invalid CREATE operation on obj %s, id %u with "
                          "outstanding api op %u", api_obj->key2str().c_str(),
                          api_ctxt->obj_id, octxt->api_op);
            PDS_API_PREPROCESS_CREATE_COUNTER_INC(invalid_op_err, 1);
            return SDK_RET_INVALID_OP;
        }
        api_op = api_op_(octxt->api_op, API_OP_CREATE);
        if (unlikely(api_op == API_OP_INVALID)) {
            PDS_TRACE_ERR("Invalid resultant api op on obj %s id %u with "
                          "API_OP_CREATE", api_obj->key2str().c_str(),
                          api_ctxt->obj_id);
            PDS_API_PREPROCESS_CREATE_COUNTER_INC(invalid_op_err, 1);
            return SDK_RET_INVALID_OP;
        }
        // update the config, by cloning the object, if needed
        if (octxt->cloned_obj == NULL) {
            // XXX-DEL-XXX-ADD or ADD-XXX-DEL-XXX-ADD scenarios, we need to
            // differentiate between these two
            if (api_op == API_OP_UPDATE) {
                // XXX-DEL-XXX-ADD scenario, clone & re-init cfg
                octxt->api_op = API_OP_UPDATE;
                octxt->api_params = api_ctxt->api_params;
                octxt->cloned_obj = api_obj->clone(api_ctxt);
                //ret = octxt->cloned_obj->init_config(api_ctxt);
                if (unlikely(octxt->cloned_obj == NULL)) {
                    PDS_TRACE_ERR("Clone failed for obj %s, api op %u",
                                  api_obj->key2str().c_str(),
                                  octxt->api_op);
                    PDS_API_PREPROCESS_CREATE_COUNTER_INC(obj_clone_err, 1);
                    return SDK_RET_OBJ_CLONE_ERR;
                }
                ret = api_obj->compute_update(octxt);
                if (unlikely(ret != SDK_RET_OK)) {
                    // this create operation resulted in an update of existing
                    // object, but the update is invalid (e.g., modifying an
                    // immutable attribute)
                    PDS_TRACE_ERR("Failed to compute update on %s during create"
                                  "preprocessing, err %u",
                                  api_obj->key2str().c_str(), ret);
                    PDS_API_PREPROCESS_CREATE_COUNTER_INC(invalid_upd_err, 1);
                    return ret;
                }
            } else {
                // ADD-XXX-DEL-XXX-ADD scenario, re-init same object
                if (unlikely(api_op != API_OP_CREATE)) {
                    PDS_TRACE_ERR("Unexpected api op %u, obj %s, id %u, "
                                  "expected CREATE", octxt->api_op,
                                  api_obj->key2str().c_str(), api_ctxt->obj_id);
                    PDS_API_PREPROCESS_CREATE_COUNTER_INC(invalid_op_err, 1);
                    return SDK_RET_INVALID_OP;
                }
                octxt->api_op = API_OP_CREATE;
                octxt->api_params = api_ctxt->api_params;
                ret = api_obj->init_config(api_ctxt);
                if (unlikely(ret != SDK_RET_OK)) {
                    PDS_TRACE_ERR("Config initialization failed for obj %s, "
                                  "id %u, api op %u, err %u",
                                  api_obj->key2str().c_str(), api_ctxt->obj_id,
                                  octxt->api_op, ret);
                    PDS_API_PREPROCESS_CREATE_COUNTER_INC(init_cfg_err, 1);
                    return ret;
                }
            }
        } else {
            // UPD-XXX-DEL-XXX-ADD scenario, re-init cloned obj's cfg (if
            // we update original obj, we may not be able to abort later)
            if (unlikely(octxt->api_op != API_OP_UPDATE)) {
                PDS_TRACE_ERR("Unexpected api op %u, obj %s, id %u, "
                              "expected CREATE", octxt->api_op,
                              api_obj->key2str().c_str(), api_ctxt->obj_id);
                PDS_API_PREPROCESS_CREATE_COUNTER_INC(invalid_op_err, 1);
                return SDK_RET_INVALID_OP;
            }
            octxt->api_op = API_OP_UPDATE;
            octxt->api_params = api_ctxt->api_params;
            ret = octxt->cloned_obj->init_config(api_ctxt);
            if (unlikely(ret != SDK_RET_OK)) {
                PDS_TRACE_ERR("Config initialization failed for obj %s, "
                              "id %u, api op %u, err %u",
                              api_obj->key2str().c_str(), api_ctxt->obj_id,
                              octxt->api_op, ret);
                PDS_API_PREPROCESS_CREATE_COUNTER_INC(init_cfg_err, 1);
                return ret;
            }
            ret = api_obj->compute_update(octxt);
            if (unlikely(ret != SDK_RET_OK)) {
                // this create operation resulted in an update of existing
                // object, but the update is invalid (e.g., modifying an
                // immutable attribute)
                PDS_TRACE_ERR("Failed to compute update on %s during create"
                              "preprocessing, err %u",
                              api_obj->key2str().c_str(), ret);
                PDS_API_PREPROCESS_CREATE_COUNTER_INC(invalid_upd_err, 1);
                return ret;
            }
        }
    }
    PDS_API_PREPROCESS_CREATE_COUNTER_INC(ok, 1);
    return ret;
}

sdk_ret_t
api_engine::pre_process_delete_(api_ctxt_t *api_ctxt) {
    api_op_t api_op;
    api_base *api_obj;
    api_obj_ctxt_t *obj_ctxt;

    // look for the object in the cfg db
    api_obj = api_base::find_obj(api_ctxt);
    if (api_obj == nullptr) {
        if (api_base::stateless(api_ctxt->obj_id)) {
            // build stateless objects on the fly
            api_obj = api_base::build(api_ctxt);
            if (api_obj) {
                // and temporarily add to the db (these must be removed during
                // commit/rollback operation)
                api_obj->add_to_db();
            } else {
                PDS_TRACE_ERR("Failed to find/build stateless obj %u",
                              api_ctxt->obj_id);
                PDS_API_PREPROCESS_DELETE_COUNTER_INC(obj_build_err, 1);
                return sdk::SDK_RET_ENTRY_NOT_FOUND;
            }
        } else {
            PDS_TRACE_ERR("Failed to preprocess delete on obj %u",
                          api_ctxt->obj_id);
            PDS_API_PREPROCESS_DELETE_COUNTER_INC(not_found_err, 1);
            return sdk::SDK_RET_ENTRY_NOT_FOUND;
        }
    }

    // by now either we built the object (and added to db) or found the object
    // that is in the db
    if (api_obj->in_dirty_list()) {
        // NOTE.
        // 1. for stateful objects, this probably is a cloned obj
        // (e.g., UPD-XXX-DEL), but that doesn't matter here
        // 2. if this is a stateless obj, we probably built this object, either
        //    due to ADD/UPD before and added to the db
        api_op = api_op_(batch_ctxt_.dom[api_obj]->api_op, API_OP_DELETE);
        if (api_op == API_OP_INVALID) {
            PDS_TRACE_ERR("Unexpected API_OP_INVALID encountered while "
                          "processing API_OP_DELETE, obj %s, id %u",
                          api_obj->key2str().c_str(), api_ctxt->obj_id);
            PDS_API_PREPROCESS_DELETE_COUNTER_INC(invalid_op_err, 1);
            return sdk::SDK_RET_INVALID_OP;
        }
        batch_ctxt_.dom[api_obj]->api_op = api_op;
    } else {
        // add the object to dirty list
        obj_ctxt = api_obj_ctxt_alloc_();
        obj_ctxt->api_op = API_OP_DELETE;
        obj_ctxt->obj_id = api_ctxt->obj_id;
        obj_ctxt->api_params = api_ctxt->api_params;
        add_to_dirty_list_(api_obj, obj_ctxt);
    }
    PDS_API_PREPROCESS_DELETE_COUNTER_INC(ok, 1);
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::pre_process_update_(api_ctxt_t *api_ctxt) {
    sdk_ret_t ret;
    api_op_t api_op;
    api_base *api_obj;
    api_obj_ctxt_t *obj_ctxt;

    api_obj = api_base::find_obj(api_ctxt);
    if (api_obj == nullptr) {
        if (api_base::stateless(api_ctxt->obj_id)) {
            // build stateless objects on the fly
            api_obj = api_base::build(api_ctxt);
            if (api_obj) {
                // and temporarily add to the db (these must be removed during
                // commit/rollback operation)
                api_obj->add_to_db();
            } else {
                PDS_TRACE_ERR("Failed to find/build stateless obj %u",
                              api_ctxt->obj_id);
                PDS_API_PREPROCESS_UPDATE_COUNTER_INC(obj_build_err, 1);
                return sdk::SDK_RET_ENTRY_NOT_FOUND;
            }
        } else {
            PDS_API_PREPROCESS_UPDATE_COUNTER_INC(not_found_err, 1);
            return sdk::SDK_RET_ENTRY_NOT_FOUND;
        }
    }

    if (api_obj->in_dirty_list()) {
        api_obj_ctxt_t *octxt =
            batch_ctxt_.dom.find(api_obj)->second;
        api_op = api_op_(octxt->api_op, API_OP_UPDATE);
        if (api_op == API_OP_INVALID) {
            PDS_TRACE_ERR("Unexpected API_OP_INVALID encountered while "
                          "processing API_OP_UPDATE, obj %s, id %u",
                          api_obj->key2str().c_str(), api_ctxt->obj_id);
            PDS_API_PREPROCESS_UPDATE_COUNTER_INC(invalid_op_err, 1);
            return sdk::SDK_RET_INVALID_OP;
        }
        octxt->api_op = api_op;
        if (octxt->cloned_obj == NULL) {
            // XXX-ADD-XXX-UPD in same batch, no need to clone yet, just
            // re-init the object with new config and continue with de-duped
            // operation as ADD
            octxt->api_params = api_ctxt->api_params;
            ret = api_obj->init_config(api_ctxt);
            if (ret != SDK_RET_OK) {
                PDS_API_PREPROCESS_UPDATE_COUNTER_INC(init_cfg_err, 1);
                return ret;
            }
        } else {
            // XXX-UPD-XXX-UPD scenario, update the cloned obj
            octxt->api_params = api_ctxt->api_params;
            ret = octxt->cloned_obj->init_config(api_ctxt);
            if (ret != SDK_RET_OK) {
                PDS_API_PREPROCESS_UPDATE_COUNTER_INC(init_cfg_err, 1);
                return ret;
            }
            // compute the updated attrs
            ret = api_obj->compute_update(octxt);
            if (unlikely(ret != SDK_RET_OK)) {
                // update is invalid (e.g., modifying an immutable attribute)
                PDS_TRACE_ERR("Failed to compute update on %s, err %u",
                              api_obj->key2str().c_str(), ret);
                PDS_API_PREPROCESS_UPDATE_COUNTER_INC(invalid_upd_err, 1);
                return ret;
            }
        }
    } else {
        // UPD-XXX scenario, update operation is seen 1st time on this
        // object in this batch
        obj_ctxt = api_obj_ctxt_alloc_();
        obj_ctxt->api_op = API_OP_UPDATE;
        obj_ctxt->obj_id = api_ctxt->obj_id;
        obj_ctxt->api_params = api_ctxt->api_params;
        obj_ctxt->cloned_obj = api_obj->clone(api_ctxt);
        if (obj_ctxt->cloned_obj == NULL) {
            PDS_API_PREPROCESS_UPDATE_COUNTER_INC(obj_clone_err, 1);
            return SDK_RET_OBJ_CLONE_ERR;
        }
        add_to_dirty_list_(api_obj, obj_ctxt);
        ret = api_obj->compute_update(obj_ctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            // update is invalid (e.g., modifying an immutable attribute)
            PDS_TRACE_ERR("Failed to compute update on %s, err %u",
                          api_obj->key2str().c_str(), ret);
            PDS_API_PREPROCESS_UPDATE_COUNTER_INC(invalid_upd_err, 1);
            return ret;
        }
    }
    PDS_API_PREPROCESS_UPDATE_COUNTER_INC(ok, 1);
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::pre_process_api_(api_ctxt_t *api_ctxt) {
    sdk_ret_t ret = SDK_RET_OK;

    PDS_TRACE_DEBUG("Performing api op %u on api object %u",
                    api_ctxt->api_op, api_ctxt->obj_id);
    SDK_ASSERT_RETURN((batch_ctxt_.stage == API_BATCH_STAGE_PRE_PROCESS),
                       sdk::SDK_RET_INVALID_OP);
    switch (api_ctxt->api_op) {
    case API_OP_CREATE:
        ret = pre_process_create_(api_ctxt);
        break;

    case API_OP_DELETE:
        ret = pre_process_delete_(api_ctxt);
        break;

    case API_OP_UPDATE:
        ret = pre_process_update_(api_ctxt);
        break;

    default:
        ret = sdk::SDK_RET_INVALID_OP;
        break;
    }

    if (unlikely(ret != SDK_RET_OK)) {
        PDS_TRACE_ERR("Failure in pre-process stage, api op %u, obj id %u, "
                      "err %u", api_ctxt->api_op, api_ctxt->obj_id, ret);
    }
    return ret;
}

sdk_ret_t
api_engine::pre_process_stage_(void) {
    sdk_ret_t     ret;

    SDK_ASSERT_RETURN((batch_ctxt_.stage == API_BATCH_STAGE_INIT),
                      sdk::SDK_RET_INVALID_ARG);
    batch_ctxt_.stage = API_BATCH_STAGE_PRE_PROCESS;
    for (auto it = batch_ctxt_.api_ctxts->begin();
         it != batch_ctxt_.api_ctxts->end(); ++it) {
        ret = pre_process_api_(*it);
        if (ret != SDK_RET_OK) {
            goto error;
        }
    }
    return SDK_RET_OK;

error:

    return ret;
}

sdk_ret_t
api_engine::add_deps_(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    // for contained/child objects, any of the ADD/DEL/UPD operations may
    // affect the parent object (e.g. route obj will affect route table object
    // if route is added or delete or updated and same with policy rule & policy
    // objects.
    // for all other objects, currently we need to add object dependencies only
    // when a object is updated, however, it is possible in future that add or
    // delete of these objects might impact other objects as well (current
    // assumption is that it is handled in the agent when such a case arises)
    if (api_base::contained(obj_ctxt->obj_id) ||
        (obj_ctxt->api_op == API_OP_UPDATE)) {
        // NOTE: we are invoking this method on the original unmodified object
        return api_obj->add_deps(obj_ctxt);
    }
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::count_api_msg_(obj_id_t obj_id, api_base *api_obj,
                           api_obj_ctxt_t *obj_ctxt) {
    if (!api_obj->circulate(obj_ctxt)) {
        // none of the IPC endpoints are interested in this API object
        return SDK_RET_OK;
    }

    // check which IPC endpoint needs what type of msg and increment
    // corresponding counters
    api_obj_ipc_peer_list_t& ipc_ep_list = ipc_peer_list(obj_id);
    for (auto it = ipc_ep_list.begin();
         (it != ipc_ep_list.end()) && (it->ipc_id != ipc_msg()->sender());
         ++it) {
        // atleast one IPC endpoint is potentially interested in this API msg
        api_obj->set_circulate();
        if (batch_ctxt_.msg_map.find(it->ipc_id) != batch_ctxt_.msg_map.end()) {
            if (it->ntfn) {
                batch_ctxt_.msg_map[it->ipc_id].num_ntfn_msgs++;
            } else {
                batch_ctxt_.msg_map[it->ipc_id].num_req_rsp_msgs++;
            }
        } else {
            if (it->ntfn) {
                batch_ctxt_.msg_map[it->ipc_id].num_req_rsp_msgs = 0;
                batch_ctxt_.msg_map[it->ipc_id].num_ntfn_msgs = 1;
            } else {
                batch_ctxt_.msg_map[it->ipc_id].num_req_rsp_msgs = 1;
                batch_ctxt_.msg_map[it->ipc_id].num_ntfn_msgs = 0;
            }
            batch_ctxt_.msg_map[it->ipc_id].req_rsp_msgs = NULL;
            batch_ctxt_.msg_map[it->ipc_id].ntfn_msgs = NULL;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::allocate_api_msgs_(void) {
    for (auto it = batch_ctxt_.msg_map.begin();
         it != batch_ctxt_.msg_map.end(); it++) {
        if (it->second.num_req_rsp_msgs) {
            it->second.req_rsp_msgs =
                (pds_msg_list_t *)core::pds_msg_list_alloc(
                                            PDS_MSG_TYPE_CFG_OBJ_SET,
                                            batch_ctxt_.epoch,
                                            it->second.num_req_rsp_msgs);
            if (it->second.req_rsp_msgs == NULL) {
                PDS_TRACE_ERR("Failed to allocate memory for %u IPC msgs for "
                              "IPC id %u", it->second.num_req_rsp_msgs,
                              it->first);
                return SDK_RET_OOM;
            }
        }
        if (it->second.num_ntfn_msgs) {
            it->second.ntfn_msgs =
                (pds_msg_list_t *)core::pds_msg_list_alloc(
                                            PDS_MSG_TYPE_CFG_OBJ_SET,
                                            batch_ctxt_.epoch,
                                            it->second.num_ntfn_msgs);
            if (it->second.ntfn_msgs == NULL) {
                PDS_TRACE_ERR("Failed to allocate memory for %u IPC msgs for "
                              "IPC id %u", it->second.num_ntfn_msgs, it->first);
                return SDK_RET_OOM;
            }
        }
        PDS_TRACE_INFO("IPC peer %u circulation list, req/rsp msgs %u, "
                       "ntfn msgs %u", it->first,
                       it->second.num_req_rsp_msgs, it->second.num_ntfn_msgs);
    }
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::populate_api_msg_(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    pds_msg_t msg;

    // form the message we want to send
    api_obj->populate_msg(&msg, obj_ctxt);

    // walk all IPC endpoints interested in this object and keep a copy
    // to be send in respective msg list
    api_obj_ipc_peer_list_t& ipc_ep_list = ipc_peer_list(obj_ctxt->obj_id);
    for (auto it = ipc_ep_list.begin();
         (it != ipc_ep_list.end()) && (it->ipc_id != ipc_msg()->sender());
         ++it) {
        if (it->ntfn) {
            // add to notification msg list
            batch_ctxt_.msg_map[it->ipc_id].add_ntfn_msg(&msg);
        } else {
            // add to req/rsp msg list
            batch_ctxt_.msg_map[it->ipc_id].add_req_rsp_msg(&msg);
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::obj_dependency_computation_stage_(void) {
    sdk_ret_t ret = SDK_RET_OK;
    api_base *api_obj;
    api_obj_ctxt_t *obj_ctxt;

    SDK_ASSERT_RETURN((batch_ctxt_.stage == API_BATCH_STAGE_PRE_PROCESS),
                      sdk::SDK_RET_INVALID_ARG);
    // walk over all the dirty objects and make a list of objects that are
    // affected because of each dirty object
    batch_ctxt_.stage = API_BATCH_STAGE_OBJ_DEPENDENCY;
    for (auto it = batch_ctxt_.dol.begin();
         it != batch_ctxt_.dol.end(); ++it) {
        api_obj = *it;
        obj_ctxt = batch_ctxt_.dom[*it];
        // Add dependency to the parent object only when the child object's
        // api operation is not none
        if (obj_ctxt->api_op != API_OP_NONE) {
            ret = add_deps_(api_obj, obj_ctxt);
        }
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_API_OBJ_DEP_UPDATE_COUNTER_INC(err, 1);
            goto error;
        }
        // if this API msg needs to relayed to other components as well,
        // count it so we can allocate resources for all them in one shot
        if (obj_ctxt->api_op != API_OP_NONE) {
            count_api_msg_(obj_ctxt->obj_id, api_obj, obj_ctxt);
        }
    }

    // if some of these APIs need to be shipped to other components, allocate
    // memory for the necessary messages
    ret = allocate_api_msgs_();
    if (likely(ret == SDK_RET_OK)) {
        PDS_API_OBJ_DEP_UPDATE_COUNTER_INC(ok, 1);
    }

error:

    return ret;
}

sdk_ret_t
api_engine::reserve_resources_(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t     ret;

    switch (obj_ctxt->api_op) {
    case API_OP_NONE:
        return SDK_RET_OK;

    case API_OP_CREATE:
        ret = api_obj->reserve_resources(api_obj, obj_ctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_TRACE_ERR("Failure in resource reservation stage during create "
                          "of obj %s,  err %u",
                          api_obj->key2str().c_str(), ret);
            PDS_API_RSV_RSC_CREATE_COUNTER_INC(err, 1);
            return ret;
        }
        PDS_API_RSV_RSC_CREATE_COUNTER_INC(ok, 1);
        break;

    case API_OP_DELETE:
        // no additional resources needed for delete operation
        PDS_API_RSV_RSC_DELETE_COUNTER_INC(ok, 1);
        break;

    case API_OP_UPDATE:
        // reserve resources on the cloned object
        ret = obj_ctxt->cloned_obj->reserve_resources(api_obj, obj_ctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_TRACE_ERR("Failure in resource reservation stage during update "
                          "of obj %s,  err %u",
                          api_obj->key2str().c_str(), ret);
            PDS_API_RSV_RSC_UPDATE_COUNTER_INC(err, 1);
            return ret;
        }
        PDS_API_RSV_RSC_UPDATE_COUNTER_INC(ok, 1);
        break;

    default:
        SDK_ASSERT_RETURN(FALSE, sdk::SDK_RET_INVALID_OP);
    }
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::resource_reservation_stage_(void) {
    sdk_ret_t ret;
    api_base *api_obj;
    api_obj_ctxt_t *obj_ctxt;

    SDK_ASSERT_RETURN((batch_ctxt_.stage == API_BATCH_STAGE_OBJ_DEPENDENCY),
                      sdk::SDK_RET_INVALID_ARG);
    // walk over all the dirty objects and reserve resources, if needed
    batch_ctxt_.stage = API_BATCH_STAGE_RESERVE_RESOURCES;
    for (auto it = batch_ctxt_.dol.begin();
         it != batch_ctxt_.dol.end(); ++it) {
        api_obj = *it;
        obj_ctxt = batch_ctxt_.dom[*it];
        ret = reserve_resources_(*it, obj_ctxt);
        if (ret != SDK_RET_OK) {
            goto error;
        }
        // if this object needs to be circulated, add it to the msg list
        if ((obj_ctxt->api_op != API_OP_NONE) && api_obj->circulate()) {
            populate_api_msg_(api_obj, obj_ctxt);
        }
    }
    return SDK_RET_OK;

error:

    return ret;
}

sdk_ret_t
api_engine::program_config_(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t     ret;

    switch (obj_ctxt->api_op) {
    case API_OP_NONE:
        // no-op, but during ACTIVATE_EPOCH/ABORT stage, free the object
        SDK_ASSERT(obj_ctxt->cloned_obj == NULL);
        break;

    case API_OP_CREATE:
        // call program_create() callback on the object, note that the object
        // should have been in the s/w db already by this time marked as dirty
        SDK_ASSERT(obj_ctxt->cloned_obj == NULL);
        ret = api_obj->program_create(obj_ctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_API_PGMCFG_CREATE_COUNTER_INC(err, 1);
            goto error;
        }
        PDS_API_PGMCFG_CREATE_COUNTER_INC(ok, 1);
        break;

    case API_OP_DELETE:
        // call cleanup_config() callback on existing object and mark it for
        // deletion
        // NOTE:
        // 1. cloned_obj, if one exists, doesn't matter and needs to be freed
        //    eventually
        // 2. delete is same as updating the h/w entries with latest epoch,
        //    which will be activated later in the commit() stage (by
        //    programming tables in stage 0, if needed)
        // 3. if the datapath packs multi-epoch data in same entry, calling
        //    cleanup_config() callback is the right thing here as this doesn't
        //    affect traffic at this stage until activation stage later on ...
        //    however, if the datapath doesn't pack multi-epoch data in same
        //    entry, right thing to do here is to not do anything (and that
        //    means not even cleaning up stage0 tables) as touching any stage0
        //    or non-stage0 tables here will affect the packets (and that is
        //    allowed only in activation stage)
        ret = api_obj->cleanup_config(obj_ctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_API_PGMCFG_DELETE_COUNTER_INC(err, 1);
            goto error;
        }
        PDS_API_PGMCFG_DELETE_COUNTER_INC(ok, 1);
        break;

    case API_OP_UPDATE:
        // call program_update() callback on the cloned object so new config
        // can be programmed in the h/w everywhere except stage0, which will
        // be programmed during commit() stage later
        // NOTE: during commit() stage, for update case, we will swap new
        //       obj with old/current one in the all the dbs as well before
        //       activating epoch is activated in h/w stage 0 (and old
        //       object should be freed back)
        SDK_ASSERT(obj_ctxt->cloned_obj != NULL);
        ret = obj_ctxt->cloned_obj->program_update(api_obj, obj_ctxt);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_API_PGMCFG_UPDATE_COUNTER_INC(err, 1);
            goto error;
        }
        PDS_API_PGMCFG_UPDATE_COUNTER_INC(ok, 1);
        break;

    default:
        SDK_ASSERT_RETURN(FALSE, sdk::SDK_RET_INVALID_OP);
    }

    // remember that hw is dirtied at this time, so we can rollback if needed
    // additionally, if there is a case where hw entry needs to be restored to
    // its original state (e.g., in the event of rollback) object implementation
    // of the above callbacks must read current state of respective entries from
    // hw (as the original spec is not available anymore), stash them in the
    // api_obj_ctxt_t, so the same state can be rewritten back to hw
    obj_ctxt->hw_dirty = 1;
    return SDK_RET_OK;

error:

    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failure in program config stage, "
                      "obj %s, op %u, err %u", api_obj->key2str().c_str(),
                      obj_ctxt->api_op, ret);
    }
    return ret;
}

sdk_ret_t
api_engine::program_config_stage_(void) {
    sdk_ret_t    ret;

    SDK_ASSERT_RETURN((batch_ctxt_.stage == API_BATCH_STAGE_RESERVE_RESOURCES),
                      sdk::SDK_RET_INVALID_ARG);

    // walk over all the dirty objects and program hw, if any
    batch_ctxt_.stage = API_BATCH_STAGE_PROGRAM_CONFIG;
    for (auto it = batch_ctxt_.dol.begin();
         it != batch_ctxt_.dol.end(); ++it) {
        ret = program_config_(*it, batch_ctxt_.dom[*it]);
        if (ret != SDK_RET_OK) {
            break;
        }
    }
    return ret;
}

sdk_ret_t
api_engine::activate_config_(dirty_obj_list_t::iterator it,
                             api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t    ret;

    PDS_TRACE_DEBUG("Activating API op %u on %s", obj_ctxt->api_op,
                    api_obj->key2str().c_str());
    // clear the circulate flag, in case it is set
    api_obj->clear_circulate();
    switch (obj_ctxt->api_op) {
    case API_OP_NONE:
        // only case where a dirty obj ends up with this opcode is when new
        // object is created and (eventually) deleted in the same batch
        SDK_ASSERT(obj_ctxt->cloned_obj == NULL);
        api_obj->del_from_db();
        del_from_dirty_list_(it, api_obj);
        api_base::free(obj_ctxt->obj_id, api_obj);
        api_obj_ctxt_free_(obj_ctxt);
        break;

    case API_OP_CREATE:
        // object is already in the database by now, just trigger stage0
        // programming and clear the dirty flag on the object
        SDK_ASSERT(obj_ctxt->cloned_obj == NULL);
        ret = api_obj->activate_config(batch_ctxt_.epoch, API_OP_CREATE,
                                       api_obj, obj_ctxt);
        SDK_ASSERT(ret == SDK_RET_OK);
        del_from_dirty_list_(it, api_obj);
        // config is accepted, clear restore flag
        api_obj->clear_in_restore_list();
        if (api_base::stateless(obj_ctxt->obj_id)) {
            // destroy this object as it is not needed anymore
            PDS_TRACE_VERBOSE("Doing soft delete of stateless obj %s",
                              api_obj->key2str().c_str());
            api_base::soft_delete(obj_ctxt->obj_id, api_obj);
        }
        api_obj_ctxt_free_(obj_ctxt);
        PDS_API_ACTCFG_CREATE_COUNTER_INC(ok, 1);
        break;

    case API_OP_DELETE:
        // we should have programmed hw entries with latest epoch and bit
        // indicating the entry is invalid already by now (except stage 0),
        // so reflect the object deletion in stage 0 now, remove the object
        // from all s/w dbs and enqueue for delay deletion (until then table
        // indices won't be freed, because there could be packets
        // circulating in the pipeline that picked older epoch still
        // NOTE:
        // 1. s/w can't access this obj anymore from this point onwards by
        //    doing db lookups
        ret = api_obj->activate_config(batch_ctxt_.epoch, API_OP_DELETE,
                                       api_obj, obj_ctxt);
        SDK_ASSERT(ret == SDK_RET_OK);
        api_obj->del_from_db();
        del_from_dirty_list_(it, api_obj);
        api_obj->delay_delete();
        // if a cloned object exits, simply free the memory (no need to do delay
        // delete here because this cloned object was never part of object db)
        if (obj_ctxt->cloned_obj) {
            api_base::free(obj_ctxt->obj_id, obj_ctxt->cloned_obj);
        }
        api_obj_ctxt_free_(obj_ctxt);
        PDS_API_ACTCFG_DELETE_COUNTER_INC(ok, 1);
        break;

    case API_OP_UPDATE:
        // other than stage 0 of datapath pipeline, all stages are updated
        // with this epoch, so update stage 0 now; but before doing that we
        // need to switch the latest epcoh in s/w, otherwise packets can
        // come to FTEs with new epoch even before s/w swich is done
        // NOTE:
        // 1. update_db() and activate_config() configs are called on cloned
        //    object
        ret = obj_ctxt->cloned_obj->update_db(api_obj, obj_ctxt);
        SDK_ASSERT(ret == SDK_RET_OK);
        ret = obj_ctxt->cloned_obj->activate_config(batch_ctxt_.epoch,
                                                    API_OP_UPDATE, api_obj,
                                                    obj_ctxt);
        SDK_ASSERT(ret == SDK_RET_OK);
        // enqueue the current (i.e., old) object for delay deletion, note that
        // the current obj is already deleted from the s/w db and swapped with
        // cloned_obj when update_db() was called on cloned_obj above
        del_from_dirty_list_(it, api_obj);
        // stateless cloned objects can be destroyed at this point
        if (api_base::stateless(obj_ctxt->obj_id)) {
            // destroy cloned object as it is not needed anymore
            PDS_TRACE_VERBOSE("Doing soft delete of stateless obj %s",
                              api_obj->key2str().c_str());
            api_base::soft_delete(obj_ctxt->obj_id, obj_ctxt->cloned_obj);
        }
        // if we allocated resources for the updated object, we need to release
        // the resources being used by original object as they are not needed
        // anymore
        if (obj_ctxt->cloned_obj->rsvd_rsc()) {
            // cloned object either took over (some or all of) the resources
            // from the original object and/or allocated its own resources,
            // we must now nuke all the resources that the original object
            // is holding and free the original object
            PDS_TRACE_DEBUG("Deleting original object & cleaning up left over "
                            "resources");
            api_obj->delay_delete();
        } else {
            // just free the original/built object (if object type is stateless)
            // and not nuke it's resources as the cloned object is re-using all
            // the original object's resources
            api_base::free(obj_ctxt->obj_id, api_obj);
        }
        api_obj_ctxt_free_(obj_ctxt);
        PDS_API_ACTCFG_UPDATE_COUNTER_INC(ok, 1);
        break;

    default:
        return sdk::SDK_RET_INVALID_OP;
    }
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::activate_config_stage_(void) {
    sdk_ret_t                     ret;
    api_obj_ctxt_t                *octxt;
    dirty_obj_list_t::iterator    dol_next_it;
    dep_obj_list_t::iterator      aol_next_it;

    // walk over all the dirty objects and activate their config in hw, if any
    batch_ctxt_.stage = API_BATCH_STAGE_CONFIG_ACTIVATE;
    for (auto it = batch_ctxt_.dol.begin(), dol_next_it = it;
         it != batch_ctxt_.dol.end(); it = dol_next_it) {
        dol_next_it++;
        octxt = batch_ctxt_.dom[*it];
        ret = activate_config_(it, *it, octxt);
        SDK_ASSERT(ret == SDK_RET_OK);
    }

    // walk over all the dependent objects and reprogram hw, if any
    for (auto it = batch_ctxt_.aol.begin();
         it != batch_ctxt_.aol.end(); ++it) {
        ret = (*it)->reprogram_config(batch_ctxt_.aom[*it]);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_API_REPGMCFG_UPDATE_COUNTER_INC(err, 1);
            goto error;
        }
        PDS_API_REPGMCFG_UPDATE_COUNTER_INC(ok, 1);
    }

    // walk over all the dependent objects & re-activate their config hw, if any
    for (auto it = batch_ctxt_.aol.begin(), aol_next_it = it;
         it != batch_ctxt_.aol.end(); it = aol_next_it) {
        aol_next_it++;
        octxt = batch_ctxt_.aom[*it];
        ret = (*it)->reactivate_config(batch_ctxt_.epoch, batch_ctxt_.aom[*it]);
        del_from_deps_list_(it, *it);
        api_obj_ctxt_free_(octxt);
        if (unlikely(ret != SDK_RET_OK)) {
            PDS_API_RE_ACTCFG_UPDATE_COUNTER_INC(err, 1);
            break;
        }
        PDS_API_RE_ACTCFG_UPDATE_COUNTER_INC(ok, 1);
    }

error:

    return ret;
}

sdk_ret_t
api_engine::rollback_config_(dirty_obj_list_t::iterator it, api_base *api_obj,
                             api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t    ret = SDK_RET_OK;

    // clear the circulate flag, in case it is set
    api_obj->clear_circulate();
    switch (obj_ctxt->api_op) {
    case API_OP_NONE:
        // only case where a dirty obj ends up with this opcode is when new
        // object is created and (eventually) deleted in the same batch. so far
        // we haven't reserved any h/w resources (and so not done any h/w
        // programming) so we can just delete the obj from db(s) and delay
        // delete the slab object
        SDK_ASSERT(obj_ctxt->cloned_obj == NULL);
        api_obj->del_from_db();
        del_from_dirty_list_(it, api_obj);
        api_base::free(obj_ctxt->obj_id, api_obj);
        break;

    case API_OP_CREATE:
        // so far we did the following for the API_OP_CREATE:
        // 1. instantiated the object (both stateful and stateless) and
        //    iniitialized config (in s/w)
        // 2. added the object to the db(s)
        // 3. added the object to the dirty list/map
        // 4. potentially reserved some resources needed for the object
        // 5. programmed the h/w (if program_config_stage_() got to this object
        //    in dirty list) before failing (i.e., set hw_dirty bit to true)
        // so, now we have to undo these actions
        SDK_ASSERT(obj_ctxt->cloned_obj == NULL);
        if (obj_ctxt->hw_dirty) {
            api_obj->cleanup_config(obj_ctxt);
            obj_ctxt->hw_dirty = 0;
        }
        // TODO contemplate checking restore list, clear this object from
        //      dirty list and prevent cleanup, for now log a message
        if (api_obj->in_restore_list()) {
            PDS_TRACE_ERR("Restored object %s id %u is being rolled back "
                          "resources will be released",
                          api_obj->key2str().c_str(), obj_ctxt->obj_id);
        }
        if (api_obj->rsvd_rsc()) {
            api_obj->release_resources();
        }
        del_from_dirty_list_(it, api_obj);
        // for both stateful and stateless objects, we have to remove the
        // object from db and free the object
        api_obj->del_from_db();
        api_base::free(obj_ctxt->obj_id, api_obj);
        // TODO:
        // As a cleanup
        // soft_delete() can be implemented on all objects by doing
        // 1. api_obj->del_from_db();
        // 2. api_base::free(obj_ctxt->obj_id, api_obj);
        // and use soft_delete() here instead of making 2 calls
        break;

    case API_OP_DELETE:
        // so far we did the following for the API_OP_DELETE:
        // 1. for stateless objs, we built the object on the fly and added to db
        // 2. added the object to the dirty list/map
        // 3. potentially cloned the original object (UPD-XXX-DEL case)
        // 4. potentially called cleanup_config() to clear (non-stage0) entries
        //    in hw (i.e., set hw_dirty bit to true)
        // so, now we have to undo these actions
        if (obj_ctxt->hw_dirty) {
            api_obj->program_create(obj_ctxt);
            obj_ctxt->hw_dirty = 0;
        }
        // if a cloned object exits, simply free the memory (no need to do delay
        // delete here because this cloned object was never part of object db)
        if (obj_ctxt->cloned_obj) {
            api_base::free(obj_ctxt->obj_id, obj_ctxt->cloned_obj);
        }
        del_from_dirty_list_(it, api_obj);
        if (api_base::stateless(obj_ctxt->obj_id)) {
            PDS_TRACE_VERBOSE("Doing soft delete of stateless obj %s",
                              api_obj->key2str().c_str());
            api_base::soft_delete(obj_ctxt->obj_id, api_obj);
        }
        break;

    case API_OP_UPDATE:
        // so far we did the following for the API_OP_UPDATE:
        // 1. for stateless objs, we built the object on the fly and added to db
        // 2. cloned the original/built (if object is stateless) object
        // 3. added original/built object to dirty list
        // 4. updated cloned obj's s/w cfg to cfg specified in latest update
        // 5. potentially reserved resources for the cloned obj
        // 6. potentially called program_update() on cloned obj (i.e., set
        //    hw_dirty to true)
        // so, now we have to undo these actions
        if (obj_ctxt->hw_dirty) {
            obj_ctxt->cloned_obj->cleanup_config(obj_ctxt);
            // we can't call program_create() here as there is no full spec,
            // expectation is that the program_update() on cloned object
            // earlier saved the original state of the table entries in the
            // datapath (e.g., by reading before modifying)
            // TODO: perhaps we need a new callback like restore_cfg() as we
            //       have to differentiate between this case and the case where
            //       program_update() is called in program_config_() function -
            //       in this case, spec should not be used in program_update()
            //       where as in program_config_()_ case spec must be used
            api_obj->program_update(api_obj, obj_ctxt);
            obj_ctxt->hw_dirty = 0;
        }
        if (obj_ctxt->cloned_obj->rsvd_rsc()) {
            obj_ctxt->cloned_obj->release_resources();
        }
        del_from_dirty_list_(it, api_obj);
        api_base::free(obj_ctxt->obj_id, obj_ctxt->cloned_obj);
        if (api_base::stateless(obj_ctxt->obj_id)) {
            PDS_TRACE_VERBOSE("Doing soft delete of stateless obj %s",
                              api_obj->key2str().c_str());
            api_base::soft_delete(obj_ctxt->obj_id, api_obj);
        }
        break;

    default:
        ret = sdk::SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

sdk_ret_t
api_engine::batch_abort_(void) {
    ::core::event_t event;
    api_obj_ctxt_t *octxt;
    sdk_ret_t ret = SDK_RET_OK;
    dirty_obj_list_t::iterator next_it;
    dep_obj_list_t::iterator aol_next_it;

    PDS_API_ABORT_COUNTER_INC(abort, 1);
    PDS_TRACE_DEBUG("Initiating batch abort for epoch %u", batch_ctxt_.epoch);
    // beyond API_BATCH_STAGE_PROGRAM_CONFIG stage is a point of no return
    SDK_ASSERT_RETURN((batch_ctxt_.stage <= API_BATCH_STAGE_PROGRAM_CONFIG),
                      sdk::SDK_RET_INVALID_ARG);
    batch_ctxt_.stage = API_BATCH_STAGE_ABORT;

    PDS_TRACE_INFO("Starting config rollback stage");

    // send a notification announcing that the API batch is being aborted
    event.event_id = EVENT_ID_PDS_API_BATCH_ABORT;
    event.batch.epoch = batch_ctxt_.epoch;
    sdk::ipc::broadcast(EVENT_ID_PDS_API_BATCH_ABORT,
                        &event, sizeof(event));

    // rollback objects in dirty list
    for (auto it = batch_ctxt_.dol.begin(), next_it = it;
         it != batch_ctxt_.dol.end(); it = next_it) {
        next_it++;
        octxt = batch_ctxt_.dom[*it];
        ret = rollback_config_(it, *it, octxt);
        api_obj_ctxt_free_(octxt);
        SDK_ASSERT(ret == SDK_RET_OK);
    }

    // rollback objects in dependent list
    for (auto it = batch_ctxt_.aol.begin(), aol_next_it = it;
         it != batch_ctxt_.aol.end(); it = aol_next_it) {
        aol_next_it++;
        octxt = batch_ctxt_.aom[*it];
        del_from_deps_list_(it, *it);
        api_obj_ctxt_free_(octxt);
    }

    PDS_TRACE_INFO("Finished config rollback stage");

    // end the table mgmt. lib transaction
    PDS_TRACE_INFO("Initiating transaction end for epoch %u",
                   batch_ctxt_.epoch);
    state_->transaction_end(true);
    ret = impl_base::pipeline_impl()->transaction_end();
    if (ret != SDK_RET_OK) {
        PDS_API_ABORT_COUNTER_INC(txn_end_err, 1);
        PDS_TRACE_ERR("Transaction end API failure, err %u", ret);
        return ret;
    }
    batch_ctxt_.clear();
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::promote_container_objs_ (void) {
    api_base *api_obj;
    api_obj_ctxt_t *octxt;

    // walk over all the dependent objects
    // TODO: handle this as part of obj_dependency_computation_stage_() itself
    //       to avoid extra walk
    for (auto it = batch_ctxt_.aol.begin(), aol_next_it = it;
         it != batch_ctxt_.aol.end(); it = aol_next_it) {
        aol_next_it++;
        api_obj = *it;
        octxt = batch_ctxt_.aom[api_obj];
        if (api_base::container(octxt->obj_id) && octxt->clist.size()) {
            // we need to move to this dirty obj list and map
            del_from_deps_list_(it, api_obj);
            // clone the object so we can do regular object update and promote
            // this object to dirty object list
            SDK_ASSERT(octxt->cloned_obj == NULL);
            octxt->cloned_obj = api_obj->clone();
            if (unlikely(octxt->cloned_obj == NULL)) {
                PDS_TRACE_ERR("Clone failed for obj %s",
                              api_obj->key2str().c_str());
                PDS_API_OBJ_DEP_UPDATE_COUNTER_INC(obj_clone_err, 1);
                return SDK_RET_OBJ_CLONE_ERR;
            }
            // allocate api params, it will be used to populate spec while
            // handling this container object update
            octxt->api_params = (api_params_t *)api_params_slab()->alloc();
            if (octxt->api_params == NULL) {
                PDS_TRACE_ERR("Failed to allocate api params for obj %s",
                              api_obj->key2str().c_str());
                PDS_API_OBJ_DEP_UPDATE_COUNTER_INC(obj_clone_err, 1);
                return SDK_RET_OBJ_CLONE_ERR;
            }
            octxt->promoted = TRUE;
            add_to_dirty_list_(api_obj, octxt);
            PDS_TRACE_VERBOSE("Promoted %s from aol to dol",
                              api_obj->key2str().c_str());
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::batch_commit_phase1_(void) {
    sdk_ret_t ret;

    // pre process the APIs by walking over the stashed API contexts to form
    // dirty object list
    PDS_TRACE_INFO("Preprocess stage start, epoch %u, API count %lu",
                   batch_ctxt_.epoch,  batch_ctxt_.api_ctxts->size());
    ret = pre_process_stage_();
    PDS_TRACE_INFO("Preprocess stage end, epoch %u, result %u",
                   batch_ctxt_.epoch, ret);
    if (ret != SDK_RET_OK) {
        goto error;
    }
    PDS_TRACE_INFO("Dirty object list size %lu, Dirty object map size %lu",
                   batch_ctxt_.dol.size(), batch_ctxt_.dom.size());

    // walk over the dirty object list and compute the objects that could be
    // effected because of dirty list objects
    PDS_TRACE_INFO("Starting object dependency computation stage for epoch %u",
                   batch_ctxt_.epoch);
    ret = obj_dependency_computation_stage_();
    PDS_TRACE_INFO("Finished object dependency computation stage for epoch %u",
                   batch_ctxt_.epoch);
    if (ret != SDK_RET_OK) {
        goto error;
    }
    PDS_TRACE_INFO("Dependency object list size %lu, map size %lu",
                   batch_ctxt_.aol.size(), batch_ctxt_.aom.size());

    // we have to promote container objects in the dependent object list
    // to dirty object list (before resource reservation phase)
    // NOTE: after this is done, no container object affected by the contained
    //       object will be in the affected object list
    ret = promote_container_objs_();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("container object promotion failed, err %u",
                      ret);
        goto error;
    }

    // start table mgmt. lib transaction
    PDS_TRACE_INFO("Initiating transaction begin for epoch %u",
                   batch_ctxt_.epoch);
    state_->transaction_begin();
    impl_base::pipeline_impl()->transaction_begin();

    PDS_TRACE_INFO("Starting resource reservation phase");
    // NOTE: by this time, all updates per object are already
    //       computed and hence we should be able to reserve
    //       all necessary resources
    ret = resource_reservation_stage_();
    PDS_TRACE_INFO("Finished resource reservation phase");
    if (ret != SDK_RET_OK) {
        goto error;
    }
    return SDK_RET_OK;

error:

    PDS_TRACE_ERR("Batch commit phase 1 failed, err %u", ret);
    return ret;
}

sdk_ret_t
api_engine::send_ntfn_msgs_(void) {
    for (auto it = batch_ctxt_.msg_map.begin();
         it != batch_ctxt_.msg_map.end(); it++) {
        if (it->second.ntfn_msgs) {
            // this IPC peer is interested in notification msgs
            // NOTE: we ned to define one event per recipient, otherwise
            //       all IPC endpoints get all notifications !!!
            sdk::ipc::broadcast(EVENT_ID_PDS_CFG_OBJ_SET,
                                it->second.ntfn_msgs,
                                core::pds_msg_list_size(it->second.ntfn_msgs));
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::batch_commit_phase2_(void) {
    sdk_ret_t ret;
    ::core::event_t event;

    // walk over the dirty object list, performe the de-duped operation on
    // each object including allocating resources and h/w programming (with the
    // exception of stage 0 programming
    PDS_TRACE_INFO("Starting program config stage for epoch %u",
                   batch_ctxt_.epoch);
    ret = program_config_stage_();
    PDS_TRACE_INFO("Finished program config stage for epoch %u",
                   batch_ctxt_.epoch);
    if (ret != SDK_RET_OK) {
        goto error;
    }

    // activate the epoch in h/w & s/w by programming stage0 tables, if any
    PDS_TRACE_INFO("Starting activate config stage for epoch %u",
                   batch_ctxt_.epoch);
    ret = activate_config_stage_();
    PDS_TRACE_INFO("Finished activate config stage for epoch %u",
                   batch_ctxt_.epoch);
    if (ret != SDK_RET_OK) {
        goto error;
    }

    // end the table mgmt. lib transaction
    PDS_TRACE_INFO("Initiating transaction end for epoch %u",
                   batch_ctxt_.epoch);
    state_->transaction_end(false);
    ret = impl_base::pipeline_impl()->transaction_end();
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Transaction end API failure, err %u", ret);
        // fall thru
    }

    // send API notification msgs to IPC peers interested in this batch
    send_ntfn_msgs_();

    // send a notification announcing that the API batch is committed now
    event.event_id = EVENT_ID_PDS_API_BATCH_COMMIT;
    event.batch.epoch = batch_ctxt_.epoch;
    sdk::ipc::broadcast(EVENT_ID_PDS_API_BATCH_COMMIT,
                        &event, sizeof(event));

    // update the epoch to current epoch
    PDS_TRACE_INFO("Advancing from epoch %u to epoch %u",
                   g_current_epoch_, batch_ctxt_.epoch);
    g_current_epoch_ = batch_ctxt_.epoch;
    SDK_ASSERT(batch_ctxt_.dol.size() == 0);
    SDK_ASSERT(batch_ctxt_.dom.size() == 0);
    SDK_ASSERT(batch_ctxt_.aol.size() == 0);
    SDK_ASSERT(batch_ctxt_.aom.size() == 0);
    batch_ctxt_.clear();
    return ret;

error:

    PDS_TRACE_ERR("Batch commit phase 2 failed, err %u", ret);
    return ret;
}

void
api_engine::process_ipc_async_result_(sdk::ipc::ipc_msg_ptr msg,
                                      const void *ctxt) {
    sdk_ret_t ret;
    ipc_msg_ptr ipc_msg;

    if (msg == nullptr) {
        ret = SDK_RET_TIMEOUT;
    } else {
        ret = *(sdk_ret_t *)msg->data();
    }

    // get the original request msg from the app
    ipc_msg = g_api_engine.ipc_msg();
    if (ret != SDK_RET_OK) {
        SDK_TRACE_DEBUG("Rcvd failure response to PDS_MSG_TYPE_CFG_OBJ_SET, "
                        "aborting batch processing ..., err %u", ret);
        // abort the batch
        g_api_engine.batch_abort_();
        // send the response back to the caller
        sdk::ipc::respond(ipc_msg, &ret, sizeof(ret));
    } else {
        // got SUCCESS from this peer, check if there are more peers to
        // send cfg messages to
        g_api_engine.batch_ctxt_.msg_map_it++;
        if (g_api_engine.batch_ctxt_.msg_map_it ==
                g_api_engine.batch_ctxt_.msg_map.end()) {
            // received OK responses from all the peers and now we can resume
            // processing the API batch
            ret = g_api_engine.batch_commit_phase2_();
            if (ret != SDK_RET_OK) {
                // abort the batch
                g_api_engine.batch_abort_();
            }
            // send the response back to the caller
            sdk::ipc::respond(ipc_msg, &ret, sizeof(ret));
        } else {
            // we need to notify the next IPC endpoint with API objects of its
            // interest
            ret = send_req_rsp_msgs_(g_api_engine.batch_ctxt_.msg_map_it->first,
                      &g_api_engine.batch_ctxt_.msg_map_it->second);
            // sync messages end up returning SDK_RET_IN_PROGRESS we go back to
            // event loop waiting for the response
            if (likely(ret != SDK_RET_OK)) {
                return;
            }
        }
    }
}

sdk_ret_t
api_engine::send_req_rsp_msgs_(pds_ipc_id_t ipc_id,
                               ipc_peer_api_msg_info_t *api_msg_info) {
    // if IPC is mocked out, don't send msg to any IPC endpoints
    if (g_pds_state.ipc_mock()) {
        return SDK_RET_OK;
    }
    if (api_msg_info->req_rsp_msgs) {
        PDS_TRACE_DEBUG("Sending cfg msg to %u, num objs %u", ipc_id,
                        api_msg_info->num_req_rsp_msgs);
        sdk::ipc::request(ipc_id, PDS_MSG_TYPE_CFG_OBJ_SET,
                          api_msg_info->req_rsp_msgs,
                          core::pds_msg_list_size(api_msg_info->req_rsp_msgs),
                          process_ipc_async_result_, NULL,
                          PDS_API_THREAD_MAX_REQUEST_WAIT_TIMEOUT);
        return SDK_RET_IN_PROGRESS;
    }
    return SDK_RET_OK;
}

sdk_ret_t
api_engine::batch_commit(api_msg_t *api_msg, sdk::ipc::ipc_msg_ptr ipc_msg) {
    sdk_ret_t    ret;
    batch_info_t *batch = &api_msg->batch;

    // init batch context
    batch_ctxt_.ipc_msg = ipc_msg;
    batch_ctxt_.epoch = batch->epoch;
    batch_ctxt_.stage = API_BATCH_STAGE_INIT;
    batch_ctxt_.api_ctxts = &batch->apis;

    ret = batch_commit_phase1_();
    if (ret != SDK_RET_OK) {
        goto error;
    }

    // if we need to send any msgs to other IPC endpoints, send it now
    // and get OK or N-OK from them
    if (!batch_ctxt_.msg_map.empty()) {
        batch_ctxt_.msg_map_it = batch_ctxt_.msg_map.begin();
        ret = send_req_rsp_msgs_(batch_ctxt_.msg_map_it->first,
                                 &batch_ctxt_.msg_map_it->second);
        // sync messages end up returning SDK_RET_IN_PROGRESS we go back to
        // event loop waiting for the response
        if (likely(ret != SDK_RET_OK)) {
            return ret;
        }
    }

    // this batch of APIs can be processed further as we don't need to wait to
    // hear from other components
    ret = batch_commit_phase2_();
    if (ret != SDK_RET_OK) {
        goto error;
    }
    return SDK_RET_OK;

error:

    batch_abort_();
    return ret;
}

sdk_ret_t
api_engine::dump_api_counters(int fd) {
    std::string stats;

    dprintf(fd, "%s\n", std::string(50, '-').c_str());
    dprintf(fd, "%-15s%-25s%-10s\n",
            "Stage", "Statistic", "Count");
    dprintf(fd, "%s\n", std::string(50, '-').c_str());

    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "create-ok",
            counters_.preprocess.create.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "create-oom-err",
            counters_.preprocess.create.oom_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "create-init-cfg-err",
            counters_.preprocess.create.init_cfg_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "create-obj-clone-err",
            counters_.preprocess.create.obj_clone_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "create-obj-exists-err",
            counters_.preprocess.create.obj_exists_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "create-invalid-op-err",
            counters_.preprocess.create.invalid_op_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "create-invalid-upd-err",
            counters_.preprocess.create.invalid_upd_err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "delete-ok",
            counters_.preprocess.del.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "delete-obj-build-err",
            counters_.preprocess.del.obj_build_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "delete-not-found-err",
            counters_.preprocess.del.not_found_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "delete-invalid-op-err",
            counters_.preprocess.del.invalid_op_err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "update-ok",
            counters_.preprocess.upd.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "update-obj-build-err",
            counters_.preprocess.upd.obj_build_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "update-init-cfg-err",
            counters_.preprocess.upd.init_cfg_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "update-obj-clone-err",
            counters_.preprocess.upd.obj_clone_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "update-not-found-err",
            counters_.preprocess.upd.not_found_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "update-invalid-op-err",
            counters_.preprocess.upd.invalid_op_err);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "PreProcess", "update-invalid-upd-err",
            counters_.preprocess.upd.invalid_upd_err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ResourceResv", "create-ok",
            counters_.rsv_rsc.create.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ResourceResv", "create-err",
            counters_.rsv_rsc.create.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ResourceResv", "delete-ok",
            counters_.rsv_rsc.del.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ResourceResv", "delete-err",
            counters_.rsv_rsc.del.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ResourceResv", "update-ok",
            counters_.rsv_rsc.upd.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ResourceResv", "update-err",
            counters_.rsv_rsc.upd.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ObjectDep", "update-ok",
            counters_.obj_dep.upd.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ObjectDep", "update-err",
            counters_.obj_dep.upd.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ProgramCfg", "create-ok",
            counters_.pgm_cfg.create.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ProgramCfg", "create-err",
            counters_.pgm_cfg.create.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ProgramCfg", "delete-ok",
            counters_.pgm_cfg.del.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ProgramCfg", "delete-err",
            counters_.pgm_cfg.del.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ProgramCfg", "update-ok",
            counters_.pgm_cfg.upd.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ProgramCfg", "update-err",
            counters_.pgm_cfg.upd.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ActivateCfg", "create-ok",
            counters_.act_cfg.create.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ActivateCfg", "create-err",
            counters_.act_cfg.create.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ActivateCfg", "delete-ok",
            counters_.act_cfg.del.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ActivateCfg", "delete-err",
            counters_.act_cfg.del.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ActivateCfg", "update-ok",
            counters_.act_cfg.upd.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ActivateCfg", "update-err",
            counters_.act_cfg.upd.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ReprogramCfg", "update-ok",
            counters_.re_pgm_cfg.upd.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ReprogramCfg", "update-err",
            counters_.re_pgm_cfg.upd.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "ReactivateCfg", "update-ok",
            counters_.re_act_cfg.upd.ok);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "ReactivateCfg", "update-err",
            counters_.re_act_cfg.upd.err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "Commit", "ipc-err",
            counters_.commit.ipc_err);

    dprintf(fd, "%-15s%-25s%-10u\n",
            "Abort", "abort-err",
            counters_.abort.abort);
    dprintf(fd, "%-15s%-25s%-10u\n",
            "Abort", "txn-end-err",
            counters_.abort.txn_end_err);

    return SDK_RET_OK;
}

sdk_ret_t
api_engine_init (state_base *state)
{
    api_params_init();
    g_api_ctxt_slab_ =
        slab::factory("api-ctxt", PDS_SLAB_ID_API_CTXT,
                      sizeof(api_ctxt_t), 512, true, true, true, NULL);
    g_api_msg_slab_ =
        slab::factory("api-msg", PDS_SLAB_ID_API_MSG,
                      sizeof(api_msg_t), 512, true, true, true, NULL);

    SDK_ASSERT(g_api_engine.init(state) == SDK_RET_OK);

    return SDK_RET_OK;
}

/// \@}

}    // namespace api
