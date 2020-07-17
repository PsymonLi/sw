/*****************************************************************************/
/* QOS table processing                                                      */
/*****************************************************************************/
action qos_info(tm_oq, dst_tm_oq) {
    /* output queue selection */
    modify_field(capri_intrinsic.tm_oq, tm_oq);
    modify_field(p4i_i2e.dst_tm_oq, dst_tm_oq);
}

@pragma stage 3
table qos {
    reads {
        control_metadata.qos_class_id   : exact;
    }
    actions {
        qos_info;
    }
    size: QOS_TABLE_SIZE;
}

control qos {
    apply(qos);
}
