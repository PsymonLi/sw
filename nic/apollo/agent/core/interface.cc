//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// -----------------------------------------------------------------------------

#include <iostream>
#include "boost/foreach.hpp"
#include "boost/optional.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "nic/apollo/agent/trace.hpp"
#include "nic/apollo/agent/core/interface.hpp"
#include "nic/apollo/agent/core/state.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/metaswitch/stubs/mgmt/pds_ms_interface.hpp"

using std::string;
namespace pt = boost::property_tree;

namespace core {

sdk_ret_t
interface_create (pds_if_spec_t *spec,
                  pds_batch_ctxt_t bctxt)
{
    sdk_ret_t ret;
    if ((spec->type == IF_TYPE_L3) ||
        (spec->type == IF_TYPE_LOOPBACK)) {
        // L3 and loopback interfaces are always sent to control-plane
        // irrespective of overlay routing mode
        if ((ret = pds_ms::interface_create(spec, bctxt)) != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to create interface {}, err {}",
                          spec->key.str(), ret);
            return ret;
        }
    } else if (!agent_state::state()->pds_mock_mode()) {
        pds_if_info_t info;
        if (pds_if_read(&spec->key, &info) != SDK_RET_ENTRY_NOT_FOUND) {
            PDS_TRACE_ERR("Failed to create interface {}, interface "
                          "exists already", spec->key.str());
            return sdk::SDK_RET_ENTRY_EXISTS;
        }
        if ((ret = pds_if_create(spec, bctxt)) != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to create interface {}, err {}",
                          spec->key.str(), ret);
            return ret;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
interface_update (pds_if_spec_t *spec,
                  pds_batch_ctxt_t bctxt)
{
    sdk_ret_t ret;

    if ((spec->type == IF_TYPE_L3) ||
        (spec->type == IF_TYPE_LOOPBACK)) {
        // L3 and loopback interfaces are always sent to control-plane
        // irrespective of overlay routing mode
        if ((ret = pds_ms::interface_update(spec, bctxt)) != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to update interface {}, err {}",
                          spec->key.str(), ret);
            return ret;
        }
    } else if (!agent_state::state()->pds_mock_mode()) {
        pds_if_info_t info;
        ret = pds_if_read(&spec->key, &info);
        if (ret != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to read interface {}, err {}",
                          spec->key.str(), ret);
            return ret;
        }
        if ((ret = pds_if_update(spec, bctxt)) != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to update interface {}, err {}",
                          spec->key.str(), ret);
            return ret;
        }
    }

    return SDK_RET_OK;
}

sdk_ret_t
interface_delete (pds_obj_key_t *key, pds_batch_ctxt_t bctxt)
{
    sdk_ret_t ret;

    // attempt to delete from control-plane
    ret = pds_ms::interface_delete(key, bctxt);

    if (ret == SDK_RET_ENTRY_NOT_FOUND) {
        // if this interface is not found in control-plane then
        // it might be a non-L3, non-loopback interface 
        PDS_TRACE_DEBUG("Deleting non-controlplane interface {}", key->str());

        pds_if_info_t info;
        if (pds_if_read(key, &info) != SDK_RET_OK) {
            PDS_TRACE_ERR("Failed to delete interface {}, interface not found",
                          key->str());
            return sdk::SDK_RET_ENTRY_EXISTS;
        }
        ret = pds_if_delete(key, bctxt);
    }
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to delete interface {}, err {}",
                      key->str(), ret);
        return ret;
    }
    return SDK_RET_OK;
}

sdk_ret_t
interface_get (pds_obj_key_t *key, pds_if_info_t *info)
{
    sdk_ret_t ret = SDK_RET_OK;

    if (!agent_state::state()->pds_mock_mode()) {
        ret = pds_if_read(key, info);
    } else {
        memset(&info->spec, 0, sizeof(info->spec));
        memset(&info->stats, 0, sizeof(info->stats));
        memset(&info->status, 0, sizeof(info->status));
    }
    return ret;
}

