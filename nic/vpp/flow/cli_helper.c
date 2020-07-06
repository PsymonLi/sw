//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#include <vlib/vlib.h>
#include <vlib/unix/plugin.h>
#include <vnic.h>
#include "node.h"

static int (*clear_nat_flows)(void);

int clear_all_flow_entries()
{
    int ret1 = 0, ret2 = 0, ret3 = 0;
    pds_flow_main_t *fm = &pds_flow_main;
    vlib_main_t *vm = vlib_get_main();

    if (clear_nat_flows == NULL) {
        clear_nat_flows =
            vlib_get_plugin_symbol("nat.so", "nat_clear_all_flows");
    }

    vlib_worker_thread_barrier_sync(vm);
    if (clear_nat_flows) {
        ret1 = clear_nat_flows();
    }
    ret2 = ftlv4_clear(fm->table4, true, false, vm->thread_index);
    ret3 = ftlv6_clear(fm->table6_or_l2, true, false);
    pds_session_id_flush();
    pds_vnic_active_session_clear();
    clib_memset(&fm->stats, 0, sizeof(pds_flow_stats_t));
    vlib_worker_thread_barrier_release(vm);

    return ret1 && ret2 && ret3;
}

uint16_t get_vlib_thread_index()
{
    vlib_main_t *vm = vlib_get_main();
    return (vm->thread_index);
}
