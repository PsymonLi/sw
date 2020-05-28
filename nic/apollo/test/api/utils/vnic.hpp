//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#ifndef __TEST_API_UTILS_VNIC_HPP__
#define __TEST_API_UTILS_VNIC_HPP__

#include "nic/apollo/api/include/pds_vnic.hpp"
#include "nic/apollo/test/api/utils/api_base.hpp"
#include "nic/apollo/test/api/utils/feeder.hpp"

namespace test {
namespace api {

// Export variables
extern const uint64_t k_feeder_mac;
extern const uint32_t k_max_vnic;

enum vnic_attrs {
    VNIC_ATTR_HOSTNAME           =  bit(0),
    VNIC_ATTR_SUBNET             =  bit(1),
    VNIC_ATTR_VNIC_ENCAP         =  bit(2),
    VNIC_ATTR_FAB_ENCAP          =  bit(3),
    VNIC_ATTR_MAC_ADDR           =  bit(4),
    VNIC_ATTR_BINDING_CHECKS_EN  =  bit(5),
    VNIC_ATTR_TX_MIRROR          =  bit(6),
    VNIC_ATTR_RX_MIRROR          =  bit(7),
    VNIC_ATTR_V4_METER           =  bit(8),
    VNIC_ATTR_V6_METER           =  bit(9),
    VNIC_ATTR_SWITCH_VNIC        =  bit(10),
    VNIC_ATTR_V4_INGPOL          =  bit(11),
    VNIC_ATTR_V6_INGPOL          =  bit(12),
    VNIC_ATTR_V4_EGRPOL          =  bit(13),
    VNIC_ATTR_V6_EGRPOL          =  bit(14),
    VNIC_ATTR_HOST_IF            =  bit(15),
    VNIC_ATTR_TX_POLICER         =  bit(16),
    VNIC_ATTR_RX_POLICER         =  bit(17),
    VNIC_ATTR_PRIMARY            =  bit(18),
    VNIC_ATTR_MAX_SESSIONS       =  bit(19),
    VNIC_ATTR_FLOW_LEARN_EN      =  bit(20),
};

#define VNIC_ATTR_POL VNIC_ATTR_V4_INGPOL | VNIC_ATTR_V6_INGPOL | \
                      VNIC_ATTR_V4_EGRPOL | VNIC_ATTR_V6_EGRPOL

// VNIC test feeder class
class vnic_feeder : public feeder {
public:
    pds_vnic_spec_t spec;
    vector <pds_vnic_info_t *> vec;

    // Constructor
    vnic_feeder() {
    }
    vnic_feeder(const vnic_feeder& feeder);


    // Initialize feeder with the base set of values
    void init(pds_obj_key_t key, pds_obj_key_t subnet,
              uint32_t num_vnic, uint64_t mac,
              pds_encap_type_t vnic_encap_type,
              pds_encap_type_t fabric_encap_type,
              bool binding_checks_en, bool configure_policy,
              uint8_t tx_mirror_session_bmap, uint8_t rx_mirror_session_bmap,
              uint8_t num_policies_per_vnic, uint8_t start_policy_index,
              pds_obj_key_t tx_policer, pds_obj_key_t rx_policer,
              bool stash_ = false);

    // Iterate helper routines
    void iter_next(int width = 1);

    // Build routines
    void key_build(pds_obj_key_t *key) const;
    void spec_build(pds_vnic_spec_t *spec) const;

    // Compare routines
    bool key_compare(const pds_obj_key_t *key) const;
    bool spec_compare(const pds_vnic_spec_t *spec) const;
    bool status_compare(const pds_vnic_status_t *status1,
                        const pds_vnic_status_t *status2) const;
};

// Dump prototypes
inline std::ostream&
operator<<(std::ostream& os, const pds_vnic_spec_t *spec) {
    os << &spec->key
       << " subnet: " << std::string(spec->subnet.str())
       << " vnic encap: " << pds_encap2str(&spec->vnic_encap)
       << " fabric encap: " << pds_encap2str(&spec->fabric_encap)
       << " mac: " << macaddr2str(spec->mac_addr)
       << " IP/MAC bindings check : " << spec->binding_checks_en
#if 0
       << " tx_mirror_session_bmap: " << +spec->tx_mirror_session_bmap
       << " rx_mirror_session_bmap: " << +spec->rx_mirror_session_bmap
#endif
       << " v4 meter: " << spec->v4_meter.str()
       << " v6 meter: " << spec->v6_meter.str()
       << "tx policer"  << spec->tx_policer.str()
       << "rx policer"  << spec->rx_policer.str()
       << " switch vnic: " << spec->switch_vnic;
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_vnic_status_t *status) {
    os << " HW id: " << status->hw_id;
    os << " NH HW id: " << status->nh_hw_id;
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_vnic_stats_t *stats) {
    os << " rx pkts: " << stats->rx_pkts
       << " rx bytes: " << stats->rx_bytes
       << " tx pkts: " << stats->tx_pkts
       << " tx bytes: " << stats->tx_bytes;
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const pds_vnic_info_t *obj) {
    os << " VNIC info =>"
       << &obj->spec
       << &obj->status
       << &obj->stats
       << std::endl;
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, const vnic_feeder& obj) {
    os << "VNIC feeder =>"
       << " key: " << std::string(obj.spec.key.str());
    return os;
}

// CRUD prototypes
API_CREATE(vnic);
API_READ(vnic);
API_READ_CMP(vnic);
API_UPDATE(vnic);
API_DELETE(vnic);

// VNIC crud helper prototypes
void vnic_create(vnic_feeder& feeder);
void vnic_read(vnic_feeder& feeder, sdk_ret_t exp_result = SDK_RET_OK);
void vnic_update(vnic_feeder& feeder, pds_vnic_spec_t *spec,
                 uint64_t chg_bmap, sdk_ret_t exp_result = SDK_RET_OK);
void vnic_delete(vnic_feeder& feeder);

// Misc function prototypes
void sample_vnic_setup(pds_batch_ctxt_t bctxt);
void sample_vnic_teardown(pds_batch_ctxt_t bctxt);
void vnic_feeder_encap_init(uint32_t id, pds_encap_type_t encap_type,
                            pds_encap_t *encap);
void vnic_feeder_encap_next(pds_encap_t *encap, int width = 1);
void vnic_spec_policy_fill(pds_vnic_spec_t *spec, uint8_t num_policies,
                           uint8_t start_policy_index);

}    // namespace api
}    // namespace test

#endif    // __TEST_API_UTILS_VNIC_HPP__