sdk_ret_t
interface_get_all (if_read_cb_t cb, void *ctxt)
{
    return pds_if_read_all(cb, ctxt);
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
interface_lldp_parse_json (bool nbrs, pds_if_lldp_info_t *lldp_status,
                           std::string lldp_status_file)
{
    pt::ptree json_pt;
    string    str;
    int       i;

    // parse the lldp status output json file
    if (access(lldp_status_file.c_str(), R_OK) < 0) {
        PDS_TRACE_ERR("{} doesn't exist or not accessible\n", lldp_status_file.c_str());
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
                    strncpy(lldp_status->if_name, str.c_str(), PDS_LLDP_MAX_NAME_LEN);
                } else {
                    PDS_TRACE_ERR("  No interface entry");
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
                    lldp_status->router_id = iface.second.get<uint32_t>("rid");
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
                                lldp_status->chassis_spec.chassis_id.type = LLDPID_SUBTYPE_IFNAME;
                            } else if (!str.compare("ifalias")) {
                                lldp_status->chassis_spec.chassis_id.type = LLDPID_SUBTYPE_IFALIAS;
                            } else if (!str.compare("local")) {
                                lldp_status->chassis_spec.chassis_id.type = LLDPID_SUBTYPE_LOCAL;
                            } else if (!str.compare("mac")) {
                                lldp_status->chassis_spec.chassis_id.type = LLDPID_SUBTYPE_MAC;
                            } else if (!str.compare("ip")) {
                                lldp_status->chassis_spec.chassis_id.type = LLDPID_SUBTYPE_IP;
                            } else if (!str.compare("port")) {
                                lldp_status->chassis_spec.chassis_id.type = LLDPID_SUBTYPE_PORT;
                            } else if (!str.compare("chassis")) {
                                lldp_status->chassis_spec.chassis_id.type = LLDPID_SUBTYPE_CHASSIS;
                            } else {
                                lldp_status->chassis_spec.chassis_id.type = LLDPID_SUBTYPE_NONE;
                            }
                        }
            
                        str = cid.second.get<std::string>("value", "");
                        memcpy(lldp_status->chassis_spec.chassis_id.value, str.c_str(),
                               PDS_LLDP_MAX_NAME_LEN);
                    }
            
                    BOOST_FOREACH (pt::ptree::value_type &cname,
                                   chassis.second.get_child("name")) {
                        str = cname.second.get<std::string>("value", "");
                        strncpy(lldp_status->chassis_spec.sysname, str.c_str(),
                                PDS_LLDP_MAX_NAME_LEN);
                    }

                    BOOST_FOREACH (pt::ptree::value_type &cdescr,
                                   chassis.second.get_child("descr")) {
                        str = cdescr.second.get<std::string>("value", "");
                        strncpy(lldp_status->chassis_spec.sysdescr, str.c_str(),
                                PDS_LLDP_MAX_DESCR_LEN);
                    }

                    BOOST_FOREACH (pt::ptree::value_type &cmgmtip,
                                   chassis.second.get_child("mgmt-ip")) {
                        str2ipv4addr(cmgmtip.second.get<std::string>("value").c_str(),
                                     &lldp_status->chassis_spec.mgmt_ip);
                    }

                    i = 0;
                    BOOST_FOREACH (pt::ptree::value_type &capability,
                                   chassis.second.get_child("capability")) {

                        str = capability.second.get<std::string>("type", "");

                        if (!str.empty()) {
                            if (!str.compare("Repeater")) {
                                lldp_status->chassis_spec.chassis_cap_spec[i].cap_type = LLDP_CAPTYPE_REPEATER;
                            } else if (!str.compare("Bridge")) {
                                lldp_status->chassis_spec.chassis_cap_spec[i].cap_type = LLDP_CAPTYPE_BRIDGE;
                            } else if (!str.compare("Router")) {
                                lldp_status->chassis_spec.chassis_cap_spec[i].cap_type = LLDP_CAPTYPE_ROUTER;
                            } else if (!str.compare("Wlan")) {
                                lldp_status->chassis_spec.chassis_cap_spec[i].cap_type = LLDP_CAPTYPE_WLAN;
                            } else if (!str.compare("Telephone")) {
                                lldp_status->chassis_spec.chassis_cap_spec[i].cap_type = LLDP_CAPTYPE_TELEPHONE;
                            } else if (!str.compare("Docsis")) {
                                lldp_status->chassis_spec.chassis_cap_spec[i].cap_type = LLDP_CAPTYPE_DOCSIS;
                            } else if (!str.compare("Station")) {
                                lldp_status->chassis_spec.chassis_cap_spec[i].cap_type = LLDP_CAPTYPE_STATION;
                            } else {
                                lldp_status->chassis_spec.chassis_cap_spec[i].cap_type = LLDP_CAPTYPE_OTHER;
                            }
                        }
                  
                        lldp_status->chassis_spec.chassis_cap_spec[i].cap_enabled = capability.second.get<bool>("enabled");
                        i++;
                        if (i >= PDS_LLDP_MAX_CAPS) break;
                    }
                }

                // record the number of capabilities seen.
                lldp_status->chassis_spec.num_caps = i;

                BOOST_FOREACH (pt::ptree::value_type &port,
                               iface.second.get_child("port")) {        

                    BOOST_FOREACH (pt::ptree::value_type &pid,
                                   port.second.get_child("id")) {
            
                        str = pid.second.get<std::string>("type", "");
                        if (!str.empty()) {
                            if (!str.compare("ifname")) {
                                lldp_status->port_spec.port_id.type = LLDPID_SUBTYPE_IFNAME;
                            } else if (!str.compare("ifalias")) {
                                lldp_status->port_spec.port_id.type = LLDPID_SUBTYPE_IFALIAS;
                            } else if (!str.compare("local")) {
                                lldp_status->port_spec.port_id.type = LLDPID_SUBTYPE_LOCAL;
                            } else if (!str.compare("mac")) {
                                lldp_status->port_spec.port_id.type = LLDPID_SUBTYPE_MAC;
                            } else if (!str.compare("ip")) {
                                lldp_status->port_spec.port_id.type = LLDPID_SUBTYPE_IP;
                            } else if (!str.compare("port")) {
                                lldp_status->port_spec.port_id.type = LLDPID_SUBTYPE_PORT;
                            } else if (!str.compare("chassis")) {
                                lldp_status->port_spec.port_id.type = LLDPID_SUBTYPE_CHASSIS;
                            } else {
                                lldp_status->port_spec.port_id.type = LLDPID_SUBTYPE_NONE;
                            }
                        }
            
                        str = pid.second.get<std::string>("value", "");
                        if (!str.empty()) {
                            memcpy(lldp_status->port_spec.port_id.value, str.c_str(),
                                   PDS_LLDP_MAX_NAME_LEN);
                        }
                    }

                    BOOST_FOREACH (pt::ptree::value_type &pdescr,
                                   port.second.get_child("descr")) {
                        str = pdescr.second.get<std::string>("value", "");
                        if (!str.empty()) {
                            strncpy(lldp_status->port_spec.port_descr, str.c_str(),
                                    PDS_LLDP_MAX_DESCR_LEN);
                        }
                    }

                    if (nbrs) {
                        BOOST_FOREACH (pt::ptree::value_type &pttl,
                                       port.second.get_child("ttl")) {
                            lldp_status->port_spec.ttl = pttl.second.get<uint32_t>("value");
                        }
                    }
                }
            }
        }
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        PDS_TRACE_ERR("Error reading lldp status json");
        remove(lldp_status_file.c_str());
        return SDK_RET_ERR;
    }
    remove(lldp_status_file.c_str());
    return SDK_RET_OK;
}

