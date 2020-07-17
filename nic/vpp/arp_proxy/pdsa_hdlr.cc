//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//
// Handlers for all messages from PDS Agent

#include <nic/vpp/infra/cfg/pdsa_db.hpp>
#include <nic/vpp/infra/ipc/pdsa_vpp_hdlr.h>
#include <nic/vpp/infra/ipc/pdsa_hdlr.hpp>
#include "pdsa_hdlr.h"

static void
pdsa_arp_device_cfg (const pds_cfg_msg_t *msg, bool del)
{
    pds_arp_device_cfg_change();
    return;
}

void
pdsa_arp_hdlr_init (void)
{
    pds_cfg_register_notify_callback(OBJ_ID_DEVICE,
                                     pdsa_arp_device_cfg);
}
