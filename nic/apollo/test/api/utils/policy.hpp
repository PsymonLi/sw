//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#ifndef __TEST_API_UTILS_POLICY_HPP__
#define __TEST_API_UTILS_POLICY_HPP__

#include "nic/apollo/api/include/pds_policy.hpp"
#include "nic/apollo/test/api/utils/api_base.hpp"
#include "nic/apollo/test/api/utils/feeder.hpp"

namespace test {
namespace api {

enum rule_attrs {
    RULE_ATTR_STATEFUL              = bit(0),
    RULE_ATTR_PRIORITY              = bit(1),
    RULE_ATTR_L3_MATCH              = bit(2),
    RULE_ATTR_L4_MATCH              = bit(3),
    RULE_ATTR_ACTION                = bit(4),
};

enum rule_info_attrs {
    RULE_INFO_ATTR_AF               = bit(0),
    RULE_INFO_ATTR_RULE             = bit(1),
    RULE_INFO_ATTR_DEFAULT_ACTION   = bit(2),
};

enum layer_t {
    L3,
    L4,
    LAYER_ALL,
};

enum action_t {
    ALLOW,
    DENY,
    RANDOM,
};

// Policy test feeder class
class policy_feeder : public feeder {
public:
    // Test params
    pds_policy_spec_t spec;
    std::string cidr_str;
    uint32_t num_rules;
    uint16_t stateful_rules;
    // constructor
    policy_feeder() { };
    policy_feeder(policy_feeder& feeder) {
        init(feeder.spec.key, feeder.stateful_rules,
             feeder.spec.rule_info->af, feeder.cidr_str, feeder.num_obj,
             feeder.spec.rule_info->num_rules);
    }

    // Initialize feeder with the base set of values
    void init(pds_obj_key_t key, uint16_t stateful_rules, uint8_t af,
              std::string cidr_str, uint32_t num_policy = 1,
              uint32_t num_rules_per_policy = 0);

    // Iterate helper routines
    void iter_next(int width = 1);

    // Init routines
    void spec_alloc(pds_policy_spec_t *spec);

    // Build routines
    void key_build(pds_obj_key_t *key) const;
    void spec_build(pds_policy_spec_t *spec) const;

    // Compare routines
    bool key_compare(const pds_obj_key_t *key) const;
    bool spec_compare(const pds_policy_spec_t *spec) const;
    bool status_compare(const pds_policy_status_t *status1,
                        const pds_policy_status_t *status2) const;
};

// Dump prototypes
inline std::ostream&
operator<<(std::ostream& os, const pds_policy_spec_t *spec) {
    os << &spec->key
       << " af: " << +(spec->rule_info ? (uint32_t)spec->rule_info->af : 0)
       << " default action: " << (spec->rule_info ?
                                  spec->rule_info->default_action.
                                  fw_action.action : SECURITY_RULE_ACTION_NONE)
       << " num rules: " << (spec->rule_info ? spec->rule_info->num_rules : 0);
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_policy_status_t *status) {
    os << "  ";
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_policy_stats_t *stats) {
    os << "  ";
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_policy_info_t *obj) {
    os << " Policy info =>" << &obj->spec << std::endl;
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const policy_feeder& obj) {
    os << "Policy feeder =>" << &obj.spec
       << " cidr_str: " << obj.cidr_str
       << std::endl;
    return os;
}

// CRUD prototypes
API_CREATE(policy);
API_READ(policy);
API_UPDATE(policy);
API_DELETE(policy);

// Policy crud helper prototypes
void policy_create(policy_feeder& feeder);
void policy_read(policy_feeder& feeder, sdk_ret_t exp_result = SDK_RET_OK);
void policy_rule_update(policy_feeder& feeder, pds_policy_spec_t *spec,
                        uint64_t chg_bmap, sdk_ret_t exp_result = SDK_RET_OK);
void policy_rule_info_update(policy_feeder& feeder, pds_policy_spec_t *spec,
                             uint64_t chg_bmap, sdk_ret_t exp_result =
                                                                SDK_RET_OK);
void policy_update(policy_feeder& feeder);
void policy_delete(policy_feeder& feeder);

void create_policy_spec(std::string cidr_str, uint8_t af=IP_AF_IPV4,
                        uint16_t num_rules=1, rule_info_t **rule_info=NULL,
                        layer_t layer=LAYER_ALL, action_t action=ALLOW,
                        uint32_t priority=0);

// Function prototypes
void sample_policy_setup(pds_batch_ctxt_t bctxt);
void sample_policy_teardown(pds_batch_ctxt_t bctxt);

// Policy rule test feeder class
class policy_rule_feeder : public feeder {
public:
    // Test params
    pds_policy_rule_spec_t spec;
    std::string cidr_str;
    ip_prefix_t base_ip_pfx;
    uint32_t base_rule_id;
    // constructor
    policy_rule_feeder() { };
    policy_rule_feeder(policy_rule_feeder& feeder) {
        pds_obj_key_t rule_id = feeder.spec.key.rule_id;
        pds_obj_key_t policy_id = feeder.spec.key.policy_id;
        init(pdsobjkey2int(rule_id), pdsobjkey2int(policy_id),
             feeder.cidr_str, feeder.num_obj);
    }

