//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of interface
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/globals.hpp"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/infra/core/mem.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/infra/core/event.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/if.hpp"
#include "nic/apollo/api/impl/apulu/if_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/include/pds_policer.hpp"
#include "nic/apollo/api/internal/lif.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/port.hpp"
#include "nic/p4/common/defines.h"
#include "gen/platform/mem_regions.hpp"
#include "gen/p4gen/apulu/include/p4pd.h"

namespace api {
namespace impl {

/// \defgroup IF_IMPL_IMPL - interface datapath implementation
/// \ingroup IF_IMPL
/// \@{

if_impl *
if_impl::factory(pds_if_spec_t *spec) {
    if_impl *impl;

    impl = if_impl_db()->alloc();
    if (unlikely(impl == NULL)) {
        return NULL;
    }
    new (impl) if_impl();
    return impl;
}

void
if_impl::destroy(if_impl *impl) {
    impl->~if_impl();
    if_impl_db()->free(impl);
}

if_impl_base *
if_impl::clone(void) {
    if_impl *cloned_impl;

    cloned_impl = if_impl_db()->alloc();
    new (cloned_impl) if_impl();
    // deep copy is not needed as we don't store pointers
    *cloned_impl = *this;
    return cloned_impl;
}

sdk_ret_t
if_impl::free(if_impl *impl) {
    destroy(impl);
    return SDK_RET_OK;
}

sdk_ret_t
if_impl::reserve_resources(api_base *api_obj, api_base *orig_obj,
                           api_obj_ctxt_t *obj_ctxt) {
    uint32_t idx;
    sdk_ret_t ret;
    if_entry *intf = (if_entry *)api_obj;
    pds_if_spec_t *spec = &obj_ctxt->api_params->if_spec;

    if (spec->type == IF_TYPE_HOST) {
        // if a policer is attached, verify that policer is valid
        if ((spec->host_if_info.tx_policer != k_pds_obj_key_invalid) &&
            (policer_db()->find(&spec->host_if_info.tx_policer) == NULL)) {
            return SDK_RET_INVALID_ARG;
        }
    }

    if (spec->type != IF_TYPE_UPLINK) {
        // nothing to reserve
        return SDK_RET_OK;
    }

    switch (obj_ctxt->api_op) {
    case API_OP_CREATE:
        // record the fact that resource reservation was attempted
        // NOTE: even if we partially acquire resources and fail eventually,
        //       this will ensure that proper release of resources will happen
        api_obj->set_rsvd_rsc();
        ret = if_impl_db()->lif_idxr()->alloc(&idx);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to alloc lif hw id for uplink if %s, "
                          "ifindex 0x%x, err %u", spec->key.str(),
                          intf->ifindex(), ret);
            return ret;
        }
        hw_id_ = idx;
        break;

