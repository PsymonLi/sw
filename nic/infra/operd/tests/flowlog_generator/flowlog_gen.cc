//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// implementation of flow_logs_generator
///
//----------------------------------------------------------------------------

#include <unistd.h>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>

#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/lib/operd/decoder.h"
#include "nic/sdk/lib/operd/logger.hpp"
#include "nic/infra/operd/decoders/vpp/flow_decoder.h"
#include "flowlog_gen.hpp"

using namespace std;

namespace test {

flow_logs_generator *
flow_logs_generator::factory(uint32_t rate, uint32_t total,
                             uint32_t lookup_id, string type) {
    srand(time(nullptr));
    return new flow_logs_generator(rate, total, lookup_id, type);
}

void
flow_logs_generator::destroy(flow_logs_generator *inst) {
    delete inst;
}

void
flow_logs_generator::show(void) {
    cout << "Flow logs rate per second: " << rate_ << endl;
    cout << "Total no. of flow logs to be generated: " << total_ << endl;
}

void
flow_logs_generator::generate(void) {
    uint32_t num_flow_logs = 0;
    uint32_t time_elapsed;
    using micros = chrono::microseconds;

    while (num_flow_logs < total_) {
        auto start = chrono::steady_clock::now();
        for (uint32_t i = 0; i < rate_; ++i) {
            record_flow_log_();
        }
        num_flow_logs += rate_;
        auto finish = chrono::steady_clock::now();
        time_elapsed = chrono::duration_cast<micros>(finish - start).count();
        usleep(1000000 - time_elapsed);
    }
    cout << "No. of flow logs generated " << total_ << endl;
}

static inline uint64_t
get_random_number (uint64_t max, uint64_t min = 0)
{
    return (rand() % max) + min;
}
#define RANDOM_PROTO_NUM              get_random_number(UINT8_MAX)
#define RANDOM_PORT_NUM               get_random_number(UINT16_MAX)
#define RANDOM_IPV4_ADDR              get_random_number(UINT32_MAX)
#define RANDOM_MAC_ADDR               get_random_number(UINT64_MAX)
#define RANDOM_NO_IN_RANGE(min, max)  get_random_number(max+1, min)

static inline void
populate_flow_info (operd_flow_t *flow_log, uint32_t flow_type)
{
    flow_log->action = RANDOM_NO_IN_RANGE(OPERD_FLOW_ACTION_ALLOW,
                                          OPERD_FLOW_ACTION_DENY);
    flow_log->logtype = RANDOM_NO_IN_RANGE(OPERD_FLOW_LOGTYPE_ADD,
                                           OPERD_FLOW_LOGTYPE_ACTIVE);
    flow_log->session_id = get_random_number(UINT32_MAX);
    flow_log->type = flow_type;
}

static inline void
populate_l2_flow_info (operd_flow_t *flow_log, uint16_t lookup_id)
{
    MAC_UINT64_TO_ADDR(flow_log->l2.src, RANDOM_MAC_ADDR);
    MAC_UINT64_TO_ADDR(flow_log->l2.dst, RANDOM_MAC_ADDR);
    flow_log->l2.ether_type = ETH_TYPE_LLDP;
    flow_log->l2.bd_id = lookup_id;
}

static inline void
populate_ipv4_flow_info (operd_flow_t *flow_log, uint16_t lookup_id)
{
    flow_log->v4.src = RANDOM_IPV4_ADDR;
    flow_log->v4.dst = RANDOM_IPV4_ADDR;
    flow_log->v4.proto = RANDOM_PROTO_NUM;
    flow_log->v4.sport = RANDOM_PORT_NUM;
    flow_log->v4.dport = RANDOM_PORT_NUM;
    flow_log->v4.lookup_id = lookup_id;
}

static inline void
populate_nat_info (operd_flow_t *flow_log)
{
    flow_log->nat_data.src_nat_addr = RANDOM_IPV4_ADDR;
    flow_log->nat_data.dst_nat_addr = RANDOM_IPV4_ADDR;
    flow_log->nat_data.src_nat_port = RANDOM_PORT_NUM;
    flow_log->nat_data.dst_nat_port = RANDOM_PORT_NUM;
}

static inline void
populate_stats (operd_flow_t *flow_log)
{
    flow_log->stats.iflow_bytes_count = get_random_number(UINT32_MAX);
    flow_log->stats.iflow_packets_count = get_random_number(UINT32_MAX);
    flow_log->stats.rflow_bytes_count = get_random_number(UINT32_MAX);
    flow_log->stats.rflow_packets_count = get_random_number(UINT32_MAX);
}

void
flow_logs_generator::record_flow_log_(void) {
    operd_flow_t *flow_log;

    flow_log = (operd_flow_t *)recorder_->get_raw_chunk(
        OPERD_DECODER_VPP, sdk::operd::INFO, sizeof(operd_flow_t));
    populate_flow_info(flow_log, type_);
    switch (flow_log->type) {
    case OPERD_FLOW_TYPE_IP4:
        populate_ipv4_flow_info(flow_log, lookup_id_);
        populate_nat_info(flow_log);
        break;
    case OPERD_FLOW_TYPE_L2:
        populate_l2_flow_info(flow_log, lookup_id_);
        break;
    default:
        assert(0);
        break;
    }
    populate_stats(flow_log);
    recorder_->commit_raw_chunk(flow_log);
    return;
}

flow_logs_generator::flow_logs_generator(uint32_t rate, uint32_t total,
                                         uint32_t lookup_id, string type) {
    rate_ = rate;
    total_ = total;
    lookup_id_ = lookup_id;
    if (type == "ipv4") {
        type_ = OPERD_FLOW_TYPE_IP4;
    } else if (type == "ipv6") {
        type_ = OPERD_FLOW_TYPE_IP6;
    } else if (type == "l2") {
        type_ = OPERD_FLOW_TYPE_L2;
    } else {
        assert(0);
    }
    recorder_ = sdk::operd::region::create(FLOW_LOG_PRODUCER_NAME);
}

}    // namespace test
