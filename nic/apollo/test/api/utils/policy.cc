//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/apollo/test/api/utils/batch.hpp"
#include "nic/apollo/test/api/utils/policy.hpp"

namespace test {
namespace api {

#define MAX_RANGE_RULES_V6 5
#define MAX_RANGE_RULES_V4 20
static const uint8_t k_base_rule_id = 1;
//----------------------------------------------------------------------------
// Policy feeder class routines
//----------------------------------------------------------------------------

void
policy_feeder::init(pds_obj_key_t key,
                    uint16_t stateful_rules,
                    uint8_t af,
                    std::string cidr_str,
                    uint32_t num_policy,
                    uint32_t num_rules_per_policy) {
    uint32_t max_rules;

    if (num_rules_per_policy) {
        max_rules = num_rules_per_policy;
    } else {
        max_rules = ((af == IP_AF_IPV4) ?
                          PDS_MAX_RULES_PER_IPV4_SECURITY_POLICY :
                          PDS_MAX_RULES_PER_IPV6_SECURITY_POLICY);
    }
    memset(&this->spec, 0, sizeof(pds_policy_spec_t));
    this->spec.key = key;
    this->spec.rule_info = NULL;
    this->cidr_str = cidr_str;
    this->num_rules = max_rules;
    num_obj = num_policy;
    create_policy_spec(this->cidr_str, af, this->num_rules,
                       (rule_info_t **)&(this->spec.rule_info));
}

void
policy_feeder::iter_next(int width) {
    this->spec.key = int2pdsobjkey(pdsobjkey2int(this->spec.key) + width);
    cur_iter_pos++;
}

static inline bool
is_l3 (layer_t layer)
{
    if (layer == L3 || layer == LAYER_ALL) {
        return true;
    }
    return false;
}

static inline bool
is_l4 (layer_t layer)
{
    if (layer == L4 || layer == LAYER_ALL) {
        return true;
    }
    return false;
}

static inline fw_action_t
action_get (action_t action)
{
    switch (action) {
    case DENY:
        return SECURITY_RULE_ACTION_DENY;
    case RANDOM:
        return rand()%2 ? SECURITY_RULE_ACTION_ALLOW :
                          SECURITY_RULE_ACTION_DENY;
    case ALLOW:
    default:
        return SECURITY_RULE_ACTION_ALLOW;
    }
}

void
policy_feeder::key_build(pds_obj_key_t *key) const {
    memcpy(key, &this->spec.key, sizeof(pds_obj_key_t));
}

void
fill_rule_attrs (pds_policy_rule_attrs_t *attrs, ip_prefix_t ip_pfx,
                 uint32_t rule_id,  uint16_t num_rules, layer_t layer,
                 action_t action, uint32_t priority, uint32_t base_rule_id)
{
    uint16_t max_range_rules = (ip_pfx.addr.af == IP_AF_IPV6)?
        MAX_RANGE_RULES_V6 : MAX_RANGE_RULES_V4;
    // create first half rules as stateful-rules
    // and rest of rules as stateful
    if (rule_id <= (base_rule_id+(num_rules/2))) {
        attrs->stateful = true;
    } else {
        attrs->stateful = false;
    }
    // setting priority to rule_id
    attrs->priority = (rule_id % PDS_MAX_RULE_PRIORITY);
    if (is_l3(layer)) {
        attrs->match.l3_match.proto_match_type = MATCH_SPECIFIC;
        // create half of stateful and non-stateful rules as icmp rules
        // and other halves as tcp rules
        if ((rule_id <= (base_rule_id+num_rules/4)) ||
            ((rule_id > base_rule_id+ num_rules/2)
             && (rule_id <= (base_rule_id + (3 * num_rules)/4)))) {
            attrs->match.l3_match.ip_proto = IP_PROTO_ICMP;
        } else {
            attrs->match.l3_match.ip_proto = IP_PROTO_TCP;
        }
        // create range match rules based on platform and it's limits,
        // and rest as prefix match rules
        if (apulu() && rule_id <= base_rule_id+max_range_rules) {
            attrs->match.l3_match.src_match_type = IP_MATCH_RANGE;
            attrs->match.l3_match.dst_match_type = IP_MATCH_RANGE;
            attrs->match.l3_match.src_ip_range.af = ip_pfx.addr.af;
            attrs->match.l3_match.dst_ip_range.af = ip_pfx.addr.af;
            memcpy(&attrs->match.l3_match.src_ip_range.ip_lo,
                   &ip_pfx.addr.addr, sizeof(ipvx_addr_t));
            memcpy(&attrs->match.l3_match.dst_ip_range.ip_lo,
                   &ip_pfx.addr.addr, sizeof(ipvx_addr_t));
            test::increment_ip_addr(&ip_pfx.addr, 2);
            memcpy(&attrs->match.l3_match.src_ip_range.ip_hi,
                   &ip_pfx.addr.addr, sizeof(ipvx_addr_t));
            memcpy(&attrs->match.l3_match.dst_ip_range.ip_hi,
                   &ip_pfx.addr.addr, sizeof(ipvx_addr_t));
        } else {
            attrs->match.l3_match.src_match_type = IP_MATCH_PREFIX;
            attrs->match.l3_match.dst_match_type = IP_MATCH_PREFIX;
            memcpy(&attrs->match.l3_match.src_ip_pfx,
                   &ip_pfx, sizeof(ip_prefix_t));
            // using same ip as dst ip just for testing
            memcpy(&attrs->match.l3_match.dst_ip_pfx,
                   &ip_pfx, sizeof(ip_prefix_t));
        }
    }
    if (is_l4(layer)) {
        if (attrs->match.l3_match.ip_proto == IP_PROTO_TCP ||
            attrs->match.l3_match.ip_proto == IP_PROTO_UDP) {
            attrs->match.l4_match.sport_range.port_lo = 0;
            attrs->match.l4_match.sport_range.port_hi = 65535;
            attrs->match.l4_match.dport_range.port_lo = 0;
            attrs->match.l4_match.dport_range.port_hi = 65535;
        }
        if (attrs->match.l3_match.ip_proto == IP_PROTO_ICMP) {
            attrs->match.l4_match.type_match_type = MATCH_SPECIFIC;
            attrs->match.l4_match.icmp_type = 1;
            attrs->match.l4_match.code_match_type = MATCH_SPECIFIC;
            attrs->match.l4_match.icmp_code = 1;
        }
    }
    attrs->action_data.fw_action.action = action_get(action);
}

void
create_policy_spec(std::string cidr_str, uint8_t af, uint16_t num_rules,
                   rule_info_t **rule_info,layer_t layer, action_t action,
                   uint32_t priority)
{
    ip_prefix_t ip_pfx;
    rule_t *rule;
    uint16_t max_range_rules = 0;

    *rule_info =
        (rule_info_t *)SDK_CALLOC(PDS_MEM_ALLOC_SECURITY_POLICY,
                                  POLICY_RULE_INFO_SIZE(num_rules));
    (*rule_info)->af = af;
    (*rule_info)->num_rules = num_rules;
    if (num_rules == 0) return;
    test::extract_ip_pfx((char *)cidr_str.c_str(), &ip_pfx);
    max_range_rules = (ip_pfx.addr.af == IP_AF_IPV6)?
        MAX_RANGE_RULES_V6 : MAX_RANGE_RULES_V4;
    for (uint32_t i = 0; i < num_rules; i++) {
        rule = &(*rule_info)->rules[i];
        rule->key = int2pdsobjkey(i+1);
        fill_rule_attrs(&rule->attrs, ip_pfx, i+1, num_rules,
                        layer, action, priority, k_base_rule_id);
        if (apulu() && (i+1 <= max_range_rules)) {
            test::increment_ip_addr(&ip_pfx.addr, 3);
        } else {
            test::increment_ip_addr(&ip_pfx.addr);
        }
    }
}

void
policy_feeder::spec_alloc(pds_policy_spec_t *spec) {
    spec->rule_info =
        (rule_info_t *)SDK_CALLOC(PDS_MEM_ALLOC_SECURITY_POLICY,
                                  POLICY_RULE_INFO_SIZE(0));
}

void
policy_feeder::spec_build(pds_policy_spec_t *spec) const {
    memcpy(spec, &this->spec, sizeof(pds_policy_spec_t));
    create_policy_spec(this->cidr_str, this->spec.rule_info->af, this->num_rules,
                       (rule_info_t **)&(spec->rule_info));
}

bool
policy_feeder::key_compare(const pds_obj_key_t *key) const {
    return (memcmp(key, &this->spec.key, sizeof(pds_obj_key_t)) == 0);
}

bool
policy_feeder::spec_compare(const pds_policy_spec_t *spec) const {
    if (spec->rule_info == NULL)
        return false;
    if (spec->rule_info->af != this->spec.rule_info->af)
        return false;
    if (spec->rule_info->num_rules != this->spec.rule_info->num_rules)
        return false;
    return true;
}

bool
policy_feeder::status_compare(const pds_policy_status_t *status1,
                              const pds_policy_status_t *status2) const {
    return true;
}

//----------------------------------------------------------------------------
// Policy CRUD helper routines
//----------------------------------------------------------------------------

void
policy_create (policy_feeder& feeder)
{
    pds_batch_ctxt_t bctxt = batch_start();

    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_create<policy_feeder>(bctxt, feeder)));
    batch_commit(bctxt);
}

