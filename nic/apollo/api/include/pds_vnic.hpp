//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines VNIC API
///
//----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_VNIC_HPP__
#define __INCLUDE_API_PDS_VNIC_HPP__

#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_subnet.hpp"

/// \defgroup PDS_VNIC vnic API
/// @{

#define PDS_MAX_VNIC              128     ///< maximum vnics
#define PDS_MAX_VNIC_POLICY       5       ///< max. #of security policies per vnic
#define PDS_MAX_MIRROR_SESSION    8

/// \brief vnic specification
typedef struct pds_vnic_spec_s {
    pds_obj_key_t key;                         ///< vnic's key
    char hostname[PDS_MAX_HOST_NAME_LEN + 1];  ///< hostname of this workload,
                                               ///< if any (expected to be empty
                                               ///< for 2nd-ary vnics)
    pds_obj_key_t subnet;                      ///< subnet of this vnic
    pds_encap_t vnic_encap;                    ///< vnic encap for this vnic
    pds_encap_t fabric_encap;                  ///< fabric encap for this vnic
    mac_addr_t mac_addr;                       ///< vnic's overlay mac address
    bool binding_checks_en;                    ///< TRUE if MAC/IP binding
                                               ///< checks are enabled
    /// Tx mirror sessions, if any
    uint8_t num_tx_mirror_session;
    pds_obj_key_t tx_mirror_session[PDS_MAX_MIRROR_SESSION];
    /// Rx mirror sessions, if any
    uint8_t num_rx_mirror_session;
    pds_obj_key_t rx_mirror_session[PDS_MAX_MIRROR_SESSION];
    /// IPv4 Metering policy to apply on Tx and Rx traffic
    pds_obj_key_t v4_meter;
    /// IPv6 Metering policy to apply on Tx and Rx traffic
    pds_obj_key_t v6_meter;
    /// TRUE if vnic is a switch/bridge vnic
    bool switch_vnic;
    /// ingress IPv4 policy table(s)
    uint8_t num_ing_v4_policy;
    pds_obj_key_t ing_v4_policy[PDS_MAX_VNIC_POLICY];
    /// ingress IPv6 policy table(s)
    uint8_t num_ing_v6_policy;
    pds_obj_key_t ing_v6_policy[PDS_MAX_VNIC_POLICY];
    /// egress IPv4 policy table(s)
    uint8_t num_egr_v4_policy;
    pds_obj_key_t egr_v4_policy[PDS_MAX_VNIC_POLICY];
    /// egress IPv6 policy table(s)
    uint8_t num_egr_v6_policy;
    pds_obj_key_t egr_v6_policy[PDS_MAX_VNIC_POLICY];
    /// when operating in PDS_DEV_OPER_MODE_HOST mode with multiple host
    /// PFs/VFs present, datapath should be told which VF/PF (aka. lif or
    /// host interface) a particular vnic is attached to by passing
    /// host_ifindex (of type IF_TYPE_LIF)
    pds_obj_key_t host_if;
    /// Tx policer, if any
    pds_obj_key_t tx_policer;
    /// Rx policer, if any
    pds_obj_key_t rx_policer;
    /// only primary vnic's will get hostname and gateway IP in the
    /// DHCP offer from local DHCP proxy
    bool primary;
    /// max_sessions is the maximum number of sessions from/to this vnic, if set
    /// session limit is not enforced, if this value is set to 0
    uint32_t max_sessions;
    /// enable or disable flow learning on per vnic basis
    /// when flow learning is enabled, flow/session logs are generated when
    /// session is cretaed upon processing of the 1st packet and eventually
    /// exported to agent/controller/collector
    bool flow_learn_en;
    /// enable or disable metering on this vnic
    /// if metering is true, when traffic from a vnic hits a route with
    /// metering enabled, such traffic is accounted (bytes & packets)
    /// against the vnic
    bool meter_en;
    /// public_mac_addr, if set, is used to translate the vnic's MAC address;
    /// in the Tx direction, when vnic sends traffic with its overlay/private
    /// MAC (mac_addr attribute in this object) as source MAC, it gets NAT-ed to
    /// public_mac_addr before the packet is put on the wire (or delivered
    /// locally to another vnic on same DSC) and in the Rx direction when
    /// traffic is received with destination as public_mac_addr, it gets
    /// translated to vnic's private MAC address before the packet is delivered
    /// to the vnic
    mac_addr_t public_mac_addr;
    /// when movable attribute is set to true, this vnic is moveable from one
    /// host to another (e.g., as part of vmotion event)
    bool movable;
} __PACK__ pds_vnic_spec_t;

/// \brief vnic status
typedef struct pds_vnic_status_s {
    uint16_t hw_id;            ///< hardware id
    uint16_t nh_hw_id;         ///< nexthop id
    uint16_t host_if_hw_id;    ///< host if id
    uint32_t epoch;            ///< current config epoch
} pds_vnic_status_t;

/// \brief vnic statistics
typedef struct pds_vnic_stats_s {
    uint64_t rx_pkts;         ///< received packet count
    uint64_t rx_bytes;        ///< received bytes
    uint64_t tx_pkts;         ///< transmit packet count
    uint64_t tx_bytes;        ///< transmit bytes
    uint64_t active_sessions; ///< current active sessions
    uint64_t meter_rx_pkts;   ///< meter received packet count
    uint64_t meter_rx_bytes;  ///< meter received bytes
    uint64_t meter_tx_pkts;   ///< meter transmit packet count
    uint64_t meter_tx_bytes;  ///< meter transmit bytes
} pds_vnic_stats_t;

/// \brief vnic information
typedef struct pds_vnic_info_s {
    pds_vnic_spec_t spec;        ///< vnic specification
    pds_vnic_status_t status;    ///< vnic status
    pds_vnic_stats_t stats;      ///< vnic stats
} pds_vnic_info_t;

/// \brief create vnic
/// \param[in] spec Specification
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vnic_create(pds_vnic_spec_t *spec,
                          pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief read vnic information
/// \param[in] key Key
/// \param[out] info Information
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vnic_read(pds_obj_key_t *key, pds_vnic_info_t *info);

typedef void (*vnic_read_cb_t)(pds_vnic_info_t *info, void *ctxt);

/// \brief read all vnic information
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vnic_read_all(vnic_read_cb_t vnic_read_cb, void *ctxt);

/// \brief Update vnic specification
/// \param[in] spec Specififcation
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vnic_update(pds_vnic_spec_t *spec,
                          pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief delete vnic
/// \param[in] key Key
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vnic_delete(pds_obj_key_t *key,
                          pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief reset the statistics of a vnic
/// \param[in] key vnic's key or NULL if this needs to be applied to all vnics
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_vnic_stats_reset(pds_obj_key_t *key);

/// @}

#endif    ///  __INCLUDE_API_PDS_VNIC_HPP__
