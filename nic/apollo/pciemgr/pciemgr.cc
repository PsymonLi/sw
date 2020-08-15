//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// pciemgr functionality
///
//----------------------------------------------------------------------------

#include <pthread.h>
#include "include/sdk/base.hpp"
#include "nic/sdk/lib/utils/port_utils.hpp"
#include "nic/sdk/platform/pciemgrd/pciemgrd.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/infra/core/event.hpp"
#include "nic/infra/core/core.hpp"
#include "nic/apollo/pciemgr/pciemgr.hpp"

/// \defgroup PDS_pciemgr
/// @{

void logger_init(void);

namespace pdspciemgr {

void *
pciemgrapi::pciemgr_thread_start(void *ctxt) {
    SDK_THREAD_INIT(ctxt);
    logger_init();
    PDS_TRACE_INFO("Initializing PCIe manager ...");
    pciemgrd_start();
    return NULL;
}

}    // namespace pciemgr

/// \@}

