//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

// This file contains functions that use the shm ipc transport to
// sync objects from VPP in domain 'A' to 'B' and functions to read
// the objects from IPC and program internal + p4 state. Uses helpers
// defined by implementations (apulu etc)

#include <stdbool.h>
#include <nic/vpp/infra/shm_ipc/shm_ipc.h>
#include "../infra/upgrade/inter_domain_common.h"
#include "node.h"
#include "inter_domain_ipc.h"

static bool shm_ipc_inited = false;

static void
session_sync (const char *qname)
{
    sess_iter_t iter;
    struct shm_ipcq *sessq;

    if (!shm_ipc_inited) {
        shm_ipc_init();
        shm_ipc_inited = true;
    }
    sessq = shm_ipc_create(qname);
    pds_session_send_begin(&iter, true);
    while (iter.ctx) {
        shm_ipc_send_start(sessq, pds_session_v4_send_cb, &iter);
    }
    // Flush end-of-record marker
    while (shm_ipc_send_eor(sessq) == false);
    pds_session_send_end(&iter);
}

static bool
session_recvcb (const uint8_t *data, const uint8_t len, void *opaq)
{
    bool *v4 = (bool *)opaq;
    
    if (v4) {
        pds_session_v4_recv_cb(data, len);
    }
    return true;
}

void
session_restore (const char *qname)
{
    int ret;
    bool v4 = true;
    struct shm_ipcq *sessq;

    if (!shm_ipc_inited) {
        shm_ipc_init();
        shm_ipc_inited = true;
    }
    sessq = shm_ipc_attach(qname);
    pds_session_recv_begin();
    while ((ret = shm_ipc_recv_start(sessq, session_recvcb,
                                     &v4)) != -1 );
    pds_session_recv_end();
}

void
inter_domain_ipc_callbacks_init (void)
{
    idipc_callbacks_t cb = {
        .id = IDIPC_OBJ_ID_SESS,
        .sync_cb = session_sync,
        .rest_cb = session_restore,
    };
    idipc_callbacks_register(&cb);
}
