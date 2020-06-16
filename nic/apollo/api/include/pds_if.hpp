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

#define PDS_INTF_ID_INVALID 0     ///< invalid interface id

/// \brief interface admin/operational state
typedef enum pds_if_state_e {
    PDS_IF_STATE_NONE = 0,
    PDS_IF_STATE_DOWN = 1,
    PDS_IF_STATE_UP   = 2,
} pds_if_state_t;

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
    uint16_t lif_id;
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
