//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

// This file contains functions that use the shm ipc transport to
// sync objects from VPP in domain 'A' to 'B' and functions to read
// the objects from IPC and program internal + p4 state. Uses helpers
// defined by implementations (apulu etc)

#include <stdbool.h>
#include <nic/vpp/infra/shm_ipc/shm_ipc.h>
#include <nic/vpp/infra/upgrade/repl_state_tp_common.h>
#include "sess_repl.h"
#include "sess_repl_state_tp.h"

static bool shm_ipc_inited = false;

static void
sess_sync (const char *q_name)
{
    sess_iter_t iter;
    struct shm_ipcq *sess_q;

    if (!shm_ipc_inited) {
        shm_ipc_init();
        shm_ipc_inited = true;
    }
    sess_q = shm_ipc_create(q_name);
    pds_sess_sync_begin(&iter, true);
    while (iter.ctx) {
        shm_ipc_send_start(sess_q, pds_sess_v4_sync_cb, &iter);
    }
    // Flush end-of-record marker
    while (shm_ipc_send_eor(sess_q) == false);
    pds_sess_sync_end(&iter);
}

static bool
sess_recv_cb (const uint8_t *data, const uint8_t len, void *opaq)
{
    bool *v4 = (bool *)opaq;
    
    if (v4) {
        pds_sess_v4_recv_cb(data, len);
    }
    return true;
}

void
sess_restore (const char *q_name)
{
    int ret;
    bool v4 = true;
    struct shm_ipcq *sess_q;

    if (!shm_ipc_inited) {
        shm_ipc_init();
        shm_ipc_inited = true;
    }
    sess_q = shm_ipc_attach(q_name);
    pds_sess_recv_begin();
    while ((ret = shm_ipc_recv_start(sess_q, sess_recv_cb, &v4)) != -1 );
    pds_sess_recv_end();
}

void
sess_repl_state_tp_callbacks_init (void)
{
    repl_state_tp_cb_t cb = {
        .id = REPL_STATE_OBJ_ID_SESS,
        .sync_cb = sess_sync,
        .restore_cb = sess_restore,
    };
    repl_state_tp_callbacks_register(&cb);
}
