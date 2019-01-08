//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include <errno.h>
#include <assert.h>
#include "platform/capri/capri_qstate.hpp"
#include "platform/capri/capri_lif_manager.hpp"
#include "nic/sdk/include/sdk/base.hpp"
#include "platform/capri/capri_tbl_rw.hpp"
#include "platform/utils/mpartition.hpp"
#include "asic/rw/asicrw.hpp"

#if 0
void push_qstate_to_capri(hal::LIFQState *qstate);
void clear_qstate(hal::LIFQState *qstate);
int32_t read_qstate(uint64_t q_addr, uint8_t *buf, uint32_t q_size);
int32_t write_qstate(uint64_t q_addr, const uint8_t *buf, uint32_t q_size);
int32_t get_pc_offset(const char *handle, const char *prog_name,
                      const char *label, uint8_t *offset);
#endif

using namespace sdk::platform::utils;


namespace sdk {
namespace platform {
namespace capri {

const static uint32_t kAllocUnit = 4096;

void LIFManager::destroy(LIFManager *lm) {
    assert(0);
    delete lm;
}

LIFManager* LIFManager::factory(mpartition *mp,
                                program_info *pinfo,
                                const char *kHBMLabel) {
    assert(mp);

    LIFManager *lm = new LIFManager();
    assert(lm);

    lm->mp_ = mp;
    lm->pinfo_ = pinfo;

    uint64_t hbm_addr = lm->mp_->start_addr(kHBMLabel);
    assert(hbm_addr > 0);

    uint32_t size = lm->mp_->size(kHBMLabel);
    uint32_t num_units = (size) / kAllocUnit;
    if (hbm_addr & 0xFFF) {
        // Not 4K aligned.
        hbm_addr = (hbm_addr + 0xFFFull) & ~0xFFFull;
        num_units--;
    }

    lm->hbm_base_ = hbm_addr;
    lm->hbm_allocator_.reset(new sdk::lib::BMAllocator(num_units));

    // Allocate 0th hw_lif_id
    uint32_t hw_lif_id = lm->LIFRangeAlloc(-1, 1);
    SDK_TRACE_DEBUG("Reserving hw_lif_id: %d", hw_lif_id);

    return lm;
}

int32_t LIFManager::InitLIFQStateImpl(LIFQState *qstate, int cos) {
    uint32_t alloc_units;

    alloc_units = (qstate->allocation_size + kAllocUnit - 1) & ~(kAllocUnit - 1);
    alloc_units /= kAllocUnit;
    int alloc_offset = hbm_allocator_->Alloc(alloc_units);
    if (alloc_offset < 0)
        return -ENOMEM;
    allocation_sizes_[alloc_offset] = alloc_units;
    alloc_offset *= kAllocUnit;
    qstate->hbm_address = hbm_base_ + alloc_offset;

    push_qstate_to_capri(qstate, cos);

    return 0;
}

void LIFManager::DeleteLIFQStateImpl(LIFQState *qstate) {

    clear_qstate(qstate);

    int alloc_offset = qstate->hbm_address - hbm_base_;
    if (allocation_sizes_.find(alloc_offset) != allocation_sizes_.end()) {
        hbm_allocator_->Free(alloc_offset, allocation_sizes_[alloc_offset]);
        allocation_sizes_.erase(alloc_offset);
    }
}

int32_t LIFManager::ReadQStateImpl(
        uint64_t q_addr, uint8_t *buf, uint32_t q_size) {

    read_qstate(q_addr, buf, q_size);

    return 0;
}

int32_t LIFManager::WriteQStateImpl(
        uint64_t q_addr, const uint8_t *buf, uint32_t q_size) {
    p4plus_cache_action_t action = P4PLUS_CACHE_ACTION_NONE;
    mpartition_region_t *reg = NULL;

    sdk::asic::asic_mem_write(q_addr, (uint8_t *)buf, q_size);

    reg = mp_->region_by_address(q_addr);
    SDK_ASSERT(reg != NULL);

    if (is_region_cache_pipe_p4plus_all(reg)) {
        action = P4PLUS_CACHE_INVALIDATE_BOTH;
    } else if (is_region_cache_pipe_p4plus_rxdma(reg)) {
        action = P4PLUS_CACHE_INVALIDATE_RXDMA;
    } else if (is_region_cache_pipe_p4plus_txdma(reg)) {
        action = P4PLUS_CACHE_INVALIDATE_TXDMA;
    }

    if (action != P4PLUS_CACHE_ACTION_NONE) {
        p4plus_invalidate_cache(q_addr, q_size, action);
    }

    return 0;
}


void
LIFManager::set_program_info(program_info *pinfo)
{
    pinfo_ = pinfo;
}

int32_t LIFManager::GetPCOffset(const char *handle,
                                const char *prog_name,
                                const char *label,
                                uint8_t *ret_offset) {
    assert(handle);
    return get_pc_offset(pinfo_, prog_name, label, ret_offset);
}

} // namespace capri
} // namespace platform
} // namespace sdk