// get the LLDP neighbor and local-interface status for given uplink lif
sdk_ret_t
interface_lldp_status_get (uint16_t lif_id, pds_if_lldp_status_t *lldp_status)
{
    int         rc;
    static int  iter = 0;
    std::string lldp_status_file;
    std::string interfaces[3] = { "dsc0", "dsc1", "oob_mnic0" };

    if (api::g_pds_state.platform_type() != platform_type_t::PLATFORM_TYPE_HW) {
        return SDK_RET_OK;
    }

    // LLDP is supported only on uplinks and OOB.
    if (lif_id > 2) {
        return SDK_RET_OK;
    }

    // create a new file for output json for every get call, in order to
    // support concurrency
    lldp_status_file = "/tmp/" + std::to_string(iter++) + "_lldp_status.json";

    // get the LLDP neighbor info in json format 
    std::string lldpcmd1 = get_lldpcli_show_cmd(interfaces[lif_id], 1, lldp_status_file);
    rc = system(lldpcmd1.c_str());
    if (rc == -1) {
        PDS_TRACE_ERR("lldp show command for neighbors failed for {} with error {}",
                      interfaces[lif_id].c_str(), rc);
        remove(lldp_status_file.c_str());
        return SDK_RET_ERR;
    }

    // parse the LLDP nbr status output json file and populate lldp status
    interface_lldp_parse_json(1, &lldp_status->neighbor_status, lldp_status_file);

    // get the LLDP local if info in json format 
    std::string lldpcmd2 = get_lldpcli_show_cmd(interfaces[lif_id], 0, lldp_status_file);
    rc = system(lldpcmd2.c_str());
    if (rc == -1) {
        PDS_TRACE_ERR("lldp show command for interface failed for {} with error {}",
                      interfaces[lif_id].c_str(), rc);
        remove(lldp_status_file.c_str());
        return SDK_RET_ERR;
    }

    // parse the LLDP local if status output json file and populate lldp status
    interface_lldp_parse_json(0, &lldp_status->local_if_status, lldp_status_file);

    return(SDK_RET_OK);
}

