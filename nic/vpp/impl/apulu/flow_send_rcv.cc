//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
#include <session_internal.pb.h>
#include <ftl_wrapper.h>
#include <nh.h>
#include <nic/apollo/p4/include/defines.h>
#include "session.h"

bool
pds_decode_one_v4_session (const uint8_t *data, const uint8_t len,
                           sess_info_t *sess, const uint16_t thread_index)
{
    static thread_local ::vppinternal::SessV4Info info;
    uint32_t nhid;

    // reset all internal state
    info.Clear();
    info.ParseFromArray(data, len);

    if (info.ismisshit()) {
        // This session is in transient state. Skip programing it.
        return false;
    }

    // Fill the Initiator flow from the protobuf
    ftlv4_cache_set_key(info.isrcip(), info.idstip(), info.proto(),
                        info.isrcport(), info.idstport(), info.ingressbd(),
                        thread_index);
    ftlv4_cache_set_session_index(info.sessid(), thread_index);
    ftlv4_cache_set_flow_role(TCP_FLOW_INITIATOR, thread_index);
    ftlv4_cache_set_l2l(info.islocaltolocal(), thread_index);
    ftlv4_cache_set_flow_miss_hit(0, thread_index);
    ftlv4_cache_set_update_flag(0, thread_index);
    if (info.isflowdrop()) {
        nhid = pds_nh_drop_id_get();
    } else {
        nhid = info.inhid();
    }
    ftlv4_cache_set_nexthop(nhid, info.inhtype(), info.inhvalid(),
                            info.ipriority(), thread_index);
    ftlv4_cache_set_hash_log(0, 0, thread_index);
    // TODO: Setting epoch
    ftlv4_cache_advance_count(1, thread_index);

    // Fill the Responder flow from the protobuf
    ftlv4_cache_set_key(info.rsrcip(), info.rdstip(), info.proto(),
                        info.rsrcport(), info.rdstport(), info.egressbd(),
                        thread_index);
    ftlv4_cache_set_session_index(info.sessid(), thread_index);
    ftlv4_cache_set_flow_role(TCP_FLOW_RESPONDER, thread_index);
    ftlv4_cache_set_l2l(info.islocaltolocal(), thread_index);
    ftlv4_cache_set_flow_miss_hit(0, thread_index);
    ftlv4_cache_set_update_flag(0, thread_index);
    if (info.isflowdrop()) {
        nhid = pds_nh_drop_id_get();
    } else {
        nhid = info.rnhid();
    }
    ftlv4_cache_set_nexthop(nhid, info.rnhtype(), info.rnhvalid(),
                            info.rpriority(), thread_index);
    ftlv4_cache_set_hash_log(0, 0, thread_index);
    // TODO: Setting epoch
    ftlv4_cache_advance_count(1, thread_index);

    // FIXME: fill NAT related fields

    sess->session_id = info.sessid();
    sess->proto = info.proto();
    sess->v4 = true;
    sess->flow_state = info.state();
    sess->packet_type = info.subtype();
    sess->ingress_bd = info.ingressbd();
    sess->iflow_rx = info.isiflowrx();
    sess->drop = info.isflowdrop();
    sess->vnic_id = info.vnicid();

    return true;
}

void
pds_encode_one_v4_session (uint8_t *data, uint8_t *len, sess_info_t *sess,
                           const uint16_t thread_index)
{
    static thread_local ::vppinternal::SessV4Info info;
    v4_flow_entry iflow, rflow;

    // reset all internal state
    info.Clear();

    // Read the iflow and rflow entries
    ftlv4_get_flow_entry((ftlv4 *)sess->flow_table, sess->iflow_handle,
                         &iflow, thread_index);
    ftlv4_get_flow_entry((ftlv4 *)sess->flow_table, sess->rflow_handle,
                         &rflow, thread_index);

    info.set_sessid(iflow.session_index);
    info.set_state(::vppinternal::FlowState(sess->flow_state));
    info.set_subtype(::vppinternal::FlowSubType(sess->packet_type));
    info.set_isiflowrx(sess->iflow_rx);
    info.set_isflowdrop(sess->drop);
    info.set_vnicid(sess->vnic_id);
    // don't encode NAT related fields

    // initiator flow attributes - client to server
    info.set_ingressbd(iflow.key_metadata_flow_lkp_id);
    info.set_isrcip(iflow.key_metadata_ipv4_src);
    info.set_idstip(iflow.key_metadata_ipv4_dst);
    info.set_isrcport(iflow.key_metadata_sport);
    info.set_idstport(iflow.key_metadata_dport);
    info.set_proto(iflow.key_metadata_proto);
    info.set_inhid(iflow.nexthop_id);
    info.set_inhtype(iflow.nexthop_type);
    info.set_inhvalid(iflow.nexthop_valid);
    info.set_ipriority(iflow.priority);
    info.set_islocaltolocal(iflow.is_local_to_local);
    info.set_ismisshit(iflow.force_flow_miss);
    // TBD: Encoding  epoch

    // responder flow attributes - server to client
    info.set_egressbd(rflow.key_metadata_flow_lkp_id);
    info.set_rsrcip(rflow.key_metadata_ipv4_src);
    info.set_rdstip(rflow.key_metadata_ipv4_dst);
    info.set_rsrcport(rflow.key_metadata_sport);
    info.set_rdstport(rflow.key_metadata_dport);
    info.set_rnhid(rflow.nexthop_id);
    info.set_rnhtype(rflow.nexthop_type);
    info.set_rnhvalid(rflow.nexthop_valid);
    info.set_rpriority(rflow.priority);

    // FIXME: fill NAT related fields

    // Encode the protobuf into the passed buffer
    size_t req_sz = info.ByteSizeLong();
    //assert(req_sz < 192);
    *len = req_sz;
    info.SerializeWithCachedSizesToArray(data);
}
