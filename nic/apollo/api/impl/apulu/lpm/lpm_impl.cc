//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// LPM API implementation provided by apulu pipeline
///
//----------------------------------------------------------------------------

#include <math.h>
#include "nic/apollo/core/trace.hpp"
#include "nic/apollo/api/impl/lpm/lpm.hpp"
#include "nic/apollo/api/impl/lpm/lpm_stag.hpp"
#include "nic/apollo/api/impl/lpm/lpm_dtag.hpp"
#include "nic/apollo/api/impl/lpm/lpm_priv.hpp"
#include "nic/apollo/api/impl/lpm/lpm_sport.hpp"
#include "nic/apollo/api/impl/lpm/lpm_ipv4_acl.hpp"
#include "nic/apollo/api/impl/lpm/lpm_ipv6_acl.hpp"
#include "nic/apollo/api/impl/lpm/lpm_ipv4_sip.hpp"
#include "nic/apollo/api/impl/lpm/lpm_ipv6_sip.hpp"
#include "nic/apollo/api/impl/lpm/lpm_ipv4_route.hpp"
#include "nic/apollo/api/impl/lpm/lpm_ipv6_route.hpp"
#include "nic/apollo/api/impl/lpm/lpm_proto_dport.hpp"

/**
 * key size is 4 bytes for IPv4, 8 bytes (assuming max prefix length of 64)
 * for IPv6, 2 bytes for port tree and 4 bytes for proto-port tree
 */
uint32_t
lpm_entry_key_size (itree_type_t tree_type)
{
    switch (tree_type) {
    case ITREE_TYPE_IPV4:
        return lpm_ipv4_route_key_size();
    case ITREE_TYPE_IPV6:
        return lpm_ipv6_route_key_size();
    case ITREE_TYPE_PORT:
        return lpm_sport_key_size();
    case ITREE_TYPE_PROTO_PORT:
        return lpm_proto_dport_key_size();
    case ITREE_TYPE_IPV4_DIP_ACL:
        return lpm_ipv4_acl_key_size();
    case ITREE_TYPE_IPV6_DIP_ACL:
        return lpm_ipv6_acl_key_size();
    case ITREE_TYPE_IPV4_SIP_ACL:
        return lpm_ipv4_sip_key_size();
    case ITREE_TYPE_IPV6_SIP_ACL:
        return lpm_ipv6_sip_key_size();
    case ITREE_TYPE_STAG:
        return lpm_stag_key_size();
    case ITREE_TYPE_DTAG:
        return lpm_dtag_key_size();
    default:
        SDK_ASSERT(0);
        break;
    }
    return 0;
}

/**
 * number of keys per table is CACHE_LINE_SIZE/(sizeof(ipv4_addr_t) = 16 for
 * IPv4 and CACHE_LINE_SIZE/8 = 8 for IPv6 (assuming max prefix length of 64)
 * NOTE: this doesn't apply to last stage where each node also has 2 byte
 * nexthop
 */
uint32_t
lpm_keys_per_table (itree_type_t tree_type)
{
    switch (tree_type) {
    case ITREE_TYPE_IPV4:
        return ((LPM_TABLE_SIZE / lpm_ipv4_route_key_size()) - 1);
    case ITREE_TYPE_IPV6:
        return ((LPM_TABLE_SIZE / lpm_ipv6_route_key_size()) - 1);
    case ITREE_TYPE_PORT:
        return ((LPM_TABLE_SIZE / lpm_sport_key_size()) - 1);
    case ITREE_TYPE_PROTO_PORT:
        return ((LPM_TABLE_SIZE / lpm_proto_dport_key_size()) - 1);
    case ITREE_TYPE_IPV4_DIP_ACL:
        return ((LPM_TABLE_SIZE / lpm_ipv4_acl_key_size()) - 1);
    case ITREE_TYPE_IPV6_DIP_ACL:
        return ((LPM_TABLE_SIZE / lpm_ipv6_acl_key_size()) - 1);
    case ITREE_TYPE_IPV4_SIP_ACL:
        return ((LPM_TABLE_SIZE / lpm_ipv4_sip_key_size()) - 1);
    case ITREE_TYPE_IPV6_SIP_ACL:
        return ((LPM_TABLE_SIZE / lpm_ipv6_sip_key_size()) - 1);
    case ITREE_TYPE_STAG:
        return ((LPM_TABLE_SIZE / lpm_stag_key_size()) - 1);
    case ITREE_TYPE_DTAG:
        return ((LPM_TABLE_SIZE / lpm_dtag_key_size()) - 1);
    default:
        SDK_ASSERT(0);
        break;
    }
    return 0;
}

