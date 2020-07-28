//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/if.hpp"
#include "nic/apollo/test/athena/api/policer/utils.hpp"

void
fill_key (pds_policer_key_t *key,
          uint16_t index)
{
     key->policer_id = index;
     return;
}

void
fill_data (pds_policer_data_t *data,
           uint64_t rate,
           uint64_t burst)
{
    data->rate = rate;
    data->burst = burst;
    return;
}

void
update_data (pds_policer_data_t *data,
             uint64_t rate,
             uint64_t burst)
{
    data->rate = rate;
    data->burst = burst;
    return;
}
