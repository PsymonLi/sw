//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#ifndef __VPP_FLOW_ALG_COMMON_CFG_H__
#define __VPP_FLOW_ALG_COMMON_CFG_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool pds_alg_cfg_init(void);

bool pds_alg_rule_db_add_modify(u32 rule_id, pds_alg_cfg_t *cfg);

bool pds_alg_rule_db_del(u32 rule_id);

#ifdef __cplusplus
}
#endif

#endif  // __VPP_FLOW_ALG_COMMON_CFG_H__
