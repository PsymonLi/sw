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
                            std::string dst_ip, uint32_t tep, uint32_t span_id,
                            uint32_t dscp, bool vlan_strip_en,
                            pds_mirror_session_type_t type, uint16_t snap_len,
                            pds_erspan_type_t erspan_type,
                            pds_erspan_dst_type_t erspan_dst_type,
                            uint32_t vpc, bool stash) {
    memset(&spec, 0, sizeof(pds_mirror_session_spec_t));
    this->num_obj = mirror_sess;
    this->stash_ = stash;
    this->dst_ip = dst_ip;
    this->type = type;
    this->span_id = span_id;
    this->vlan_strip_en = vlan_strip_en;
    this->erspan_type = erspan_type;
    this->dst_type = erspan_dst_type;
    this->interface = interface;
    this->encap.type = PDS_ENCAP_TYPE_DOT1Q;
    this->encap.val.vlan_tag = vlan_tag;
    this->vpc = int2pdsobjkey(vpc);
    this->tep = tep;
    memset(&this->dst_ip_addr, 0x0, sizeof(ip_addr_t));
    test::extract_ip_addr((char *)dst_ip.c_str(), &this->dst_ip_addr);
    this->dscp = dscp;
    this->snap_len = snap_len;
    spec.key = key;
    create_mirror_session_spec((pds_mirror_session_spec_t *)&spec, type,
                               interface, vlan_tag, dst_ip, tep,
                               span_id, dscp, vlan_strip_en, snap_len,
                               erspan_type, erspan_dst_type, vpc);
}

void
mirror_session_feeder::iter_next(int width) {
    // update both feeder's variables and spec
    spec.key = int2pdsobjkey(pdsobjkey2int(spec.key) + width);
    if (type == PDS_MIRROR_SESSION_TYPE_RSPAN) {
        type = PDS_MIRROR_SESSION_TYPE_ERSPAN;
        spec.erspan_spec.vpc = vpc;
        spec.erspan_spec.span_id = ++span_id;
        spec.erspan_spec.dscp = ++dscp;
        vlan_strip_en = ~vlan_strip_en;
        spec.erspan_spec.vlan_strip_en = vlan_strip_en;
        if (erspan_type == PDS_ERSPAN_TYPE_1) {
            erspan_type = PDS_ERSPAN_TYPE_2;
        } else if (erspan_type == PDS_ERSPAN_TYPE_2) {
            erspan_type = PDS_ERSPAN_TYPE_3;
        } else {
            erspan_type = PDS_ERSPAN_TYPE_1;
        }
        spec.erspan_spec.type = erspan_type;
        if (dst_type == PDS_ERSPAN_DST_TYPE_TEP) {
            dst_type = PDS_ERSPAN_DST_TYPE_IP;
            test::increment_ip_addr(&dst_ip_addr, width);
            memcpy(&spec.erspan_spec.ip_addr, &dst_ip_addr, sizeof(ip_addr_t));
        } else {
            dst_type = PDS_ERSPAN_DST_TYPE_TEP;
            spec.erspan_spec.tep = int2pdsobjkey(tep);
        }
        spec.erspan_spec.dst_type = dst_type;
    } else if (type == PDS_MIRROR_SESSION_TYPE_ERSPAN) {
        type = PDS_MIRROR_SESSION_TYPE_RSPAN;
        spec.rspan_spec.interface = interface;
        if (encap.type  == PDS_ENCAP_TYPE_DOT1Q) {
            spec.rspan_spec.encap.type = PDS_ENCAP_TYPE_DOT1Q;
            spec.rspan_spec.encap.val.vlan_tag = ++encap.val.vlan_tag;
            spec.type = type;
        } else if (encap.type == PDS_ENCAP_TYPE_QINQ) {
            spec.rspan_spec.encap.type = PDS_ENCAP_TYPE_QINQ;
            spec.rspan_spec.encap.val.qinq.c_tag++;
            spec.rspan_spec.encap.val.qinq.s_tag++;
        }
    }
    spec.type = type;
    cur_iter_pos++;
}

void
mirror_session_feeder::key_build(pds_obj_key_t *key) const {
    memcpy(key, &this->spec.key, sizeof(pds_obj_key_t));
}

void
mirror_session_feeder::spec_build(pds_mirror_session_spec_t *spec) const {
    memcpy(spec, &this->spec, sizeof(pds_mirror_session_spec_t));
}

bool
mirror_session_feeder::key_compare(const pds_obj_key_t *key) const {
    if (this->spec.key != *key)
        return false;
    return true;
}

bool
mirror_session_feeder::spec_compare(
                                const pds_mirror_session_spec_t *spec) const {
    if (spec->type != this->spec.type)
        return false;
    if (spec->snap_len != this->spec.snap_len)
        return false;
    if (spec->type == PDS_MIRROR_SESSION_TYPE_RSPAN) {
        // validate rspan spec
        if ((spec->rspan_spec.interface != this->spec.rspan_spec.interface) ||
            (spec->rspan_spec.encap.type != this->spec.rspan_spec.encap.type) ||
            (spec->rspan_spec.encap.val.vlan_tag !=
             this->spec.rspan_spec.encap.val.vlan_tag))
            return false;
    } else if (spec->type == PDS_MIRROR_SESSION_TYPE_ERSPAN) {
        // validate erspan spec
        if ((spec->erspan_spec.type != this->spec.erspan_spec.type) ||
            (spec->erspan_spec.dst_type != this->spec.erspan_spec.dst_type) ||
            (spec->erspan_spec.vlan_strip_en !=
             this->spec.erspan_spec.vlan_strip_en))
            return false;
        // @sarat we can uncomment below code post the hw_id_ fix
        /*
        if ((spec->erspan_spec.vpc != this->spec.erspan_spec.vpc) ||
            (spec->erspan_spec.dscp != this->spec.erspan_spec.dscp) ||
            (spec->erspan_spec.span_id != this->spec.erspan_spec.span_id))
            return false;
        if (spec->erspan_spec.dst_type == PDS_ERSPAN_DST_TYPE_TEP &&
            spec->erspan_spec.tep != this->spec.erspan_spec.tep) {
            return false;
        } else {
            if (ip_addr_is_equal(&spec->erspan_spec.ip_addr,
                &this->spec.erspan_spec.ip_addr) == false) {
                return false;
            }
        }
        */
    }
    return true;
}

