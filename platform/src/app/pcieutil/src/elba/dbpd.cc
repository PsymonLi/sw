/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <cinttypes>

#include "db.h"

void
dberrpd_display(db_err_activity_log_entry_t *dberr, qstate_qinfo_t *qinfo)
{
    const int w = 40;

#define PDB(fmt, field) \
    printf("%-*s: " fmt "\n", w, #field, dberr->field)

    PDB("%" PRIu64, vld);
    if (qinfo) {
        const uint64_t qstate_addr = (uint64_t)dberr->qstateaddr_or_qid << 5;
        printf("%-*s: 0x%" PRIx64 " "
               "(qstateaddr 0x%" PRIx64 " lif %d qtype %d qid %d)\n",
               w, "qstateaddr_or_qid", dberr->qstateaddr_or_qid,
               qstate_addr, qinfo->lif, qinfo->qtype, qinfo->qid);
    } else {
        PDB("0x%" PRIx64, qstateaddr_or_qid);
    }
    PDB("0x%" PRIx64, pid_or_lif_type);
    PDB("%" PRIu64, cnt);
    PDB("%" PRIu64, doorbell_merged);
    PDB("%" PRIu64, addr_conflict_or_wqe_upd_or_size_err);
    PDB("%" PRIu64, tot_ring_err);
    PDB("%" PRIu64, host_ring_err);
    PDB("%" PRIu64, pid_or_wqe_addr_chk_fail);
    PDB("%" PRIu64, qid_ovflow);
}
