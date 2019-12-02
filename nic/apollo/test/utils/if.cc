//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/apollo/test/utils/if.hpp"
#include "nic/apollo/test/utils/utils.hpp"
#include "nic/sdk/include/sdk/if.hpp"

namespace api_test {

std::string k_if_ippfx("90.0.0.1");
uint32_t k_l3_if_id = 0x70000001;

//----------------------------------------------------------------------------
// Interface feeder class routines
//----------------------------------------------------------------------------

#define l3_info spec_feeder.l3_if_info
void
if_feeder::init(pds_if_id_t id, std::string ip_pfx_str, 
                pds_if_type_t type, int num_ifs) {
    spec_feeder.key.id = id;
    spec_feeder.type = type;
    spec_feeder.admin_state = PDS_IF_STATE_UP;

    if (type == PDS_IF_TYPE_L3) {
        spec_feeder.key.id = L3_IFINDEX(id);
        l3_info.vpc.id = 1;
        str2ipv4pfx((char *)ip_pfx_str.c_str(), &l3_info.ip_prefix);
        MAC_UINT64_TO_ADDR(l3_info.mac_addr,
                           (uint64_t)l3_info.ip_prefix.addr.addr.v4_addr);
        l3_info.port_num = TM_PORT_UPLINK_0;
        l3_info.encap.type = PDS_ENCAP_TYPE_VXLAN;
        l3_info.encap.val.vnid = 1;
    }
    
    num_obj = num_ifs;
}

void
if_feeder::iter_next(int width) {
    ip_addr_t ipaddr = {0};

    spec_feeder.key.id += width;
    if (spec_feeder.type == PDS_IF_TYPE_L3) {
        ip_prefix_ip_next(&l3_info.ip_prefix, &ipaddr);
        memcpy(&l3_info.ip_prefix.addr, &ipaddr, sizeof(ip_addr_t));
        MAC_UINT64_TO_ADDR(l3_info.mac_addr,
                           (uint64_t)l3_info.ip_prefix.addr.addr.v4_addr);
        l3_info.port_num = TM_PORT_UPLINK_1;
    }

    cur_iter_pos++;
}

void
if_feeder::spec_build(pds_if_spec_t *spec) const {
    memcpy(spec, &spec_feeder, sizeof(pds_if_spec_t));
}

void
if_feeder::key_build(pds_if_key_t *key) const {
    memcpy(key, &spec_feeder.key, sizeof(pds_if_key_t));
}

bool
if_feeder::spec_compare(const pds_if_spec_t *spec) const {
    return(!memcmp(spec, &spec_feeder, sizeof(pds_if_spec_t)));
}

bool
if_feeder::key_compare(const pds_if_key_t *key) const {
    return(!memcmp(key, &spec_feeder.key, sizeof(pds_if_key_t)));
}

//----------------------------------------------------------------------------
// Misc routines
//----------------------------------------------------------------------------

// do not modify these sample values as rest of system is sync with these
static if_feeder k_if_feeder;

void sample_if_setup(pds_batch_ctxt_t bctxt) {
    k_if_feeder.init(1);
    create(bctxt, k_if_feeder);
}

void sample_if_teardown(pds_batch_ctxt_t bctxt) {
    k_if_feeder.init(1);
    del(bctxt, k_if_feeder);
}

}    // namespace api_test
