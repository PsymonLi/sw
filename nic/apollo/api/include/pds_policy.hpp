//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines policy API
///
//----------------------------------------------------------------------------

#ifndef __INCLUDE_API_PDS_POLICY_HPP__
#define __INCLUDE_API_PDS_POLICY_HPP__

#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/sdk/include/sdk/l4.hpp"
#include "nic/sdk/include/sdk/mem.hpp"
#include "nic/apollo/api/include/pds.hpp"

/// \defgroup PDS_POLICY Policy API
/// @{

#define PDS_MAX_SECURITY_POLICY                  1023    ///< Maximum security policies
#define PDS_MAX_RULES_PER_IPV4_SECURITY_POLICY   1023    ///< Maximum IPV4 rules per policy
#define PDS_MAX_RULES_PER_IPV6_SECURITY_POLICY     15    ///< Maximum IPV6 rules per policy
#define PDS_MAX_RULE_PRIORITY                    1022    ///< Numerically highest priority

/// \brief    rule direction
typedef enum rule_dir_e {
    /// INGRESS is w.r.t vnic, policy is applied on traffic sent towards vnic
    RULE_DIR_INGRESS = 0,
    /// EGRESS is w.r.t vnic, policy is applied on traffic sent from vnic
    RULE_DIR_EGRESS  = 1,
} rule_dir_t;

/// \brief type of the IP field used in rule match
typedef enum ip_match_type_s {
    IP_MATCH_NONE   = 0,    ///< match none
    IP_MATCH_PREFIX = 1,    ///< match based on the prefix
    IP_MATCH_RANGE  = 2,    ///< match based on the range
    IP_MATCH_TAG    = 3,    ///< match based on the tag
} ip_match_type_t;

typedef enum match_type_s {
    MATCH_ANY      = 0, ///< match any
    MATCH_SPECIFIC = 1, ///< match specific
} match_type_t;

/// \brief    rule L3 match criteria
typedef struct rule_l3_match_s {
    match_type_t     proto_match_type;///< Protocol match type
    uint8_t          ip_proto;        ///< IP protocol
    ip_match_type_t  src_match_type;  ///< src match condition type
    ip_match_type_t  dst_match_type;  ///< dst match condition type
    union {
        ip_prefix_t  src_ip_pfx;      ///< src IP prefix
        ipvx_range_t src_ip_range;    ///< src IP range
        uint32_t     src_tag;         ///< src tag value
    };
    union {
        ip_prefix_t  dst_ip_pfx;      ///< dst IP prefix
        ipvx_range_t dst_ip_range;    ///< drc IP range
        uint32_t     dst_tag;         ///< drc tag value
    };
} __PACK__ rule_l3_match_t;

/// \brief    rule L4 match criteria
typedef struct rule_l4_match_s {
    union {
        struct {
            port_range_t    sport_range;    ///< source port range
            port_range_t    dport_range;    ///< destination port range
        };
        struct {
            match_type_t    type_match_type;///< match type for type field
            uint8_t         icmp_type;      ///< ICMP type
            match_type_t    code_match_type;///< match type for code field
            uint8_t         icmp_code;      ///< ICMP code
        };
    };
} __PACK__ rule_l4_match_t;

/// \brief    rule match
typedef struct rule_match_s {
    rule_l3_match_t    l3_match;    ///< Layer 3 match criteria
    rule_l4_match_t    l4_match;    ///< Layer 4 match criteria
} __PACK__ rule_match_t;

/// \brief    security rule action
typedef enum fw_action_e {
    SECURITY_RULE_ACTION_NONE  = 0,    ///< invalid action
    SECURITY_RULE_ACTION_ALLOW = 1,    ///< allow the packet
    SECURITY_RULE_ACTION_DENY  = 2,    ///< drop the packet silently
} fw_action_t;

/// \brief    security rule specific action data
typedef struct fw_action_data_s {
    fw_action_t    action;    ///< Firewall action
} fw_action_data_t;

/// \brief    generic rule action data
typedef union rule_action_data_s {
    fw_action_data_t    fw_action;    ///< Firewall action data
} rule_action_data_t;

