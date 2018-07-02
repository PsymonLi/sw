
#include "ingress.h"
#include "INGRESS_p.h"

/*
 * TESTCASE:
 *      single descriptor send (ci0 - pi0 == 1)
 *
 * ASSERTIONS:
 *      (Table)
 *      ci0 == ci0 + 1
 *      pi1 == pi1 + 1
 *      eval_db_cnt == eval_db_cnt + 1
 *
 *      (PHV)
 *      table0_valid == 1
 *      table0_lock_en == 0
 *      table0_pc == 0x3ffffff
 *      table0_addr == d.ring_base
 *      table0_size == 4
 *      num_desc == 1
 *      cq_desc_addr == d.cq_ring_base
 *      intr_assert_addr == d.intr_assert_addr
 *      intr_assert_data == INTR_ASSERT_CMD     (0x1000000)
 *      intr_global_drop == 0
 */

struct phv_ p;
struct tx_table_s0_t0_k k;
struct tx_table_s0_t0_eth_tx_fetch_desc_d d;

p = {
    app_header_table0_valid = 0;
    app_header_table1_valid = 0;
    app_header_table2_valid = 0;
    app_header_table3_valid = 0;
    common_te0_phv_table_lock_en = 0;
    common_te0_phv_table_pc = 0;
    common_te0_phv_table_addr = 0;
    common_te0_phv_table_raw_table_size = 0;
    p4_intr_global_drop = 0;
};

k = {
    p4_txdma_intr_qtype = 1;
    p4_txdma_intr_qid = 0;
};

d = {
    enable = 0x1;
    p_index0 = 0x0100;
    c_index0 = 0x0;
    comp_index = 0x0;
    ring_size = 0x0a00;
    ring_base = 0x8000000000010000;
    cq_ring_base = 0x8000000000020000;
    sg_ring_base = 0x8000000000030000;
#if 0
    eval_db_cnt = 0x0;
#endif
};
