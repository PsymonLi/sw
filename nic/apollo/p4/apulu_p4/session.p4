/*****************************************************************************/
/* Session                                                                   */
/*****************************************************************************/
action session_info(iflow_tcp_state, iflow_tcp_seq_num, iflow_tcp_ack_num,
                    iflow_tcp_win_sz, iflow_tcp_win_scale, rflow_tcp_state,
                    rflow_tcp_seq_num, rflow_tcp_ack_num, rflow_tcp_win_sz,
                    rflow_tcp_win_scale, tx_policer_id, tx_rewrite_flags,
                    tx_xlate_id, rx_policer_id, rx_rewrite_flags, rx_xlate_id,
                    timestamp, drop) {
    subtract(capri_p4_intrinsic.packet_len, capri_p4_intrinsic.frame_size,
             offset_metadata.l2_1);
    modify_field(control_metadata.rx_packet, p4e_i2e.rx_packet);

    if (p4e_i2e.session_id == 0) {
        egress_drop(P4E_DROP_SESSION_INVALID);
    }

    modify_field(scratch_metadata.flag, drop);
    if (drop == TRUE) {
        egress_drop(P4E_DROP_SESSION_HIT);
    }

    // update stats and state only on first pass through egress pipeline
    if (egress_recirc.valid == FALSE) {
        modify_field(scratch_metadata.session_stats_addr,
                     scratch_metadata.session_stats_addr +
                     (p4e_i2e.session_id * 8 * 4));
        modify_field(scratch_metadata.in_bytes, capri_p4_intrinsic.packet_len);

        if (tcp.valid == TRUE) {
            modify_field(scratch_metadata.tcp_flags, tcp.flags);
            if (p4e_i2e.flow_role == TCP_FLOW_INITIATOR) {
                modify_field(scratch_metadata.tcp_state, iflow_tcp_state);
                modify_field(scratch_metadata.tcp_seq_num, iflow_tcp_seq_num);
                modify_field(scratch_metadata.tcp_ack_num, iflow_tcp_ack_num);
                modify_field(scratch_metadata.tcp_win_sz, iflow_tcp_win_sz);
                modify_field(scratch_metadata.tcp_win_scale, iflow_tcp_win_scale);
            } else {
                modify_field(scratch_metadata.tcp_state, rflow_tcp_state);
                modify_field(scratch_metadata.tcp_seq_num, rflow_tcp_seq_num);
                modify_field(scratch_metadata.tcp_ack_num, rflow_tcp_ack_num);
                modify_field(scratch_metadata.tcp_win_sz, rflow_tcp_win_sz);
                modify_field(scratch_metadata.tcp_win_scale, rflow_tcp_win_scale);
            }
        }

        modify_field(scratch_metadata.timestamp, timestamp);
    }

    if (p4e_i2e.rx_packet == FALSE) {
        modify_field(rewrite_metadata.policer_id, tx_policer_id);
        modify_field(rewrite_metadata.flags, tx_rewrite_flags);
        modify_field(rewrite_metadata.xlate_id, tx_xlate_id);

    } else {
        modify_field(rewrite_metadata.policer_id, rx_policer_id);
        modify_field(rewrite_metadata.flags, rx_rewrite_flags);
        modify_field(rewrite_metadata.xlate_id, rx_xlate_id);
    }
}

@pragma stage 0
@pragma hbm_table
@pragma table_write
@pragma index_table
table session {
    reads {
        p4e_i2e.session_id  : exact;
    }
    actions {
        session_info;
    }
    size : SESSION_TABLE_SIZE;
}

control session_lookup {
    apply(session);
}
