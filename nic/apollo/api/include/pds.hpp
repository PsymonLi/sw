//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Base enums and macros for PDS APIs
///
//----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_HPP__
#define __INCLUDE_API_PDS_HPP__

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/lib/ht/ht.hpp"

/// \defgroup PDS_BASE Base enums and macros
/// @{

#define PDS_EPOCH_INVALID             0x0    ///< invalid epoch
#define PDS_BATCH_CTXT_INVALID        0x0    ///< invalid batch context
#define PDS_PORT_INVALID              0xFF   ///< invalid port#

#define PDS_MAX_HOST_NAME_LEN         128
#define PDS_MAX_DOMAIN_NAME_LEN       128

// 16B keys for the objects
#define PDS_MAX_KEY_LEN               16
#define PDSM_MAX_KEY_STR_LEN          36

// forward declaration for generic API object key
typedef struct pds_obj_key_s pds_obj_key_t;

// invalid object key (all 0s)
extern const pds_obj_key_t k_pds_obj_key_invalid;

// generic API object key
struct pds_obj_key_s {
    char id[PDS_MAX_KEY_LEN + 1];

    void reset(void) {
        memset(id, 0, sizeof(id));
    }
    char *str(void) const {
        char *buf;
        static thread_local uint8_t next_str = 0;
        static thread_local char key_str[4][PDSM_MAX_KEY_STR_LEN + 1];

        buf = key_str[next_str++ & 0x3];
        sprintf(buf, "%08x", htonl((*(uint32_t *)&id[0]) & 0xFFFFFFFF));
        buf[8] = '-';
        sprintf(&buf[9], "%04x", htons((*(uint16_t *)&id[4]) & 0xFFFF));
        buf[13] = '-';
        sprintf(&buf[14], "%04x", htons((*(uint16_t *)&id[6]) & 0xFFFF));
        buf[18] = '-';
        sprintf(&buf[19], "%04x", htons((*(uint16_t *)&id[8]) & 0xFFFF));
        buf[23] = '-';
        for (uint32_t i = 0; i < 6; i++) {
            sprintf(&buf[24 + (i << 1)], "%02x", id[10 + i] & 0xFF);
        }
        buf[PDSM_MAX_KEY_STR_LEN] = '\0';
        return buf;
    }
    bool operator==(const pds_obj_key_t& other) const {
        if (!memcmp(id, other.id, PDS_MAX_KEY_LEN)) {
            return true;
        }
        return false;
    }
    bool operator!=(const pds_obj_key_t& other) const {
        if (memcmp(id, other.id, PDS_MAX_KEY_LEN)) {
            return true;
        }
        return false;
    }
    bool valid(void) const {
        return this->operator!=(k_pds_obj_key_invalid);
    }
} __PACK__;

// helper class for hash computation of the object key
class pds_obj_key_hash {
public:
    std::size_t operator()(const pds_obj_key_t& key) const {
        return hash_algo::fnv_hash((void *)&key, sizeof(key));
    }
};

// basic PDS data types
typedef uint64_t    pds_batch_ctxt_t;          ///< opaque batch context
typedef uint32_t    pds_rule_id_t;             ///< rule identifier
typedef uint32_t    pds_rsc_pool_id_t;         ///< resource pool id
typedef uint32_t    pds_epoch_t;               ///< epoch id
typedef uint32_t    pds_slot_id_t;             ///< MPLS tag value
typedef uint16_t    pds_vlan_id_t;             ///< VLAN tag value
typedef uint32_t    pds_vnid_id_t;             ///< VxLAN id
typedef uint32_t    pds_mpls_tag_t;            ///< MPLS tag value
typedef uint16_t    pds_lif_id_t;              ///< (internal) lif identifier

/// \brief encapsulation type
typedef enum pds_encap_type_e {
    PDS_ENCAP_TYPE_NONE     = 0,    ///< No encap
    PDS_ENCAP_TYPE_DOT1Q    = 1,    ///< dot1q encap
    PDS_ENCAP_TYPE_QINQ     = 2,    ///< QinQ encap
    PDS_ENCAP_TYPE_MPLSoUDP = 3,    ///< MPLSoUDP
    PDS_ENCAP_TYPE_VXLAN    = 4,    ///< VxLAN encap
} pds_encap_type_t;

/// \brief QinQ tag values
typedef struct pds_qinq_tag_s {
    uint16_t c_tag;    ///< customer VLAN tag
    uint16_t s_tag;    ///< service VLAN tag
} pds_qinq_tag_t;

/// \brief encapsulation values
typedef union pds_encap_val_u {
    uint16_t       vlan_tag;    ///< 12 bit .1q tag
    pds_qinq_tag_t qinq_tag;    ///< QinQ tag
    pds_vnid_id_t  vnid;        ///< 24 bit VxLAN vnid
    pds_mpls_tag_t mpls_tag;    ///< 20-bit MPLS tag/slot
    uint32_t       value;       ///< generic value to refer to other values
} pds_encap_val_t;

/// \brief encapsulation config
typedef struct pds_encap_s {
    pds_encap_type_t type;    ///< encap type
    pds_encap_val_t  val;     ///< encap value
} pds_encap_t;

