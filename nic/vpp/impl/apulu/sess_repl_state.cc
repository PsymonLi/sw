//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
#include <sess_sync.pb.h>
#include <ftl_wrapper.h>
#include <nh.h>
#include <nic/apollo/p4/include/defines.h>
#include <sess.h>
#include "sess_restore.h"
#include <flow_info.h>
#include "gen/p4gen/p4/include/ftl.h"

bool
pds_decode_one_v4_session (const uint8_t *data, const uint8_t len,
                           sess_info_t *sess, const uint16_t thread_index)
{
    static thread_local ::sess_sync::SessInfo info;
    uint32_t nhid;

    // reset all internal state
    info.Clear();
    info.ParseFromArray(data, len);

    if (info.type() != ::sess_sync::FLOW_TYPE_IPV4) {
        // Skip non ipv4 sessions
        return false;
    }
    if (info.ismisshit()) {
        // This session is in transient state. Skip programing it.
        return false;
    }

    // Fill the Initiator flow from the protobuf
    ftlv4_cache_set_key(info.initiatorflowsrcipv4(),
                        info.initiatorflowdstipv4(),
                        info.ipprotocol(), info.initiatorflowsrcport(),
                        info.initiatorflowdstport(), info.ingressbd(),
                        thread_index);
    ftlv4_cache_set_session_index(info.id(), thread_index);
    ftlv4_cache_set_flow_role(TCP_FLOW_INITIATOR, thread_index);
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
    ftlv4_cache_set_epoch(info.initiatorflowepoch(), thread_index);
    ftlv4_cache_set_hash_log(0, 0, thread_index);
    ftlv4_cache_advance_count(1, thread_index);

    // Fill the Responder flow from the protobuf
    ftlv4_cache_set_key(info.responderflowsrcipv4(),
                        info.responderflowdstipv4(),
                        info.ipprotocol(), info.responderflowsrcport(),
                        info.responderflowdstport(), info.egressbd(),
                        thread_index);
    ftlv4_cache_set_session_index(info.id(), thread_index);
    ftlv4_cache_set_flow_role(TCP_FLOW_RESPONDER, thread_index);
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
    ftlv4_cache_set_epoch(info.responderflowepoch(), thread_index);
    ftlv4_cache_set_hash_log(0, 0, thread_index);
    ftlv4_cache_advance_count(1, thread_index);

    // Fill the flow info from the protobuf
    pds_flow_info_program(info.id(), info.islocaltolocal());

    sess->id = info.id();
    sess->proto = info.ipprotocol();
    sess->v4 = true;
    sess->flow_state = info.state();
    sess->packet_type = info.pkttype();
    sess->ingress_bd = info.ingressbd();
    sess->iflow_rx = info.isinitiatorflowrx();
    sess->drop = info.isflowdrop();
    sess->src_vnic_id = info.srcvnicid();
    sess->dst_vnic_id = info.dstvnicid();
    sess->napt = info.isnapt();
    sess->nat = info.isnat();

    // Fill NAT related fields
    if (sess->nat) {
        sess->rx_xlate_id = info.rxsrcxlateid();
        sess->tx_xlate_id = info.txsrcxlateid();
        sess->rx_xlate_id2 = info.rxdstxlateid();
        sess->tx_xlate_id2 = info.txdstxlateid();
    } else {
        sess->rx_xlate_id = 0;
        sess->tx_xlate_id = 0;
        sess->rx_xlate_id2 = 0;
        sess->tx_xlate_id2 = 0;
    }

    return true;
}

void
pds_encode_one_v4_session (uint8_t *data, uint8_t *len, sess_info_t *sess,
                           const uint16_t thread_index)
{
    static thread_local ::sess_sync::SessInfo info;
    v4_flow_entry iflow, rflow;
    flow_info_entry_t flow_info;
    session_info_t session_entry;

    // reset all internal state
    info.Clear();

    // Read the session entry for xlate indices
    if (sess->nat) {
        pds_session_get_info(sess->id, &session_entry);
    }

    // Read the iflow and rflow entries
    ftlv4_get_flow_entry((ftlv4 *)sess->flow_table, sess->iflow_handle,
                         &iflow, thread_index);
    ftlv4_get_flow_entry((ftlv4 *)sess->flow_table, sess->rflow_handle,
                         &rflow, thread_index);

    // read the flow info table
    pds_flow_info_get(iflow.session_index, (void *)&flow_info);

    info.set_id(iflow.session_index);
    info.set_state(::sess_sync::FlowState(sess->flow_state));
    info.set_type(::sess_sync::FLOW_TYPE_IPV4);
    info.set_pkttype(::sess_sync::FlowPacketType(sess->packet_type));
    info.set_isinitiatorflowrx(sess->iflow_rx);
    info.set_isflowdrop(sess->drop);
    info.set_srcvnicid(sess->src_vnic_id);
    info.set_dstvnicid(sess->dst_vnic_id);
    info.set_isnapt(sess->napt);
    info.set_isnat(sess->nat);

    // initiator flow attributes - client to server
    info.set_ingressbd(iflow.key_metadata_flow_lkp_id);
    info.set_initiatorflowsrcipv4(iflow.key_metadata_ipv4_src);
    info.set_initiatorflowdstipv4(iflow.key_metadata_ipv4_dst);
    info.set_initiatorflowsrcport(iflow.key_metadata_sport);
    info.set_initiatorflowdstport(iflow.key_metadata_dport);
    info.set_ipprotocol(iflow.key_metadata_proto);
    info.set_initiatorflownhid(iflow.nexthop_id);
    info.set_initiatorflownhtype(iflow.nexthop_type);
    info.set_initiatorflownhvalid(iflow.nexthop_valid);
    info.set_initiatorflowpriority(iflow.priority);
    info.set_initiatorflowepoch(iflow.epoch);
    info.set_ismisshit(iflow.force_flow_miss);

    info.set_islocaltolocal(flow_info.is_local_to_local);

    // responder flow attributes - server to client
    info.set_egressbd(rflow.key_metadata_flow_lkp_id);
    info.set_responderflowsrcipv4(rflow.key_metadata_ipv4_src);
    info.set_responderflowdstipv4(rflow.key_metadata_ipv4_dst);
    info.set_responderflowsrcport(rflow.key_metadata_sport);
    info.set_responderflowdstport(rflow.key_metadata_dport);
    info.set_responderflownhid(rflow.nexthop_id);
    info.set_responderflownhtype(rflow.nexthop_type);
    info.set_responderflownhvalid(rflow.nexthop_valid);
    info.set_responderflowpriority(rflow.priority);
    info.set_responderflowepoch(rflow.epoch);

    // Fill NAT related fields
    if (sess->nat) {
        info.set_rxsrcxlateid(session_entry.rx_xlate_id);
        info.set_txsrcxlateid(session_entry.tx_xlate_id);
        info.set_rxdstxlateid(session_entry.rx_xlate_id2);
        info.set_txdstxlateid(session_entry.tx_xlate_id2);
    }

    // Encode the protobuf into the passed buffer
    size_t req_sz = info.ByteSizeLong();
    //assert(req_sz < 192);
    *len = req_sz;
    info.SerializeWithCachedSizesToArray(data);
}
