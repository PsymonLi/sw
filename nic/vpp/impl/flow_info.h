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

int pds_flow_info_program(uint32_t ses_id, bool iflow, uint8_t l2l);
int pds_flow_info_update_l2l(uint32_t ses_id, bool iflow, uint8_t l2l);
void pds_flow_info_get_flow_info(uint32_t ses_id, bool iflow, void *flow_info);

#ifdef __cplusplus
}
#endif

#endif  // __VPP_IMPL_FLOW_INFO_H__

