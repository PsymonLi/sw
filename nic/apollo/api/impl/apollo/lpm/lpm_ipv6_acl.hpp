/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    lpm_ipv6_acl.hpp
 *
 * @brief   LPM library
 */

#if !defined (__LPM_IPV6_ACL_HPP__)
#define __LPM_IPV6_ACL_HPP__

#include "nic/apollo/api/impl/lpm/lpm.hpp"

/**< key size is 16 bytes for IPv6 acl */
#define LPM_IPV6_ACL_KEY_SIZE    (16)

sdk_ret_t lpm_ipv6_acl_add_key_to_stage(uint8_t *bytes, uint32_t idx,
                                        lpm_inode_t *lpm_inode);
sdk_ret_t lpm_ipv6_acl_set_default_data(uint8_t *bytes,
                                        uint32_t default_data);
sdk_ret_t lpm_ipv6_acl_add_key_to_last_stage(uint8_t *bytes, uint32_t idx,
                                             lpm_inode_t *lpm_inode);
sdk_ret_t lpm_ipv6_acl_write_stage_table(mem_addr_t addr, uint8_t *bytes);
sdk_ret_t lpm_ipv6_acl_write_last_stage_table(mem_addr_t addr, uint8_t *bytes);
uint32_t lpm_ipv6_acl_key_size(void);
uint32_t lpm_ipv6_acl_stages(uint32_t num_intrvls);

#endif    // __LPM_IPV6_ACL_HPP__
