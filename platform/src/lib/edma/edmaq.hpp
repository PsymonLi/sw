/*
* Copyright (c) 2019, Pensando Systems Inc.
*/

#ifndef __EDMAQ_HPP__
#define __EDMAQ_HPP__

#include <chrono>
#include <cstdint>
#include <ev.h>

#include "nic/include/edmaq.h"
#include "nic/sdk/platform/evutils/include/evutils.h"
#include "nic/sdk/platform/utils/program.hpp"

using namespace sdk::lib;
using namespace sdk::platform::utils;

#define EDMAQ_COMP_POLL_US          1000 // 1 ms
#define EDMAQ_COMP_POLL_S           (0.001) // 1 ms
#define EDMAQ_COMP_TIMEOUT_S        (0.01)  // 10 ms
#define EDMAQ_COMP_TIMEOUT_MS       10  // 10 ms
#define EDMAQ_MAX_TRANSFER_SZ       (((1U << 14) - 1) & (~63U))

typedef void (*edmaq_cb_t) (void *obj);

struct edmaq_ctx {
    edmaq_cb_t cb;
    void *obj;
    chrono::steady_clock::time_point deadline;
};

class EdmaQ {
public:
    EdmaQ() {}
    ~EdmaQ() {}
    static EdmaQ *factory(
        const char *name,
        uint16_t lif,
        uint8_t qtype,
        uint32_t qid,
        uint64_t ring_base,
        uint64_t comp_base,
        uint16_t ring_size,
        EV_P
    );

    bool Init(sdk::platform::utils::program_info *pinfo, uint64_t qstate_ptr, uint8_t cos_sel,
        uint8_t cosA, uint8_t cosB);
    bool Reset();
    bool Debug(bool enable);

    bool Post(edma_opcode opcode, uint64_t from, uint64_t to, uint16_t size,
        struct edmaq_ctx *ctx);
    static void PollCb(void *obj);
    void Flush();

private:
    const char *name;
    bool init;

    uint16_t lif;
    uint8_t qtype;
    uint32_t qid;

    uint16_t head;
    uint16_t tail;
    uint64_t qstate_addr;
    uint64_t ring_base;
    uint64_t comp_base;
    uint16_t ring_size;
    uint16_t comp_tail;
    uint8_t exp_color;

    struct edmaq_ctx *pending;

    // Tasks
    EV_P;
    evutil_prepare prepare = {0};

    bool Empty();
    bool Poll();
};

#endif    /* __EDMAQ_HPP__ */
