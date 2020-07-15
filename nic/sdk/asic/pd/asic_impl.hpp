//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains asic pd internal APIs.
///
//----------------------------------------------------------------------------

#ifndef __SDK_ASIC_ASIC_IMPL_HPP__
#define __SDK_ASIC_ASIC_IMPL_HPP__

#include "include/sdk/base.hpp"

namespace sdk {
namespace asic {
namespace pd {

uint64_t asicpd_impl_get_p4plus_table_mpu_pc(int tableid);
void asicpd_impl_program_p4plus_tbl_mpu_pc(int tableid, int stage_tbl_id, int stage);
void asicpd_impl_program_tbl_mpu_pc(int tableid, bool gress, int stage,
                                    int stage_tableid, uint64_t table_asm_err_offset,
                                    uint64_t table_asm_base);
void asicpd_impl_set_action_asm_base(int tableid, int actionid, uint64_t asm_base);
void asicpd_impl_set_action_rxdma_asm_base(int tableid, int actionid,
                                           uint64_t asm_base);
void asicpd_impl_set_action_txdma_asm_base(int tableid, int actionid,
                                           uint64_t asm_base);
void asicpd_impl_set_table_rxdma_asm_base(int tableid, uint64_t asm_base);
void asicpd_impl_set_table_txdma_asm_base(int tableid, uint64_t asm_base);
sdk_ret_t asicpd_impl_program_tcam_table_offset(int tableid, p4pd_table_dir_en gress,
                                                int stage, int stage_tableid);
                                                
}    // namespace pd
}    // namespace asic
}    // namespace sdk

#endif    // __SDK_ASIC_ASIC_ASIC_IMPL_HPP__
