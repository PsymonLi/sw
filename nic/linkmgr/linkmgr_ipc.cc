//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "linkmgr_src.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/hal/core/event_ipc.hpp"
#include "linkmgr_event_recorder.hpp"

namespace linkmgr {
namespace ipc {

void
port_event_notify (port_event_info_t *port_event_info)
{
    hal::core::event_t event;
    uint32_t logical_port = port_event_info->logical_port;
    uint32_t ifindex = sdk::lib::catalog::logical_port_to_ifindex(logical_port);

    port_event_t port_event = port_event_info->event;
    port_speed_t port_speed = port_event_info->speed;
    port_fec_type_t fec_type = port_event_info->fec_type;

    sdk::linkmgr::port_set_leds(logical_port, port_event);

    memset(&event, 0, sizeof(event));
    event.event_id = event_id_t::EVENT_ID_PORT_STATUS;
    event.port.id = ifindex;
    event.port.event = port_event;
    event.port.speed = port_speed;
    event.port.fec_type = fec_type;
    sdk::ipc::broadcast(event_id_t::EVENT_ID_PORT_STATUS, &event, sizeof(event));

    // publish to event recorder
    linkmgr::port_event_recorder_notify(port_event_info);
}

static void
send_xcvr_event (xcvr_event_info_t *xcvr_event_info, bool dom)
{
    sdk::types::sdk_event_t sdk_event;

    memcpy(&sdk_event.xcvr_event_info, xcvr_event_info,
           sizeof(xcvr_event_info_t));
    if (dom) {
        sdk_event.event_id =
            sdk_ipc_event_id_t::SDK_IPC_EVENT_ID_XCVR_DOM_STATUS;
    } else {
        SDK_TRACE_DEBUG("send event xcvr status, phy_port %u, ifindex 0x%x, "
                        "type %u, state %u, pid %u, cable_type %u",
                        sdk_event.xcvr_event_info.phy_port,
                        sdk_event.xcvr_event_info.ifindex,
                        sdk_event.xcvr_event_info.type,
                        sdk_event.xcvr_event_info.state,
                        sdk_event.xcvr_event_info.pid,
                        sdk_event.xcvr_event_info.cable_type);
        sdk_event.event_id = sdk_ipc_event_id_t::SDK_IPC_EVENT_ID_XCVR_STATUS;
    }
    sdk::ipc::broadcast(sdk_event.event_id, &sdk_event,
                        sizeof(sdk::types::sdk_event_t));
}

void
xcvr_event_notify (xcvr_event_info_t *xcvr_event_info, bool dom)
{
    send_xcvr_event(xcvr_event_info, dom);
}

}    // namespace ipc
}    // namespace linkmgr
