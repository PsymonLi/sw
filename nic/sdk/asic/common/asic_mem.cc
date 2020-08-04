//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Asic memory handling utility implementation
///
//----------------------------------------------------------------------------

#include "lib/pal/pal.hpp"
#include "asic/common/asic_mem.hpp"
#include "asic/rw/asicrw.hpp"
#include "asic/common/asic_state.hpp"

uint8_t zero_kb[1024] = { 0 };

namespace sdk {
namespace asic {

static mpartition *
get_mem_partition (void)
{
    return g_asic_state->mempartition();
}

asic_cfg_t *get_asic_cfg (void)
{
    return g_asic_state->asic_cfg();
}

mem_addr_t
asic_get_mem_base (void)
{
    return get_mem_partition()->base();
}

mem_addr_t
asic_get_mem_offset (const char *reg_name)
{
    return get_mem_partition()->start_offset(reg_name);
}

mem_addr_t
asic_get_mem_addr (const char *reg_name)
{
    return get_mem_partition()->start_addr(reg_name);
}

uint32_t
asic_get_mem_size_kb (const char *reg_name)
{
    return (get_mem_partition()->size(reg_name) >> 10);
}

mpartition_region_t *
asic_get_mem_region (char *reg_name)
{
    return get_mem_partition()->region(reg_name);
}

mpartition_region_t *
asic_get_hbm_region_by_address (uint64_t addr)
{
    return get_mem_partition()->region_by_address(addr);
}

void
asic_mem_reset (mem_addr_t pa, uint64_t size)
{
    int64_t rem;
    mem_addr_t va;

    va = (mem_addr_t)sdk::lib::pal_mem_map(pa, size);
    if (va) {
        memset((void *)va, 0, size);
        sdk::lib::pal_mem_unmap((void *)va);
    } else {
        rem = size;
        while (rem > 0) {
            sdk::asic::asic_mem_write(pa, zero_kb,
                                      ((uint64_t)rem > sizeof(zero_kb)) ?
                                          sizeof(zero_kb) : rem);
            pa += sizeof(zero_kb);
            rem -= sizeof(zero_kb);
        }
    }
}

void
asic_reset_mem_region (mpartition_region_t *reg)
{
    mem_addr_t pa;

    SDK_TRACE_VERBOSE("Resetting %s memory region of size %lu",
                      reg->mem_reg_name, reg->size);
    pa = get_mem_partition()->addr(reg->start_offset);
    asic_mem_reset(pa, reg->size);
    SDK_TRACE_VERBOSE("Resetting %s memory region done", reg->mem_reg_name);
}


// for HW platform this is now done during uboot
void
asic_reset_hbm_regions (asic_cfg_t *asic_cfg)
{
    mpartition_region_t *reg;
    bool upgrade_init =
        sdk::platform::sysinit_mode_default(asic_cfg->init_mode) ? false : true;

    if (!asic_cfg)
        return;

    if (!upgrade_init) {
        if (asic_cfg->platform != platform_type_t::PLATFORM_TYPE_HAPS &&
            asic_cfg->platform != platform_type_t::PLATFORM_TYPE_HW) {
            return;
        }
    }

    for (int i = 0; i < get_mem_partition()->num_regions(); i++) {
        reg = get_mem_partition()->region(i);
        if (reg->reset) {
            asic_reset_mem_region(reg);
        }
    }
}

}    // asic
}    // sdk
