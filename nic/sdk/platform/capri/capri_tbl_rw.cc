// {C} Copyright 2018 Pensando Systems Inc. All rights reserved

/*
 * capri_tbl_rw.cc
 * Mahesh Shirshyad (Pensando Systems)
 */
#include <map>
#include "platform/capri/capri_common.hpp"
#include "gen/p4gen/common_rxdma_actions/include/common_rxdma_actions_p4pd.h"
#include "lib/p4/p4_api.hpp"
#include "lib/utils/time_profile.hpp"
#include "platform/capri/capri_tbl_rw.hpp"
#include "platform/capri/capri_hbm_rw.hpp"
#include "platform/capri/capri_tm_rw.hpp"
#include "platform/capri/capri_txs_scheduler.hpp"
#include "platform/capri/capri_state.hpp"
#include "platform/capri/csrint/csr_init.hpp"
#include "third-party/asic/capri/model/utils/cap_csr_py_if.h"
#include "asic/rw/asicrw.hpp"
#include "asic/pd/pd.hpp"
#include "third-party/asic/capri/model/utils/cap_blk_reg_model.h"
#include "third-party/asic/capri/model/cap_top/cap_top_csr.h"
#include "third-party/asic/capri/model/cap_pic/cap_pict_csr.h"
#include "third-party/asic/capri/model/cap_pic/cap_pics_csr.h"
#include "third-party/asic/capri/model/cap_te/cap_te_csr.h"
#include "third-party/asic/ip/verif/pcpp/cpp_int_helper.h"
#include "third-party/asic/capri/verif/apis/cap_pics_api.h"
#include "third-party/asic/capri/verif/apis/cap_pict_api.h"
#include "platform/capri/csr/asicrw_if.hpp"
#include "gen/platform/mem_regions.hpp"

extern pen_csr_base *get_csr_base_from_path(string);

