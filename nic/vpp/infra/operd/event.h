//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_INFRA_OPERD_EVENT_H__
#define __VPP_INFRA_OPERD_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

void operd_event_vnic_session_limit_exhaustion(const uint8_t *vnic_key,
                                               uint32_t cur, uint32_t max);

void operd_event_vnic_session_limit_approach(const uint8_t *vnic_key,
                                             uint32_t cur, uint32_t max);

void operd_event_vnic_session_within_limit(const uint8_t *vnic_key,
                                           uint32_t cur, uint32_t max);

#ifdef __cplusplus
}
#endif

#endif  // __VPP_INFRA_OPERD_EVENT_H__

