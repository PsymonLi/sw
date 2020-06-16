//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// datapath implementation of interface
///
//----------------------------------------------------------------------------

#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/apollo/core/mem.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/framework/api_engine.hpp"
#include "nic/apollo/framework/api_params.hpp"
#include "nic/apollo/api/if.hpp"
#include "nic/apollo/api/impl/apulu/if_impl.hpp"
#include "nic/apollo/api/impl/apulu/pds_impl_state.hpp"
#include "nic/apollo/api/include/pds_policer.hpp"
#include "nic/apollo/api/pds_state.hpp"
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

impl_base *
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
    // TODO: cleanup capri dependency
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
if_impl::activate_host_if_(if_entry *intf, pds_if_spec_t *spec) {
    sdk_ret_t ret;
    sdk::qos::policer_t policer;
    pds_policer_info_t policer_info;
    lif_impl *lif = (lif_impl *)lif_impl_db()->find(&spec->key);

    if (!lif) {
        PDS_TRACE_ERR("activate host if %s failed, lif %s not found",
                      intf->key2str().c_str(), spec->key.str());
        return SDK_RET_ENTRY_NOT_FOUND;
    }

    ret = pds_policer_read(&spec->host_if_info.tx_policer, &policer_info);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("activate host if %s failed, policer %s not found",
                      intf->key2str().c_str(),
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
    return lif->program_tx_policer(&policer);
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
    p4pd_error_t p4pd_ret;
    lif_actiondata_t lif_data = { 0 };

    PDS_TRACE_DEBUG("Activating if %s, type %u, admin state %u",
                    spec->key.str(), spec->type, spec->admin_state);
    if (spec->type == IF_TYPE_UPLINK) {
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
        if (g_pds_state.device_oper_mode() == PDS_DEV_OPER_MODE_HOST) {
            // in "host" enabled mode, uplinks are uplinks
            lif_data.lif_action.direction = P4_LIF_DIR_UPLINK;
            lif_data.lif_action.lif_type = P4_LIF_TYPE_UPLINK;
        } else {
            // in "bitw" mode, we program uplinks as host interfaces
            lif_data.lif_action.direction = P4_LIF_DIR_HOST;
            lif_data.lif_action.lif_type = P4_LIF_TYPE_HOST;
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
        return activate_host_if_(intf, spec);

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
        ret = activate_update_(epoch, (if_entry *)api_obj, obj_ctxt);
        break;

    default:
        ret = SDK_RET_INVALID_OP;
        break;
    }
    return ret;
}

sdk_ret_t
if_impl::read_hw(api_base *api_obj, obj_key_t *key, obj_info_t *info) {
    if_entry *intf;
    uint32_t port_num;
    pds_if_spec_t *spec;
    p4pd_error_t p4pd_ret;
    pds_if_info_t *if_info = (pds_if_info_t *)info;
    p4i_device_info_actiondata_t p4i_device_info_data;

    intf = if_db()->find((pds_obj_key_t *)key);
    spec = &if_info->spec;
    if_info->status.ifindex = intf->ifindex();
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
    } else if (spec->type == IF_TYPE_HOST) {
        uint8_t num_lifs = 0;
        lif_impl *lif = (lif_impl *)lif_impl_db()->find(&spec->key);
        if_info->status.host_if_status.lifs[num_lifs++] = lif->key();
        if_info->status.host_if_status.num_lifs = num_lifs;
        strncpy(if_info->status.host_if_status.name,
                intf->name().c_str(), SDK_MAX_NAME_LEN);
        MAC_ADDR_COPY(if_info->status.host_if_status.mac_addr, intf->host_if_mac());
    }
    return SDK_RET_OK;
}

/// \@}    // end of IF_IMPL_IMPL

}    // namespace impl
}    // namespace api
