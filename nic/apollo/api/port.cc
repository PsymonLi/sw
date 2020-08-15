/**
 * Copyright (c) 2019 Pensando Systems, Inc.
 *
 * @file    port.hpp
 *
 * @brief   This file handles port operations
 */

#include "nic/sdk/linkmgr/port.hpp"
#include "nic/sdk/platform/drivers/xcvr.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/sdk/linkmgr/linkmgr.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/infra/core/core.hpp"
#include "nic/infra/core/event.hpp"
#include "nic/infra/operd/alerts/alerts.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/api/if.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/apollo/api/internal/metrics.hpp"
#include "nic/apollo/api/internal/pds_if.hpp"
#include "nic/apollo/api/impl/lif_impl.hpp"

using sdk::linkmgr::linkmgr_async_response_cb_t;

namespace api {

static bool
port_event_lif_cb (void *entry, void *ctxt)
{
    api::impl::lif_impl *lif = (api::impl::lif_impl *)entry;
    event_id_t event_id = *(event_id_t *)ctxt;
    ::core::event_t event;

    // send host dev notification only for host lifs
    if ((lif->type() != lif_type_t::LIF_TYPE_HOST) &&
        (lif->type() != lif_type_t::LIF_TYPE_MNIC_INBAND_MGMT)) {
        return false;
    }

    memset(&event, 0, sizeof(event));
    event.event_id = event_id;
    event.host_dev.id = lif->id();
    sdk::ipc::broadcast(event_id, &event, sizeof(event));
    return false;
}

static void
port_event_lif_notif (sdk_ipc_event_id_t event_id)
{
    lif_db()->walk(port_event_lif_cb, &event_id);
}

static void
port_event_lif_handle (port_event_info_t *port_event_info)
{
    // if this is a link up event and the number of uplinks link up is 1,
    // then send host dev up notification
    if ((port_event_info->event == port_event_t::PORT_EVENT_LINK_UP) &&
            (sdk::linkmgr::num_uplinks_link_up() == 1)) {
        PDS_TRACE_DEBUG("Sending host dev up notification");
        port_event_lif_notif(SDK_IPC_EVENT_ID_HOST_DEV_UP);
    } else if (sdk::linkmgr::num_uplinks_link_up() == 0) {
        // if all uplinks are link down, then send host dev down notification
        PDS_TRACE_DEBUG("All uplinks down, sending host dev down notification");
        port_event_lif_notif(SDK_IPC_EVENT_ID_HOST_DEV_DOWN);
    }
}

/**
 * @brief        Handle link UP/Down events
 * @param[in]    port_event_info port event information
 */
void
port_event_cb (port_event_info_t *port_event_info)
{
    sdk_ret_t ret;
    ::core::event_t event;
    pds_event_t pds_event;
    port_event_t port_event = port_event_info->event;
    port_speed_t port_speed = port_event_info->speed;
    port_fec_type_t fec_type = port_event_info->fec_type;
    uint32_t logical_port = port_event_info->logical_port;

    sdk::linkmgr::port_set_leds(logical_port, port_event);

    // broadcast the event to other components of interest
    memset(&event, 0, sizeof(event));
    event.event_id = EVENT_ID_PORT_STATUS;
    event.port.ifindex =
        sdk::lib::catalog::logical_port_to_ifindex(logical_port);
    event.port.event = port_event;
    event.port.speed = port_speed;
    event.port.fec_type = fec_type;
    sdk::ipc::broadcast(EVENT_ID_PORT_STATUS, &event, sizeof(event));

    // notify the agent
    memset(&pds_event, 0, sizeof(pds_event));
    if (port_event_info->event == port_event_t::PORT_EVENT_LINK_DOWN) {
        pds_event.event_id = PDS_EVENT_ID_PORT_DOWN;
    } else if (port_event_info->event == port_event_t::PORT_EVENT_LINK_UP) {
        pds_event.event_id = PDS_EVENT_ID_PORT_UP;
    } else {
        SDK_ASSERT(FALSE);
    }
    ret = port_get(&event.port.ifindex, &pds_event.port_info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to get port %s, err %u",
                      eth_ifindex_to_str(event.port.ifindex).c_str(), ret);
        return;
    }
    // notify the agent
    g_pds_state.event_notify(&pds_event);

    port_event_lif_handle(port_event_info);

    // Raise an event
    operd::alerts::operd_alerts_t alert = operd::alerts::LINK_DOWN;
    if (port_event_info->event == port_event_t::PORT_EVENT_LINK_UP) {
        alert = operd::alerts::LINK_UP;
    }
    operd::alerts::alert_recorder::get()->alert(
        alert, "Port %s, key %s",
        eth_ifindex_to_str(event.port.ifindex).c_str(),
        pds_event.port_info.spec.key.str());
}

