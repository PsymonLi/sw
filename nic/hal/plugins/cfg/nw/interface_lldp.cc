//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include <google/protobuf/util/json_util.h>
#include "boost/foreach.hpp"
#include "boost/optional.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "nic/include/base.hpp"
#include "nic/hal/iris/include/hal_state.hpp"
#include "nic/hal/plugins/cfg/nw/interface_lldp.hpp"
#include "nic/sdk/platform/fru/fru.hpp"
#include "nic/sdk/lib/lldp/lldp.hpp"
#include "gen/proto/interface.pb.h"
#include "nic/sdk/lib/runenv/runenv.h"
//#include "nic/sdk/platform/ncsi/ncsi_mgr.h"

#define TNNL_ENC_TYPE intf::IfTunnelEncapType

using intf::LldpIfStatus;
using namespace sdk::lib;

using std::string;
namespace pt = boost::property_tree;

namespace hal {

static void
system_mac_init (void)
{
    std::string       mac_str, pname;
    mac_addr_t        mac_addr;

    int ret = sdk::platform::readfrukey(BOARD_MACADDRESS_KEY, mac_str);
    if (ret < 0) {
        // invalid fru. set system MAC to default
        MAC_UINT64_TO_ADDR(mac_addr, PENSANDO_NIC_MAC);
    } else {
        // valid fru
        mac_str_to_addr((char *)mac_str.c_str(), mac_addr);
    }
    if (is_platform_type_hw()) {
        if (ret < 0) {
            SDK_ASSERT(0);
        }
    }
    g_hal_state->set_system_mac(mac_addr);
    HAL_TRACE_DEBUG("system mac {}",
                    macaddr2str(g_hal_state->system_mac()));

    sdk::platform::readfrukey(BOARD_PRODUCTNAME_KEY, pname);
    if (pname.empty() || pname == "") {
        pname = std::string("-");
    }
    g_hal_state->set_product_name(pname);
}

hal_ret_t
hal_lldp_config_init (void)
{
    char cmd[PATH_MAX];
    int  rc;

    system_mac_init();

    // set system name to be the FRU mac by default
    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli configure system hostname %s",
             macaddr2str(g_hal_state->system_mac()));
    rc = system(cmd);
    if (rc == -1) {
        HAL_TRACE_ERR("lldp configure system hostname {} failed with error {}",
                      macaddr2str(g_hal_state->system_mac()), rc);
    }
    HAL_TRACE_DEBUG("Configured LLDP system name {}", cmd);

    // set chassis-id to be the FRU mac by default
    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli configure system chassisid %s",
             macaddr2str(g_hal_state->system_mac()));
    rc = system(cmd);
    if (rc == -1) {
        HAL_TRACE_ERR("lldp configure system chassisid {} failed with error {}",
                      macaddr2str(g_hal_state->system_mac()), rc);
    }
    HAL_TRACE_DEBUG("Configured LLDP system chassisid {}", cmd);

    // set system description to be the product name by default
    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli configure system description \"%s %s\"",
             DSC_DESCRIPTION, g_hal_state->product_name().c_str());
    rc = system(cmd);
    if (rc == -1) {
        HAL_TRACE_ERR("lldp configure system description {} failed with error {}",
                      macaddr2str(g_hal_state->system_mac(), true), rc);
    }
    HAL_TRACE_DEBUG("Configured LLDP system description {}", cmd);

    // Enable LLDP on OOB - For NCSI Cards, dont enable
    if (runenv::is_feature_enabled(NCSI_FEATURE) != FEATURE_ENABLED) {
        snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli configure system interface pattern inb*,oob*");
        rc = system(cmd);
        if (rc == -1) {
            HAL_TRACE_ERR("lldp configure interface pattern failed with error {}", rc);
        }
        HAL_TRACE_DEBUG("Configured LLDP interface pattern {}", cmd);
    }
    return HAL_RET_OK;
}

