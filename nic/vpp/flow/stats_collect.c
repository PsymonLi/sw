//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include <assert.h>
#include <system.h>
#include <vlib/vlib.h>
#include <vlib/threads.h>
#include "node.h"
#include "nic/vpp/dhcp_relay/node.h"
#include "nic/vpp/arp_proxy/node.h"
#include "stats.h"

static const char *
accumulate_node_names[] = {
#define _(n,s)  s,
    foreach_accumulate_node
#undef _
    NULL
};

static const datapath_assist_stats_accumulate_t
accumulate_data[] = {
    {
        PDS_DATAPATH_ASSIST_STAT_TOTALPKTSRX,
        PDS_FLOW_ACCUMULATE_NODE_DPDK_INPUT,
        true,
        {-1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_TOTALDROPS,
        PDS_FLOW_ACCUMULATE_NODE_DROP,
        true,
        {-1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_DHCPPKTSRX,
        PDS_FLOW_ACCUMULATE_NODE_DHCP_HOST,
        true,
        {-1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_DHCPPKTSTXTOPROXYSERVER,
        PDS_FLOW_ACCUMULATE_NODE_DHCP_HOST,
        false,
        {DHCP_RELAY_CLFY_COUNTER_TO_PROXY_SERVER, -1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_DHCPPKTSTXTORELAYSERVER,
        PDS_FLOW_ACCUMULATE_NODE_DHCP_HOST,
        false,
        {DHCP_RELAY_CLFY_COUNTER_TO_RELAY_SERVER, -1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_DHCPPKTSTXTORELAYCLIENT,
        PDS_FLOW_ACCUMULATE_NODE_DHCP_HOST,
        false,
        {DHCP_RELAY_CLFY_COUNTER_TO_RELAY_CLIENT, -1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_DHCPDROPS,
        PDS_FLOW_ACCUMULATE_NODE_DHCP_HOST,
        false,
        {DHCP_RELAY_CLFY_COUNTER_NO_VNIC, DHCP_RELAY_CLFY_COUNTER_NO_DHCP, -1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_DHCPPKTSTXTOPROXYSERVER,
        PDS_FLOW_ACCUMULATE_NODE_DHCP_UPLINK,
        false,
        {DHCP_RELAY_CLFY_COUNTER_TO_PROXY_SERVER, -1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_DHCPPKTSTXTORELAYSERVER,
        PDS_FLOW_ACCUMULATE_NODE_DHCP_UPLINK,
        false,
        {DHCP_RELAY_CLFY_COUNTER_TO_RELAY_SERVER, -1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_DHCPPKTSTXTORELAYCLIENT,
        PDS_FLOW_ACCUMULATE_NODE_DHCP_UPLINK,
        false,
        {DHCP_RELAY_CLFY_COUNTER_TO_RELAY_CLIENT, -1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_DHCPDROPS,
        PDS_FLOW_ACCUMULATE_NODE_DHCP_UPLINK,
        false,
        {DHCP_RELAY_CLFY_COUNTER_NO_VNIC, DHCP_RELAY_CLFY_COUNTER_NO_DHCP, -1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_ARPPKTSRX,
        PDS_FLOW_ACCUMULATE_NODE_ARP_PROXY,
        true,
        {-1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_ARPREPLIESSENT,
        PDS_FLOW_ACCUMULATE_NODE_ARP_PROXY,
        false,
        {ARP_PROXY_COUNTER_REPLY_SUCCESS, -1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_ARPDROPS,
        PDS_FLOW_ACCUMULATE_NODE_ARP_PROXY,
        false,
        {
            ARP_PROXY_COUNTER_SUBNET_CHECK_FAIL,
            ARP_PROXY_COUNTER_VNIC_MISSING,
            ARP_PROXY_COUNTER_NO_MAC,
            ARP_PROXY_COUNTER_NOT_ARP_REQUEST,
            ARP_PROXY_COUNTER_DAD_DROP,
            -1
        }
    },
    {
        PDS_DATAPATH_ASSIST_STAT_TOTALSESSIONSLEARNED,
        PDS_FLOW_ACCUMULATE_NODE_SESSION_PROG,
        true,
        {-1}
    },
    {
        PDS_DATAPATH_ASSIST_STAT_TOTALSESSIONSAGED,
        PDS_FLOW_ACCUMULATE_NODE_AGE_PROCESS,
        false,
        {PDS_FLOW_SESSION_DELETED, -1}
    },
    {
        -1,
        -1,
        true,
        {-1}
    }
};

void
pds_flow_monitor_accumulate_stats (void *vvm, uint64_t *counter) {
    int i, j, k;
    vlib_main_t *vm = (vlib_main_t *)vvm;
    vlib_main_t *stat_vm;
    vlib_node_t *n;
    vlib_node_t **nodes = 0;
    vlib_node_t ***node_vecs = 0; // vector of vectors
    vlib_error_main_t *em;
    u32 code;

    // Barrier sync across stats scraping, otherwise, counts will be inaccurate.
    vlib_worker_thread_barrier_sync(vm);
    for (j = 0; j < vec_len(vlib_mains); j++) {
        stat_vm = vlib_mains[j];
        nodes = NULL;

        // sync stats of each node of interest from the vlib_main
        for (i = 0; i < PDS_FLOW_ACCUMULATE_NODE_LAST; i++) {
            n = vlib_get_node_by_name(stat_vm, (u8 *)accumulate_node_names[i]);
            vec_add1(nodes, n);
            if (PREDICT_FALSE(!n)) {
                continue;
            }
            vlib_node_sync_stats(stat_vm, n);
        }

        // store vector of nodes in a vector
        assert(vec_len(nodes) == PDS_FLOW_ACCUMULATE_NODE_LAST);
        vec_add1(node_vecs, nodes);
    }
    vlib_worker_thread_barrier_release (vm);

    assert(vec_len(node_vecs) == vec_len(vlib_mains));

    // accumulate each stat
    for (k = 0; accumulate_data[k].stats_idx != -1; k++) {
        // collect stat from each vlib_main
        for (j = 0; j < vec_len(vlib_mains); j++) {
            nodes = node_vecs[j];
            n = nodes[accumulate_data[k].node_idx];
            if (PREDICT_FALSE(!n)) {
                continue;
            }
            if (accumulate_data[k].use_vectors) {
                counter[accumulate_data[k].stats_idx] += n->stats_total.vectors -
                    n->stats_last_clear.vectors;
            } else {
                stat_vm = vlib_mains[j];
                em = &stat_vm->error_main;
                for (i = 0; accumulate_data[k].error_idx[i] != -1; i++) {
                    code = n->error_heap_index + accumulate_data[k].error_idx[i];
                    counter[accumulate_data[k].stats_idx] += em->counters[code];
                    if (code < vec_len(em->counters_last_clear)) {
                        counter[accumulate_data[k].stats_idx] -= em->counters_last_clear[code];
                    }
                }
            }
        }
    }

    for (j = 0; j < vec_len(node_vecs); j++) {
        vec_free(node_vecs[j]);
    }
    vec_free(node_vecs);
}

