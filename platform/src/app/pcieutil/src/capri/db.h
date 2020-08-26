/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#ifndef __CAPRI_DB_H__
#define __CAPRI_DB_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

#define DBERR_BASE \
    (ASIC_(ADDR_BASE_DB_WA_OFFSET) + \
     ASIC_(WA_CSR_DHS_DOORBELL_ERR_ACTIVITY_LOG_ENTRY_BYTE_ADDRESS))
#define DBERR_COUNT \
    ASIC_(WA_CSR_DHS_DOORBELL_ERR_ACTIVITY_LOG_ENTRY_ARRAY_COUNT)
#define DBERR_STRIDE            0x8
#define DBERR_NWORDS            2

typedef union {
    struct {
        uint64_t vld:1;
        uint64_t qstateaddr_or_qid:29;
        uint64_t pid_or_lif_type:16;
        uint64_t cnt:11;
        uint64_t doorbell_merged:1;
        uint64_t addr_conflict:1;
        uint64_t tot_ring_err:1;
        uint64_t host_ring_err:1;
        uint64_t pid_fail:1;
        uint64_t qid_ovflow:1;
    } __attribute__((packed));
    uint32_t w[DBERR_NWORDS];
    uint64_t w64;
} db_err_activity_log_entry_t;

#define LIF_QSTATE_MAP_BASE \
    (ASIC_(ADDR_BASE_DB_WA_OFFSET) + \
     ASIC_(WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_BYTE_ADDRESS))
#define LIF_QSTATE_MAP_COUNT \
    ASIC_(WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ARRAY_COUNT)
#define LIF_QSTATE_MAP_STRIDE   0x10
#define LIF_QSTATE_MAP_NWORDS   4

typedef union {
    struct {
        uint64_t vld:1;
        uint64_t qstate_base:22;
        uint64_t length0:5;
        uint64_t size0:3;
        uint64_t length1:5;
        uint64_t size1:3;
        uint64_t length2:5;
        uint64_t size2:3;
        uint64_t length3:5;
        uint64_t size3:3;
        uint64_t length4:5;
        uint64_t size4:3;
        uint64_t length5:5;
        uint64_t size5:3;
        uint64_t length6:5;
        uint64_t size6:3;
        uint64_t length7:5;
        uint64_t size7:3;
        uint64_t sched_hint_en:1;
        uint64_t sched_hint_cos:4;
        uint64_t spare:4;
        uint64_t ecc:8;
    } __attribute__((packed));
    uint32_t w[LIF_QSTATE_MAP_NWORDS];
} lif_qstate_map_entry_t;

typedef struct {
    int lif;
    int qtype;
    int qid;
} qstate_qinfo_t;

void
dberrpd_display(db_err_activity_log_entry_t *dberr, qstate_qinfo_t *qinfo);

#ifdef __cplusplus
}
#endif

#endif /* __CAPRI_DB_H__ */
