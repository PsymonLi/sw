/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    lpm_ipv4_tag.hpp
 *
 * @brief   LPM for IPv4 TAG
 */

#if !defined (__LPM_IPV4_TAG_HPP__)
#define __LPM_IPV4_TAG_HPP__

#include "nic/apollo/lpm/lpm.hpp"

/**< key size is 4 bytes for IPv4 Tag */
#define LPM_IPV4_TAG_KEY_SIZE    (4)

sdk_ret_t lpm_ipv4_tag_add_key_to_stage(uint8_t *bytes, uint32_t idx,
                                        lpm_inode_t *lpm_inode);
sdk_ret_t lpm_ipv4_tag_set_default_data(uint8_t *bytes,
                                        uint32_t default_data);
sdk_ret_t lpm_ipv4_tag_add_key_to_last_stage(uint8_t *bytes, uint32_t idx,
                                             lpm_inode_t *lpm_inode);
sdk_ret_t lpm_ipv4_tag_write_stage_table(mem_addr_t addr, uint8_t *bytes);
sdk_ret_t lpm_ipv4_tag_write_last_stage_table(mem_addr_t addr, uint8_t *bytes);
uint32_t lpm_ipv4_tag_key_size (void);
uint32_t lpm_ipv4_tag_stages (uint32_t num_intrvls);

#endif    // __LPM_IPV4_TAG_HPP__
