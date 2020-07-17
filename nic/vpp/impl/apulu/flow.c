//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#include <nic/apollo/api/impl/apulu/nacl_data.h>
#include <api.h>
#include "impl_db.h"

void
pds_flow_nacl_init (void)
{
    pds_infra_api_reg_t params = {0};
    pds_impl_db_device_entry_t *dev;

    dev = pds_impl_db_device_get();
    params.nacl_id = NACL_DATA_ID_FLOW_MISS_IP4_IP6;
    params.unreg = true;
    if (0 != pds_register_nacl_id_to_node(&params)) {
        ASSERT(0);
    }

    if (dev->oper_mode == PDS_DEV_MODE_BITW_SMART_SERVICE) {
        params.node = format(0, "pds-svc-flow-classify");
    } else {
        params.node = format(0, "pds-flow-classify");
    }
    //flow = vlib_get_node_by_name(vlib_get_main(), (u8 *) "pds-flow-classify");
    //params.frame_queue_index = vlib_frame_queue_main_init (flow->index, 0);
    //params.handoff_thread = 0;
    params.frame_queue_index = ~0;
    params.handoff_thread = ~0;
    params.offset = 0;
    params.unreg = false;

    if (0 != pds_register_nacl_id_to_node(&params)) {
        ASSERT(0);
    }
    vec_free(params.node);
    return;
}
