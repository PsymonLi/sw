//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation for displaying info from each queue
/// elbmon -q
///
//===----------------------------------------------------------------------===//

#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>

#include "elb_top_csr_defines.h"
#include "elb_wa_c_hdr.h"

#include "nic/sdk/platform/pal/include/pal.h"

using namespace std;

void
qstate_queue_dump (int verbose, int qid, int rid, int rsize, int poll,
                   u_int64_t qaddr)
{
    u_int8_t buf[64];

    typedef struct {
        uint8_t pc_offset;
        uint8_t rsvd0;
        uint8_t cosA : 4;
        uint8_t cosB : 4;
        uint8_t cos_sel;
        uint8_t eval_last;
        uint8_t host : 4;
        uint8_t total : 4;
        uint16_t pid;
        struct {
            uint16_t pi;
            uint16_t ci;
        } rings[8];
    } qstate_t;

    pal_mem_rd(qaddr, buf, 64, 0);
    qstate_t *q = (qstate_t *)buf;

    if (verbose) {
      printf("QID %u: ", qid);
      printf(" total_rings=%02d cosA=%02d cosB=%02d cos_sel=0x%02x pc=0x%02x",
             q->total, q->cosA, q->cosB, q->cos_sel, q->pc_offset);
      printf("\n");
    }
    for (uint8_t ring = 0; ring < q->total; ring++) {
      if (((rid == -1) || (rid == ring)) & (q->rings[ring].pi != q->rings[ring].ci)) {
          printf("QID %u: ", qid);
          printf(" ring %02d: PI=%06d CI=%06d", ring, q->rings[ring].pi,
                 q->rings[ring].ci);
          printf("\n");
      }
      if (rid == ring) {
          break;
      }
    }


    if ((poll > 0) && (q->total > 0)) {
        long long depth_avg[8] = {};
        int depth_max[8] = {};

        printf("Polling qid=%d rid=%d\n", qid, rid);
        for (int i = 0; i < poll; i++) {
            pal_mem_rd(qaddr, buf, 64, 0);
            q = (qstate_t *)buf;
            q->total = (q->total > 8) ? 8 : q->total;
            for (uint8_t ring = 0; ring < q->total; ring++) {
                int depth =
                    (q->rings[ring].pi + rsize - q->rings[ring].ci) % rsize;
                depth_avg[ring] += depth;
                if (depth > depth_max[ring]) {
                    depth_max[ring] = depth;
                }
            }
        }

        printf("%-14s%-14s%-14s\n", "RING", "AVERAGE", "MAX");
        for (uint8_t ring = 0; ring < q->total; ring++) {
            depth_avg[ring] = depth_avg[ring] / poll;
            if ((rid == -1) || (rid == ring)) {
                printf("%-14d%-14lld%-14d\n", ring, depth_avg[ring],
                       depth_max[ring]);
            }
        }
    }
}

void
qstate_qtype_dump (int queue_type, int qid_start, int qid_end, int rid,
                   int rsize, int poll, u_int64_t base, int len, int size,
                   int verbose)
{
    int q = 0;

    printf(
        " type %u type_base = 0x%lx, length=%u entries, qstate_size=%u bytes\n",
        queue_type, base, len, size);
    for (q = 0; q < len; q++) {
        if ((q >= qid_start) && (q <= qid_end)) {
            qstate_queue_dump(verbose, q, rid, rsize, poll,
                              base + (u_int64_t)(q * size));
        }
        if (q >= qid_end) {
            break;
        }
    }
}

void
qstate_lif_dump (int lif_start, int lif_end, int queue_type, int qid_start,
                 int qid_end, int rid, int rsize, int poll, int verbose)
{
    u_int32_t cnt[8], size[8], length[8];
    u_int32_t valid;
    u_int64_t base;
    int type, max_type;
    int this_size, this_len;
    int lif = 0;

    for (lif = lif_start; lif <= lif_end; lif++) {
        pal_reg_rd32w(ELB_ADDR_BASE_DB_WA_OFFSET +
                          ELB_WA_CSR_DHS_LIF_QSTATE_MAP_BYTE_ADDRESS +
                          (32 * lif),
                      cnt, 8);
        valid = ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_0_8_VLD_GET(cnt[0]);
        if (!valid) {
            continue;
        }

        // decode lif qstate table:
        base = (u_int64_t)
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_0_8_QSTATE_BASE_GET(
                cnt[0]);

        // 3 bit size is qstate size: 32B/64B/128B...
        size[0] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_0_8_SIZE0_0_0_GET(cnt[0]);
        size[1] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_1_8_SIZE1_GET(cnt[1]);
        size[2] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_1_8_SIZE2_GET(cnt[1]);
        size[3] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_1_8_SIZE3_GET(cnt[1]);
        size[4] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_1_8_SIZE4_0_0_GET(cnt[1]);
        size[5] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_2_8_SIZE5_GET(cnt[2]);
        size[6] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_2_8_SIZE6_GET(cnt[2]);
        size[7] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_2_8_SIZE7_GET(cnt[2]);
        // 5 bit length is lg2 # entries
        length[0] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_0_8_LENGTH0_GET(cnt[0]);
        length[1] =
	    ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_1_8_LENGTH1_GET(cnt[1]);
        length[2] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_1_8_LENGTH2_GET(cnt[1]);
        length[3] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_1_8_LENGTH3_GET(cnt[1]);
        length[4] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_1_8_LENGTH4_GET(cnt[1]);
        length[5] =
 	    ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_2_8_LENGTH5_GET(cnt[2]);
        length[6] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_2_8_LENGTH6_GET(cnt[2]);
        length[7] =
            ELB_WA_CSR_DHS_LIF_QSTATE_MAP_ENTRY_ENTRY_2_8_LENGTH7_GET(cnt[2]);

        for (max_type = 0; max_type < 8; max_type++) {
            if (size[max_type] == 0) {
                break;
            }
        }

        base = base << 12; // base is 4KB aligned
        printf("LIF %u valid, qstate_base=0x%lx\n", lif,
               base);

        for (type = 0; type < max_type; type++) {
            this_len = 1 << length[type];
            this_size = 32 * (1 << size[type]);
            if ((queue_type == -1) || (queue_type == type)) {
                qstate_qtype_dump(type, qid_start, qid_end, rid, rsize, poll,
                                  base, this_len, this_size, verbose);
            }
            if (queue_type == type) {
                break;
            }
            base += this_size * this_len;
        }

        if (queue_type >= max_type) {
            printf("Warning: Size of queue type %d is 0.\n", queue_type);
            return;
        }
    }
}
