//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines interface API
///
//----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_INTF_HPP__
#define __INCLUDE_API_PDS_INTF_HPP__

#include "nic/sdk/include/sdk/types.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/sdk/linkmgr/linkmgr.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_mirror.hpp"
#include "nic/sdk/lib/utils/in_mem_fsm_logger.hpp"

/// \defgroup PDS_INTF Interface API
/// @{

#define PDS_INTF_ID_INVALID     0     ///< invalid interface id
#define PDS_LLDP_MAX_NAME_LEN   64
#define PDS_LLDP_MAX_DESCR_LEN  256
#define PDS_LLDP_MAX_OUI_LEN    64
#define PDS_LLDP_MAX_CAPS       8     ///< max. no of LLDP capabilities
#define PDS_LLDP_MAX_UNK_TLVS   8     ///< max. no of LLDP unknown TLVs

/// \brief interface admin/operational state
typedef enum pds_if_state_e {
    PDS_IF_STATE_NONE = 0,
    PDS_IF_STATE_DOWN = 1,
    PDS_IF_STATE_UP   = 2,
} pds_if_state_t;

/// \brief LLDP interface statistics
typedef struct pds_if_lldp_stats_s {
    uint32_t tx_count;        ///< transmitted pkts
    uint32_t rx_count;        ///< received pkts
    uint32_t rx_discarded;    ///< rx discarded pkts
    uint32_t rx_unrecognized; ///< rx unrecognized pkts
    uint32_t ageout_count;    ///< count of entry aged out
    uint32_t insert_count;    ///< count of inserts
    uint32_t delete_count;    ///< count of deletes
} pds_if_lldp_stats_t;

/// \brief LLDP chassis/port identifier key type
typedef enum pds_if_lldp_idtype_e {
    LLDPID_SUBTYPE_NONE    = 0, ///< "unknown"
    LLDPID_SUBTYPE_IFNAME  = 1, ///< "ifname"
    LLDPID_SUBTYPE_IFALIAS = 2, ///< "ifalias",
    LLDPID_SUBTYPE_LOCAL   = 3, ///< "local"
    LLDPID_SUBTYPE_MAC     = 4, ///< "mac"
    LLDPID_SUBTYPE_IP      = 5, ///< "ip"
    LLDPID_SUBTYPE_PORT    = 6, ///< "portnum"
    LLDPID_SUBTYPE_CHASSIS = 7, ///< "chasiss-str"
} pds_if_lldp_idtype_t;

/// \brief protocol mode used for LLDP
typedef enum pds_if_lldp_mode_e {
    LLDP_MODE_NONE  = 0,
    LLDP_MODE_LLDP  = 1,
    LLDP_MODE_CDPV1 = 2,
    LLDP_MODE_CDPV2 = 3,
    LLDP_MODE_EDP   = 4,
    LLDP_MODE_FDP   = 5,
    LLDP_MODE_SONMP = 6,
} pds_if_lldp_mode_t;

/// \brief key type and value for LLDP chassis/port identifier
typedef struct pds_if_lldp_id_s {
    pds_if_lldp_idtype_t type;
    uint8_t              value[PDS_LLDP_MAX_NAME_LEN];
} pds_if_lldp_id_t;

/// \brief capabilities on the interface
typedef enum pds_if_lldp_captype_e {
    LLDP_CAPTYPE_OTHER     = 0,
    LLDP_CAPTYPE_REPEATER  = 1,
    LLDP_CAPTYPE_BRIDGE    = 2,
    LLDP_CAPTYPE_ROUTER    = 3,
    LLDP_CAPTYPE_WLAN      = 4,
    LLDP_CAPTYPE_TELEPHONE = 5,
    LLDP_CAPTYPE_DOCSIS    = 6,
    LLDP_CAPTYPE_STATION   = 7,
} pds_if_lldp_captype_t;

