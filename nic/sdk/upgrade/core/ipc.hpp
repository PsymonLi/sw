// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// common definitions for core upgrade data structure for external uses
///
//----------------------------------------------------------------------------

#ifndef __UPGRADE_CORE_IPC_HPP__
#define __UPGRADE_CORE_IPC_HPP__

#include <ev.h>
#include "include/sdk/base.hpp"
#include "lib/ipc/ipc.hpp"
#include "upgrade/include/upgrade.hpp"

namespace sdk {
namespace upg {

typedef enum ipc_svc_dom_id_s {
    IPC_SVC_DOM_ID_NONE = 0, ///< invalid
    IPC_SVC_DOM_ID_A    = 1, ///< first and default domain
    IPC_SVC_DOM_ID_B    = 2, ///< second domain
} ipc_svc_dom_id_t;

static const char *ipc_svc_dom_id_name[] =  {
    [IPC_SVC_DOM_ID_NONE] = "none",
    [IPC_SVC_DOM_ID_A] = "A",
    [IPC_SVC_DOM_ID_B] = "B"
};

static inline const char *
ipc_svc_dom_id_to_name (ipc_svc_dom_id_t id)
{
    return ipc_svc_dom_id_name[id];
}

/// ipc callback which expects service name and id filled by the endpoint
typedef void (*upg_async_response_cb_t)(upg_event_msg_t *event);
/// broadcast to a domain
void upg_send_broadcast_request(ipc_svc_dom_id_t dom_id, upg_stage_t stage,
                                upg_mode_t mode, uint32_t num_svcs,
                                ev_tstamp timeout);
void upg_send_exit_event(upg_mode_t mode);
/// unicast request to a service in a domain
void upg_send_request(ipc_svc_dom_id_t dom_id, upg_stage_t stage,
                      uint32_t svc_id, upg_mode_t mode);
/// initialize the ipc service
void upg_ipc_init(upg_async_response_cb_t response_cb);

}   // namespace upg
}   // namespace sdk

#endif   // __UPGRADE_CORE_IPC_HPP__
