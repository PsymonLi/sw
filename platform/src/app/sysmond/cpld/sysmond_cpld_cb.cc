//------------------------------------------------------------------------------
// Copyright (c) 2019 Pensando Systems, Inc.
//------------------------------------------------------------------------------
#include "sysmond_cpld_cb.hpp"
#include "platform/pal/include/pal.h"
#include "platform/src/app/sysmond/sysmond_cb.hpp"

#define PORT_1 0x11010001
#define PORT_2 0x11020001
#define PHY_PORT_1 1
#define PHY_PORT_2 2

void
cpld_liveness_event_cb (void)
{
    port::PortOperState port1_status;
    port::PortOperState port2_status;

    //TODO get the port across all pipelines
    port_get(PORT_1, &port1_status);
    port_get(PORT_2, &port2_status);
    pal_cpld_set_port_link_status(PHY_PORT_1, (uint8_t)port1_status);
    pal_cpld_set_port_link_status(PHY_PORT_2, (uint8_t)port2_status);

    return;
}
