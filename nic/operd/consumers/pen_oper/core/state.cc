//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//------------------------------------------------------------------------------

#include "nic/sdk/include/sdk/mem.hpp"
#include "state.hpp"
#include "gen/alerts/alert_defs.h"

namespace core {

class pen_oper_state *g_state;

class pen_oper_state *
pen_oper_state::state(void) {
    return g_state;
}

sdk_ret_t
pen_oper_state::init(void) {
    void *mem;

    mem = SDK_CALLOC(SDK_MEM_ALLOC_INFRA, sizeof(pen_oper_state));
    if (mem) {
        g_state = new(mem) pen_oper_state();
    }
    SDK_ASSERT_RETURN((g_state != NULL), SDK_RET_ERR);
    return SDK_RET_OK;
}

pen_oper_state::pen_oper_state() {
    client_mgr_[PENOPER_INFO_TYPE_NONE] = NULL;
    client_mgr_[PENOPER_INFO_TYPE_EVENT] =
        eventmgr::factory(operd::alerts::OPERD_EVENT_TYPE_MAX);
    client_mgr_[PENOPER_INFO_TYPE_MAX] = NULL;
}

pen_oper_state::~pen_oper_state() {
    for (uint32_t i = PENOPER_INFO_TYPE_MIN; i < PENOPER_INFO_TYPE_MAX; i++) {
        eventmgr::destroy(client_mgr_[i]);
    }
}

eventmgr *
pen_oper_state::client_mgr(pen_oper_info_type_t info_type) {
    return client_mgr_[info_type];
}

bool
pen_oper_state::is_client_active(void *ctxt) {
    for (uint32_t info_type = PENOPER_INFO_TYPE_MIN;
         info_type < PENOPER_INFO_TYPE_MAX; info_type++) {
        if (client_mgr_[info_type]->is_listener_active(ctxt)) {
            // client is still interested in this info_type
            return true;
        } else {
            client_mgr_[info_type]->unsubscribe_listener(ctxt);
        }
    }
    // client is not subscribed to any oper info
    return false;
}

sdk_ret_t
pen_oper_state::add_client(pen_oper_info_type_t info_type, uint32_t info_id,
                           void *ctxt) {
    return client_mgr_[info_type]->subscribe(info_id, ctxt);
}

sdk_ret_t
pen_oper_state::delete_client(pen_oper_info_type_t info_type,
                              uint32_t info_id, void *ctxt) {
    return client_mgr_[info_type]->unsubscribe(info_id, ctxt);
}

sdk_ret_t
pen_oper_state::delete_client(pen_oper_info_type_t info_type,
                              void *ctxt) {
    return client_mgr_[info_type]->unsubscribe_listener(ctxt);
}

void
pen_oper_state::delete_client(void *ctxt) {
    for (uint32_t info_type = PENOPER_INFO_TYPE_MIN;
         info_type < PENOPER_INFO_TYPE_MAX; info_type++) {
        client_mgr_[info_type]->unsubscribe_listener(ctxt);
    }
}

}    // namespace core
