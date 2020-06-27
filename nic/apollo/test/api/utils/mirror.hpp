//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#ifndef __TEST_API_UTILS_MIRROR_HPP__
#define __TEST_API_UTILS_MIRROR_HPP__

#include "nic/apollo/api/include/pds_mirror.hpp"
#include "nic/apollo/test/api/utils/api_base.hpp"
#include "nic/apollo/test/api/utils/batch.hpp"
#include "nic/apollo/test/api/utils/feeder.hpp"

namespace test {
namespace api {

// Enumerate mirror session attributes
enum mirror_session_attrs{
    MIRROR_SESSION_ATTR_SNAP_LEN = bit(0),
    MIRROR_SESSION_ATTR_TYPE     = bit(1),
    MIRROR_SESSION_ATTR_RSPAN    = bit(2),
    MIRROR_SESSION_ATTR_ERSPAN   = bit(3),
};

// MIRROR test feeder class
class mirror_session_feeder : public feeder {
public:
    pds_obj_key_t vpc;
    pds_mirror_session_type_t type;
    uint32_t snap_len;
    uint32_t tep;
    std::string dst_ip;
    ip_addr_t dst_ip_addr;
    uint32_t span_id;
    uint32_t dscp;
    pds_obj_key_t interface;
    pds_encap_t encap;
    pds_erspan_dst_type_t dst_type;
    pds_erspan_type_t erspan_type;
    bool vlan_strip_en;
    pds_mirror_session_spec_t spec;

    // Constructor
    mirror_session_feeder() { };
    // Copy constructor
    mirror_session_feeder(mirror_session_feeder& feeder) {
        init(feeder.spec.key, feeder.num_obj, feeder.interface,
             feeder.encap.val.vlan_tag, feeder.dst_ip, feeder.tep, 
             feeder.span_id, feeder.dscp, feeder.vlan_strip_en, 
             feeder.spec.type, feeder.spec.snap_len, feeder.erspan_type, 
             feeder.dst_type, pdsobjkey2int(feeder.vpc));
    }
    // initalize feeder with base set of values
    void init(pds_obj_key_t key, uint8_t mirror_sess,
              pds_obj_key_t interface, uint16_t vlan_tag,
              std::string dst_ip, uint32_t tep,
              uint32_t span_id = 1, uint32_t dscp = 1,
              bool vlan_strip_en = false, 
              pds_mirror_session_type_t type = PDS_MIRROR_SESSION_TYPE_RSPAN,
              uint16_t snap_len = 100,
              pds_erspan_type_t erspan_type = PDS_ERSPAN_TYPE_1,
              pds_erspan_dst_type_t erspan_dst_type = PDS_ERSPAN_DST_TYPE_TEP,
              uint32_t vpc = 1, bool stash = false);

    // Iterate helper routines
    void iter_next(int width = 1);

    // Build routines
    void key_build(pds_obj_key_t *key) const;
    void spec_build(pds_mirror_session_spec_t *spec) const;

    // Compare routines
    bool key_compare(const pds_obj_key_t *key) const;
    bool spec_compare(const pds_mirror_session_spec_t *spec) const;
    bool status_compare(const pds_mirror_session_status_t *status1,
                        const pds_mirror_session_status_t *status2) const;
};

// Dump prototypes
inline std::ostream&
operator<<(std::ostream& os, const mirror_session_feeder& obj) {
    os << "MIRROR feeder =>"
       << "id: " << std::string(obj.spec.key.str()) << "  "
       << "type: " << obj.spec.type
       << "snap_len: " << obj.spec.snap_len;
    return os;
}

// CRUD prototypes
API_CREATE(mirror_session);
API_READ(mirror_session);
API_UPDATE(mirror_session);
API_DELETE(mirror_session);

// mirror session CRUD helper functions
void create_mirror_session_spec(pds_mirror_session_spec_t* spec,
                                pds_mirror_session_type_t type,
                                pds_obj_key_t interface, uint16_t vlan_tag,
                                std::string dst_ip, uint32_t tep,
                                uint32_t span_id = 1, uint32_t dscp = 1,
                                bool vlan_strip_en = false,
                                uint16_t snap_len = 100,
                                pds_erspan_type_t erspan_type = PDS_ERSPAN_TYPE_1,
                                pds_erspan_dst_type_t erspan_dst_type =
                                PDS_ERSPAN_DST_TYPE_TEP,
                                uint32_t vpc = 1);
void mirror_session_create(mirror_session_feeder& feeder);
void mirror_session_read(mirror_session_feeder& feeder,
                         sdk_ret_t exp_result = SDK_RET_OK);
void mirror_session_update(mirror_session_feeder& feeder,
                           pds_mirror_session_spec_t *spec, uint64_t chg_bmap, 
                           sdk_ret_t exp_result = SDK_RET_OK);
void mirror_session_delete(mirror_session_feeder& feeder);

}    // namespace api
}    // namespace test

#endif    // __TEST_API_UTILS_MIRROR_HPP__