void
policy_read (policy_feeder& feeder, sdk_ret_t exp_result)
{
    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_read<policy_feeder>(feeder, exp_result)));
}

static void
rule_attr_update (policy_feeder& feeder, rule_info_t *info,
                  uint64_t chg_bmap)
{
    SDK_ASSERT(info);
    feeder.spec.rule_info->num_rules = info->num_rules;
    memcpy(feeder.spec.rule_info->rules, info->rules,
           sizeof(rule_t) * info->num_rules);
}

static void
rule_info_attr_update (policy_feeder& feeder, rule_info_t *info,
                       uint64_t chg_bmap)
{
    if (bit_isset(chg_bmap, RULE_INFO_ATTR_AF)) {
        feeder.spec.rule_info->af = info->af;
        feeder.spec.rule_info->num_rules = info->num_rules;
        memcpy(&feeder.spec.rule_info->rules, &info->rules,
               sizeof(rule_t) * info->num_rules);
    }
    if (bit_isset(chg_bmap, RULE_INFO_ATTR_RULE)) {
        memcpy(&feeder.spec.rule_info->rules, &info->rules,
               sizeof(rule_t) * info->num_rules);
    }
    if (bit_isset(chg_bmap, RULE_INFO_ATTR_DEFAULT_ACTION)) {
        feeder.spec.rule_info->default_action.fw_action.action =
                            info->default_action.fw_action.action;
        memcpy(&feeder.spec.rule_info->default_action,
               &info->default_action,
               sizeof(rule_action_data_t));
    }
}

