/**
 * Copyright (c) 2018 Pensando Systems, Inc.
 *
 * @file    pipeline_impl_base.cc
 *
 * @brief   implementation of pipeline impl methods
 */

#include "nic/apollo/framework/pipeline_impl_base.hpp"
#include "nic/apollo/api/impl/apollo_impl.hpp"

namespace api {
namespace impl {

/**
 * @defgroup PDS_PIPELINE_IMPL - pipeline wrapper implementation
 * @ingroup PDS_PIPELINE
 * @{
 */

/**
 * @brief    factory method to pipeline impl instance
 * @param[in] pipeline_cfg    pipeline information
 * @return    new instance of pipeline impl or NULL, in case of error
 */
pipeline_impl_base *
pipeline_impl_base::factory(pipeline_cfg_t *pipeline_cfg) {
    if (pipeline_cfg->name == "apollo") {
        return apollo_impl::factory(pipeline_cfg);
    }
    return NULL;
}

/** @} */    // end of PDS_PIPELINE_IMPL

}    // namespace impl
}    // namespace api
