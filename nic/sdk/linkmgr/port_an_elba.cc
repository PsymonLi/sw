//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Auto negotiation method definitions for Elba
///
//----------------------------------------------------------------------------

#include "port.hpp"

namespace sdk {
namespace linkmgr {

int
port::port_an_start (serdes_info_t *serdes_info)
{
    return mac_fns(port_type())->mac_an_start(this->mac_ch_, user_cap(),
                                              fec_ability(), fec_request());
}

bool
port::port_an_wait_hcd (void)
{
    return mac_fns(port_type())->mac_an_wait_hcd(this->mac_ch_);
}

int
port::port_an_hcd_read (void)
{
    return mac_fns(port_type())->mac_an_hcd_read(this->mac_ch_);
}

int
port::port_an_fec_enable_read (void)
{
    return mac_fns(port_type())->mac_an_fec_enable_read(this->mac_ch_);
}

int
port::port_an_rsfec_enable_read (void)
{
    return mac_fns(port_type())->mac_an_rsfec_enable_read(this->mac_ch_);
}

int
port::port_an_hcd_cfg (uint32_t an_hcd, uint32_t rx_term)
{
    return serdes_fns()->serdes_an_hcd_rxterm_cfg(port_sbus_addr(0),
                                                  this->sbus_addr_,
                                                  an_hcd, rx_term);
}

}    // namespace linkmgr
}    // namespace sdk
