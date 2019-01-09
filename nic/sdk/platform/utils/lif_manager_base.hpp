//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef _LIF_MANAGER_BASE_HPP_
#define _LIF_MANAGER_BASE_HPP_

#include "include/sdk/bm_allocator.hpp"

#include <set>
#include <map>
#include <mutex>
#include <strings.h>

namespace sdk {
namespace platform {
namespace utils {

const static uint32_t kNumQTypes = 8;
const static uint32_t kMaxQStateSize = 4096;

// Parameters for the InitLIFToQstate call.
struct LIFQStateParams {
  struct {
    uint8_t entries:5,
            size:3;
    uint8_t cosA:4,
            cosB:4;
  } type[kNumQTypes];
  bool dont_zero_memory;
};

// Per LIF queue state managed by the LIF Manager.
struct LIFQState {
  uint32_t lif_id;
  uint32_t allocation_size;   // Amount of HBM allocated.
  uint64_t hbm_address;       // Use uint64_t to support tests.
  LIFQStateParams params_in;  // A copy of user input.
  struct {                    // Per type data.
    uint32_t hbm_offset;
    uint32_t qsize;
    uint32_t rsvd;
    uint32_t num_queues;
    uint8_t  coses;            // stores cosA in bits 0-3, cosB in bits 4-7
  } type[kNumQTypes];
};

class LIFManagerBase {
 public:
  LIFManagerBase();

  // Allocates a range of LIFs.
  // If 'start' == -1, pick an appropriate starting point, else
  // allocate from 'start'
  // Returns:
  //    - A positive LIF number if the allocation suceeded.
  //    - -errno in case of failure.
  int32_t LIFRangeAlloc(int32_t start, uint32_t count);
  int32_t LIFRangeMarkAlloced(int32_t start, uint32_t count);

  // Delete a LIF. Also deletes any associated QState and disables LIF->QState
  // map entries. Also frees internal states.
  void DeleteLIF(int32_t hw_lif_id);

  // Initialize the LIF to Qstate Map in hw and allocate any
  // metadata.
  // Returns
  //   0 -      In case of success.
  //   -errno - In case of failure.
  int32_t InitLIFQState(uint32_t lif_id, LIFQStateParams *params, uint8_t hint_cos);

  int64_t GetLIFQStateBaseAddr(uint32_t, uint32_t);

  // Get the qstate address for a LIF, type and qid.
  // Returns:
  //   Positive address in case of success.
  //   -errno in case of failure.
  int64_t GetLIFQStateAddr(uint32_t lif_id, uint32_t type, uint32_t qid);

  // Return QState for a LIF/Type/QID. The user must pass in the buffer
  // to read the state in. Its ok to pass a buffer of smaller size (> 0).
  // If the passed in buffer is larger than the queue size, only queue size
  // worth of data will be read.
  // Returns:
  //   0     - success.
  //   -errno- failure.
  int32_t ReadQState(uint32_t lif_id, uint32_t type, uint32_t qid,
                     uint8_t *buf, uint32_t bufsize);

  // Set QState for a LIF/Type/QID. The user must pass in the buffer
  // to write the state. Its ok to pass a buffer of smaller size (> 0).
  // If the passed in buffer is larger than the queue size, the call will
  // fail.
  // Returns:
  //   0     - success.
  //   -errno- failure.
  int32_t WriteQState(uint32_t lif_id, uint32_t type, uint32_t qid,
                      uint8_t *buf, uint32_t bufsize);

  // GetPCOffset for a specific P4+ program.
  virtual int32_t GetPCOffset(
      const char *handle, const char *prog_name,
      const char *label, uint8_t *offset) = 0;

  LIFQState *GetLIFQState(uint32_t lif_id);

  // Implement later
  // void DestroyLIFToQstate(uint32_t lif_id);

 protected:
  // Interactions with Model/HW.
  // Initialize the LIF Qstate in hw and allocate any memory.
  // Returns
  //   0 -      In case of success.
  //   -errno - In case of failure.
  virtual int32_t InitLIFQStateImpl(LIFQState *qstate, int cos) = 0;

  virtual int32_t ReadQStateImpl(
      uint64_t q_addr, uint8_t *buf, uint32_t q_size) = 0;
  virtual int32_t WriteQStateImpl(
      uint64_t q_addr, const uint8_t *buf, uint32_t q_size) = 0;
  virtual void DeleteLIFQStateImpl(LIFQState *qstate) = 0;

 private:
  const uint32_t kNumMaxLIFs = 2048;


  std::mutex lk_;
  sdk::lib::BMAllocator lif_allocator_;
  std::map<uint32_t, LIFQState> alloced_lifs_;

  // Test only
  friend class MockLIFManager;
};

}   // namespace utils
}   // namespace platform
}   // namespace sdk

#endif  // _LIF_MANAGER_BASE_HPP_
