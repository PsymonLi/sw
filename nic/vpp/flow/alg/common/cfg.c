//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#include "cfg.h"
#include "db.h"

bool
pds_alg_cfg_init (void)
{
    // register functions for policy config
    return true;
}

bool
pds_alg_rule_db_add_modify (u32 rule_id, pds_alg_cfg_t *cfg)
{
    pds_alg_cfg_t *new_data = clib_mem_alloc(sizeof(pds_alg_cfg_t));
    pds_alg_cfg_t *old_data;
    void *proto_cfg = NULL;

    if (!new_data) {
        return false;
    }
    new_data->proto = cfg->proto;
    new_data->idle_timeout = cfg->idle_timeout;
    new_data->proto_cfg = NULL;

    switch (new_data->proto) {
#define _(p, n)                                                         \
    case ALG_PROTOCOL_##p:                                              \
        pds_alg_##n_t *##n = clib_mem_alloc(sizeof(pds_alg_##n_t));     \
        if (!##n) {                                                     \
            goto fail;                                                  \
        }                                                               \
        *##n = *((pds_alg_##n_t *) cfg->proto_cfg);                     \
        new_data->proto_cfg = (void *)##n;                              \
        break;                                                          \
    foreach_alg_protocol
#undef _
    default:
        return false;
    }
    old_data = (pds_alg_cfg_t *) hash_get(pds_alg_main.rule_db_ht, rule_id);
    hash_set(pds_alg_main.rule_db_ht, rule_id, new_data);
    if (old_data) {
        if (old_data->proto_cfg) {
            clib_mem_free(old_data->proto_cfg);
        }
        clib_mem_free(old_data);
    }
    return true;

fail:
    if (new_data) {
        if (new_data->proto_cfg) {
            clib_mem_free(new_data->proto_cfg);
        }
        clib_mem_free(new_data);
    }
    return false;
}

bool
pds_alg_rule_db_del (u32 rule_id)
{
    pds_alg_cfg_t *cfg =
            (pds_alg_cfg_t *) hash_get(pds_alg_main.rule_db_ht, rule_id);

    if (!cfg) {
        return false;
    }
    hash_unset(pds_alg_main.rule_db_ht, rule_id);
    if (cfg->proto_cfg) {
        clib_mem_free(cfg->proto_cfg);
    }
    clib_mem_free(cfg);
    return true;
}
