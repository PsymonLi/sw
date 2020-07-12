/**
 * Copyright (c) 2020 Pensando Systems, Inc.
 *
 * @file    lpm_stag.hpp
 *
 * @brief   LPM STAG Implementation
 */

#if !defined (__LPM_STAG_HPP__)
#define __LPM_STAG_HPP__

#include "nic/apollo/api/impl/lpm/lpm.hpp"

#define LPM_STAG_KEY_SIZE    (4)

sdk_ret_t lpm_stag_add_key_to_stage(uint8_t *bytes, uint32_t idx,
                                    lpm_inode_t *lpm_inode);
sdk_ret_t lpm_stag_add_key_to_last_stage(uint8_t *bytes, uint32_t idx,
                                         lpm_inode_t *lpm_inode);
sdk_ret_t lpm_stag_set_default_data(uint8_t *bytes,
                                    uint32_t default_data);
sdk_ret_t lpm_stag_write_stage_table(mem_addr_t addr, uint8_t *bytes);
sdk_ret_t lpm_stag_write_last_stage_table(mem_addr_t addr,
                                          uint8_t *bytes);
uint32_t lpm_stag_key_size(void);
uint32_t lpm_stag_stages(uint32_t num_intrvls);

#endif    //__LPM_STAG_HPP__