static inline char *
pds_encap2str (const pds_encap_t *encap)
{
    static thread_local char       encap_str[4][20];
    static thread_local uint8_t    encap_str_next = 0;
    char                           *buf;

    buf = encap_str[encap_str_next++ & 0x3];
    switch (encap->type) {
    case PDS_ENCAP_TYPE_DOT1Q:
        snprintf(buf, 20, "Dot1q/%u", encap->val.value);
        break;
    case PDS_ENCAP_TYPE_QINQ:
        snprintf(buf, 20, "QinQ/%u", encap->val.value);
        break;
    case PDS_ENCAP_TYPE_MPLSoUDP:
        snprintf(buf, 20, "MPLSoUDP/%u", encap->val.value);
        break;
    case PDS_ENCAP_TYPE_VXLAN:
        snprintf(buf, 20, "VxLAN/%u", encap->val.value);
        break;
    default:
        snprintf(buf, 20, "-");
        break;
    }
    return buf;
}

/// \brief NAT type
typedef enum pds_nat_type_e {
    PDS_NAT_TYPE_NONE        = 0,
    /// static 1:1 NAT
    PDS_NAT_TYPE_STATIC      = 1,
    /// (dynamic) NAPT to public (i.e., Internet) address space
    PDS_NAT_TYPE_NAPT_PUBLIC = 2,
    /// (dynamic) NAPT to internal service address space
    PDS_NAT_TYPE_NAPT_SVC    = 3,
} pds_nat_type_t;

/// \brief NAT action
typedef struct pds_nat_action_s {
    pds_nat_type_t src_nat_type;
    ip_addr_t      dst_nat_ip;
} pds_nat_action_t;

/// \brief    mapping type
typedef enum pds_mapping_type_e {
    PDS_MAPPING_TYPE_NONE = 0,
    PDS_MAPPING_TYPE_L2   = 1,
    PDS_MAPPING_TYPE_L3   = 2,
} pds_mapping_type_t;

/// \brief    2nd-ary/internal key of mapping
typedef struct pds_mapping_key_s {
    pds_mapping_type_t type;
    union {
        ///< L3 key
        struct {
            pds_obj_key_t vpc;    ///< VPC this IP belongs to
            ip_addr_t ip_addr;    ///< IP address of the mapping
        };
        ///< L2 key
        struct {
            pds_obj_key_t subnet; ///< subnet of the mapping
            mac_addr_t mac_addr;  ///< MAC address of the mapping
        };
    };
} __PACK__ pds_mapping_key_t;

/// \brief    service mapping key
typedef struct pds_svc_mapping_key_s {
    pds_obj_key_t vpc;       ///< VPC of the backend
    ip_addr_t backend_ip;    ///< IP address of the backend
    uint16_t backend_port;   ///< backend's L4 port

    // operator== is required to compare keys in case of hash collision
    /// compare operator
    bool operator==(const struct pds_svc_mapping_key_s &p) const {
        return (vpc.id == p.vpc.id) && (backend_port == p.backend_port) &&
            (!memcmp(&backend_ip, &p.backend_ip, sizeof(ip_addr_t)));
    }
} __PACK__ pds_svc_mapping_key_t;

/// \brief    service mapping hash function
class pds_svc_mapping_hash_fn {
public:
    /// function call operator
    std::size_t operator()(const pds_svc_mapping_key_t &key) const {
        return hash_algo::fnv_hash((void *)&key, sizeof(key));
    }
};

/// \brief address type (public/Internet or service)
typedef enum address_type_s {
    ADDR_TYPE_NONE    = 0,
    ADDR_TYPE_PUBLIC  = 1,    ///< public/Internet address space
    ADDR_TYPE_SERVICE = 2,    ///< service/infra address space
} address_type_t;

/// \brief firewall action transposition scheme
/// NOTE:
/// for every new session from/to a vnic, there are potentially multiple
/// policies that need to be evaluated. For example, upto 2 ingress/egress
/// policies attached to vnic's subnet must be evaluated and upto 5
/// ingress/egress policies directly attached to vnic must evaluated. Each
/// policy may give potentially different and conflicting result w.r.t the
/// action that needs to be taken (allow or deny). So, user needs to be able to
/// specify how to resolve this and arrive at final action to pick for that
/// session.
/// fw_policy_xposn_t defines how such action transposition needs to be
/// performed
typedef enum fw_policy_xposn_e {
    FW_POLICY_XPOSN_NONE            = 0,
    /// if GLOBAL_PRIORITY transposition scheme is configured, rule that is
    /// picked is the rule with highest priority (numerically lowest value)
    /// across all policies that are evaluated for a given packet
    FW_POLICY_XPOSN_GLOBAL_PRIORITY = 1,
    // if ANY_DENY transposition scheme is configured, then if a given packet
    // hits a deny rule in any of the policies evaluated for that packet, then
    // session is created with deny/drop action
    FW_POLICY_XPOSN_ANY_DENY        = 2,
} fw_policy_xposn_t;

/// \brief module ids
typedef enum pds_mod_id_e {
    PDS_MOD_ID_PDS = sdk_mod_id_t::SDK_MOD_ID_MAX + 1,
} pds_mod_id_t;

/// @}

#endif    // __INCLUDE_API_PDS_HPP__