///< \brief    policy rule attributes
typedef struct pds_policy_rule_attrs_s {
    uint8_t               alg_type;       ///< algo type
    bool                  stateful;       ///< true, if rule is stateful
    // priority of this rule (lower the numeric value, higher the priority is)
    // if there are multiple rules with same highest priority that match the
    // packet, the first such matching rule in the policy is picked (i.e.,
    // based on the order, first in the list of such matching rules)
    uint32_t              priority;
    rule_match_t          match;          ///< rule match
    rule_action_data_t    action_data;    ///< action and related information
} pds_policy_rule_attrs_t;

/// \brief    generic rule
typedef struct rule_s {
    pds_obj_key_t key;             ///< rule identifier
    pds_policy_rule_attrs_t attrs; ///< rule attributes
} __PACK__ rule_t;

/// \brief rule info contains all rule specific information
typedef struct rule_info_s {
    uint8_t            af;             ///< address family
    rule_action_data_t default_action; ///< default action for this policy
    uint32_t           num_rules;      ///< number of rules in the list
    rule_t             rules[0];       ///< list or rules
} rule_info_t;
#define POLICY_RULE_INFO_SIZE(count)        \
            (sizeof(rule_info_t) + (count) * sizeof(rule_t))

/// \brief    generic policy specification
typedef struct pds_policy_spec_s    pds_policy_spec_t;
struct pds_policy_spec_s {
    pds_obj_key_t   key;           ///< policy key
    rule_info_t     *rule_info;    ///< rule set of this policy

    pds_policy_spec_s() { rule_info = NULL; }
    ~pds_policy_spec_s() {
        if (rule_info) {
            SDK_FREE(PDS_MEM_ALLOC_SECURITY_POLICY, rule_info);
        }
    }
    /// assignment operator
    pds_policy_spec_t& operator=(const pds_policy_spec_t& policy) {
        // self-assignment guard
        if (this == &policy) {
            return *this;
        }
        if (rule_info) {
            SDK_FREE(PDS_MEM_ALLOC_SECURITY_POLICY, rule_info);
        }
        rule_info =
            (rule_info_t *)SDK_MALLOC(PDS_MEM_ALLOC_SECURITY_POLICY,
                               POLICY_RULE_INFO_SIZE(policy.rule_info->num_rules));
        SDK_ASSERT(rule_info != NULL);
        key = policy.key;
        memcpy(rule_info, policy.rule_info,
               POLICY_RULE_INFO_SIZE(policy.rule_info->num_rules));
        return *this;
    }
} __PACK__;

/// \brief policy status
typedef struct pds_policy_status_s {
    mem_addr_t policy_base_addr;    ///< policy base address
} pds_policy_status_t;

typedef struct rule_stats_s {
    /// number of times rule is hit
    uint64_t num_rule_hit;
} rule_stats_t;
#define POLICY_RULE_STATS_SIZE(count)    (sizeof(rule_stats_t) * (count))

/// \brief policy statistics
typedef struct pds_policy_stats_s {
    rule_stats_t *rule_stats;
} pds_policy_stats_t;

/// \brief policy info
typedef struct pds_policy_info_s {
    pds_policy_spec_t spec;         ///< specification
    pds_policy_status_t status;     ///< status
    pds_policy_stats_t stats;       ///< statistics
} pds_policy_info_t;

