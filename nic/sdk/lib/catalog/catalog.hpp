// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __CATALOG_HPP__
#define __CATALOG_HPP__

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "include/sdk/base.hpp"
#include "include/sdk/types.hpp"

namespace sdk {
namespace lib {

using boost::property_tree::ptree;

#define MAX_ASICS          1
#define MAX_ASIC_PORTS     8
#define MAX_UPLINK_PORTS   MAX_ASIC_PORTS
#define MAX_PORT_LANES     4
#define MAX_MAC_PROFILES   11
#define MAX_PORT_SPEEDS    6
#define MAX_SERDES         9
#define SERDES_SBUS_START  34
#define MAX_CABLE_TYPE     2

typedef enum mac_mode_e {
    MAC_MODE_1x100g,
    MAC_MODE_1x40g,
    MAC_MODE_1x50g,
    MAC_MODE_2x40g,
    MAC_MODE_2x50g,
    MAC_MODE_1x50g_2x25g,
    MAC_MODE_2x25g_1x50g,
    MAC_MODE_4x25g,
    MAC_MODE_4x10g,
    MAC_MODE_4x1g
} mac_mode_t;

typedef struct serdes_info_s {
    uint32_t sbus_divider;
    uint32_t tx_slip_value;
    uint32_t rx_slip_value;
    uint32_t width;
    uint8_t  tx_pol;
    uint8_t  rx_pol;
} serdes_info_t;


typedef struct ch_profile_ {
   uint32_t ch_mode;
   uint32_t speed;
   uint32_t port_enable;
} ch_profile_t;

typedef struct mac_profile_ {
   uint32_t     glbl_mode;
   uint32_t     channel;
   uint32_t     tdm_slot;
   ch_profile_t ch_profile[MAX_PORT_LANES];
} mac_profile_t;

typedef struct catalog_uplink_port_s {
    uint32_t          asic;
    uint32_t          asic_port;
    uint32_t          num_lanes;
    bool              enabled;
    port_speed_t      speed;
    port_type_t       type;
} catalog_uplink_port_t;

typedef struct catalog_asic_port_s {
    uint32_t    mac_id;
    uint32_t    mac_ch;
    uint32_t    sbus_addr;
} catalog_asic_port_t;

typedef struct catalog_asic_s {
    std::string  name;
    uint32_t     max_ports;
    catalog_asic_port_t ports[MAX_ASIC_PORTS];
} catalog_asic_t;

typedef struct qos_profile_s {
    bool sw_init_enable;
    bool sw_cfg_write_enable;
    uint32_t jumbo_mtu;
    uint32_t num_uplink_qs;
    uint32_t num_p4ig_qs;
    uint32_t num_p4eg_qs;
    uint32_t num_dma_qs;
} qos_profile_t;

typedef struct catalog_s {
    uint32_t                 card_index;                        // card index for the board
    uint32_t                 max_mpu_per_stage;                 // max MPU per pipeline stage
    uint32_t                 mpu_trace_size;                    // MPU trace size
    uint64_t                 cores_mask;                        // mask of all control/data cores
    uint32_t                 num_asics;                         // number of asics on the board
    uint32_t                 num_uplink_ports;                  // number of uplinks in the board
    platform_type_t          platform_type;                     // platform type
    bool                     access_mock_mode;                  // do not access HW, dump only reads/writes
    catalog_asic_t           asics[MAX_ASICS];                  // per asic information
    catalog_uplink_port_t    uplink_ports[MAX_UPLINK_PORTS];    // per port information
    qos_profile_t            qos_profile;                       // qos asic profile
    mac_profile_t            mac_profiles[MAX_MAC_PROFILES];    // MAC profiles

    // serdes parameters
    serdes_info_t            serdes[MAX_SERDES][MAX_PORT_SPEEDS][MAX_CABLE_TYPE];
} catalog_t;

class catalog {
public:
    static catalog *factory(std::string catalog_file);
    static void destroy(catalog *clog);
    static port_speed_t catalog_speed_to_port_speed(std::string speed);
    static port_type_t catalog_type_to_port_type(std::string type);
    static platform_type_t catalog_platform_type_to_platform_type(
                                            std::string platform_type);

    static sdk_ret_t get_child_str(std::string catalog_file,
                                   std::string path,
                                   std::string& child_str);

    catalog_t *catalog_db(void) { return &catalog_db_; }
    uint32_t num_uplink_ports(void) const { return catalog_db_.num_uplink_ports; }
    platform_type_t platform_type(void) const { return catalog_db_.platform_type; }
    bool access_mock_mode(void) { return catalog_db_.access_mock_mode; }
    uint32_t sbus_addr(uint32_t asic_num, uint32_t asic_port, uint32_t lane);