void
policy_rule_update (policy_feeder& feeder, pds_policy_spec_t *spec,
                    uint64_t chg_bmap, sdk_ret_t exp_result)
{
    pds_batch_ctxt_t bctxt = batch_start();
    rule_info_t *rule_info = spec ? spec->rule_info : NULL;

    rule_attr_update(feeder, rule_info, chg_bmap);
    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_update<policy_feeder>(bctxt, feeder)));

    // if expected result is err, batch commit should fail
    if (exp_result == SDK_RET_ERR)
        batch_commit_fail(bctxt);
    else
        batch_commit(bctxt);
}

void
policy_rule_info_update (policy_feeder& feeder, pds_policy_spec_t *spec,
                         uint64_t chg_bmap, sdk_ret_t exp_result)
{
    pds_batch_ctxt_t bctxt = batch_start();

    rule_info_attr_update(feeder, spec->rule_info, chg_bmap);
    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_update<policy_feeder>(bctxt, feeder)));

    // if expected result is err, batch commit should fail
    if (exp_result == SDK_RET_ERR)
        batch_commit_fail(bctxt);
    else
        batch_commit(bctxt);
}

void
policy_update (policy_feeder& feeder)
{
    pds_batch_ctxt_t bctxt = batch_start();

    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_update<policy_feeder>(bctxt, feeder)));
    batch_commit(bctxt);
}