bool
xvcr_event_walk_cb (void *entry, void *ctxt)
{
    int phy_port;
    uint32_t logical_port;
    if_index_t ifindex;
    if_entry *intf = (if_entry *)entry;
    xcvr_event_info_t *xcvr_event_info = (xcvr_event_info_t *)ctxt;
    sdk::types::sdk_event_t sdk_event;

    // if the interface is already added to db, but port_info is not yet set
    if (intf->port_info() == NULL) {
        return false;
    }
    ifindex = intf->ifindex();
    logical_port = sdk::lib::catalog::ifindex_to_logical_port(ifindex);
    phy_port = sdk::lib::catalog::logical_port_to_phy_port(logical_port);
    if ((phy_port == -1) ||
        (phy_port != (int)xcvr_event_info->phy_port)) {
        return false;
    }
    sdk::linkmgr::port_update_xcvr_event(intf->port_info(), xcvr_event_info);

    xcvr_event_info->ifindex = ifindex;
    sdk_event.event_id = sdk_ipc_event_id_t::SDK_IPC_EVENT_ID_XCVR_STATUS;
    memcpy(&sdk_event.xcvr_event_info, xcvr_event_info,
           sizeof(xcvr_event_info_t));
    sdk::ipc::broadcast(sdk_event.event_id, &sdk_event,
                        sizeof(sdk::types::sdk_event_t));
    return false;
}

/**
 * @brief        Handle transceiver insert/remove events
 * @param[in]    xcvr_event_info    transceiver info filled by linkmgr
 */
void
xcvr_event_cb (xcvr_event_info_t *xcvr_event_info)
{
    /**< ignore xcvr events if xcvr valid check is disabled */
    if (!sdk::platform::xcvr_valid_check_enabled()) {
        return;
    }

    /**
     * if xcvr is removed, bring link down
     * if xcvr sprom read is successful, bring linkup if user admin enabled.
     * ignore all other xcvr states.
     */
    if (xcvr_event_info->state != xcvr_state_t::XCVR_REMOVED &&
        xcvr_event_info->state != xcvr_state_t::XCVR_SPROM_READ) {
        return;
    }
    if_db()->walk(IF_TYPE_ETH, xvcr_event_walk_cb, xcvr_event_info);
}

sdk_ret_t
port_quiesce_all (linkmgr_async_response_cb_t response_cb, void *response_ctxt)
{
    sdk_ret_t ret;

    ret = sdk::linkmgr::port_quiesce_all(response_cb, response_ctxt);
    if (ret != SDK_RET_OK) {
        return SDK_RET_ERR;
    }
    return SDK_RET_IN_PROGRESS;
}

static bool
if_walk_port_stats_reset_cb (void *entry, void *ctxt)
{
    sdk_ret_t ret;
    if_entry *intf = (if_entry *)entry;

    ret = sdk::linkmgr::port_stats_reset(intf->port_info());
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to reset port stats for %s, err %u",
                      eth_ifindex_to_str(intf->ifindex()).c_str(),
                      ret);
    }
    return false;
}

sdk_ret_t
port_stats_reset (const pds_obj_key_t *key)
{
    if_entry *intf;

    if ((key == NULL) || (*key == k_pds_obj_key_invalid)) {
        if_db()->walk(IF_TYPE_ETH, if_walk_port_stats_reset_cb, NULL);
    } else {
        intf = if_db()->find(key);
        if (intf == NULL)  {
            PDS_TRACE_ERR("Port %s not found",
                          eth_ifindex_to_str(intf->ifindex()).c_str());
            return SDK_RET_INVALID_OP;
        }
        if_walk_port_stats_reset_cb(intf, NULL);
    }
    return SDK_RET_OK;
}

sdk_ret_t
port_update (pds_if_spec_t *spec,
             pds_batch_ctxt_t bctxt)
{
    sdk_ret_t ret;
    pds_if_info_t info;

    if (pds_if_read(&spec->key, &info) != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to update port %s, port not found",
                      spec->key.str());
        return sdk::SDK_RET_ENTRY_NOT_FOUND;
    }
    if ((ret = pds_if_update(spec, bctxt)) != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to port %s, err %u",
                      spec->key.str(), ret);
        return ret;
    }
    return SDK_RET_OK;
}

sdk_ret_t
port_get (pds_obj_key_t *key, pds_if_info_t *info)
{
    return pds_if_read(key, info);
}

sdk_ret_t
port_get_all (if_read_cb_t cb, void *ctxt)
{
    return pds_if_read_all(cb, ctxt);
}

}    // namespace api
