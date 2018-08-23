#include "nic/include/base.hpp"
#include "sdk/thread.hpp"
#include "nic/include/hal.hpp"
#include "nic/include/hal_mem.hpp"
namespace hal {

uint64_t               hal_handle = 1;
thread                 *g_hal_threads[HAL_THREAD_ID_MAX];
bool                   gl_super_user = false;
thread_local thread    *t_curr_thread;
class hal_state        *g_hal_state;

slab *
hal_handle_slab(void)
{
    return NULL;
}

slab *
hal_handle_ht_entry_slab(void)
{
    return NULL;
}

ht *
hal_handle_id_ht (void)
{
    return NULL;
}

void
hal_handle_cfg_db_lock (bool readlock, bool lock)
{
    return;
}

hal_ret_t
free_to_slab (hal_slab_t slab_id, void *elem)
{
    return HAL_RET_OK;
}

hal_handle_t
hal_alloc_handle (void)
{
    return hal_handle++;
}

thread *
hal_get_current_thread (void)
{
    return t_curr_thread;
}

namespace pd {

hal_ret_t
free_to_slab (hal_slab_t slab_id, void *elem)
{
    return HAL_RET_OK;
}

}    // namespace pd
}    // namespace hal
