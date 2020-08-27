//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#include <vlib/vlib.h>

extern void tcp_poll_proc();

uword
pds_tcp_process (vlib_main_t * vm,
                 vlib_node_runtime_t * rt, vlib_frame_t * f)
{
    uword *event_data = 0;

    while (1) {
        vec_reset_length (event_data);

        //  Call tcp control msg poll/processing
        tcp_poll_proc();

        vlib_process_wait_for_event_or_clock (vm, 5);
        (void) vlib_process_get_events (vm, &event_data);
    }
    return 0;
}

/* *INDENT-OFF* */
VLIB_REGISTER_NODE (pds_tcp_node, static) =
{
  .function = pds_tcp_process,
  .type = VLIB_NODE_TYPE_PROCESS,
  .name = "pds-tcp-process",
};
/* *INDENT-ON* */