    port_speed_t port_speed(uint32_t port);
    uint32_t     num_lanes (uint32_t port);
    port_type_t  port_type (uint32_t port);
    bool         enabled   (uint32_t port);
    uint32_t     mac_id    (uint32_t port, uint32_t lane);
    uint32_t     mac_ch    (uint32_t port, uint32_t lane);
    uint32_t     sbus_addr (uint32_t port, uint32_t lane);

    uint32_t     glbl_mode  (mac_mode_t mac_mode);
    uint32_t     ch_mode    (mac_mode_t mac_mode, uint32_t ch);

    serdes_info_t* serdes_info_get(uint32_t sbus_addr,
                                   uint32_t port_speed,
                                   uint32_t cable_type);

    uint32_t max_mpu_per_stage(void) const {
        return catalog_db_.max_mpu_per_stage;
    }

    uint32_t mpu_trace_size (void) const {
        return catalog_db_.mpu_trace_size;
    }

    uint64_t cores_mask (void) const { return catalog_db_.cores_mask; }

    const qos_profile_t* qos_profile(void) { return &catalog_db_.qos_profile; }
    bool qos_sw_init_enabled(void) { return catalog_db_.qos_profile.sw_init_enable; }

private:
    catalog_t    catalog_db_;   // whole catalog database

private:
    catalog() {};
    ~catalog();

    sdk_ret_t init(std::string &catalog_file);

    // populate the board level config
    sdk_ret_t populate_catalog(std::string &catalog_file, ptree &prop_tree);

    // populate asic level config
    sdk_ret_t populate_asic(ptree::value_type &asic,
                            catalog_asic_t *asic_p);

    // populate config for all asics
    sdk_ret_t populate_asics(ptree &prop_tree);

    // populate asic port level config
    sdk_ret_t populate_asic_port(ptree::value_type &asic_port,
                                 catalog_asic_port_t *asic_port_p);

    // populate config for all ports
    sdk_ret_t populate_asic_ports(ptree::value_type &asic,
                                  catalog_asic_t *asic_p);

    // populate uplink port level config
    sdk_ret_t populate_uplink_port(ptree::value_type &uplink_port,
                                   catalog_uplink_port_t *uplink_port_p);

    // populate config for all uplink ports
    sdk_ret_t populate_uplink_ports(ptree &prop_tree);

    catalog_uplink_port_t *uplink_port(uint32_t port);

    // populate qos asic profile
    sdk_ret_t populate_qos_profile(ptree &prop_tree);

    sdk_ret_t populate_mac_profile(mac_profile_t *mac_profile,
                                   std::string   str,
                                   ptree         &prop_tree);

    sdk_ret_t populate_mac_profiles(ptree &prop_tree);

    sdk_ret_t populate_mac_ch_profile(ch_profile_t *ch_profile,
                                      std::string  profile_str,
                                      ptree        &prop_tree);

    static sdk_ret_t get_ptree_(std::string& catalog_file, ptree& prop_tree);

    sdk_ret_t serdes_init(std::string& serdes_file);
    sdk_ret_t populate_serdes(ptree &prop_tree);
    uint32_t  serdes_index_get(uint32_t sbus_addr);
    uint8_t   cable_type_get(std::string cable_type_str);
};

}    // namespace lib
}    // namespace sdk

using sdk::lib::mac_mode_t;
using sdk::lib::serdes_info_t;

#define MAC_MODE_1x100g mac_mode_t::MAC_MODE_1x100g
#define MAC_MODE_1x40g mac_mode_t::MAC_MODE_1x40g
#define MAC_MODE_1x50g mac_mode_t::MAC_MODE_1x50g
#define MAC_MODE_2x40g mac_mode_t::MAC_MODE_2x40g
#define MAC_MODE_2x50g mac_mode_t::MAC_MODE_2x50g
#define MAC_MODE_1x50g_2x25g mac_mode_t::MAC_MODE_1x50g_2x25g
#define MAC_MODE_2x25g_1x50g mac_mode_t::MAC_MODE_2x25g_1x50g
#define MAC_MODE_4x25g mac_mode_t::MAC_MODE_4x25g
#define MAC_MODE_4x10g mac_mode_t::MAC_MODE_4x10g
#define MAC_MODE_4x1g mac_mode_t::MAC_MODE_4x1g

#endif    //__CATALOG_HPP__

