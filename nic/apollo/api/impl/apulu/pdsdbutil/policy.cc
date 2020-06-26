//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This module defines routines to display policy object by pdsdbutil
///
//----------------------------------------------------------------------------

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include "nic/apollo/api/include/pds_policy.hpp"
#include "nic/apollo/api/impl/apulu/pdsdbutil/pdsdbutil.hpp"

static int g_total_policies = 0;
static long int g_total_rules = 0;

static inline void
pds_print_policy_legend (void)
{
    printf("ICMP T/C : ICMP Type/Code\n\n");
}

static inline void
pds_print_policy_summary (void)
{
    printf("\nNo. of security policies : %u\n", g_total_policies);
    printf("Total no. of rules: %lu\n\n", g_total_rules);
}

static inline void
pds_print_policy_header (void)
{
    string hdr_line = string(203, '-');
    printf("%-203s\n", hdr_line.c_str());
    printf("%-40s%-9s%-48s%-48s%-12s%-12s%-8s%-10s%-10s%-6s\n",
           "RuleID", "IPProto", "        Source", "     Destination",
           "SrcPort", "DestPort", "ICMP", "Priority", "Stateful", "Action");
     printf("%-40s%-9s%-48s%-48s%-12s%-12s%-8s%-10s%-10s%-6s\n",
            "", "", "Prefix | Range | Tag", "Prefix | Range | Tag",
            "Range", "Range", "T/C", "", "", "");
    printf("%-203s\n", hdr_line.c_str());
}

