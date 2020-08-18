//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.
//

#include <netinet/in.h>
#include <pthread.h>
#include <nic/sdk/asic/pd/pd.hpp>
#include <nic/sdk/asic/port.hpp>
#include <nic/sdk/include/sdk/table.hpp>
#include <nic/sdk/lib/p4/p4_utils.hpp>
#include <nic/sdk/lib/p4/p4_api.hpp>
#include <nic/sdk/platform/capri/capri_tbl_rw.hpp>
#include <nic/p4/common/defines.h>
#include <nic/apollo/p4/include/defines.h>
#include <nic/vpp/impl/nat.h>
#include "gen/p4gen/p4/include/ftl2.h"

using namespace sdk;
using namespace sdk::table;
using namespace sdk::platform;

extern "C" {

int
pds_nat_init(void)
{
    return 0;
}

int
pds_snat_tbl_write_ip4(int nat_index, uint32_t ip, uint16_t port)
{
    sdk_ret_t ret = SDK_RET_OK;
    nat_rewrite_entry_t nat_rewrite_entry;

    memset(&nat_rewrite_entry, 0, sizeof(nat_rewrite_entry_t));
    nat_rewrite_entry.set_port(htons(port));
    *(uint32_t *)&nat_rewrite_entry.ip[12] = htonl(ip);
 
    ret = nat_rewrite_entry.write(nat_index);
    if (ret != SDK_RET_OK) {
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        return ret;
    }
    
    return 0;
}

int
pds_dnat_tbl_write_ip4(int nat_index, uint32_t ip, uint16_t port)
{
    sdk_ret_t ret = SDK_RET_OK;
    nat2_rewrite_entry_t nat2_rewrite_entry;

    memset(&nat2_rewrite_entry, 0, sizeof(nat2_rewrite_entry_t));
    nat2_rewrite_entry.set_port(htons(port));
    *(uint32_t *)&nat2_rewrite_entry.ip[12] = htonl(ip);
 
    ret = nat2_rewrite_entry.write(nat_index);
    if (ret != SDK_RET_OK) {
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        return ret;
    }
    
    return 0;
}

int
pds_snat_tbl_read_ip4(int nat_index, uint32_t *ip, uint16_t *port)
{
    sdk_ret_t ret = SDK_RET_OK;
    nat_rewrite_entry_t nat_rewrite_entry;

    ret = nat_rewrite_entry.read(nat_index);
    if (ret != SDK_RET_OK) {
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        return ret;
    }

    *port = ntohs(nat_rewrite_entry.get_port());
    *ip = ntohl(*(uint32_t *)&nat_rewrite_entry.ip[12]);
 
    return 0;
}

int
pds_dnat_tbl_read_ip4(int nat_index, uint32_t *ip, uint16_t *port)
{
    sdk_ret_t ret = SDK_RET_OK;
    nat2_rewrite_entry_t nat2_rewrite_entry;

    ret = nat2_rewrite_entry.read(nat_index);
    if (ret != SDK_RET_OK) {
        ret = sdk::SDK_RET_HW_PROGRAM_ERR;
        return ret;
    }

    *port = ntohs(nat2_rewrite_entry.get_port());
    *ip = ntohl(*(uint32_t *)&nat2_rewrite_entry.ip[12]);
 
    return 0;
}

}