    case API_OP_UPDATE:
        // we will use the same h/w resources as the original object
    default:
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_impl::release_resources(api_base *api_obj) {
    if_entry *intf = (if_entry *)api_obj;

    if (intf->type() != IF_TYPE_UPLINK) {
        return SDK_RET_OK;
    }
    if (hw_id_ != 0xFFFF) {
        if_impl_db()->lif_idxr()->free(hw_id_);
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_impl::nuke_resources(api_base *api_obj) {
    return this->release_resources(api_obj);
}

sdk_ret_t
if_impl::populate_msg(pds_msg_t *msg, api_base *api_obj,
                      api_obj_ctxt_t *obj_ctxt) {
    if_index_t if_index;
    if_entry *intf = (if_entry *)api_obj;

    if (intf->type() == IF_TYPE_HOST) {
        msg->cfg_msg.intf.status.host_if_status.num_lifs = 1;
        if_index = LIF_IFINDEX(HOST_IFINDEX_TO_IF_ID(
                       objid_from_uuid(msg->cfg_msg.intf.spec.key)));
        msg->cfg_msg.intf.status.host_if_status.lifs[0] =
            uuid_from_objid(if_index);
    };
    return SDK_RET_OK;
}

uint32_t
if_impl::port(if_entry *intf) {
    const if_entry *eth_if = if_entry::eth_if(intf);

    if (eth_if == NULL) {
        return PDS_PORT_INVALID;
    }
    return g_pds_state.catalogue()->ifindex_to_tm_port(eth_if->ifindex());
}

#define lif_action         action_u.lif_lif_info
#define p4i_device_info    action_u.p4i_device_info_p4i_device_info
sdk_ret_t
if_impl::program_l3_if_(if_entry *intf, pds_if_spec_t *spec) {
    p4pd_error_t p4pd_ret;
    p4i_device_info_actiondata_t p4i_device_info_data;
    uint32_t port_num;

    // if MAC is explicitly set in the spec, use it or else continue using the
    // MAC from the corresponding lif
    if (!is_mac_set(spec->l3_if_info.mac_addr)) {
        return SDK_RET_OK;
    }

    // perform a read-modify-write of the device entry to update the MAC with
    // the given MAC
    p4pd_ret = p4pd_global_entry_read(P4TBL_ID_P4I_DEVICE_INFO, 0,
                                      NULL, NULL, &p4i_device_info_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to read P4I_DEVICE_INFO table");
        return sdk::SDK_RET_HW_READ_ERR;
    }
    port_num = if_impl::port(intf);
    if (port_num == TM_PORT_UPLINK_0) {
        sdk::lib::memrev(p4i_device_info_data.p4i_device_info.device_mac_addr1,
                         spec->l3_if_info.mac_addr, ETH_ADDR_LEN);
    } else if (port_num == TM_PORT_UPLINK_1) {
        sdk::lib::memrev(p4i_device_info_data.p4i_device_info.device_mac_addr2,
                         spec->l3_if_info.mac_addr, ETH_ADDR_LEN);
    }
    // program the P4I_DEVICE_INFO table
    p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4I_DEVICE_INFO, 0,
                                       NULL, NULL, &p4i_device_info_data);
    if (p4pd_ret != P4PD_SUCCESS) {
        PDS_TRACE_ERR("Failed to program P4I_DEVICE_INFO table");
        return sdk::SDK_RET_HW_PROGRAM_ERR;
    }
    return SDK_RET_OK;
}

static inline std::string
get_set_interface_address_cmd (std::string ns, std::string if_name,
                               ip_prefix_t ip_prefix)
{
    char cmd[PATH_MAX];
    ipv4_addr_t mask = ipv4_prefix_len_to_mask(ip_prefix.len);

    if (ns.empty()) {
        snprintf(cmd, PATH_MAX, "ifconfig %s %s netmask %s up",
                 if_name.c_str(), ipaddr2str(&ip_prefix.addr),
                 ipv4addr2str(mask));
    } else {
        snprintf(cmd, PATH_MAX, "ip netns exec %s ifconfig %s %s netmask %s up",
                 ns.c_str(), if_name.c_str(),
                 ipaddr2str(&ip_prefix.addr), ipv4addr2str(mask));
    }
    return std::string(cmd);
}

static inline std::string
get_set_interface_hw_address_cmd (std::string ns, std::string if_name,
                                  mac_addr_t mac_addr)
{
    char cmd[PATH_MAX];

    if (ns.empty()) {
        snprintf(cmd, PATH_MAX, "ip link set %s address %s",
                 if_name.c_str(), macaddr2str(mac_addr));
    } else {
        snprintf(cmd, PATH_MAX, "ip netns exec %s ip link set %s address %s",
                 ns.c_str(), if_name.c_str(), macaddr2str(mac_addr));
    }
    return std::string(cmd);
}

static inline std::string
get_default_route_add_cmd (std::string ns, ip_addr_t gateway)
{
    char cmd[PATH_MAX];

    if (ns.empty()) {
        snprintf(cmd, PATH_MAX, "ip route add 0.0.0.0/0 via %s",
                 ipaddr2str(&gateway));
    } else {
        snprintf(cmd, PATH_MAX, "ip netns exec %s ip route add 0.0.0.0/0 via %s",
                 ns.c_str(), ipaddr2str(&gateway));
    }
    return std::string(cmd);
}

sdk_ret_t
if_impl::activate_host_if_(if_entry *intf, if_entry *orig_intf,
                           pds_if_spec_t *spec, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    if_index_t if_index;
    ::core::event_t event;
    pds_obj_key_t lif_key;
    sdk::qos::policer_t policer;
    pds_policer_info_t policer_info;

    if_index = LIF_IFINDEX(HOST_IFINDEX_TO_IF_ID(objid_from_uuid(spec->key)));
    lif_key = uuid_from_objid(if_index);
    lif_impl *lif = (lif_impl *)lif_impl_db()->find(&lif_key);
    if (!lif) {
        PDS_TRACE_ERR("activate host if %s failed, lif %s not found",
                      intf->key2str().c_str(), spec->key.str());
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    // handle Tx policer update
    if (obj_ctxt->upd_bmap & PDS_IF_UPD_TX_POLICER) {
        if (spec->host_if_info.tx_policer != k_pds_obj_key_invalid) {
            ret = pds_policer_read(&spec->host_if_info.tx_policer,
                                   &policer_info);
            if (ret != SDK_RET_OK) {
                PDS_TRACE_ERR("Activate host if %s failed, policer %s not "
                              "found", intf->key2str().c_str(),
                              spec->host_if_info.tx_policer.str());
                return SDK_RET_ENTRY_NOT_FOUND;
            }
            policer.type = policer_info.spec.type;
            if (policer.type == sdk::qos::POLICER_TYPE_PPS) {
                policer.rate = policer_info.spec.pps;
                policer.burst = policer_info.spec.pps_burst;
            } else {
                policer.rate = policer_info.spec.bps;
                policer.burst = policer_info.spec.bps_burst;
            }
            ret = lif->program_tx_policer(&policer);
            SDK_ASSERT(ret == SDK_RET_OK);
        } else if (orig_intf->host_if_tx_policer() != k_pds_obj_key_invalid) {
            // clean up policer programming
            ret = lif->program_tx_policer(NULL);
            SDK_ASSERT(ret == SDK_RET_OK);
        }
    }

    // handle admin state update from provider
    if (obj_ctxt->upd_bmap & PDS_IF_UPD_ADMIN_STATE) {
        // provider wants to keep this device down
        if (spec->admin_state == PDS_IF_STATE_DOWN) {
            // send the host device down event, if its not down already
            if (lif->state() != sdk::types::LIF_STATE_DOWN) {
                // notify nicmgr to inform host driver to bring this device down
                memset(&event, 0, sizeof(event));
                event.event_id = (event_id_t)SDK_IPC_EVENT_ID_HOST_DEV_DOWN;
                event.host_dev.id = lif->id();
                sdk::ipc::broadcast(SDK_IPC_EVENT_ID_HOST_DEV_DOWN, &event,
                                    sizeof(event));
            }
        } else if (lif->state() != sdk::types::LIF_STATE_UP) {
                // notify nicmgr to inform host driver to bring this device up
                memset(&event, 0, sizeof(event));
                event.event_id = (event_id_t)SDK_IPC_EVENT_ID_HOST_DEV_UP;
                event.host_dev.id = lif->id();
                sdk::ipc::broadcast(SDK_IPC_EVENT_ID_HOST_DEV_UP, &event,
                                    sizeof(event));
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_impl::activate_control_if_(if_entry *intf, pds_if_spec_t *spec) {
    int rc;
    auto ifcmd = get_set_interface_address_cmd(
                    std::string(PDS_IMPL_CONTROL_NAMESPACE),
                    std::string(PDS_IMPL_CONTROL_IF_NAME),
                    spec->control_if_info.ip_prefix);
    auto ifhwcmd = get_set_interface_hw_address_cmd(
                    std::string(PDS_IMPL_CONTROL_NAMESPACE),
                    std::string(PDS_IMPL_CONTROL_IF_NAME),
                    spec->control_if_info.mac_addr);

    PDS_TRACE_DEBUG("%s", ifcmd.c_str());
    rc = system(ifcmd.c_str());
    if (rc == -1) {
        PDS_TRACE_ERR("set control if address failed with ret %d", rc);
        return SDK_RET_ERR;
    }

    rc = system(ifhwcmd.c_str());
    if (rc == -1) {
        PDS_TRACE_ERR("set control if mac address failed with ret %d", rc);
        return SDK_RET_ERR;
    }

    if (!ip_addr_is_zero(&spec->control_if_info.gateway)) {
        auto routecmd = get_default_route_add_cmd(
                std::string(PDS_IMPL_CONTROL_NAMESPACE),
                spec->control_if_info.gateway);
        PDS_TRACE_DEBUG("%s", routecmd.c_str());
        rc = system(routecmd.c_str());
        if (rc == -1) {
            PDS_TRACE_ERR("route add for control network failed with ret %d", rc);
            return SDK_RET_ERR;
        }
    }

    return SDK_RET_OK;
}

sdk_ret_t
if_impl::activate_create_(pds_epoch_t epoch, if_entry *intf,
                          pds_if_spec_t *spec) {
    sdk_ret_t ret;
    uint32_t tm_port;
    if_entry *eth_if;
    p4pd_error_t p4pd_ret;
    port_type_t port_type;
    lif_actiondata_t lif_data = { 0 };

    PDS_TRACE_DEBUG("Activating if %s, type %u, admin state %u",
                    spec->key.str(), spec->type, spec->admin_state);
    if (spec->type == IF_TYPE_UPLINK) {
        eth_if = if_entry::eth_if(intf);
        port_type = sdk::linkmgr::port_type(eth_if->port_info());
        // program the lif id in the TM
        tm_port = if_impl::port(intf);
        PDS_TRACE_DEBUG("Creating uplink if %s, ifidx 0x%x, port %s, "
                        "hw_id_ %u, tm_port %u", spec->key.str(),
                        intf->ifindex(), spec->uplink_info.port.str(),
                        hw_id_, tm_port);
        ret = sdk::asic::pd::asicpd_tm_uplink_lif_set(tm_port, hw_id_);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to program uplink %s's lif %u in TM "
                          "register", spec->key.str(), hw_id_);
        }
        // program LIF table
        lif_data.action_id = LIF_LIF_INFO_ID;
        if ((g_pds_state.device_oper_mode() == PDS_DEV_OPER_MODE_HOST) ||
            (port_type == port_type_t::PORT_TYPE_MGMT)) {
            // in "host" enabled mode, uplinks are uplinks
            lif_data.lif_action.direction = P4_LIF_DIR_UPLINK;
            lif_data.lif_action.lif_type = P4_LIF_TYPE_UPLINK;
        } else if (g_pds_state.device_oper_mode() ==
                       PDS_DEV_OPER_MODE_BITW_SMART_SERVICE) {
            // in "bitw" mode, we program uplinks as host interfaces
            lif_data.lif_action.direction = P4_LIF_DIR_HOST;
            lif_data.lif_action.lif_type = P4_LIF_TYPE_HOST;
        } else {
            SDK_ASSERT(FALSE);
        }
        lif_data.lif_action.vnic_id = PDS_IMPL_RSVD_VNIC_HW_ID;
        lif_data.lif_action.bd_id = PDS_IMPL_RSVD_BD_HW_ID;
        lif_data.lif_action.vpc_id = PDS_IMPL_RSVD_VPC_HW_ID;
        p4pd_ret = p4pd_global_entry_write(P4TBL_ID_LIF, hw_id_,
                                           NULL, NULL, &lif_data);
        if (p4pd_ret != P4PD_SUCCESS) {
            PDS_TRACE_ERR("Failed to program LIF table for uplink lif %u",
                          hw_id_);
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
        p4pd_ret = p4pd_global_entry_write(P4TBL_ID_LIF2, hw_id_,
                                           NULL, NULL, &lif_data);
        if (p4pd_ret != P4PD_SUCCESS) {
            PDS_TRACE_ERR("Failed to program LIF2 table for uplink lif %u",
                          hw_id_);
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
    } else if (spec->type == IF_TYPE_L3) {
        ret = program_l3_if_(intf, spec);
    } else if (spec->type == IF_TYPE_CONTROL) {
        ret = activate_control_if_(intf, spec);
    }
    return SDK_RET_OK;
}

static inline std::string
get_unset_interface_address_cmd (std::string ns, std::string if_name)
{
    char cmd[PATH_MAX];

    if (ns.empty()) {
        snprintf(cmd, PATH_MAX, "ifconfig %s 0.0.0.0",
                 if_name.c_str());
    } else {
        snprintf(cmd, PATH_MAX, "ip netns exec %s ifconfig %s 0.0.0.0",
                 ns.c_str(), if_name.c_str());
    }
    return std::string(cmd);
}

static inline std::string
get_default_route_del_cmd (std::string ns)
{
    char cmd[PATH_MAX];

    if (ns.empty()) {
        snprintf(cmd, PATH_MAX, "ip route del 0.0.0.0/0");
    } else {
        snprintf(cmd, PATH_MAX, "ip netns exec %s ip route del 0.0.0.0/0",
                 ns.c_str());
    }
    return std::string(cmd);
}

sdk_ret_t
if_impl::deactivate_control_if_(if_entry *intf) {
    int rc;
    auto ifcmd = get_unset_interface_address_cmd(
            std::string(PDS_IMPL_CONTROL_NAMESPACE),
            std::string(PDS_IMPL_CONTROL_IF_NAME));

    PDS_TRACE_DEBUG("%s", ifcmd.c_str());
    rc = system(ifcmd.c_str());
    if (rc == -1) {
        PDS_TRACE_ERR("unset control if address failed with ret %d", rc);
        return SDK_RET_ERR;
    }

    auto gateway = intf->control_gateway();
    if (!ip_addr_is_zero(&gateway)) {
        auto routecmd = get_default_route_del_cmd(
                std::string(PDS_IMPL_CONTROL_NAMESPACE));
        rc = system(routecmd.c_str());
        if (rc == -1) {
            PDS_TRACE_ERR("route del for control network failed with ret %d", rc);
            return SDK_RET_ERR;
        }
    }

    return SDK_RET_OK;
}

sdk_ret_t
if_impl::activate_delete_(pds_epoch_t epoch, if_entry *intf) {
    p4pd_error_t p4pd_ret;
    p4i_device_info_actiondata_t p4i_device_info_data;
    sdk_ret_t ret = SDK_RET_OK;

    if (intf->type() == IF_TYPE_L3) {
        p4pd_ret = p4pd_global_entry_read(P4TBL_ID_P4I_DEVICE_INFO, 0,
                                          NULL, NULL, &p4i_device_info_data);
        if (p4pd_ret != P4PD_SUCCESS) {
            PDS_TRACE_ERR("Failed to read P4I_DEVICE_INFO table");
            return sdk::SDK_RET_HW_READ_ERR;
        }
        // TODO: cleanup capri dependency
        if (if_impl::port(intf) == TM_PORT_UPLINK_0) {
            memset(p4i_device_info_data.p4i_device_info.device_mac_addr1,
                   0, ETH_ADDR_LEN);
        } else if (if_impl::port(intf) == TM_PORT_UPLINK_1) {
            memset(p4i_device_info_data.p4i_device_info.device_mac_addr2,
                   0, ETH_ADDR_LEN);
        }
        // program the P4I_DEVICE_INFO table
        p4pd_ret = p4pd_global_entry_write(P4TBL_ID_P4I_DEVICE_INFO, 0,
                                           NULL, NULL, &p4i_device_info_data);
        if (p4pd_ret != P4PD_SUCCESS) {
            PDS_TRACE_ERR("Failed to program P4I_DEVICE_INFO table");
            return sdk::SDK_RET_HW_PROGRAM_ERR;
        }
    } else if (intf->type() == IF_TYPE_CONTROL) {
        ret = deactivate_control_if_(intf);
    } else {
        PDS_TRACE_ERR("Delete unsupported for interface type %u",
                      intf->type());
        return sdk::SDK_RET_INVALID_OP;
    }
    return ret;
}

sdk_ret_t
if_impl::activate_update_(pds_epoch_t epoch, if_entry *intf,
                          if_entry *orig_intf,
                          api_obj_ctxt_t *obj_ctxt) {
    pds_if_spec_t *spec = &obj_ctxt->api_params->if_spec;

    switch(spec->type) {
    case IF_TYPE_UPLINK:
        if (obj_ctxt->upd_bmap & PDS_IF_UPD_ADMIN_STATE) {
            // TODO: @akoradha, we need to bring port down here !!
            return SDK_RET_INVALID_OP;
        }
        return SDK_RET_OK;

    case IF_TYPE_CONTROL:
        return activate_control_if_(intf, spec);

    case IF_TYPE_L3:
        return program_l3_if_(intf, spec);

    case IF_TYPE_ETH:
        return SDK_RET_OK;

    case IF_TYPE_HOST:
        return activate_host_if_(intf, orig_intf, spec, obj_ctxt);

    default:
        break;
    }
    return SDK_RET_INVALID_ARG;
}

sdk_ret_t
if_impl::activate_hw(api_base *api_obj, api_base *orig_obj, pds_epoch_t epoch,
                     api_op_t api_op, api_obj_ctxt_t *obj_ctxt) {
    sdk_ret_t ret;
    pds_if_spec_t *spec;

    switch (api_op) {
    case API_OP_CREATE:
        spec = &obj_ctxt->api_params->if_spec;
        ret = activate_create_(epoch, (if_entry *)api_obj, spec);
        break;

    case API_OP_DELETE:
        // spec is not available for DELETE operations
        ret = activate_delete_(epoch, (if_entry *)api_obj);
        break;

    case API_OP_UPDATE:
        ret = activate_update_(epoch, (if_entry *)api_obj,
                               (if_entry *)orig_obj, obj_ctxt);
        break;

    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

sdk_ret_t
if_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    uint32_t port_num;
    pds_if_spec_t *spec;
    p4pd_error_t p4pd_ret;
    if_entry *intf = (if_entry *)api_obj;
    pds_if_info_t *if_info = (pds_if_info_t *)info;
    p4i_device_info_actiondata_t p4i_device_info_data;

    spec = &if_info->spec;
    if (spec->type == IF_TYPE_L3) {
        p4pd_ret = p4pd_global_entry_read(P4TBL_ID_P4I_DEVICE_INFO, 0,
                                          NULL, NULL, &p4i_device_info_data);
        if (p4pd_ret != P4PD_SUCCESS) {
            PDS_TRACE_ERR("Failed to read P4I_DEVICE_INFO table");
            return sdk::SDK_RET_HW_READ_ERR;
        }
        port_num = if_impl::port(intf);
        if (port_num == TM_PORT_UPLINK_0) {
            sdk::lib::memrev(spec->l3_if_info.mac_addr,
                          p4i_device_info_data.p4i_device_info.device_mac_addr1,
                          ETH_ADDR_LEN);
        } else if (port_num == TM_PORT_UPLINK_1) {
            sdk::lib::memrev(spec->l3_if_info.mac_addr,
                          p4i_device_info_data.p4i_device_info.device_mac_addr2,
                          ETH_ADDR_LEN);
        }
    } else if (spec->type == IF_TYPE_UPLINK) {
        if_info->status.uplink_status.lif_id = hw_id_;
    }
    return SDK_RET_OK;
}

sdk_ret_t
if_impl::track_pps(api_base *api_obj, uint32_t interval) {
    sdk_ret_t ret;
    port_args_t port_args;
    lif_metrics_t lif_metrics = { 0 };
    uint64_t curr_tx_pkts, curr_tx_bytes;
    uint64_t curr_rx_pkts, curr_rx_bytes;
    if_entry *intf = (if_entry *)api_obj, *eth_if;
    uint64_t tx_pkts, tx_bytes, rx_pkts, rx_bytes;
    mem_addr_t base_addr, if_addr, write_mem_addr;
    uint64_t stats[MAX_MAC_STATS];

    base_addr = g_pds_state.mempartition()->start_addr(MEM_REGION_LIF_STATS_NAME);
    if_addr = base_addr + (hw_id_ << LIF_STATS_SIZE_SHIFT);

    ret = sdk::asic::asic_mem_read(if_addr, (uint8_t *)&lif_metrics,
                                   sizeof(lif_metrics_t));
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Error reading stats for if %s hw id %u, err %u",
                      intf->key().str(), hw_id_, ret);
        return ret;
    }

    tx_pkts  = lif_metrics.tx_pkts;
    tx_bytes = lif_metrics.tx_bytes;
    rx_pkts  = lif_metrics.rx_pkts;
    rx_bytes = lif_metrics.rx_bytes;

    port_args.stats_data = stats;
    // get the eth interface corresponding to this interface entry
    eth_if = if_entry::eth_if(intf);
    if (!eth_if) {
        PDS_TRACE_ERR("Failed to get corresponding eth interface for uplink %s",
                      intf->key().str());
        return SDK_RET_ERR;
    }
    ret = eth_if->port_get(&port_args);
    if(ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to get port info for uplink %s, err %u",
                      intf->key().str(), ret);
        return ret;
    }
    if (port_args.port_type == port_type_t::PORT_TYPE_MGMT) {
        curr_tx_pkts = port_args.stats_data[PDS_MGMT_MAC_FRAMES_TX_ALL];
        curr_tx_bytes = port_args.stats_data[PDS_MGMT_MAC_OCTETS_TX_TOTAL];
        curr_rx_pkts = port_args.stats_data[PDS_MGMT_MAC_FRAMES_RX_ALL];
        curr_rx_bytes = port_args.stats_data[PDS_MGMT_MAC_OCTETS_RX_ALL];
    } else {
        curr_tx_pkts = port_args.stats_data[PDS_FRAMES_TX_ALL];
        curr_tx_bytes = port_args.stats_data[PDS_OCTETS_TX_TOTAL];
        curr_rx_pkts = port_args.stats_data[PDS_FRAMES_RX_ALL];
        curr_rx_bytes = port_args.stats_data[PDS_OCTETS_RX_ALL];
    }
    lif_metrics.tx_pps = RATE_OF_X(tx_pkts, curr_tx_pkts, interval);
    lif_metrics.tx_bps = RATE_OF_X(tx_bytes, curr_tx_bytes, interval);
    lif_metrics.rx_pps = RATE_OF_X(rx_pkts, curr_rx_pkts, interval);
    lif_metrics.rx_bps = RATE_OF_X(rx_bytes, curr_rx_bytes, interval);
    lif_metrics.tx_pkts = curr_tx_pkts;
    lif_metrics.tx_bytes = curr_tx_bytes;
    lif_metrics.rx_pkts = curr_rx_pkts;
    lif_metrics.rx_bytes = curr_rx_bytes;

    write_mem_addr = if_addr + LIF_STATS_TX_PKTS_OFFSET;
    ret = sdk::asic::asic_mem_write(write_mem_addr,
                                    (uint8_t *)&lif_metrics.tx_pkts,
                                    PDS_LIF_NUM_PPS_STATS *
                                        PDS_LIF_COUNTER_SIZE); // 8 fields
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Writing stats for lif %s hw id %u failed, err %u",
                      intf->key().str(), hw_id_, ret);
    }
    return ret;
}

void
if_impl::dump_stats(api_base *api_obj, uint32_t fd) {
    sdk_ret_t ret;
    uint16_t hw_id;
    pds_if_info_t if_info;
    mem_addr_t base_addr, if_addr;
    lif_metrics_t lif_metrics = { 0 };
    if_entry *intf = (if_entry *)api_obj;

    base_addr = g_pds_state.mempartition()->start_addr(MEM_REGION_LIF_STATS_NAME);
    if_addr = base_addr + (hw_id_ << LIF_STATS_SIZE_SHIFT);
    ret = sdk::asic::asic_mem_read(if_addr, (uint8_t *)&lif_metrics,
                                   sizeof(lif_metrics_t));
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Error reading stats for if %s hw id %u, err %u",
                      intf->key().str(), hw_id_, ret);
        return;
    }

    dprintf(fd, "\nInterface ID: %s\n", intf->key().str());
    dprintf(fd, "%s\n", std::string(50, '-').c_str());
    dprintf(fd, "tx_pkts                         : %lu\n", lif_metrics.tx_pkts);
    dprintf(fd, "tx_bytes                        : %lu\n", lif_metrics.tx_bytes);
    dprintf(fd, "rx_pkts                         : %lu\n", lif_metrics.rx_pkts);
    dprintf(fd, "rx_bytes                        : %lu\n", lif_metrics.rx_bytes);
    dprintf(fd, "tx_pps                          : %lu\n", lif_metrics.tx_pps);
    dprintf(fd, "tx_bps                          : %lu\n", lif_metrics.tx_bps);
    dprintf(fd, "rx_pps                          : %lu\n", lif_metrics.rx_pps);
    dprintf(fd, "rx_bps                          : %lu\n", lif_metrics.rx_bps);

}

/// \@}    // end of IF_IMPL_IMPL

}    // namespace impl
}    // namespace api
