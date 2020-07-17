//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include <arp_proxy.h>

void
pds_arp_device_cfg_change (void)
{
    pds_arp_proxy_nacl_init();
}
