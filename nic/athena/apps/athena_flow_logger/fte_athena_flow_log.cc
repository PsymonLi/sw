//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains all session aging test cases for athena
///
//----------------------------------------------------------------------------
#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "nic/infra/core/trace.hpp"
#include "fte_athena_flow_log.hpp"


namespace flow_logger {

pds_flow_log_table_t    active_table;
static event::timer_t   fl_timer;
uint32_t                timeOut;
std::string             logDir;
typedef struct flow_logger_ctx_t_ {
    uint32_t  timeout;
    std::string logdir;
} flow_logger_ctx_t;
flow_logger_ctx_t       thread_ctx;

void
fte_ath_flow_log_cb_fn (pds_flow_log_table_iter_cb_arg_t *arg)
{   

    pds_flow_log_entry_key_t  *key = NULL;
    pds_flow_log_entry_data_t *data = NULL;
    FILE                      *fp = NULL;
    char                      srcstr[INET6_ADDRSTRLEN];
    char                      dststr[INET6_ADDRSTRLEN];
    uint32_t                  src_addr = 0, dst_addr=0;
    uint8_t                   ntohs_ip[IP6_ADDR8_LEN];

    if (arg == NULL) {
        PDS_TRACE_ERR("fte_ath_flow_log_cb_fn called with NULL arguments \n");
        return;
    }

    fp = (FILE *)arg->app_data;

    if (fp == NULL) {
        PDS_TRACE_ERR("File pointer not specified \n");
        return;
    }
   
    key = &arg->info.key;
    data = &arg->info.data;

    if (key == NULL || data == NULL) {
        PDS_TRACE_ERR("Invalid arguments, key or data is NULL \n");
        return;
    }

    if (key->key_type == KEY_TYPE_IPV6) {
        ipv6_addr_ntoh(key->ip_saddr, ntohs_ip);
        inet_ntop(AF_INET6, ntohs_ip, srcstr, INET6_ADDRSTRLEN);
        ipv6_addr_ntoh(key->ip_daddr, ntohs_ip);
        inet_ntop(AF_INET6, ntohs_ip, dststr, INET6_ADDRSTRLEN);
    } else if (key->key_type == KEY_TYPE_IPV4) {
        src_addr = ntohl(*(uint32_t *)key->ip_saddr);
        dst_addr = ntohl(*(uint32_t *)key->ip_daddr);
        inet_ntop(AF_INET, &src_addr, srcstr, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &dst_addr, dststr, INET_ADDRSTRLEN);
    } else {
        PDS_TRACE_ERR("Unsupported Key type %d. Return\n", key->key_type);
        return;
    }

    fprintf(fp, "Index %u:\n" 
            "Key ==> SrcIP: %s, DstIP: %s, "
            "Dport: %u, Sport: %u, Proto %u, "
            "Ktype: %u, VNICID:%u, disposition:%u\n"
            "Data ==>  pkts_from_host: %u, pkts_from_switch: %u, "
            "bytes_from_host: %lu, bytes_from_switch:%lu, "
            "start_timestamp: %lu, end_timestamp: %lu\n",
            arg->key.entry_idx, 
            srcstr, dststr,
            key->dport, key->sport,
            key->proto, key->key_type, key->vnic_id, key->disposition,
            data->pkt_from_host, data->pkt_from_switch,
            data->bytes_from_host, data->bytes_from_switch,
            data->start_time, data->last_time);
}

void 
fte_ath_handle_flow_log_timer_expiry (event::timer_t *t)
{

    pds_flow_log_table_t                 flow_tbl_sel;
    pds_flow_log_table_selector_spec_t   spec;
    pds_ret_t                            ret = PDS_RET_OK;
    pds_flow_log_table_iter_cb_arg_t     cb_arg;
    FILE                                 *fp = NULL;
    std::string                          fname;
    std::string                          fpath;

    PDS_TRACE_VERBOSE("%s: Entered. Active table %d \n", 
                      __FUNCTION__, active_table);

    if (t == NULL) {
        PDS_TRACE_ERR("Args for timer callback are NULL\n");
        return;
    }

    flow_tbl_sel = active_table;

    if (flow_tbl_sel == PDS_FLOW_LOG_TABLE0) {
        spec.flow_log_table = PDS_FLOW_LOG_TABLE1;
    } else {
        spec.flow_log_table = PDS_FLOW_LOG_TABLE0;
    }
    active_table = spec.flow_log_table;

    ret = pds_flow_log_table_selector_update(&spec);
    if (ret != PDS_RET_OK) {
        PDS_TRACE_ERR("unable to select table %d, err %d \n", 
                      flow_tbl_sel, ret);
        return;
    }

    fname = "flow_log_"+std::to_string((int)flow_tbl_sel)+".txt";
    fpath = logDir + "/" + fname;
    PDS_TRACE_VERBOSE("Writing into file %s \n", fpath.c_str());
    fp = fopen(fpath.c_str(), "w+");
    if (fp == NULL) {
        PDS_TRACE_ERR("Unable to open file %s", fname.c_str());
        return;
    }

    cb_arg.app_data = (void *)fp;
    pds_flow_log_table_entry_iterate(fte_ath_flow_log_cb_fn, 
                                     flow_tbl_sel, 
                                     &cb_arg);
    //ret = pds_flow_log_table_reset(flow_tbl_sel);
    //if (ret != PDS_RET_OK) {
    //    PDS_TRACE_ERR("Unable to reset contents of flow table %d", 
    //                 flow_tbl_sel);
    //}
    fclose(fp);
}

void
fte_flow_log_timer_init(void *ctx)
{
    //unsigned long data = 0;
    flow_logger_ctx_t  *flow_log_ctx = NULL;
  
    if (ctx == NULL) {
        return;
    }

    flow_log_ctx = (flow_logger_ctx_t *)ctx;
    timeOut = flow_log_ctx->timeout;

    logDir = flow_log_ctx->logdir;


    event::timer_init(&fl_timer, fte_ath_handle_flow_log_timer_expiry, 
                      (double) timeOut, (double) timeOut);
    event::timer_start(&fl_timer);
}

void
fte_flow_log_timer_exit (void *ctxt)
{
    PDS_TRACE_VERBOSE("%s: called \n", __FUNCTION__);
}

void
fte_flow_log_timer_cb (void *msg, void *ctxt)
{
    PDS_TRACE_VERBOSE("%s: called \n", __FUNCTION__);
}

sdk_ret_t
fte_athena_spawn_flow_log_thread (int numSeconds, 
                                  std::string logdir)                                              
{                                                                                                                      
    event::event_thread *new_thread = NULL;                                                                       
 
    // spawn learn thread                                                                                              
    PDS_TRACE_DEBUG("Spawning Flow log thread, logdir %s", logdir.c_str());
    new_thread =
        sdk::event_thread::event_thread::factory(                                                                      
            "flow_log", 1,                                                                              
            sdk::lib::THREAD_ROLE_CONTROL, 0,
            flow_logger::fte_flow_log_timer_init,
            flow_logger::fte_flow_log_timer_exit,
            flow_logger::fte_flow_log_timer_cb,
            sdk::lib::thread::priority_by_role(sdk::lib::THREAD_ROLE_CONTROL),                                         
            sdk::lib::thread::sched_policy_by_role(
                                 sdk::lib::THREAD_ROLE_CONTROL),                                     
            (THREAD_YIELD_ENABLE | THREAD_SYNC_IPC_ENABLE));                                                                                               
    SDK_ASSERT_TRACE_RETURN((new_thread != NULL), SDK_RET_ERR,                                                         
                            "learn thread create failure");                                                            
    thread_ctx.logdir = logdir;
    thread_ctx.timeout = numSeconds;

    new_thread->start(&thread_ctx);
    return SDK_RET_OK;
}

}

