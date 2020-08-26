/*
 * Copyright (c) 2019-2020, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <cinttypes>

#include "nic/sdk/platform/pal/include/pal.h"
#include "cmd.h"
#include "pcieutilpd.h"
#include "db.h"

#define DBF_QSTATE      0x1

static uint64_t
lif_qstate_map_addr(const int lif)
{
    return LIF_QSTATE_MAP_BASE + lif * LIF_QSTATE_MAP_STRIDE;
}

static void
lif_qstate_map_read(const int lif, lif_qstate_map_entry_t *qstmap)
{
    pal_reg_rd32w(lif_qstate_map_addr(lif), qstmap->w, LIF_QSTATE_MAP_NWORDS);
}

static unsigned int
qstate_raw_length(const lif_qstate_map_entry_t *qstmap, const int type)
{
    unsigned int length = 0;

    switch (type) {
    case 0: length = qstmap->length0; break;
    case 1: length = qstmap->length1; break;
    case 2: length = qstmap->length2; break;
    case 3: length = qstmap->length3; break;
    case 4: length = qstmap->length4; break;
    case 5: length = qstmap->length5; break;
    case 6: length = qstmap->length6; break;
    case 7: length = qstmap->length7; break;
    default: length = 0; break;
    }
    return length;
}

static unsigned int
qstate_raw_size(const lif_qstate_map_entry_t *qstmap, const int type)
{
    unsigned int size = 0;

    switch (type) {
    case 0: size = qstmap->size0; break;
    case 1: size = qstmap->size1; break;
    case 2: size = qstmap->size2; break;
    case 3: size = qstmap->size3; break;
    case 4: size = qstmap->size4; break;
    case 5: size = qstmap->size5; break;
    case 6: size = qstmap->size6; break;
    case 7: size = qstmap->size7; break;
    default: size = 0; break;
    }
    return size;
}

static unsigned int
qstate_length(const lif_qstate_map_entry_t *qstmap, const int type)
{
    return 1 << qstate_raw_length(qstmap, type);
}

static unsigned int
qstate_size(const lif_qstate_map_entry_t *qstmap, const int type)
{
    return 1 << (qstate_raw_size(qstmap, type) + 5);
}

static int
qstate_qinfo(const lif_qstate_map_entry_t *qstmap,
             const uint64_t addr,
             qstate_qinfo_t *qinfo)
{
    uint64_t base = (uint64_t)qstmap->qstate_base << 12;

    for (int type = 0; type < 8; type++) {
        const uint32_t qsize = qstate_size(qstmap, type);
        const uint32_t qcount = qstate_length(qstmap, type);
        const uint64_t qstate_end = base + qcount * qsize;

        if (base <= addr && addr < qstate_end) {
            const uint32_t qstate_offset = addr - base;

            qinfo->qtype = type;
            qinfo->qid = qstate_offset / qsize;
            return 1;
        }
        base = qstate_end;
    }
    return 0;
}

static int
lif_qstate_map_qinfo(const uint64_t qstate_addr,
                     qstate_qinfo_t *qinfo)
{
    lif_qstate_map_entry_t qstmap;
    int lif;

    for (lif = 0; lif < LIF_QSTATE_MAP_COUNT; lif++) {
        lif_qstate_map_read(lif, &qstmap);
        if (!qstmap.vld) continue;
        if (qstate_qinfo(&qstmap, qstate_addr, qinfo)) {
            qinfo->lif = lif;
            return 1;
        }
    }
    return 0;
}

static uint64_t
dberr_addr(const int entry)
{
    return DBERR_BASE + entry * DBERR_STRIDE;
}

static void
dberr_read(const int entry, db_err_activity_log_entry_t *dberr)
{
    pal_reg_rd32w(dberr_addr(entry), dberr->w, DBERR_NWORDS);
}

static void
dberr_display(db_err_activity_log_entry_t *dberr, const int flags)
{
    const uint64_t qstate_addr = (uint64_t)dberr->qstateaddr_or_qid << 5;
    qstate_qinfo_t qinfo;

    if ((flags & DBF_QSTATE) &&
        lif_qstate_map_qinfo(qstate_addr, &qinfo)) {
        dberrpd_display(dberr, &qinfo);
    } else {
        dberrpd_display(dberr, NULL);
    }
}

static void
dberr(int argc, char *argv[])
{
    db_err_activity_log_entry_t dberr;
    int opt, i, single_entry, flags;

    flags = 0;
    single_entry = 0;
    optind = 0;
    while ((opt = getopt(argc, argv, "e:q")) != -1) {
        switch (opt) {
        case 'e':
            dberr.w64 = strtoull(optarg, NULL, 0);
            single_entry = 1;
            break;
        case 'q':
            flags |= DBF_QSTATE;
            break;
        default:
            return;
        }
    }

    if (single_entry) {
        dberr_display(&dberr, flags);
    } else {
        for (i = 0; i < DBERR_COUNT; i++) {
            dberr_read(i, &dberr);
            if (!dberr.vld) continue;
            printf("\nentry %d (0x%" PRIx64 ")\n", i, dberr.w64);
            dberr_display(&dberr, flags);
        }
    }
}
CMDFUNC(dberr,
"display doorbell_err_activity_log entries",
"dberr [-q][-e <hexdigits>]\n"
"    -e <hexdigits>     decode raw entry (e.g. capview read)\n"
"    -q                 decode qstate addr/lif/qtype/qid\n");
