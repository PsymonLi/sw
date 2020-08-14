//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Auto negotiation method definitions for Capri
///
//----------------------------------------------------------------------------

#include "port.hpp"

namespace sdk {
namespace linkmgr {

int
port::port_an_start (serdes_info_t *serdes_info)
{
    return serdes_fns()->serdes_an_start(port_sbus_addr(0), serdes_info,
                                         user_cap(), fec_ability(),
                                         fec_request());
}

bool
port::port_an_wait_hcd (void)
{
    return serdes_fns()->serdes_an_wait_hcd(port_sbus_addr(0));
}

int
port::port_an_hcd_read (void)
{
    return serdes_fns()->serdes_an_hcd_read(port_sbus_addr(0));
}

int
port::port_an_fec_enable_read (void)
{
    return serdes_fns()->serdes_an_fec_enable_read(port_sbus_addr(0));
}

int
port::port_an_rsfec_enable_read (void)
{
    return serdes_fns()->serdes_an_rsfec_enable_read(port_sbus_addr(0));
}

int
port::port_an_hcd_cfg (uint32_t an_hcd, uint32_t rx_term)
{
    return serdes_fns()->serdes_an_hcd_cfg(port_sbus_addr(0), this->sbus_addr_);
}

}    // namespace linkmgr
}    // namespace sdk
