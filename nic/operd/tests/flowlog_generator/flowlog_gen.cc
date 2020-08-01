//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// implementation of flow logs generator
///
//----------------------------------------------------------------------------

#include <unistd.h>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>

#include "nic/sdk/lib/operd/decoder.h"
#include "nic/sdk/lib/operd/logger.hpp"
#include "nic/operd/decoders/vpp/flow_decoder.h"
#include "flowlog_gen.hpp"

using namespace std;

namespace test {
namespace flow_logs {

generator *
generator::factory(uint32_t rate, uint32_t total)
{
    srand(time(nullptr));
    return new generator(rate, total);
}

void
generator::destroy(generator *inst) {
    delete inst;
}

void
generator::show(void) {
    cout << "Flow logs rate per second: " << rate_ << endl;
    cout << "Total no. of flow logs to be generated: " << total_ << endl;
}

void
generator::generate(void) {
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

static inline uint32_t
get_random_number(uint32_t max)
{
    return rand() % max;
}
#define RANDOM_PROTO_NUM  get_random_number(UINT8_MAX)
#define RANDOM_PORT_NUM   get_random_number(UINT16_MAX)
#define RANDOM_IPV4_ADDR  get_random_number(UINT32_MAX)

static inline void
populate_ipv4_flow_info(operd_flow_t *flow_log)
{
    flow_log->type = OPERD_FLOW_TYPE_IP4;
    flow_log->action = OPERD_FLOW_ACTION_ALLOW;
    flow_log->logtype = OPERD_FLOW_LOGTYPE_ADD;
    flow_log->v4.src = RANDOM_IPV4_ADDR;
    flow_log->v4.dst = RANDOM_IPV4_ADDR;
    flow_log->v4.proto = RANDOM_PROTO_NUM;
    flow_log->v4.sport = RANDOM_PORT_NUM;
    flow_log->v4.dport = RANDOM_PORT_NUM;
}

void
generator::record_flow_log_(void)
{
    operd_flow_t *flow_log;

    flow_log = (operd_flow_t *)recorder_->get_raw_chunk(
        OPERD_DECODER_VPP, sdk::operd::INFO, sizeof(operd_flow_t));
    // TODO: Handle different flow types
    populate_ipv4_flow_info(flow_log);
    recorder_->commit_raw_chunk(flow_log);
    return;
}

}    // namespace flow_logs
}    // namespace test
