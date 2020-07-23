/*
* Copyright (c) 2019, Pensando Systems Inc.
*/

#include <chrono>
#include <iostream>
#include "edmaq.hpp"
#include "spdlog/spdlog.h"
#include "nic/sdk/lib/utils/utils.hpp"
#include "nic/sdk/asic/pd/pd.hpp"
#include "nic/sdk/asic/pd/db.hpp"
#include "nic/sdk/asic/common/asic_common.hpp"
#include "nic/sdk/asic/common/asic_qstate.hpp"
#include "nic/sdk/platform/pal/include/pal_mem.h"
#include "nic/sdk/lib/pal/pal.hpp"

#define MEM_SET(pa, val, sz, flags) { \
    uint8_t v = val; \
    for (size_t i = 0; i < sz; i += sizeof(v)) { \
        sdk::lib::pal_mem_write(pa + i, &v, sizeof(v)); \
    } \
}
#define MEM_CLR(pa, va, sz) { \
        MEM_SET(pa, 0, sz, 0); \
}

using namespace sdk;

EdmaQ *EdmaQ::factory(
    const char *name,
    uint16_t lif,
    uint8_t qtype,
    uint32_t qid,
    uint64_t ring_base,
    uint64_t comp_base,
    uint16_t ring_size, void *sw_phv, EV_P)
{
    EdmaQ *edmaq;

    edmaq = (EdmaQ *)SDK_CALLOC(SDK_MEM_ALLOC_EDMAQ, sizeof(EdmaQ));
    new (edmaq) EdmaQ();

    edmaq->loop = loop;
    edmaq->name = name;
    edmaq->lif = lif;
    edmaq->qtype = qtype;
    edmaq->qid = qid;
    edmaq->ring_base = ring_base;
    edmaq->comp_base = comp_base;
    edmaq->ring_size = ring_size;
    edmaq->pending =
        (struct edmaq_ctx *)SDK_CALLOC(SDK_MEM_ALLOC_EDMAQ_PENDING,
        sizeof(struct edmaq_ctx) * ring_size);
    edmaq->init = false;
    edmaq->sw_phv = sw_phv;

    return edmaq;
}

bool
EdmaQ::Init(sdk::platform::utils::program_info *pinfo, uint64_t qstate_ptr, uint8_t cos_sel,
    uint8_t cosA, uint8_t cosB)
{
    edma_qstate_t qstate = {0};

    head = 0;
    tail = 0;
    comp_tail = 0;
    exp_color = 1;

    // Init the edmaq instance field
    qstate_addr = qstate_ptr;
 
    SDK_TRACE_INFO("%s: Initializing edmaq lif 0x%x qtype %d qid 0x%x",
        name, lif, qtype, qid);

    // Init rings
    MEM_SET(ring_base, 0, (sizeof(struct edma_cmd_desc) * ring_size), 0);
    MEM_SET(comp_base, 0, (sizeof(struct edma_comp_desc) * ring_size), 0);
    SDK_TRACE_INFO("%s: Initializing edma qstate 0x%lx", name, qstate_addr);

    uint8_t off;
    if (sdk::asic::get_pc_offset(pinfo, "txdma_stage0.bin", "edma_stage0", &off) < 0) {
        SDK_TRACE_DEBUG("Failed to get PC offset of program: txdma_stage0.bin label: edma_stage0"); 
       return false;
    }
    qstate.pc_offset = off;
    qstate.cos_sel = cos_sel;
    qstate.cosA = cosA;
    qstate.cosB = cosB;
    qstate.host = 0;
    qstate.total = 1;
    qstate.pid = 0;
    qstate.p_index0 = head;
    qstate.c_index0 = 0;
    qstate.comp_index = comp_tail;
    qstate.sta.color = exp_color;
    qstate.cfg.enable = 1;
    qstate.ring_base = ring_base;
    qstate.ring_size = log2(ring_size);
    qstate.cq_ring_base = comp_base;
    qstate.cfg.intr_enable = 0;
    qstate.intr_assert_index = 0;
    sdk::lib::pal_mem_write(qstate_addr, (uint8_t *)&qstate, sizeof(qstate), 0);

    PAL_barrier();
    sdk::asic::pd::asicpd_p4plus_invalidate_cache(qstate_addr, sizeof(edma_qstate_t), P4PLUS_CACHE_INVALIDATE_TXDMA);

    init = true;

    return true;
}

