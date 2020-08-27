//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#include <stddef.h>
#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/tcp/tcp.h>
#include <vnet/session/session.h>
#include <vnet/dpo/load_balance.h>
#include <vnet/plugin/plugin.h>
#include "tcp_prog_hw.hpp"

static ip4_proxy_params_t ip4_proxy_params;

void
ip4_proxy_set_params(uint32_t my_ip,
                     uint32_t client_addr4,
                     uint32_t server_addr4)
{
    ip4_proxy_params.my_ip = my_ip;
    ip4_proxy_params.client_ip = client_addr4;
    ip4_proxy_params.server_ip = server_addr4;
}

ip4_proxy_params_t *
ip4_proxy_get_params(void)
{
    return &ip4_proxy_params;
}

////////////////////////////////////////////////////////////////////////////////
// ip-proxy-rx
////////////////////////////////////////////////////////////////////////////////
VNET_FEATURE_INIT (ip_proxy_rx, static) =
{
    .arc_name = "ip4-unicast",
    .node_name = "ip4-proxy-rx",
    .runs_before = VNET_FEATURES ("ip4-lookup"),
};

always_inline uword
ip_proxy_rx_inline (vlib_main_t * vm,
			vlib_node_runtime_t * node,
			vlib_frame_t * frame, u32 is_ip4)
{
    u32 *from, *to_next, n_left_from, n_left_to_next, next_index;

    from = vlib_frame_vector_args (frame);
    n_left_from = frame->n_vectors;
    next_index = node->cached_next_index;

    if (node->flags & VLIB_NODE_FLAG_TRACE)
        ip4_forward_next_trace (vm, node, frame, VLIB_TX);

    while (n_left_from > 0)
    {
        vlib_get_next_frame (vm, node, next_index, to_next, n_left_to_next);

        while (n_left_from > 0 && n_left_to_next > 0)
        {
            vlib_buffer_t *b0;
            ip4_header_t *ip40;
            u32 bi0, next0;
            u8 proto0;
            ip4_address_t old_dst_ip0;
            ip_csum_t sum0;

            bi0 = to_next[0] = from[0];
            from += 1;
            n_left_from -= 1;
            to_next += 1;
            n_left_to_next -= 1;

            b0 = vlib_get_buffer (vm, bi0);
            if (is_ip4) {
                ip40 = vlib_buffer_get_current (b0);
                proto0 = ip40->protocol;
                /*
                 * dst nat
                 * Very basic matching based on protocol and fixed client/server
                 * address
                 */
                if (proto0 == IP_PROTOCOL_TCP &&
                    (ip40->dst_address.as_u32 == ip4_proxy_params.client_ip ||
                     ip40->dst_address.as_u32 == ip4_proxy_params.server_ip)) {
                    old_dst_ip0.as_u32 = ip40->dst_address.as_u32;

                    // update checksum
                    sum0 = ip40->checksum;
                    sum0 = ip_csum_update (sum0, old_dst_ip0.as_u32,
                            ip4_proxy_params.my_ip, ip4_header_t,
                            dst_address.as_u32 /* changed member */ );
                    ip40->checksum = ip_csum_fold (sum0);
                    ip40->dst_address.as_u32 = ip4_proxy_params.my_ip;
                }
            }

            /* Setup packet for next IP feature */
            vnet_feature_next (&next0, b0);

            vlib_validate_buffer_enqueue_x1 (vm, node, next_index,
					   to_next, n_left_to_next,
					   bi0, next0);
        }

        vlib_put_next_frame (vm, node, next_index, n_left_to_next);
    }

    return frame->n_vectors;
}

VLIB_NODE_FN (ip_proxy_rx_node) (vlib_main_t * vm,
				      vlib_node_runtime_t * node,
				      vlib_frame_t * frame)
{
    return ip_proxy_rx_inline (vm, node, frame, /* is_ip4 */ 1);
}

VLIB_REGISTER_NODE (ip_proxy_rx_node) =
{
    .name = "ip4-proxy-rx",
    .vector_size = sizeof (u32),
    //.n_next_nodes = IP_VXLAN_BYPASS_N_NEXT,
    //.next_nodes = {
      //[IP_VXLAN_BYPASS_NEXT_DROP] = "error-drop",
      //[IP_VXLAN_BYPASS_NEXT_VXLAN] = "vxlan4-input",
    //},
    .format_buffer = format_ip4_header,
    .format_trace = format_ip4_forward_next_trace,
};


