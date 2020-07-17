action gettimeofday(ticks, time_in_ns, time_in_ms, multiplier_ns,
                    multiplier_ms, pad) {
    if (multiplier_ns == 0) {
        // return;
    }

    modify_field(scratch_metadata.ticks, 0 /* current ticks from R4 */);
    if (scratch_metadata.ticks > ticks) {
        modify_field(scratch_metadata.ticks, scratch_metadata.ticks - ticks);
    } else {
        modify_field(scratch_metadata.ticks,
                     0xFFFFFFFFFFFF + (scratch_metadata.ticks - ticks));
    }
    modify_field(scratch_metadata.delta,
                 (scratch_metadata.ticks * multiplier_ns) >> 31);
    modify_field(scratch_metadata.current_time_in_ns,
                 time_in_ns + scratch_metadata.delta);

    modify_field(erspan3.timestamp, scratch_metadata.current_time_in_ns);
    modify_field(erspan3_opt.timestamp,
                 scratch_metadata.current_time_in_ns >> 32);

    modify_field(scratch_metadata.size64, ticks);
    modify_field(scratch_metadata.size64, time_in_ns);
    modify_field(scratch_metadata.size64, time_in_ms);
    modify_field(scratch_metadata.size64, multiplier_ns);
    modify_field(scratch_metadata.size64, multiplier_ms);
    modify_field(scratch_metadata.pad192, pad);
}

@pragma stage 3
@pragma hbm_table
@pragma index_table
table clock {
    reads {
        control_metadata.clock_id   : exact;
    }
    actions {
        gettimeofday;
    }
    size : 1;
}
