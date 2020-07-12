/**
 * Copyright (c) 2020 Pensando Systems, Inc.
 *
 * @file    lpm_dtag.hpp
 *
 * @brief   LPM DTAG Implementation
 */

#if !defined (__LPM_DTAG_HPP__)
#define __LPM_DTAG_HPP__

#include "nic/apollo/api/impl/lpm/lpm.hpp"

#define LPM_DTAG_KEY_SIZE    (4)

sdk_ret_t lpm_dtag_add_key_to_stage(uint8_t *bytes, uint32_t idx,
                                    lpm_inode_t *lpm_inode);
sdk_ret_t lpm_dtag_add_key_to_last_stage(uint8_t *bytes, uint32_t idx,
                                         lpm_inode_t *lpm_inode);
sdk_ret_t lpm_dtag_set_default_data(uint8_t *bytes,
                                    uint32_t default_data);
sdk_ret_t lpm_dtag_write_stage_table(mem_addr_t addr, uint8_t *bytes);
sdk_ret_t lpm_dtag_write_last_stage_table(mem_addr_t addr,
                                          uint8_t *bytes);
uint32_t lpm_dtag_key_size(void);
uint32_t lpm_dtag_stages(uint32_t num_intrvls);

#endif    //__LPM_DTAG_HPP__
