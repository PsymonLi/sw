//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//

#include <nic/sdk/include/sdk/ip.hpp>
#include <nic/sdk/include/sdk/table.hpp>
#include <nic/sdk/lib/ipc/ipc.hpp>
#include <nic/apollo/core/event.hpp>
#include <nic/apollo/api/utils.hpp>
#include <nic/vpp/infra/ipc/pdsa_vpp_hdlr.h>
#include <nic/vpp/infra/cfg/pdsa_db.hpp>
#include "log.h"
#include "pdsa_uds_hdlr.h"
#include "fixup.h"

using namespace core;

extern "C" {

static uint32_t
pds_event_id_to_vpp_event_id (event_id_t event_id)
{
    switch (event_id) {
    case EVENT_ID_IP_MOVE_L2R:
        return PDS_FLOW_IP_MOVE_L2R;
    case EVENT_ID_IP_MOVE_R2L:
        return PDS_FLOW_IP_MOVE_R2L;
    case EVENT_ID_IP_DELETE:
    case EVENT_ID_IP_AGE:
        return PDS_FLOW_IP_DELETE;
    default:
        flow_log_error("unhandled move event %u", event_id);
        return -1;
    }
}

int
pds_vpp_bd_hw_id_get (pds_obj_key_t key, uint16_t *bd_hw_id)
{
    pds_cfg_msg_t subnet_obj;

    vpp_config_data &config = vpp_config_data::get();
    subnet_obj.obj_id = OBJ_ID_SUBNET;
    subnet_obj.subnet.key = key;
    if (!config.get(subnet_obj)) {
        return -1;
    }
    *bd_hw_id = subnet_obj.subnet.status.hw_id;
    return 0;
}

static bool
pds_ipc_ip_move_handle (core::event_t *event, bool del_event)
{
    learn_event_info_t *move_evt = &event->learn;
    ip_addr_t ip_addr = move_evt->ip_addr;
    uint32_t ipv4_addr;
    uint16_t bd_hw_id;

    if (likely(ip_addr.af == IP_AF_IPV4)) {
        ipv4_addr = ip_addr.addr.v4_addr;
        if (unlikely(!del_event &&
            pds_vpp_bd_hw_id_get(move_evt->subnet, &bd_hw_id) == -1)) {
            flow_log_error("Subnet[%s] does not exist for move event[%u], "
                           "address 0x%x",
                           move_evt->subnet.str(), event->event_id,
                           move_evt->ip_addr.addr.v4_addr);
            return false;
        }
        flow_log_notice("Received event[%u], IP[0x%x], BD[%u], vnic[%u]",
                        event->event_id, ipv4_addr,
                        bd_hw_id, move_evt->vnic_hw_id);
        pds_ip_flow_fixup(pds_event_id_to_vpp_event_id(event->event_id),
                          ipv4_addr, bd_hw_id, move_evt->vnic_hw_id);
    }

    return true;
}

void
pds_ipc_ip_move_handle_cb (sdk::ipc::ipc_msg_ptr msg, const void *ctx)
{
    core::event_t *event = (core::event_t *) msg->data();

    if (!event) {
        flow_log_error("NULL event");
        return;
    }
    switch (event->event_id) {
    case EVENT_ID_IP_MOVE_L2R:
    case EVENT_ID_IP_MOVE_R2L:
        (void)pds_ipc_ip_move_handle(event, false);
        break;
    case EVENT_ID_IP_DELETE:
    case EVENT_ID_IP_AGE:
        (void)pds_ipc_ip_move_handle(event, true);
        break;
    default:
        flow_log_error("Unhandled move event %u", event->event_id);
        break;
    }
    return;
}

void pds_vpp_learn_subscribe ()
{
    sdk::ipc::subscribe(EVENT_ID_IP_MOVE_R2L, &pds_ipc_ip_move_handle_cb, NULL);
    sdk::ipc::subscribe(EVENT_ID_IP_MOVE_L2R, &pds_ipc_ip_move_handle_cb, NULL);
    sdk::ipc::subscribe(EVENT_ID_IP_DELETE, &pds_ipc_ip_move_handle_cb, NULL);
    sdk::ipc::subscribe(EVENT_ID_IP_AGE, &pds_ipc_ip_move_handle_cb, NULL);
}

}
