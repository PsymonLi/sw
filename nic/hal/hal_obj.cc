// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#include "nic/include/hal_mem.hpp"
#include "nic/include/hal_cfg.hpp"
#include "nic/include/hal_state.hpp"

namespace hal {

uint32_t
hal_default_marshall_cb (void *obj, uint8_t *mem, uint32_t len)
{
    return 0;
}

uint32_t
hal_default_unmarshall_cb (uint8_t *mem)
{
    return 0;
}

hal_obj_meta    *g_obj_meta[HAL_OBJ_ID_MAX];

void
hal_obj_meta_init (void)
{
    g_obj_meta[HAL_OBJ_ID_LIF] =
        new hal_obj_meta(HAL_SLAB_LIF,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_INTERFACE] =
        new hal_obj_meta(HAL_SLAB_IF,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_VRF] =
        new hal_obj_meta(HAL_SLAB_VRF,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_L2SEG] =
        new hal_obj_meta(HAL_SLAB_L2SEG,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_NETWORK] =
        new hal_obj_meta(HAL_SLAB_NETWORK,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_SECURITY_PROFILE] =
        new hal_obj_meta(HAL_SLAB_SECURITY_PROFILE,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_ENDPOINT] =
        new hal_obj_meta(HAL_SLAB_EP,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_SESSION] =
        new hal_obj_meta(HAL_SLAB_SESSION,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_ACL] =
        new hal_obj_meta(HAL_SLAB_ACL,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_QOS_CLASS] =
        new hal_obj_meta(HAL_SLAB_QOS_CLASS,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_COPP] =
        new hal_obj_meta(HAL_SLAB_COPP,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_MC_ENTRY] =
        new hal_obj_meta(HAL_SLAB_MC_ENTRY,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
}

}    // namespace hal