void
policy_delete (policy_feeder& feeder)
{
    pds_batch_ctxt_t bctxt = batch_start();

    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_delete<policy_feeder>(bctxt, feeder)));
    batch_commit(bctxt);
}

//----------------------------------------------------------------------------
// Misc routines
//----------------------------------------------------------------------------

// do not modify these sample values as rest of system is sync with these
static policy_feeder k_pol_feeder;

void sample_policy_setup(pds_batch_ctxt_t bctxt) {
    pds_obj_key_t pol_key = int2pdsobjkey(TEST_POLICY_ID_BASE + 1);

    // setup and teardown parameters should be in sync
    k_pol_feeder.init(pol_key, 512, IP_AF_IPV4, "10.0.0.1/16", 5);
    many_create(bctxt, k_pol_feeder);

    pol_key = int2pdsobjkey(pdsobjkey2int(pol_key) + 5);
    k_pol_feeder.init(pol_key, 512, IP_AF_IPV6, "2001::1/64", 5);
    many_create(bctxt, k_pol_feeder);

    pol_key = int2pdsobjkey(pdsobjkey2int(pol_key) + 5);
    k_pol_feeder.init(pol_key, 512, IP_AF_IPV4, "20.0.0.1/16", 5);
    many_create(bctxt, k_pol_feeder);

    pol_key = int2pdsobjkey(pdsobjkey2int(pol_key) + 5);
    k_pol_feeder.init(pol_key, 512, IP_AF_IPV6, "3001::1/64", 5);
    many_create(bctxt, k_pol_feeder);
}

void sample_policy_teardown(pds_batch_ctxt_t bctxt) {
    pds_obj_key_t pol_key = int2pdsobjkey(TEST_POLICY_ID_BASE + 1);

    // this feeder base values doesn't matter in case of deletes
    k_pol_feeder.init(pol_key, 512, IP_AF_IPV4, "10.0.0.1/16", 5);
    many_delete(bctxt, k_pol_feeder);

    pol_key = int2pdsobjkey(pdsobjkey2int(pol_key) + 5);
    k_pol_feeder.init(pol_key, 512, IP_AF_IPV6, "2001::1/64", 5);
    many_delete(bctxt, k_pol_feeder);

    pol_key = int2pdsobjkey(pdsobjkey2int(pol_key) + 5);
    k_pol_feeder.init(pol_key, 512, IP_AF_IPV4, "20.0.0.1/16", 5);
    many_delete(bctxt, k_pol_feeder);

    pol_key = int2pdsobjkey(pdsobjkey2int(pol_key) + 5);
    k_pol_feeder.init(pol_key, 512, IP_AF_IPV6, "3001::1/64", 5);
    many_delete(bctxt, k_pol_feeder);
}

//----------------------------------------------------------------------------
// Policy route feeder class routines
//----------------------------------------------------------------------------

void
policy_rule_feeder::init(uint32_t rule_id, uint32_t policy_id,
                         std::string cidr_str, uint32_t num_rules) {
    ip_prefix_t ip_pfx;

    memset(&this->spec, 0, sizeof(pds_policy_rule_spec_t));
    spec.key.rule_id = int2pdsobjkey(rule_id);
    spec.key.policy_id = int2pdsobjkey(policy_id);
    this->cidr_str = cidr_str;
    this->base_rule_id = rule_id;
    num_obj = num_rules;
    test::extract_ip_pfx((char *)cidr_str.c_str(), &ip_pfx);
    fill_rule_attrs(&this->spec.attrs, ip_pfx, rule_id, num_obj,
                    LAYER_ALL, ALLOW, 0, this->base_rule_id);
}

