//------------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
//
#ifndef __SDK_LLDP_HPP__
#define __SDK_LLDP_HPP__

#include "include/sdk/base.hpp"
#include "include/sdk/types.hpp"
#include "include/sdk/ip.hpp"

using std::string;
namespace sdk {
namespace lib {

#define LLDP_MAX_NAME_LEN   64
#define LLDP_MAX_DESCR_LEN  512
#define LLDP_MAX_OUI_LEN    64
#define LLDP_MAX_CAPS       8     ///< max. no of LLDP capabilities
#define LLDP_MAX_UNK_TLVS   8     ///< max. no of LLDP unknown TLVs

/// \brief LLDP interface statistics
typedef struct if_lldp_stats_s {
    uint32_t tx_count;        ///< transmitted pkts
    uint32_t rx_count;        ///< received pkts
    uint32_t rx_discarded;    ///< rx discarded pkts
    uint32_t rx_unrecognized; ///< rx unrecognized pkts
    uint32_t ageout_count;    ///< count of entry aged out
    uint32_t insert_count;    ///< count of inserts
    uint32_t delete_count;    ///< count of deletes
} if_lldp_stats_t;

/// \brief LLDP chassis/port identifier key type
typedef enum if_lldp_idtype_e {
    LLDPID_SUBTYPE_NONE    = 0, ///< "unknown"
    LLDPID_SUBTYPE_IFNAME  = 1, ///< "ifname"
    LLDPID_SUBTYPE_IFALIAS = 2, ///< "ifalias",
    LLDPID_SUBTYPE_LOCAL   = 3, ///< "local"
    LLDPID_SUBTYPE_MAC     = 4, ///< "mac"
    LLDPID_SUBTYPE_IP      = 5, ///< "ip"
    LLDPID_SUBTYPE_PORT    = 6, ///< "portnum"
    LLDPID_SUBTYPE_CHASSIS = 7, ///< "chasiss-str"
} if_lldp_idtype_t;

/// \brief protocol mode used for LLDP
typedef enum if_lldp_mode_e {
    LLDP_MODE_NONE  = 0,
    LLDP_MODE_LLDP  = 1,
    LLDP_MODE_CDPV1 = 2,
    LLDP_MODE_CDPV2 = 3,
    LLDP_MODE_EDP   = 4,
    LLDP_MODE_FDP   = 5,
    LLDP_MODE_SONMP = 6,
} if_lldp_mode_t;

/// \brief key type and value for LLDP chassis/port identifier
typedef struct if_lldp_id_s {
    if_lldp_idtype_t type;
    uint8_t              value[LLDP_MAX_NAME_LEN];
} if_lldp_id_t;

/// \brief capabilities on the interface
typedef enum if_lldp_captype_e {
    LLDP_CAPTYPE_OTHER     = 0,
    LLDP_CAPTYPE_REPEATER  = 1,
    LLDP_CAPTYPE_BRIDGE    = 2,
    LLDP_CAPTYPE_ROUTER    = 3,
    LLDP_CAPTYPE_WLAN      = 4,
    LLDP_CAPTYPE_TELEPHONE = 5,
    LLDP_CAPTYPE_DOCSIS    = 6,
    LLDP_CAPTYPE_STATION   = 7,
} if_lldp_captype_t;

typedef struct lldp_chassis_capability_info_s {
    if_lldp_captype_t cap_type;    ///< capability type
    bool              cap_enabled; ///< enabled/disabled
} lldp_chassis_capability_info_t;

typedef struct lldp_chassis_info_s {
    char                           sysname[LLDP_MAX_NAME_LEN];      ///< system name
    if_lldp_id_t                   chassis_id;                      ///< system identifier
    char                           sysdescr[LLDP_MAX_DESCR_LEN];    ///< description string
    ipv4_addr_t                    mgmt_ip;                         ///< management IP
    lldp_chassis_capability_info_t chassis_cap_info[LLDP_MAX_CAPS]; ///< list of capabilities
    uint16_t                       num_caps;                        ///< number of capabilities
} lldp_chassis_info_t;

typedef lldp_chassis_info_t if_lldp_chassis_status_t;

typedef struct if_lldp_port_status_s {
    if_lldp_id_t port_id;                            ///< port identifier
    char             port_descr[LLDP_MAX_DESCR_LEN]; ///< description string
    uint32_t         ttl;                            ///< TTL
} if_lldp_port_status_t;

typedef struct if_lldp_unknown_tlv_s {
    char     oui[LLDP_MAX_OUI_LEN];    ///< OUI string
    uint32_t subtype;                  ///< subtype
    uint32_t len;                      ///< length
    char     value[LLDP_MAX_NAME_LEN]; ///< value string
} if_lldp_unknown_tlv_t;

typedef struct if_lldp_unknown_tlv_status_s {
    if_lldp_unknown_tlv_t tlvs[LLDP_MAX_UNK_TLVS];
    uint16_t                  num_tlvs;
} if_lldp_unknown_tlv_status_t;

typedef struct lldp_status_s {
    char                         if_name[LLDP_MAX_NAME_LEN]; ///< interface name
    uint32_t                     router_id;                  ///< router-id
    if_lldp_mode_t               mode;                       ///< LLDP/CDP/EDP/FDP/..
    string                       age;                        ///< uptime string
    if_lldp_chassis_status_t     chassis_status;             ///< chassis info
    if_lldp_port_status_t        port_status;                ///< physical port info
    if_lldp_unknown_tlv_status_t unknown_tlv_status;         ///< unknown tlv info
} lldp_status_t;

typedef struct if_lldp_status_s {
    lldp_status_t local_if_status; ///< local interface info
    lldp_status_t neighbor_status; ///< neighbor info
} if_lldp_status_t;

sdk_ret_t if_lldp_status_get(std::string if_name, if_lldp_status_t *lldp_status);
sdk_ret_t if_lldp_stats_get(std::string if_name, if_lldp_stats_t *lldp_stats);

}    // namespace lib
}    // namespace sdk

#endif    // __SDK_LLDP_HPP__