typedef struct pds_lldp_chassis_capability_info_s {
    pds_if_lldp_captype_t cap_type;    ///< capability type
    bool                  cap_enabled; ///< enabled/disabled
} pds_lldp_chassis_capability_info_t;

typedef struct pds_lldp_chassis_info_s {
    char                               sysname[PDS_LLDP_MAX_NAME_LEN];      ///< system name
    pds_if_lldp_id_t                   chassis_id;                          ///< system identifier
    char                               sysdescr[PDS_LLDP_MAX_DESCR_LEN];    ///< description string
    ipv4_addr_t                        mgmt_ip;                             ///< management IP
    pds_lldp_chassis_capability_info_t chassis_cap_info[PDS_LLDP_MAX_CAPS]; ///< list of capabilities
    uint16_t                           num_caps;                            ///< number of capabilities
} pds_lldp_chassis_info_t;

typedef pds_lldp_chassis_info_t pds_if_lldp_chassis_spec_t;
typedef pds_lldp_chassis_info_t pds_if_lldp_chassis_status_t;

typedef struct pds_if_lldp_port_status_s {
    pds_if_lldp_id_t port_id;                            ///< port identifier
    char             port_descr[PDS_LLDP_MAX_DESCR_LEN]; ///< description string
    uint32_t         ttl;                                ///< TTL
} pds_if_lldp_port_status_t;

typedef struct pds_if_lldp_unknown_tlv_s {
    char     oui[PDS_LLDP_MAX_OUI_LEN];    ///< OUI string
    uint32_t subtype;                      ///< subtype
    uint32_t len;                          ///< length
    char     value[PDS_LLDP_MAX_NAME_LEN]; ///< value string
} pds_if_lldp_unknown_tlv_t;

typedef struct pds_if_lldp_unknown_tlv_status_s {
    pds_if_lldp_unknown_tlv_t tlvs[PDS_LLDP_MAX_UNK_TLVS];
    uint16_t                  num_tlvs;
} pds_if_lldp_unknown_tlv_status_t;

typedef struct pds_lldp_status_s {
    char                             if_name[PDS_LLDP_MAX_NAME_LEN]; ///< interface name
    uint32_t                         router_id;                      ///< router-id
    pds_if_lldp_mode_t               mode;                           ///< LLDP/CDP/EDP/FDP/..
    string                           age;                            ///< uptime string
    pds_if_lldp_chassis_status_t     chassis_status;                 ///< chassis info
    pds_if_lldp_port_status_t        port_status;                    ///< physical port info
    pds_if_lldp_unknown_tlv_status_t unknown_tlv_status;             ///< unknown tlv info
} pds_lldp_status_t;

typedef struct pds_if_lldp_status_s {
    pds_lldp_status_t local_if_status; ///< local interface info
    pds_lldp_status_t neighbor_status; ///< neighbor info
} pds_if_lldp_status_t;

typedef struct pds_if_lldp_spec_s {
    pds_if_lldp_chassis_spec_t lldp_chassic_spec; ///< chassis info
} pds_if_lldp_spec_t;

/// \brief uplink specific configuration
typedef struct pds_uplink_info_s {
    pds_obj_key_t   port;    ///< uplink port# this interface corresponds to
} __PACK__ pds_uplink_info_t;

/// \brief uplink port-channel specific configuration
typedef struct pds_uplink_pc_info_s {
    uint64_t    port_bmap;    ///< port bitmap
} __PACK__ pds_uplink_pc_info_t;

/// \brief L3 interface specific configuration
typedef struct pds_l3_if_info_s {
    pds_obj_key_t   vpc;          ///< vpc this L3 if belongs to
    ip_prefix_t     ip_prefix;    ///< IP address and subnet of this L3 intf
    pds_obj_key_t   port;         ///< physical port of this L3 interface
    pds_encap_t     encap;        ///< (optional) encap used for egress rewrite
    mac_addr_t      mac_addr;     ///< MAC address of this L3 interface
} __PACK__ pds_l3_if_info_t;

