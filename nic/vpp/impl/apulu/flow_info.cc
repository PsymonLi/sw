#include <nic/vpp/impl/flow_info.h>
#include "gen/p4gen/p4/include/ftl.h"
#include <nic/apollo/p4/include/apulu_table_sizes.h>

int
pds_flow_info_program (uint32_t ses_id, bool iflow, uint8_t l2l)
{
    flow_info_entry_t flow_info = {0};
    sdk_ret_t ret = SDK_RET_OK;
    uint32_t index;

    flow_info.is_local_to_local = l2l;

    // Iflow is stored at index = ses_id
    // Rflow is stored at index = ses_id + (FLOW_INFO_TABLE_SIZE/2)
    index = iflow ? ses_id : (ses_id + (FLOW_INFO_TABLE_SIZE / 2));

    ret = flow_info.write(index);

    if (ret != SDK_RET_OK) {
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;;
        return ret;
    }

    return 0;
}

void
pds_flow_info_get_flow_info (uint32_t ses_id, bool iflow, void *flow_info)
{
    uint32_t index;

    // Iflow is stored at index = ses_id
    // Rflow is stored at index = ses_id + (FLOW_INFO_TABLE_SIZE/2)
    index = iflow ? ses_id : (ses_id + (FLOW_INFO_TABLE_SIZE / 2));

    sdk_ret_t retval = ((flow_info_entry_t *)flow_info)->read(index);
    SDK_ASSERT(retval == SDK_RET_OK);
}

int
pds_flow_info_update_l2l (uint32_t ses_id, bool iflow, uint8_t l2l)
{
    // Once we have more parameters, we can pass the read values to
    // program flow info
    // pds_flow_info_get_flow_info(ses_id, iflow, (void *)&flow_info);

    return pds_flow_info_program(ses_id, iflow, l2l);
}