bool
mirror_session_feeder::status_compare(
    const pds_mirror_session_status_t *status1,
    const pds_mirror_session_status_t *status2) const {
    return true;
}

void
create_mirror_session_spec (pds_mirror_session_spec_t* spec,
                            pds_mirror_session_type_t type,
                            pds_obj_key_t interface, uint16_t vlan_tag,
                            std::string dst_ip, uint32_t tep,
                            uint32_t span_id, uint32_t dscp,
                            bool vlan_strip_en, uint16_t snap_len,
                            pds_erspan_type_t erspan_type,
                            pds_erspan_dst_type_t erspan_dst_type,
                            uint32_t vpc)
{
    spec->snap_len = snap_len;
    spec->type = type;
    if (spec->type == PDS_MIRROR_SESSION_TYPE_RSPAN) {
        spec->rspan_spec.interface = interface;
        spec->rspan_spec.encap.type = PDS_ENCAP_TYPE_DOT1Q;
        spec->rspan_spec.encap.val.vlan_tag = vlan_tag;
    }
    else if(spec->type == PDS_MIRROR_SESSION_TYPE_ERSPAN) {
        spec->erspan_spec.type = erspan_type;
        spec->erspan_spec.vpc = int2pdsobjkey(vpc);
        spec->erspan_spec.dst_type = erspan_dst_type;
        if (erspan_dst_type == PDS_ERSPAN_DST_TYPE_TEP) {
            spec->erspan_spec.tep = int2pdsobjkey(tep);
        } else if (erspan_dst_type == PDS_ERSPAN_DST_TYPE_IP) {
            memset(&spec->erspan_spec.ip_addr, 0x0, sizeof(ip_addr_t));
            test::extract_ip_addr((char *)dst_ip.c_str(),
                                  &spec->erspan_spec.ip_addr);
        }
        spec->erspan_spec.dscp = dscp;
        spec->erspan_spec.span_id = span_id;
        spec->erspan_spec.vlan_strip_en = vlan_strip_en;
    }
}

void
mirror_session_create (mirror_session_feeder& feeder) {
    pds_batch_ctxt_t bctxt = batch_start();
    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_create<mirror_session_feeder>(bctxt, feeder)));
    batch_commit(bctxt);
}

void
mirror_session_read (mirror_session_feeder& feeder, sdk_ret_t exp_result) {
    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_read<mirror_session_feeder>(feeder, exp_result)));
}

static void
mirror_session_attr_update (mirror_session_feeder& feeder,
                            pds_mirror_session_spec_t *spec,
                            uint64_t chg_bmap) {
    if (bit_isset(chg_bmap, MIRROR_SESSION_ATTR_SNAP_LEN)) {
        feeder.spec.snap_len = spec->snap_len;
    }
    if (bit_isset(chg_bmap, MIRROR_SESSION_ATTR_TYPE)) {
        feeder.spec.type = spec->type;
    }
    if (bit_isset(chg_bmap, MIRROR_SESSION_ATTR_RSPAN)) {
        feeder.spec.rspan_spec.interface = spec->rspan_spec.interface;
        memcpy(&feeder.spec.rspan_spec.encap, &spec->rspan_spec.encap,
                sizeof(pds_encap_t));
    }
    if (bit_isset(chg_bmap, MIRROR_SESSION_ATTR_ERSPAN)) {
        feeder.spec.erspan_spec.type = spec->erspan_spec.type;
        feeder.spec.erspan_spec.vpc = spec->erspan_spec.vpc;
        feeder.spec.erspan_spec.dst_type = spec->erspan_spec.dst_type;
        feeder.spec.erspan_spec.tep = spec->erspan_spec.tep;
        feeder.spec.erspan_spec.dscp = spec->erspan_spec.dscp;
        feeder.spec.erspan_spec.span_id = spec->erspan_spec.span_id;
        feeder.spec.erspan_spec.vlan_strip_en =
            spec->erspan_spec.vlan_strip_en;
    }
}

void
mirror_session_update (mirror_session_feeder& feeder,
                       pds_mirror_session_spec_t *spec,
                       uint64_t chg_bmap, sdk_ret_t exp_result) {
    pds_batch_ctxt_t bctxt = batch_start();
    mirror_session_attr_update(feeder, spec, chg_bmap);
    SDK_ASSERT_RETURN_VOID(
            (SDK_RET_OK == many_update<mirror_session_feeder>(bctxt, feeder)));
    //if expected result is err, batch commit should fail
    if (exp_result == SDK_RET_ERR) {
        batch_commit_fail(bctxt);
    } else {
        batch_commit(bctxt);
    }
}

void
mirror_session_delete (mirror_session_feeder& feeder) {
    pds_batch_ctxt_t bctxt = batch_start();
    SDK_ASSERT_RETURN_VOID(
        (SDK_RET_OK == many_delete<mirror_session_feeder>(bctxt, feeder)));
    batch_commit(bctxt);
}

}    // namespace api
}    // namespace test
