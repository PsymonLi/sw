//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

// This file contains function vectors that implement the actual object
// sync and restore during an ISSU upgrade across domains..

#include <assert.h>
#include "repl_state_tp_common.h"

static repl_state_tp_cb_t ob_cbs_arr[REPL_STATE_OBJ_ID_MAX];

void
repl_state_tp_callbacks_register (repl_state_tp_cb_t *cb)
{
    assert(cb->id < REPL_STATE_OBJ_ID_MAX);
    ob_cbs_arr[cb->id] = *cb;
}

void
repl_state_tp_sync (repl_state_obj_id_t obj_id, const char *qname)
{
    repl_state_tp_cb_t *cb;

    assert(obj_id < REPL_STATE_OBJ_ID_MAX);
    cb = &ob_cbs_arr[obj_id];
    cb->sync_cb(qname);
}

void
repl_state_tp_restore (repl_state_obj_id_t obj_id, const char *qname)
{
    repl_state_tp_cb_t *cb;

    assert(obj_id < REPL_STATE_OBJ_ID_MAX);
    cb = &ob_cbs_arr[obj_id];
    cb->restore_cb(qname);
}