void
policy_rule_feeder::iter_next(int width) {
    uint32_t rule_id = 0;
    ip_prefix_t ip_pfx;
    uint16_t max_range_rules = 0 ;
    rule_id = pdsobjkey2int(spec.key.rule_id) + width;
    spec.key.rule_id = int2pdsobjkey(rule_id);
    cur_iter_pos++;
    test::extract_ip_pfx((char *)this->cidr_str.c_str(), &ip_pfx);
    max_range_rules = (ip_pfx.addr.af == IP_AF_IPV6)?
        MAX_RANGE_RULES_V6 : MAX_RANGE_RULES_V4;
    if (apulu() && (rule_id-width <= this->base_rule_id+max_range_rules)) {
        memcpy(&ip_pfx.addr.addr,
               &spec.attrs.match.l3_match.src_ip_range.ip_hi,
               sizeof(ipvx_addr_t));
        test::increment_ip_addr(&ip_pfx.addr);
    } else {
        memcpy(&ip_pfx, &spec.attrs.match.l3_match.src_ip_pfx,
               sizeof(ip_prefix_t));
        test::increment_ip_addr(&ip_pfx.addr);
    }
    memcpy(&this->base_ip_pfx, &ip_pfx, sizeof(ip_prefix_t));
    fill_rule_attrs(&spec.attrs, ip_pfx, rule_id, num_obj,
                    LAYER_ALL, ALLOW, 0, this->base_rule_id);
}

void
policy_rule_feeder::key_build(pds_policy_rule_key_t *key) const {
    memset(key, 0, sizeof(pds_policy_rule_key_t));
    key->rule_id = spec.key.rule_id;
    key->policy_id = spec.key.policy_id;
}

void
policy_rule_feeder::spec_build(pds_policy_rule_spec_t *spec) const {
    ip_prefix_t ip_pfx;
    uint32_t rule_id = 0;
    memcpy(spec, &this->spec, sizeof(pds_policy_rule_spec_t));
    rule_id =  pdsobjkey2int(spec->key.rule_id);
    if (rule_id == this->base_rule_id) {
        test::extract_ip_pfx((char *)this->cidr_str.c_str(), &ip_pfx);
    } else {
        memcpy(&ip_pfx, &this->base_ip_pfx, sizeof(ip_prefix_t));
    }
    fill_rule_attrs(&spec->attrs, ip_pfx, rule_id, num_obj,
                    LAYER_ALL, ALLOW, 0, this->base_rule_id);
}

bool
policy_rule_feeder::key_compare(const pds_policy_rule_key_t *key) const {
    if (key->rule_id != spec.key.rule_id)
        return false;
    if (key->policy_id != spec.key.policy_id)
        return false;
    return true;
}