    // Initialize feeder with the base set of values
    void init(uint32_t rule_id, uint32_t policy_id,
              std::string cidr_str, uint32_t num_rules);

    // Iterate helper routines
    void iter_next(int width = 1);

    // Build routines
    void key_build(pds_policy_rule_key_t *key) const;
    void spec_build(pds_policy_rule_spec_t *spec) const;

    // Compare routines
    bool key_compare(const pds_policy_rule_key_t *key) const;
    bool spec_compare(const pds_policy_rule_spec_t *spec) const;
    bool status_compare(const pds_policy_rule_status_t *status1,
                        const pds_policy_rule_status_t *status2) const;
};

// Dump prototypes
inline std::ostream&
operator<<(std::ostream& os, const pds_policy_rule_spec_t *spec) {
    os << " rule id: " << spec->key.rule_id.str()
    << " policy id: " << spec->key.policy_id.str()
    << " stateful: " << (spec->attrs.stateful ? "true" : "false")
    << " priority: " << spec->attrs.priority;
    os << std::endl;
    os << " l3 match => "
    << " protocol match type: " << spec->attrs.match.l3_match.proto_match_type
    << " ip protocol: " << (int)spec->attrs.match.l3_match.ip_proto
    << " source match type: " << spec->attrs.match.l3_match.src_match_type
    << " destination match type: " << spec->attrs.match.l3_match.dst_match_type;
    switch (spec->attrs.match.l3_match.src_match_type) {
    case IP_MATCH_PREFIX:
        os << " source ip prefix: "
            << ippfx2str(&spec->attrs.match.l3_match.src_ip_pfx);
        break;
    case IP_MATCH_RANGE:
        os << " source ip range: "
            << " af: " << (uint32_t)spec->attrs.match.l3_match.src_ip_range.af
            << " ip lower-upper: "
            << ipvxrange2str((ipvx_range_t*)
                             &spec->attrs.match.l3_match.src_ip_range);
        break;
    case IP_MATCH_TAG:
        os << " source ip tag: " << spec->attrs.match.l3_match.src_tag;
        break;
    default:
        break;
    }
    switch (spec->attrs.match.l3_match.dst_match_type) {
    case IP_MATCH_PREFIX:
        os << " destination ip prefix: "
            << ippfx2str(&spec->attrs.match.l3_match.dst_ip_pfx);
        break;
    case IP_MATCH_RANGE:
        os << " destination ip range: "
            << " af: " << (uint32_t)spec->attrs.match.l3_match.dst_ip_range.af
            << " ip lower-upper: "
            << ipvxrange2str((ipvx_range_t*)
                             &spec->attrs.match.l3_match.dst_ip_range);
        break;
    case IP_MATCH_TAG:
        os << " destination ip tag: " << spec->attrs.match.l3_match.src_tag;
        break;
    default:
        break;
    }
    os << std::endl;
    os << " l4 match => ";
    if (spec->attrs.match.l3_match.ip_proto == IP_PROTO_TCP ||
        spec->attrs.match.l3_match.ip_proto == IP_PROTO_UDP) {
        os << " source port range: ip lower: "
            << spec->attrs.match.l4_match.sport_range.port_lo
            << " ip upper: "
            << spec->attrs.match.l4_match.sport_range.port_hi
            << " destination port range: ip lower: "
            << spec->attrs.match.l4_match.dport_range.port_lo
            << " ip upper: "
            << spec->attrs.match.l4_match.dport_range.port_hi;
    }
    if (spec->attrs.match.l3_match.ip_proto == IP_PROTO_ICMP) {
        os << " type match type: "
            << spec->attrs.match.l4_match.type_match_type
            << " icmp type: "
            << (int)spec->attrs.match.l4_match.icmp_type
            << " code match type: "
            << spec->attrs.match.l4_match.code_match_type
            << " icmp code: "
            << (int)spec->attrs.match.l4_match.icmp_code;
    }
    os << std::endl;
    os << " action data:" << spec->attrs.action_data.fw_action.action;
    os << std::endl;
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_policy_rule_status_t *status) {
    os << "  ";
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_policy_rule_stats_t *stats) {
    os << "  ";
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_policy_rule_info_t *obj) {
    os << " Policy rule info =>"
    << &obj->spec
    << &obj->status
    << &obj->stats
    << std::endl;
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const policy_rule_feeder& obj) {
    os << "Policy rule feeder =>"
    << &obj.spec
    << " cidr_str: " << obj.cidr_str
    << std::endl;
    return os;
}

// CRUD prototypes
API_CREATE(policy_rule);
API_READ_1(policy_rule);
API_UPDATE(policy_rule);
API_DELETE_1(policy_rule);
}    // namespace api
}    // namespace test

#endif    // __TEST_API_UTILS_POLICY_HPP__