/// \brief loopback interface specific configuration
typedef struct pds_loopback_info_s {
    ip_prefix_t    ip_prefix;     ///< IP address and subnet host on this intf
} __PACK__ pds_loopback_info_t;

/// \brief inband control interface specific configuration
typedef struct pds_control_info_s {
    ip_prefix_t    ip_prefix;     ///< IP address and subnet host on this intf
    mac_addr_t     mac_addr;      ///< MAC address of this interface
    ip_addr_t      gateway;
} __PACK__ pds_control_info_t;

/// \brief eth port specific configuration
typedef struct pds_port_info_s {
    port_admin_state_t   admin_state;       ///< admin state of the port
    port_type_t          type;              ///< type of the port
    port_speed_t         speed;             ///< speed of the port
    port_fec_type_t      fec_type;          ///< fec type of the port
    bool                 autoneg_en;        ///< enable/disable auto negotiation
    uint32_t             debounce_timeout;  ///< link debounce time in ms
    uint32_t             mtu;               ///< MTU size in bytes
    port_pause_type_t    pause_type;        ///< pause type - link level or pfc
    bool                 tx_pause_en;       ///< enable/disable tX pause
    bool                 rx_pause_en;       ///< enable/disable rx pause
    port_loopback_mode_t loopback_mode;     ///< loopback mode - MAC/PHY
    uint32_t             num_lanes;         ///< number of lanes for the port
} __PACK__ pds_port_info_t;

/// \brief host interface specific configuration
typedef struct pds_host_if_info_s {
    ///< tx policer bound to the host interface
    pds_obj_key_t tx_policer;
    ///< tx/egress mirror session id list, if any
    pds_obj_key_t tx_mirror_sessions[PDS_MAX_MIRROR_SESSION];
    ///< rx/ingress mirror session id list, if any
    pds_obj_key_t rx_mirror_sessions[PDS_MAX_MIRROR_SESSION];
} __PACK__ pds_host_if_info_t;

/// \brief interface specification
typedef struct pds_if_spec_s {
    pds_obj_key_t            key;           ///< interface key
    if_type_t                type;          ///< type of the interface
    pds_if_state_t           admin_state;   ///< admin state of the interface
    union {
        pds_uplink_info_t    uplink_info;
        pds_uplink_pc_info_t uplink_pc_info;
        pds_l3_if_info_t     l3_if_info;
        pds_loopback_info_t  loopback_if_info;
        pds_control_info_t   control_if_info;
        pds_port_info_t      port_info;
        pds_host_if_info_t   host_if_info;
    };
    // Tx/egress mirror session id list, if any
    uint8_t num_tx_mirror_sessions;
    pds_obj_key_t tx_mirror_sessions[PDS_MAX_MIRROR_SESSION];
    // Rx/ingress mirror session id list, if any
    uint8_t num_rx_mirror_sessions;
    pds_obj_key_t rx_mirror_sessions[PDS_MAX_MIRROR_SESSION];
} __PACK__ pds_if_spec_t;

/// \brief uplink interface status
typedef struct pds_if_uplink_status_s {
    uint16_t             lif_id;
} __PACK__ pds_if_uplink_status_t;

/// \brief L3 interface status
typedef struct pds_l3_if_status_s {
} __PACK__ pds_l3_if_status_t;

typedef struct pds_if_loopback_status_s {
    ///< name of the loopback interface as seen on linux
    char name[SDK_MAX_NAME_LEN];
} __PACK__ pds_if_loopback_status_t;

/// \brief eth port interface link status
typedef struct pds_if_link_status_s {
    port_oper_status_t oper_state;     ///< operational state of the port
    port_speed_t speed;                ///< operational speed of the port
    bool autoneg_en;                   ///< autoneg enabled/disabled
    uint32_t num_lanes;                ///< number of lanes of the port
    port_fec_type_t fec_type;          ///< operational fec type of the port
} __PACK__ pds_if_link_status_t;

