//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/sdk/lib/eventmgr/eventmgr.hpp"
#include "nic/sdk/lib/slab/slab.hpp"
#include "nic/apollo/agent/core/core.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/agent/core/event.hpp"

namespace core {

#define ADD_TO_OBJ_DB(obj, key, value) {                                       \
    if (key == NULL) {                                                         \
        return SDK_RET_ERR;                                                    \
    }                                                                          \
    obj##_map()->insert(make_pair(*(key), value));                             \
    return SDK_RET_OK;                                                         \
}

#define FIND_IN_OBJ_DB(obj, key) {                                             \
    if (key == NULL) {                                                         \
        return NULL;                                                           \
    }                                                                          \
    obj##_db_t::const_iterator iterator =                                      \
        obj##_map()->find(*key);                                               \
    if (iterator == obj##_map()->end()) {                                      \
        return NULL;                                                           \
    }                                                                          \
    return iterator->second;                                                   \
}

#define DEL_FROM_OBJ_DB(obj, key) {                                            \
    obj##_db_t::const_iterator iterator =                                      \
        obj##_map()->find(*key);                                               \
    obj##_slab()->free(iterator->second);                                      \
    obj##_map()->erase(*key);                                                  \
    return true;                                                               \
}

#define ADD_TO_DB(obj, key, value) {                                           \
    if (key == NULL) {                                                         \
        return SDK_RET_ERR;                                                    \
    }                                                                          \
    obj##_map()->insert(make_pair((uint32_t)key->id, value));                  \
    return SDK_RET_OK;                                                         \
}

#define FIND_IN_DB(obj, key) {                                                 \
    if (key == NULL) {                                                         \
        return NULL;                                                           \
    }                                                                          \
    obj##_db_t::const_iterator iterator =                                      \
        obj##_map()->find((uint32_t)key->id);                                  \
    if (iterator == obj##_map()->end()) {                                      \
        return NULL;                                                           \
    }                                                                          \
    return iterator->second;                                                   \
}

#define DEL_FROM_DB(obj, key) {                                                \
    obj##_db_t::const_iterator iterator =                                      \
        obj##_map()->find((uint32_t)key->id);                                  \
    obj##_slab()->free(iterator->second);                                      \
    obj##_map()->erase((uint32_t)key->id);                                     \
    return true;                                                               \
}

// APIs for cases where key isn't just an id
#define ADD_TO_DB_WITH_KEY(obj, key, value) {                                  \
    obj##_map()->insert(make_pair(*key, value));                               \
    return SDK_RET_OK;                                                         \
}

#define FIND_IN_DB_WITH_KEY(obj, key) {                                        \
    obj##_db_t::const_iterator iterator =                                      \
        obj##_map()->find(*key);                                               \
    if (iterator == obj##_map()->end()) {                                      \
        return NULL;                                                           \
    }                                                                          \
    return iterator->second;                                                   \
}

#define DEL_FROM_DB_WITH_KEY(obj, key) {                                       \
    obj##_map()->erase(*key);                                                  \
    return true;                                                               \
}
#define DB_BEGIN(obj) (obj##_map()->begin())
#define DB_END(obj) (obj##_map()->end())

class agent_state *g_state;

//------------------------------------------------------------------------------
// init() function to instantiate all the config db init state
//------------------------------------------------------------------------------
bool
cfg_db::init(void) {
    void *mem;

    mem = CALLOC(MEM_ALLOC_ID_INFRA, sizeof(service_db_t));
    if (mem == NULL) {
        return false;
    }
    service_map_ = new(mem) service_db_t();

    slabs_[SLAB_ID_IF] =
        slab::factory("if", SLAB_ID_IF, sizeof(pds_if_spec_t),
                      16, true, true, true);
    slabs_[SLAB_ID_SERVICE] =
        slab::factory("service", SLAB_ID_SERVICE, sizeof(pds_svc_mapping_spec_t),
                      16, true, true, true);
    return true;
}

//------------------------------------------------------------------------------
// (private) constructor method
//------------------------------------------------------------------------------
cfg_db::cfg_db() {
    service_map_ = NULL;
    memset(&device_, 0, sizeof(pds_device_spec_t));
    memset(slabs_, 0, sizeof(slabs_));
}

//------------------------------------------------------------------------------
// factory method
//------------------------------------------------------------------------------
cfg_db *
cfg_db::factory(void) {
    void *mem;
    cfg_db *db;

    mem = CALLOC(MEM_ALLOC_ID_INFRA, sizeof(cfg_db));
    if (mem) {
        db = new(mem) cfg_db();
        if (db->init() == false) {
            db->~cfg_db();
            FREE(MEM_ALLOC_ID_INFRA, mem);
            return NULL;
        }
        return db;
    }
    return NULL;
}

//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
cfg_db::~cfg_db() {
    uint32_t i;

    FREE(MEM_ALLOC_ID_INFRA, service_map_);
    for (i = SLAB_ID_MIN; i < SLAB_ID_MAX; i++) {
        if (slabs_[i]) {
            slab::destroy(slabs_[i]);
        }
    }
}

//------------------------------------------------------------------------------
// destroy method
//------------------------------------------------------------------------------
void
cfg_db::destroy(cfg_db *cfg_db) {
    if (!cfg_db) {
        return;
    }
    cfg_db->~cfg_db();
    FREE(MEM_ALLOC_ID_INFRA, cfg_db);
}

//------------------------------------------------------------------------------
// slab walk method
//------------------------------------------------------------------------------
sdk_ret_t
cfg_db::slab_walk(sdk::lib::slab_walk_cb_t walk_cb, void *ctxt) {
    for (uint32_t i = SLAB_ID_MIN; i < SLAB_ID_MAX; i++) {
        if (slabs_[i]) {
            walk_cb(slabs_[i], ctxt);
        }
    }
    return SDK_RET_OK;
}

pds_svc_mapping_spec_t *
agent_state::find_in_service_db(pds_svc_mapping_key_t *key) {
    FIND_IN_DB_WITH_KEY(service, key);
}

sdk_ret_t
agent_state::service_db_walk(service_walk_cb_t cb, void *ctxt) {
    auto it_begin = DB_BEGIN(service);
    auto it_end = DB_END(service);

    for (auto it = it_begin; it != it_end; it ++) {
        cb(it->second, ctxt);
    }
    return SDK_RET_OK;
}

bool
agent_state::del_from_service_db(pds_svc_mapping_key_t *key) {
    DEL_FROM_DB_WITH_KEY(service, key);
}

class agent_state *
agent_state::state(void) {
    return g_state;
}

sdk_ret_t
agent_state::init(void) {
    void *mem;

    mem = CALLOC(MEM_ALLOC_ID_INFRA, sizeof(agent_state));
    if (mem) {
        g_state = new(mem) agent_state();
    }
    SDK_ASSERT_RETURN((g_state != NULL), SDK_RET_OOM);
    g_state->cfg_db_ = cfg_db::factory();
    SDK_ASSERT_GOTO(g_state->cfg_db_, error);
    g_state->evmgr_ = eventmgr::factory(PDS_EVENT_ID_MAX);
    SDK_ASSERT_GOTO(g_state->evmgr_, error);
    g_state->init_mode_ = sdk::upg::upg_init_mode();
    g_state->domain_ = sdk::upg::upg_init_domain();
    return SDK_RET_OK;

error:

    if (g_state->cfg_db_) {
        cfg_db::destroy(g_state->cfg_db_);
    }
    g_state->~agent_state();
    return SDK_RET_OOM;
}

}    // namespace core