/**
 * @brief    compute the number of stages needed for LPM lookup given the
 *           interval table scale
 * @param[in]    tree_type     type of the tree being built
 * @param[in]    num_intrvls   number of intervals in the interval table
 * @return       number of lookup stages (aka. depth of the interval tree)
 *
 * for IPv6:
 *     The last stage gives an 4-way decision. The other stages each give
 *     a 8-way decision.
 *     #stages = 1 + log8(num_intrvls/4.0)
 *             = 1 + log2(num_intrvls/4.0)/log2(8)
 *             = 1 + log2(num_intrvls/4.0)/3.0
 */
uint32_t
lpm_stages (itree_type_t tree_type, uint32_t num_intrvls)
{
    switch (tree_type) {
    case ITREE_TYPE_IPV4:
        return lpm_ipv4_route_stages(num_intrvls);
    case ITREE_TYPE_IPV6:
        return lpm_ipv6_route_stages(num_intrvls);
    case ITREE_TYPE_PORT:
        return lpm_sport_stages(num_intrvls);
    case ITREE_TYPE_PROTO_PORT:
        return lpm_proto_dport_stages(num_intrvls);
    case ITREE_TYPE_IPV4_DIP_ACL:
        return lpm_ipv4_acl_stages(num_intrvls);
    case ITREE_TYPE_IPV6_DIP_ACL:
        return lpm_ipv6_acl_stages(num_intrvls);
    case ITREE_TYPE_IPV4_SIP_ACL:
        return lpm_ipv4_sip_stages(num_intrvls);
    case ITREE_TYPE_IPV6_SIP_ACL:
        return lpm_ipv6_sip_stages(num_intrvls);
    case ITREE_TYPE_STAG:
        return lpm_stag_stages(num_intrvls);
    case ITREE_TYPE_DTAG:
        return lpm_dtag_stages(num_intrvls);
    default:
        SDK_ASSERT(0);
        break;
    }
    return 0;
}

sdk_ret_t
lpm_add_key_to_stage (itree_type_t tree_type, lpm_stage_info_t *stage,
                      lpm_inode_t *lpm_inode)
{
    switch (tree_type) {
    case ITREE_TYPE_IPV4:
        lpm_ipv4_route_add_key_to_stage(stage->curr_table,
                                        stage->curr_index,
                                        lpm_inode);
        break;
    case ITREE_TYPE_IPV6:
        lpm_ipv6_route_add_key_to_stage(stage->curr_table,
                                        stage->curr_index,
                                        lpm_inode);
        break;
    case ITREE_TYPE_PORT:
        lpm_sport_add_key_to_stage(stage->curr_table,
                                   stage->curr_index,
                                   lpm_inode);
        break;
    case ITREE_TYPE_PROTO_PORT:
        lpm_proto_dport_add_key_to_stage(stage->curr_table,
                                         stage->curr_index,
                                         lpm_inode);
        break;
    case ITREE_TYPE_IPV4_DIP_ACL:
        lpm_ipv4_acl_add_key_to_stage(stage->curr_table,
                                      stage->curr_index,
                                      lpm_inode);
        break;
    case ITREE_TYPE_IPV6_DIP_ACL:
        lpm_ipv6_acl_add_key_to_stage(stage->curr_table,
                                      stage->curr_index,
                                      lpm_inode);
        break;
    case ITREE_TYPE_IPV4_SIP_ACL:
        lpm_ipv4_sip_add_key_to_stage(stage->curr_table,
                                      stage->curr_index,
                                      lpm_inode);
        break;
    case ITREE_TYPE_IPV6_SIP_ACL:
        lpm_ipv6_sip_add_key_to_stage(stage->curr_table,
                                      stage->curr_index,
                                      lpm_inode);
        break;
    case ITREE_TYPE_STAG:
        lpm_stag_add_key_to_stage(stage->curr_table,
                                  stage->curr_index,
                                  lpm_inode);
        break;
    case ITREE_TYPE_DTAG:
        lpm_dtag_add_key_to_stage(stage->curr_table,
                                  stage->curr_index,
                                  lpm_inode);
        break;
    default:
        SDK_ASSERT(0);
        break;
    }
    stage->curr_index++;
    return SDK_RET_OK;
}

