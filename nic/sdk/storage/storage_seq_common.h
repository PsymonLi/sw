/*
 * Copyright 2018-2019 Pensando Systems, Inc.  All rights reserved.
 */
 
#ifndef __STORAGE_SEQ_COMMON_H__
#define __STORAGE_SEQ_COMMON_H__

/**
 * Storage HBM handle as defined in nic/conf/iris/hbm_mem.json
 */
#define STORAGE_SEQ_HBM_HANDLE                  "storage"

/**
 * Storage Sequencer p4+ program names
 */
#define STORAGE_SEQ_PGM_NAME_SQ_GEN             "storage_seq_barco_entry_handler.bin"
#define STORAGE_SEQ_PGM_NAME_CPDC_STATUS0       "storage_seq_comp_status_desc0_handler.bin"
#define STORAGE_SEQ_PGM_NAME_CPDC_STATUS1       "storage_seq_comp_status_desc1_handler.bin"
#define STORAGE_SEQ_PGM_NAME_CRYPTO_STATUS0     "storage_seq_xts_status_desc0_handler.bin"
#define STORAGE_SEQ_PGM_NAME_CRYPTO_STATUS1     "storage_seq_xts_status_desc1_handler.bin"


/*
 * Storage LIF rate limit bytes units scale (down) factor in bytes.
 */
#define STORAGE_SEQ_RL_UNITS_SCALE_DFLT         1024
#define STORAGE_SEQ_RL_UNITS_SCALE_SHFT         10

#ifndef __STORAGE_ASM_DEFINES_H__

/**
 * storage_seq_qtype_t - Storage Sequencer qtype
 */
typedef enum storage_seq_qtype {
    STORAGE_SEQ_QTYPE_SQ        = 0,

    /*
     * STORAGE_SEQ_QTYPE_ADMIN must equal 2 to match lib_driver ADMIN queue type
     */
    STORAGE_SEQ_QTYPE_UNUSED    = 1,
    STORAGE_SEQ_QTYPE_ADMIN     = 2,
    STORAGE_SEQ_QTYPE_MAX
} __attribute__((packed)) storage_seq_qtype_t;


/**
 * storage_seq_wring_t - Storage Sequencer work ring
 */
typedef enum {
    STORAGE_SEQ_WRING0,
    STORAGE_SEQ_WRING_MAX
} __attribute__((packed)) storage_seq_wring_t;


/**
 * storage_seq_qgroup_t - Storage Sequencer queue group
 */
typedef enum {
    STORAGE_SEQ_QGROUP_CPDC,
    STORAGE_SEQ_QGROUP_CPDC_STATUS,
    STORAGE_SEQ_QGROUP_CRYPTO,
    STORAGE_SEQ_QGROUP_CRYPTO_STATUS,
    STORAGE_SEQ_QGROUP_MAX
} __attribute__((packed)) storage_seq_qgroup_t;


/**
 * storage_seq_metrics_t - Storage Sequencer queue metrics
 *   - Field types must meet delphi requirements
 *   - HW control block size must be a power of 2. For example, if the total
 *     number of metrics exceeds, say, 128 bytes, then the CB size must
 *     be rounded up to the next power of 2, which is 256 bytes.
 */
typedef struct storage_seq_qmetrics {
    uint64_t    metrics_0[8];
    uint64_t    metrics_1[8];
    uint64_t    metrics_2[8];
}  __attribute__((packed)) storage_seq_qmetrics_t;

/**
 * storage_seq_qstate_t - Storage Sequencer queue state
 */
typedef struct storage_seq_qstate {
    uint8_t     pc_offset;
    uint8_t     rsvd0;
    uint8_t     cosA        : 4,
                cosB        : 4;
    uint8_t     cos_sel;
    uint8_t     eval_last;
    uint8_t     host_wrings : 4,
                total_wrings: 4;
    uint16_t    pid;
    uint16_t    p_ndx;
    uint16_t    c_ndx;
    uint16_t    unused;
    uint16_t    wring_size;
    uint64_t    wring_base;
    uint16_t    entry_size;
    uint32_t    desc0_next_pc;
    uint32_t    desc1_next_pc;

    uint8_t     enable;
    uint8_t     abort;
    uint8_t     desc1_next_pc_valid;
    storage_seq_qgroup_t qgroup;
    uint16_t    core_id;
    uint8_t     pad[23];
    uint8_t     eop_p2m_rsvd;   // reserved for PHV2MEM eop cmd cancel write

    storage_seq_qmetrics_t metrics;

} __attribute__((packed)) storage_seq_qstate_t;

/*
 * HW control block (i.e., qstate) definitions
 */
#define HW_CB_SINGLE_SIZE                       64
#define HW_CB_SINGLE_SHFT                       6

#define HW_CB_MULTIPLE(cb_size_shft)            \
    ((cb_size_shft) - HW_CB_SINGLE_SHFT + 1)

#define HW_CB_MIN_MULTIPLE                      1

#define STORAGE_SEQ_CB_SIZE                     256
#define STORAGE_SEQ_CB_SIZE_SHFT                8       /* log2(STORAGE_SEQ_CB_SIZE) */

static_assert (sizeof(storage_seq_qstate_t) == STORAGE_SEQ_CB_SIZE, "");

#endif

#endif    // __STORAGE_SEQ_COMMON_H__
