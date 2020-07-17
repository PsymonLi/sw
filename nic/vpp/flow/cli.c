//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#include <stddef.h>
#include <arpa/inet.h>
#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <sess.h>
#include <system.h>
#include <inttypes.h>
#include "node.h"
#include "cli_helper.h"
#include "pdsa_hdlr.h"
#include "stats.h"

// *INDENT-OFF*
VLIB_PLUGIN_REGISTER () = {
    .description = "Pensando Flow Plugin",
};
// *INDENT-ON*

extern u8 g_dis_reinject;

static const char *flow_state_str[] = {
    "Init",
    "TCPSynRcvd",
    "TCPAckTcvd",
    "TCPSynAckRcvd",
    "Established",
    "FinRcvd",
    "BidirFinRcvd",
    "Reset"
};

static const char *pds_flow_pkt_type_str[] = {
    "PDS_FLOW_L2L_INTRA_SUBNET",
    "PDS_FLOW_L2L_INTER_SUBNET",
    "PDS_FLOW_L2R_INTRA_SUBNET",
    "PDS_FLOW_L2R_INTER_SUBNET",
    "PDS_FLOW_R2L_INTRA_SUBNET",
    "PDS_FLOW_R2L_INTER_SUBNET",
    "PDS_FLOW_L2N_OVERLAY_ROUTE_EN",
    "PDS_FLOW_L2N_OVERLAY_ROUTE_EN_NAPT",
    "PDS_FLOW_L2N_OVERLAY_ROUTE_EN_NAT",
    "PDS_FLOW_L2N_OVERLAY_ROUTE_DIS",
    "PDS_FLOW_L2N_OVERLAY_ROUTE_DIS_NAPT",
    "PDS_FLOW_L2N_OVERLAY_ROUTE_DIS_NAT",
    "PDS_FLOW_L2N_OVERLAY_ROUTE_DIS_TWICE_NAT",
    "PDS_FLOW_L2N_INTRA_VCN_ROUTE",
    "PDS_FLOW_N2L_OVERLAY_ROUTE_EN",
    "PDS_FLOW_N2L_OVERLAY_ROUTE_EN_NAT",
    "PDS_FLOW_N2L_OVERLAY_ROUTE_DIS",
    "PDS_FLOW_N2L_OVERLAY_ROUTE_DIS_NAT",
    "PDS_FLOW_N2L_OVERLAY_ROUTE_EN_SVC_NAT",
    "PDS_FLOW_N2L_OVERLAY_ROUTE_DIS_SVC_NAT",
    "PDS_FLOW_N2L_INTRA_VCN_ROUTE",
    "PDS_FLOW_PKT_TYPE_MAX",
};

static const char *pds_flow_state_str[] = {
    "Init",
    "Setup",
    "Established",
    "KeepaliveSent",
    "HalfCloseIflow",
    "HalfCloseRflow",
    "Close",
    "PendingDelete"
};

static clib_error_t *
set_flow_test_command_fn (vlib_main_t * vm,
                          unformat_input_t * input,
                          vlib_cli_command_t * cmd)
{
    int skip_flow_prog = 0, skip_ses_prog = 0,
        dis_reinject = 0, reset = 0;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat(input, "skip-flow-program")) {
            skip_flow_prog = 1;
        } else if (unformat(input, "skip-session-program")) {
            skip_ses_prog = 1;
        } else if (unformat(input, "disable-reinject")) {
            dis_reinject = 1;
        } else if (unformat(input, "reset")) {
            reset = 1;
        } else {
            vlib_cli_output(vm, "ERROR: Invalid command");
            goto done;
        }
    }

    if (reset) {
        set_skip_ftl_program(0);
        set_skip_session_program(0);
        g_dis_reinject = 0;
        goto done;
    }
    set_skip_ftl_program(skip_flow_prog);
    set_skip_session_program(skip_ses_prog);
    g_dis_reinject = dis_reinject;

done:
    return 0;
}

VLIB_CLI_COMMAND (set_flow_test_command, static) =
{
    .path = "set flow test",
    .short_help = "set flow test [skip-flow-program] [skip-session-program] "
                  "[disable-reinject] [reset]",
    .function = set_flow_test_command_fn,
};

static void
get_worker_flow_stats_summary_ftlv4 (vlib_main_t * vm)
{
    pds_flow_main_t *fm = &pds_flow_main;

    ftlv4_cache_stats(fm->table4, vm->thread_index);
    clib_callback_enable_disable
        (vm->worker_thread_main_loop_callbacks,
         vm->worker_thread_main_loop_callback_tmp,
         vm->worker_thread_main_loop_callback_lock,
         (void *) get_worker_flow_stats_summary_ftlv4, 0 /* disable */ );
}

static void
get_worker_flow_stats_summary_ftlv6 (vlib_main_t * vm)
{
    pds_flow_main_t *fm = &pds_flow_main;

    ftlv6_cache_stats(fm->table6_or_l2, vm->thread_index);
    clib_callback_enable_disable
        (vm->worker_thread_main_loop_callbacks,
         vm->worker_thread_main_loop_callback_tmp,
         vm->worker_thread_main_loop_callback_lock,
         (void *) get_worker_flow_stats_summary_ftlv6, 0 /* disable */ );
}

static void
get_worker_flow_stats_ftlv4 (vlib_main_t * vm)
{
    pds_flow_main_t *fm = &pds_flow_main;

    ftlv4_dump_stats(fm->table4, fm->stats_buf, DISPLAY_BUF_SIZE - 1);
    clib_callback_enable_disable
        (vm->worker_thread_main_loop_callbacks,
         vm->worker_thread_main_loop_callback_tmp,
         vm->worker_thread_main_loop_callback_lock,
         (void *) get_worker_flow_stats_ftlv4, 0 /* disable */ );
}

static void
get_worker_flow_stats_ftlv6 (vlib_main_t * vm)
{
    pds_flow_main_t *fm = &pds_flow_main;

    ftlv6_dump_stats(fm->table6_or_l2, fm->stats_buf, DISPLAY_BUF_SIZE - 1);
    clib_callback_enable_disable
        (vm->worker_thread_main_loop_callbacks,
         vm->worker_thread_main_loop_callback_tmp,
         vm->worker_thread_main_loop_callback_lock,
         (void *) get_worker_flow_stats_ftlv6, 0 /* disable */ );
}

