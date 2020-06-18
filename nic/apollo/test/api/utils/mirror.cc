//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------

#include "nic/apollo/test/api/utils/mirror.hpp"

namespace test {
namespace api {

//----------------------------------------------------------------------------
// Mirror feeder class routines
//----------------------------------------------------------------------------

void
mirror_session_feeder::init(pds_obj_key_t key, uint8_t mirror_sess,
                            pds_obj_key_t interface, uint16_t vlan_tag,
                            std::string dst_ip, uint32_t tep,
                            uint32_t span_id, uint32_t dscp,
                            bool vlan_strip_en) {

    this->key =  key;
    this->snap_len = 100;
    // init with rspan type, then alternate b/w rspan and erspan in iter_next
    this->type = PDS_MIRROR_SESSION_TYPE_RSPAN;
    this->interface = interface;
    this->encap.type = PDS_ENCAP_TYPE_DOT1Q;
    this->encap.val.vlan_tag = vlan_tag;
    // init with erspan packet type as type_1, then iterate in iter_next
    this->erspan_type = PDS_ERSPAN_TYPE_1;
    this->vpc = int2pdsobjkey(1);
    // init with erspan dst type as tep, then alernate b/w tep and ip
    this->dst_type = PDS_ERSPAN_DST_TYPE_TEP;
    this->tep = tep;
    this->dst_ip = dst_ip;
    memset(&this->dst_ip_addr, 0x0, sizeof(ip_addr_t));
    test::extract_ip_addr((char *)dst_ip.c_str(), &this->dst_ip_addr);
    this->dscp = dscp;
    this->span_id = span_id;
    this->vlan_strip_en = vlan_strip_en;
    this->num_obj = mirror_sess;
}

void
mirror_session_feeder::iter_next(int width) {
    key = int2pdsobjkey(pdsobjkey2int(key) + width);
    if (type == PDS_MIRROR_SESSION_TYPE_RSPAN) {
        type = PDS_MIRROR_SESSION_TYPE_ERSPAN;
        span_id++;
        dscp++;
        vlan_strip_en = ~vlan_strip_en;
        if (erspan_type == PDS_ERSPAN_TYPE_1)
            erspan_type = PDS_ERSPAN_TYPE_2;
        else if (erspan_type == PDS_ERSPAN_TYPE_2)
            erspan_type = PDS_ERSPAN_TYPE_3;
        else
            erspan_type = PDS_ERSPAN_TYPE_1;
        if (dst_type == PDS_ERSPAN_DST_TYPE_TEP) {
            dst_type = PDS_ERSPAN_DST_TYPE_IP;
            // dst_ip_addr.addr.v4_addr += 1;
            test::increment_ip_addr(&dst_ip_addr, width);
        } else {
            dst_type = PDS_ERSPAN_DST_TYPE_TEP;
            // tep++;
        }
    } else if (type == PDS_MIRROR_SESSION_TYPE_ERSPAN) {
        type = PDS_MIRROR_SESSION_TYPE_RSPAN;
        if (encap.type  == PDS_ENCAP_TYPE_DOT1Q)
            encap.val.vlan_tag++;
        else if (encap.type == PDS_ENCAP_TYPE_QINQ) {
            encap.val.qinq_tag.c_tag++;
            encap.val.qinq_tag.s_tag++;
        }
    }
    cur_iter_pos++;
}

void
mirror_session_feeder::key_build(pds_obj_key_t *key) const {
    *key = this->key;
}

void
mirror_session_feeder::spec_build(pds_mirror_session_spec_t *spec) const {
    memset(spec, 0, sizeof(pds_mirror_session_spec_t));

    spec->snap_len = snap_len;
    spec->key = this->key;
    spec->type = type;
    if (type  == PDS_MIRROR_SESSION_TYPE_RSPAN) {
        spec->rspan_spec.interface = interface;
        memcpy(&spec->rspan_spec.encap, &encap, sizeof(pds_encap_t));
    } else if (type == PDS_MIRROR_SESSION_TYPE_ERSPAN) {
        spec->erspan_spec.type = erspan_type;
        spec->erspan_spec.vpc = vpc;
        spec->erspan_spec.dst_type = dst_type;
        if (spec->erspan_spec.dst_type == PDS_ERSPAN_DST_TYPE_TEP) {
            spec->erspan_spec.tep = int2pdsobjkey(tep);
        } else {
            memcpy(&spec->erspan_spec.ip_addr, &dst_ip_addr,
                   sizeof(ip_addr_t));
            spec->erspan_spec.ip_addr.af = IP_AF_IPV4;
        }
        spec->erspan_spec.dscp = dscp;
        spec->erspan_spec.span_id = span_id;
        spec->erspan_spec.vlan_strip_en = vlan_strip_en;
        // todo: need to handle mapping key
    }
}
bool
mirror_session_feeder::key_compare(const pds_obj_key_t *key) const {
    if (this->key != *key)
        return false;
    return true;
}

bool
mirror_session_feeder::spec_compare(
                                const pds_mirror_session_spec_t *spec) const {
    if (spec->type != type)
        return false;

    if (spec->snap_len != snap_len)
        return false;

    if (spec->type == PDS_MIRROR_SESSION_TYPE_RSPAN) {
        // validate rspan spec
        if ((spec->rspan_spec.interface != interface) ||
            (spec->rspan_spec.encap.type != encap.type) ||
            (spec->rspan_spec.encap.val.vlan_tag != encap.val.vlan_tag))
            return false;

    } else if (type == PDS_MIRROR_SESSION_TYPE_ERSPAN) {
        // validate erspan spec
        if ((spec->erspan_spec.type != erspan_type) ||
            (spec->erspan_spec.dst_type != dst_type) ||
            (spec->erspan_spec.vlan_strip_en != vlan_strip_en))
            return false;
        if ((spec->erspan_spec.vpc != vpc) ||
            (spec->erspan_spec.dscp != dscp) ||
            (spec->erspan_spec.span_id != span_id))
            return false;
        if (spec->erspan_spec.dst_type == PDS_ERSPAN_DST_TYPE_TEP &&
            spec->erspan_spec.tep != int2pdsobjkey(tep)) {
            return false;
        } else {
            if (ip_addr_is_equal(&spec->erspan_spec.ip_addr, &dst_ip_addr) ==
                false) {
                return false;
            }
        }
    }
    return true;
}

bool
mirror_session_feeder::status_compare(
    const pds_mirror_session_status_t *status1,
    const pds_mirror_session_status_t *status2) const {
    return true;
}

}    // namespace api
}    // namespace test
