//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// implementation of pipeline impl methods
///
//----------------------------------------------------------------------------

#include "nic/apollo/framework/pipeline_impl_base.hpp"
#include "nic/apollo/api/impl/apulu/apulu_impl.hpp"

namespace api {
namespace impl {

/// \defgroup PDS_PIPELINE_IMPL Pipeline wrapper implementation
/// @{

pipeline_impl_base *
pipeline_impl_base::factory(pipeline_cfg_t *pipeline_cfg) {
    pipeline_impl_base *impl;
    impl = apulu_impl::factory(pipeline_cfg);
    impl->pipeline_cfg_ = *pipeline_cfg;
    return impl;
}

void
pipeline_impl_base::destroy(pipeline_impl_base *impl) {
    apulu_impl::destroy(static_cast<apulu_impl *>(impl));
}

sdk_ret_t
pipeline_impl_base::p4plus_write(uint64_t va, uint64_t pa, uint8_t *data,
                                 uint32_t size, p4plus_cache_action_t action) {
    sdk_ret_t ret = SDK_RET_OK;

    if (va) {
        memcpy((void *)va, (void *)data, size);
    } else {
        ret = sdk::asic::asic_mem_write(pa, data, size);
    }
    if (ret == SDK_RET_OK && action != P4PLUS_CACHE_ACTION_NONE) {
        if (!sdk::platform::capri::p4plus_invalidate_cache(pa, size, action)) {
            return SDK_RET_ERR;
        }
    }

    return ret;
}

sdk_ret_t
pipeline_impl_base::p4plus_read(uint64_t va, uint64_t pa, uint8_t *data,
                                uint32_t size) {
    if (va) {
        memcpy((void *)data, (void *)va, size);
    } else {
        return sdk::asic::asic_mem_read(pa, data, size);
    }
    return SDK_RET_OK;
}

/// \@}

}    // namespace impl
}    // namespace api