static clib_error_t *
show_flow_stats_command_fn (vlib_main_t * vm,
                            unformat_input_t * input,
                            vlib_cli_command_t * cmd)
{
    char *buf = NULL;
    pds_flow_main_t *fm = &pds_flow_main;
    int no_of_threads = fm->no_threads;
    u32 all_threads = 1, thread_id, ip4 = 0, ip6 = 0, af_set = 0, detail = 0;
    uint32_t i;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat(input, "thread %u", &thread_id)) {
            if (thread_id == 0 || thread_id >= fm->no_threads) {
                vlib_cli_output(vm, "ERROR: Invalid thread-id, "
                                "valid range - 1 - %u", fm->no_threads - 1);
                goto done;
            }
            if (detail) {
                vlib_cli_output(vm, "ERROR: Option \"detail\" can't be used with option \"thread\"");
                goto done;
            }
            all_threads = 0;
        } else if (unformat(input, "detail")) {
            if (!all_threads) {
                vlib_cli_output(vm, "ERROR: Option \"detail\" can't be used with option \"thread\"");
                goto done;
            }
            detail = 1;
        } else if (unformat(input, "ip4")) {
            ip4 = af_set = 1;
        } else if (unformat(input, "ip6")) {
            ip6 = af_set = 1;
        } else {
            vlib_cli_output(vm, "ERROR: Invalid command");
            goto done;
        }
    }

    if (!af_set) {
        ip4 = ip6 = 1;
    }

    if (vlib_get_suspend_resume_worker_threads()) {
        vlib_cli_output(vm, "Worker threads are suspended!");
        goto done;
    }

    buf = calloc(1, DISPLAY_BUF_SIZE);
    if (!buf) {
        vlib_cli_output(vm, "ERROR: Failed to allocate  display buffer!");
        goto done;
    }

    if (detail || !all_threads) {
        if (ip4) {
            vlib_cli_output(vm, "IPv4 flow statistics\n");
            if (detail) {
                vlib_cli_output(vm, "Total number of IPv4 flow entries in hardware %u",
                                ftlv4_get_flow_count(fm->table4,
                                                     vm->thread_index));
            }
            for (i = 1; i < no_of_threads; i++) {
                if ((!all_threads) && (thread_id != i)) {
                    continue;
                }
                vlib_worker_thread_t *w = vlib_worker_threads + i;
                clib_callback_enable_disable
                    (vlib_mains[i]->worker_thread_main_loop_callbacks,
                     vlib_mains[i]->worker_thread_main_loop_callback_tmp,
                     vlib_mains[i]->worker_thread_main_loop_callback_lock,
                     (void *) get_worker_flow_stats_ftlv4, 1 /* enable */ );
                while (clib_callback_is_set
                        (vlib_mains[i]->worker_thread_main_loop_callbacks,
                         vlib_mains[i]->worker_thread_main_loop_callback_lock,
                         (void *) get_worker_flow_stats_ftlv4)) {
                    vlib_cli_output(vm, "Getting stats for worker %d", i);
                    usleep(100000);
                }
                vlib_cli_output(vm, "---------------------\n");
                if (w->cpu_id > -1) {
                    vlib_cli_output (vm, "Thread %d %s (lcore %u)\n", i, w->name,
                                     w->cpu_id);
                } else {
                    vlib_cli_output (vm, "Thread %d %s\n", i, w->name);
                }
                vlib_cli_output(vm, "---------------------\n");
                vlib_cli_output(vm, "%s", fm->stats_buf);
            }
        }
        if (ip6) {
            vlib_cli_output(vm, "\nIPv6 flow statistics\n");
            if (detail) {
                vlib_cli_output(vm, "Total number of IPv6 flow entries in hardware %u",
                                ftlv6_get_flow_count(fm->table6_or_l2));
            }
            for (i = 1; i < no_of_threads; i++) {
                if ((!all_threads) && (thread_id != i)) {
                    continue;
                }
                vlib_worker_thread_t *w = vlib_worker_threads + i;
                clib_callback_enable_disable
                    (vlib_mains[i]->worker_thread_main_loop_callbacks,
                     vlib_mains[i]->worker_thread_main_loop_callback_tmp,
                     vlib_mains[i]->worker_thread_main_loop_callback_lock,
                     (void *) get_worker_flow_stats_ftlv6, 1 /* enable */ );
                while (clib_callback_is_set
                        (vlib_mains[i]->worker_thread_main_loop_callbacks,
                         vlib_mains[i]->worker_thread_main_loop_callback_lock,
                         (void *) get_worker_flow_stats_ftlv6)) {
                    vlib_cli_output(vm, "Getting stats for worker %d", i);
                    usleep(100000);
                }
                vlib_cli_output(vm, "---------------------\n");
                if (w->cpu_id > -1) {
                    vlib_cli_output (vm, "Thread %d %s (lcore %u)\n", i, w->name,
                            w->cpu_id);
                } else {
                    vlib_cli_output (vm, "Thread %d %s\n", i, w->name);
                }
                vlib_cli_output(vm, "---------------------\n");
                vlib_cli_output(vm, "%s", fm->stats_buf);
            }
        }
        goto done;
    }

    /* Summary case */
    if (ip4) {
        vlib_cli_output(vm, "\nIPv4 flow statistics summary:\n");
        vlib_cli_output(vm, "Total number of IPv4 flow entries in hardware %u",
                        ftlv4_get_flow_count(fm->table4, vm->thread_index));
        ftlv4_init_stats_cache();
        for (i = 1; i < no_of_threads; i++) {
            clib_callback_enable_disable
                (vlib_mains[i]->worker_thread_main_loop_callbacks,
                 vlib_mains[i]->worker_thread_main_loop_callback_tmp,
                 vlib_mains[i]->worker_thread_main_loop_callback_lock,
                 (void *) get_worker_flow_stats_summary_ftlv4, 1 /* enable */ );
            while (clib_callback_is_set
                    (vlib_mains[i]->worker_thread_main_loop_callbacks,
                     vlib_mains[i]->worker_thread_main_loop_callback_lock,
                     (void *) get_worker_flow_stats_summary_ftlv4)) {
                vlib_cli_output(vm, "Getting stats for worker %d", i);
                usleep(100000);
            }
        }
        ftlv4_dump_stats_cache(buf, DISPLAY_BUF_SIZE - 1);
        vlib_cli_output(vm, "%s", buf);
    }
    if (ip6) {
        vlib_cli_output(vm, "\nIPv6 flow statistics summary:\n");
        vlib_cli_output(vm, "Total number of IPv6 flow entries in hardware %u",
                        ftlv6_get_flow_count(fm->table6_or_l2));
        ftlv6_init_stats_cache();
        for (i = 1; i < no_of_threads; i++) {
            clib_callback_enable_disable
                (vlib_mains[i]->worker_thread_main_loop_callbacks,
                 vlib_mains[i]->worker_thread_main_loop_callback_tmp,
                 vlib_mains[i]->worker_thread_main_loop_callback_lock,
                 (void *) get_worker_flow_stats_summary_ftlv6, 1 /* enable */ );
            while (clib_callback_is_set
                    (vlib_mains[i]->worker_thread_main_loop_callbacks,
                     vlib_mains[i]->worker_thread_main_loop_callback_lock,
                     (void *) get_worker_flow_stats_summary_ftlv6)) {
                vlib_cli_output(vm, "Getting stats for worker %d", i);
                usleep(100000);
            }
        }
        ftlv6_dump_stats_cache(buf, DISPLAY_BUF_SIZE - 1);
        vlib_cli_output(vm, "%s", buf);
    }