bool
EdmaQ::Reset()
{
    uint64_t db_data;
    asic_db_addr_t db_addr = { 0 };
    uint64_t addr = qstate_addr;

    SDK_TRACE_INFO("%s: Resetting edmaq lif 0x%x qtype %d qid 0x%x",
        name, lif, qtype, qid);

    if (!init)
        return true;
    MEM_SET(addr, 0, fldsiz(edma_qstate_t, pc_offset), 0);
    PAL_barrier();
    sdk::asic::pd::asicpd_p4plus_invalidate_cache(addr, sizeof(edma_qstate_t),
                                                  P4PLUS_CACHE_INVALIDATE_TXDMA);

    db_addr.lif_id = lif;
    db_addr.q_type = qtype;
    db_addr.upd = ASIC_DB_ADDR_UPD_FILL(ASIC_DB_UPD_SCHED_CLEAR,
                                        ASIC_DB_UPD_INDEX_UPDATE_NONE, false);

    db_data = qid << 24;
    sdk::asic::pd::asic_ring_db(&db_addr, db_data);

    init = false;

    return true;
}

bool
EdmaQ::Debug(bool enable)
{
    struct edma_cfg_qstate cfg = {0};
    uint64_t addr = qstate_addr;

    sdk::lib::pal_mem_read(addr + offsetof(struct edma_qstate, cfg),
        (uint8_t *)&cfg, sizeof(cfg), 0);
    cfg.debug = enable;
    sdk::lib::pal_mem_write(addr + offsetof(struct edma_qstate, cfg),
        (uint8_t *)&cfg, sizeof(cfg), 0);

    sdk::asic::pd::asicpd_p4plus_invalidate_cache(addr, sizeof(edma_qstate_t),
                                                  P4PLUS_CACHE_INVALIDATE_TXDMA);

    return true;
}

bool
EdmaQ::Post(edma_opcode opcode, uint64_t from, uint64_t to, uint16_t size,
            struct edmaq_ctx *ctx)
{
    uint64_t addr;
    auto offset = 0;
    auto chunk_sz = (size < EDMAQ_MAX_TRANSFER_SZ) ? size : EDMAQ_MAX_TRANSFER_SZ;
    auto transfer_sz = 0;
    auto bytes_left = size;
    struct edma_cmd_desc cmd = {0};
    uint16_t head_idx = head;
    sdk_ret_t ret;

    if (!init)
        return false;

    /* MS only supports reading/writing 64B per DMA transaction */
    if (from <= ASIC_HBM_BASE || to <= ASIC_HBM_BASE) {
        chunk_sz = (chunk_sz < 64) ? chunk_sz : 64;
    }

    while (bytes_left > 0) {

        transfer_sz = (bytes_left <= chunk_sz) ? bytes_left : chunk_sz;

        auto ring_full = [](uint16_t h, uint16_t t, uint16_t sz) -> bool {
            return (((h + 1) % sz) == t);
        };

        /* If the ring is full, then do a blocking wait for a completion */
        if (ring_full(head_idx, tail, ring_size)) {
            while (!Poll()) {
                usleep(EDMAQ_COMP_POLL_US);
            }
            /* Blocking wait for completion above should have made space in the ring.
               So we should not hit the following condition at all */
            if (ring_full(head_idx, tail, ring_size)) {
                SDK_TRACE_INFO("%s: EDMA queue full head %d tail %d", name, head_idx, tail);
                return false;
            }
        }

        /* If this is the last chunk then set the completion callback */
        if (bytes_left <= chunk_sz && ctx)
            pending[head_idx] = *ctx;
        else
            pending[head_idx] = {0};

        pending[head_idx].deadline = chrono::steady_clock::now() +
            chrono::milliseconds(EDMAQ_COMP_TIMEOUT_MS);

        /* enqueue edma command */
        addr = ring_base + head_idx * sizeof(struct edma_cmd_desc);
        cmd = {0};
        cmd.opcode = opcode;
        cmd.len = transfer_sz;
        cmd.src_lif = lif;
        cmd.src_addr = from + offset;
        cmd.dst_lif = lif;
        cmd.dst_addr = to + offset;
        sdk::lib::pal_mem_write(addr, (uint8_t *)&cmd, sizeof(struct edma_cmd_desc), 0);
        head_idx = (head_idx + 1) % ring_size;
        offset += transfer_sz;
        bytes_left -= transfer_sz;
    }

