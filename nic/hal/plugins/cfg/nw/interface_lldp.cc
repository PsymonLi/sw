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
#include "gen/proto/interface.pb.h"

#define TNNL_ENC_TYPE intf::IfTunnelEncapType

using intf::LldpIfStatus;

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
    return HAL_RET_OK;
}

static inline std::string
get_lldpcli_show_cmd (std::string intf, bool nbrs, std::string status_file)
{
    char cmd[PATH_MAX];

    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli show %s ports %s -f json0 > %s",
             nbrs ? "neighbors" : "interface", intf.c_str(), status_file.c_str());
    return std::string(cmd);
}

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
    std::ifstream json_cfg(lldp_status_file.c_str());
    read_json(json_cfg, json_pt);

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
        HAL_TRACE_ERR("Error reading lldp status json");
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

// get the LLDP neighbor and local-interface status for given uplink lif
sdk_ret_t
interface_lldp_status_get (uint16_t uplink_idx, intf::UplinkResponseInfo *rsp)
{
    int         rc;
    static int  iter = 0;
    std::string lldp_status_file;
    std::string interfaces[3] = { "inb_mnic0", "inb_mnic1", "oob_mnic0" };

    if (!is_platform_type_hw()) {
        return SDK_RET_OK;
    }
    
    HAL_TRACE_DEBUG("Getting lldp for if_idx: {}", uplink_idx);

    // LLDP is supported only on uplinks and OOB.
    if (uplink_idx > 2) {
        return SDK_RET_OK;
    }

    auto lldp_status = rsp->mutable_lldpstatus();

    // create a new file for output json for every get call, in order to
    // support concurrency
    lldp_status_file = "/tmp/" + std::to_string(iter++) + "_lldp_status.json";

    // get the LLDP neighbor info in json format 
    std::string lldpcmd1 = get_lldpcli_show_cmd(interfaces[uplink_idx], 1, lldp_status_file);
    HAL_TRACE_DEBUG("lldp neighbor cmd: {}", lldpcmd1);
    rc = system(lldpcmd1.c_str());
    if (rc == -1) {
        HAL_TRACE_ERR("lldp show command for neighbors failed for {} with error {}",
                      interfaces[uplink_idx].c_str(), rc);
        remove(lldp_status_file.c_str());
        return SDK_RET_ERR;
    }

    auto lldp_nbr_ifstatus = lldp_status->mutable_lldpnbrstatus();
    // parse the LLDP nbr status output json file and populate lldp status
    interface_lldp_parse_json(1, lldp_nbr_ifstatus, lldp_status_file);

    // get the LLDP local if info in json format 
    std::string lldpcmd2 = get_lldpcli_show_cmd(interfaces[uplink_idx], 0, lldp_status_file);
    HAL_TRACE_DEBUG("lldp neighbor cmd: {}", lldpcmd2);
    rc = system(lldpcmd2.c_str());
    if (rc == -1) {
        HAL_TRACE_ERR("lldp show command for interface failed for {} with error {}",
                      interfaces[uplink_idx].c_str(), rc);
        remove(lldp_status_file.c_str());
        return SDK_RET_ERR;
    }

    auto lldp_ifstatus = lldp_status->mutable_lldpifstatus();
    // parse the LLDP local if status output json file and populate lldp status
    interface_lldp_parse_json(0, lldp_ifstatus, lldp_status_file);

    // remove the json output file
    // remove(lldp_status_file.c_str());

    return(SDK_RET_OK);
}

static sdk_ret_t
lldp_stats_parse_json (LldpIfStats *lldp_stats,
                       std::string lldp_stats_file)
{
    pt::ptree json_pt;
    string    str;

    // parse the lldp stats output json file
    if (access(lldp_stats_file.c_str(), R_OK) < 0) {
        HAL_TRACE_ERR("{} doesn't exist or not accessible\n", lldp_stats_file.c_str());
        return SDK_RET_ERR;
    }
    std::ifstream json_cfg(lldp_stats_file.c_str());
    read_json(json_cfg, json_pt);

    try {
        BOOST_FOREACH (pt::ptree::value_type &lldp,
                       json_pt.get_child("lldp")) {
            BOOST_FOREACH (pt::ptree::value_type &iface,
                           lldp.second.get_child("interface")) {
                str = iface.second.get<std::string>("name", "");
                if (str.empty()) {
                    HAL_TRACE_ERR("  No interface entry");
                }

                BOOST_FOREACH (pt::ptree::value_type &tx,
                               iface.second.get_child("tx")) {
                    lldp_stats->set_txcount(tx.second.get<uint32_t>("tx"));
                }

                BOOST_FOREACH (pt::ptree::value_type &rx,
                               iface.second.get_child("rx")) {
                    lldp_stats->set_rxcount(rx.second.get<uint32_t>("rx"));
                }

                BOOST_FOREACH (pt::ptree::value_type &rx_discarded,
                               iface.second.get_child("rx_discarded_cnt")) {
                    lldp_stats->set_rxdiscarded(rx_discarded.second.get<uint32_t>("rx_discarded_cnt"));
                }

                BOOST_FOREACH (pt::ptree::value_type &rx_unrecognized,
                               iface.second.get_child("rx_unrecognized_cnt")) {
                    lldp_stats->set_rxunrecognized(
                        rx_unrecognized.second.get<uint32_t>("rx_unrecognized_cnt"));
                }

                BOOST_FOREACH (pt::ptree::value_type &ageout,
                               iface.second.get_child("ageout_cnt")) {
                    lldp_stats->set_ageoutcount(
                        ageout.second.get<uint32_t>("ageout_cnt"));
                }

                BOOST_FOREACH (pt::ptree::value_type &insert,
                               iface.second.get_child("insert_cnt")) {
                    lldp_stats->set_insertcount(
                        insert.second.get<uint32_t>("insert_cnt"));
                }

                BOOST_FOREACH (pt::ptree::value_type &delete_cnt,
                               iface.second.get_child("delete_cnt")) {
                    lldp_stats->set_deletecount(
                        delete_cnt.second.get<uint32_t>("delete_cnt"));
                }
            }
        }
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        HAL_TRACE_ERR("Error reading lldp stats json");
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

// get the LLDP interface stats
sdk_ret_t
interface_lldp_stats_get (uint16_t uplink_idx, LldpIfStats *lldp_stats)
{
    int         rc;
    static int  iter = 0;
    std::string stats_file;
    std::string interfaces[3] = { "inb_mnic0", "inb_mnic1", "oob_mnic0" };
    char        cmd[PATH_MAX];

    if (!is_platform_type_hw()) {
        return SDK_RET_OK;
    }

    // LLDP is supported only on uplinks and OOB.
    if (uplink_idx > 2) {
        return SDK_RET_OK;
    }

    // create a new file for output json for every get call, in order to
    // support concurrency
    stats_file = "/tmp/" + std::to_string(iter++) + "_lldp_stats.json";

    // get the LLDP stats in json format
    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli show statistics ports %s -f json0 > %s",
             interfaces[uplink_idx].c_str(), stats_file.c_str());
    std::string lldpcmd = std::string(cmd);
    rc = system(lldpcmd.c_str());
    if (rc == -1) {
        HAL_TRACE_ERR("lldp stats command failed for {} with error {}",
                      interfaces[uplink_idx].c_str(), rc);
        remove(stats_file.c_str());
        return SDK_RET_ERR;
    }

    // parse the LLDP stats output json file and populate lldp stats
    lldp_stats_parse_json(lldp_stats, stats_file);

    remove(stats_file.c_str());
    return SDK_RET_OK;
}


}    // namespace hal
