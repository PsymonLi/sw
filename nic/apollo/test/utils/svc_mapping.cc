//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains the svc mapping test utility routines implementation
///
//----------------------------------------------------------------------------

#include "nic/apollo/api/service_utils.hpp"
#include "nic/apollo/test/utils/svc_mapping.hpp"
#include "nic/apollo/test/utils/utils.hpp"

namespace api_test {

//----------------------------------------------------------------------------
// Svc maping feeder class routines
//----------------------------------------------------------------------------

void
svc_mapping_feeder::init(int vpc_id, std::string vip_str, uint16_t svc_port,
                         int backend_vpc_id, std::string backend_ip_str,
                         uint16_t backend_svc_port, std::string backend_pip_str,
                         uint32_t num_svc_mapping) {
    spec.key.vpc.id = vpc_id;
    extract_ip_addr(vip_str.c_str(), &spec.key.vip);
    spec.key.svc_port = svc_port;
    spec.vpc.id = backend_vpc_id;
    extract_ip_addr(backend_ip_str.c_str(), &spec.backend_ip);
    spec.svc_port = backend_svc_port;
    extract_ip_addr(backend_pip_str.c_str(), &spec.backend_provider_ip);
    num_obj = num_svc_mapping;
}

void
svc_mapping_feeder::iter_next(int width) {
    spec.key.vpc.id += width;
    spec.key.vip.addr.v6_addr.addr64[1] += width;
    spec.key.svc_port += width;
    spec.vpc.id += width;
    spec.backend_ip.addr.v6_addr.addr64[1] += width;
    spec.svc_port += width;
    if (!ip_addr_is_zero(&spec.backend_provider_ip))
        spec.backend_provider_ip.addr.v6_addr.addr64[1] += width;
    cur_iter_pos++;
}

void
svc_mapping_feeder::key_build(pds_svc_mapping_key_t *key) const {
    memset(key, 0, sizeof(pds_svc_mapping_key_t));
    *key = spec.key;
}

void
svc_mapping_feeder::spec_build(pds_svc_mapping_spec_t *spec) const {
    memset(spec, 0, sizeof(pds_svc_mapping_spec_t));
    this->key_build(&spec->key);
    spec->vpc.id = this->spec.vpc.id;
    spec->backend_ip = this->spec.backend_ip;
    spec->svc_port = this->spec.svc_port;
    spec->backend_provider_ip = this->spec.backend_provider_ip;
}

bool
svc_mapping_feeder::key_compare(const pds_svc_mapping_key_t *key) const {
    return (*key == this->spec.key);
}

bool
svc_mapping_feeder::spec_compare(const pds_svc_mapping_spec_t *spec) const {
    // todo @njose remove this once read() is complete
    return true;

    if (spec->vpc.id != this->spec.vpc.id)
        return false;

    if (!IPADDR_EQ(&spec->backend_ip, &this->spec.backend_ip))
        return false;

    if (spec->svc_port != this->spec.svc_port)
        return false;

    if (!IPADDR_EQ(&spec->backend_provider_ip, &this->spec.backend_provider_ip))
        return false;

    return true;
}

}    // namespace api_test
