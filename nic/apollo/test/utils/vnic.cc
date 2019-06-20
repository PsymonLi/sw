//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the vnic test utility routines implementation
///
//----------------------------------------------------------------------------

#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/apollo/api/utils.hpp"
#include "nic/apollo/test/utils/utils.hpp"
#include "nic/apollo/test/utils/base.hpp"
#include "nic/apollo/test/utils/vnic.hpp"

namespace api_test {

const uint64_t k_feeder_mac = 0xa010101000000000;
const uint32_t k_max_vnic = ::apollo() ? 64 : PDS_MAX_VNIC;

static inline void
vnic_feeder_encap_init (uint32_t id, pds_encap_type_t encap_type,
                        pds_encap_t *encap)
{
    encap->type = encap_type;
    // update encap value to seed base
    switch (encap_type) {
    case PDS_ENCAP_TYPE_DOT1Q:
        encap->val.vlan_tag = id;
        break;
    case PDS_ENCAP_TYPE_QINQ:
        encap->val.qinq_tag.c_tag = id;
        encap->val.qinq_tag.s_tag = id + 4096;
        break;
    case PDS_ENCAP_TYPE_MPLSoUDP:
        encap->val.mpls_tag = id;
        break;
    case PDS_ENCAP_TYPE_VXLAN:
        encap->val.vnid = id;
        break;
    default:
        encap->val.value = id;
        break;
    }
}


static inline void
vnic_feeder_encap_next (pds_encap_t *encap, int width = 1)
{
    switch (encap->type) {
    case PDS_ENCAP_TYPE_DOT1Q:
        encap->val.vlan_tag += width;
        break;
    case PDS_ENCAP_TYPE_QINQ:
        encap->val.qinq_tag.c_tag += width;
        encap->val.qinq_tag.s_tag += width;
        break;
    case PDS_ENCAP_TYPE_MPLSoUDP:
        encap->val.mpls_tag += width;
        break;
    case PDS_ENCAP_TYPE_VXLAN:
        encap->val.vnid += width;
        break;
    default:
        encap->val.value += width;
        break;
    }
}

//----------------------------------------------------------------------------
// VNIC feeder class routines
//----------------------------------------------------------------------------

void
vnic_feeder::init(uint32_t id, uint32_t num_vnic, uint64_t mac,
                  pds_encap_type_t vnic_encap_type,
                  pds_encap_type_t fabric_encap_type,
                  bool src_dst_check) {
    this->id = id;
    this->vpc_id = 1;
    this->subnet_id = 1;
    this->rsc_pool_id = 0;
    this->mac_u64 = mac;
    vnic_feeder_encap_init(id, vnic_encap_type, &vnic_encap);
    vnic_feeder_encap_init(id, fabric_encap_type, &fabric_encap);
    this->src_dst_check = src_dst_check;
    num_obj = num_vnic;
}

void
vnic_feeder::iter_next(int width) {
    id += width;
    vnic_feeder_encap_next(&vnic_encap);
    vnic_feeder_encap_next(&fabric_encap);
    mac_u64 += width;
    cur_iter_pos++;
}

void
vnic_feeder::key_build(pds_vnic_key_t *key) const {
    memset(key, 0, sizeof(pds_vnic_key_t));
    key->id = this->id;
}

void
vnic_feeder::spec_build(pds_vnic_spec_t *spec) const {
    memset(spec, 0, sizeof(pds_vnic_spec_t));
    this->key_build(&spec->key);

    spec->vpc.id = vpc_id;
    spec->subnet.id = subnet_id;
    spec->vnic_encap = vnic_encap;
    spec->fabric_encap = fabric_encap;
    MAC_UINT64_TO_ADDR(spec->mac_addr, mac_u64);
    spec->rsc_pool_id = rsc_pool_id;
    spec->src_dst_check = src_dst_check;
}

bool
vnic_feeder::key_compare(const pds_vnic_key_t *key) const {
    if (id != key->id)
        return false;
    return true;
}

bool
vnic_feeder::spec_compare(const pds_vnic_spec_t *spec) const {
    if (!api::pdsencap_isequal(&vnic_encap, &spec->vnic_encap))
        return false;

    if (!api::pdsencap_isequal(&fabric_encap, &spec->fabric_encap))
        return false;

    if (rsc_pool_id != spec->rsc_pool_id)
        return false;

    if (src_dst_check != spec->src_dst_check)
        return false;

    mac_addr_t mac = {0};
    MAC_UINT64_TO_ADDR(mac, mac_u64);
    if (memcmp(mac, &spec->mac_addr, sizeof(mac)))
        return false;

    return true;
}

} // namespace api_test