done:
    if (buf) {
        free(buf);
    }
    return 0;
}

VLIB_CLI_COMMAND (show_flow_stats_command, static) =
{
    .path = "show flow statistics",
    .short_help = "show flow statistics [detail] [thread <thread-id>] [ip4 | ip6]",
    .function = show_flow_stats_command_fn,
    .is_mp_safe = 1,
};

static clib_error_t *
show_pds_flow_replication_stats_command_fn (vlib_main_t * vm,
                                            unformat_input_t * input,
                                            vlib_cli_command_t * cmd)
{
    pds_flow_main_t *fm = &pds_flow_main;
    bool sync_set = true, restore_set = true;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat(input, "sync")) {
            restore_set = false;
        } else if (unformat (input, "restore")) {
            sync_set = false;
        } else {
            vlib_cli_output(vm, "ERROR: Invalid command.\n");
            return 0;
        }
    }
    vlib_cli_output(vm, "\nIPv4 flow replication statistics:\n");
    vlib_cli_output(vm, "  Total number of IPv4 flow entries in hardware %u",
                    ftlv4_get_flow_count(fm->table4, vm->thread_index));
    if (sync_set) {
        vlib_cli_output(vm, "  Session sync statistics:");
        vlib_cli_output(vm, "%-40s: %d", "    Synced",
                        fm->repl_stats.sync_success);
    }

    if (restore_set) {
        vlib_cli_output(vm, "  Session restore statistics:");
        vlib_cli_output(vm, "%-40s: %d", "    Restored",
                        fm->repl_stats.restore_success);
        vlib_cli_output(vm, "%-40s: %d", "    Decode failure",
                        fm->repl_stats.restore_failure_decode);
        vlib_cli_output(vm, "%-40s: %d", "    Unknown flow_state",
                        fm->repl_stats.restore_failure_unknown_flow_state);
        vlib_cli_output(vm, "%-40s: %d", "    Unknown flow_pkt_type",
                        fm->repl_stats.restore_failure_unknown_flow_type);
        vlib_cli_output(vm, "%-40s: %d", "    Unknown src vnic",
                        fm->repl_stats.restore_failure_unknown_src_vnic);
        vlib_cli_output(vm, "%-40s: %d", "    Unknown dst vnic",
                        fm->repl_stats.restore_failure_unknown_dst_vnic);
        vlib_cli_output(vm, "%-40s: %d", "    Vnic session limit exceeded",
                        fm->repl_stats.restore_failure_vnic_limit_exceeded);
        vlib_cli_output(vm, "%-40s: %d", "    Flow insert failed",
                        fm->repl_stats.restore_failure_flow_insert);
    }
    return 0;
}

VLIB_CLI_COMMAND (show_pds_flow_replication_stats_command, static) =
{
    .path = "show pds flow replication statistics",
    .short_help = "show pds flow replication statistics [sync|restore]",
    .function = show_pds_flow_replication_stats_command_fn,
};

static clib_error_t *
clear_pds_flow_replication_stats_command_fn (vlib_main_t * vm,
                                             unformat_input_t * input,
                                             vlib_cli_command_t * cmd)
{
    pds_flow_main_t *fm = &pds_flow_main;

    clib_memset(&fm->repl_stats, 0, sizeof(pds_flow_repl_stats_t));
    return 0;
}

VLIB_CLI_COMMAND (clear_pds_flow_replication_stats_command, static) =
{
    .path = "clear pds flow replication statistics",
    .short_help = "clear pds flow replication statistics",
    .function = clear_pds_flow_replication_stats_command_fn,
};