static sdk_ret_t
lldp_stats_parse_json (pds_if_lldp_stats_t *lldp_stats,
                       std::string lldp_stats_file)
{
    pt::ptree json_pt;
    string    str;

    // parse the lldp stats output json file
    if (access(lldp_stats_file.c_str(), R_OK) < 0) {
        PDS_TRACE_ERR("{} doesn't exist or not accessible\n", lldp_stats_file.c_str());
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
                    PDS_TRACE_ERR("  No interface entry");
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
        PDS_TRACE_ERR("Error reading lldp stats json");
        return SDK_RET_ERR;
    }
    return SDK_RET_OK;
}

// get the LLDP interface stats
sdk_ret_t
interface_lldp_stats_get (uint16_t lif_id, pds_if_lldp_stats_t *lldp_stats)
{
    int         rc;
    static int  iter = 0;
    std::string stats_file;
    std::string interfaces[3] = { "dsc0", "dsc1", "oob_mnic0" };
    char        cmd[PATH_MAX];

    if (api::g_pds_state.platform_type() != platform_type_t::PLATFORM_TYPE_HW) {
        return SDK_RET_OK;
    }

    // LLDP is supported only on uplinks and OOB.
    if (lif_id > 2) {
        return SDK_RET_OK;
    }

    // create a new file for output json for every get call, in order to
    // support concurrency
    stats_file = "/tmp/" + std::to_string(iter++) + "_lldp_stats.json";

    // get the LLDP stats in json format
    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli show statistics ports %s -f json0 > %s",
             interfaces[lif_id].c_str(), stats_file.c_str());
    std::string lldpcmd = std::string(cmd);
    rc = system(lldpcmd.c_str());
    if (rc == -1) {
        PDS_TRACE_ERR("lldp stats command failed for {} with error {}",
                      interfaces[lif_id].c_str(), rc);
        remove(stats_file.c_str());
        return SDK_RET_ERR;
    }

    // parse the LLDP stats output json file and populate lldp stats
    lldp_stats_parse_json(lldp_stats, stats_file);

    remove(stats_file.c_str());
    return SDK_RET_OK;
}

// initialize LLDP config parameters
sdk_ret_t
lldp_config_init (void)
{
    char cmd[PATH_MAX];
    int  rc;

    // set system name to be the FRU mac by default
    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli configure system hostname %s",
             macaddr2str(api::g_pds_state.system_mac()));
    rc = system(cmd);
    if (rc == -1) {
        PDS_TRACE_ERR("lldp configure system hostname {} failed with error {}",
                      macaddr2str(api::g_pds_state.system_mac()), rc);
    }
    PDS_TRACE_DEBUG("Configured LLDP system name {}", cmd);

    // set chassis-id to be the FRU mac by default
    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli configure system chassisid %s",
             macaddr2str(api::g_pds_state.system_mac()));
    rc = system(cmd);
    if (rc == -1) {
        PDS_TRACE_ERR("lldp configure system chassisid {} failed with error {}",
                      macaddr2str(api::g_pds_state.system_mac()), rc);
    }
    PDS_TRACE_DEBUG("Configured LLDP system chassisid {}", cmd);

    // set system description to be the product name by default
    snprintf(cmd, PATH_MAX, "/usr/sbin/lldpcli configure system description \"%s %s\"",
             DSC_DESCRIPTION, api::g_pds_state.product_name().c_str());
    rc = system(cmd);
    if (rc == -1) {
        PDS_TRACE_ERR("lldp configure system description {} failed with error {}",
                      macaddr2str(api::g_pds_state.system_mac(), true), rc);
    }
    PDS_TRACE_DEBUG("Configured LLDP system description {}", cmd);
    return SDK_RET_OK;
}

}    // namespace core