sdk_ret_t
lpm_write_stage_table (itree_type_t tree_type, lpm_stage_info_t *stage)
{
    switch (tree_type) {
    case ITREE_TYPE_IPV4:
        lpm_ipv4_route_write_stage_table(stage->curr_table_addr,
                                         stage->curr_table);
        break;
    case ITREE_TYPE_IPV6:
        lpm_ipv6_route_write_stage_table(stage->curr_table_addr,
                                         stage->curr_table);
        break;
    case ITREE_TYPE_PORT:
        lpm_sport_write_stage_table(stage->curr_table_addr,
                                    stage->curr_table);
        break;
    case ITREE_TYPE_PROTO_PORT:
        lpm_proto_dport_write_stage_table(stage->curr_table_addr,
                                          stage->curr_table);
        break;
    case ITREE_TYPE_IPV4_DIP_ACL:
        lpm_ipv4_acl_write_stage_table(stage->curr_table_addr,
                                       stage->curr_table);
        break;
    case ITREE_TYPE_IPV6_DIP_ACL:
        lpm_ipv6_acl_write_stage_table(stage->curr_table_addr,
                                       stage->curr_table);
        break;
    case ITREE_TYPE_IPV4_SIP_ACL:
        lpm_ipv4_sip_write_stage_table(stage->curr_table_addr,
                                       stage->curr_table);
        break;
    case ITREE_TYPE_IPV6_SIP_ACL:
        lpm_ipv6_sip_write_stage_table(stage->curr_table_addr,
                                       stage->curr_table);
        break;
    case ITREE_TYPE_STAG:
        lpm_stag_write_stage_table(stage->curr_table_addr,
                                   stage->curr_table);
        break;
    case ITREE_TYPE_DTAG:
        lpm_dtag_write_stage_table(stage->curr_table_addr,
                                   stage->curr_table);
        break;
    default:
        SDK_ASSERT(0);
        break;
    }
    /**< update this stage meta for next time */
    stage->curr_index = 0;
    stage->curr_table_addr += LPM_TABLE_SIZE;
    memset(stage->curr_table, 0xFF, LPM_TABLE_SIZE);
    return SDK_RET_OK;
}

sdk_ret_t
lpm_add_key_to_last_stage (itree_type_t tree_type, lpm_stage_info_t *stage,
                           lpm_inode_t *lpm_inode)
{
    switch (tree_type) {
    case ITREE_TYPE_IPV4:
        lpm_ipv4_route_add_key_to_last_stage(stage->curr_table,
                                             stage->curr_index,
                                             lpm_inode);
        break;
    case ITREE_TYPE_IPV6:
        lpm_ipv6_route_add_key_to_last_stage(stage->curr_table,
                                             stage->curr_index,
                                             lpm_inode);
        break;
    case ITREE_TYPE_PORT:
        lpm_sport_add_key_to_last_stage(stage->curr_table,
                                        stage->curr_index,
                                        lpm_inode);
        break;
    case ITREE_TYPE_PROTO_PORT:
        lpm_proto_dport_add_key_to_last_stage(stage->curr_table,
                                              stage->curr_index,
                                              lpm_inode);
        break;
    case ITREE_TYPE_IPV4_DIP_ACL:
        lpm_ipv4_acl_add_key_to_last_stage(stage->curr_table,
                                           stage->curr_index,
                                           lpm_inode);
        break;
    case ITREE_TYPE_IPV6_DIP_ACL:
        lpm_ipv6_acl_add_key_to_last_stage(stage->curr_table,
                                           stage->curr_index,
                                           lpm_inode);
        break;
    case ITREE_TYPE_IPV4_SIP_ACL:
        lpm_ipv4_sip_add_key_to_last_stage(stage->curr_table,
                                           stage->curr_index,
                                           lpm_inode);
        break;
    case ITREE_TYPE_IPV6_SIP_ACL:
        lpm_ipv6_sip_add_key_to_last_stage(stage->curr_table,
                                           stage->curr_index,
                                           lpm_inode);
        break;
    case ITREE_TYPE_STAG:
        lpm_stag_add_key_to_last_stage(stage->curr_table,
                                       stage->curr_index,
                                       lpm_inode);
        break;
    case ITREE_TYPE_DTAG:
        lpm_dtag_add_key_to_last_stage(stage->curr_table,
                                       stage->curr_index,
                                       lpm_inode);
        break;
    default:
        SDK_ASSERT(0);
        break;
    }
    stage->curr_index++;
    return SDK_RET_OK;
}

