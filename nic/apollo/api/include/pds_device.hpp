//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
///----------------------------------------------------------------------------
///
/// \file
/// This module defines Device API
///
///----------------------------------------------------------------------------

#ifndef __INCLUDE_API_DEVICE_HPP__
#define __INCLUDE_API_DEVICE_HPP__

#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/platform.hpp"
#include "nic/apollo/api/include/pds.hpp"

#define PDS_DROP_REASON_MAX        64    ///< Maximum packet drop reasons
#define PDS_MAX_DROP_NAME_LEN      32    ///< Packet drop reason string length
#define PDS_MAX_DEVICE              1    ///< only one instance of device

using namespace std;

/// \defgroup PDS_DEVICE Device API
/// \@{

/// operational mode of the device
/// NOTE:
/// multiple connectivity models are possible in SMART_SWITCH and SMART_SERVICE
/// modes:
///     a. one uplink handles traffic from/to host side and other uplink handles
///        traffic from/to fabric/sdn
///     b. both uplinks can send and receive traffic from/to host and from/to
///        fabric side (i.e., both links are treated the same way and probably
///        ECMP-ed), this connectivity model is useful when DSC is deployed
///        behind/alongside a switch and switch is redirecting traffic to DSCs
///        (aka. sidecar model) and making forwarding decisions
typedef enum pds_device_oper_mode_e {
    PDS_DEV_OPER_MODE_NONE                = 0,
    /// HOST mode with workloads on pcie; it is flow based and supports all
    /// features (IP services like firewall, NAT etc.) depending on the memory
    /// profile configured
    PDS_DEV_OPER_MODE_HOST                = 1,
    /// in SMART_SWITCH mode, DSC acts as bump-in-the-wire device; it is flow
    /// based and does forwarding (based on mappings and routes) while
    /// supporting all IP (smart) service features (and is flow based) depending
    /// on the memory profile configured; the switch connected via the sdn
    /// port(s) will do only underlay routing on the encapped (by DSC) traiffc
    PDS_DEV_OPER_MODE_BITW_SMART_SWITCH   = 2,
    /// in SMART_SERVICE mode, DSC is flow based and provides IP service
    /// features; it doesn't do forwarding (i.e. L2 or L3 lookups), i.e. no
    /// L2/L3 mappings need to be configured and IP routing is not enabled
    PDS_DEV_OPER_MODE_BITW_SMART_SERVICE  = 3,
    /// in CLASSIC_SWITCH mode, DSC performs routing and no IP services are
    /// performed; additionally this is not flow based mode and hence every
    /// packet is subjected to route table and/or mapping lookups (routes are
    /// either programmed via grpc or distributed via control protocol like BGP)
    PDS_DEV_OPER_MODE_BITW_CLASSIC_SWITCH = 4,
} pds_device_oper_mode_t;

typedef enum pds_device_profile_e {
    PDS_DEVICE_PROFILE_DEFAULT            = 0,
    PDS_DEVICE_PROFILE_2PF                = 1,
    PDS_DEVICE_PROFILE_3PF                = 2,
    PDS_DEVICE_PROFILE_4PF                = 3,
    PDS_DEVICE_PROFILE_5PF                = 4,
    PDS_DEVICE_PROFILE_6PF                = 5,
    PDS_DEVICE_PROFILE_7PF                = 6,
    PDS_DEVICE_PROFILE_8PF                = 7,
    PDS_DEVICE_PROFILE_16PF               = 8,
    PDS_DEVICE_PROFILE_32VF               = 9,
    PDS_DEVICE_PROFILE_BITW_SMART_SERVICE = 10,
} pds_device_profile_t;

typedef enum pds_memory_profile_e {
    PDS_MEMORY_PROFILE_DEFAULT = 0,
    /// router profile will support 256K routes per IPv4
    /// route table and a total of 32 such route tables
    PDS_MEMORY_PROFILE_ROUTER  = 1,
    /// use IPSEC memory profile for IPSec feature
    PDS_MEMORY_PROFILE_IPSEC   = 2,
} pds_memory_profile_t;

