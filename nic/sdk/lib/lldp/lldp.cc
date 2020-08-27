//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines LLDP APIs.
///
//----------------------------------------------------------------------------

#include <iostream>
#include "lib/lldp/lldp.hpp"
#include "boost/foreach.hpp"
#include "boost/optional.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

namespace sdk {
namespace lib {

namespace pt = boost::property_tree;

static inline std::string
get_lldpcli_show_cmd (std::string intf, bool nbrs)
{
    char cmd[PATH_MAX];

    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli show %s ports %s -f json0",
             nbrs ? "neighbors" : "interface", intf.c_str());

    return std::string(cmd);
}

static sdk_ret_t
interface_lldp_parse_json (bool nbrs, lldp_status_t *lldp_status, 
                           string json_str)
{
    pt::ptree json_pt;
    string str;
    std::stringstream ss;
    int i = 0;

    SDK_TRACE_VERBOSE("Parsing json: \n%s", json_str.c_str());

    try {
        ss.str(json_str);
        read_json(ss, json_pt);
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        SDK_TRACE_ERR("Error parsing lldp status json");
        return SDK_RET_ERR;
    }

    try {
        BOOST_FOREACH (pt::ptree::value_type &lldp,
                       json_pt.get_child("lldp")) {

            // check if any interfaces present in the lldp tree
            boost::optional<pt::ptree&> interface_opt = lldp.second.get_child_optional("interface");
            if (!interface_opt) {
                continue;
            }

            BOOST_FOREACH (pt::ptree::value_type &iface,
                           lldp.second.get_child("interface")) {
                str = iface.second.get<std::string>("name", "");
                if (!str.empty()) {
                    strncpy(lldp_status->if_name, str.c_str(), LLDP_MAX_NAME_LEN);
                    lldp_status->if_name[LLDP_MAX_NAME_LEN-1] = '\0';
                } else {
                    SDK_TRACE_ERR("  No interface entry");
                }

                str = iface.second.get<std::string>("via", "");
                if (!str.empty()) {
                    if (!str.compare("LLDP")) {
                        lldp_status->mode = LLDP_MODE_LLDP;
                    } else if (!str.compare("CDPv1")) {
                        lldp_status->mode = LLDP_MODE_CDPV1;
                    } else if (!str.compare("CDPv2")) {
                        lldp_status->mode = LLDP_MODE_CDPV2;
                    } else if (!str.compare("EDP")) {
                        lldp_status->mode = LLDP_MODE_EDP;
                    } else if (!str.compare("FDP")) {
                        lldp_status->mode = LLDP_MODE_FDP;
                    } else if (!str.compare("SONMP")) {
                        lldp_status->mode = LLDP_MODE_SONMP;
                    } else {
                        lldp_status->mode = LLDP_MODE_NONE;
                    }
                }

                if (nbrs) {

                    // router id is present only in neighbor tlvs
                    boost::optional<std::string> rid_opt = iface.second.get_optional<std::string>("rid");
                    if (rid_opt) {
                        lldp_status->router_id = iface.second.get<uint32_t>("rid");
                    }
                }

                str = iface.second.get<std::string>("age", "");
                lldp_status->age = str;

                BOOST_FOREACH (pt::ptree::value_type &chassis,
                               iface.second.get_child("chassis")) {

                    BOOST_FOREACH (pt::ptree::value_type &cid,
                                   chassis.second.get_child("id")) {
        
                        str = cid.second.get<std::string>("type", "");
                        if (!str.empty()) {
                            if (!str.compare("ifname")) {
                                lldp_status->chassis_status.chassis_id.type = LLDPID_SUBTYPE_IFNAME;
                            } else if (!str.compare("ifalias")) {
                                lldp_status->chassis_status.chassis_id.type = LLDPID_SUBTYPE_IFALIAS;
                            } else if (!str.compare("local")) {
                                lldp_status->chassis_status.chassis_id.type = LLDPID_SUBTYPE_LOCAL;
                            } else if (!str.compare("mac")) {
                                lldp_status->chassis_status.chassis_id.type = LLDPID_SUBTYPE_MAC;
                            } else if (!str.compare("ip")) {
                                lldp_status->chassis_status.chassis_id.type = LLDPID_SUBTYPE_IP;
                            } else if (!str.compare("port")) {
                                lldp_status->chassis_status.chassis_id.type = LLDPID_SUBTYPE_PORT;
                            } else if (!str.compare("chassis")) {
                                lldp_status->chassis_status.chassis_id.type = LLDPID_SUBTYPE_CHASSIS;
                            } else {
                                lldp_status->chassis_status.chassis_id.type = LLDPID_SUBTYPE_NONE;
                            }
                        }
            
                        str = cid.second.get<std::string>("value", "");
                        if (!str.empty()) {
                            strncpy((char *)lldp_status->chassis_status.chassis_id.value, str.c_str(),
                                    LLDP_MAX_NAME_LEN);
                            lldp_status->chassis_status.chassis_id.value[LLDP_MAX_NAME_LEN-1] = '\0';
                        }
                    }
            
                    BOOST_FOREACH (pt::ptree::value_type &cname,
                                   chassis.second.get_child("name")) {
                        str = cname.second.get<std::string>("value", "");
                        if (!str.empty()) {
                            strncpy(lldp_status->chassis_status.sysname, str.c_str(),
                                    LLDP_MAX_NAME_LEN);
                            lldp_status->chassis_status.sysname[LLDP_MAX_NAME_LEN-1] = '\0';
                        }
                    }

                    BOOST_FOREACH (pt::ptree::value_type &cdescr,
                                   chassis.second.get_child("descr")) {
                        str = cdescr.second.get<std::string>("value", "");
                        if (!str.empty()) {
                            strncpy(lldp_status->chassis_status.sysdescr, str.c_str(),
                                    LLDP_MAX_DESCR_LEN);
                            lldp_status->chassis_status.sysdescr[LLDP_MAX_DESCR_LEN-1] = '\0';
                        }
                    }

                    // mgmt ip tlv may not be present in certain chassis tlvs
                    boost::optional<pt::ptree&> mgmtip_opt = chassis.second.get_child_optional("mgmt-ip");
                    if (mgmtip_opt) {
                        BOOST_FOREACH (pt::ptree::value_type &cmgmtip,
                                       chassis.second.get_child("mgmt-ip")) {
                            str2ipv4addr(cmgmtip.second.get<std::string>("value").c_str(),
                                         &lldp_status->chassis_status.mgmt_ip);
                        }
                    }

                    boost::optional<pt::ptree&> cap_opt = chassis.second.get_child_optional("capability");
                    if (cap_opt) {
                        i = 0;
                        BOOST_FOREACH (pt::ptree::value_type &capability,
                                       chassis.second.get_child("capability")) {

                            str = capability.second.get<std::string>("type", "");

                            if (!str.empty()) {
                                if (!str.compare("Repeater")) {
                                    lldp_status->chassis_status.chassis_cap_info[i].cap_type = LLDP_CAPTYPE_REPEATER;
                                } else if (!str.compare("Bridge")) {
                                    lldp_status->chassis_status.chassis_cap_info[i].cap_type = LLDP_CAPTYPE_BRIDGE;
                                } else if (!str.compare("Router")) {
                                    lldp_status->chassis_status.chassis_cap_info[i].cap_type = LLDP_CAPTYPE_ROUTER;
                                } else if (!str.compare("Wlan")) {
                                    lldp_status->chassis_status.chassis_cap_info[i].cap_type = LLDP_CAPTYPE_WLAN;
                                } else if (!str.compare("Telephone")) {
                                    lldp_status->chassis_status.chassis_cap_info[i].cap_type = LLDP_CAPTYPE_TELEPHONE;
                                } else if (!str.compare("Docsis")) {
                                    lldp_status->chassis_status.chassis_cap_info[i].cap_type = LLDP_CAPTYPE_DOCSIS;
                                } else if (!str.compare("Station")) {
                                    lldp_status->chassis_status.chassis_cap_info[i].cap_type = LLDP_CAPTYPE_STATION;
                                } else {
                                    lldp_status->chassis_status.chassis_cap_info[i].cap_type = LLDP_CAPTYPE_OTHER;
                                }
                            }

                            lldp_status->chassis_status.chassis_cap_info[i].cap_enabled = capability.second.get<bool>("enabled");
                            i++;
                            if (i >= LLDP_MAX_CAPS) {
                                break;
                            }
                        }
                    }
                }

                // record the number of capabilities seen.
                lldp_status->chassis_status.num_caps = i;

                BOOST_FOREACH (pt::ptree::value_type &port,
                               iface.second.get_child("port")) {        

                    BOOST_FOREACH (pt::ptree::value_type &pid,
                                   port.second.get_child("id")) {
            
                        str = pid.second.get<std::string>("type", "");
                        if (!str.empty()) {
                            if (!str.compare("ifname")) {
                                lldp_status->port_status.port_id.type = LLDPID_SUBTYPE_IFNAME;
                            } else if (!str.compare("ifalias")) {
                                lldp_status->port_status.port_id.type = LLDPID_SUBTYPE_IFALIAS;
                            } else if (!str.compare("local")) {
                                lldp_status->port_status.port_id.type = LLDPID_SUBTYPE_LOCAL;
                            } else if (!str.compare("mac")) {
                                lldp_status->port_status.port_id.type = LLDPID_SUBTYPE_MAC;
                            } else if (!str.compare("ip")) {
                                lldp_status->port_status.port_id.type = LLDPID_SUBTYPE_IP;
                            } else if (!str.compare("port")) {
                                lldp_status->port_status.port_id.type = LLDPID_SUBTYPE_PORT;
                            } else if (!str.compare("chassis")) {
                                lldp_status->port_status.port_id.type = LLDPID_SUBTYPE_CHASSIS;
                            } else {
                                lldp_status->port_status.port_id.type = LLDPID_SUBTYPE_NONE;
                            }
                        }
            
                        str = pid.second.get<std::string>("value", "");
                        if (!str.empty()) {
                            strncpy((char *)lldp_status->port_status.port_id.value, str.c_str(),
                                    LLDP_MAX_NAME_LEN);
                            lldp_status->port_status.port_id.value[LLDP_MAX_NAME_LEN-1] = '\0';
                        }
                    }

                    BOOST_FOREACH (pt::ptree::value_type &pdescr,
                                   port.second.get_child("descr")) {
                        str = pdescr.second.get<std::string>("value", "");
                        if (!str.empty()) {
                            strncpy(lldp_status->port_status.port_descr, str.c_str(),
                                    LLDP_MAX_DESCR_LEN);
                            lldp_status->port_status.port_descr[LLDP_MAX_DESCR_LEN-1] = '\0';
                        }
                    }

                    // ttl tlv is present inside the port block only in neighbor tlvs
                    boost::optional<pt::ptree&> ttl_block = port.second.get_child_optional("ttl");
                    if (ttl_block) {
                        BOOST_FOREACH (pt::ptree::value_type &pttl,
                                       port.second.get_child("ttl")) {
                            lldp_status->port_status.ttl = pttl.second.get<uint32_t>("value");
                        }
                    }
                }

                lldp_status->unknown_tlv_status.num_tlvs = 0;
                boost::optional<pt::ptree&> unknown_tlv_opt = iface.second.get_child_optional("unknown-tlvs");
                if (unknown_tlv_opt) {
                    BOOST_FOREACH (pt::ptree::value_type &unknown_tlvs,
                                   iface.second.get_child("unknown-tlvs")) {

                        i = 0;
                        BOOST_FOREACH (pt::ptree::value_type &tlv,
                                       unknown_tlvs.second.get_child("unknown-tlv")) {
                            str = tlv.second.get<std::string>("oui", "");
                            if (!str.empty()) {
                                strncpy(lldp_status->unknown_tlv_status.tlvs[i].oui, str.c_str(),
                                        LLDP_MAX_OUI_LEN);
                                lldp_status->unknown_tlv_status.tlvs[i].oui[LLDP_MAX_OUI_LEN-1] = '\0';
                            }

                            lldp_status->unknown_tlv_status.tlvs[i].subtype = tlv.second.get<uint32_t>("subtype");
                            lldp_status->unknown_tlv_status.tlvs[i].len = tlv.second.get<uint32_t>("len");
                            str = tlv.second.get<std::string>("value", "");
                            if (!str.empty()) {
                                strncpy(lldp_status->unknown_tlv_status.tlvs[i].value, str.c_str(),
                                        LLDP_MAX_NAME_LEN);
                                lldp_status->unknown_tlv_status.tlvs[i].value[LLDP_MAX_NAME_LEN-1] = '\0';
                            }
                            i++;
                            if (i >= LLDP_MAX_UNK_TLVS) {
                                break;
                            }
                        }
                    }

                    // record the number of unknown tlvs seen
                    lldp_status->unknown_tlv_status.num_tlvs = i;
                }

                // in the interface output, the ttl block is present under interface block
                boost::optional<pt::ptree&> ttl_block = iface.second.get_child_optional("ttl");
                if (ttl_block) {
                    BOOST_FOREACH (pt::ptree::value_type &ttl_list,
                                   iface.second.get_child("ttl")) {
                        lldp_status->port_status.ttl = ttl_list.second.get<uint32_t>("ttl");
                    }
                }
            }
        }
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        SDK_TRACE_ERR("Error reading lldp status json");
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

std::string 
cmd_exec (const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

    if (!pipe) {
        SDK_TRACE_ERR("popen failed...skip lldp query");
        goto end;
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

end:
    return result;
}

//------------------------------------------------------------------------------
// get the LLDP neighbor and local-interface status for given uplink if name
//------------------------------------------------------------------------------
sdk_ret_t
if_lldp_status_get (std::string if_name, if_lldp_status_t *lldp_status)
{
    std::string output;

    // get the LLDP neighbor info in json format 
    std::string lldpcmd1 = get_lldpcli_show_cmd(if_name, 1);
    output = cmd_exec(lldpcmd1.c_str());

    // parse the LLDP nbr status output json file and populate lldp status
    interface_lldp_parse_json(1, &lldp_status->neighbor_status, output);

    // get the LLDP local if info in json format 
    std::string lldpcmd2 = get_lldpcli_show_cmd(if_name, 0);
    output = cmd_exec(lldpcmd2.c_str());

    // parse the LLDP local if status output json file and populate lldp status
    interface_lldp_parse_json(0, &lldp_status->local_if_status, output);

    return SDK_RET_OK;
}

static sdk_ret_t
lldp_stats_parse_json (if_lldp_stats_t *lldp_stats, string json_str)
{
    pt::ptree json_pt;
    string    str;
    std::stringstream ss;

    try {
        ss.str(json_str);
        read_json(ss, json_pt);
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        SDK_TRACE_ERR("Error parsing lldp stats json");
        return SDK_RET_ERR;
    }

    try {
        BOOST_FOREACH (pt::ptree::value_type &lldp,
                       json_pt.get_child("lldp")) {
            BOOST_FOREACH (pt::ptree::value_type &iface,
                           lldp.second.get_child("interface")) {
                str = iface.second.get<std::string>("name", "");
                if (str.empty()) {
                    SDK_TRACE_ERR("  No interface entry");
                }

                BOOST_FOREACH (pt::ptree::value_type &tx,
                               iface.second.get_child("tx")) {
                    lldp_stats->tx_count = tx.second.get<uint32_t>("tx");
                }

                BOOST_FOREACH (pt::ptree::value_type &rx,
                               iface.second.get_child("rx")) {
                    lldp_stats->rx_count = rx.second.get<uint32_t>("rx");
                }

                BOOST_FOREACH (pt::ptree::value_type &rx_discarded,
                               iface.second.get_child("rx_discarded_cnt")) {
                    lldp_stats->rx_discarded =
                        rx_discarded.second.get<uint32_t>("rx_discarded_cnt");
                }

                BOOST_FOREACH (pt::ptree::value_type &rx_unrecognized,
                               iface.second.get_child("rx_unrecognized_cnt")) {
                    lldp_stats->rx_unrecognized =
                        rx_unrecognized.second.get<uint32_t>("rx_unrecognized_cnt");
                }

                BOOST_FOREACH (pt::ptree::value_type &ageout,
                               iface.second.get_child("ageout_cnt")) {
                    lldp_stats->ageout_count =
                        ageout.second.get<uint32_t>("ageout_cnt");
                }

                BOOST_FOREACH (pt::ptree::value_type &insert,
                               iface.second.get_child("insert_cnt")) {
                    lldp_stats->insert_count =
                        insert.second.get<uint32_t>("insert_cnt");
                }

                BOOST_FOREACH (pt::ptree::value_type &delete_cnt,
                               iface.second.get_child("delete_cnt")) {
                    lldp_stats->delete_count =
                        delete_cnt.second.get<uint32_t>("delete_cnt");
                }
            }
        }
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        SDK_TRACE_ERR("Error reading lldp stats json");
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

//------------------------------------------------------------------------------
// get LLDP stats on interfaces
//------------------------------------------------------------------------------
sdk_ret_t 
if_lldp_stats_get (std::string if_name, if_lldp_stats_t *lldp_stats)
{
    char cmd[PATH_MAX];
    std::string output;

    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli show statistics ports %s -f json0",
             if_name.c_str());
    output = cmd_exec(cmd);
    
    if (!output.empty()) {
        lldp_stats_parse_json(lldp_stats, output);
    }

    return SDK_RET_OK;
}

}    // namespace lib
}    // namespace sdk