static clib_error_t *
dump_flow_entry_command_fn (vlib_main_t * vm,
                            unformat_input_t * input,
                            vlib_cli_command_t * cmd)
{
    pds_flow_main_t *fm = &pds_flow_main;
    int ret;
    ip46_address_t src = ip46_address_initializer,
                   dst = ip46_address_initializer;
    u32 sport = 0, dport = 0, lkp_id = 0;
    u8 ip_proto,
       src_set = 0,
       dst_set = 0,
       src_port_set = 0,
       dst_port_set = 0,
       proto_set = 0,
       lkp_id_set = 0,
       ip4 = 0;
    char *buf = NULL;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat (input, "source-ip %U", unformat_ip46_address, &src)) {
            src_set = 1;
        } else if (unformat (input, "destination-ip %U",
                             unformat_ip46_address, &dst)) {
            dst_set = 1;
        } else if (unformat(input, "ip-protocol %U",
                            unformat_ip_protocol, &ip_proto)) {
            proto_set = 1;
        } else if (unformat (input, "source-port %u", &sport)) {
            src_port_set = 1;
        } else if (unformat (input, "destination-port %u", &dport)) {
            dst_port_set = 1;
        } else if (unformat (input, "lookup-id %u", &lkp_id)) {
            lkp_id_set = 1;
        } else {
            vlib_cli_output(vm, "ERROR: Invalid command.\n");
            goto done;
        }
    }

    if (!src_set || !dst_set || !proto_set || !lkp_id_set) {
        vlib_cli_output(vm, "ERROR: Invalid input.\n");
        goto done;
    }

    if (((ip_proto == IP_PROTOCOL_TCP) || (ip_proto == IP_PROTOCOL_UDP)) &&
        (!src_port_set || ! dst_port_set || (sport > 0xffff) || (dport > 0xffff))) {
        vlib_cli_output(vm, "ERROR: Invalid source/destination ports.\n");
        goto done;
    }

    ip4 = ip46_address_is_ip4(&src);
    if (ip4 != ip46_address_is_ip4(&dst)) {
        vlib_cli_output(vm, "ERROR: Source and Destination IP address "
                        "belong to different address-family.\n");
        goto done;
    }

    buf = calloc(1, DISPLAY_BUF_SIZE);
    if (!buf) {
        vlib_cli_output(vm, "ERROR: Failed to allocate  display buffer!");
        goto done;
    }

    vlib_worker_thread_barrier_sync(vm);
    if (ip4) {
        ret = ftlv4_dump_hw_entry(fm->table4,
                                  clib_net_to_host_u32(src.ip4.as_u32),
                                  clib_net_to_host_u32(dst.ip4.as_u32),
                                  ip_proto,
                                  (u16)sport,
                                  (u16)dport,
                                  (u16)lkp_id,
                                  buf, DISPLAY_BUF_SIZE-1,
                                  vm->thread_index);
    } else {
        ret = ftlv6_dump_hw_entry(fm->table6_or_l2,
                                  src.ip6.as_u8,
                                  dst.ip6.as_u8,
                                  ip_proto,
                                  (u16)sport,
                                  (u16)dport,
                                  (u16)lkp_id,
                                  buf, DISPLAY_BUF_SIZE-1);
    }
    vlib_worker_thread_barrier_release(vm);
    if (ret < 0) {
        vlib_cli_output(vm, "Entry not found.\n");
    } else {
        vlib_cli_output(vm, "Entry found:\n%s", buf);
    }

done:
    free(buf);
    return 0;
}

VLIB_CLI_COMMAND (dump_flow_entry_command, static) =
{
    .path = "dump flow entry",
    .short_help = "dump flow entry "
                  "source-ip <ip-address> destination-ip <ip-address> "
                  "ip-protocol <TCP|UDP|ICMP> "
                  "[source-port <UDP/TCP port number>] "
                  "[destination-port <UDP/TCP port number>] "
                  "lookup-id <number>",
    .function = dump_flow_entry_command_fn,
    .is_mp_safe = 1,
};

static clib_error_t *
dump_flow_entries_command_fn (vlib_main_t * vm,
                              unformat_input_t * input,
                              vlib_cli_command_t * cmd)
{
    char *logfile = NULL;
    pds_flow_main_t *fm = &pds_flow_main;
    int ret;
    u8 detail = 0;
    u32 ip4 = 0, ip6 = 0, af_set = 0;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat (input, "file %s", &logfile))
            ;
        else if (unformat (input, "detail")) {
            detail = 1;
        } else if (unformat(input, "ip4")) {
            ip4 = af_set = 1;
        } else if (unformat(input, "ip6")) {
            ip6 = af_set = 1;
        } else {
            vlib_cli_output(vm, "ERROR: Invalid command\n");
            goto done;
        }
    }

    if (logfile == NULL) {
        vlib_cli_output(vm, "ERROR: Invalid filename\n");
        goto done;
    }

    if (!af_set) {
        ip4 = ip6 = 1;
    }

    if (ip4) {
        vlib_cli_output(vm, "Reading IPv4 flow entries from HW, Please wait...\n");
        vlib_worker_thread_barrier_sync(vm);
        ret = ftlv4_dump_hw_entries(fm->table4, logfile, detail,
                                    vm->thread_index);
        vlib_worker_thread_barrier_release(vm);
        if (ret < 0) {
            vlib_cli_output(vm, "Error writing IPv4 to %s\n", logfile);
        } else {
            vlib_cli_output(vm, "Written %d valid IPv4 entries to %s successfully.\n",
                    ret, logfile);
        }
    }
    if (ip6) {
        vlib_cli_output(vm, "Reading IPv6 flow entries from HW, Please wait...\n");
        vlib_worker_thread_barrier_sync(vm);
        ret = ftlv6_dump_hw_entries(fm->table6_or_l2, logfile, detail);
        vlib_worker_thread_barrier_release(vm);
        if (ret < 0) {
            vlib_cli_output(vm, "Error writing IPv6 to %s\n", logfile);
        } else {
            vlib_cli_output(vm, "Written %d valid IPv6 entries to %s successfully.\n",
                    ret, logfile);
        }
    }

done:
    vec_free(logfile);
    return 0;
}

VLIB_CLI_COMMAND (dump_flow_entries_command, static) =
{
    .path = "dump flow entries",
    .short_help = "dump flow entries file "
                  "<absolute file path to dump hw entries> [detail] [ip4 | ip6]",
    .function = dump_flow_entries_command_fn,
    .is_mp_safe = 1,
};

static clib_error_t *
clear_flow_entries_command_fn (vlib_main_t * vm,
                               unformat_input_t * input,
                               vlib_cli_command_t * cmd)
{
    int ret = 0;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        vlib_cli_output(vm, "ERROR: Invalid command");
        goto done;
    }

    ret = clear_all_flow_entries();

    if (ret) {
        vlib_cli_output(vm, "ERROR: Failed to clear all flows");
    } else {
        vlib_cli_output(vm, "Successfully cleared all flows");
    }

done:
    return 0;
}

VLIB_CLI_COMMAND (clear_flow_entries_command, static) =
{
    .path = "clear flow entries",
    .short_help = "clear flow entries",
    .function = clear_flow_entries_command_fn,
    .is_mp_safe = 1,
};

static void
clear_worker_flow_stats_ftlv4(vlib_main_t *vm)
{
    pds_flow_main_t *fm = &pds_flow_main;
    (void)ftlv4_clear(fm->table4, false, true, vm->thread_index);
    clib_callback_enable_disable
        (vm->worker_thread_main_loop_callbacks,
         vm->worker_thread_main_loop_callback_tmp,
         vm->worker_thread_main_loop_callback_lock,
         (void *) clear_worker_flow_stats_ftlv4, 0 /* disable */ );
}

static void
clear_worker_flow_stats_ftlv6(vlib_main_t *vm)
{
    pds_flow_main_t *fm = &pds_flow_main;
    (void)ftlv6_clear(fm->table6_or_l2, false, true);
    clib_callback_enable_disable
        (vm->worker_thread_main_loop_callbacks,
         vm->worker_thread_main_loop_callback_tmp,
         vm->worker_thread_main_loop_callback_lock,
         (void *) clear_worker_flow_stats_ftlv6, 0 /* disable */ );
}

