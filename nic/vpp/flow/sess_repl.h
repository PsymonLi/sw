/*
 *  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
 */

#ifndef __SESS_REPL_H__
#define __SESS_REPL_H__

#include "node.h"

void pds_sess_recv_begin();
void pds_sess_recv_end();
bool pds_sess_v4_recv_cb (const uint8_t *data, const uint8_t len);
void pds_sess_cleanup_all();

typedef struct sess_iter_ {
    bool v4;
    int32_t read_index;
    void *flow_table;
    pds_flow_hw_ctx_t *pool;
    pds_flow_hw_ctx_t *ctx;
} sess_iter_t;

void pds_sess_sync_begin(sess_iter_t *iter, bool v4);
void pds_sess_sync_end(sess_iter_t *iter);
bool pds_sess_v4_sync_cb (uint8_t *data, uint8_t *len, void *opaq);

#endif  /* __SESS_REPL_H__ */
