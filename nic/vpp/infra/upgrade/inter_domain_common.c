//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

// This file contains function vectors that implement the actual object
// sync and restore during an ISSU upgrade across domains..

#include <assert.h>
#include "inter_domain_common.h"

static idipc_callbacks_t ob_cbs_arr[IDIPC_OBJ_ID_MAX];

void
idipc_callbacks_register (idipc_callbacks_t *cb)
{
    assert(cb->id < IDIPC_OBJ_ID_MAX);
    ob_cbs_arr[cb->id] = *cb;
}

void
idipc_sync (idipc_obj_id_t obj_id, const char *qname)
{
    idipc_callbacks_t *cb;

    assert(obj_id < IDIPC_OBJ_ID_MAX);
    cb = &ob_cbs_arr[obj_id];
    cb->sync_cb(qname);
}

void
idipc_restore (idipc_obj_id_t obj_id, const char *qname)
{
    idipc_callbacks_t *cb;

    assert(obj_id < IDIPC_OBJ_ID_MAX);
    cb = &ob_cbs_arr[obj_id];
    cb->rest_cb(qname);
}
