//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Base object definition for all interface impl objects
///
//----------------------------------------------------------------------------

#ifndef __IF_IMPL_BASE_HPP__
#define __IF_IMPL_BASE_HPP__


#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/framework/api.hpp"
#include "nic/apollo/framework/obj_base.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/impl_base.hpp"

namespace api {
namespace impl {

/// \brief Base class for if impl objects
class if_impl_base : public impl_base {
public:
    /// \brief track per second rate statistics
    /// \param[in]  interval    sampling interval in seconds
    /// #SDK_RET_OK on success, failure status code on error
    virtual sdk_ret_t track_pps(api_base *api_obj, uint32_t interval) {
        return sdk::SDK_RET_INVALID_OP;
    }

    /// \brief dump statistics to file descriptor
    /// \param[in]  fd          file descriptor
    virtual void dump_stats(api_base *api_obj, uint32_t fd) {
    }
};

}    // namespace impl
}    // namespace api

using api::impl::if_impl_base;

#endif    // __IF_IMPL_BASE_HPP__
