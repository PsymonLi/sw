//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#ifndef __VPP_IMPL_APULU_FIXUP_H__
#define __VPP_IMPL_APULU_FIXUP_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    PDS_FLOW_IP_DELETE,
    PDS_FLOW_IP_MOVE_L2R,
    PDS_FLOW_IP_MOVE_R2L,
} pds_flow_learn_move_id;

void pds_vpp_learn_subscribe(void);
void pds_ip_flow_fixup(uint32_t id,
                       uint32_t ip_addr,
                       uint16_t bd_id,
                       uint16_t vnic_id,
                       bool del);

#ifdef __cplusplus
}
#endif
#endif  // __VPP_IMPL_APULU_FIXUP_H__
