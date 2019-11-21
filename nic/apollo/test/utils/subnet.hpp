//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#ifndef __TEST_UTILS_SUBNET_HPP__
#define __TEST_UTILS_SUBNET_HPP__

#include "nic/sdk/include/sdk/ip.hpp"
#include "nic/apollo/api/include/pds_subnet.hpp"
#include "nic/apollo/api/include/pds_vpc.hpp"
#include "nic/apollo/api/subnet_utils.hpp"
#include "nic/apollo/test/utils/api_base.hpp"
#include "nic/apollo/test/utils/feeder.hpp"

namespace api_test {

// Subnet test feeder class
class subnet_feeder : public feeder {
public:
    pds_subnet_spec_t spec;

    // Constructor
    subnet_feeder() { };
    subnet_feeder(const subnet_feeder& feeder);

    // Initialize feeder with the base set of values
    void init(pds_subnet_key_t key, pds_vpc_key_t vpc_key,
              std::string cidr_str, std::string vrmac_str,
              int num_subnet = 1);

    // Iterate helper routines
    void iter_next(int width = 1);

    // Build routines
    void key_build(pds_subnet_key_t *key) const;
    void spec_build(pds_subnet_spec_t *spec) const;

    // Compare routines
    bool key_compare(const pds_subnet_key_t *key) const;
    bool spec_compare(const pds_subnet_spec_t *spec) const;
};

// Dump prototypes
inline std::ostream&
operator<<(std::ostream& os, const subnet_feeder& obj) {
    os << "Subnet feeder =>" << &obj.spec;
    return os;
}

// CRUD prototypes
API_CREATE(subnet);
API_READ(subnet);
API_UPDATE(subnet);
API_DELETE(subnet);

// Export variables
extern pds_subnet_key_t k_subnet_key;

// Misc function prototypes
void sample_subnet_setup(pds_batch_ctxt_t bctxt);
void sample_subnet_teardown(pds_batch_ctxt_t bctxt);
void sample1_subnet_setup(pds_batch_ctxt_t bctxt);
void sample1_subnet_teardown(pds_batch_ctxt_t bctxt);

}    // namespace api_test

#endif    // __TEST_UTILS_SUBNET_HPP__