static inline void
pds_print_policy (void *key_, void *rule_info_, void *ctxt) {
    uuid    *uuid_ = (uuid *)ctxt;
    uuid    rule_uuid;
    rule_t  *rule = NULL;
    string  src_ip_str, dst_ip_str, src_ip_str2, dst_ip_str2;
    string  src_port_str, dst_port_str;
    string  action_str, proto_str, icmp_str, rule_id_str;
    bool    need_second_line;
    pds_obj_key_t   *key;
    rule_l3_match_t *l3_match;
    rule_l4_match_t *l4_match;
    rule_info_t     *rule_info = (rule_info_t *)rule_info_;

    if (!rule_info) {
        return;
    }

    g_total_policies++;
    g_total_rules += rule_info->num_rules;

    if (uuid_->is_nil()) {
        key = (pds_obj_key_t *)key_;
        printf("%-18s : %-40s\n", "Policy ID", key->str());
    } else {
        printf("%-18s : %-40s\n", "Policy ID", to_string(*uuid_).c_str());
    }

    printf("%-18s : %-10s\n", "Address Family", ipaf2str(rule_info->af));

    switch (rule_info->default_action.fw_action.action) {
    case SECURITY_RULE_ACTION_ALLOW:
        printf("%-18s : %-20s\n", "Default FW Action", "ALLOW");
        break;
    case SECURITY_RULE_ACTION_DENY:
        printf("%-18s : %-20s\n", "Default FW Action", "DENY");
        break;
    case SECURITY_RULE_ACTION_NONE:
        printf("%-18s : %-20s\n", "Default FW Action", "NONE");
        break;
    }

    pds_print_policy_header();

    for (uint32_t i = 0; i < rule_info->num_rules; i++) {
        rule = &rule_info->rules[i];
        l3_match = &rule->attrs.match.l3_match;
        l4_match = &rule->attrs.match.l4_match;

        proto_str = "-";
        src_port_str = "*";
        dst_port_str = "*";
        switch (l3_match->proto_match_type) {
        case MATCH_SPECIFIC :
            proto_str = to_string(l3_match->ip_proto);
            break;
        case MATCH_ANY:
            proto_str = "*";
            src_port_str = "-";
            dst_port_str = "-";
            break;
        }

        src_ip_str = "*";
        src_ip_str2 = "";
        need_second_line = false;
        switch (l3_match->src_match_type) {
        case IP_MATCH_PREFIX:
            src_ip_str = ippfx2str(&l3_match->src_ip_pfx);
            break;
        case IP_MATCH_RANGE:
            src_ip_str = ipvxrange2str(&l3_match->src_ip_range);
            if (src_ip_str.length() > 47) {
                if (l3_match->src_ip_range.af == IP_AF_IPV4) {
                    src_ip_str = ipv4addr2str(l3_match->src_ip_range.ip_lo.v4_addr);
                    src_ip_str2 = ipv4addr2str(l3_match->src_ip_range.ip_hi.v4_addr);
                } else if (l3_match->src_ip_range.af == IP_AF_IPV6) {
                    src_ip_str = ipv6addr2str(l3_match->src_ip_range.ip_lo.v6_addr);
                    src_ip_str2 = ipv6addr2str(l3_match->src_ip_range.ip_hi.v6_addr);
                }
                need_second_line = true;
            }
            break;
        case IP_MATCH_TAG:
            src_ip_str = to_string(l3_match->src_tag);
            break;

        case IP_MATCH_NONE:
        default:
            break;
        }

        dst_ip_str = "*";
        dst_ip_str2 = "";
        switch (l3_match->dst_match_type) {
        case IP_MATCH_PREFIX:
            dst_ip_str = ippfx2str(&l3_match->dst_ip_pfx);
            break;
        case IP_MATCH_RANGE:
            dst_ip_str = ipvxrange2str(&l3_match->dst_ip_range);
            if (dst_ip_str.length() > 47) {
                if (l3_match->dst_ip_range.af == IP_AF_IPV4) {
                    dst_ip_str = ipv4addr2str(l3_match->dst_ip_range.ip_lo.v4_addr);
                    dst_ip_str2 = ipv4addr2str(l3_match->dst_ip_range.ip_hi.v4_addr);
                } else if (l3_match->dst_ip_range.af == IP_AF_IPV4) {
                    dst_ip_str = ipv6addr2str(l3_match->dst_ip_range.ip_lo.v6_addr);
                    dst_ip_str2 = ipv6addr2str(l3_match->dst_ip_range.ip_hi.v6_addr);
                }
                need_second_line = true;
            }
            break;
        case IP_MATCH_TAG:
            dst_ip_str = to_string(l3_match->dst_tag);
            break;
        case IP_MATCH_NONE:
        default:
            break;
        }

        icmp_str = "-";
        if (l3_match->ip_proto != IP_PROTO_ICMP) {
             src_port_str = to_string(l4_match->sport_range.port_lo);
             src_port_str += "-";
             src_port_str += to_string(l4_match->sport_range.port_hi);
             dst_port_str = to_string(l4_match->dport_range.port_lo);
             dst_port_str += "-";
             dst_port_str += to_string(l4_match->dport_range.port_hi);
        } else {
            icmp_str = "-";
            if (l4_match->type_match_type == MATCH_SPECIFIC) {
                icmp_str = to_string(l4_match->icmp_type);
            } else if (l4_match->type_match_type == MATCH_ANY) {
                icmp_str = "*";
            }

            icmp_str += "/";
            if (l4_match->code_match_type == MATCH_SPECIFIC) {
                icmp_str = to_string(l4_match->icmp_code);
            } else if (l4_match->code_match_type == MATCH_ANY) {
                icmp_str = "*";
            }
        }

        switch (rule->attrs.action_data.fw_action.action) {
        case SECURITY_RULE_ACTION_ALLOW:
            action_str = "A";
            break;
        case SECURITY_RULE_ACTION_DENY:
            action_str = "D";
            break;
        default:
            action_str = "-";
            break;
        }

        rule_id_str = "-";
        rule_uuid = string_generator()(string(rule->key.str()));
        if (!rule_uuid.is_nil()) {
            rule_id_str = to_string(rule_uuid);
        }

        if (need_second_line) {
            printf("%-40s%-9s%-48s%-48s%-12s%-12s%-8s%-10d%-10s%-6s\n",
                   rule_id_str.c_str(), proto_str.c_str(),
                   (src_ip_str + " -").c_str(), (dst_ip_str + " -").c_str(),
                   src_port_str.c_str(), dst_port_str.c_str(), icmp_str.c_str(),
                   rule->attrs.priority, rule->attrs.stateful ? "T" : "F",
                   action_str.c_str());
            printf("%-40s%-9s%-48s%-48s%-50s\n",
                    "", "", src_ip_str2.c_str(), dst_ip_str2.c_str(), "");
        } else {
            printf("%-40s%-9s%-48s%-48s%-12s%-12s%-8s%-10d%-10s%-6s\n",
                   rule_id_str.c_str(), proto_str.c_str(),
                   src_ip_str.c_str(), dst_ip_str.c_str(),
                   src_port_str.c_str(), dst_port_str.c_str(), icmp_str.c_str(),
                   rule->attrs.priority, rule->attrs.stateful ? "T" : "F",
                   action_str.c_str());
         }
     }
}

sdk_ret_t
pds_get_policy (uuid *uuid)
{
    string prefix_match = "policy";
    sdk_ret_t ret;

    g_total_policies = 0;
    g_total_rules = 0;

    if (uuid && !uuid->is_nil()) {
        prefix_match += ":";
        prefix_match += string((char *)uuid, uuid->size());
    }
    pds_print_policy_legend();
    ret = g_kvstore->iterate(pds_print_policy, uuid, prefix_match);
    pds_print_policy_summary();
    return ret;
}
