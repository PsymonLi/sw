#ifndef __HAL_TEST_UTILS_HPP__
#define __HAL_TEST_UTILS_HPP__

#include "nic/hal/hal_trace.hpp"
#include "nic/include/hal_mem.hpp"
#include "nic/include/base.hpp"
#include "nic/sdk/lib/utils/mtrack.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "lib/slab/slab.hpp"

using hal::hal_slab_t;
using sdk::lib::slab_id_t;
using sdk::lib::slab;
using sdk::utils::mem_mgr;
using sdk::utils::mtrack_info_t;

#define PORT_NUM_1 0x11010001
#define PORT_NUM_2 0x11020001
#define PORT_NUM_3 0x11030001

#define UPLINK_IF_INDEX1 (0x01010001 | (IF_TYPE_UPLINK << IF_TYPE_SHIFT))
#define UPLINK_IF_INDEX2 (0x01020001 | (IF_TYPE_UPLINK << IF_TYPE_SHIFT))
#define UPLINK_IF_INDEX3 (0x01030001 | (IF_TYPE_UPLINK << IF_TYPE_SHIFT))

#define LOGICAL_PORT_NUM_1 1
#define LOGICAL_PORT_NUM_2 5
#define LOGICAL_PORT_NUM_3 9

typedef struct slab_stats_s {
    hal_slab_t  slab_id;
    uint32_t    num_in_use;
    uint32_t    num_allocs;
    uint32_t    num_frees;
} slab_stats_t;

slab *hal_test_utils_get_slab(hal_slab_t slab_id);
void hal_test_utils_populate (slab_stats_t *stats, slab *slab);
void hal_test_utils_trace(slab_stats_t *pre, slab_stats_t *post);

// Public APIs
// Slab related APIs
void hal_test_utils_slab_disable_delete(void);
slab_stats_t *hal_test_utils_collect_slab_stats();
void hal_test_utils_slab_stats_free(slab_stats_t *stats);
void hal_test_utils_check_slab_leak(slab_stats_t *pre, slab_stats_t *post,
                                    bool *is_leak);

// Mtrack related APIs
mtrack_info_t *hal_test_utils_collect_mtrack_stats();
void hal_test_utils_check_mtrack_leak(mtrack_info_t *pre,
                                      mtrack_info_t *post, bool *is_leak);
void hal_test_utils_mtrack_info_free(mtrack_info_t *minfo);

namespace hal {
namespace test {
void hal_test_preserve_state(void);
void hal_test_restore_state(void);
}
}

#endif // __HAL_TEST_UTILS_HPP__
