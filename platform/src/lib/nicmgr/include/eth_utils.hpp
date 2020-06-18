//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines utility functions for eth devices
///
//----------------------------------------------------------------------------

#ifndef __ETH_UTILS_HPP__
#define __ETH_UTILS_HPP__

#include "nic/sdk/include/sdk/types.hpp"
#include "platform/src/lib/nicmgr/include/eth_if.h"

static inline enum ionic_port_oper_status
port_event_to_oper_status(port_event_t port_event)
{
    switch(port_event) {
    case port_event_t::PORT_EVENT_LINK_NONE:
        return ionic_port_oper_status::IONIC_PORT_OPER_STATUS_NONE;

    case port_event_t::PORT_EVENT_LINK_UP:
        return ionic_port_oper_status::IONIC_PORT_OPER_STATUS_UP;

    case port_event_t::PORT_EVENT_LINK_DOWN:
        return ionic_port_oper_status::IONIC_PORT_OPER_STATUS_DOWN;

    default:
        return ionic_port_oper_status::IONIC_PORT_OPER_STATUS_NONE;
    }
}

#endif /* __ETH_UTILS_HPP__ */