/// \brief eth port interface transceiver status
typedef struct pds_if_xcvr_status_s {
    uint32_t port;                          ///< transceiver port number
    xcvr_state_t state;                     ///< transceiver operational state
    xcvr_pid_t pid;                         ///< pid of the transceiver
    cable_type_t media_type;                ///< media type of the transceiver
    uint8_t xcvr_sprom[XCVR_SPROM_SIZE];    ///< transceiver sprom bytes
} pds_if_xcvr_status_t;

/// \brief eth port interface status
typedef struct pds_if_port_status_s {
    pds_if_link_status_t link_status;   ///< link status of the port
    pds_if_xcvr_status_t xcvr_status;   ///< transceiver status of the port
    port_link_sm_t  fsm_state;          ///< port link state machine
    uint32_t mac_id;                    ///< MAC ID of the port
    uint32_t mac_ch;                    ///< MAC channel of the port
    in_mem_fsm_logger *sm_logger;       ///< port state machine logger
} __PACK__ pds_if_port_status_t;

#define PDS_MAX_HOST_IF_LIFS (8)
typedef struct pds_host_if_status_s {
    ///< number of lifs
    uint8_t num_lifs;
    ///< lifs behind host interface
    pds_obj_key_t lifs[PDS_MAX_HOST_IF_LIFS];
    ///< name of host interface
    char name[SDK_MAX_NAME_LEN];
    ///< mac address of host interface
    mac_addr_t mac_addr;
} __PACK__ pds_host_if_status_t;

/// \brief interface status
typedef struct pds_if_status_s {
    if_index_t     ifindex;  ///< encoded interface index
    pds_if_state_t state;    ///< operational status of the interface
    union {
        /// uplink interface operational status
        pds_if_uplink_status_t uplink_status;
        /// L3 interface operational status
        pds_l3_if_status_t l3_if_status;
        /// loopback interface operational status
        pds_if_loopback_status_t loopback_status;
        /// eth port operational status
        pds_if_port_status_t port_status;
        /// host interface operational status
        pds_host_if_status_t host_if_status;
    };
} __PACK__ pds_if_status_t;

/// \brief eth port interface stats
typedef struct pds_if_port_stats_s {
    uint64_t stats[MAX_MAC_STATS];  ///< MAC statistics
    uint32_t num_linkdown;          ///< number of linkdown for the port
} __PACK__ pds_if_port_stats_t;

/// \brief interface statistics
typedef struct pds_if_stats_s {
    union {
        /// eth port statistics
        pds_if_port_stats_t port_stats;
    };
} __PACK__ pds_if_stats_t;

/// \brief interface information
typedef struct pds_if_info_s {
    pds_if_spec_t      spec;      ///< specification
    pds_if_status_t    status;    ///< status
    pds_if_stats_t     stats;     ///< statistics
} __PACK__ pds_if_info_t;

typedef void (*if_read_cb_t)(void *info, void *ctxt);

/// \brief     create interface
/// \param[in] spec specification
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_if_create(pds_if_spec_t *spec,
                        pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief      read interface
/// \param[in]  key  key
/// \param[out] info information
/// \return     #SDK_RET_OK on success, failure status code on error
/// \remark     valid interface key should be passed
sdk_ret_t pds_if_read(pds_obj_key_t *key, pds_if_info_t *info);

/// \brief      read interface
/// \param[in]  cb   callback
/// \param[out] ctxt context for callback
/// \return     #SDK_RET_OK on success, failure status code on error
/// \remark     valid interface key should be passed
sdk_ret_t pds_if_read_all(if_read_cb_t cb, void *ctxt);

/// \brief     update interface
/// \param[in] spec specification
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
/// \remark    A valid interface specification should be passed
sdk_ret_t pds_if_update(pds_if_spec_t *spec,
                        pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief     delete interface
/// \param[in] key key
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
/// \remark    A valid interface key should be passed
sdk_ret_t pds_if_delete(pds_obj_key_t *key,
                        pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// @}

#endif    // __INCLUDE_API_PDS_INTF_HPP__
