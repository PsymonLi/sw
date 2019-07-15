//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#ifndef __TEST_UTILS_VNIC_HPP__
#define __TEST_UTILS_VNIC_HPP__

#include "nic/apollo/api/include/pds_vnic.hpp"
#include "nic/apollo/test/utils/api_base.hpp"
#include "nic/apollo/test/utils/feeder.hpp"

namespace api_test {

// Export variables
extern const uint64_t k_feeder_mac;
extern const uint32_t k_max_vnic;

// VNIC test feeder class
class vnic_feeder : public feeder {
public:
    pds_vnic_id_t id;
    pds_vpc_id_t vpc_id;
    pds_subnet_id_t subnet_id;
    pds_encap_t vnic_encap;
    pds_encap_t fabric_encap;
    uint64_t mac_u64;
    pds_rsc_pool_id_t rsc_pool_id;
    bool src_dst_check;

    // Constructor
    vnic_feeder() { };
    vnic_feeder(const vnic_feeder& feeder) {
        init(feeder.id, feeder.num_obj, feeder.mac_u64, feeder.vnic_encap.type,
             feeder.fabric_encap.type, feeder.src_dst_check);
    }

    // Initialize feeder with the base set of values
    void init(uint32_t id, uint32_t num_vnic = k_max_vnic,
              uint64_t mac = k_feeder_mac,
              pds_encap_type_t vnic_encap_type = PDS_ENCAP_TYPE_DOT1Q,
              pds_encap_type_t fabric_encap_type = PDS_ENCAP_TYPE_MPLSoUDP,
              bool src_dst_check = true);

    // Iterate helper routines
    void iter_next(int width = 1);

    // Build routines
    void key_build(pds_vnic_key_t *key) const;
    void spec_build(pds_vnic_spec_t *spec) const;

    // Compare routines
    bool key_compare(const pds_vnic_key_t *key) const;
    bool spec_compare(const pds_vnic_spec_t *spec) const;
};

// Dump prototypes
inline std::ostream&
operator<<(std::ostream& os, const vnic_feeder& obj) {
    os << "VNIC feeder =>"
       << " id: " << obj.id;
    return os;
}

// CRUD prototypes
API_CREATE(vnic);
API_READ(vnic);
API_UPDATE(vnic);
API_DELETE(vnic);

} // namespace api_test

#endif // __TEST_UTILS_VNIC_HPP__
