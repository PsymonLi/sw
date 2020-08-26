//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//
#include <nic/vpp/impl/flow_info.h>

#include <nic/vpp/impl/flow_info.h>

void
pds_flow_info_program(uint32_t tbl_id, bool l2l)
{
    return;
}

void
pds_flow_info_l2l_update (uint32_t tbl_id, bool l2l)
{
    return;
}

void
pds_flow_info_get (uint32_t tbl_id, void *flow_info)
{
    return;
}

void
pds_flow_info_cache_entry_add (uint16_t worker_id, bool l2l)
{
    return;
}

void
pds_flow_info_cache_entry_prog (uint16_t worker_id,
                                uint32_t entry_index,
                                uint32_t table_index)
{
    return;
}

void
pds_flow_info_cache_reset (uint16_t worker_id)
{
    return;
}

void
pds_flow_info_init (uint16_t workers, uint16_t cache_size)
{
    return;
}