/// \brief    create policy
/// \param[in] spec    policy configurationn
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_policy_create(pds_policy_spec_t *spec,
                            pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief    read policy
/// \param[in]  key    policy key
/// \param[out] info    policy information
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_policy_read(pds_obj_key_t *key, pds_policy_info_t *info);

typedef void (*policy_read_cb_t)(pds_policy_info_t *info, void *ctxt);

/// \brief      read all policy
/// \param[in]  cb      callback function
/// \param[in]  ctxt    opaque context passed to cb
/// \return     #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_policy_read_all(policy_read_cb_t policy_get_cb, void *ctxt);

/// \brief    update policy
/// \param[in] spec    policy configuration
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_policy_update(pds_policy_spec_t *spec,
                            pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief    delete policy
/// \param[in] key    policy key
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_policy_delete(pds_obj_key_t *key,
                            pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief    policy rule key
///< \remark  when rules are deleted individually or queried individually, the
///           key must contain both parent policy key and individual policy rule
///           key/(uu)id
typedef struct pds_policy_rule_key_s {
    pds_obj_key_t rule_id;      ///< policy rule identifier
    pds_obj_key_t policy_id;    ///< policy this rule is part of
} pds_policy_rule_key_t;

/// \brief    policy rule specification
typedef struct pds_policy_rule_spec_s {
    pds_policy_rule_key_t key;        ///< rule identifier
    pds_policy_rule_attrs_t attrs;    ///< rule configuration
} pds_policy_rule_spec_t;

/// \brief policy rule status
typedef struct pds_policy_rule_status_s {
} pds_policy_rule_status_t;

/// \brief policy rule statistics
typedef struct pds_policy_rule_stats_s {
    /// number of times rule is hit
    uint64_t num_rule_hit;
} pds_policy_rule_stats_t;

/// \brief policy rule info
typedef struct pds_policy_rule_info_s {
    pds_policy_rule_spec_t spec;         ///< specification
    pds_policy_rule_status_t status;     ///< status
    pds_policy_rule_stats_t stats;       ///< statistics
} pds_policy_rule_info_t;

/// \brief    create policy rule
/// \param[in] spec    rule configurationn
/// \param[in] bctxt   batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_policy_rule_create(pds_policy_rule_spec_t *spec,
                                 pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief    read policy rule
/// \param[in]  key    rule key
/// \param[out] info    rule information
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_policy_rule_read(pds_policy_rule_key_t *key,
                               pds_policy_rule_info_t *info);

/// \brief    update policy rule
/// \param[in] spec    rule configuration
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_policy_rule_update(pds_policy_rule_spec_t *spec,
                                  pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief    delete policy rule
/// \param[in] key    rule key
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_policy_rule_delete(pds_policy_rule_key_t *key,
                                 pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

#define PDS_MAX_SECURITY_PROFILE  1 ///< only one instance of security profile

/// \brief security profile spec
typedef struct pds_security_profile_spec_s {
    pds_obj_key_t         key;                   ///< id
    // NOTE: if conn_track_en is modified on the fly, it will take affect only
    //       on sessions created thereafter and doesn't affect existing sessions
    bool                  conn_track_en;         ///< connection tracking on/off
    rule_action_data_t    default_action;        ///< action and related information
    uint32_t              tcp_idle_timeout;      ///< TCP idle timeout
    uint32_t              udp_idle_timeout;      ///< UDP idle timeout
    uint32_t              icmp_idle_timeout;     ///< ICMP idle timeout
    uint32_t              other_idle_timeout;    ///< other idle timeout
    uint32_t              tcp_syn_timeout;       ///< TCP syn timeout
    uint32_t              tcp_halfclose_timeout; ///< TCP half close timeout
    uint32_t              tcp_close_timeout;     ///< TCP close timeout
    uint32_t              tcp_drop_timeout;      ///< TCP drop timeout
    uint32_t              udp_drop_timeout;      ///< UDP drop timeout
    uint32_t              icmp_drop_timeout;     ///< ICMP drop timeout
    uint32_t              other_drop_timeout;    ///< other drop timeout
} pds_security_profile_spec_t;

/// \brief security profile status
typedef struct pds_security_profile_status_s {
} pds_security_profile_status_t;

/// \brief security profile stats
typedef struct pds_security_profile_stats_s {
} pds_security_profile_stats_t;

/// \brief security profile info
typedef struct pds_security_profile_info_s {
    pds_security_profile_spec_t spec;     ///< spec
    pds_security_profile_status_t status; ///< status
    pds_security_profile_stats_t stats;   ///< stats
} pds_security_profile_info_t;

typedef void (*pds_security_profile_read_cb_t)(const pds_security_profile_info_t *info, void *ctxt);

/// \brief    create security profile
/// \param[in] spec  security profile configuration
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_security_profile_create(pds_security_profile_spec_t *spec,
                                      pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief    read security profile
/// \param[in]  cb    callback to translate spec(got from vpp ipc) to protobuf
/// \param[out] ctxt  security profile protobuf response
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_security_profile_read(pds_security_profile_read_cb_t cb,
                                    void *ctxt);
/// \brief    update security profile
/// \param[in] spec  security profile configuration
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_security_profile_update(pds_security_profile_spec_t *spec,
                                      pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// \brief    delete security profile
/// \param[in] key   security profile key
/// \param[in] bctxt batch context if API is invoked in a batch
/// \return    #SDK_RET_OK on success, failure status code on error
sdk_ret_t pds_security_profile_delete(pds_obj_key_t *key,
                                      pds_batch_ctxt_t bctxt = PDS_BATCH_CTXT_INVALID);

/// @}

#endif    // __INCLUDE_API_PDS_POLICY_HPP__