    if (sw_phv)
        ret = post_via_sw_phv(head_idx);
    else
        ret = post_via_db(head_idx);

    if (ret != SDK_RET_OK) {
       return false;
    }

    // update head in edma object
    head = head_idx;

    if (ctx == NULL) {
        Flush();
    } else {
        if (ctx->cb && loop != NULL)
            evutil_add_prepare(EV_A_ &prepare, EdmaQ::PollCb, this);
    }

    return true;
}

sdk_ret_t
EdmaQ::post_via_db(uint16_t index) {

    asic_db_addr_t db_addr = { 0 };
    uint64_t db_data;

    db_addr.lif_id = lif;
    db_addr.q_type = qtype;
    db_addr.upd = ASIC_DB_ADDR_UPD_FILL(ASIC_DB_UPD_SCHED_SET,
                                        ASIC_DB_UPD_INDEX_SET_PINDEX, false);

    PAL_barrier();

    db_data = (qid << 24) | index;
    sdk::asic::pd::asic_ring_db(&db_addr, db_data);

    return SDK_RET_OK;
}

sdk_ret_t
EdmaQ::post_via_sw_phv(uint16_t index) {
    sdk_ret_t ret;

    // write PI into CB
    sdk::lib::pal_mem_write(qstate_addr + offsetof(edma_qstate_t, p_index0),
                            (uint8_t *)&index, sizeof(uint16_t), 0);
    sdk::asic::pd::asicpd_p4plus_invalidate_cache(qstate_addr,
                                                  sizeof(edma_qstate_t),
                                                  P4PLUS_CACHE_INVALIDATE_TXDMA);

    PAL_barrier();

    ret = sdk::asic::pd::asicpd_sw_phv_inject(sdk::asic::ASIC_SWPHV_TYPE_TXDMA,
                                              0, 0, 1, sw_phv);
    SDK_TRACE_INFO("sw phv inject returned %d", ret);
    return ret;
}

void
EdmaQ::PollCb(void *obj)
{
    EdmaQ *edmaq = (EdmaQ *)obj;
    edmaq->Poll();
}

bool
EdmaQ::Poll()
{
    struct edma_comp_desc comp = {0};
    uint64_t addr = comp_base + comp_tail * sizeof(struct edma_comp_desc);
    bool timeout =  (chrono::steady_clock::now() > pending[tail].deadline);

    sdk::lib::pal_mem_read(addr, (uint8_t *)&comp, sizeof(struct edma_comp_desc), 0);
    if (comp.color == exp_color || timeout) {
        if (pending[tail].cb) {
            pending[tail].cb(pending[tail].obj);
            memset(&pending[tail], 0, sizeof(pending[tail]));
        }
        comp_tail = (comp_tail + 1) % ring_size;
        if (comp_tail == 0) {
            exp_color = exp_color ? 0 : 1;
        }
        tail = (tail + 1) % ring_size;
        if (Empty()) {
            evutil_remove_prepare(EV_A_ &prepare);
        }
        return true;
    }

    return false;
}

bool
EdmaQ::Empty()
{
    return (head == tail);
}

void
EdmaQ::Flush()
{
    while (!Empty()) {
        Poll();
        usleep(EDMAQ_COMP_POLL_US);
    };
}
