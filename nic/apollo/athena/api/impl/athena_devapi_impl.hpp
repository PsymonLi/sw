//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// apollo pipeline implementation of dev apis
///
//----------------------------------------------------------------------------

#ifndef __PDS_ATHENA_DEVAPI_IMPL_HPP__
#define __PDS_ATHENA_DEVAPI_IMPL_HPP__

#include "nic/apollo/api/impl/devapi_impl.hpp"

namespace api {
namespace impl {

using sdk::platform::devapi;
using api::impl::devapi_impl;

#define DEVAPI_IMPL_ADMIN_COS 1

// 14 bytes of eth , 20 bytes of IP, 8 bytes of UDP, 8 bytes of VxLAN headers
/// \defgroup PDS_DEVAPI_IMPL - dev api implementation
/// \ingroup PDS_DEVAPI
/// @{
class athena_devapi_impl : public devapi_impl {
public:
    // generic APIs
    static devapi *factory(void) {
        athena_devapi_impl    *impl;

        impl = (athena_devapi_impl *)SDK_CALLOC(SDK_MEM_ALLOC_DEVAPI_IMPL,
                                    sizeof(athena_devapi_impl));
        if (!impl) {
            return NULL;
        }
        new (impl) athena_devapi_impl();
        return impl;
    }
    virtual uint32_t max_encap_hdr_len(void) const override {
        return 0;
    }
    static void destroy(devapi *impl) {
        (dynamic_cast<athena_devapi_impl *>(impl))->~athena_devapi_impl();
        SDK_FREE(SDK_MEM_ALLOC_DEVAPI_IMPL, impl);
    }
private:
    athena_devapi_impl() {}
    ~athena_devapi_impl() {}

};

/// \@}

}    // namespace impl
}    // namespace api

#endif    // __PDS_DEVAPI_IMPL_HPP__
