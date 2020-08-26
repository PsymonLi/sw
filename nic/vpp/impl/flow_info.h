//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_FLOW_INFO_H__
#define __VPP_IMPL_FLOW_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void pds_flow_info_program(uint32_t tbl_id, bool l2l);
void pds_flow_info_l2l_update(uint32_t tbl_id, bool l2l);
void pds_flow_info_get(uint32_t tbl_id, void *flow_info);
void pds_flow_info_cache_entry_add(uint16_t worker_id, bool l2l);
void pds_flow_info_cache_entry_prog(uint16_t worker_id,
                                    uint32_t entry_index,
                                    uint32_t table_index);
void pds_flow_info_cache_reset(uint16_t worker_id);
void pds_flow_info_init(uint16_t workers, uint16_t cache_size);

#ifdef __cplusplus
}
#endif

#endif  // __VPP_IMPL_FLOW_INFO_H__

