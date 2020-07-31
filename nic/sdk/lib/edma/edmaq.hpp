//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// edma library 
///
//----------------------------------------------------------------------------

#ifndef __EDMAQ_HPP__
#define __EDMAQ_HPP__

#include <chrono>
#include <cstdint>
#include <ev.h>

#include "lib/edma/edmaq.h"
#include "platform/evutils/include/evutils.h"
#include "platform/utils/program.hpp"

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

class edma_q {
public:
    edma_q() {}
    ~edma_q() {}
    static edma_q *factory(
        const char *name,
        uint16_t lif,
        uint8_t qtype,
        uint32_t qid,
        uint64_t ring_base,
        uint64_t comp_base,
        uint16_t ring_size,
        void *sw_phv = NULL,
        EV_P = NULL
    );

    bool init(sdk::platform::utils::program_info *pinfo, uint64_t qstate_ptr, uint8_t cos_sel,
              uint8_t cosA, uint8_t cosB);
    bool reset(void);
    bool debug(bool enable);
    bool post(edma_opcode opcode, uint64_t from, uint64_t to, uint16_t size,
              struct edmaq_ctx *ctx);
    static void poll_cb(void *obj);
    void flush(void);

    // Getters & setters for private variables
    // name
    void set_name(const char *name) { name_ = name; }
    const char *name(void) { return name_; }

    // init
    void set_init(bool init) { init_ = init; }
    bool init(void) { return init_; }

    // lif
    void set_lif(uint16_t lif) { lif_ = lif; }
    uint16_t lif(void) { return lif_; }

    // qtype
    void set_qtype(uint8_t qtype) { qtype_ = qtype; }
    uint8_t qtype(void) { return qtype_; }

    // qid
    void set_qid(uint32_t qid) { qid_ = qid; }
    uint32_t qid(void) { return qid_; }

    // head
    void set_head(uint16_t head) { head_ = head; }
    uint16_t head(void) { return head_; }

    // tail
    void set_tail(uint16_t tail) { tail_ = tail; }
    uint16_t tail(void) { return tail_; }

    // ring_size
    void set_ring_size(uint16_t ring_size) { ring_size_ = ring_size; }
    uint16_t ring_size(void) { return ring_size_; }

    // comp_tail
    void set_comp_tail(uint16_t comp_tail) { comp_tail_ = comp_tail; }
    uint16_t comp_tail(void) { return comp_tail_; }

    // qstate_addr
    void set_qstate_addr(uint64_t qstate_addr) { qstate_addr_ = qstate_addr; }
    uint64_t qstate_addr(void) { return qstate_addr_; }

    // ring_base
    void set_ring_base(uint64_t ring_base) { ring_base_ = ring_base; }
    uint64_t ring_base(void) { return ring_base_; }

    // comp_base
    void set_comp_base(uint64_t comp_base) { comp_base_ = comp_base; }
    uint64_t comp_base(void) { return comp_base_; }

    // exp_color
    void set_exp_color(uint8_t exp_color) { exp_color_ = exp_color; }
    uint8_t exp_color(void) { return exp_color_; }

    // sw_phv
    void set_sw_phv(void *sw_phv) { sw_phv_ = sw_phv; }
    void *sw_phv(void) { return sw_phv_; }

private:
    bool empty_(void);
    bool poll_(void);
    sdk_ret_t post_via_sw_phv_(uint16_t index);
    sdk_ret_t post_via_db_(uint16_t index);

private:
    const char *name_;
    bool init_;

    uint16_t lif_;
    uint8_t  qtype_;
    uint32_t qid_;

    uint16_t head_;
    uint16_t tail_;
    uint64_t qstate_addr_;
    uint64_t ring_base_;
    uint64_t comp_base_;
    uint16_t ring_size_;
    uint16_t comp_tail_;
    uint8_t  exp_color_;
    void     *sw_phv_;

    struct edmaq_ctx *pending_;

    // Tasks
    EV_P;
    evutil_prepare prepare_ = {0};
};

#endif    /* __EDMAQ_HPP__ */