static clib_error_t *
clear_flow_stats_command_fn (vlib_main_t * vm,
                             unformat_input_t * input,
                             vlib_cli_command_t * cmd)
{
    pds_flow_main_t *fm = &pds_flow_main;
    u32 all_threads = 1, thread_id, ip4 = 0, ip6 = 0, af_set = 0;
    uint32_t i = 0;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat(input, "thread %u", &thread_id)) {
            if ((thread_id == 0) || (thread_id >= fm->no_threads)) {
                vlib_cli_output(vm, "ERROR: Invalid thread-id, "
                                "valid range - 1 - %u", fm->no_threads - 1);
                goto done;
            }
            all_threads = 0;
        } else if (unformat(input, "ip4")) {
            ip4 = af_set = 1;
        } else if (unformat(input, "ip6")) {
            ip6 = af_set = 1;
        } else {
            vlib_cli_output(vm, "ERROR: Invalid command");
            goto done;
        }
    }

    if (!af_set) {
        ip4 = ip6 = 1;
    }

    if (vlib_get_suspend_resume_worker_threads()) {
        vlib_cli_output(vm, "Worker threads are suspended!");
        goto done;
    }

    if (all_threads) {
        for (i = 1; i < fm->no_threads; i++) {
            if (ip4) {
                vlib_cli_output(vm, "Clearing IPv4 flow statistcs for thread[%d]", i);
                clib_callback_enable_disable
                    (vlib_mains[i]->worker_thread_main_loop_callbacks,
                     vlib_mains[i]->worker_thread_main_loop_callback_tmp,
                     vlib_mains[i]->worker_thread_main_loop_callback_lock,
                     (void *) clear_worker_flow_stats_ftlv4, 1 /* enable */ );
                while (clib_callback_is_set
                        (vlib_mains[i]->worker_thread_main_loop_callbacks,
                         vlib_mains[i]->worker_thread_main_loop_callback_lock,
                         (void *) clear_worker_flow_stats_ftlv4)) {
                    vlib_cli_output(vm, "Clearing stats for worker %d", i);
                    usleep(100000);
                }
            }
            if (ip6) {
                vlib_cli_output(vm, "Clearing IPv6 flow statistcs for thread[%d]", i);
                clib_callback_enable_disable
                    (vlib_mains[i]->worker_thread_main_loop_callbacks,
                     vlib_mains[i]->worker_thread_main_loop_callback_tmp,
                     vlib_mains[i]->worker_thread_main_loop_callback_lock,
                     (void *) clear_worker_flow_stats_ftlv6, 1 /* enable */ );
                while (clib_callback_is_set
                        (vlib_mains[i]->worker_thread_main_loop_callbacks,
                         vlib_mains[i]->worker_thread_main_loop_callback_lock,
                         (void *) clear_worker_flow_stats_ftlv6)) {
                    vlib_cli_output(vm, "Clearing stats for worker %d", i);
                    usleep(100000);
                }
            }
        }
    } else {
        if (ip4) {
            vlib_cli_output(vm, "Clearing IPv4 flow statistcs for thread[%d]", thread_id);
            clib_callback_enable_disable
                (vlib_mains[thread_id]->worker_thread_main_loop_callbacks,
                 vlib_mains[thread_id]->worker_thread_main_loop_callback_tmp,
                 vlib_mains[thread_id]->worker_thread_main_loop_callback_lock,
                 (void *) clear_worker_flow_stats_ftlv4, 1 /* enable */ );
            while (clib_callback_is_set
                    (vlib_mains[thread_id]->worker_thread_main_loop_callbacks,
                     vlib_mains[thread_id]->worker_thread_main_loop_callback_lock,
                     (void *) clear_worker_flow_stats_ftlv4)) {
                vlib_cli_output(vm, "Clearing stats for worker %d", i);
                usleep(100000);
            }
        }
        if (ip6) {
            vlib_cli_output(vm, "Clearing IPv6 flow statistcs for thread[%d]", thread_id);
            clib_callback_enable_disable
                (vlib_mains[thread_id]->worker_thread_main_loop_callbacks,
                 vlib_mains[thread_id]->worker_thread_main_loop_callback_tmp,
                 vlib_mains[thread_id]->worker_thread_main_loop_callback_lock,
                 (void *) clear_worker_flow_stats_ftlv6, 1 /* enable */ );
            while (clib_callback_is_set
                    (vlib_mains[thread_id]->worker_thread_main_loop_callbacks,
                     vlib_mains[thread_id]->worker_thread_main_loop_callback_lock,
                     (void *) clear_worker_flow_stats_ftlv6)) {
                vlib_cli_output(vm, "Clearing stats for worker %d", i);
                usleep(100000);
            }
        }
    }
    vlib_cli_output(vm, "Successfully cleared flow statistics");

done:
    return 0;
}

VLIB_CLI_COMMAND (clear_flow_stats_command, static) =
{
    .path = "clear flow statistics",
    .short_help = "clear flow statistics [thread <thread-id>] [ip4 | ip6]",
    .function = clear_flow_stats_command_fn,
    .is_mp_safe = 1,
};

static clib_error_t *
show_flow_session_info_command_fn (vlib_main_t * vm,
                                   unformat_input_t * input,
                                   vlib_cli_command_t * cmd)
{
    u32 index = ~0;
    u8 *entry = NULL;
    u32 size = 0;
    pds_flow_main_t *fm = &pds_flow_main;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat(input, "index %u", &index)) {

        } else {
            vlib_cli_output(vm, "ERROR: Invalid command\n");
            goto done;
        }
    }

    if ((fm->max_sessions < index) || (0 == index)) {
        vlib_cli_output(vm, "Invalid session index, Valid range 0-%u",
                        fm->max_sessions);
        goto done;
    }
    session_get_addr(index, &entry, &size);
    vlib_cli_output(vm, "Session Id[%u]\n Session data:\n");
    vlib_cli_output(vm, "%U", format_hex_bytes, entry, size);

done:
    return 0;
}

VLIB_CLI_COMMAND (show_flow_session_command, static) =
{
    .path = "show flow session-info",
    .short_help = "show flow session-info [<index <id>>]",
    .function = show_flow_session_info_command_fn,
};

