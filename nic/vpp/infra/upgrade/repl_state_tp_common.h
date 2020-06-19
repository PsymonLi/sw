//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

// This file contains function vectors that implement the actual object
// sync and restore during an ISSU upgrade across domains..

#ifndef __REPL_STATE_TP_COMMON_H__
#define __REPL_STATE_TP_COMMON_H__

/* one id per object we want to sync from domain 'A' to 'B' */

typedef enum repl_state_obj_id_ {
    REPL_STATE_OBJ_ID_SESS = 0,
    REPL_STATE_OBJ_ID_NAT = 1,
    REPL_STATE_OBJ_ID_MAX,
} repl_state_obj_id_t;

typedef void (*obj_sync_cb)(const char *q_name);
typedef void (*obj_rest_cb)(const char *q_name);

typedef struct repl_state_tp_cb_ {
    repl_state_obj_id_t id;
    obj_sync_cb sync_cb;
    obj_rest_cb restore_cb;
} repl_state_tp_cb_t;


#ifdef __cplusplus
extern "C" {
#endif
void repl_state_tp_callbacks_register(repl_state_tp_cb_t *cb);
void repl_state_tp_sync(repl_state_obj_id_t obj_id, const char *qname);
void repl_state_tp_restore(repl_state_obj_id_t obj_id, const char *qname);
#ifdef __cplusplus
} // extern "C"
#endif

#endif  /* __REPL_STATE_TP_COMMON_H__ */
