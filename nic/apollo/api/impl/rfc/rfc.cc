/**
 * Copyright (c) 2019 Pensando Systems, Inc.
 *
 * @file    rfc.cc
 *
 * @brief   RFC library implementation
 */

#include <string>
#include "include/sdk/platform.hpp"
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/pds_state.hpp"
#include "nic/apollo/api/impl/lpm/lpm.hpp"
#include "nic/apollo/api/impl/rfc/rfc.hpp"
#include "nic/apollo/api/impl/rfc/rfc_priv.hpp"
#include "nic/apollo/api/impl/rfc/rfc_impl.hpp"

namespace rfc {

void
rfc_policy_rule_dump (policy_t *policy, uint32_t rule_num)
{
    rule_t    *rule = &policy->rules[rule_num];
    string    rule_str = "";

    if (!pds_trace(sdk::types::trace_debug)) {
        return;
    }
    rule_str += std::to_string(rule_num) + ". ";
    rule_str += (policy->af == IP_AF_IPV4) ? "v4, " : "v6, ";
    rule_str += rule->attrs.stateful ? "stateful : " : "stateless : ";
    if (rule->attrs.match.l3_match.proto_match_type == MATCH_SPECIFIC) {
        rule_str += "match = (proto " +
                    std::to_string(rule->attrs.match.l3_match.ip_proto) + ", ";
    } else {
        rule_str += "match = (proto ANY, ";
    }
    if (rule->attrs.match.l3_match.src_match_type == IP_MATCH_PREFIX) {
        rule_str+=
                "src " + string(ippfx2str(&rule->attrs.match.l3_match.src_ip_pfx)) + ", ";
    } else if (rule->attrs.match.l3_match.src_match_type == IP_MATCH_RANGE) {
        rule_str+=
            "src " + string(ipvxrange2str(&rule->attrs.match.l3_match.src_ip_range)) + ", ";
    } else if (rule->attrs.match.l3_match.src_match_type == IP_MATCH_TAG) {
        rule_str+= "stag " + std::to_string(rule->attrs.match.l3_match.src_tag) + ", ";
    } else if (rule->attrs.match.l3_match.src_match_type == IP_MATCH_NONE) {
        rule_str+= "src none, ";
    }
    if (rule->attrs.match.l3_match.dst_match_type == IP_MATCH_PREFIX) {
        rule_str+=
            "dst " + string(ippfx2str(&rule->attrs.match.l3_match.dst_ip_pfx)) + ", ";
    } else if (rule->attrs.match.l3_match.dst_match_type == IP_MATCH_RANGE) {
        rule_str+=
            "dst " + string(ipvxrange2str(&rule->attrs.match.l3_match.dst_ip_range)) + ", ";
    } else if (rule->attrs.match.l3_match.dst_match_type == IP_MATCH_TAG) {
        rule_str+= "dtag " + std::to_string(rule->attrs.match.l3_match.dst_tag) + ", ";
    } else if (rule->attrs.match.l3_match.dst_match_type == IP_MATCH_NONE) {
        rule_str+= "dst none, ";
    }
    if (rule->attrs.match.l3_match.proto_match_type == MATCH_SPECIFIC) {
        if (rule->attrs.match.l3_match.ip_proto == IP_PROTO_ICMP) {
            if (rule->attrs.match.l4_match.type_match_type == MATCH_SPECIFIC) {
                if (rule->attrs.match.l4_match.code_match_type == MATCH_SPECIFIC) {
                    rule_str += "ICMP type/code " +
                            std::to_string(rule->attrs.match.l4_match.icmp_type) + "/" +
                            std::to_string(rule->attrs.match.l4_match.icmp_code) + ", ";
                } else {
                    rule_str += "ICMP type/code " +
                            std::to_string(rule->attrs.match.l4_match.icmp_type) + "/" +
                            "ANY, ";
                }
            } else {
                rule_str += "ICMP type/code ANY/ANY, ";
            }
        } else if ((rule->attrs.match.l3_match.ip_proto == IP_PROTO_UDP) ||
                   (rule->attrs.match.l3_match.ip_proto == IP_PROTO_TCP)) {
            rule_str +=
                    "sport [" +
                    std::to_string(rule->attrs.match.l4_match.sport_range.port_lo) + "-" +
                    std::to_string(rule->attrs.match.l4_match.sport_range.port_hi) + "], ";
            rule_str +=
                    "dport [" +
                    std::to_string(rule->attrs.match.l4_match.dport_range.port_lo) + "-" +
                    std::to_string(rule->attrs.match.l4_match.dport_range.port_hi) + "]) ";
        }
    }

    rule_str += "action = ";
    if (rule->attrs.action_data.fw_action.action ==
            SECURITY_RULE_ACTION_ALLOW) {
        rule_str += "A";
    } else if (rule->attrs.action_data.fw_action.action ==
                   SECURITY_RULE_ACTION_DENY) {
        rule_str += "D";
    } else {
        rule_str += "U";
    }
    rule_str += ", prio = " + std::to_string(rule->attrs.priority);
    PDS_TRACE_DEBUG_NO_HEADER("%s", rule_str.c_str());
}

static inline void
rfc_p0_eq_class_tables_dump (rfc_ctxt_t *rfc_ctxt)
{
    PDS_TRACE_DEBUG("RFC P0 SIP prefix interval tree dump :");
    rfc_itree_dump(&rfc_ctxt->sip_tree,
                   (rfc_ctxt->policy->af == IP_AF_IPV4) ?
                       ITREE_TYPE_IPV4_SIP_ACL : ITREE_TYPE_IPV6_SIP_ACL);
    PDS_TRACE_DEBUG("RFC P0 SIP prefix tree equivalence class table dump :");
    rfc_eq_class_table_dump(rfc_ctxt->sip_tree.rfc_table);

    PDS_TRACE_DEBUG("RFC P0 STAG interval tree dump :");
    rfc_itree_dump(&rfc_ctxt->stag_tree, ITREE_TYPE_STAG);
    PDS_TRACE_DEBUG("RFC P0 STAG equivalence class table dump :");
    rfc_eq_class_table_dump(rfc_ctxt->stag_tree.rfc_table);

    PDS_TRACE_DEBUG("RFC P0 DIP prefix interval tree dump :");
    rfc_itree_dump(&rfc_ctxt->dip_tree,
                   (rfc_ctxt->policy->af == IP_AF_IPV4) ?
                       ITREE_TYPE_IPV4_DIP_ACL : ITREE_TYPE_IPV6_DIP_ACL);
    PDS_TRACE_DEBUG("RFC P0 DIP prefix tree equivalence class table dump :");
    rfc_eq_class_table_dump(rfc_ctxt->dip_tree.rfc_table);

    PDS_TRACE_DEBUG("RFC P0 DTAG interval tree dump :");
    rfc_itree_dump(&rfc_ctxt->dtag_tree, ITREE_TYPE_DTAG);
    PDS_TRACE_DEBUG("RFC P0 DTAG equivalence class table dump :");
    rfc_eq_class_table_dump(rfc_ctxt->dtag_tree.rfc_table);

    PDS_TRACE_DEBUG("RFC P0 port interval tree dump :");
    rfc_itree_dump(&rfc_ctxt->port_tree, ITREE_TYPE_PORT);
    PDS_TRACE_DEBUG("RFC P0 port tree equivalence class table dump :");
    rfc_eq_class_table_dump(rfc_ctxt->port_tree.rfc_table);

    PDS_TRACE_DEBUG("RFC P0 (proto, port) interval tree dump :");
    rfc_itree_dump(&rfc_ctxt->proto_port_tree, ITREE_TYPE_PROTO_PORT);
    PDS_TRACE_DEBUG("RFC P0 (proto, port) tree equivalence class table dump :");
    rfc_eq_class_table_dump(rfc_ctxt->proto_port_tree.rfc_table);
}

sdk_ret_t
rfc_tree_reserve_default_classid (rfc_ctxt_t *rfc_ctxt, rfc_table_t *rfc_table)
{
    uint8_t       *bits;
    uint16_t      class_id;
    rte_bitmap    *cbm_new;

    // PDS_IMPL_DEFAULT_CLASSID has to be defined to be 0.
    if (rfc_table->num_classes == 0) {

        // allocate memory for bitmap, and initialize
        posix_memalign((void **) &bits, CACHE_LINE_SIZE, rfc_ctxt->cbm_size);
        cbm_new = rte_bitmap_init(rfc_ctxt->policy->max_rules, bits,
                                  rfc_ctxt->cbm_size);

        if (cbm_new == NULL) {
            PDS_TRACE_ERR("cant allocate memory for bitmap");
            return SDK_RET_OOM;
        }

        class_id = rfc_table->num_classes++;
        PDS_TRACE_VERBOSE("default class id allocated is %u", class_id);
        rfc_table->cbm_table[class_id].class_id = class_id;
        rfc_table->cbm_table[class_id].cbm = cbm_new;
        rfc_table->cbm_map[cbm_new] = class_id;
    }

    return SDK_RET_OK;
}

sdk_ret_t
rfc_policy_reserve_default_classids (rfc_ctxt_t *rfc_ctxt)
{
    sdk_ret_t ret;

    if (rfc_ctxt->sip_tree.num_intervals == 0) {
        ret = rfc_tree_reserve_default_classid(rfc_ctxt,
                                               rfc_ctxt->sip_tree.rfc_table);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    if (rfc_ctxt->dip_tree.num_intervals == 0) {
        ret = rfc_tree_reserve_default_classid(rfc_ctxt,
                                               rfc_ctxt->dip_tree.rfc_table);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    if (rfc_ctxt->stag_tree.num_intervals == 0) {
        ret = rfc_tree_reserve_default_classid(rfc_ctxt,
                                               rfc_ctxt->stag_tree.rfc_table);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    if (rfc_ctxt->dtag_tree.num_intervals == 0) {
        ret = rfc_tree_reserve_default_classid(rfc_ctxt,
                                               rfc_ctxt->dtag_tree.rfc_table);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    if (rfc_ctxt->port_tree.num_intervals == 0) {
        ret = rfc_tree_reserve_default_classid(rfc_ctxt,
                                               rfc_ctxt->port_tree.rfc_table);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    if (rfc_ctxt->proto_port_tree.num_intervals == 0) {
        ret = rfc_tree_reserve_default_classid(rfc_ctxt,
                                               rfc_ctxt->proto_port_tree.rfc_table);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    return SDK_RET_OK;
}

sdk_ret_t
rfc_policy_create (policy_params_t *policy_params)

{
    sdk_ret_t     ret;
    rfc_ctxt_t    rfc_ctxt;

    if (policy_params->policy.num_rules > policy_params->policy.max_rules) {
        return sdk::SDK_RET_NO_RESOURCE;
    }

    ///< allocate memory for all the RFC itree tables
    ret = rfc_ctxt_init(&rfc_ctxt, policy_params);
    if (ret != SDK_RET_OK) {
        goto cleanup;
    }

    ///< reserve default classids for each tree to represent no rule match
    ret = rfc_policy_reserve_default_classids(&rfc_ctxt);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to add default rules, err %u", ret);
        goto cleanup;
    }

    ///< build all the interval trees with the given policy
    rfc_build_itables(&rfc_ctxt);

    ///< sort intervals in all the trees
    rfc_sort_itables(&rfc_ctxt);

    ///< compute equivalence classes for the intervals in the interval trees
    rfc_compute_p0_classes(&rfc_ctxt);
    rfc_p0_eq_class_tables_dump(&rfc_ctxt);

    ///< build LPM trees for phase 0 of RFC
    ret = rfc_build_lpm_trees(&rfc_ctxt, policy_params->rfc_tree_root_addr,
                              policy_params->rfc_mem_size);
    if (ret != SDK_RET_OK) {
        PDS_TRACE_ERR("Failed to build RFC LPM trees, err %u", ret);
        goto cleanup;
    }

    // build equivalence class index tables and result tables for subsequent
    // phases of RFC
    rfc_build_eqtables(&rfc_ctxt);

cleanup:

    ///< free all the temporary state
    rfc_ctxt_destroy(&rfc_ctxt);

    return ret;
}

}    // namespace rfc