namespace sdk {
namespace platform {
namespace capri {

/* When ready to use unified memory mgmt library, change CALLOC and FREE then */
#define CAPRI_CALLOC  calloc
#define CAPRI_FREE    free

typedef int capri_error_t;

/*  Design decisions + Table Update Flow:
 *
 *  Maintain Shadow memory for SRAM and TCAM Units.
 *
 *   1.  Shadow memory maintained in ARM memory will be mirror representation
 *       of how capri TCAM/SRAM addressing is done. This implies
 *       uint8_t ram_row[10 blocks * 16 bytes]. 4k such rows will be created.
 *
 *   2.  To start with there will be single lock to protect updates to shadow
 *       memory (if HAL updates can come from multiple threads)
 *       To improve update performance and avoid lock contention, 4K rows
 *       can be divided into zones and one lock per zone can be maintained.
 *       (TO START WITH, I HAVE DECIDED TO USE ONE LOCK)
 *
 *   3. Using table property API provided by p4pd_api, start block, start
 *      row, and number of buckets (number of table entries within
 *      a single row) are obtained.
 *
 *   4. Using index (at which table entry need to be written to or read from)
 *      row#, one or more blocks#, and start word# in first block,
 *      end word# in last block are computed.
 *
 *   5. Read + modify of the relevant blocks and push those blocks to Capri
 *
 */

/*
 *
 *
 *  CAPRI SRAM ADDRESSING:
 *
 *       +=================================================================+
 * Row0  | Block0 (128bits) | Block1 (128b) | ...........| Block9 (128b)   |
 *       +-----------------------------------------------------------------+
 * Row1  | Block0 (128bits) | Block1 (128b) | ...........| Block9 (128b)   |
 *       +-----------------------------------------------------------------+
 * Row2  | Block0 (128bits) | Block1 (128b) | ...........| Block9 (128b)   |
 *       +-----------------------------------------------------------------+
 *
 *                          :
 *                          :
 *       +-----------------------------------------------------------------+
 * Row   | Block0 (128bits) | Block1 (128b) | ...........| Block9 (128b)   |
 * 4K-1  +=================================================================+
 *
 *
 *     1. Any memory writes / reads are done in units of block. To update
 *        a table entry that is within one or more blocks, all such memory
 *        blocks on a particular Row will need to modified and written back
 *        to capri.
 *
 *     2. Table entry start and end on 16b boundary. Multiple such 16b words
 *        are updated or read from when performing table write or read.
 *
 */

#define CAPRI_SRAM_BLOCK_COUNT      (8)
#define CAPRI_SRAM_BLOCK_WIDTH      (128) // bits
#define CAPRI_SRAM_WORD_WIDTH       (16)  // bits; is also unit of allocation.
#define CAPRI_SRAM_WORDS_PER_BLOCK  (8)
#define CAPRI_SRAM_ROWS             (0x1000) // 4K

#define CAPRI_TCAM_BLOCK_COUNT      (8)
#define CAPRI_TCAM_BLOCK_WIDTH      (128) // bits
#define CAPRI_TCAM_WORD_WIDTH       (16)  // bits; is also unit of allocation.
#define CAPRI_TCAM_WORDS_PER_BLOCK  (8)
#define CAPRI_TCAM_ROWS             (0x400) // 1K

typedef struct capri_sram_shadow_mem_ {
    uint8_t zones;          // Using entire memory as one zone.
                            // TBD: carve into multiple zones
                            // to reduce access/update contention

    //pthread_mutex_t mutex; // TBD: when its decided to make HAL thread safe

    // Since writes/read access to SRAM are in done in units of block
    // a three dim array is maintained
    // Since word width is 16bits, uint16_t is used. A table entry starts at 16b
    // boundary
    uint16_t mem[CAPRI_SRAM_ROWS][CAPRI_SRAM_BLOCK_COUNT][CAPRI_SRAM_WORDS_PER_BLOCK];

} capri_sram_shadow_mem_t;

typedef struct capri_tcam_shadow_mem_ {
    uint8_t zones;          // Using entire memory as one zone.
                            // TBD: carve into multiple zones
                            // to reduce access/update contention

    //pthread_mutex_t mutex; // TBD: when its decided to make HAL thread safe

    uint16_t mem_x[CAPRI_TCAM_ROWS][CAPRI_TCAM_BLOCK_COUNT][CAPRI_TCAM_WORDS_PER_BLOCK];
    uint16_t mem_y[CAPRI_TCAM_ROWS][CAPRI_TCAM_BLOCK_COUNT][CAPRI_TCAM_WORDS_PER_BLOCK];

} capri_tcam_shadow_mem_t;


static capri_sram_shadow_mem_t *g_shadow_sram_p4[2];
static capri_tcam_shadow_mem_t *g_shadow_tcam_p4[2];
static capri_sram_shadow_mem_t *g_shadow_sram_rxdma;
static capri_sram_shadow_mem_t *g_shadow_sram_txdma;

static capri_sram_shadow_mem_t *
get_sram_shadow_for_table (uint32_t tableid, int gress)
{
    if ((tableid >= p4pd_tableid_min_get()) &&
        (tableid <= p4pd_tableid_max_get())) {
        return (g_shadow_sram_p4[gress]);
    } else if ((tableid >= p4pd_rxdma_tableid_min_get()) &&
               (tableid <= p4pd_rxdma_tableid_max_get())) {
        return (g_shadow_sram_rxdma);
    } else if ((tableid >= p4pd_txdma_tableid_min_get()) &&
               (tableid <= p4pd_txdma_tableid_max_get())) {
        return (g_shadow_sram_txdma);
    } else {
        SDK_ASSERT(0);
    }
    return NULL;
}

/* HBM base address in System memory map; Cached once at the init time */
static uint64_t hbm_mem_base_addr;

/* Store action pc for every action of the table. */
static uint64_t capri_action_asm_base[P4TBL_ID_MAX][P4TBL_MAX_ACTIONS];
static uint64_t capri_action_rxdma_asm_base[P4TBL_ID_MAX][P4TBL_MAX_ACTIONS];
static uint64_t capri_action_txdma_asm_base[P4TBL_ID_MAX][P4TBL_MAX_ACTIONS];
static uint64_t capri_table_rxdma_asm_base[P4TBL_ID_MAX];
static uint64_t capri_table_txdma_asm_base[P4TBL_ID_MAX];

typedef enum capri_tbl_rw_logging_levels_ {
    CAP_TBL_RW_LOG_LEVEL_ALL = 0,
    CAP_TBL_RW_LOG_LEVEL_INFO,
    CAP_TBL_RW_LOG_LEVEL_ERROR,
} capri_tbl_rw_logging_levels;

void
capri_program_table_mpu_pc (int tableid, bool ingress, int stage,
                            int stage_tableid,
                            uint64_t capri_table_asm_err_offset,
                            uint64_t capri_table_asm_base)
{
    /* Program table base address into capri TE */
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();

    assert(stage_tableid < 16);
    SDK_TRACE_DEBUG("====Stage: %d Tbl_id: %u, Stg_Tbl_id %u, Tbl base: 0x%lx==",
                    stage, tableid, stage_tableid, capri_table_asm_base);
    if (ingress) {
        cap_te_csr_t &te_csr = cap0.sgi.te[stage];
        // Push to HW/Capri from entry_start_block to block
        te_csr.cfg_table_property[stage_tableid].read();
        te_csr.cfg_table_property[stage_tableid]
                .mpu_pc(((capri_table_asm_base) >> 6));
        te_csr.cfg_table_property[stage_tableid]
                .mpu_pc_ofst_err(capri_table_asm_err_offset);
        te_csr.cfg_table_property[stage_tableid].write();
    } else {
        cap_te_csr_t &te_csr = cap0.sge.te[stage];
        // Push to HW/Capri from entry_start_block to block
        te_csr.cfg_table_property[stage_tableid].read();
        te_csr.cfg_table_property[stage_tableid]
                .mpu_pc(((capri_table_asm_base) >> 6));
        te_csr.cfg_table_property[stage_tableid]
                .mpu_pc_ofst_err(capri_table_asm_err_offset);
        te_csr.cfg_table_property[stage_tableid].write();
    }
}

void
capri_program_hbm_table_base_addr (int stage_tableid, char *tablename,
                                   int stage, int pipe)
{
    hbm_addr_t start_offset;

#ifdef MEM_REGION_RSS_INDIR_TABLE_NAME
    if (strcmp(tablename, MEM_REGION_RSS_INDIR_TABLE_NAME) == 0) {
        // TODO: Work with Neel to clean capri_toeplitz_init
        return;
    }
#endif

    assert(stage_tableid < 16);
    start_offset = get_mem_addr(tablename);
    SDK_TRACE_DEBUG("===HBM Tbl Name: %s, Stage: %d, StageTblID: %u, "
                    "Addr: 0x%lx}===",
                    tablename, stage, stage_tableid, start_offset);
    if (start_offset == INVALID_MEM_ADDRESS) {
        return;
    }

    /* Program table base address into capri TE */
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    if (pipe == P4_PIPELINE_INGRESS) {
        cap_te_csr_t &te_csr = cap0.sgi.te[stage];
        te_csr.cfg_table_property[stage_tableid].read();
        te_csr.cfg_table_property[stage_tableid].addr_base(start_offset);
        te_csr.cfg_table_property[stage_tableid].write();
    } else if (pipe == P4_PIPELINE_EGRESS) {
        cap_te_csr_t &te_csr = cap0.sge.te[stage];
        te_csr.cfg_table_property[stage_tableid].read();
        te_csr.cfg_table_property[stage_tableid].addr_base(start_offset);
        te_csr.cfg_table_property[stage_tableid].write();
    } else if (pipe == P4_PIPELINE_RXDMA) {
        cap_te_csr_t &te_csr = cap0.pcr.te[stage];
        te_csr.cfg_table_property[stage_tableid].read();
        te_csr.cfg_table_property[stage_tableid].addr_base(start_offset);
        te_csr.cfg_table_property[stage_tableid].write();
    } else if (pipe == P4_PIPELINE_TXDMA) {
        cap_te_csr_t &te_csr = cap0.pct.te[stage];
        te_csr.cfg_table_property[stage_tableid].read();
        te_csr.cfg_table_property[stage_tableid].addr_base(start_offset);
        te_csr.cfg_table_property[stage_tableid].write();
    } else {
        SDK_ASSERT(0);
    }
}

#define CAPRI_P4PLUS_RX_STAGE0_QSTATE_OFFSET_0            0
#define CAPRI_P4PLUS_RX_STAGE0_QSTATE_OFFSET_64           64

static void
capri_program_p4plus_table_mpu_pc_args (int tbl_id, cap_te_csr_t *te_csr,
                                        uint64_t pc, uint32_t offset)
{
    te_csr->cfg_table_property[tbl_id].read();
    te_csr->cfg_table_property[tbl_id].mpu_pc(pc >> 6);
    te_csr->cfg_table_property[tbl_id].mpu_pc_dyn(1);
    te_csr->cfg_table_property[tbl_id].addr_base(offset);
    te_csr->cfg_table_property[tbl_id].write();
}

#define CAPRI_P4PLUS_HANDLE         "p4plus"
#define CAPRI_P4PLUS_RXDMA_PROG		"rxdma_stage0.bin"
#define CAPRI_P4PLUS_RXDMA_EXT_PROG	"rxdma_stage0_ext.bin"
#define CAPRI_P4PLUS_TXDMA_PROG		"txdma_stage0.bin"
#define CAPRI_P4PLUS_TXDMA_EXT_PROG	"txdma_stage0_ext.bin"

void
capri_program_p4plus_sram_table_mpu_pc (int tableid, int stage_tbl_id,
                                        int stage)
{
    uint64_t pc = 0;
    cap_te_csr_t *te_csr = NULL;
    bool pipe_rxdma = false;

    cap_top_csr_t &cap0 = g_capri_state_pd->cap_top();

    if ((uint32_t)tableid >= p4pd_rxdma_tableid_min_get() &&
        (uint32_t)tableid < p4pd_rxdma_tableid_max_get()) {
        uint32_t lcl_tableid = tableid - p4pd_rxdma_tableid_min_get();
        te_csr = &cap0.pcr.te[stage];
        pc = capri_table_rxdma_asm_base[lcl_tableid];
        pipe_rxdma = true;
    } else if ((uint32_t)tableid >= p4pd_txdma_tableid_min_get() &&
               (uint32_t)tableid < p4pd_txdma_tableid_max_get()) {
        uint32_t lcl_tableid = tableid - p4pd_txdma_tableid_min_get();
        te_csr = &cap0.pct.te[stage];
        pc = capri_table_txdma_asm_base[lcl_tableid];
        pipe_rxdma = false;
    }
    if (pc == 0) {
        return;
    }
    SDK_TRACE_DEBUG("====Pipe: %s Stage: %d Tbl_id: %u, Stg_Tbl_id %u, "
                    "Tbl base: 0x%lx====", ((pipe_rxdma) ? "RxDMA" : "TxDMA"),
                    stage, tableid, stage_tbl_id, pc);
    te_csr->cfg_table_property[stage_tbl_id].read();
    te_csr->cfg_table_property[stage_tbl_id].mpu_pc(pc >> 6);
    te_csr->cfg_table_property[stage_tbl_id].mpu_pc_dyn(0);
    te_csr->cfg_table_property[stage_tbl_id].addr_base(0);
    te_csr->cfg_table_property[stage_tbl_id].write();
}

// RSS Topelitz Table
#define ETH_RSS_INDIR_PROGRAM               "eth_rx_rss_indir.bin"
// Maximum number of queue per LIF
#define ETH_RSS_MAX_QUEUES                  (128)
// Number of entries in a LIF's indirection table
#define ETH_RSS_LIF_INDIR_TBL_LEN           ETH_RSS_MAX_QUEUES
// Size of each LIF indirection table entry
#define ETH_RSS_LIF_INDIR_TBL_ENTRY_SZ      (sizeof(eth_rx_rss_indir_eth_rx_rss_indir_t))
// Size of a LIF's indirection table
#define ETH_RSS_LIF_INDIR_TBL_SZ            (ETH_RSS_LIF_INDIR_TBL_LEN * ETH_RSS_LIF_INDIR_TBL_ENTRY_SZ)
// Max number of LIFs supported
#define MAX_LIFS                            (2048)
// Size of the entire LIF indirection table
#define ETH_RSS_INDIR_TBL_SZ                (MAX_LIFS * ETH_RSS_LIF_INDIR_TBL_SZ)

int
capri_toeplitz_init (int stage, int stage_tableid)
{
    int tbl_id;
    uint64_t pc;
    uint64_t tbl_base;
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    cap_te_csr_t *te_csr = NULL;

    if (sdk::p4::p4_program_to_base_addr((char *) CAPRI_P4PLUS_HANDLE,
                                   (char *) ETH_RSS_INDIR_PROGRAM,
                                   &pc) != 0) {
        SDK_TRACE_DEBUG("Could not resolve handle %s program %s",
                        (char *) CAPRI_P4PLUS_HANDLE,
                        (char *) ETH_RSS_INDIR_PROGRAM);
        return CAPRI_FAIL;
    }
    SDK_TRACE_DEBUG("Resolved handle %s program %s to PC 0x%lx",
                    (char *) CAPRI_P4PLUS_HANDLE,
                    (char *) ETH_RSS_INDIR_PROGRAM,
                    pc);

    // Program rss params table with the PC
    te_csr = &cap0.pcr.te[stage];

    tbl_id = stage_tableid;

#ifdef MEM_REGION_RSS_INDIR_TABLE_NAME
    tbl_base = get_mem_addr(MEM_REGION_RSS_INDIR_TABLE_NAME);
    SDK_ASSERT(tbl_base != INVALID_MEM_ADDRESS);
#else
    SDK_ASSERT(0);
#endif
    // Align the table address because while calculating the read address TE shifts the LIF
    // value by LOG2 of size of the per lif indirection table.
    tbl_base = (tbl_base + ETH_RSS_INDIR_TBL_SZ) & ~(ETH_RSS_INDIR_TBL_SZ - 1);

    SDK_TRACE_DEBUG("rss_indir_table id %u table_base %llx\n", tbl_id, tbl_base);

    te_csr->cfg_table_property[tbl_id].read();
    te_csr->cfg_table_property[tbl_id].mpu_pc(pc >> 6);
    te_csr->cfg_table_property[tbl_id].mpu_pc_dyn(0);
    // HBM Table
    te_csr->cfg_table_property[tbl_id].axi(0); //1==table in SRAM, 0== table in HBM
    // TE addr = hash
    // TE mask = (1 << addr_sz) - 1
    te_csr->cfg_table_property[tbl_id].addr_sz((uint8_t)log2(ETH_RSS_LIF_INDIR_TBL_LEN));
    // TE addr <<= addr_shift
    te_csr->cfg_table_property[tbl_id].addr_shift((uint8_t)log2(ETH_RSS_LIF_INDIR_TBL_ENTRY_SZ));
    // TE addr = (hash & mask) + addr_base
    te_csr->cfg_table_property[tbl_id].addr_base(tbl_base);
    // TE lif_shift_en
    te_csr->cfg_table_property[tbl_id].addr_vf_id_en(1);
    // TE lif_shift
    te_csr->cfg_table_property[tbl_id].addr_vf_id_loc((uint8_t)log2(ETH_RSS_LIF_INDIR_TBL_SZ));
    // addr |= (lif << lif_shift)
    // TE addr = addr & ((1 << chain_shift) - 1) if 0 <= cycle_id < 63 else addr
    te_csr->cfg_table_property[tbl_id].chain_shift(0x3f);
    // size of each indirection table entry
    te_csr->cfg_table_property[tbl_id].lg2_entry_size((uint8_t)log2(ETH_RSS_LIF_INDIR_TBL_ENTRY_SZ));
    te_csr->cfg_table_property[tbl_id].write();

    return CAPRI_OK;
}

int
capri_p4plus_table_init (platform_type_t platform_type,
                         int stage_apphdr, int stage_tableid_apphdr,
                         int stage_apphdr_ext, int stage_tableid_apphdr_ext,
                         int stage_apphdr_off, int stage_tableid_apphdr_off,
                         int stage_apphdr_ext_off, int stage_tableid_apphdr_ext_off,
                         int stage_txdma_act, int stage_tableid_txdma_act,
                         int stage_txdma_act_ext, int stage_tableid_txdma_act_ext)
{
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    cap_te_csr_t *te_csr = NULL;
    uint64_t capri_action_p4plus_asm_base;

    // Resolve the p4plus rxdma stage 0 program to its action pc
    if (sdk::p4::p4_program_to_base_addr((char *) CAPRI_P4PLUS_HANDLE,
                                   (char *) CAPRI_P4PLUS_RXDMA_PROG,
                                   &capri_action_p4plus_asm_base) != 0) {
        SDK_TRACE_DEBUG("Could not resolve handle %s program %s",
                        (char *) CAPRI_P4PLUS_HANDLE,
                        (char *) CAPRI_P4PLUS_RXDMA_PROG);
        return CAPRI_FAIL;
    }
    SDK_TRACE_DEBUG("Resolved handle %s program %s to PC 0x%lx",
                    (char *) CAPRI_P4PLUS_HANDLE,
                    (char *) CAPRI_P4PLUS_RXDMA_PROG,
                    capri_action_p4plus_asm_base);

    // Program app-header table config @(stage, stage_tableid) with the PC
    te_csr = &cap0.pcr.te[stage_apphdr];
    capri_program_p4plus_table_mpu_pc_args(
            stage_tableid_apphdr, te_csr,
            capri_action_p4plus_asm_base,
            CAPRI_P4PLUS_RX_STAGE0_QSTATE_OFFSET_0);

    // Program app-header offset 64 table config @(stage, stage_tableid) with the same PC as above
    capri_program_p4plus_table_mpu_pc_args(
            stage_tableid_apphdr_off, te_csr,
            capri_action_p4plus_asm_base,
            CAPRI_P4PLUS_RX_STAGE0_QSTATE_OFFSET_64);


    // Resolve the p4plus rxdma stage 0 "ext" program to its action pc
    if (sdk::p4::p4_program_to_base_addr((char *) CAPRI_P4PLUS_HANDLE,
                                   (char *) CAPRI_P4PLUS_RXDMA_EXT_PROG,
                                   &capri_action_p4plus_asm_base) != 0) {
        SDK_TRACE_DEBUG("Could not resolve handle %s program %s",
                        (char *) CAPRI_P4PLUS_HANDLE,
                        (char *) CAPRI_P4PLUS_RXDMA_EXT_PROG);
        return CAPRI_FAIL;
    }
    SDK_TRACE_DEBUG("Resolved handle %s program %s to PC %llx",
                    (char *) CAPRI_P4PLUS_HANDLE,
                    (char *) CAPRI_P4PLUS_RXDMA_EXT_PROG,
                    capri_action_p4plus_asm_base);

    // Program app-header table config @(stage, stage_tableid) with the PC
    te_csr = &cap0.pcr.te[stage_apphdr_ext];
    capri_program_p4plus_table_mpu_pc_args(
            stage_tableid_apphdr_ext, te_csr,
            capri_action_p4plus_asm_base,
            CAPRI_P4PLUS_RX_STAGE0_QSTATE_OFFSET_0);

    // Program app-header offset 64 table config @(stage, stage_tableid) with the same PC as above
    capri_program_p4plus_table_mpu_pc_args(
            stage_tableid_apphdr_ext_off, te_csr,
            capri_action_p4plus_asm_base,
            CAPRI_P4PLUS_RX_STAGE0_QSTATE_OFFSET_64);

    // Resolve the p4plus txdma stage 0 program to its action pc
    if (sdk::p4::p4_program_to_base_addr((char *) CAPRI_P4PLUS_HANDLE,
                                   (char *) CAPRI_P4PLUS_TXDMA_PROG,
                                   &capri_action_p4plus_asm_base) != 0) {
        SDK_TRACE_DEBUG("Could not resolve handle %s program %s",
                        (char *) CAPRI_P4PLUS_HANDLE,
                        (char *) CAPRI_P4PLUS_TXDMA_PROG);
        return CAPRI_FAIL;
    }
    SDK_TRACE_DEBUG("Resolved handle %s program %s to PC 0x%lx",
                    (char *) CAPRI_P4PLUS_HANDLE,
                    (char *) CAPRI_P4PLUS_TXDMA_PROG,
                    capri_action_p4plus_asm_base);

    // Program table config @(stage, stage_tableid) with the PC
    te_csr = &cap0.pct.te[stage_txdma_act];
    capri_program_p4plus_table_mpu_pc_args(
            stage_tableid_txdma_act, te_csr,
            capri_action_p4plus_asm_base, 0);

    if ((stage_txdma_act == 0) &&
        (platform_type != platform_type_t::PLATFORM_TYPE_SIM)) {
        // TODO: This should 16 as we can process 16 packets per doorbell.
        te_csr->cfg_table_property[stage_tableid_txdma_act].max_bypass_cnt(0x10);
        te_csr->cfg_table_property[stage_tableid_txdma_act].write();
    }

    // Resolve the p4plus txdma stage 0 "ext" program to its action pc
    if (sdk::p4::p4_program_to_base_addr((char *) CAPRI_P4PLUS_HANDLE,
                                   (char *) CAPRI_P4PLUS_TXDMA_EXT_PROG,
                                   &capri_action_p4plus_asm_base) != 0) {
        SDK_TRACE_DEBUG("Could not resolve handle %s program %s",
                        (char *) CAPRI_P4PLUS_HANDLE,
                        (char *) CAPRI_P4PLUS_TXDMA_EXT_PROG);
        return CAPRI_FAIL;
    }
    SDK_TRACE_DEBUG("Resolved handle %s program %s to PC 0x%lx",
                    (char *) CAPRI_P4PLUS_HANDLE,
                    (char *) CAPRI_P4PLUS_TXDMA_EXT_PROG,
                    capri_action_p4plus_asm_base);

    // Program table config @(stage, stage_tableid) with the PC
    te_csr = &cap0.pct.te[stage_txdma_act_ext];
    capri_program_p4plus_table_mpu_pc_args(
            stage_tableid_txdma_act_ext, te_csr,
            capri_action_p4plus_asm_base, 0);

    if ((stage_txdma_act_ext == 0) &&
        (platform_type != platform_type_t::PLATFORM_TYPE_SIM)) {
        // TODO: This should 16 as we can process 16 packets per doorbell.
        te_csr->cfg_table_property[stage_tableid_txdma_act_ext].max_bypass_cnt(0x10);
        te_csr->cfg_table_property[stage_tableid_txdma_act_ext].write();
    }

    return CAPRI_OK ;
}

void
capri_p4plus_recirc_init (void)
{
    cap_top_csr_t &cap0 = g_capri_state_pd->cap_top();

    // RxDMA
    cap0.pr.pr.psp.cfg_profile.read();
    cap0.pr.pr.psp.cfg_profile.recirc_max_enable(1);
    cap0.pr.pr.psp.cfg_profile.recirc_max(7);
    cap0.pr.pr.psp.cfg_profile.write();

    // TxDMA
    cap0.pt.pt.psp.cfg_profile.read();
    cap0.pt.pt.psp.cfg_profile.recirc_max_enable(1);
    cap0.pt.pt.psp.cfg_profile.recirc_max(7);
    cap0.pt.pt.psp.cfg_profile.write();
}

void
capri_timer_init_helper (uint32_t key_lines)
{
    uint64_t timer_key_hbm_base_addr;
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    cap_txs_csr_t *txs_csr = &cap0.txs.txs;

    timer_key_hbm_base_addr = get_mem_addr(MEM_REGION_TIMERS_NAME);

    txs_csr->cfg_timer_static.read();
    SDK_TRACE_DEBUG("hbm_base %llx", (uint64_t)txs_csr->cfg_timer_static.hbm_base());
    SDK_TRACE_DEBUG("timer hash depth %u", txs_csr->cfg_timer_static.tmr_hsh_depth());
    SDK_TRACE_DEBUG("timer wheel depth %u", txs_csr->cfg_timer_static.tmr_wheel_depth());
    txs_csr->cfg_timer_static.hbm_base(timer_key_hbm_base_addr);
    txs_csr->cfg_timer_static.tmr_hsh_depth(key_lines - 1);
    txs_csr->cfg_timer_static.tmr_wheel_depth(CAPRI_TIMER_WHEEL_DEPTH - 1);
    txs_csr->cfg_timer_static.write();

    txs_csr->cfg_fast_timer_dbell.read();
    txs_csr->cfg_fast_timer_dbell.addr_update(DB_IDX_UPD_PIDX_INC | DB_SCHED_UPD_EVAL);
    txs_csr->cfg_fast_timer_dbell.write();

    txs_csr->cfg_slow_timer_dbell.read();
    txs_csr->cfg_slow_timer_dbell.addr_update(DB_IDX_UPD_PIDX_INC | DB_SCHED_UPD_EVAL);
    txs_csr->cfg_slow_timer_dbell.write();

    // TODO:remove
    txs_csr->cfg_timer_static.read();
    SDK_TRACE_DEBUG("hbm_base %llx", (uint64_t)txs_csr->cfg_timer_static.hbm_base());
    SDK_TRACE_DEBUG("timer hash depth %u", txs_csr->cfg_timer_static.tmr_hsh_depth());
    SDK_TRACE_DEBUG("timer wheel depth %u", txs_csr->cfg_timer_static.tmr_wheel_depth());

    // initialize timer wheel to 0
#if 0
    SDK_TRACE_DEBUG("Initializing timer wheel...");
    for (int i = 0; i <= CAPRI_TIMER_WHEEL_DEPTH; i++) {
        SDK_TRACE_DEBUG("timer wheel index {}", i);
        txs_csr->dhs_tmr_cnt_sram.entry[i].read();
        txs_csr->dhs_tmr_cnt_sram.entry[i].slow_bcnt(0);
        txs_csr->dhs_tmr_cnt_sram.entry[i].slow_lcnt(0);
        txs_csr->dhs_tmr_cnt_sram.entry[i].fast_bcnt(0);
        txs_csr->dhs_tmr_cnt_sram.entry[i].fast_lcnt(0);
        txs_csr->dhs_tmr_cnt_sram.entry[i].write();
    }
#endif
    SDK_TRACE_DEBUG("Done initializing timer wheel");
}

void
capri_timer_init (void)
{
    capri_timer_init_helper(CAPRI_TIMER_NUM_KEY_CACHE_LINES);
}

/* This function initializes the stage id register for p4 plus pipelines such that:
         val0  : 4
         val1  : 5
         val2  : 6
         val3  : 7
         val4  : 0
         val5  : 1
         val6  : 2
         val7  : 3
*/
static void
capri_p4p_stage_id_init (void)
{
    cap_top_csr_t &cap0 = g_capri_state_pd->cap_top();
    cap0.rpc.pics.cfg_stage_id.all(0x688FAC);
    cap0.rpc.pics.cfg_stage_id.write();
    cap0.tpc.pics.cfg_stage_id.all(0x688FAC);
    cap0.tpc.pics.cfg_stage_id.write();
}

static inline bool
p4plus_invalidate_cache_aligned(uint64_t addr, uint32_t size_in_bytes,
        p4plus_cache_action_t action)
{
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();

    assert ((addr & ~CACHE_LINE_SIZE_MASK) == addr);

    while ((int)size_in_bytes > 0) {
        if (action & p4plus_cache_action_t::P4PLUS_CACHE_INVALIDATE_RXDMA) {
            cap_pics_csr_t & pics_csr = cap0.rpc.pics;
            pics_csr.picc.dhs_cache_invalidate.entry.addr(addr >> CACHE_LINE_SIZE_SHIFT);
            pics_csr.picc.dhs_cache_invalidate.entry.write();
        }
        if (action & p4plus_cache_action_t::P4PLUS_CACHE_INVALIDATE_TXDMA) {
            cap_pics_csr_t & pics_csr = cap0.tpc.pics;
            pics_csr.picc.dhs_cache_invalidate.entry.addr(addr >> CACHE_LINE_SIZE_SHIFT);
            pics_csr.picc.dhs_cache_invalidate.entry.write();
        }
        size_in_bytes -= CACHE_LINE_SIZE;
        addr += CACHE_LINE_SIZE;
    }

    return true;
}

bool
p4plus_invalidate_cache(uint64_t addr, uint32_t size_in_bytes,
        p4plus_cache_action_t action)
{
    bool ret;

    if ((addr & ~CACHE_LINE_SIZE_MASK) == addr) {
        ret = p4plus_invalidate_cache_aligned(addr, size_in_bytes, action);
    } else {
        int unalign_size = addr & CACHE_LINE_SIZE_MASK;
        ret = p4plus_invalidate_cache_aligned(addr & ~CACHE_LINE_SIZE_MASK,
                                        size_in_bytes + unalign_size,
                                        action);
    }

    return ret;
}


void
capri_deparser_init(int tm_port_ingress, int tm_port_egress) {
    cap_top_csr_t &cap0 = g_capri_state_pd->cap_top();
    cpp_int recirc_rw_bm = 0;
    // Ingress deparser is indexed with 1
    cap0.dpr.dpr[1].cfg_global_2.read();
    cap0.dpr.dpr[1].cfg_global_2.increment_recirc_cnt_en(1);
    cap0.dpr.dpr[1].cfg_global_2.drop_max_recirc_cnt(1);
    // Drop after 4 recircs
    cap0.dpr.dpr[1].cfg_global_2.max_recirc_cnt(4);
    cap0.dpr.dpr[1].cfg_global_2.recirc_oport(tm_port_ingress);
    cap0.dpr.dpr[1].cfg_global_2.clear_recirc_bit_en(1);
    recirc_rw_bm |= 1<<tm_port_ingress;
    recirc_rw_bm |= 1<<tm_port_egress;
    cap0.dpr.dpr[1].cfg_global_2.recirc_rw_bm(recirc_rw_bm);
    cap0.dpr.dpr[1].cfg_global_2.write();
    // Egress deparser is indexed with 0
    cap0.dpr.dpr[0].cfg_global_2.read();
    cap0.dpr.dpr[0].cfg_global_2.increment_recirc_cnt_en(0);
    cap0.dpr.dpr[0].cfg_global_2.write();
}

static void
capri_mpu_icache_invalidate (void)
{
    int i;
    cap_top_csr_t &cap0 = g_capri_state_pd->cap_top();
    for (i = 0; i < CAPRI_P4_NUM_STAGES; i++) {
        cap0.sgi.mpu[i].icache.read();
        cap0.sgi.mpu[i].icache.invalidate(1);
        cap0.sgi.mpu[i].icache.write();
        cap0.sge.mpu[i].icache.read();
        cap0.sge.mpu[i].icache.invalidate(1);
        cap0.sge.mpu[i].icache.write();
    }
    for (i = 0; i < CAPRI_P4PLUS_NUM_STAGES; i++) {
        cap0.pcr.mpu[i].icache.read();
        cap0.pcr.mpu[i].icache.invalidate(1);
        cap0.pcr.mpu[i].icache.write();
        cap0.pct.mpu[i].icache.read();
        cap0.pct.mpu[i].icache.invalidate(1);
        cap0.pct.mpu[i].icache.write();
    }
}

/*
 * Reset tcam memories
 */
static void
capri_tcam_memory_init (capri_cfg_t *capri_cfg)
{
    if (!capri_cfg ||
        ((capri_cfg->platform != platform_type_t::PLATFORM_TYPE_HAPS) &&
        (capri_cfg->platform != platform_type_t::PLATFORM_TYPE_HW))) {
        return;
    }

    cap_pict_zero_init_tcam(0, 0, 8);
    cap_pict_zero_init_tcam(0, 1, 4);
}

void
capri_table_rw_cleanup (void)
{
    for (int i = P4_GRESS_INGRESS; i <= P4_GRESS_EGRESS; i++) {
        if (g_shadow_sram_p4[i]) {
            CAPRI_FREE(g_shadow_sram_p4[i]);
        }
        g_shadow_sram_p4[i] = NULL;
        if (g_shadow_tcam_p4[i]) {
            CAPRI_FREE(g_shadow_tcam_p4[i]);
        }
        g_shadow_tcam_p4[i] = NULL;
    }
}

static int
capri_p4_shadow_init (void)
{
    for (int i = P4_GRESS_INGRESS; i <= P4_GRESS_EGRESS; i++) {
        g_shadow_sram_p4[i] = (capri_sram_shadow_mem_t*)
                CAPRI_CALLOC(1, sizeof(capri_sram_shadow_mem_t));
        g_shadow_tcam_p4[i] = (capri_tcam_shadow_mem_t*)
                CAPRI_CALLOC(1, sizeof(capri_tcam_shadow_mem_t));
        if (!g_shadow_sram_p4[i] || !g_shadow_tcam_p4[i]) {
            // TODO: Log error/trace
            capri_table_rw_cleanup();
            return CAPRI_FAIL;
        }
        // Initialize shadow tcam to match all ones. This makes all entries
        // to be treated as inactive
        memset(g_shadow_tcam_p4[i]->mem_x, 0xFF,
               sizeof(g_shadow_tcam_p4[i]->mem_x));
    }

    return CAPRI_OK;
}

static void
capri_sram_memory_init (capri_cfg_t *capri_cfg)
{
    if (!capri_cfg ||
        ((capri_cfg->platform != platform_type_t::PLATFORM_TYPE_HAPS) &&
        (capri_cfg->platform != platform_type_t::PLATFORM_TYPE_HW))) {
        return;
    }

    cap_pics_zero_init_sram(0, 0, 3);
    cap_pics_zero_init_sram(0, 1, 8);
    cap_pics_zero_init_sram(0, 2, 8);
    cap_pics_zero_init_sram(0, 3, 3);
}

void
capri_p4plus_table_rw_cleanup (void)
{
    if (g_shadow_sram_rxdma) {
        CAPRI_FREE(g_shadow_sram_rxdma);
    }
    if (g_shadow_sram_txdma) {
        CAPRI_FREE(g_shadow_sram_txdma);
    }
    g_shadow_sram_rxdma = NULL;
    g_shadow_sram_txdma = NULL;
}

static int
capri_p4plus_shadow_init (void)
{
    g_shadow_sram_rxdma = (capri_sram_shadow_mem_t*)CAPRI_CALLOC(1,
                                                                 sizeof(capri_sram_shadow_mem_t));
    g_shadow_sram_txdma = (capri_sram_shadow_mem_t*)CAPRI_CALLOC(1,
                                                                 sizeof(capri_sram_shadow_mem_t));

    if (!g_shadow_sram_rxdma || !g_shadow_sram_txdma) {
        // TODO: Log error/trace
        capri_p4plus_table_rw_cleanup();
        return CAPRI_FAIL;
    }

    return CAPRI_OK;
}

int
capri_table_rw_init (capri_cfg_t *capri_cfg)
{
    int ret;
    // !!!!!!
    // Before making this call, it is expected that
    // in HAL init sequence, p4pd_init() is already called..
    // !!!!!!
    /* 1. Create shadow memory and init to zero */

    ret = capri_p4_shadow_init();
    if (ret != CAPRI_OK) {
        return ret;
    }

    ret = capri_p4plus_shadow_init();
    if (ret != CAPRI_OK) {
        return ret;
    }

    /* Initialize stage id registers for p4p */
    capri_p4p_stage_id_init();

    hbm_mem_base_addr = get_mem_addr(MEM_REGION_P4_PROGRAM_NAME);

    capri_mpu_icache_invalidate();

    /* Initialize tcam memories */
    capri_tcam_memory_init(capri_cfg);

    /* Initialize sram memories */
    capri_sram_memory_init(capri_cfg);

    return (CAPRI_OK);
}

int    
capri_p4plus_table_rw_init (void)    
{    
    // !!!!!!    
    // Before making this call, it is expected that    
    // in HAL init sequence, p4pd_init() is already called..    
    // !!!!!!    
    capri_p4plus_shadow_init();    
    sdk::platform::capri::csr_init();    

     return (CAPRI_OK);    
}

uint8_t
capri_get_action_pc (uint32_t tableid, uint8_t actionid)
{
    if ((tableid >= p4pd_tableid_min_get()) &&
        (tableid <= p4pd_tableid_max_get())) {
        return ((uint8_t)capri_action_asm_base[tableid][actionid]);
    } else if ((tableid >= p4pd_rxdma_tableid_min_get()) &&
               (tableid <= p4pd_rxdma_tableid_max_get())) {
        uint32_t lcl_tableid = tableid - p4pd_rxdma_tableid_min_get();
        return ((uint8_t)capri_action_rxdma_asm_base[lcl_tableid][actionid]);
    } else if ((tableid >= p4pd_txdma_tableid_min_get()) &&
               (tableid <= p4pd_txdma_tableid_max_get())) {
        uint32_t lcl_tableid = tableid - p4pd_txdma_tableid_min_get();
        return ((uint8_t)capri_action_txdma_asm_base[lcl_tableid][actionid]);
    } else {
        SDK_ASSERT(0);
    }
}

uint8_t
capri_get_action_id (uint32_t tableid, uint8_t actionpc)
{
    if ((tableid >= p4pd_tableid_min_get()) &&
        (tableid <= p4pd_tableid_max_get())) {
        for (int j = 0; j < p4pd_get_max_action_id(tableid); j++) {
            if (capri_action_asm_base[tableid][j] == actionpc) {
                return j;
            }
        }
    } else if ((tableid >= p4pd_rxdma_tableid_min_get()) &&
               (tableid <= p4pd_rxdma_tableid_max_get())) {
        for (int j = 0; j < p4pd_rxdma_get_max_action_id(tableid); j++) {
            uint32_t lcl_tableid = tableid - p4pd_rxdma_tableid_min_get();
            if (capri_action_rxdma_asm_base[lcl_tableid][j] == actionpc) {
                return j;
            }
        }
    } else if ((tableid >= p4pd_txdma_tableid_min_get()) &&
               (tableid <= p4pd_txdma_tableid_max_get())) {
        for (int j = 0; j < p4pd_txdma_get_max_action_id(tableid); j++) {
            uint32_t lcl_tableid = tableid - p4pd_txdma_tableid_min_get();
            if (capri_action_txdma_asm_base[lcl_tableid][j] == actionpc) {
                return j;
            }
        }
    }
    return (0xff);
}



static void
capri_sram_entry_details_get (uint32_t index,
                              int *sram_row, int *entry_start_block,
                              int *entry_end_block, int *entry_start_word,
                              uint16_t top_left_x, uint16_t top_left_y,
                              uint8_t top_left_block, uint16_t btm_right_y,
                              uint8_t num_buckets, uint16_t entry_width)
{
    *sram_row = top_left_y + (index/num_buckets);
    assert(*sram_row <= btm_right_y);
    int tbl_col = index % num_buckets;
    /* entry_width is in units of SRAM word  -- 16b */

    *entry_start_word = (top_left_x + (tbl_col * entry_width))
                        % CAPRI_SRAM_WORDS_PER_BLOCK;
    /* Capri 16b word within a 128b block is numbered from right to left.*/
    //*entry_start_word = (CAPRI_SRAM_WORDS_PER_BLOCK - 1) - *entry_start_word;

    *entry_start_block = (top_left_block * CAPRI_SRAM_ROWS)
                         + ((((tbl_col * entry_width) + top_left_x)
                           / CAPRI_SRAM_WORDS_PER_BLOCK) * CAPRI_SRAM_ROWS)
                         + top_left_y + (index/num_buckets);

    *entry_end_block = *entry_start_block + (((entry_width - 1) +
                         (*entry_start_word % CAPRI_SRAM_WORDS_PER_BLOCK))
                         / CAPRI_SRAM_WORDS_PER_BLOCK) * CAPRI_SRAM_ROWS;

}

cap_pics_csr_t *
capri_global_pics_get (uint32_t tableid)
{
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    if ((tableid >= p4pd_tableid_min_get()) &&
        (tableid <= p4pd_tableid_max_get())) {
        return &cap0.ssi.pics;
    } else if ((tableid >= p4pd_rxdma_tableid_min_get()) &&
               (tableid <= p4pd_rxdma_tableid_max_get())) {
        return &cap0.rpc.pics;
    } else if ((tableid >= p4pd_txdma_tableid_min_get()) &&
               (tableid <= p4pd_txdma_tableid_max_get())) {
        return &cap0.tpc.pics;
    } else {
        SDK_ASSERT(0);
    }
    return ((cap_pics_csr_t*)nullptr);
}

int
capri_table_entry_write (uint32_t tableid,
                         uint32_t index,
                         uint8_t  *hwentry,
                         uint8_t  *hwentry_mask,
                         uint16_t hwentry_bit_len,
                         p4_table_mem_layout_t &tbl_info, int gress,
                         bool is_oflow_table, bool ingress,
                         uint32_t ofl_parent_tbl_depth)
{
    /* 1. When a Memory line is shared by multiple tables, only tableid's
     *    table entry bits need to be modified in the memory line.
     *    1. read Shadow memory line (entire 128bits in case of SRAM)
     *    2. clear out bits that corresponds to table.
     * 2. Argument hwentry contains byte stream that is already in format that
     *    agrees to Capri.
     *    Bytes read from match-table (SRAM or TCAM) are swizzled before
     *    comparing key bits. Today as per HW team, byte swizzing is
     *    Byte 0 in memory is Byte 63 in KeyMaker (512b keymaker)
     *    Byte 1 in memory is Byte 62 in KeyMaker (512b keymaker)
     *    :
     *    Byte 63 in memory is Byte 0 in KeyMaker (512b keymaker)
     * 3. Write all 128bits back to HW. In case of wide key write back
     *    multiple 128b blocks. When writing back all 128b blocks its
     *    possible to update neighbour table's entry back with same value
     *    as before.
     */

    // Assuming a table entry is contained within a SRAM row...
    // Entry cannot be wider than entire row (10 x 128bits)

    int sram_row, entry_start_block, entry_end_block;
    int entry_start_word;
    capri_sram_shadow_mem_t *shadow_sram;

    shadow_sram = get_sram_shadow_for_table(tableid, gress);

    // In case of overflow TCAM, SRAM associated with the table
    // is folded along with its parent's hash table.
    // Change index to parent table size + index
    if (is_oflow_table) {
        index += ofl_parent_tbl_depth;
    }

    capri_sram_entry_details_get(index,
                                 &sram_row, &entry_start_block,
                                 &entry_end_block, &entry_start_word,
                                 tbl_info.top_left_x,
                                 tbl_info.top_left_y,
                                 tbl_info.top_left_block,
                                 tbl_info.btm_right_y,
                                 tbl_info.num_buckets,
                                 tbl_info.entry_width);
    int tbl_col = index % tbl_info.num_buckets;
    int blk = tbl_info.top_left_block
                 + (((tbl_col * tbl_info.entry_width) + tbl_info.top_left_x) /
                     CAPRI_SRAM_WORDS_PER_BLOCK);
    int block = blk;
    int copy_bits = hwentry_bit_len;
    uint16_t *_hwentry = (uint16_t*)hwentry;

    if (hwentry_mask) {
        /* If mask is specified, it should encompass the entire macros currently */
        if ((entry_start_word != 0) || (tbl_info.entry_width % CAPRI_SRAM_WORDS_PER_BLOCK)) {
            SDK_TRACE_ERR("Masked write with entry_start_word %u and width %u "
                          "not supported",
                          entry_start_word, tbl_info.entry_width);
            return SDK_RET_INVALID_ARG;
        }
    }

    for (int j = 0; j < tbl_info.entry_width; j++) {
        if (copy_bits >= 16) {
            shadow_sram->mem[sram_row][block % CAPRI_SRAM_BLOCK_COUNT][entry_start_word] = *_hwentry;
            _hwentry++;
            copy_bits -= 16;
        } else if (copy_bits) {
            assert(0);
        }
        entry_start_word++;
        if (entry_start_word % CAPRI_SRAM_WORDS_PER_BLOCK == 0) {
            // crossed over to next block
            //block += CAPRI_SRAM_ROWS;
            block++;
            entry_start_word = 0;
        }
    }

    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    // Push to HW/Capri from entry_start_block to block
    pu_cpp_int<128> sram_block_data;
    pu_cpp_int<128> sram_block_datamask;
    uint8_t temp[16], tempmask[16];
    for (int i = entry_start_block; i <= entry_end_block; i += CAPRI_SRAM_ROWS, blk++) {
        //all shadow_sram->mem[sram_row][i] to be pushed to capri..
        uint8_t *s = (uint8_t*)(shadow_sram->mem[sram_row][blk]);
        for (int p = 15; p >= 0; p--) {
            temp[p] = *s; s++;
        }
        cap_pics_csr_t *pics_csr;
        if (ingress) {
            pics_csr = capri_global_pics_get(tableid);
        } else {
            pics_csr = &cap0.sse.pics;
        }
        sram_block_data = 0;
        cpp_int_helper::s_cpp_int_from_array(sram_block_data, 0, 15, temp);

        if (hwentry_mask) {
            uint8_t *m = hwentry_mask + (i-entry_start_block)*(CAPRI_SRAM_BLOCK_WIDTH>>3) ;
            for (int p = 15; p >= 0; p--) {
                tempmask[p] = *m; m++;
            }

            sram_block_datamask = 0;
            cpp_int_helper::s_cpp_int_from_array(sram_block_datamask, 0, 15, tempmask);

            pics_csr->dhs_sram_update_addr.entry.address(i);
            pics_csr->dhs_sram_update_addr.entry.write();
            pics_csr->dhs_sram_update_data.entry.data(sram_block_data);
            pics_csr->dhs_sram_update_data.entry.mask(sram_block_datamask);
            pics_csr->dhs_sram_update_data.entry.write();

        } else {
            pics_csr->dhs_sram.entry[i]
                .data((pu_cpp_int<128>)sram_block_data);
            pics_csr->dhs_sram.entry[i].write();
        }
    }
    return (CAPRI_OK);
}

int
capri_table_entry_read (uint32_t tableid,
                        uint32_t index,
                        uint8_t  *hwentry,
                        uint16_t *hwentry_bit_len,
                        p4_table_mem_layout_t &tbl_info, int gress,
                        bool is_oflow_table,
                        uint32_t ofl_parent_tbl_depth)
{
    /*
     * Unswizzing of the bytes into readable format is
     * expected to be done by caller of the API.
     */
    int sram_row, entry_start_block, entry_end_block;
    int entry_start_word;
    // In case of overflow TCAM, SRAM associated with the table
    // is folded along with its parent's hash table.
    // Change index to parent table size + index
    if (is_oflow_table) {
        index += ofl_parent_tbl_depth;
    }
    capri_sram_entry_details_get(index,
                                 &sram_row, &entry_start_block,
                                 &entry_end_block, &entry_start_word,
                                 tbl_info.top_left_x,
                                 tbl_info.top_left_y,
                                 tbl_info.top_left_block,
                                 tbl_info.btm_right_y,
                                 tbl_info.num_buckets,
                                 tbl_info.entry_width);
    int tbl_col = index % tbl_info.num_buckets;
    int blk = tbl_info.top_left_block
                 + (((tbl_col * tbl_info.entry_width) + tbl_info.top_left_x) /
                     CAPRI_SRAM_WORDS_PER_BLOCK);
    int block = blk;
    int copy_bits = tbl_info.entry_width_bits;
    uint16_t *_hwentry = (uint16_t*)hwentry;

    capri_sram_shadow_mem_t *shadow_sram;

    shadow_sram = get_sram_shadow_for_table(tableid, gress);

    while(copy_bits) {
        if (copy_bits >= 16) {
            *_hwentry = shadow_sram->mem[sram_row][block % CAPRI_SRAM_BLOCK_COUNT][entry_start_word];
            _hwentry++;
            copy_bits -= 16;
        } else {
            if (copy_bits > 8) {
                *_hwentry = shadow_sram->mem[sram_row][block % CAPRI_SRAM_BLOCK_COUNT][entry_start_word];
            } else {
                *(uint8_t*)_hwentry =
                    shadow_sram->mem[sram_row][block % CAPRI_SRAM_BLOCK_COUNT][entry_start_word] >> 8;
            }
            copy_bits = 0;
        }
        entry_start_word++;
        if (entry_start_word % CAPRI_SRAM_WORDS_PER_BLOCK == 0) {
            // crossed over to next block
            block++;
            entry_start_word = 0;
        }
    }

    *hwentry_bit_len = tbl_info.entry_width_bits;

    return CAPRI_OK;
}

int
capri_table_hw_entry_read (uint32_t tableid,
                           uint32_t index,
                           uint8_t  *hwentry,
                           uint16_t *hwentry_bit_len,
                           p4_table_mem_layout_t &tbl_info, int gress,
                           bool is_oflow_table, bool ingress,
                           uint32_t ofl_parent_tbl_depth)
{
    /*
     * Unswizzing of the bytes into readable format is
     * expected to be done by caller of the API.
     */
    int sram_row, entry_start_block, entry_end_block;
    int entry_start_word;
    // In case of overflow TCAM, SRAM associated with the table
    // is folded along with its parent's hash table.
    // Change index to parent table size + index
    if (is_oflow_table) {
        index += ofl_parent_tbl_depth;
    }
    capri_sram_entry_details_get(index,
                                 &sram_row, &entry_start_block,
                                 &entry_end_block, &entry_start_word,
                                 tbl_info.top_left_x,
                                 tbl_info.top_left_y,
                                 tbl_info.top_left_block,
                                 tbl_info.btm_right_y,
                                 tbl_info.num_buckets,
                                 tbl_info.entry_width);
    int copy_bits = tbl_info.entry_width_bits;
    uint8_t *_hwentry = (uint8_t*)hwentry;
    uint8_t  byte, to_copy;
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    // read from HW/Capri from entry_start_block to block
    cpp_int sram_block_data;
    uint8_t temp[16];
    for (int i = entry_start_block; (i <= entry_end_block) && (copy_bits > 0);
         i += CAPRI_SRAM_ROWS) {
        if (ingress) {
            cap_pics_csr_t *pics_csr = capri_global_pics_get(tableid);
            pics_csr->dhs_sram.entry[i].read();
            sram_block_data = pics_csr->dhs_sram.entry[i].data();
            cpp_int_helper::s_array_from_cpp_int(sram_block_data, 0, 15, temp);
        } else {
            cap_pics_csr_t & pics_csr = cap0.sse.pics;
            pics_csr.dhs_sram.entry[i].read();
            sram_block_data = pics_csr.dhs_sram.entry[i].data();
            cpp_int_helper::s_array_from_cpp_int(sram_block_data, 0, 15, temp);
        }
        for (int p = 15; p >= 8; p--) {
            byte = temp[p];
            temp[p] = temp[15-p];
            temp[15-p] = byte;
        }
        if (entry_start_word) {
            int bits_in_macro = 128 - (entry_start_word * 16);
            to_copy = (copy_bits > bits_in_macro) ? bits_in_macro : copy_bits;
        } else {
            to_copy = (copy_bits > 128) ? 128 : copy_bits;
        }
        uint8_t to_copy_bytes = ((((to_copy - 1) >> 4) + 1) << 4) / 8;
        memcpy(_hwentry, temp + (entry_start_word << 1), to_copy_bytes);
        copy_bits -= to_copy;
        _hwentry += to_copy_bytes;
        entry_start_word = 0;
    }
    *hwentry_bit_len = tbl_info.entry_width_bits;
    return (CAPRI_OK);
}

/*
 *  CAPRI TCAM ADDRESSING:
 *
 *       +====================================================================+
 * Row0  | V | Block0 (128b value,mask) | .... | V | Block7 (128b value,mask) |
 *       +--------------------------------------------------------------------+
 * Row1  | V | Block0 (128b value,mask) | .... | V | Block7 (128b value,mask) |
 *       +--------------------------------------------------------------------+
 *                              :
 *                              :
 *       +--------------------------------------------------------------------+
 * Row   | V | Block0 (128b value,mask) | .... | V | Block7 (128b value,mask) |
 * 1K-1  +====================================================================+
 *
 *
 *     1. Any memory writes / reads are done in units of block. To update
 *        a table entry that is within one or more blocks, all such memory
 *        blocks on a particular Row will need to modified and written back
 *        to capri.
 *
 *     2. Table entry start and end on 16b boundary. Multiple such 16b words
 *        are updated or read from when performing table write or read.
 */
static void
capri_tcam_entry_details_get (uint32_t index,
                              int *tcam_row, int *entry_start_block,
                              int *entry_end_block, int *entry_start_word,
                              uint16_t top_left_y, uint8_t top_left_block,
                              uint16_t btm_right_y, uint8_t num_buckets,
                              uint16_t entry_width, uint32_t start_index)
{
    *tcam_row = top_left_y + (index/num_buckets);
    assert (*tcam_row <= btm_right_y);
    int tbl_col = index % num_buckets;

    /* entry_width is in units of TCAM word  -- 16b */
    /* Since every tcam table entry occupies one TCAM block */
    *entry_start_word = start_index % CAPRI_TCAM_WORDS_PER_BLOCK;
    /* Capri 16b word within a 128b block is numbered from right to left.*/
    //*entry_start_word = (CAPRI_TCAM_WORDS_PER_BLOCK - 1) - *entry_start_word;

    /* Start block will be column away from top-left because in case of
     * tcam, atmost one entry/column of table can occupy a TCAM block.
     */
    *entry_start_block = ((top_left_block + tbl_col) * CAPRI_TCAM_ROWS)
                         + top_left_y
                         + (index/num_buckets);
    *entry_end_block = *entry_start_block + (((entry_width - 1) +
                         (*entry_start_word % CAPRI_TCAM_WORDS_PER_BLOCK))
                          / CAPRI_TCAM_WORDS_PER_BLOCK) * CAPRI_TCAM_ROWS;
}

int
capri_tcam_table_entry_write (uint32_t tableid,
                              uint32_t index,
                              uint8_t  *trit_x,
                              uint8_t  *trit_y,
                              uint16_t hwentry_bit_len,
                              p4_table_mem_layout_t &tbl_info,
                              int gress, bool ingress)
{
    /* 1. When a Memory line is shared by multiple tables, only tableid's
     *    table entry bits need to be modified in the memory line.
     *    1. read Shadow memory line (entire 64bits)
     *    2. clear out bits that corresponds to table
     * 2. Argument trit_x contains key byte stream that is already in format
     *    that agrees to Capri. trit_y contains mask byte stream that is
     *    already in format
     *    Bytes read from match-table (TCAM) are swizzled before
     *    comparing key bits. Today as per HW team, byte swizzing is
     *    Byte 0 in memory is Byte 63 in KeyMaker (512b keymaker)
     *    Byte 1 in memory is Byte 62 in KeyMaker (512b keymaker)
     *    :
     *    Byte 63 in memory is Byte 0 in KeyMaker (512b keymaker)
     * 3. Write all 64bits key,mask back to HW. In case of wide key write back
     *    multiple 64b blocks. When writing back all 64b blocks its
     *    possible to update neighbour table's entry back with same value
     *    as before.
     */
    int tcam_row, entry_start_block, entry_end_block;
    int entry_start_word;

    capri_tcam_entry_details_get(index,
                                 &tcam_row, &entry_start_block,
                                 &entry_end_block, &entry_start_word,
                                 tbl_info.top_left_y,
                                 tbl_info.top_left_block,
                                 tbl_info.btm_right_y,
                                 tbl_info.num_buckets,
                                 tbl_info.entry_width,
                                 tbl_info.start_index);
    int tbl_col = index % tbl_info.num_buckets;
    int blk = tbl_info.top_left_block
                 + ((tbl_col * tbl_info.entry_width) /
                     CAPRI_TCAM_WORDS_PER_BLOCK);
    int block = blk;
    int copy_bits = hwentry_bit_len;
    uint16_t *_trit_x = (uint16_t*)trit_x;
    uint16_t *_trit_y = (uint16_t*)trit_y;
    for (int j = 0; j < tbl_info.entry_width; j++) {
        if (copy_bits >= 16) {
            g_shadow_tcam_p4[gress]->mem_x[tcam_row]
                [block % CAPRI_TCAM_BLOCK_COUNT][entry_start_word] = *_trit_x;
            g_shadow_tcam_p4[gress]->mem_y[tcam_row]
                [block % CAPRI_TCAM_BLOCK_COUNT][entry_start_word] = *_trit_y;
            _trit_x++;
            _trit_y++;
            copy_bits -= 16;
        } else if (copy_bits) {
            assert(0);
        } else {
            // do not match remaining bits from end of entry bits to next 16b
            // aligned word
            g_shadow_tcam_p4[gress]->mem_x[tcam_row]
                [block % CAPRI_TCAM_BLOCK_COUNT][entry_start_word] = 0;
            g_shadow_tcam_p4[gress]->mem_y[tcam_row]
                [block % CAPRI_TCAM_BLOCK_COUNT][entry_start_word] = 0;
        }
        entry_start_word++;
        if (entry_start_word % CAPRI_TCAM_WORDS_PER_BLOCK == 0) {
            // crossed over to next block
            block++;
            entry_start_word = 0;
        }
    }

    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    // Push to HW/Capri from entry_start_block to block
    pu_cpp_int<128> tcam_block_data_x;
    pu_cpp_int<128> tcam_block_data_y;
    uint8_t temp_x[16];
    uint8_t temp_y[16];
    for (int i = entry_start_block; i <= entry_end_block; i += CAPRI_TCAM_ROWS, blk++) {
        uint8_t *s = (uint8_t*)(g_shadow_tcam_p4[gress]->mem_x[tcam_row][blk]);
        for (int p = 15; p >= 0; p--) {
            temp_x[p] = *s; s++;
        }
        s = (uint8_t*)(g_shadow_tcam_p4[gress]->mem_y[tcam_row][blk]);
        for (int p = 15; p >= 0; p--) {
            temp_y[p] = *s; s++;
        }
        if (ingress) {
            cap_pict_csr_t & pict_csr = cap0.tsi.pict;
            tcam_block_data_x = 0;
            tcam_block_data_y = 0;
            cpp_int_helper::s_cpp_int_from_array(tcam_block_data_x, 0, 15, temp_x);
            cpp_int_helper::s_cpp_int_from_array(tcam_block_data_y, 0, 15, temp_y);
            pict_csr.dhs_tcam_xy.entry[i].x((pu_cpp_int<128>)tcam_block_data_x);
            pict_csr.dhs_tcam_xy.entry[i].y((pu_cpp_int<128>)tcam_block_data_y);
            pict_csr.dhs_tcam_xy.entry[i].valid(1);
            pict_csr.dhs_tcam_xy.entry[i].write();
        } else {
            cap_pict_csr_t & pict_csr = cap0.tse.pict;
            tcam_block_data_x = 0;
            tcam_block_data_y = 0;
            cpp_int_helper::s_cpp_int_from_array(tcam_block_data_x, 0, 15, temp_x);
            cpp_int_helper::s_cpp_int_from_array(tcam_block_data_y, 0, 15, temp_y);
            pict_csr.dhs_tcam_xy.entry[i].x((pu_cpp_int<128>)tcam_block_data_x);
            pict_csr.dhs_tcam_xy.entry[i].y((pu_cpp_int<128>)tcam_block_data_y);
            pict_csr.dhs_tcam_xy.entry[i].valid(1);
            pict_csr.dhs_tcam_xy.entry[i].write();
        }
    }
    return CAPRI_OK;
}

int
capri_tcam_table_entry_read (uint32_t tableid,
                             uint32_t index,
                             uint8_t  *trit_x,
                             uint8_t  *trit_y,
                             uint16_t *hwentry_bit_len,
                             p4_table_mem_layout_t &tbl_info,
                             int gress)
{
    int tcam_row, entry_start_block, entry_end_block;
    int entry_start_word;

    capri_tcam_entry_details_get(index,
                                &tcam_row, &entry_start_block,
                                &entry_end_block, &entry_start_word,
                                tbl_info.top_left_y,
                                tbl_info.top_left_block,
                                tbl_info.btm_right_y,
                                tbl_info.num_buckets,
                                tbl_info.entry_width,
                                tbl_info.start_index);
    int tbl_col = index % tbl_info.num_buckets;
    int blk = tbl_info.top_left_block
                 + ((tbl_col * tbl_info.entry_width) /
                     CAPRI_TCAM_WORDS_PER_BLOCK);
    int block = blk;
    int copy_bits = tbl_info.entry_width_bits;
    int start_word = entry_start_word;
    uint16_t *_trit_x = (uint16_t*)trit_x;
    uint16_t *_trit_y = (uint16_t*)trit_y;
    while(copy_bits) {
        if (copy_bits >= 16) {
            *_trit_x = g_shadow_tcam_p4[gress]->mem_x[tcam_row]
                [block%CAPRI_TCAM_BLOCK_COUNT][start_word];
            *_trit_y = g_shadow_tcam_p4[gress]->mem_y[tcam_row]
                [block%CAPRI_TCAM_BLOCK_COUNT][start_word];
            _trit_x++;
            _trit_y++;
            copy_bits -= 16;
        } else if (copy_bits) {
            if (copy_bits > 8) {
                *_trit_x = g_shadow_tcam_p4[gress]->mem_x[tcam_row]
                    [block%CAPRI_TCAM_BLOCK_COUNT][start_word];
                *_trit_y = g_shadow_tcam_p4[gress]->mem_y[tcam_row]
                    [block%CAPRI_TCAM_BLOCK_COUNT][start_word];
            } else {
                *(uint8_t*)_trit_x = g_shadow_tcam_p4[gress]->mem_x[tcam_row]
                    [block%CAPRI_TCAM_BLOCK_COUNT][start_word] >> 8;
                *(uint8_t*)_trit_y = g_shadow_tcam_p4[gress]->mem_y[tcam_row]
                    [block%CAPRI_TCAM_BLOCK_COUNT][start_word] >> 8;
            }
            copy_bits = 0;
        }
        start_word++;
        if (start_word % CAPRI_TCAM_WORDS_PER_BLOCK == 0) {
            // crossed over to next block
            block++;
            start_word = 0;
        }
    }

    *hwentry_bit_len = tbl_info.entry_width_bits;;

    return CAPRI_OK;
}

int
capri_tcam_table_hw_entry_read (uint32_t tableid,
                                uint32_t index,
                                uint8_t  *trit_x,
                                uint8_t  *trit_y,
                                uint16_t *hwentry_bit_len,
                                p4_table_mem_layout_t &tbl_info,
                                bool ingress)
{
    int tcam_row, entry_start_block, entry_end_block;
    int entry_start_word;

    capri_tcam_entry_details_get(index,
                                 &tcam_row, &entry_start_block,
                                 &entry_end_block, &entry_start_word,
                                 tbl_info.top_left_y,
                                 tbl_info.top_left_block,
                                 tbl_info.btm_right_y,
                                 tbl_info.num_buckets,
                                 tbl_info.entry_width,
                                 tbl_info.start_index);
    int copy_bits = tbl_info.entry_width_bits;
    uint8_t byte, to_copy;
    uint8_t *_trit_x = (uint8_t*)trit_x;
    uint8_t *_trit_y = (uint8_t*)trit_y;

    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    // Push to HW/Capri from entry_start_block to block
    cpp_int tcam_block_data_x;
    cpp_int tcam_block_data_y;
    uint8_t temp_x[16];
    uint8_t temp_y[16];
    for (int i = entry_start_block; (i <= entry_end_block) && (copy_bits > 0);
         i += CAPRI_TCAM_ROWS) {
        if (ingress) {
            cap_pict_csr_t & pict_csr = cap0.tsi.pict;
            pict_csr.dhs_tcam_xy.entry[i].read();
            tcam_block_data_x = pict_csr.dhs_tcam_xy.entry[i].x();
            tcam_block_data_y = pict_csr.dhs_tcam_xy.entry[i].y();
            cpp_int_helper::s_array_from_cpp_int(tcam_block_data_x, 0, 15, temp_x);
            cpp_int_helper::s_array_from_cpp_int(tcam_block_data_y, 0, 15, temp_y);
        } else {
            cap_pict_csr_t & pict_csr = cap0.tse.pict;
            pict_csr.dhs_tcam_xy.entry[i].read();
            tcam_block_data_x = pict_csr.dhs_tcam_xy.entry[i].x();
            tcam_block_data_y = pict_csr.dhs_tcam_xy.entry[i].y();
            cpp_int_helper::s_array_from_cpp_int(tcam_block_data_x, 0, 15, temp_x);
            cpp_int_helper::s_array_from_cpp_int(tcam_block_data_y, 0, 15, temp_y);
        }
        for (int p = 15; p >= 8; p--) {
            byte = temp_x[p];
            temp_x[p] = temp_x[15-p];
            temp_x[15-p] = byte;
        }
        for (int p = 15; p >= 8; p--) {
            byte = temp_y[p];
            temp_y[p] = temp_y[15-p];
            temp_y[15-p] = byte;
        }
        if (entry_start_word) {
            int bits_in_macro = 128 - (entry_start_word * 16);
            to_copy = (copy_bits > bits_in_macro) ? bits_in_macro : copy_bits;
        } else {
            to_copy = (copy_bits > 128) ? 128: copy_bits;
        }
        uint8_t to_copy_bytes = ((((to_copy - 1) >> 4) + 1) << 4) / 8;
        memcpy(_trit_x, temp_x + (entry_start_word << 1), to_copy_bytes);
        memcpy(_trit_y, temp_y + (entry_start_word << 1), to_copy_bytes);
        _trit_x += to_copy_bytes;
        _trit_y += to_copy_bytes;
        entry_start_word = 0;
        copy_bits -= to_copy;
    }
    *hwentry_bit_len = tbl_info.entry_width_bits;;

    return (CAPRI_OK);
}

int
capri_hbm_table_entry_write (uint32_t tableid,
                             uint32_t index,
                             uint8_t *hwentry,
                             uint16_t entry_size,
                             p4_table_mem_layout_t &tbl_info)
{
    time_profile_begin(sdk::utils::time_profile::CAPRI_HBM_TABLE_ENTRY_WRITE);
    assert((entry_size >> 3) <= tbl_info.entry_width);
    assert(index < tbl_info.tabledepth);
    uint64_t entry_start_addr = (index * tbl_info.entry_width);

    sdk::asic::asic_mem_write(get_mem_addr(tbl_info.tablename) + entry_start_addr,
                              hwentry, (entry_size >> 3));
    time_profile_end(sdk::utils::time_profile::CAPRI_HBM_TABLE_ENTRY_WRITE);
    return CAPRI_OK;
}

int
capri_hbm_table_entry_cache_invalidate (bool ingress,
                                        uint64_t entry_addr,
                                        p4_table_mem_layout_t &tbl_info)
{
    time_profile_begin(sdk::utils::time_profile::CAPRI_HBM_TABLE_ENTRY_CACHE_INVALIDATE);
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();

    if (ingress) {
        cap_pics_csr_t & pics_csr = cap0.ssi.pics;
        // write upper 28b of 34b hbm addr.
        pics_csr.picc.dhs_cache_invalidate.entry.addr((get_mem_addr(tbl_info.tablename) + entry_addr) >> 6);
        pics_csr.picc.dhs_cache_invalidate.entry.write();
    } else {
        cap_pics_csr_t & pics_csr = cap0.sse.pics;
        // write upper 28b of 34b hbm addr.
        pics_csr.picc.dhs_cache_invalidate.entry.addr((get_mem_addr(tbl_info.tablename) + entry_addr) >> 6);
        pics_csr.picc.dhs_cache_invalidate.entry.write();
    }
    time_profile_end(sdk::utils::time_profile::CAPRI_HBM_TABLE_ENTRY_CACHE_INVALIDATE);
    return CAPRI_OK;
}


int
capri_hbm_table_entry_read (uint32_t tableid,
                            uint32_t index,
                            uint8_t *hwentry,
                            uint16_t *entry_size,
                            p4_table_mem_layout_t &tbl_info)
{
    assert(index < tbl_info.tabledepth);
    uint64_t entry_start_addr = (index * tbl_info.entry_width);

    sdk::asic::asic_mem_read(get_mem_addr(tbl_info.tablename) + entry_start_addr,
                             hwentry, tbl_info.entry_width);
    *entry_size = tbl_info.entry_width;
    return CAPRI_OK;
}

int
capri_table_constant_write (uint64_t val, uint32_t stage,
                            uint32_t stage_tableid, bool ingress)
{
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    if (ingress) {
        cap_te_csr_t &te_csr = cap0.sgi.te[stage];
        te_csr.cfg_table_mpu_const[stage_tableid].value(val);
        te_csr.cfg_table_mpu_const[stage_tableid].write();
    } else {
        cap_te_csr_t &te_csr = cap0.sge.te[stage];
        te_csr.cfg_table_mpu_const[stage_tableid].value(val);
        te_csr.cfg_table_mpu_const[stage_tableid].write();
    }
    return CAPRI_OK;
}

int
capri_table_constant_read (uint64_t *val, uint32_t stage,
                           int stage_tableid, bool ingress)
{
    cap_top_csr_t & cap0 = g_capri_state_pd->cap_top();
    if (ingress) {
        cap_te_csr_t &te_csr = cap0.sgi.te[stage];
        te_csr.cfg_table_mpu_const[stage_tableid].read();
        *val = te_csr.cfg_table_mpu_const[stage_tableid].
            value().convert_to<uint64_t>();
    } else {
        cap_te_csr_t &te_csr = cap0.sge.te[stage];
        te_csr.cfg_table_mpu_const[stage_tableid].read();
        *val = te_csr.cfg_table_mpu_const[stage_tableid].
            value().convert_to<uint64_t>();
    }
    return CAPRI_OK;
}

void
capri_set_action_asm_base (int tableid, int actionid,
                           uint64_t asm_base)
{
    capri_action_asm_base[tableid][actionid] = asm_base;
    return;
}

void
capri_set_action_rxdma_asm_base (int tableid, int actionid,
                                 uint64_t asm_base)
{
    uint32_t lcl_tableid = tableid - p4pd_rxdma_tableid_min_get();
    capri_action_rxdma_asm_base[lcl_tableid][actionid] = asm_base;
    return;
}

void
capri_set_action_txdma_asm_base (int tableid, int actionid,
                                 uint64_t asm_base)
{
    uint32_t lcl_tableid = tableid - p4pd_txdma_tableid_min_get();
    capri_action_txdma_asm_base[lcl_tableid][actionid] = asm_base;
    return;
}

void
capri_set_table_rxdma_asm_base (int tableid,
                                uint64_t asm_base)
{
    uint32_t lcl_tableid = tableid - p4pd_rxdma_tableid_min_get();
    capri_table_rxdma_asm_base[lcl_tableid] = asm_base;
    return;
}

void
capri_set_table_txdma_asm_base (int tableid,
                                uint64_t asm_base)
{
    uint32_t lcl_tableid = tableid - p4pd_txdma_tableid_min_get();
    capri_table_txdma_asm_base[lcl_tableid] = asm_base;
    return;
}

vector < tuple < string, string, string >>
asic_csr_dump_reg (char *block_name, bool exclude_mem)
{

    typedef vector< tuple< std::string, std::string, std::string> > reg_data;
    pen_csr_base *objP = get_csr_base_from_path(block_name);
    if (objP == 0) { SDK_TRACE_DEBUG("invalid reg name"); return reg_data(); };
    vector<pen_csr_base *> cap_child_base = objP->get_children(-1);
    reg_data data_tl;
    for (auto itr : cap_child_base) {
        if (itr->get_csr_type() == pen_csr_base::CSR_TYPE_REGISTER) {
            if(itr->get_parent() != nullptr && exclude_mem) {
                if (itr->get_parent()->get_csr_type() == pen_csr_base::CSR_TYPE_MEMORY) {
                    continue;
                }
            }
            // read name of register
            string name = itr->get_hier_path();
            // read data - same as csr_read()
            itr->read();
            cpp_int data = itr->all();
            stringstream ss;
            ss << hex << "0x" << data;

            // read offset
            uint64_t offset = itr->get_offset();
            stringstream addr;
            addr << hex << "0x" << offset;

            data_tl.push_back( tuple< std::string, string, std::string>(name, addr.str(), ss.str()));

        }
    }
    return data_tl;
}

vector <string>
asic_csr_list_get (string path, int level)
{
    pen_csr_base *objP = get_csr_base_from_path(path);
    vector <string> block_name;
    if (objP == 0) return vector<string>();
    for (auto itr : objP->get_children(level)) {
        block_name.push_back(itr->get_hier_path());
    }
    return block_name;
}

} // namespace capri
} // namespace platform
} // namespace sdk
