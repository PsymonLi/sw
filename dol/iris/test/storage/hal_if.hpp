#ifndef _HAL_IF_HPP_
#define _HAL_IF_HPP_

#include <stdint.h>
#include "gen/proto/types.pb.h"
#include "logger.hpp"
#include "nic/hal/plugins/cfg/aclqos/barco_rings.hpp"

#define LIF_MAX_TYPES    8
#define LIF_BASE_QUEUE_SIZE  5

namespace hal_if {

typedef struct lif_params_ {
  uint32_t sw_lif_id;
  struct {
    bool valid;
    uint16_t queue_size; // Log2 value 
    uint16_t num_queues; // Log2 value
    int queue_purpose;
  } type[LIF_MAX_TYPES];

  // RDMA parameters
  bool rdma_enable;
  uint32_t rdma_max_keys;
  uint32_t rdma_max_ahs;
  uint32_t rdma_max_pt_entries;
} lif_params_t;

void init_hal_if();
int get_pgm_base_addr(const char *prog_name, uint64_t *base_addr);
int get_pgm_label_offset(const char *prog_name,
                         const char *label, uint8_t *off);
int set_lif_qstate(uint32_t lif, uint32_t qtype, uint32_t qid,
                   uint8_t *qstate);
int set_lif_qstate_size(uint32_t lif, uint32_t qtype, uint32_t qid,
                        uint8_t *qstate, uint32_t size);
int create_lif(lif_params_t *params, uint64_t *lif_id);
int set_lif_bdf(uint32_t hw_lif_id, uint32_t bdf_id);
int get_lif_info(uint32_t sw_lif_id, uint64_t *ret_hw_lif_id);
int get_lif_qstate_addr(uint32_t lif, uint32_t qtype, uint32_t qid,
                        uint64_t *qaddr);
int get_lif_qstate(uint32_t lif, uint32_t qtype, uint32_t qid, uint8_t *qstate);
int alloc_hbm_address(const char *handle, uint64_t *addr, uint32_t *size);
int get_xts_ring_base_address(bool is_decr, uint64_t *addr, bool is_gcm=false);
int get_ring_meta_config(types::BarcoRings ring_type,
                         hal::barco_ring_meta_config_t *meta);
int get_key_index(char* key, types::CryptoKeyType key_type, uint32_t key_size, uint32_t* key_index);
int delete_key(uint32_t key_index);
int get_asym_key_index(const uint8_t *key_desc, 
                       uint32_t key_size,
                       uint32_t *key_index);
int delete_asym_key(uint32_t key_index);
int get_xts_opaque_tag_addr(bool is_decr, uint64_t* addr, bool is_gcm=false);

}  // namespace hal_if

#endif
