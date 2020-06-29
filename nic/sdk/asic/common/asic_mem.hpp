//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Asic memory handling utility function headers
///
//----------------------------------------------------------------------------

#ifndef __ASIC_MEM_HPP__
#define __ASIC_MEM_HPP__

#include "asic/asic.hpp"
#include "platform/utils/mpartition.hpp"
#include "include/sdk/types.hpp"
#include "asic/rw/asicrw.hpp"

namespace sdk  {
namespace asic {

mem_addr_t asic_get_mem_base(void);
mpartition_region_t *asic_get_mem_region(char *reg_name);
mpartition_region_t *asic_get_hbm_region_by_address(uint64_t addr);
void asic_reset_hbm_regions(asic_cfg_t *asic_cfg);
mem_addr_t asic_get_mem_offset(const char *reg_name);
mem_addr_t asic_get_mem_addr(const char *reg_name);
uint32_t asic_get_mem_size_kb(const char *reg_name);

}    // namespace asic
}    // namespace sdk

#endif    // __ASIC_MEM_HPP__
