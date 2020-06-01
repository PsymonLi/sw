//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#ifndef __VPP_IMPL_APULU_SESSION_H__
#define __VPP_IMPL_APULU_SESSION_H__

#include <nic/vpp/infra/utils.h>

#ifdef __cplusplus
extern "C" {
#endif

bool pds_session_is_in_use(u32 sess_id);
void pds_flow_delete_session(u32 sess_id);

#ifdef __cplusplus
}
#endif
#endif  // __VPP_IMPL_APULU_SESSION_H__
