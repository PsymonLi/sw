#ifndef __HOSTMEM_HPP__
#define __HOSTMEM_HPP__

// TODO Need to move bm_allocator to utils.
#include "lib/bm_allocator/bm_allocator.hpp"

#include "nic/sdk/model_sim/include/host_mem_params.hpp"

namespace utils {

static const size_t kShmTopRegionSize = 12 * 64 * 1024 * 1024;

class HostMem {
 public:
  static HostMem *New(bool bhalf = false);
  ~HostMem();
  void *Alloc(size_t size, uint32_t align=1);
  void Free(void *ptr);
  uint64_t VirtToPhys(void *ptr);
  void *PhysToVirt(uint64_t);

 private:
  HostMem() {}

  int shmid_ = -1;
  void *shmaddr_ = nullptr;
  void *base_addr_;
  std::unique_ptr<sdk::lib::BMAllocator> allocator_;
 
  // allocations_ track the sizes of different allocations which
  // are then used to free the allocations.
  std::map<int, uint32_t> allocations_;
  int offset_ = 0;
};

}  // namespace utils

#endif