void
pds_flow_session_info_show (vlib_main_t *vm, u32 ses_id, u8 detail)
{
    pds_flow_main_t *fm = &pds_flow_main;
    session_info_t session_info = { 0 };
    session_track_info_t session_track_info = { 0 };
    v4_flow_info_t flow_info = { 0 };
    pds_flow_hw_ctx_t *session;
    struct in_addr src, dst;
    char srcstr[INET_ADDRSTRLEN + 1], dststr[INET_ADDRSTRLEN + 1];
    char handle_str[255] = {0};
    char delimiter[130] = "";
    int ret = 0;

    pds_session_get_info(ses_id, &session_info);
    pds_session_track_get_info(ses_id, &session_track_info);
    session = pds_flow_get_session(ses_id);
    if (!session) {
        vlib_cli_output(vm, "Session does not exist");
        return;
    }

    vlib_cli_output(vm, "Session Info");
    vlib_cli_output(vm, "------------");
    vlib_cli_output(vm, "  Transmit Info");
    vlib_cli_output(vm, "    %-19s: 0x%x", "Rewrite Flags",
                    session_info.tx_rewrite_flags);
    vlib_cli_output(vm, "    %-19s: %d", "NAT ID 1",
                    session_info.tx_xlate_id);
    vlib_cli_output(vm, "    %-19s: %d", "NAT ID 2",
                    session_info.tx_xlate_id2);
    vlib_cli_output(vm, "  Receive Info");
    vlib_cli_output(vm, "    %-19s: 0x%x", "Rewrite Flags",
                    session_info.rx_rewrite_flags);
    vlib_cli_output(vm, "    %-19s: %d", "NAT ID 1",
                    session_info.rx_xlate_id);
    vlib_cli_output(vm, "    %-19s: %d", "NAT ID 2",
                    session_info.rx_xlate_id2 );
    vlib_cli_output(vm, "  %-21s: %d", "Meter ID",
                    session_info.meter_id);
    vlib_cli_output(vm, "  %-21s: %lu", "Last seen timestamp",
                    pds_session_get_timestamp(ses_id));
    vlib_cli_output(vm, "  %-21s: %lu", "Current timestamp",
                    pds_system_get_current_tick());
    vlib_cli_output(vm, "  %-21s: %s", "Session tracking",
                    session_info.session_tracking_en ? "Enabled" : "Disabled");
    vlib_cli_output(vm, "  %-21s: %s", "Session action",
                    session_info.drop ? "Drop" : "Allow");
    if (session_info.qid_en) {
        vlib_cli_output(vm, "  %-21s: %u", "Session qid",
                        session_info.qid);
    } else {
        vlib_cli_output(vm, "  %-21s: %s", "Session qid",
                        "Disabled");
    }
    vlib_cli_output(vm, "  %-21s: %s", "Session state",
                    pds_flow_state_str[session->flow_state]);
    vlib_cli_output(vm, "  %-21s: %d", "Keepalive retries",
                    session->keep_alive_retry);
    vlib_cli_output(vm, "  %-21s: %d", "Ingress Subnet ID",
                    session->ingress_bd);
    vlib_cli_output(vm, "  %-21s: %lu", "Timer Handle", session->timer_hdl);
    ftlv4_get_handle_str(handle_str, session->iflow.handle);
    vlib_cli_output(vm, "  %-21s: %s", "IFlow handle", handle_str);
    ftlv4_get_handle_str(handle_str, session->rflow.handle);
    vlib_cli_output(vm, "  %-21s: %s", "RFlow handle", handle_str);
    vlib_cli_output(vm, "  %-21s: %s", "Packet type",
                    pds_flow_pkt_type_str[session->packet_type]);
    vlib_cli_output(vm, "  %-21s: %s", "Flags",
                    session->iflow_rx ? "IFLOW_RX_PACKET" : "None" );
    vlib_cli_output(vm, "  %-21s: %d\n", "Src Vnic ID", session->src_vnic_id);
    if (pds_flow_packet_l2l(session->packet_type)) {
        vlib_cli_output(vm, "  %-21s: %d\n", "Dst Vnic ID",
                        session->dst_vnic_id);
    }

    for (int i = 0; i < 130; i++) {
        strcat(delimiter, "-");
    }
    vlib_cli_output(vm, "%s", delimiter);
    vlib_cli_output(vm, "%-6s%-40s%-40s%-8s%-8s%-8s%-5s%-11s%-5s",
                    "BDId", "SrcAddr", "DstAddr", "SrcPort",
                    "DstPort", "IPProto", "Role", "SessionIdx", "Epoch");
    if (detail) {
        vlib_cli_output(vm, "%-4s%-10s%-10s%-6s%-10s", "", "NhValid", "NhType",
                        "NhId", "NhPriority");
    }
    vlib_cli_output(vm, "%s", delimiter);
    if (session->v4) {
        ret = ftlv4_dump_entry_with_handle(fm->table4, session->iflow.handle,
                                           &flow_info, vm->thread_index);
        if (ret < 0) {
            vlib_cli_output(vm, "Iflow entry not found.");
        } else {
            src.s_addr = clib_net_to_host_u32(flow_info.key_metadata_ipv4_src);
            dst.s_addr = clib_net_to_host_u32(flow_info.key_metadata_ipv4_dst);
            inet_ntop(AF_INET, &src, srcstr, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &dst, dststr, INET_ADDRSTRLEN);
            vlib_cli_output(vm, "%-6d%-40s%-40s%-8d%-8d%-8U%-5s%-11d%-5d",
                            flow_info.key_metadata_flow_lkp_id,
                            srcstr,
                            dststr,
                            flow_info.key_metadata_sport,
                            flow_info.key_metadata_dport,
                            format_ip_protocol,
                            flow_info.key_metadata_proto,
                            "I",
                            flow_info.session_index,
                            flow_info.epoch);
            if (detail) {
                vlib_cli_output(vm, "%-4s%-10d%-10d%-6d%-10d",
                                "",
                                flow_info.nexthop_valid,
                                flow_info.nexthop_type,
                                flow_info.nexthop_id,
                                flow_info.nexthop_priority);
            }
        }

        ret = ftlv4_dump_entry_with_handle(fm->table4, session->rflow.handle,
                                           &flow_info, vm->thread_index);
        if (ret < 0) {
            vlib_cli_output(vm, "Rflow entry not found.");
        } else {
            src.s_addr = clib_net_to_host_u32(flow_info.key_metadata_ipv4_src);
            dst.s_addr = clib_net_to_host_u32(flow_info.key_metadata_ipv4_dst);
            inet_ntop(AF_INET, &src, srcstr, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &dst, dststr, INET_ADDRSTRLEN);
            vlib_cli_output(vm, "%-6d%-40s%-40s%-8d%-8d%-8U%-5s%-11d%-5d",
                            flow_info.key_metadata_flow_lkp_id,
                            srcstr,
                            dststr,
                            flow_info.key_metadata_sport,
                            flow_info.key_metadata_dport,
                            format_ip_protocol,
                            flow_info.key_metadata_proto,
                            "R",
                            flow_info.session_index,
                            flow_info.epoch);
            if (detail) {
                vlib_cli_output(vm, "%-4s%-10d%-10d%-6d%-10d",
                                "",
                                flow_info.nexthop_valid,
                                flow_info.nexthop_type,
                                flow_info.nexthop_id,
                                flow_info.nexthop_priority);
            }
        }
    }

    if (session_info.session_tracking_en && detail) {
        vlib_cli_output(vm, "Session tracking info");
        vlib_cli_output(vm, "%s", delimiter);
        vlib_cli_output(vm, "%-10s%-20s%-20s%-20s%-10s%-10s%-6s%-12s%-15s",
                        "FlowRole", "FlowState", "SeqNo", "AckNo", "WinSize",
                        "WinScale", "MSS", "Exceptions", "WinScaleSent");
        vlib_cli_output(vm, "%s", delimiter);
        vlib_cli_output(vm, "%-10s%-20s%-20lu%-20lu%-10d%-10d%-6d%-12d%-15d", "I",
                        flow_state_str[session_track_info.iflow_tcp_state],
                        session_track_info.iflow_tcp_seq_num,
                        session_track_info.iflow_tcp_ack_num,
                        session_track_info.iflow_tcp_win_size,
                        session_track_info.iflow_tcp_win_scale,
                        session_track_info.iflow_tcp_mss,
                        session_track_info.iflow_tcp_exceptions,
                        session_track_info.iflow_tcp_win_scale_option_sent);
        vlib_cli_output(vm, "%-10s%-20s%-20lu%-20lu%-10d%-10d%-6d%-12d%-15s", "R",
                        flow_state_str[session_track_info.rflow_tcp_state],
                        session_track_info.rflow_tcp_seq_num,
                        session_track_info.rflow_tcp_ack_num,
                        session_track_info.rflow_tcp_win_size,
                        session_track_info.rflow_tcp_win_scale,
                        session_track_info.rflow_tcp_mss,
                        session_track_info.rflow_tcp_exceptions,
                        "-");
    }
}