/// support learn modes
typedef enum pds_learn_mode_e {
  ///  when learn mode is set to PDS_LEARN_MODE_NONE, learning is completely
  /// disabled
  PDS_LEARN_MODE_NONE   = 0,
  /// in LEARN_MODE_NOTIFY mode, when a unknown MAC/IP is seen, notification
  /// is generated via operd to the app, learn module will not populate the
  /// p4 tables with the MAC or IP; they will be programmed when app comes back
  /// and install vnics and/or IP mappings because of these learn notifications
  /// NOTE:
  /// 1. as learn events are simply notified to app, learn module doesn't need
  ///    to perform aging of the MAC/IP entries in this mode.
  /// 2. in order to de-dup back-to-back learn events and not bombard the app
  ///    listening  to these notifications, some state will be maintained about
  ///    the notified MAC/IP entries and will be deleted within short time
  PDS_LEARN_MODE_NOTIFY = 1,
  /// in LEARN_MODE_AUTO, learn module will learn and automatically program the
  /// learnt MAC/IP in the datapath. Additionally, notifications will be
  /// generated for the clients of interest via operd
  PDS_LEARN_MODE_AUTO   = 2,
} pds_learn_mode_t;

/// supported types of packets for learning
typedef struct pds_learn_source_s {
    /// enable/disable learning from ARP/RARP/GARP
    bool arp_learn_en;
    /// enable/disable learning from dhcp traffic
    bool dhcp_learn_en;
    /// enable/disable learning from data packets
    bool data_pkt_learn_en;
} pds_learn_source_t;

/// MAC/IP learning related configuration knobs
typedef struct pds_learn_spec_s {
    /// MAC/IP learning mode
    pds_learn_mode_t   learn_mode;
    /// MAC, IP aging timeout (in seconds) for learnt entries
    /// NOTE: if learning is enabled and this parameter is 0, default
    ///       age timeout of 5 mins is assumed
    uint32_t           learn_age_timeout;
    /// possible sources for MAC/IP learning
    pds_learn_source_t learn_source;
} pds_learn_spec_t;

/// \brief device specification
typedef struct pds_device_s {
    /// device IP address
    ip_addr_t              device_ip_addr;
    /// device MAC address
    mac_addr_t             device_mac_addr;
    /// gateway IP address
    ip_addr_t              gateway_ip_addr;
    /// enable or disable forwarding based on L2 entries
    bool                   bridging_en;
    /// MAC/IP learning controls
    pds_learn_spec_t       learn_spec;
    /// enable or disable evpn for overlay routing/bridging
    /// NOTE: when overlay_routing_en is modified, it will take affect only
    ///        after reboot of NAPLES/DSC
    bool                   overlay_routing_en;
    /// when symmetric_routing_en is set to true, its called symmetric routing
    /// and symmetric_routing_en is set to false, it is called asymmetric
    //  routing
    /// Below is the datapath behavior in various cases:
    /// 1. mapping is hit (or mapping hit result is pickedi over route hit),
    ///    intra-subnet traffic, symmetric_routing_en = true or false:
    ///    a. encapped packets are sent out with destination subnet's vnid
    ///    b. incoming encapped packets are expected with destination subnet's
    ///       vnid
    /// 2. mapping is hit (or mapping hit result is pickedi over route hit),
    ///    inter-subnet traffic, symmetric_routing_en = true:
    ///    a. encapped packets are sent out with destination vpc's vnid
    ///    b. incoming encapped packets are expected with destination vpc's vnid
    /// 3. mapping is hit (or mapping hit result is pickedi over route hit),
    ///    inter-subnet traffic, symmetric_routing_en = false:
    ///    a. encapped packets are sent out with destination subnet's vnid
    ///    b. incoming encapped packets are expected with destination subnet's
    ///        vnid
    /// 4. route is hit (route hit result is picked over mapping),
    ///    symmetric_routing_en = false:
    ///    a. encapped packets are sent out with vpc's vnid
    ///    b. incoming encapped packets are expected with destination subnet's
    ///       vnid
    /// 5. route is hit (route hit result is picked over mapping),
    ///    symmetric_routing_en = true:
    ///    a. encapped packets are sent out with destination vpc's vnid
    ///    b. incoming encapped packets are expected with destination vpc's vnid
    /// NOTE:
    /// 1. by default symmetric_routing_en is false i.e., DSC is in asymmetric
    ///    routing mode
    /// 2. if the value of this attribute is updated on the fly, it will not
    ///    affect the flows/sessions that are already created, but it will take
    ///    affect only on the new sessions/flows created after such an update
    bool                   symmetric_routing_en;
    /// device profile
    pds_device_profile_t   device_profile;
    /// memory profile
    pds_memory_profile_t   memory_profile;
    /// device operational mode
    pds_device_oper_mode_t dev_oper_mode;
    /// priority class of IP mapping entries, is used to break the tie in case
    /// both LPM/prefix and a mapping entry are hit in the datapath (i.e., /32
    /// IP mapping entry is also in some LPM prefix) for the same packet.
    /// NOTE:
    /// 1. by default IP mapping always takes precedence over LPM hit as the
    ///    default value of this attribute is 0 (and lower the numerical value,
    ///    higher the priority, hence 0 is the highest priority)
    /// 2. valid priority value range is 0 to 31
    /// 3. if mapping and route are both hit and both have same class priority,
    ///    mapping result will take precedence over route (even if it is /32
    ///    route)
    /// 4. if the value of this attribute is updated on the fly, it will not
    ///    affect the flows/sessions that are already created, but it will take
    ///    affect only on the new sessions/flows created after such an update
    uint16_t               ip_mapping_class_priority;
    /// firewall action transposition scheme
    fw_policy_xposn_t      fw_action_xposn_scheme;
    /// Tx policer, if any
    pds_obj_key_t          tx_policer;
    /// systemname, if configured, will be used as system name in protocols
    /// like LLDP etc
    char                   sysname[PDS_MAX_SYSTEM_NAME_LEN+1];
} __PACK__ pds_device_spec_t;