static inline std::string
get_lldpcli_show_cmd (std::string intf, bool nbrs, std::string status_file)
{
    char cmd[PATH_MAX];

    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli show %s ports %s -f json0 > %s; sync;",
             nbrs ? "neighbors" : "interface", intf.c_str(), status_file.c_str());
    return std::string(cmd);
}

#if 0
static sdk_ret_t
interface_lldp_parse_json (bool nbrs, LldpIfStatus *lldp_status,
                           std::string lldp_status_file)
{
    pt::ptree json_pt;
    string    str;
    int       i;

    // parse the lldp status output json file
    if (access(lldp_status_file.c_str(), R_OK) < 0) {
        HAL_TRACE_ERR("{} doesn't exist or not accessible\n", lldp_status_file.c_str());
        return SDK_RET_ERR;
    }

    try {
        std::ifstream json_cfg(lldp_status_file.c_str());
        read_json(json_cfg, json_pt);
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        HAL_TRACE_VERBOSE("Error parsing lldp status json");
        return SDK_RET_ERR;
    }

    try {
        BOOST_FOREACH (pt::ptree::value_type &lldp,
                       json_pt.get_child("lldp")) {
            BOOST_FOREACH (pt::ptree::value_type &iface,
                           lldp.second.get_child("interface")) {
                str = iface.second.get<std::string>("name", "");
                if (!str.empty()) {
                    lldp_status->set_ifname(str);
                } else {
                    HAL_TRACE_ERR("  No interface entry");
                }

                str = iface.second.get<std::string>("via", "");
                if (!str.empty()) {
                    if (!str.compare("LLDP")) {
                        lldp_status->set_proto(intf::LLDP_MODE_LLDP);
                    } else if (!str.compare("CDPv1")) {
                        lldp_status->set_proto(intf::LLDP_MODE_CDPV1);
                    } else if (!str.compare("CDPv2")) {
                        lldp_status->set_proto(intf::LLDP_MODE_CDPV2);
                    } else if (!str.compare("EDP")) {
                        lldp_status->set_proto(intf::LLDP_MODE_EDP);
                    } else if (!str.compare("FDP")) {
                        lldp_status->set_proto(intf::LLDP_MODE_FDP);
                    } else if (!str.compare("SONMP")) {
                        lldp_status->set_proto(intf::LLDP_MODE_SONMP);
                    } else {
                        lldp_status->set_proto(intf::LLDP_MODE_NONE);
                    }
                }

                if (nbrs) {

                    // router id is present only in neighbor tlvs
                    boost::optional<std::string> rid_opt = iface.second.get_optional<std::string>("rid");
                    if (rid_opt) {
                        lldp_status->set_routerid(iface.second.get<uint32_t>("rid"));
                    }
                }

                str = iface.second.get<std::string>("age", "");
                lldp_status->set_age(str);

                auto chassis_status = lldp_status->mutable_lldpifchassisstatus();
                auto chassis_id = chassis_status->mutable_chassisid();

                BOOST_FOREACH (pt::ptree::value_type &chassis,
                               iface.second.get_child("chassis")) {

                    BOOST_FOREACH (pt::ptree::value_type &cid,
                                   chassis.second.get_child("id")) {
        
                        str = cid.second.get<std::string>("type", "");
                        if (!str.empty()) {
                            if (!str.compare("ifname")) {
                                chassis_id->set_type(intf::LLDPID_SUBTYPE_IFNAME);
                            } else if (!str.compare("ifalias")) {
                                chassis_id->set_type(intf::LLDPID_SUBTYPE_IFALIAS);
                            } else if (!str.compare("local")) {
                                chassis_id->set_type(intf::LLDPID_SUBTYPE_LOCAL);
                            } else if (!str.compare("mac")) {
                                chassis_id->set_type(intf::LLDPID_SUBTYPE_MAC);
                            } else if (!str.compare("ip")) {
                                chassis_id->set_type(intf::LLDPID_SUBTYPE_IP);
                            } else if (!str.compare("port")) {
                                chassis_id->set_type(intf::LLDPID_SUBTYPE_PORT);
                            } else if (!str.compare("chassis")) {
                                chassis_id->set_type(intf::LLDPID_SUBTYPE_CHASSIS);
                            } else {
                                chassis_id->set_type(intf::LLDPID_SUBTYPE_NONE);
                            }
                        }
            
                        str = cid.second.get<std::string>("value", "");
                        chassis_id->set_value(str);
                    }
            
                    BOOST_FOREACH (pt::ptree::value_type &cname,
                                   chassis.second.get_child("name")) {
                        str = cname.second.get<std::string>("value", "");
                        chassis_status->set_sysname(str);
                    }

                    BOOST_FOREACH (pt::ptree::value_type &cdescr,
                                   chassis.second.get_child("descr")) {
                        str = cdescr.second.get<std::string>("value", "");
                        chassis_status->set_sysdescr(str);
                    }

                    ip_addr_t mgmt_ip;
                    // mgmt ip tlv may not be present in certain chassis tlvs
                    boost::optional<pt::ptree&> mgmtip_opt = chassis.second.get_child_optional("mgmt-ip");
                    if (mgmtip_opt) {
                        BOOST_FOREACH (pt::ptree::value_type &cmgmtip,
                                       chassis.second.get_child("mgmt-ip")) {
                            str2ipaddr(cmgmtip.second.get<std::string>("value").c_str(), &mgmt_ip);
                            ip_addr_to_spec(chassis_status->mutable_mgmtip(), &mgmt_ip);
                        }
                    }

                    i = 0;
                    BOOST_FOREACH (pt::ptree::value_type &capability,
                                   chassis.second.get_child("capability")) {

                        str = capability.second.get<std::string>("type", "");
                        auto cap_info = chassis_status->add_capability();

                        if (!str.empty()) {
                            if (!str.compare("Repeater")) {
                                cap_info->set_captype(intf::LLDP_CAPTYPE_REPEATER);
                            } else if (!str.compare("Bridge")) {
                                cap_info->set_captype(intf::LLDP_CAPTYPE_BRIDGE);
                            } else if (!str.compare("Router")) {
                                cap_info->set_captype(intf::LLDP_CAPTYPE_ROUTER);
                            } else if (!str.compare("Wlan")) {
                                cap_info->set_captype(intf::LLDP_CAPTYPE_WLAN);
                            } else if (!str.compare("Telephone")) {
                                cap_info->set_captype(intf::LLDP_CAPTYPE_TELEPHONE);
                            } else if (!str.compare("Docsis")) {
                                cap_info->set_captype(intf::LLDP_CAPTYPE_DOCSIS);
                            } else if (!str.compare("Station")) {
                                cap_info->set_captype(intf::LLDP_CAPTYPE_STATION);
                            } else {
                                cap_info->set_captype(intf::LLDP_CAPTYPE_OTHER);
                            }
                        }
                  
                        cap_info->set_capenabled(capability.second.get<bool>("enabled"));
                        i++;
                        if (i >= 8) break;
                    }
                }

                // record the number of capabilities seen.
                // lldp_status->chassis_status.num_caps = i;

                auto port_status = lldp_status->mutable_lldpifportstatus();
                auto port_id = port_status->mutable_portid();

                BOOST_FOREACH (pt::ptree::value_type &port,
                               iface.second.get_child("port")) {        

                    BOOST_FOREACH (pt::ptree::value_type &pid,
                                   port.second.get_child("id")) {
            
                        str = pid.second.get<std::string>("type", "");
                        if (!str.empty()) {
                            if (!str.compare("ifname")) {
                                port_id->set_type(intf::LLDPID_SUBTYPE_IFNAME);
                            } else if (!str.compare("ifalias")) {
                                port_id->set_type(intf::LLDPID_SUBTYPE_IFALIAS);
                            } else if (!str.compare("local")) {
                                port_id->set_type(intf::LLDPID_SUBTYPE_LOCAL);
                            } else if (!str.compare("mac")) {
                                port_id->set_type(intf::LLDPID_SUBTYPE_MAC);
                            } else if (!str.compare("ip")) {
                                port_id->set_type(intf::LLDPID_SUBTYPE_IP);
                            } else if (!str.compare("port")) {
                                port_id->set_type(intf::LLDPID_SUBTYPE_PORT);
                            } else if (!str.compare("chassis")) {
                                port_id->set_type(intf::LLDPID_SUBTYPE_CHASSIS);
                            } else {
                                port_id->set_type(intf::LLDPID_SUBTYPE_NONE);
                            }
                        }
            
                        str = pid.second.get<std::string>("value", "");
                        if (!str.empty()) {
                            port_id->set_value(str);
                        }
                    }

                    BOOST_FOREACH (pt::ptree::value_type &pdescr,
                                   port.second.get_child("descr")) {
                        str = pdescr.second.get<std::string>("value", "");
                        if (!str.empty()) {
                            port_status->set_portdescr(str);
                        }
                    }

                    // ttl tlv is present inside the port block only in neighbor tlvs
                    boost::optional<pt::ptree&> ttl_block = port.second.get_child_optional("ttl");
                    if (ttl_block) {
                        BOOST_FOREACH (pt::ptree::value_type &pttl,
                                       port.second.get_child("ttl")) {
                            port_status->set_ttl(pttl.second.get<uint32_t>("value"));
                        }
                    }
                }

                // lldp_status->unknown_tlv_status.num_tlvs = 0;
                boost::optional<pt::ptree&> unknown_tlv_opt = iface.second.get_child_optional("unknown-tlvs");
                if (unknown_tlv_opt) {
                    BOOST_FOREACH (pt::ptree::value_type &unknown_tlvs,
                                   iface.second.get_child("unknown-tlvs")) {

                        i = 0;
                        BOOST_FOREACH (pt::ptree::value_type &tlv,
                                       unknown_tlvs.second.get_child("unknown-tlv")) {

                            auto tlv_status = lldp_status->add_lldpunknowntlvstatus();
                            str = tlv.second.get<std::string>("oui", "");
                            if (!str.empty()) {
                                tlv_status->set_oui(str);
                            }

                            tlv_status->set_subtype(tlv.second.get<uint32_t>("subtype"));
                            tlv_status->set_len(tlv.second.get<uint32_t>("len"));
                            str = tlv.second.get<std::string>("value", "");
                            if (!str.empty()) {
                                tlv_status->set_value(str);
                            }
                            i++;
                            if (i >= 8) break;
                        }
                    }

                    // record the number of unknown tlvs seen
                    // lldp_status->unknown_tlv_status.num_tlvs = i;
                }

                // in the interface output, the ttl block is present under interface block
                boost::optional<pt::ptree&> ttl_block = iface.second.get_child_optional("ttl");
                if (ttl_block) {
                    BOOST_FOREACH (pt::ptree::value_type &ttl_list,
                                   iface.second.get_child("ttl")) {
                        port_status->set_ttl(ttl_list.second.get<uint32_t>("ttl"));
                    }
                }
            }
        }
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        HAL_TRACE_VERBOSE("Error reading lldp status json");
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}
#endif

static inline intf::LldpProtoMode
lldpmode_to_proto_lldpmode (sdk::lib::if_lldp_mode_t mode)
{
    switch (mode) {
    case LLDP_MODE_LLDP:
        return intf::LLDP_MODE_LLDP;
    case LLDP_MODE_CDPV1:
        return intf::LLDP_MODE_CDPV1;
    case LLDP_MODE_CDPV2:
        return intf::LLDP_MODE_CDPV2;
    case LLDP_MODE_EDP:
        return intf::LLDP_MODE_EDP;
    case LLDP_MODE_FDP:
        return intf::LLDP_MODE_FDP;
    case LLDP_MODE_SONMP:
        return intf::LLDP_MODE_SONMP;
    default:
        return intf::LLDP_MODE_NONE;
    }
}

static inline intf::LldpIdType
lldpidtype_to_proto_lldpidtype (if_lldp_idtype_t idtype)
{
    switch (idtype) {
    case LLDPID_SUBTYPE_IFNAME:
        return intf::LLDPID_SUBTYPE_IFNAME;
    case LLDPID_SUBTYPE_IFALIAS:
        return intf::LLDPID_SUBTYPE_IFALIAS;
    case LLDPID_SUBTYPE_LOCAL:
        return intf::LLDPID_SUBTYPE_LOCAL;
    case LLDPID_SUBTYPE_MAC:
        return intf::LLDPID_SUBTYPE_MAC;
    case LLDPID_SUBTYPE_IP:
        return intf::LLDPID_SUBTYPE_IP;
    case LLDPID_SUBTYPE_PORT:
        return intf::LLDPID_SUBTYPE_PORT;
    case LLDPID_SUBTYPE_CHASSIS:
        return intf::LLDPID_SUBTYPE_CHASSIS;
    default:
        return intf::LLDPID_SUBTYPE_NONE;
    }
}

static inline intf::LldpCapType
lldpcaptype_to_proto_lldpcaptype (if_lldp_captype_t captype)
{
    switch (captype) {
    case LLDP_CAPTYPE_REPEATER:
        return intf::LLDP_CAPTYPE_REPEATER;
    case LLDP_CAPTYPE_BRIDGE:
        return intf::LLDP_CAPTYPE_BRIDGE;
    case LLDP_CAPTYPE_ROUTER:
        return intf::LLDP_CAPTYPE_ROUTER;
    case LLDP_CAPTYPE_WLAN:
        return intf::LLDP_CAPTYPE_WLAN;
    case LLDP_CAPTYPE_TELEPHONE:
        return intf::LLDP_CAPTYPE_TELEPHONE;
    case LLDP_CAPTYPE_DOCSIS:
        return intf::LLDP_CAPTYPE_DOCSIS;
    case LLDP_CAPTYPE_STATION:
        return intf::LLDP_CAPTYPE_STATION;
    default:
        return intf::LLDP_CAPTYPE_OTHER;
    }
}

static inline void
if_lldp_status_to_proto (intf::UplinkResponseInfo *rsp, if_lldp_status_t *api_lldp_status)
{
    auto lldp_rsp = rsp->mutable_lldpstatus();


    // fill in the lldp local interface status
    auto lldpifstatus = lldp_rsp->mutable_lldpifstatus();
    lldpifstatus->set_ifname(api_lldp_status->local_if_status.if_name);
    lldpifstatus->set_routerid(api_lldp_status->local_if_status.router_id);
    lldpifstatus->set_proto(lldpmode_to_proto_lldpmode(api_lldp_status->local_if_status.mode));
    lldpifstatus->set_age((api_lldp_status->local_if_status.age.length() > 0) ? 
                          api_lldp_status->local_if_status.age.c_str() : "");
    auto lldpchstatus = lldpifstatus->mutable_lldpifchassisstatus();
    lldpchstatus->set_sysname(api_lldp_status->local_if_status.chassis_status.sysname);
    lldpchstatus->set_sysdescr(api_lldp_status->local_if_status.chassis_status.sysdescr);
    ipv4_addr_to_spec(lldpchstatus->mutable_mgmtip(),
                      &api_lldp_status->local_if_status.chassis_status.mgmt_ip);
    auto lldpchidstatus = lldpchstatus->mutable_chassisid();
    lldpchidstatus->set_type(
        lldpidtype_to_proto_lldpidtype(api_lldp_status->local_if_status.chassis_status.chassis_id.type));
    lldpchidstatus->set_value((char *)api_lldp_status->local_if_status.chassis_status.chassis_id.value);
    for (int i = 0; i < api_lldp_status->local_if_status.chassis_status.num_caps; i++) {
        auto lldpchcap = lldpchstatus->add_capability();
        lldpchcap->set_captype(
            lldpcaptype_to_proto_lldpcaptype(api_lldp_status->local_if_status.chassis_status.chassis_cap_info[i].cap_type));
        lldpchcap->set_capenabled(api_lldp_status->local_if_status.chassis_status.chassis_cap_info[i].cap_enabled);
    }
    auto lldpportstatus = lldpifstatus->mutable_lldpifportstatus();
    auto lldppidstatus = lldpportstatus->mutable_portid();
    lldppidstatus->set_type(lldpidtype_to_proto_lldpidtype(api_lldp_status->local_if_status.port_status.port_id.type));
    lldppidstatus->set_value((char *)api_lldp_status->local_if_status.port_status.port_id.value);
    lldpportstatus->set_portdescr(api_lldp_status->local_if_status.port_status.port_descr);
    lldpportstatus->set_ttl(api_lldp_status->local_if_status.port_status.ttl);

    for (int i = 0; i < api_lldp_status->local_if_status.unknown_tlv_status.num_tlvs; i++) {
        auto lldp_unknown_tlv = lldpifstatus->add_lldpunknowntlvstatus();
        lldp_unknown_tlv->set_oui((char *)api_lldp_status->local_if_status.unknown_tlv_status.tlvs[i].oui);
        lldp_unknown_tlv->set_subtype(api_lldp_status->local_if_status.unknown_tlv_status.tlvs[i].subtype);
        lldp_unknown_tlv->set_len(api_lldp_status->local_if_status.unknown_tlv_status.tlvs[i].len);
        lldp_unknown_tlv->set_value((char *)api_lldp_status->local_if_status.unknown_tlv_status.tlvs[i].value);
    }

    // fill in the lldp neighbor status
    auto lldpnbrstatus = lldp_rsp->mutable_lldpnbrstatus();
    lldpnbrstatus->set_ifname(api_lldp_status->neighbor_status.if_name);
    lldpnbrstatus->set_routerid(api_lldp_status->neighbor_status.router_id);
    lldpnbrstatus->set_proto(lldpmode_to_proto_lldpmode(api_lldp_status->neighbor_status.mode));
    lldpnbrstatus->set_age((api_lldp_status->neighbor_status.age.length() > 0) ?
                           api_lldp_status->neighbor_status.age.c_str() : "");
    lldpchstatus = lldpnbrstatus->mutable_lldpifchassisstatus();
    lldpchstatus->set_sysname(api_lldp_status->neighbor_status.chassis_status.sysname);
    lldpchstatus->set_sysdescr(api_lldp_status->neighbor_status.chassis_status.sysdescr);
    ipv4_addr_to_spec(lldpchstatus->mutable_mgmtip(),
                      &api_lldp_status->neighbor_status.chassis_status.mgmt_ip);
    lldpchidstatus = lldpchstatus->mutable_chassisid();
    lldpchidstatus->set_type(
        lldpidtype_to_proto_lldpidtype(api_lldp_status->neighbor_status.chassis_status.chassis_id.type));
    lldpchidstatus->set_value((char *)api_lldp_status->neighbor_status.chassis_status.chassis_id.value);
    for (int i = 0; i < api_lldp_status->neighbor_status.chassis_status.num_caps; i++) {
        auto lldpchcap = lldpchstatus->add_capability();
        lldpchcap->set_captype(
            lldpcaptype_to_proto_lldpcaptype(api_lldp_status->neighbor_status.chassis_status.chassis_cap_info[i].cap_type));
        lldpchcap->set_capenabled(api_lldp_status->neighbor_status.chassis_status.chassis_cap_info[i].cap_enabled);
    }
    lldpportstatus = lldpnbrstatus->mutable_lldpifportstatus();
    lldppidstatus = lldpportstatus->mutable_portid();
    lldppidstatus->set_type(lldpidtype_to_proto_lldpidtype(api_lldp_status->neighbor_status.port_status.port_id.type));
    lldppidstatus->set_value((char *)api_lldp_status->neighbor_status.port_status.port_id.value);
    lldpportstatus->set_portdescr(api_lldp_status->neighbor_status.port_status.port_descr);
    lldpportstatus->set_ttl(api_lldp_status->neighbor_status.port_status.ttl);

    for (int i = 0; i < api_lldp_status->neighbor_status.unknown_tlv_status.num_tlvs; i++) {
        auto lldp_unknown_tlv = lldpnbrstatus->add_lldpunknowntlvstatus();
        lldp_unknown_tlv->set_oui((char *)api_lldp_status->neighbor_status.unknown_tlv_status.tlvs[i].oui);
        lldp_unknown_tlv->set_subtype(api_lldp_status->neighbor_status.unknown_tlv_status.tlvs[i].subtype);
        lldp_unknown_tlv->set_len(api_lldp_status->neighbor_status.unknown_tlv_status.tlvs[i].len);
        lldp_unknown_tlv->set_value((char *)api_lldp_status->neighbor_status.unknown_tlv_status.tlvs[i].value);
    }

}

// get the LLDP neighbor and local-interface status for given uplink lif
sdk_ret_t
interface_lldp_status_get (uint16_t uplink_idx, intf::UplinkResponseInfo *rsp)
{
    sdk_ret_t ret = SDK_RET_OK;
    if_lldp_status_t lldp_status;
    std::string interfaces[3] = { "inb_mnic0", "inb_mnic1", "oob_mnic0" };

    if (!is_platform_type_hw()) {
        return SDK_RET_OK;
    }
    
    // LLDP is supported only on uplinks and OOB.
    if (uplink_idx > 2) {
        return SDK_RET_OK;
    }

    memset(&lldp_status, 0, sizeof(if_lldp_status_t));

    ret = sdk::lib::if_lldp_status_get(interfaces[uplink_idx], &lldp_status);
    if (ret != SDK_RET_OK) {
        HAL_TRACE_ERR("Unable to get lldp status for if: {}. ret: {}", 
                      uplink_idx, ret);
        goto end;
    }

    if (strlen(lldp_status.local_if_status.if_name) != 0) {
        // Populate proto
        if_lldp_status_to_proto(rsp, &lldp_status);
    }

end:
    return ret;
}

// get the LLDP interface stats
sdk_ret_t
interface_lldp_stats_get (uint16_t uplink_idx, LldpIfStats *lldp_stats)
{
    sdk_ret_t ret = SDK_RET_OK;
    std::string interfaces[3] = { "inb_mnic0", "inb_mnic1", "oob_mnic0" };
    if_lldp_stats_t api_lldp_stats;

    if (!is_platform_type_hw()) {
        return SDK_RET_OK;
    }

    // LLDP is supported only on uplinks and OOB.
    if (uplink_idx > 2) {
        return SDK_RET_OK;
    }

    memset(&api_lldp_stats, 0, sizeof(if_lldp_stats_t));

    ret = if_lldp_stats_get(interfaces[uplink_idx], &api_lldp_stats);
    if (ret != SDK_RET_OK) {
        HAL_TRACE_ERR("Failed to get LLDP stats. ret: {}", ret);
        goto end;
    }

    // fill the stats
    lldp_stats->set_txcount(api_lldp_stats.tx_count);
    lldp_stats->set_rxcount(api_lldp_stats.rx_count);
    lldp_stats->set_rxdiscarded(api_lldp_stats.rx_discarded);
    lldp_stats->set_rxunrecognized(api_lldp_stats.rx_unrecognized);
    lldp_stats->set_ageoutcount(api_lldp_stats.ageout_count);
    lldp_stats->set_insertcount(api_lldp_stats.insert_count);
    lldp_stats->set_deletecount(api_lldp_stats.delete_count);

end:
    return ret;
}


}    // namespace hal
