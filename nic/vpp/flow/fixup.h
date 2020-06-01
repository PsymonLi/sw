//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#ifndef __VPP_IMPL_APULU_FIXUP_H__
#define __VPP_IMPL_APULU_FIXUP_H__

#ifdef __cplusplus
extern "C" {
#endif

void pds_vpp_learn_subscribe(void);
void pds_ip_flow_fixup(uint32_t id, uint32_t ip_addr, uint16_t bd_id,
                       uint16_t vnic_id);

#ifdef __cplusplus
}
#endif
#endif  // __VPP_IMPL_APULU_FIXUP_H__
