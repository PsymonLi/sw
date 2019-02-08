#ifndef _QUEUES_HPP_
#define _QUEUES_HPP_

#include <stdint.h>
#include "dol/iris/test/storage/dp_mem.hpp"
#include "dol/iris/test/storage/chain_params.hpp"
#include "gflags/gflags.h"
#include "nic/include/accel_ring.h"

DECLARE_uint64(nicmgr_lif);

extern bool run_nicmgr_tests;

using namespace dp_mem;

#define SQ_TYPE		0
#define CQ_TYPE		1
#define HQ_TYPE		2
#define EQ_TYPE		3

namespace queues {

extern accel_ring_t nicmgr_accel_ring_tbl[];

/*
 * Sequencer descriptor
 */
typedef struct {
    uint64_t    acc_desc_addr;          // accelerator descriptor address
    uint64_t    acc_pndx_addr;          // accelerator producer index register address
    uint64_t    acc_pndx_shadow_addr;   // accelerator producer index shadow register address
    uint64_t    acc_ring_addr;          // accelerator ring address
    uint8_t     acc_desc_size;          // log2(accelerator descriptor size)
    uint8_t     acc_pndx_size;          // log2(accelerator producer index size)
    uint8_t     acc_ring_size;          // log2(accelerator ring size)
    uint8_t     rsvd0                : 4,
                acc_rate_limit_en    : 1,
                acc_rate_limit_dst_en: 1,
                acc_rate_limit_src_en: 1,
                acc_batch_mode       : 1;// acc_desc_addr is 1st descriptor of a batch
    uint16_t    acc_batch_size;         // number of batch descriptors
    uint16_t    filler0;
    uint32_t    acc_src_data_len;
    uint32_t    acc_dst_data_len;
    uint64_t    filler1[2];
} __attribute__((packed)) seq_desc_t;


// P4+ TxDMA max MEM2MEM transfer size is limited to 14 bits
// which influences how many (Barco) descriptors can be batch
// enqueued to a Sequencer queue.
#define QUEUE_BATCH_LIMIT(desc_size)        \
    ((tests::kMaxMem2MemSize) / (desc_size))

typedef void (*seq_sq_batch_end_notify_t)(void *user_ctx,
                                          dp_mem_t *seq_desc,
                                          uint16_t batch_size);

int nvme_e2e_ssd_handle();

void seq_queue_pdma_num_set(uint64_t& num_pdma_queues);

int resources_init();

int lifs_setup();

int nvme_pvm_queues_setup();

int nvme_dp_queues_setup();

int nvme_dp_update_cqs();

int pvm_queues_setup();

int seq_queues_setup();

void seq_queue_acc_sub_num_set(uint64_t& acc_scale_submissions,
                               uint64_t& acc_scale_chain_replica,
                               uint32_t acc_scale_test_max,
                               uint32_t acc_scale_test_num_true_chaining_tests);
int arm_queues_setup();

int pvm_roce_sq_init(uint16_t rsq_lif, uint16_t rsq_qtype, uint32_t rsq_qid, 
                 uint16_t rrq_lif, uint16_t rrq_qtype, uint32_t rrq_qid, 
                 dp_mem_t *mem, uint32_t num_entries, uint32_t entry_size,
                 uint64_t rrq_base_pa, uint8_t post_buf);
int pvm_roce_cq_init(uint16_t rcq_lif, uint16_t rcq_qtype, uint32_t rcq_qid, 
                     dp_mem_t *mem, uint32_t num_entries, uint32_t entry_size,
                     uint64_t xlate_addr);

dp_mem_t *nvme_sq_consume_entry(uint16_t qid, uint16_t *index);

dp_mem_t *pvm_sq_consume_entry(uint16_t qid, uint16_t *index);

dp_mem_t *seq_sq_consume_entry(uint16_t qid, uint16_t *index);

dp_mem_t *seq_sq_batch_consume_entry(uint16_t qid,
                                     uint64_t batch_id,
                                     uint16_t *index,
                                     bool *new_batch,
                                     seq_sq_batch_end_notify_t end_notify_fn,
                                     void *user_ctx);
void seq_sq_batch_consume_end(uint16_t qid);

dp_mem_t *nvme_cq_consume_entry(uint16_t qid, uint16_t *index);

dp_mem_t *pvm_cq_consume_entry(uint16_t qid, uint16_t *index);

dp_mem_t * seq_sq_consumed_entry_get(uint16_t qid, uint16_t *index);

uint16_t get_nvme_lif();

uint16_t get_pvm_lif();

uint16_t get_seq_lif();

uint16_t get_arm_lif();

uint32_t get_nvme_bdf();

uint32_t get_host_nvme_sq(uint32_t offset);

uint32_t get_pvm_nvme_sq(uint32_t offset);

uint32_t get_pvm_r2n_tgt_sq(uint32_t offset);

uint32_t get_pvm_r2n_init_sq(uint32_t offset);

uint32_t get_pvm_r2n_host_sq(uint32_t offset);

uint32_t get_pvm_nvme_be_sq(uint32_t offset);

uint32_t get_pvm_ssd_sq(uint32_t offset);

uint32_t get_seq_pdma_sq(uint32_t offset);

uint32_t get_seq_r2n_sq(uint32_t offset);

uint32_t get_seq_xts_sq(uint32_t offset);

uint32_t get_seq_xts_status_sq(uint32_t offset);

uint32_t get_seq_roce_sq(uint32_t offset);

uint32_t get_seq_comp_sq(uint32_t offset);

uint32_t get_seq_comp_status_sq(uint32_t offset);

uint32_t get_host_nvme_cq(uint32_t offset);

uint32_t get_pvm_nvme_cq(uint32_t offset);

uint32_t get_pvm_r2n_cq(uint32_t offset);

uint32_t get_pvm_nvme_be_cq(uint32_t offset);

void ring_nvme_e2e_ssd();

void get_nvme_doorbell(uint16_t lif, uint8_t qtype, uint32_t qid, 
                       uint8_t ring, uint16_t index, 
                       uint64_t *db_addr, uint64_t *db_data);

void get_host_doorbell(uint16_t lif, uint8_t qtype, uint32_t qid, 
                       uint8_t ring, uint16_t index, 
                       uint64_t *db_addr, uint64_t *db_data);

void get_capri_doorbell(uint16_t lif, uint8_t qtype, uint32_t qid, 
                        uint8_t ring, uint16_t index, 
                        uint64_t *db_addr, uint64_t *db_data);
void get_capri_doorbell_with_pndx_inc(uint16_t lif, uint8_t qtype, uint32_t qid, 
                                      uint8_t ring, uint64_t *db_addr, uint64_t *db_data);

uint64_t get_storage_hbm_addr();

}  // namespace queues

#endif
