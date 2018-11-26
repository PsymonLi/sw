//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/include/hal_mem.hpp"
#include "nic/include/hal_cfg.hpp"
#include "nic/hal/iris/include/hal_state.hpp"
#include "nic/hal/plugins/cfg/nw/vrf.hpp"
#include "nic/hal/plugins/cfg/nw/nw.hpp"
#include "nic/hal/plugins/cfg/aclqos/acl.hpp"
#include "nic/hal/plugins/cfg/aclqos/qos.hpp"
#include "nic/hal/plugins/cfg/nw/l2segment.hpp"
#include "nic/hal/plugins/cfg/nw/endpoint.hpp"
#include "nic/hal/plugins/cfg/nw/interface.hpp"
#include "nic/hal/plugins/sfw/cfg/nwsec.hpp"

namespace hal {

hal_obj_meta    *g_obj_meta[HAL_OBJ_ID_MAX];

hal_ret_t
hal_default_marshall_cb (void *obj, uint8_t *mem, uint32_t len, uint32_t *mlen)
{
    HAL_ASSERT((obj != NULL) && (mlen != NULL));
    *mlen = 0;
    return HAL_RET_OK;
}

uint32_t
hal_default_unmarshall_cb (void *mem, uint32_t len)
{
    return 0;
}

void
hal_obj_meta_init (void)
{
    g_obj_meta[HAL_OBJ_ID_LIF] =
        new hal_obj_meta(HAL_SLAB_LIF,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_INTERFACE] =
        new hal_obj_meta(HAL_SLAB_IF,
                         if_store_cb,
                         if_restore_cb);
    g_obj_meta[HAL_OBJ_ID_VRF] =
        new hal_obj_meta(HAL_SLAB_VRF,
                         vrf_store_cb,
                         vrf_restore_cb);
    g_obj_meta[HAL_OBJ_ID_L2SEG] =
        new hal_obj_meta(HAL_SLAB_L2SEG,
                         l2seg_store_cb,
                         l2seg_restore_cb);
    g_obj_meta[HAL_OBJ_ID_NETWORK] =
        new hal_obj_meta(HAL_SLAB_NETWORK,
                         nw_store_cb,
                         nw_restore_cb);
    g_obj_meta[HAL_OBJ_ID_SECURITY_PROFILE] =
        new hal_obj_meta(HAL_SLAB_SECURITY_PROFILE,
                         nwsec_prof_store_cb,
                         nwsec_prof_restore_cb);
    g_obj_meta[HAL_OBJ_ID_ENDPOINT] =
        new hal_obj_meta(HAL_SLAB_EP,
                         ep_store_cb,
                         ep_restore_cb);
    g_obj_meta[HAL_OBJ_ID_SESSION] =
        new hal_obj_meta(HAL_SLAB_SESSION,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
    g_obj_meta[HAL_OBJ_ID_ACL] =
        new hal_obj_meta(HAL_SLAB_ACL,
                         acl_store_cb,
                         acl_restore_cb);
    g_obj_meta[HAL_OBJ_ID_QOS_CLASS] =
        new hal_obj_meta(HAL_SLAB_QOS_CLASS,
                         qos_class_store_cb,
                         qos_class_restore_cb);
    g_obj_meta[HAL_OBJ_ID_COPP] =
        new hal_obj_meta(HAL_SLAB_COPP,
                         copp_store_cb,
                         copp_restore_cb);
    g_obj_meta[HAL_OBJ_ID_MC_ENTRY] =
        new hal_obj_meta(HAL_SLAB_MC_ENTRY,
                         hal_default_marshall_cb,
                         hal_default_unmarshall_cb);
}

}    // namespace hal