/// \brief device status
typedef struct pds_device_status_s {
    mac_addr_t  fru_mac;           ///< FRU MAC
    uint8_t     memory_cap;        ///< Memory capacity
    string      mnfg_date;         ///< FRU Manufacturing date
    string      product_name;      ///< FRU Product name
    string      serial_num;        ///< FRU Serial Number
    string      part_num;          ///< FRU Part Number
    string      description;       ///< Description
    string      vendor_id;         ///< Vendor ID
    sdk::platform::asic_type_t chip_type; ///< Chip Type
    string      hardware_revision; ///< Hardware Revision
    string      cpu_vendor;        ///< CPU Vendor
    string      cpu_specification; ///< CPU Specification
    string      fw_version;        ///< Firmware Version
    string      soc_os_version;    ///< SOC OS Version
    string      soc_disk_size;     ///< SOC Disk Size
    string      pcie_specification;///< PCIe Specification
    string      pcie_bus_info;     ///< PCIe Bus Information
    uint32_t    num_pcie_ports;    ///< Number of PCIe ports
    uint32_t    num_ports;         ///< Number of NIC ports
    string      vendor_name;       ///< Vendor Name
    float       pxe_version;       ///< PXE Version
    float       uefi_version;      ///< UEFI Version
    uint32_t    num_host_if;       ///< number of host interfaces
    string      fw_description;    ///< Firmware Description
    string      fw_build_time;     ///< Firmware Build Time
} pds_device_status_t;

/// \brief Drop statistics
typedef struct pds_device_drop_stats_s {
    char     name[PDS_MAX_DROP_NAME_LEN];   ///< drop reason name
    uint64_t count;                         ///< drop count
} __PACK__ pds_device_drop_stats_t;

/// \brief device statistics
typedef struct pds_device_stats_s {
    ///< number of entries in the ingress drop statistics
    uint32_t ing_drop_stats_count;
    ///< number of entries in the egress drop statistics
    uint32_t egr_drop_stats_count;
    ///<< ingress drop statistics
    pds_device_drop_stats_t ing_drop_stats[PDS_DROP_REASON_MAX];
    ///< egress drop statistics
    pds_device_drop_stats_t egr_drop_stats[PDS_DROP_REASON_MAX];
} __PACK__ pds_device_stats_t;

/// \brief device information
typedef struct pds_device_info_s {
    pds_device_spec_t   spec;      ///< specification
    pds_device_status_t status;    ///< status
    pds_device_stats_t  stats;     ///< statistics
} pds_device_info_t;

/// \brief     create device
/// \param[in] spec specification
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
/// \remark    device is a global configuration and can be created only once.
///            Any other validation that is expected on the TEP should be done
///            by the caller
sdk_ret_t pds_device_create(pds_device_spec_t *spec,
                            pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief      read device
/// \param[out] info device information
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_device_read(pds_device_info_t *info);

/// \brief      reset device statistics
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_device_stats_reset(void);

/// \brief     update device
/// \param[in] spec specification
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
/// \remark    A valid device specification should be passed
sdk_ret_t pds_device_update(pds_device_spec_t *spec,
                            pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief  delete device
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_device_delete(pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// @}

#endif    // __INCLUDE_API_PDS_DEVICE_HPP__