static clib_error_t *
show_pds_flow_session_info_command_fn (vlib_main_t * vm,
                                       unformat_input_t * input,
                                       vlib_cli_command_t * cmd)
{
    pds_flow_main_t *fm = &pds_flow_main;
    int ret = 0;
    u32 session_index = ~0;
    u8 session_index_set = 0, detail = 0;
    ip46_address_t src = ip46_address_initializer,
                   dst = ip46_address_initializer;
    u32 sport = 0, dport = 0, bd_id = 0;
    u8 ip_proto,
       src_set = 0,
       dst_set = 0,
       src_port_set = 0,
       dst_port_set = 0,
       proto_set = 0,
       lkp_id_set = 0,
       ip4 = 0,
       timer_wheel = 0;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat(input, "session %u", &session_index)) {
            session_index_set = 1;
        } else if (unformat(input, "source-ip %U", unformat_ip46_address, &src)) {
            src_set = 1;
        } else if (unformat(input, "destination-ip %U",
                            unformat_ip46_address, &dst)) {
            dst_set = 1;
        } else if (unformat(input, "ip-protocol %U",
                            unformat_ip_protocol, &ip_proto)) {
            proto_set = 1;
        } else if (unformat(input, "source-port %u", &sport)) {
            src_port_set = 1;
        } else if (unformat(input, "destination-port %u", &dport)) {
            dst_port_set = 1;
        } else if (unformat(input, "bd-id %u", &bd_id)) {
            lkp_id_set = 1;
        } else if (unformat(input, "detail")) {
            detail = 1;
        } else if (unformat(input, "timer-wheel")) {
            timer_wheel = 1;
        } else {
            vlib_cli_output(vm, "ERROR: Invalid command.\n");
            return 0;
        }
    }

    if (!session_index_set &&
        (!src_set || !dst_set || !proto_set || !lkp_id_set) &&
        !timer_wheel) {
        vlib_cli_output(vm, "ERROR: Invalid input.\n");
        return 0;
    }

    if (timer_wheel) {
        for (int i = 1; i < fm->no_threads; i++) {
            vlib_cli_output(vm, "Timer wheel for worker thread %d :", i);
            vlib_cli_output(vm, "%U", format_tw_timer_wheel_16t_2w_4096sl,
                            &fm->timer_wheel[i]);
        }
        return 0;
    }

    if (session_index_set) {
        if ((fm->max_sessions < session_index) || (0 == session_index)) {
            vlib_cli_output(vm, "Invalid session index, Valid range 0-%u",
                            fm->max_sessions);
            return 0;
        } else if (pool_is_free_index(fm->session_index_pool, session_index - 1)) {
            vlib_cli_output(vm, "Session does not exist");
            return 0;
        }
    } else {
        if (((ip_proto == IP_PROTOCOL_TCP) || (ip_proto == IP_PROTOCOL_UDP)) &&
            (!src_port_set || ! dst_port_set || (sport > 0xffff) || (dport > 0xffff))) {
            vlib_cli_output(vm, "ERROR: Invalid source/destination ports.\n");
            return 0;
        }

        ip4 = ip46_address_is_ip4(&src);
        if (ip4 != ip46_address_is_ip4(&dst)) {
            vlib_cli_output(vm, "ERROR: Source and Destination IP address "
                            "belong to different address-family.\n");
            return 0;
        }

        // Retrieve session index based on 6 tuple
        if (ip4) {
            ret = ftlv4_read_session_index(fm->table4,
                                           clib_net_to_host_u32(src.ip4.as_u32),
                                           clib_net_to_host_u32(dst.ip4.as_u32),
                                           ip_proto,
                                           (u16)sport,
                                           (u16)dport,
                                           (u16)bd_id,
                                           &session_index,
                                           vm->thread_index);
        } else {
            ret = ftlv6_read_session_index(fm->table6_or_l2,
                                           src.ip6.as_u8,
                                           dst.ip6.as_u8,
                                           ip_proto,
                                           (u16)sport,
                                           (u16)dport,
                                           (u16)bd_id,
                                           &session_index);
        }
        if (ret < 0) {
            vlib_cli_output(vm, "Entry not found\n");
            return 0;
        }
    }

    pds_flow_session_info_show(vm, session_index, detail);
    return 0;
}