bool
policy_rule_feeder::spec_compare(const pds_policy_rule_spec_t *spec) const {
    if (spec->attrs.stateful != this->spec.attrs.stateful)
        return false;
    if (spec->attrs.priority != this->spec.attrs.priority)
        return false;
    if (spec->attrs.match.l3_match.proto_match_type
        != this->spec.attrs.match.l3_match.proto_match_type)
        return false;
    if (spec->attrs.match.l3_match.ip_proto
        != this->spec.attrs.match.l3_match.ip_proto)
        return false;
    if (spec->attrs.match.l3_match.src_match_type
        != this->spec.attrs.match.l3_match.src_match_type)
        return false;
    if (spec->attrs.match.l3_match.dst_match_type
        != this->spec.attrs.match.l3_match.dst_match_type)
        return false;
    switch (spec->attrs.match.l3_match.src_match_type) {
    case IP_MATCH_PREFIX:
        if (ip_prefix_is_equal(
                (ip_prefix_t*)&spec->attrs.match.l3_match.src_ip_pfx,
                (ip_prefix_t*)&this->spec.attrs.match.l3_match.src_ip_pfx)
            == false)
            return false;
        break;
    case IP_MATCH_RANGE:
        if (spec->attrs.match.l3_match.src_ip_range.af !=
            this->spec.attrs.match.l3_match.src_ip_range.af)
            return false;
        if (ip_addr_is_equal(
                (ip_addr_t*)
                &spec->attrs.match.l3_match.src_ip_range.ip_lo.v4_addr,
                (ip_addr_t*)
                &this->spec.attrs.match.l3_match.src_ip_range.ip_lo.v4_addr)
            == false)
            return false;
        if (ip_addr_is_equal(
                (ip_addr_t*)
                &spec->attrs.match.l3_match.src_ip_range.ip_lo.v4_addr,
                (ip_addr_t*)
                &this->spec.attrs.match.l3_match.src_ip_range.ip_lo.v4_addr)
            == false)
            return false;
        break;
    case IP_MATCH_TAG:
        if (spec->attrs.match.l3_match.src_tag !=
            this->spec.attrs.match.l3_match.src_tag)
            return false;
        break;
    default:
        return false;
    }
    switch (spec->attrs.match.l3_match.dst_match_type) {
    case IP_MATCH_PREFIX:
        if (ip_prefix_is_equal(
                (ip_prefix_t*)&spec->attrs.match.l3_match.dst_ip_pfx,
                (ip_prefix_t*)&this->spec.attrs.match.l3_match.dst_ip_pfx)
            == false)
            return false;
        break;
    case IP_MATCH_RANGE:
        if (spec->attrs.match.l3_match.dst_ip_range.af !=
            this->spec.attrs.match.l3_match.dst_ip_range.af)
            return false;
        if (ip_addr_is_equal(
                (ip_addr_t*)
                &spec->attrs.match.l3_match.dst_ip_range.ip_lo.v4_addr,
                (ip_addr_t*)
                &this->spec.attrs.match.l3_match.dst_ip_range.ip_lo.v4_addr)
            == false)
            return false;
        if (ip_addr_is_equal(
                (ip_addr_t*)
                &spec->attrs.match.l3_match.dst_ip_range.ip_lo.v4_addr,
                (ip_addr_t*)
                &this->spec.attrs.match.l3_match.dst_ip_range.ip_lo.v4_addr)
            == false)
            return false;
        break;
    case IP_MATCH_TAG:
        if (spec->attrs.match.l3_match.dst_tag !=
            this->spec.attrs.match.l3_match.dst_tag)
            return false;
        break;
    default:
        return false;
    }
    if (spec->attrs.match.l4_match.sport_range.port_lo !=
        this->spec.attrs.match.l4_match.sport_range.port_lo)
        return false;
    if (spec->attrs.match.l4_match.dport_range.port_hi !=
        this->spec.attrs.match.l4_match.dport_range.port_hi)
        return false;
    if (spec->attrs.match.l4_match.type_match_type !=
        this->spec.attrs.match.l4_match.type_match_type)
        return false;
    if (spec->attrs.match.l4_match.icmp_type !=
        this->spec.attrs.match.l4_match.icmp_type)
        return false;
    if (spec->attrs.match.l4_match.code_match_type !=
        this->spec.attrs.match.l4_match.code_match_type)
        return false;
    if (spec->attrs.match.l4_match.icmp_code !=
        this->spec.attrs.match.l4_match.icmp_code)
        return false;
    if (spec->attrs.action_data.fw_action.action !=
        this->spec.attrs.action_data.fw_action.action)
        return false;
    return true;
}

bool
policy_rule_feeder::status_compare(
    const pds_policy_rule_status_t *status1,
    const pds_policy_rule_status_t *status2) const {
    return true;
}
}    // namespace api
}    // namespace test
