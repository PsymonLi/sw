// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
/*
 * capri_tbl_rw.hpp
 * Mahesh Shirshyad (Pensando Systems)
 */

#ifndef __CAPRI_TBL_RW_HPP__
#define __CAPRI_TBL_RW_HPP__

#include <stdio.h>
#include <string>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include "include/sdk/base.hpp"
#include "platform/capri/capri_cfg.hpp"
#include "asic/pd/pd.hpp"

namespace sdk {
namespace platform {
namespace capri {

#define CAPRI_P4_NUM_STAGES     6
#define CAPRI_P4PLUS_NUM_STAGES 8

#define CAPRI_OK (0)
#define CAPRI_FAIL (-1)

typedef enum {
    P4PLUS_CACHE_ACTION_NONE        = 0x0,
    P4PLUS_CACHE_INVALIDATE_RXDMA   = 0x1,
    P4PLUS_CACHE_INVALIDATE_TXDMA   = 0x2,
    P4PLUS_CACHE_INVALIDATE_BOTH    = P4PLUS_CACHE_INVALIDATE_RXDMA |
                                      P4PLUS_CACHE_INVALIDATE_TXDMA
} p4plus_cache_action_t;

typedef struct p4plus_prog_s {
    int stageid;
    int stage_tableid;
    int stage_tableid_off;
    const char *control;
    const char *prog_name;
    p4pd_pipeline_t pipe;
} p4plus_prog_t;

int capri_table_rw_init(capri_cfg_t *capri_cfg);
int capri_p4plus_table_rw_init(void);

void capri_table_rw_cleanup();

int capri_table_entry_write(uint32_t tableid,
                            uint32_t index,
                            uint8_t  *hwentry,
                            uint8_t  *hwentry_mask,
                            uint16_t hwentry_bit_len,
                            p4_table_mem_layout_t &tbl_info, int gress,
                            bool is_oflow_table, bool ingress,
                            uint32_t ofl_parent_tbl_depth);

int capri_table_entry_read(uint32_t tableid,
                           uint32_t index,
                           uint8_t  *hwentry,
                           uint16_t *hwentry_bit_len,
                           p4_table_mem_layout_t &tbl_info, int gress,
                           bool is_oflow_table,
                           uint32_t ofl_parent_tbl_depth);

int capri_table_hw_entry_read(uint32_t tableid,
                              uint32_t index,
                              uint8_t  *hwentry,
                              uint16_t *hwentry_bit_len,
                              p4_table_mem_layout_t &tbl_info, int gress,
                              bool is_oflow_table, bool ingress,
                              uint32_t ofl_parent_tbl_depth);

int capri_tcam_table_entry_write (uint32_t tableid,
                                  uint32_t index,
                                  uint8_t  *trit_x,
                                  uint8_t  *trit_y,
                                  uint16_t hwentry_bit_len,
                                  p4_table_mem_layout_t &tbl_info,
                                  int gress, bool ingress);

int capri_tcam_table_entry_read(uint32_t tableid,
                                uint32_t index,
                                uint8_t  *trit_x,
                                uint8_t  *trit_y,
                                uint16_t *hwentry_bit_len,
                                p4_table_mem_layout_t &tbl_info,
                                int gress);

int capri_tcam_table_hw_entry_read(uint32_t tableid,
                                   uint32_t index,
                                   uint8_t  *trit_x,
                                   uint8_t  *trit_y,
                                   uint16_t *hwentry_bit_len,
                                   p4_table_mem_layout_t &tbl_info,
                                   bool ingress);

int capri_hbm_table_entry_write(uint32_t tableid,
                                uint32_t index,
                                uint8_t *hwentry,
                                uint16_t entry_size,
                                p4_table_mem_layout_t &tbl_info);

int capri_hbm_table_entry_cache_invalidate (bool ingress,
                                            uint64_t entry_addr,
                                            p4_table_mem_layout_t &tbl_info);

int capri_hbm_table_entry_read(uint32_t tableid,
                               uint32_t index,
                               uint8_t *hwentry,
                               uint16_t *entry_size,
                                p4_table_mem_layout_t &tbl_info);

int capri_table_constant_write(uint64_t val, uint32_t stage,
                               uint32_t stage_tableid, bool ingress);

int capri_table_constant_read(uint64_t *val, uint32_t stage,
                              int stage_tableid, bool ingress);

void capri_set_action_asm_base(int tableid, int actionid,
                               uint64_t asm_base);

void capri_set_action_rxdma_asm_base(int tableid, int actionid,
                                     uint64_t asm_base);

void capri_set_action_txdma_asm_base(int tableid, int actionid,
                                     uint64_t asm_base);

void capri_set_table_rxdma_asm_base (int tableid, uint64_t asm_base);

void capri_set_table_txdma_asm_base (int tableid, uint64_t asm_base);

void capri_program_p4plus_sram_table_mpu_pc(int tableid, int stage_tbl_id,
                                            int stage);

void capri_program_table_mpu_pc(int tableid, bool gress, int stage, int stage_tableid,
                           uint64_t capri_table_asm_err_offset,
                           uint64_t capri_table_asm_base);

int capri_p4plus_table_init(p4plus_prog_t *prog, platform_type_t platform_type);

int capri_p4plus_table_init(platform_type_t platform_type,
                            int stage_apphdr, int stage_tableid_apphdr,
                            int stage_apphdr_ext, int stage_tableid_apphdr_ext,
                            int stage_apphdr_off, int stage_tableid_apphdr_off,
                            int stage_apphdr_ext_off, int stage_tableid_apphdr_ext_off,
                            int stage_txdma_act, int stage_tableid_txdma_act,
                            int stage_txdma_act_ext, int stage_tableid_txdma_act_ext);

void capri_deparser_init(int tm_port_ingress, int tm_port_egress);

void capri_program_hbm_table_base_addr(int stage_tableid, char *tablename,
                                       int stage, int pipe);

void capri_p4plus_recirc_init();

uint8_t capri_get_action_id(uint32_t tableid, uint8_t actionpc);

uint8_t capri_get_action_pc(uint32_t tableid, uint8_t actionid);

bool p4plus_invalidate_cache(uint64_t addr, uint32_t size_in_bytes,
                             p4plus_cache_action_t action);


} // namespace capri
} // namespace platform
} // namespace sdk

using sdk::platform::capri::p4plus_cache_action_t;
using sdk::platform::capri::p4plus_cache_action_t::P4PLUS_CACHE_ACTION_NONE;
using sdk::platform::capri::p4plus_cache_action_t::P4PLUS_CACHE_INVALIDATE_RXDMA;
using sdk::platform::capri::p4plus_cache_action_t::P4PLUS_CACHE_INVALIDATE_TXDMA;
using sdk::platform::capri::p4plus_cache_action_t::P4PLUS_CACHE_INVALIDATE_BOTH;
using sdk::platform::capri::p4plus_prog_t;

#endif