////////////////////////////////////////////////////////////////////////////////
// ip-proxy-tx
////////////////////////////////////////////////////////////////////////////////
VNET_FEATURE_INIT (ip_proxy_tx, static) =
{
    .arc_name = "ip4-output",
    .node_name = "ip4-proxy-tx",
    .runs_before = VNET_FEATURES ("interface-output"),
};

always_inline uword
ip_proxy_tx_inline (vlib_main_t * vm,
			vlib_node_runtime_t * node,
			vlib_frame_t * frame, u32 is_ip4)
{
    u32 *from, *to_next, n_left_from, n_left_to_next, next_index;

    from = vlib_frame_vector_args (frame);
    n_left_from = frame->n_vectors;
    next_index = node->cached_next_index;

    while (n_left_from > 0)
    {
        vlib_get_next_frame (vm, node, next_index, to_next, n_left_to_next);

        while (n_left_from > 0 && n_left_to_next > 0)
        {
            vlib_buffer_t *b0;
            ip4_header_t *ip40;
            u32 bi0, next0, iph_offset;
            u8 proto0;
            ip4_address_t new_src_ip0 = { 0 };
            ip4_address_t old_src_ip0;
            ip_csum_t sum0;

            bi0 = to_next[0] = from[0];
            from += 1;
            n_left_from -= 1;
            to_next += 1;
            n_left_to_next -= 1;

            b0 = vlib_get_buffer (vm, bi0);
            iph_offset = vnet_buffer (b0)->ip.save_rewrite_length;
            if (is_ip4) {
                ip40 = vlib_buffer_get_current (b0) + iph_offset;
                proto0 = ip40->protocol;
                /*
                 * src nat
                 * Very basic matching based on protocol and fixed client/server
                 * address
                 */
                if (proto0 == IP_PROTOCOL_TCP) {
                    if (ip40->dst_address.as_u32 == ip4_proxy_params.client_ip)
                        new_src_ip0.as_u32 = ip4_proxy_params.server_ip;
                    else if (ip40->dst_address.as_u32 == ip4_proxy_params.server_ip)
                        new_src_ip0.as_u32 = ip4_proxy_params.client_ip;
                    if (new_src_ip0.as_u32) {
                        b0->flags |= VNET_BUFFER_F_OFFLOAD_TCP_CKSUM;
                        old_src_ip0.as_u32 = ip40->src_address.as_u32;

                        // update checksum
                        sum0 = ip40->checksum;
                        sum0 = ip_csum_update (sum0, old_src_ip0.as_u32,
                                new_src_ip0.as_u32, ip4_header_t,
                                src_address.as_u32 /* changed member */ );
                        ip40->checksum = ip_csum_fold (sum0);
                        ip40->src_address.as_u32 = new_src_ip0.as_u32;
                    }
                }
            }

            /* Setup packet for next IP feature */
            vnet_feature_next (&next0, b0);

            vlib_validate_buffer_enqueue_x1 (vm, node, next_index,
					   to_next, n_left_to_next,
					   bi0, next0);
        }

        vlib_put_next_frame (vm, node, next_index, n_left_to_next);
    }

    return frame->n_vectors;
}

VLIB_NODE_FN (ip_proxy_tx_node) (vlib_main_t * vm,
				      vlib_node_runtime_t * node,
				      vlib_frame_t * frame)
{
    return ip_proxy_tx_inline (vm, node, frame, /* is_ip4 */ 1);
}


VLIB_REGISTER_NODE (ip_proxy_tx_node) = {
    .name = "ip4-proxy-tx",
    .vector_size = sizeof (u32),
};

////////////////////////////////////////////////////////////////////////////////
// tcp-proxy enable cli
////////////////////////////////////////////////////////////////////////////////
static clib_error_t *
tcp_proxy_enable_command (vlib_main_t *vm,
                          unformat_input_t *input,
                          vlib_cli_command_t *cmd)
{
    int ret;
    unformat_input_t _line_input, *line_input = &_line_input;
    clib_error_t *error = NULL;

    /* Get a line of input */
    if (unformat_user(input, unformat_line_input, line_input)) {
        error = unformat_parse_error (line_input);
        goto done;
    }

    ret = vnet_feature_enable_disable("ip4-unicast", "ip4-proxy-rx", 1, 1, 0, 0);
    ret = vnet_feature_enable_disable("ip4-output", "ip4-proxy-tx", 1, 1, 0, 0);

    vlib_cli_output(vm, "tcp-proxy enabled! ret = %d", ret);
done:
    return error;
}

VLIB_CLI_COMMAND (ip_proxy_command, static) =
{
    .path = "tcp-proxy enable",
    .short_help = "tcp-proxy enable <dst-ip-addr>/<width>",
    .function = tcp_proxy_enable_command,
};

