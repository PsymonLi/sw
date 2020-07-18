//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include <nic/apollo/api/impl/apulu/nacl_data.h>
#include <api.h>
#include "impl_db.h"

void
pds_dhcp_relay_nacl_init (void)
{
    pds_infra_api_reg_t params = {0};
    pds_impl_db_device_entry_t *dev;

    params.nacl_id = NACL_DATA_ID_FLOW_MISS_DHCP_HOST;
    params.unreg = true;
    if (0 != pds_register_nacl_id_to_node(&params)) {
        ASSERT(0);
    }
    params.nacl_id = NACL_DATA_ID_FLOW_MISS_DHCP_UPLINK;
    params.unreg = true;
    if (0 != pds_register_nacl_id_to_node(&params)) {
        ASSERT(0);
    }

    dev = pds_impl_db_device_get();
    if (dev->oper_mode == PDS_DEV_MODE_BITW_SMART_SERVICE) {
        // we don't expect dhcp packets in this mode
        return;
    }
    params.node = format(0, "pds-dhcp-relay-host-classify");
    params.nacl_id = NACL_DATA_ID_FLOW_MISS_DHCP_HOST;
    params.frame_queue_index = ~0;
    params.handoff_thread = ~0;
    params.offset = 0;
    params.unreg = false;
    if (0 != pds_register_nacl_id_to_node(&params)) {
        ASSERT(0);
    }
    vec_free(params.node);

    params.nacl_id = NACL_DATA_ID_FLOW_MISS_DHCP_UPLINK;
    params.node = format(0, "pds-dhcp-relay-uplink-classify");
    if (0 != pds_register_nacl_id_to_node(&params)) {
        ASSERT(0);
    }
    vec_free(params.node);
    return;
}
