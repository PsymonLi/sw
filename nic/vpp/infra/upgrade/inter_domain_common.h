//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

// This file contains function vectors that implement the actual object
// sync and restore during an ISSU upgrade across domains..

#ifndef __INTER_DOMAIN_COMMON_H__
#define __INTER_DOMAIN_COMMON_H__

/* one id per object we want to sync from domain 'A' to 'B' */

typedef enum idipc_obj_id_ {
    IDIPC_OBJ_ID_SESS = 0,
    IDIPC_OBJ_ID_MAX,
} idipc_obj_id_t;

typedef void (*obj_sync_cb)(const char *qname);
typedef void (*obj_rest_cb)(const char *qname);

typedef struct idipc_callbacks_ {
    idipc_obj_id_t id;
    obj_sync_cb sync_cb;
    obj_rest_cb rest_cb;
} idipc_callbacks_t;


void idipc_callbacks_register(idipc_callbacks_t *cb);
void idipc_sync(idipc_obj_id_t obj_id, const char *qname);
void idipc_restore(idipc_obj_id_t obj_id, const char *qname);

#endif  /* __INTER_DOMAIN_COMMON_H__ */