VLIB_CLI_COMMAND (show_pds_flow_session_command, static) =
{
    .path = "show pds flow",
    .short_help = "show pds flow"
                  "{{source-ip <ip-address> destination-ip <ip-address> "
                  "ip-protocol <TCP|UDP|ICMP> "
                  "[source-port <UDP/TCP port number>] "
                  "[destination-port <UDP/TCP port number>] "
                  "bd-id <number>)} | session <session-index> | "
                  "timer-wheel} [detail]",
    .function = show_pds_flow_session_info_command_fn,
    .is_mp_safe = 1,
};

static clib_error_t *
set_vpp_pds_secprofile_command_fn (vlib_main_t *vm,
                                   unformat_input_t *input,
                                   vlib_cli_command_t *cmd)
{
    pds_flow_main_t *fm = &pds_flow_main;
    u32 tcp_idle_timeout, udp_idle_timeout, icmp_idle_timeout, other_idle_timeout;
    u32 tcp_syn_timeout, tcp_half_close_timeout, tcp_close_timeout;
    u32 tcp_drop_timeout, udp_drop_timeout, icmp_drop_timeout, other_drop_timeout;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat(input, "tcp-idle-timeout %u", &tcp_idle_timeout)) {
            fm->idle_timeout[PDS_FLOW_PROTO_TCP] = PDS_FLOW_SEC_TO_TIMER_TICK(tcp_idle_timeout);
        } else if (unformat(input, "udp-idle-timeout %u", &udp_idle_timeout)) {
            fm->idle_timeout[PDS_FLOW_PROTO_UDP] = PDS_FLOW_SEC_TO_TIMER_TICK(udp_idle_timeout);
        } else if (unformat(input, "icmp-idle-timeout %u", &icmp_idle_timeout)) {
            fm->idle_timeout[PDS_FLOW_PROTO_ICMP] = PDS_FLOW_SEC_TO_TIMER_TICK(icmp_idle_timeout);
        } else if (unformat(input, "other-idle-timeout %u", &other_idle_timeout)) {
            fm->idle_timeout[PDS_FLOW_PROTO_OTHER] = PDS_FLOW_SEC_TO_TIMER_TICK(other_idle_timeout);
        } else if (unformat(input, "tcp-syn-timeout %u", &tcp_syn_timeout)) {
            fm->tcp_con_setup_timeout = PDS_FLOW_SEC_TO_TIMER_TICK(tcp_syn_timeout);
        } else if (unformat(input, "tcp-half-close-timeout %u", &tcp_half_close_timeout)) {
            fm->tcp_half_close_timeout = PDS_FLOW_SEC_TO_TIMER_TICK(tcp_half_close_timeout);
        } else if (unformat(input, "tcp-close-timeout %u", &tcp_close_timeout)) {
            fm->tcp_close_timeout = PDS_FLOW_SEC_TO_TIMER_TICK(tcp_close_timeout);
        } else if (unformat(input, "tcp-drop-timeout %u", &tcp_drop_timeout)) {
            fm->drop_timeout[PDS_FLOW_PROTO_TCP] = PDS_FLOW_SEC_TO_TIMER_TICK(tcp_drop_timeout);
        } else if (unformat(input, "udp-drop-timeout %u", &udp_drop_timeout)) {
            fm->drop_timeout[PDS_FLOW_PROTO_UDP] = PDS_FLOW_SEC_TO_TIMER_TICK(udp_drop_timeout);
        } else if (unformat(input, "icmp-drop-timeout %u", &icmp_drop_timeout)) {
            fm->drop_timeout[PDS_FLOW_PROTO_ICMP] = PDS_FLOW_SEC_TO_TIMER_TICK(icmp_drop_timeout);
        } else if (unformat(input, "other-drop-timeout %u", &other_drop_timeout)) {
            fm->drop_timeout[PDS_FLOW_PROTO_OTHER] = PDS_FLOW_SEC_TO_TIMER_TICK(other_drop_timeout);
        } else {
            vlib_cli_output(vm, "ERROR: Invalid command");
            goto done;
        }
    }

done:
    return 0;
}

VLIB_CLI_COMMAND (set_vpp_pds_secprofile_command, static) =
{
    .path = "set pds security-profile",
    .short_help = "set pds security-profile [tcp-idle-timeout <timeout>] "
                  "[udp-idle-timeout <timeout>] [icmp-idle-timeout <timeout>] "
                  "[other-idle-timeout <timeout>] [tcp-syn-timeout <timeout>] "
                  "[tcp-half-close-timeout <timeout>] [tcp-close-timeout <timeout>] "
                  "[tcp-drop-timeout <timeout>] [udp-drop-timeout <timeout>] "
                  "[icmp-drop-timeout <timeout>] [other-drop-timeout <timeout>]",
    .function = set_vpp_pds_secprofile_command_fn,
};

static const char *
datapath_stat_names[PDS_DATAPATH_ASSIST_STAT_MAX] = {
#define _(s,n) n,
    foreach_datapath_assist_stats_counter
#undef _
};

static clib_error_t *
show_pds_datapath_stats (vlib_main_t *vm,
                         unformat_input_t *input,
                         vlib_cli_command_t *cmd)
{
    uint64_t counter[PDS_DATAPATH_ASSIST_STAT_MAX] = {0};
    int i;

    pds_flow_monitor_accumulate_stats((void *)vm, counter);
    for (i = 0; i < PDS_DATAPATH_ASSIST_STAT_MAX; i++) {
        vlib_cli_output(vm, "%-35s : %lu\n", datapath_stat_names[i], counter[i]);
    }
    return 0;
}

VLIB_CLI_COMMAND (show_pds_stats_datapath_assist, static) =
{
    .path = "show pds statistics datapath-assist",
    .short_help = "show pds statistics datapath-assist",
    .function = show_pds_datapath_stats,
};

