//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
#include <sess_sync.pb.h>
#include <ftl_wrapper.h>
#include <nh.h>
#include <nic/apollo/p4/include/defines.h>
#include "sess_restore.h"

bool
pds_decode_one_v4_session (const uint8_t *data, const uint8_t len,
                           sess_info_t *sess, const uint16_t thread_index)
{
    static thread_local ::sess_sync::SessV4Info info;
    uint32_t nhid;

    // reset all internal state
    info.Clear();
    info.ParseFromArray(data, len);

    if (info.ismisshit()) {
        // This session is in transient state. Skip programing it.
        return false;
    }

    // Fill the Initiator flow from the protobuf
    ftlv4_cache_set_key(info.initiatorflowsrcip(), info.initiatorflowdstip(),
                        info.proto(), info.initiatorflowsrcport(),
                        info.initiatorflowdstport(), info.ingressbd(),
                        thread_index);
    ftlv4_cache_set_session_index(info.id(), thread_index);
    ftlv4_cache_set_flow_role(TCP_FLOW_INITIATOR, thread_index);
    ftlv4_cache_set_l2l(info.islocaltolocal(), thread_index);
    ftlv4_cache_set_flow_miss_hit(0, thread_index);
    ftlv4_cache_set_update_flag(0, thread_index);
    if (info.isflowdrop()) {
        nhid = pds_nh_drop_id_get();
    } else {
        nhid = info.initiatorflownhid();
    }
    ftlv4_cache_set_nexthop(nhid, info.initiatorflownhtype(),
                            info.initiatorflownhvalid(),
                            info.initiatorflowpriority(), thread_index);
    ftlv4_cache_set_hash_log(0, 0, thread_index);
    // TODO: Setting epoch
    ftlv4_cache_advance_count(1, thread_index);

    // Fill the Responder flow from the protobuf
    ftlv4_cache_set_key(info.responderflowsrcip(), info.responderflowdstip(),
                        info.proto(), info.responderflowsrcport(),
                        info.responderflowdstport(), info.egressbd(),
                        thread_index);
    ftlv4_cache_set_session_index(info.id(), thread_index);
    ftlv4_cache_set_flow_role(TCP_FLOW_RESPONDER, thread_index);
    ftlv4_cache_set_l2l(info.islocaltolocal(), thread_index);
    ftlv4_cache_set_flow_miss_hit(0, thread_index);
    ftlv4_cache_set_update_flag(0, thread_index);
    if (info.isflowdrop()) {
        nhid = pds_nh_drop_id_get();
    } else {
        nhid = info.responderflownhid();
    }
    ftlv4_cache_set_nexthop(nhid, info.responderflownhtype(),
                            info.responderflownhvalid(),
                            info.responderflowpriority(), thread_index);
    ftlv4_cache_set_hash_log(0, 0, thread_index);
    // TODO: Setting epoch
    ftlv4_cache_advance_count(1, thread_index);

    // FIXME: fill NAT related fields

    sess->id = info.id();
    sess->proto = info.proto();
    sess->v4 = true;
    sess->flow_state = info.state();
    sess->packet_type = info.subtype();
    sess->ingress_bd = info.ingressbd();
    sess->iflow_rx = info.isinitiatorflowrx();
    sess->drop = info.isflowdrop();
    sess->vnic_id = info.vnicid();

    return true;
}

void
pds_encode_one_v4_session (uint8_t *data, uint8_t *len, sess_info_t *sess,
                           const uint16_t thread_index)
{
    static thread_local ::sess_sync::SessV4Info info;
    v4_flow_entry iflow, rflow;

    // reset all internal state
    info.Clear();

    // Read the iflow and rflow entries
    ftlv4_get_flow_entry((ftlv4 *)sess->flow_table, sess->iflow_handle,
                         &iflow, thread_index);
    ftlv4_get_flow_entry((ftlv4 *)sess->flow_table, sess->rflow_handle,
                         &rflow, thread_index);

    info.set_id(iflow.session_index);
    info.set_state(::sess_sync::FlowState(sess->flow_state));
    info.set_subtype(::sess_sync::FlowSubType(sess->packet_type));
    info.set_isinitiatorflowrx(sess->iflow_rx);
    info.set_isflowdrop(sess->drop);
    info.set_vnicid(sess->vnic_id);
    // don't encode NAT related fields

    // initiator flow attributes - client to server
    info.set_ingressbd(iflow.key_metadata_flow_lkp_id);
    info.set_initiatorflowsrcip(iflow.key_metadata_ipv4_src);
    info.set_initiatorflowdstip(iflow.key_metadata_ipv4_dst);
    info.set_initiatorflowsrcport(iflow.key_metadata_sport);
    info.set_initiatorflowdstport(iflow.key_metadata_dport);
    info.set_proto(iflow.key_metadata_proto);
    info.set_initiatorflownhid(iflow.nexthop_id);
    info.set_initiatorflownhtype(iflow.nexthop_type);
    info.set_initiatorflownhvalid(iflow.nexthop_valid);
    info.set_initiatorflowpriority(iflow.priority);
    info.set_islocaltolocal(iflow.is_local_to_local);
    info.set_ismisshit(iflow.force_flow_miss);
    // TBD: Encoding  epoch

    // responder flow attributes - server to client
    info.set_egressbd(rflow.key_metadata_flow_lkp_id);
    info.set_responderflowsrcip(rflow.key_metadata_ipv4_src);
    info.set_responderflowdstip(rflow.key_metadata_ipv4_dst);
    info.set_responderflowsrcport(rflow.key_metadata_sport);
    info.set_responderflowdstport(rflow.key_metadata_dport);
    info.set_responderflownhid(rflow.nexthop_id);
    info.set_responderflownhtype(rflow.nexthop_type);
    info.set_responderflownhvalid(rflow.nexthop_valid);
    info.set_responderflowpriority(rflow.priority);

    // FIXME: fill NAT related fields

    // Encode the protobuf into the passed buffer
    size_t req_sz = info.ByteSizeLong();
    //assert(req_sz < 192);
    *len = req_sz;
    info.SerializeWithCachedSizesToArray(data);
}
