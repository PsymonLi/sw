//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#include "node.h"
#include "cfg.h"

pds_alg_main_t pds_alg_main;

static clib_error_t *
pds_alg_init (vlib_main_t * vm)
{
    clib_memset(&pds_alg_main, 0, sizeof(pds_alg_main_t));
    pds_alg_main.ip4_wildcard_db_ht = hash_create_mem(0, sizeof(pds_alg_ip4_flow_key_t),
                                                      sizeof(u8));
    pds_alg_main.ip6_wildcard_db_ht = hash_create_mem(0, sizeof(pds_alg_ip6_flow_key_t),
                                                      sizeof(u8));

    pds_alg_cfg_init();
    return 0;
}

VLIB_INIT_FUNCTION(pds_alg_init) =
{
    .runs_after = VLIB_INITS("pds_infra_init"),
};
