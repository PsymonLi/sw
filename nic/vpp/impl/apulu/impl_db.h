//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_APULU_DB_H__
#define __VPP_IMPL_APULU_DB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <vnet/ip/ip.h>
#include <nic/vpp/infra/utils.h>
#include <nic/apollo/p4/include/apulu_table_sizes.h>
#include <api.h>
#include "pdsa_impl_db_hdlr.h"

#define PDS_VPP_MAX_SUBNET  BD_TABLE_SIZE
#define PDS_VPP_MAX_VNIC    VNIC_INFO_TABLE_SIZE
#define PDS_VPP_MAX_VPC     VPC_TABLE_SIZE
#define PDS_MAX_KEY_LEN     16 // mirrors defn in nic/apollo/api/include/pds.hpp
#define VNIC_SESSION_LIMIT_MIN_THRESHOLD (0.8) // reset approach on 80%
#define VNIC_SESSION_LIMIT_MAX_THRESHOLD (0.9) // reset exhaust / raise approach event at 90%

typedef enum {
    PDS_ETH_ENCAP_NO_VLAN,
    PDS_ETH_ENCAP_DOT1Q,    // single tag
    PDS_ETH_ENCAP_DOT1AD,   // double tag
} pds_eth_encap_type;

typedef enum {
    PDS_DEV_MODE_NONE                = 0,
    // flow based host PCIe(tagged/untagged) <----> uplinks(encapped).
    PDS_DEV_MODE_HOST                = 1,
    // flow based uplink0/1(tagged/untagged) <----> uplink1/0(encapped).
    PDS_DEV_MODE_BITW_SMART_SWITCH   = 2,
    // flow based IP services over uplink0/1
    PDS_DEV_MODE_BITW_SMART_SERVICE  = 3,
    // non-flow based forwarding
    PDS_DEV_MODE_BITW_CLASSIC_SWITCH = 4,
} pds_device_mode;

typedef struct {
    u8 obj_key[PDS_MAX_KEY_LEN];         // UUID of the vnic
    mac_addr_t mac;                      // vnic mac
    u32 max_sessions;                    // max sessions from this vnic
    u32 ses_alert_min_threshold;         // threshold to raise alert
    u32 ses_alert_max_threshold;         // threshold to clear condition
    volatile u32 active_ses_count;       // currently active sessions on vnic
    u16 subnet_hw_id;                    // subnet hwid to index subnet store in infra
    u8 flow_log_en : 1;                  // flag indicating flow logging enabled
    u8 ses_alert_limit_exceeded: 1;      // flag to raise session limit alert
    u8 ses_alert_threshold_exceeded : 1; // flag to raise session threshold alert
    u8 reserve : 5;
    u8 encap_type;                  // pds_eth_encap_type
    u8 l2_encap_len;                // layer2 encapsulation length
    u16 vlan_id;                    // vlan id if encap type is != no vlan
    u16 nh_hw_id;                   // nexthop id
    u16 host_lif_hw_id;             // host lif id
    u8 *rewrite;
} pds_impl_db_vnic_entry_t;

typedef struct {
    mac_addr_t mac;                 // subnet mac
    u8 prefix_len;                  // subnet prefix len
    ip46_address_t vr_ip;           // subnet VR ip
    u32 vnid;
    u16 vpc_id;                     // VPC id to which the subnet belongs
} pds_impl_db_subnet_entry_t;

typedef struct {
    u16 hw_bd_id;                   // vpc's bd id
    u16 flags;                      // vpc flags
} pds_impl_db_vpc_entry_t;

typedef struct {
    mac_addr_t device_mac;      // device MAC address
    ip46_address_t device_ip;   // device IP address
    u8 overlay_routing_en : 1;  // overlay Routing enabled or not
    u8 symmetric_routing_en : 1;// symmetric routing - VNI for i/rflow is same
    u16 mapping_prio;           // mapping priority, used for mappping vs route
    pds_device_mode oper_mode;  // device operational mode - host or bitw
} pds_impl_db_device_entry_t;

#define foreach_impl_db_element                         \
 _(uint16_t, subnet)                                    \
 _(uint16_t, vnic)                                      \
 _(uint16_t, vpc)                                       \

typedef struct {
#define _(type, obj)                                    \
    u16 * obj##_pool_idx;                               \
    pds_impl_db_##obj##_entry_t * obj##_pool_base;

    foreach_impl_db_element
#undef _
    pds_impl_db_device_entry_t device;
} pds_impl_db_ctx_t;

extern pds_impl_db_ctx_t impl_db_ctx;

#define _(type, obj)                                                \
pds_impl_db_##obj##_entry_t * pds_impl_db_##obj##_get(type hw_id);
    foreach_impl_db_element
#undef _

pds_impl_db_device_entry_t * pds_impl_db_device_get(void);
int pds_impl_db_vr_ip_mac_get (uint16_t subnet, uint32_t *vr_ip, uint8_t **vr_mac);
bool pds_impl_db_vr_ip_get (uint16_t subnet, uint32_t *vr_ip);

typedef void pds_foreach_vnic_cb(pds_impl_db_vnic_entry_t *);
void  pds_impl_db_foreach_vnic(pds_foreach_vnic_cb *cb);

#ifdef __cplusplus
}
#endif

#endif    // __VPP_IMPL_APULU_DB_H__
