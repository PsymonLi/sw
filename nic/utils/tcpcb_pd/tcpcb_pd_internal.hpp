// {C} Copyright 2019 Pensando Systems Inc. All rights reserved

#ifndef __TCPCB_PD_INTERNAL_HPP__
#define __TCPCB_PD_INTERNAL_HPP__

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define CALL_AND_CHECK_FN(_f) \
    ret = _f; \
    if (ret != SDK_RET_OK) { \
        return ret; \
    }

#define TCP_QTYPE0 0
#define TCP_QTYPE1 1

using sdk::asic::asic_mem_read;
using sdk::asic::asic_mem_write;
using sdk::asic::pd::asicpd_p4plus_invalidate_cache;

namespace sdk {
namespace tcp {

// **Note: offsets need to match stats update in tcp-rx-stats.h
typedef struct __attribute__((__packed__)) __tcp_rx_stats_t {
    uint64_t bytes_rcvd;
    uint64_t pkts_rcvd;
    uint64_t bytes_acked;
    uint64_t pure_acks_rcvd;
    uint64_t dup_acks_rcvd;
    uint64_t ooo_cnt;
    uint64_t stats6;
    uint64_t stats7;
} tcp_rx_stats_t;

// **Note: offsets need to match stats update in tcp-tx-stats.h
typedef struct __attribute__((__packed__)) __tcp_tx_stats_t {
    uint64_t bytes_sent;
    uint64_t pkts_sent;
    uint64_t pure_acks_sent;
    uint64_t stats3;
    uint64_t stats4;
    uint64_t stats5;
    uint64_t stats6;
    uint64_t stats7;
} tcp_tx_stats_t;

static inline sdk_ret_t
tcpcb_pd_write_one(uint64_t addr, uint8_t *data, uint32_t size)
{
    sdk_ret_t ret;
    ret = asic_mem_write(addr, data, size);
    if (ret == SDK_RET_OK) {
        if (!asicpd_p4plus_invalidate_cache(addr, size,
                                            P4PLUS_CACHE_INVALIDATE_BOTH)) {
            return SDK_RET_ERR;
        }
    }

    return ret;
}

static inline sdk_ret_t
tcpcb_pd_read_one(uint64_t addr, uint8_t *data, uint32_t size)
{
    sdk_ret_t ret;

    ret = asic_mem_read(addr, data, size);

    return ret;
}

} // namespace tcp
} // namespace sdk

#endif // __TCPCB_PD_INTERNAL_HPP__