sdk_ret_t
lpm_set_default_data (itree_type_t tree_type, lpm_stage_info_t *stage,
                      uint32_t default_data)
{
    switch (tree_type) {
    case ITREE_TYPE_IPV4:
        lpm_ipv4_route_set_default_data(stage->curr_table, default_data);
        break;
    case ITREE_TYPE_IPV6:
        lpm_ipv6_route_set_default_data(stage->curr_table, default_data);
        break;
    case ITREE_TYPE_PORT:
        lpm_sport_set_default_data(stage->curr_table, default_data);
        break;
    case ITREE_TYPE_PROTO_PORT:
        lpm_proto_dport_set_default_data(stage->curr_table, default_data);
        break;
    case ITREE_TYPE_IPV4_DIP_ACL:
        lpm_ipv4_acl_set_default_data(stage->curr_table, default_data);
        break;
    case ITREE_TYPE_IPV6_DIP_ACL:
        lpm_ipv6_acl_set_default_data(stage->curr_table, default_data);
        break;
    case ITREE_TYPE_IPV4_SIP_ACL:
        lpm_ipv4_sip_set_default_data(stage->curr_table, default_data);
        break;
    case ITREE_TYPE_IPV6_SIP_ACL:
        lpm_ipv6_sip_set_default_data(stage->curr_table, default_data);
        break;
    case ITREE_TYPE_STAG:
        lpm_stag_set_default_data(stage->curr_table, default_data);
        break;
    case ITREE_TYPE_DTAG:
        lpm_dtag_set_default_data(stage->curr_table, default_data);
        break;
    default:
        SDK_ASSERT(0);
        break;
    }
    return SDK_RET_OK;
}

sdk_ret_t
lpm_write_last_stage_table (itree_type_t tree_type, lpm_stage_info_t *stage)
{
    switch (tree_type) {
    case ITREE_TYPE_IPV4:
        lpm_ipv4_route_write_last_stage_table(stage->curr_table_addr,
                                              stage->curr_table);
        break;
    case ITREE_TYPE_IPV6:
        lpm_ipv6_route_write_last_stage_table(stage->curr_table_addr,
                                              stage->curr_table);
        break;
    case ITREE_TYPE_PORT:
        lpm_sport_write_last_stage_table(stage->curr_table_addr,
                                         stage->curr_table);
        break;
    case ITREE_TYPE_PROTO_PORT:
        lpm_proto_dport_write_last_stage_table(stage->curr_table_addr,
                                               stage->curr_table);
        break;
    case ITREE_TYPE_IPV4_DIP_ACL:
        lpm_ipv4_acl_write_last_stage_table(stage->curr_table_addr,
                                            stage->curr_table);
        break;
    case ITREE_TYPE_IPV6_DIP_ACL:
        lpm_ipv6_acl_write_last_stage_table(stage->curr_table_addr,
                                            stage->curr_table);
        break;
    case ITREE_TYPE_IPV4_SIP_ACL:
        lpm_ipv4_sip_write_last_stage_table(stage->curr_table_addr,
                                            stage->curr_table);
        break;
    case ITREE_TYPE_IPV6_SIP_ACL:
        lpm_ipv6_sip_write_last_stage_table(stage->curr_table_addr,
                                            stage->curr_table);
        break;
    case ITREE_TYPE_STAG:
        lpm_stag_write_last_stage_table(stage->curr_table_addr,
                                        stage->curr_table);
        break;
    case ITREE_TYPE_DTAG:
        lpm_dtag_write_last_stage_table(stage->curr_table_addr,
                                        stage->curr_table);
        break;
    default:
        SDK_ASSERT(0);
        break;
    }
    /**< update last stage meta for next time */
    stage->curr_index = 0;
    stage->curr_table_addr += LPM_TABLE_SIZE;
    memset(stage->curr_table, 0xFF, LPM_TABLE_SIZE);
    return SDK_RET_OK;
}
