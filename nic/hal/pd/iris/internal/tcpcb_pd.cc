#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "nic/include/base.hpp"
#include <arpa/inet.h>
#include "nic/sdk/include/sdk/lock.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/hal/pd/iris/internal/tcpcb_pd.hpp"
#include "nic/hal/pd/iris/p4pd_tcp_proxy_api.h"
#include "nic/hal/pd/iris/p4pd_mem.hpp"
#include "nic/hal/pd/libs/wring/wring_pd.hpp"
#include "nic/include/pd.hpp"
#include "nic/hal/src/internal/proxy.hpp"
#include "nic/hal/hal.hpp"
#include "asic/common/asic_common.hpp"
#include "nic/include/tcp_common.h"
#include "nic/include/tls_common.h"
#include "nic/include/app_redir_shared.h"
#include "nic/hal/pd/iris/internal/tlscb_pd.hpp"
#ifdef ELBA
#include "nic/sdk/platform/elba/elba_txs_scheduler.hpp"
#else
#include "nic/sdk/platform/capri/capri_txs_scheduler.hpp"
#include "nic/sdk/platform/capri/capri_barco_crypto.hpp"
#endif
#include "nic/utils/tcpcb_pd/tcpcb_pd.hpp"
#include "p4/loader/loader.hpp"

#ifdef ELBA
using namespace sdk::platform::elba;
/* TBD-ELBA-REBASE: get these from elba_barco_crypto.hpp */
#define ELBA_MAX_TLS_PAD_SIZE              512
#define ELBA_GC_GLOBAL_TABLE               (ELBA_MAX_TLS_PAD_SIZE + 128)
#define ELBA_GC_GLOBAL_OOQ_TX2RX_FP_PI     (ELBA_GC_GLOBAL_TABLE + 8)
#else
using namespace sdk::platform::capri;
#endif

namespace hal {
namespace pd {
// byte array to hex string for logging
std::string hex_dump(const uint8_t *buf, size_t sz)
{
    std::ostringstream result;

    for(size_t i = 0; i < sz; i+=8) {
        result << " 0x";
        for (size_t j = i ; j < sz && j < i+8; j++) {
            result << std::setw(2) << std::setfill('0') << std::hex << (int)buf[j];
        }
    }

    return result.str();
}

void *
tcpcb_pd_get_hw_key_func (void *entry)
{
    SDK_ASSERT(entry != NULL);
    return (void *)&(((pd_tcpcb_t *)entry)->hw_id);
}

uint32_t
tcpcb_pd_hw_key_size ()
{
    return sizeof(tcpcb_hw_id_t);
}

/********************************************
 * RxDMA
 * ******************************************/

uint64_t tcpcb_pd_serq_prod_ci_addr_get(uint32_t qid)
{
    uint64_t addr;

    addr = lif_manager()->get_lif_qstate_addr(SERVICE_LIF_TCP_PROXY, 0,
            qid) + (P4PD_TCPCB_STAGE_ENTRY_OFFSET * P4PD_HWID_TCP_RX_READ_TX2RX);
    // offsetof does not work on bitfields
    //addr += offsetof(common_p4plus_stage0_app_header_table_read_tx2rx_d, serq_cidx);
    addr += 24;

    return addr;
}

uint64_t tcpcb_pd_ooo_rx2tx_prod_ci_addr_get(uint32_t qid)
{
    uint64_t addr;

    addr = lif_manager()->get_lif_qstate_addr(SERVICE_LIF_TCP_PROXY, 0,
            qid) + TCP_TCB_OOO_QADDR_CI_OFFSET;

    return addr;
}


hal_ret_t
p4pd_get_tcp_rx_ooq_entry(pd_tcpcb_t* tcpcb_pd)
{
    s6_t2_tcp_rx_ooo_qbase_cb_load_d cb_load_d = {0};
    s3_t2_tcp_rx_ooo_book_keeping_d bookkeeping_d = {0};

    // hardware index for this entry
    tcpcb_hw_id_t hwid = tcpcb_pd->hw_id +
        (P4PD_TCPCB_STAGE_ENTRY_OFFSET * P4PD_HWID_TCP_RX_OOO_QADDR_OFFSET);
    if(sdk::asic::asic_mem_read(hwid,  (uint8_t *)&cb_load_d, sizeof(cb_load_d))){
        HAL_TRACE_ERR("Failed to read rx: ooq qaddr entry for TCP CB");
        return HAL_RET_HW_FAIL;
    }
    hwid = tcpcb_pd->hw_id +
        (P4PD_TCPCB_STAGE_ENTRY_OFFSET * P4PD_HWID_TCP_RX_OOO_BOOK_KEEPING_OFFSET);
    if(sdk::asic::asic_mem_read(hwid,  (uint8_t *)&bookkeeping_d,
                sizeof(bookkeeping_d))){
        HAL_TRACE_ERR("Failed to read rx: ooq bookkeeping entry for TCP CB");
        return HAL_RET_HW_FAIL;
    }

    if (bookkeeping_d.tcp_opt_flags & TCP_OPT_FLAG_SACK_PERM) {
        tcpcb_pd->tcpcb->sack_perm = true;
    }
    if (bookkeeping_d.tcp_opt_flags & TCP_OPT_FLAG_TIMESTAMPS) {
        tcpcb_pd->tcpcb->timestamps = true;
    }
    HAL_TRACE_DEBUG("tcp_opt_flags: {:#x}", bookkeeping_d.tcp_opt_flags);

    tcpcb_pd->tcpcb->ooq_entry[0].queue_addr = ntohll(cb_load_d.ooo_qbase_addr0);
    tcpcb_pd->tcpcb->ooq_entry[0].start_seq = ntohl(bookkeeping_d.start_seq0);
    tcpcb_pd->tcpcb->ooq_entry[0].end_seq = ntohl(bookkeeping_d.end_seq0);
    tcpcb_pd->tcpcb->ooq_entry[0].num_entries = ntohs(bookkeeping_d.tail_index0);

    tcpcb_pd->tcpcb->ooq_entry[1].queue_addr = ntohll(cb_load_d.ooo_qbase_addr1);
    tcpcb_pd->tcpcb->ooq_entry[1].start_seq = ntohl(bookkeeping_d.start_seq1);
    tcpcb_pd->tcpcb->ooq_entry[1].end_seq = ntohl(bookkeeping_d.end_seq1);
    tcpcb_pd->tcpcb->ooq_entry[1].num_entries = ntohs(bookkeeping_d.tail_index1);

    tcpcb_pd->tcpcb->ooq_entry[2].queue_addr = ntohll(cb_load_d.ooo_qbase_addr2);
    tcpcb_pd->tcpcb->ooq_entry[2].start_seq = ntohl(bookkeeping_d.start_seq2);
    tcpcb_pd->tcpcb->ooq_entry[2].end_seq = ntohl(bookkeeping_d.end_seq2);
    tcpcb_pd->tcpcb->ooq_entry[2].num_entries = ntohs(bookkeeping_d.tail_index2);

    tcpcb_pd->tcpcb->ooq_entry[3].queue_addr = ntohll(cb_load_d.ooo_qbase_addr3);
    tcpcb_pd->tcpcb->ooq_entry[3].start_seq = ntohl(bookkeeping_d.start_seq3);
    tcpcb_pd->tcpcb->ooq_entry[3].end_seq = ntohl(bookkeeping_d.end_seq3);
    tcpcb_pd->tcpcb->ooq_entry[3].num_entries = ntohs(bookkeeping_d.tail_index3);

    return HAL_RET_OK;
}

static void
p4pd_convert_sdk_tcpcb_to_hal_tcpcb(tcpcb_pd_t *sdk_tcpcb,
        pd_tcpcb_t *hal_tcpcb_pd)
{
    tcpcb_t *hal_tcpcb = hal_tcpcb_pd->tcpcb;

    hal_tcpcb->other_qid = sdk_tcpcb->other_qid;
    if (sdk_tcpcb->app_type == TCP_APP_TYPE_BYPASS)
        hal_tcpcb->proxy_type = types::PROXY_TYPE_TCP;
    else if (sdk_tcpcb->app_type == TCP_APP_TYPE_TLS)
        hal_tcpcb->proxy_type = types::PROXY_TYPE_TLS;
    else
        hal_tcpcb->proxy_type = types::PROXY_TYPE_NVME;
    hal_tcpcb->sesq_base = sdk_tcpcb->sesq_base;


    hal_tcpcb->rcv_nxt = sdk_tcpcb->rcv_nxt;
    hal_tcpcb->debug_dol = sdk_tcpcb->debug_dol;
    hal_tcpcb->debug_dol_tx = sdk_tcpcb->debug_dol_tx;
    hal_tcpcb->snd_nxt = sdk_tcpcb->snd_nxt;
    hal_tcpcb->rcv_wup = sdk_tcpcb->rcv_wup;
    hal_tcpcb->rcv_wnd = sdk_tcpcb->rcv_wnd;
    hal_tcpcb->snd_una = sdk_tcpcb->snd_una;
    hal_tcpcb->pred_flags = sdk_tcpcb->pred_flags;
    hal_tcpcb->rcv_wscale = sdk_tcpcb->rcv_wscale;
    hal_tcpcb->ato = sdk_tcpcb->ato;
    hal_tcpcb->snd_wnd = sdk_tcpcb->snd_wnd;
    hal_tcpcb->rtt_seq_tsoffset = sdk_tcpcb->rtt_seq_tsoffset;
    hal_tcpcb->rtt_time = sdk_tcpcb->rtt_time;
    hal_tcpcb->ts_learned = sdk_tcpcb->ts_learned;
    hal_tcpcb->delay_ack = sdk_tcpcb->delay_ack;
    hal_tcpcb->ooo_queue = sdk_tcpcb->ooo_queue;
    hal_tcpcb->sack_perm = sdk_tcpcb->sack_perm;
    hal_tcpcb->timestamps = sdk_tcpcb->timestamps;
    hal_tcpcb->cc_algo = sdk_tcpcb->cc_algo;
    hal_tcpcb->smss = sdk_tcpcb->smss;
    hal_tcpcb->snd_cwnd = sdk_tcpcb->snd_cwnd;
    hal_tcpcb->snd_ssthresh = sdk_tcpcb->snd_ssthresh;
    hal_tcpcb->snd_wscale = sdk_tcpcb->snd_wscale;
    hal_tcpcb->abc_l_var = sdk_tcpcb->abc_l_var;
    hal_tcpcb->rcv_mss = sdk_tcpcb->rcv_mss;
    hal_tcpcb->initial_window = sdk_tcpcb->initial_window;
    hal_tcpcb->source_lif = sdk_tcpcb->source_lif;
    hal_tcpcb->source_port = sdk_tcpcb->source_port;
    hal_tcpcb->dest_port = sdk_tcpcb->dest_port;
    hal_tcpcb->header_len = sdk_tcpcb->header_len;
    hal_tcpcb->ts_offset = sdk_tcpcb->ts_offset;
    hal_tcpcb->ts_time = sdk_tcpcb->ts_time;
    hal_tcpcb->ts_recent = sdk_tcpcb->ts_recent;
    memcpy(hal_tcpcb->header_template, sdk_tcpcb->header_template,
            sizeof(hal_tcpcb->header_template));

    hal_tcpcb->serq_ci = sdk_tcpcb->serq_ci;
    hal_tcpcb->serq_pi = sdk_tcpcb->serq_pi;

    hal_tcpcb->state = sdk_tcpcb->state;
    hal_tcpcb->rx_ts = sdk_tcpcb->rx_ts;
    hal_tcpcb->snd_recover = sdk_tcpcb->snd_recover;
    hal_tcpcb->ooq_not_empty = sdk_tcpcb->ooq_not_empty;
    hal_tcpcb->cc_flags = sdk_tcpcb->cc_flags;
    hal_tcpcb->rto = sdk_tcpcb->rto;
    hal_tcpcb->srtt_us = sdk_tcpcb->srtt_us;
    hal_tcpcb->retx_snd_una = sdk_tcpcb->retx_snd_una;
    hal_tcpcb->ato_deadline = sdk_tcpcb->ato_deadline;
    hal_tcpcb->sesq_base = sdk_tcpcb->sesq_base;
    hal_tcpcb->rto_deadline = sdk_tcpcb->rto_deadline;
    hal_tcpcb->keepa_deadline = sdk_tcpcb->keepa_deadline;
    hal_tcpcb->rto_backoff = sdk_tcpcb->rto_backoff;
    hal_tcpcb->read_notify_bytes = sdk_tcpcb->read_notify_bytes;
    hal_tcpcb->read_notify_bytes_local = sdk_tcpcb->read_notify_bytes_local;
}

static void
p4pd_convert_sdk_stats_tcpcb_to_hal_tcpcb(tcpcb_stats_pd_t *sdk_stats_tcpcb,
        pd_tcpcb_t *hal_tcpcb_pd)
{
    tcpcb_t *hal_tcpcb = hal_tcpcb_pd->tcpcb;

    hal_tcpcb->bytes_rcvd = sdk_stats_tcpcb->bytes_rcvd;
    hal_tcpcb->bytes_acked = sdk_stats_tcpcb->bytes_acked;
    hal_tcpcb->pure_acks_rcvd = sdk_stats_tcpcb->pure_acks_rcvd;
    hal_tcpcb->dup_acks_rcvd = sdk_stats_tcpcb->dup_acks_rcvd;
    hal_tcpcb->ooo_cnt = sdk_stats_tcpcb->ooo_cnt;
    hal_tcpcb->pkts_rcvd = sdk_stats_tcpcb->pkts_rcvd;
    hal_tcpcb->slow_path_cnt = sdk_stats_tcpcb->slow_path_cnt;
    hal_tcpcb->serq_full_cnt = sdk_stats_tcpcb->serq_full_cnt;
    hal_tcpcb->rx_drop_cnt = sdk_stats_tcpcb->rx_drop_cnt;

    hal_tcpcb->zero_window_sent = sdk_stats_tcpcb->zero_window_sent;
    hal_tcpcb->bytes_sent = sdk_stats_tcpcb->bytes_sent;
    hal_tcpcb->pkts_sent = sdk_stats_tcpcb->pkts_sent;
    hal_tcpcb->pure_acks_sent = sdk_stats_tcpcb->pure_acks_sent;
    hal_tcpcb->sesq_pi = sdk_stats_tcpcb->sesq_pi;
    hal_tcpcb->sesq_ci = sdk_stats_tcpcb->sesq_ci;
    hal_tcpcb->sesq_retx_ci = sdk_stats_tcpcb->sesq_retx_ci;
    hal_tcpcb->sesq_tx_ci = sdk_stats_tcpcb->sesq_tx_ci;
    hal_tcpcb->send_ack_pi = sdk_stats_tcpcb->send_ack_pi;
    hal_tcpcb->send_ack_ci = sdk_stats_tcpcb->send_ack_ci;
    hal_tcpcb->fast_timer_pi = sdk_stats_tcpcb->fast_timer_pi;
    hal_tcpcb->fast_timer_ci = sdk_stats_tcpcb->fast_timer_ci;
    hal_tcpcb->del_ack_pi = sdk_stats_tcpcb->del_ack_pi;
    hal_tcpcb->del_ack_ci = sdk_stats_tcpcb->del_ack_ci;
    hal_tcpcb->asesq_pi = sdk_stats_tcpcb->asesq_pi;
    hal_tcpcb->asesq_ci = sdk_stats_tcpcb->asesq_ci;
    hal_tcpcb->asesq_retx_ci = sdk_stats_tcpcb->asesq_retx_ci;
    hal_tcpcb->pending_tx_pi = sdk_stats_tcpcb->pending_tx_pi;
    hal_tcpcb->pending_tx_ci = sdk_stats_tcpcb->pending_tx_ci;
    hal_tcpcb->fast_retrans_pi = sdk_stats_tcpcb->fast_retrans_pi;
    hal_tcpcb->fast_retrans_ci = sdk_stats_tcpcb->fast_retrans_ci;
    hal_tcpcb->clean_retx_pi = sdk_stats_tcpcb->clean_retx_pi;
    hal_tcpcb->clean_retx_ci = sdk_stats_tcpcb->clean_retx_ci;

    hal_tcpcb->packets_out = sdk_stats_tcpcb->packets_out;
    hal_tcpcb->window_full_cnt = sdk_stats_tcpcb->window_full_cnt;
    hal_tcpcb->retx_cnt = sdk_stats_tcpcb->retx_cnt;
    hal_tcpcb->tx_ring_pi = sdk_stats_tcpcb->tx_ring_pi;
    hal_tcpcb->partial_pkt_ack_cnt = sdk_stats_tcpcb->partial_pkt_ack_cnt;
    hal_tcpcb->tx_window_update_pi = sdk_stats_tcpcb->tx_window_update_pi;
}

hal_ret_t
p4pd_get_tcpcb_rxdma_entry(pd_tcpcb_t* tcpcb_pd)
{
    hal_ret_t   ret = HAL_RET_OK;

    ret = p4pd_get_tcp_rx_ooq_entry(tcpcb_pd);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to get ooq entry");
        goto cleanup;
    }

    return HAL_RET_OK;
cleanup:
    /* TODO: CLEANUP */
    return ret;
}

/********************************************
 * TxDMA
 * ******************************************/

#ifdef SIM
#define     DEBUG_DOL_TEST_TIMER_FULL_SET       1
#define     DEBUG_DOL_TEST_TIMER_FULL_RESET     0
#define     DEBUG_DOL_TEST_TIMER_NUM_KEY_LINES  2

static void
debug_dol_init_timer_full_area(int state)
{
    uint64_t timer_key_hbm_base_addr;
    uint8_t byte;
    uint64_t data[DEBUG_DOL_TEST_TIMER_NUM_KEY_LINES * 2 * 8];

    timer_key_hbm_base_addr = (uint64_t)asicpd_get_mem_addr((char *)JTIMERS);
#ifdef ELBA
    timer_key_hbm_base_addr += (DEBUG_DOL_TEST_TIMER_NUM_KEY_LINES *
                    ELBA_TIMER_NUM_KEY_PER_CACHE_LINE * 64);
#else
     timer_key_hbm_base_addr += (DEBUG_DOL_TEST_TIMER_NUM_KEY_LINES *
                     CAPRI_TIMER_NUM_KEY_PER_CACHE_LINE * 64);
#endif
    if (state == DEBUG_DOL_TEST_TIMER_FULL_SET) {
        byte = 0xff;
    } else {
        byte = 0;
    }

    memset(&data, byte, sizeof(data));

    p4plus_hbm_write(timer_key_hbm_base_addr, (uint8_t *)&data, sizeof(data),
            P4PLUS_CACHE_INVALIDATE_BOTH);
}

static void
debug_dol_test_timer_full(int state)
{
#ifdef ELBA
    if (state == DEBUG_DOL_TEST_TIMER_FULL_SET) {
        /*
         * Debug code to force num key_lines to 2
         */
        HAL_TRACE_DEBUG("setting num key_lines = 2");
        sdk::platform::elba::elba_txs_timer_init_hsh_depth(DEBUG_DOL_TEST_TIMER_NUM_KEY_LINES);
    } else {
        HAL_TRACE_DEBUG("resetting num key_lines back to default");
        sdk::platform::elba::elba_txs_timer_init_hsh_depth(ELBA_TIMER_NUM_KEY_CACHE_LINES);
    }
#else
    if (state == DEBUG_DOL_TEST_TIMER_FULL_SET) {
        /*
         * Debug code to force num key_lines to 2
         */
        HAL_TRACE_DEBUG("setting num key_lines = 2");
        sdk::platform::capri::capri_txs_timer_init_hsh_depth(DEBUG_DOL_TEST_TIMER_NUM_KEY_LINES);
    } else {
        HAL_TRACE_DEBUG("resetting num key_lines back to default");
        capri_txs_timer_init_hsh_depth(CAPRI_TIMER_NUM_KEY_CACHE_LINES);
        sdk::platform::capri::capri_txs_timer_init_hsh_depth(CAPRI_TIMER_NUM_KEY_CACHE_LINES);
    }
#endif

    debug_dol_init_timer_full_area(state);
}

hal_ret_t
p4pd_add_or_del_tcp_tx_test_timer_full(pd_tcpcb_t* tcpcb_pd, bool del)
{
    static tcpcb_hw_id_t debug_dol_timer_full_hw_id;
    tcpcb_hw_id_t hwid = tcpcb_pd->hw_id_qtype1;

    if (!debug_dol_timer_full_hw_id &&
            tcpcb_pd->tcpcb->debug_dol_tx & TCP_TX_DDOL_FORCE_TIMER_FULL) {
        debug_dol_test_timer_full(DEBUG_DOL_TEST_TIMER_FULL_SET);
        debug_dol_timer_full_hw_id = hwid;
    } else if (debug_dol_timer_full_hw_id &&
            !(tcpcb_pd->tcpcb->debug_dol_tx & TCP_TX_DDOL_FORCE_TIMER_FULL)) {
        debug_dol_test_timer_full(DEBUG_DOL_TEST_TIMER_FULL_RESET);
        debug_dol_timer_full_hw_id = 0;
    }

    return HAL_RET_OK;
}
#endif

hal_ret_t
p4pd_get_tcp_ooo_rx2tx_entry(pd_tcpcb_t* tcpcb_pd)
{
    s0_t0_ooq_tcp_tx_d             data = { 0 };
    hal_ret_t                      ret = HAL_RET_OK;

    // hardware index for this entry
    tcpcb_hw_id_t hwid = tcpcb_pd->hw_id_qtype1;

    if(sdk::asic::asic_mem_read(hwid,  (uint8_t *)&data, sizeof(data))){
        HAL_TRACE_ERR("Failed to get ooq tx: stage0 entry for TCP CB");
        return HAL_RET_HW_FAIL;
    }

    tcpcb_pd->tcpcb->ooq_rx2tx_pi = data.u.load_stage0_d.pi_0;
    tcpcb_pd->tcpcb->ooq_rx2tx_ci = data.u.load_stage0_d.ci_0;
    tcpcb_pd->tcpcb->window_update_pi = data.u.load_stage0_d.pi_1;
    tcpcb_pd->tcpcb->window_update_ci = data.u.load_stage0_d.ci_1;

    tcpcb_pd->tcpcb->ooo_rx2tx_qbase = htonll(data.u.load_stage0_d.ooo_rx2tx_qbase);

    return ret;
}

hal_ret_t
p4pd_get_tcp_tx_read_rx2tx_entry(pd_tcpcb_t* tcpcb_pd)
{
    s0_t0_tcp_tx_d          data = {0};
    uint16_t                debug_dol_tx;

    // hardware index for this entry
    tcpcb_hw_id_t hwid = tcpcb_pd->hw_id +
        (P4PD_TCPCB_STAGE_ENTRY_OFFSET * P4PD_HWID_TCP_TX_READ_RX2TX);

    if(sdk::asic::asic_mem_read(hwid,  (uint8_t *)&data, sizeof(data))){
        HAL_TRACE_ERR("Failed to get tx: read_rx2tx entry for TCP CB");
        return HAL_RET_HW_FAIL;
    }
    debug_dol_tx = ntohs(data.u.read_rx2tx_d.debug_dol_tx);
    if (debug_dol_tx & TCP_TX_DDOL_FORCE_TBL_SETADDR) {
        /*
         * DEBUG ONLY : read at a shifted offset
         */
        HAL_TRACE_DEBUG("reading rx2tx at an offset {}",
                TCP_DDOL_TBLADDR_SHIFT_OFFSET);
        hwid += TCP_DDOL_TBLADDR_SHIFT_OFFSET;
        if(sdk::asic::asic_mem_read(hwid,  (uint8_t *)&data, sizeof(data))){
            HAL_TRACE_ERR("Failed to get tx: read_rx2tx entry for TCP CB");
            return HAL_RET_HW_FAIL;
        }
    }
    tcpcb_pd->tcpcb->debug_dol_tx = debug_dol_tx;
    tcpcb_pd->tcpcb->debug_dol_tblsetaddr = data.u.read_rx2tx_d.debug_dol_tblsetaddr;

    return HAL_RET_OK;
}

hal_ret_t
p4pd_get_tcpcb_txdma_entry(pd_tcpcb_t* tcpcb_pd)
{
    hal_ret_t   ret = HAL_RET_OK;

    // qtype 1
    ret = p4pd_get_tcp_ooo_rx2tx_entry(tcpcb_pd);
    if(ret != HAL_RET_OK) {
        goto cleanup;
    }

    // qtype 0
    ret = p4pd_get_tcp_tx_read_rx2tx_entry(tcpcb_pd);
    if(ret != HAL_RET_OK) {
        goto cleanup;
    }

cleanup:
    return ret;
}

/**************************/

tcpcb_hw_id_t
pd_tcpcb_get_base_hw_index(pd_tcpcb_t* tcpcb_pd, uint32_t qtype)
{
    SDK_ASSERT(NULL != tcpcb_pd);
    SDK_ASSERT(NULL != tcpcb_pd->tcpcb);

    // Get the base address of TCP CB from LIF Manager.
    // Set qtype and qid as 0 to get the start offset.
    uint64_t offset = lif_manager()->get_lif_qstate_addr(SERVICE_LIF_TCP_PROXY, qtype,
                    tcpcb_pd->tcpcb->cb_id);
    HAL_TRACE_DEBUG("received offset {:#x}", offset);
    return offset;
}

uint64_t tcpcb_pd_read_notify_addr_get(uint32_t qid)
{
    uint64_t addr;

    addr = lif_manager()->get_lif_qstate_addr(SERVICE_LIF_TCP_PROXY, 0,
            qid) + (P4PD_TCPCB_STAGE_ENTRY_OFFSET * P4PD_HWID_TCP_TX_TCP_RETX);
    // offsetof does not work on bitfields
    // addr += offsetof(s3_t0_tcp_tx_retx_d, read_notify_bytes);
    addr += 0;

    return addr;
}

static void
p4pd_convert_hal_tcpcb_to_sdk_tcpcb(pd_tcpcb_t *hal_tcpcb_pd,
        tcpcb_pd_t *sdk_tcpcb)
{
    wring_hw_id_t ring_base;
    tcpcb_t *hal_tcpcb = hal_tcpcb_pd->tcpcb;

    sdk_tcpcb->qid = hal_tcpcb->cb_id;
    sdk_tcpcb->other_qid = hal_tcpcb->other_qid;
    if (hal_tcpcb->proxy_type == types::PROXY_TYPE_TCP) {
        sdk_tcpcb->app_qid = hal_tcpcb->other_qid;
        sdk_tcpcb->app_lif = SERVICE_LIF_TCP_PROXY;
        sdk_tcpcb->lg2_app_num_slots = ASIC_SESQ_RING_SLOTS_SHIFT;
        sdk_tcpcb->app_type = TCP_APP_TYPE_BYPASS;
        sdk_tcpcb->app_ring = TCP_SCHED_RING_SESQ;
        sdk_tcpcb->app_nde_shift = ASIC_SESQ_ENTRY_SIZE_SHIFT;
        sdk_tcpcb->app_nde_offset = ASIC_SESQ_DESC_OFFSET;
        wring_pd_get_base_addr(types::WRING_TYPE_SESQ,
                sdk_tcpcb->app_qid,
                &ring_base);
        sdk_tcpcb->serq_base = ring_base;
        sdk_tcpcb->gc_base_addr = lif_manager()->get_lif_qstate_addr(
                SERVICE_LIF_GC,
                ASIC_HBM_GC_RNMDR_QTYPE,
                ASIC_RNMDR_GC_TCP_RING_PRODUCER) + TCP_GC_CB_SW_PI_OFFSET;
        sdk_tcpcb->sesq_prod_ci_addr =
                tcpcb_pd_serq_prod_ci_addr_get(hal_tcpcb->other_qid);
        sdk_tcpcb->read_notify_addr = tcpcb_pd_read_notify_addr_get(hal_tcpcb->other_qid);
    } else {
        sdk_tcpcb->app_qid = hal_tcpcb->cb_id;
        sdk_tcpcb->app_lif = SERVICE_LIF_TLS_PROXY;
        sdk_tcpcb->lg2_app_num_slots = ASIC_SERQ_RING_SLOTS_SHIFT;
        sdk_tcpcb->app_type = TCP_APP_TYPE_TLS;
        sdk_tcpcb->app_ring = TLS_SCHED_RING_SERQ;
        sdk_tcpcb->app_nde_shift = ASIC_SERQ_ENTRY_SIZE_SHIFT;
        sdk_tcpcb->app_nde_offset = ASIC_SERQ_DESC_OFFSET;
        wring_pd_get_base_addr(types::WRING_TYPE_SERQ,
                sdk_tcpcb->app_qid,
                &ring_base);
        sdk_tcpcb->serq_base = ring_base;
        sdk_tcpcb->gc_base_addr = lif_manager()->get_lif_qstate_addr(
                SERVICE_LIF_GC,
                ASIC_HBM_GC_TNMDR_QTYPE,
                ASIC_RNMDR_GC_TCP_RING_PRODUCER) + TCP_GC_CB_SW_PI_OFFSET;
        sdk_tcpcb->sesq_prod_ci_addr =
                lif_manager()->get_lif_qstate_addr(SERVICE_LIF_TLS_PROXY, 0,
                hal_tcpcb->other_qid) + pd_tlscb_sesq_ci_offset_get();
        sdk_tcpcb->page_headroom = TLS_NIC_PAGE_HEADROOM;
    }
    sdk_tcpcb->rx_gc_base_addr = lif_manager()->get_lif_qstate_addr(
            SERVICE_LIF_GC,
            ASIC_HBM_GC_RNMDR_QTYPE,
            ASIC_RNMDR_GC_TCP_RX_RING_PRODUCER) + TCP_GC_CB_SW_PI_OFFSET;
    wring_pd_get_base_addr(types::WRING_TYPE_SESQ,
                sdk_tcpcb->qid,
                &ring_base);
    sdk_tcpcb->sesq_base = ring_base;
    sdk_tcpcb->base_pa[0] = hal_tcpcb_pd->hw_id;
    sdk_tcpcb->base_pa[1] = hal_tcpcb_pd->hw_id_qtype1;
    sdk_tcpcb->other_base_pa[0] = lif_manager()->get_lif_qstate_addr(
            SERVICE_LIF_TCP_PROXY, 0, hal_tcpcb->other_qid);
    sdk_tcpcb->stats_base = asicpd_get_mem_addr(ASIC_HBM_REG_TCP_PROXY_PER_FLOW_STATS) + 
        sdk_tcpcb->qid * TCP_PER_FLOW_STATS_SIZE + TCP_RX_PER_FLOW_STATS_OFFSET;
    wring_pd_get_base_addr(types::WRING_TYPE_TCP_OOO_RX2TX,
                sdk_tcpcb->qid,
                &ring_base);
    sdk_tcpcb->ooo_rx2tx_qbase = ring_base;
#ifdef ELBA
    sdk_tcpcb->ooo_rx2tx_free_pi_addr =
        asicpd_get_mem_addr(ASIC_HBM_REG_TLS_PROXY_PAD_TABLE) +
        ELBA_GC_GLOBAL_OOQ_TX2RX_FP_PI;
#else
    sdk_tcpcb->ooo_rx2tx_free_pi_addr =
        asicpd_get_mem_addr(ASIC_HBM_REG_TLS_PROXY_PAD_TABLE) +
        CAPRI_GC_GLOBAL_OOQ_TX2RX_FP_PI;
#endif
    sdk_tcpcb->ooo_prod_ci_addr = tcpcb_pd_ooo_rx2tx_prod_ci_addr_get(
            hal_tcpcb->cb_id);

    uint64_t prog_offset;
    sdk::p4::p4_program_label_to_offset("p4plus",
            (char *)"rxdma_stage0.bin", (char *)"tcp_rx_stage0", &prog_offset);
    sdk_tcpcb->rxdma_action_id = prog_offset >> CACHE_LINE_SIZE_SHIFT;
    sdk::p4::p4_program_label_to_offset("p4plus",
            (char *)"txdma_stage0.bin", (char *)"tcp_tx_stage0", &prog_offset);
    sdk_tcpcb->txdma_action_id = prog_offset >> CACHE_LINE_SIZE_SHIFT;
    sdk::p4::p4_program_label_to_offset("p4plus",
            (char *)"txdma_stage0.bin", (char *)"tcp_ooq_rx2tx_stage0",
            &prog_offset);
    sdk_tcpcb->txdma_ooq_action_id = prog_offset >> CACHE_LINE_SIZE_SHIFT;

    sdk_tcpcb->rcv_nxt = hal_tcpcb->rcv_nxt;
    sdk_tcpcb->debug_dol = hal_tcpcb->debug_dol;
    sdk_tcpcb->debug_dol_tx = hal_tcpcb->debug_dol_tx;
    sdk_tcpcb->snd_nxt = hal_tcpcb->snd_nxt;
    sdk_tcpcb->rcv_wup = hal_tcpcb->rcv_wup;
    sdk_tcpcb->rcv_wnd = hal_tcpcb->rcv_wnd;
    sdk_tcpcb->snd_una = hal_tcpcb->snd_una;
    sdk_tcpcb->pred_flags = hal_tcpcb->pred_flags;
    sdk_tcpcb->rcv_wscale = hal_tcpcb->rcv_wscale;
    sdk_tcpcb->ato = hal_tcpcb->ato;
    sdk_tcpcb->snd_wnd = hal_tcpcb->snd_wnd;
    sdk_tcpcb->rtt_seq_tsoffset = hal_tcpcb->rtt_seq_tsoffset;
    sdk_tcpcb->rtt_time = hal_tcpcb->rtt_time;
    sdk_tcpcb->ts_learned = hal_tcpcb->ts_learned;
    sdk_tcpcb->delay_ack = hal_tcpcb->delay_ack;
    sdk_tcpcb->ooo_queue = hal_tcpcb->ooo_queue;
    sdk_tcpcb->sack_perm = hal_tcpcb->sack_perm;
    sdk_tcpcb->timestamps = hal_tcpcb->timestamps;
    sdk_tcpcb->ecn = true;
    sdk_tcpcb->cc_algo = hal_tcpcb->cc_algo;
    sdk_tcpcb->smss = hal_tcpcb->smss;
    sdk_tcpcb->snd_cwnd = hal_tcpcb->snd_cwnd;
    sdk_tcpcb->snd_ssthresh = hal_tcpcb->snd_ssthresh;
    sdk_tcpcb->snd_wscale = hal_tcpcb->snd_wscale;
    sdk_tcpcb->abc_l_var = hal_tcpcb->abc_l_var;
    sdk_tcpcb->rcv_mss = hal_tcpcb->rcv_mss;
    sdk_tcpcb->initial_window = hal_tcpcb->initial_window;
    sdk_tcpcb->source_lif = hal_tcpcb->source_lif;
    sdk_tcpcb->source_port = hal_tcpcb->source_port;
    sdk_tcpcb->dest_port = hal_tcpcb->dest_port;
    sdk_tcpcb->header_len = hal_tcpcb->header_len;
    sdk_tcpcb->ts_offset = hal_tcpcb->ts_offset;
    sdk_tcpcb->ts_time = hal_tcpcb->ts_time;
    sdk_tcpcb->ts_recent = hal_tcpcb->ts_recent;
    memcpy(sdk_tcpcb->header_template, hal_tcpcb->header_template,
            sizeof(sdk_tcpcb->header_template));

    sdk_tcpcb->serq_ci = hal_tcpcb->serq_ci;
    sdk_tcpcb->serq_pi = hal_tcpcb->serq_pi;

    sdk_tcpcb->state = hal_tcpcb->state;
}

hal_ret_t
p4pd_add_or_del_tcpcb_entry(pd_tcpcb_t* tcpcb_pd, bool del)
{
    sdk_ret_t                   sdk_ret;
    tcpcb_pd_t                  sdk_tcpcb = { 0 };

    if (!del) {
        p4pd_convert_hal_tcpcb_to_sdk_tcpcb(tcpcb_pd, &sdk_tcpcb);
        sdk_ret = sdk::tcp::tcpcb_pd_create(&sdk_tcpcb);
        if (sdk_ret != SDK_RET_OK) {
            return HAL_RET_ERR;
        }
#ifdef SIM
        p4pd_add_or_del_tcp_tx_test_timer_full(tcpcb_pd, del);
#endif
    }

    return HAL_RET_OK;
}

static
hal_ret_t
p4pd_get_tcpcb_entry(pd_tcpcb_t* tcpcb_pd)
{
    hal_ret_t                   ret = HAL_RET_OK;
    sdk_ret_t                   sdk_ret;
    tcpcb_pd_t                  sdk_tcpcb = { 0 };
    tcpcb_stats_pd_t            sdk_stats_tcpcb = { 0 };
    
    sdk_tcpcb.qid = tcpcb_pd->tcpcb->cb_id;
    sdk_tcpcb.base_pa[0] = tcpcb_pd->hw_id;
    sdk_tcpcb.base_pa[1] = tcpcb_pd->hw_id_qtype1;
    sdk_ret = sdk::tcp::tcpcb_pd_get(&sdk_tcpcb);
    if (sdk_ret != SDK_RET_OK) {
        HAL_TRACE_ERR("Failed to get entry for tcpcb");
        return HAL_RET_ERR;
    }
    p4pd_convert_sdk_tcpcb_to_hal_tcpcb(&sdk_tcpcb, tcpcb_pd);

    sdk_stats_tcpcb.qid = tcpcb_pd->tcpcb->cb_id;
    sdk_stats_tcpcb.base_pa[0] = tcpcb_pd->hw_id;
    sdk_stats_tcpcb.base_pa[1] = tcpcb_pd->hw_id_qtype1;
    sdk_stats_tcpcb.stats_base = asicpd_get_mem_addr(ASIC_HBM_REG_TCP_PROXY_PER_FLOW_STATS) + 
        sdk_tcpcb.qid * TCP_PER_FLOW_STATS_SIZE + TCP_RX_PER_FLOW_STATS_OFFSET;
    sdk_ret = sdk::tcp::tcpcb_pd_get_stats(&sdk_stats_tcpcb);
    if (sdk_ret != SDK_RET_OK) {
        HAL_TRACE_ERR("Failed to get entry for tcpcb");
        return HAL_RET_ERR;
    }
    p4pd_convert_sdk_stats_tcpcb_to_hal_tcpcb(&sdk_stats_tcpcb, tcpcb_pd);

    ret = p4pd_get_tcpcb_rxdma_entry(tcpcb_pd);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to get rxdma entry for tcpcb");
        goto err;
    }

    ret = p4pd_get_tcpcb_txdma_entry(tcpcb_pd);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to get txdma entry for tcpcb");
        goto err;
    }

err:
    /*TODO: cleanup */
    return ret;
}

static hal_ret_t
p4pd_init_tcpcb_stats(pd_tcpcb_t* tcpcb_pd)
{
    tcpcb_hw_id_t hwid;
    tcp_rx_stats_t rx_stats = { 0 };
    tcp_tx_stats_t tx_stats = { 0 };

    hwid = asicpd_get_mem_addr(ASIC_HBM_REG_TCP_PROXY_PER_FLOW_STATS) +
                tcpcb_pd->tcpcb->cb_id * TCP_PER_FLOW_STATS_SIZE +
                TCP_RX_PER_FLOW_STATS_OFFSET;
    if (!p4plus_hbm_write(hwid, (uint8_t *)&rx_stats, sizeof(rx_stats),
                P4PLUS_CACHE_ACTION_NONE)) {
        HAL_TRACE_ERR("Failed to init rx_stats");
        return HAL_RET_HW_FAIL;
    }

    hwid = asicpd_get_mem_addr(ASIC_HBM_REG_TCP_PROXY_PER_FLOW_STATS) +
                tcpcb_pd->tcpcb->cb_id * TCP_PER_FLOW_STATS_SIZE +
                TCP_TX_PER_FLOW_STATS_OFFSET;
    if (!p4plus_hbm_write(hwid, (uint8_t *)&tx_stats, sizeof(tx_stats),
                P4PLUS_CACHE_ACTION_NONE)) {
        HAL_TRACE_ERR("Failed to init tx_stats");
        return HAL_RET_HW_FAIL;
    }

    return HAL_RET_OK;
}

/********************************************
 * APIs
 *******************************************/

hal_ret_t
pd_tcpcb_create (pd_func_args_t *pd_func_args)
{
    hal_ret_t               ret;
    pd_tcpcb_create_args_t *args = pd_func_args->pd_tcpcb_create;
    pd_tcpcb_s              *tcpcb_pd;

    HAL_TRACE_DEBUG("Creating pd state for TCP CB.");

    // allocate PD tcpcb state
    tcpcb_pd = tcpcb_pd_alloc_init();
    if (tcpcb_pd == NULL) {
        return HAL_RET_OOM;
    }
    HAL_TRACE_DEBUG("Alloc done");
    tcpcb_pd->tcpcb = args->tcpcb;
    // get hw-id for this TCPCB
    tcpcb_pd->hw_id = pd_tcpcb_get_base_hw_index(tcpcb_pd, TCP_TX_QTYPE);
    tcpcb_pd->tcpcb->cb_base = tcpcb_pd->hw_id;
    tcpcb_pd->hw_id_qtype1 = pd_tcpcb_get_base_hw_index(tcpcb_pd, TCP_OOO_RX2TX_QTYPE);
    tcpcb_pd->tcpcb->cb_base_qtype1 = tcpcb_pd->hw_id_qtype1;

    HAL_TRACE_DEBUG("Creating TCP CB at addr: 0x{:x} qid: {}",
            tcpcb_pd->hw_id, tcpcb_pd->tcpcb->cb_id);

    // program tcpcb
    ret = p4pd_add_or_del_tcpcb_entry(tcpcb_pd, false);
    if(ret != HAL_RET_OK) {
        goto cleanup;
    }

    ret = p4pd_init_tcpcb_stats(tcpcb_pd);

    HAL_TRACE_DEBUG("Programming done");
    // add to db
    ret = add_tcpcb_pd_to_db(tcpcb_pd);
    if (ret != HAL_RET_OK) {
       goto cleanup;
    }
    HAL_TRACE_DEBUG("DB add done");
    args->tcpcb->pd = tcpcb_pd;

    return HAL_RET_OK;

cleanup:

    if (tcpcb_pd) {
        tcpcb_pd_free(tcpcb_pd);
    }
    return ret;
}

hal_ret_t
pd_tcpcb_update (pd_func_args_t *pd_func_args)
{
    hal_ret_t               ret;
    pd_tcpcb_update_args_t *args = pd_func_args->pd_tcpcb_update;

    if(!args) {
       return HAL_RET_INVALID_ARG;
    }

    tcpcb_t*                tcpcb = args->tcpcb;
    pd_tcpcb_t*             tcpcb_pd = (pd_tcpcb_t*)tcpcb->pd;

    HAL_TRACE_DEBUG("TCPCB pd update");

    // program tcpcb
    ret = p4pd_add_or_del_tcpcb_entry(tcpcb_pd, false);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to update tcpcb");
    }
    return ret;
}

hal_ret_t
pd_tcpcb_delete (pd_func_args_t *pd_func_args)
{
    hal_ret_t               ret;
    pd_tcpcb_delete_args_t *args = pd_func_args->pd_tcpcb_delete;

    if(!args) {
       return HAL_RET_INVALID_ARG;
    }

    tcpcb_t*                tcpcb = args->tcpcb;
    pd_tcpcb_t*             tcpcb_pd = (pd_tcpcb_t*)tcpcb->pd;

    HAL_TRACE_DEBUG("TCPCB pd delete");

    // program tcpcb
    ret = p4pd_add_or_del_tcpcb_entry(tcpcb_pd, true);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to delete tcpcb entry");
    }

    del_tcpcb_pd_from_db(tcpcb_pd);

    tcpcb_pd_free(tcpcb_pd);

    return ret;
}

hal_ret_t
pd_tcpcb_get (pd_func_args_t *pd_func_args)
{
    hal_ret_t               ret;
    pd_tcpcb_get_args_t *args = pd_func_args->pd_tcpcb_get;
    pd_tcpcb_t              tcpcb_pd;

    HAL_TRACE_DEBUG("TCPCB pd get for id: {}", args->tcpcb->cb_id);

    // allocate PD tcpcb state
    tcpcb_pd_init(&tcpcb_pd);
    tcpcb_pd.tcpcb = args->tcpcb;

    // get hw-id for this TCPCB
    tcpcb_pd.hw_id = pd_tcpcb_get_base_hw_index(&tcpcb_pd, TCP_TX_QTYPE);
    tcpcb_pd.tcpcb->cb_base = tcpcb_pd.hw_id;
    tcpcb_pd.hw_id_qtype1 = pd_tcpcb_get_base_hw_index(&tcpcb_pd, TCP_OOO_RX2TX_QTYPE);
    tcpcb_pd.tcpcb->cb_base_qtype1 = tcpcb_pd.hw_id_qtype1;
    HAL_TRACE_DEBUG("Received hw-id {:#x}", tcpcb_pd.hw_id);

    // get hw tcpcb entry
    ret = p4pd_get_tcpcb_entry(&tcpcb_pd);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Get request failed for id: {:#x}", tcpcb_pd.tcpcb->cb_id);
    }
    return ret;
}

hal_ret_t
pd_tcp_global_stats_get (pd_func_args_t *pd_func_args)
{
    sdk_ret_t ret;

    sdk::types::mem_addr_t stats_mem_addr =
        asicpd_get_mem_addr(TCP_PROXY_STATS);
    HAL_TRACE_DEBUG("TCP global stats mem_addr: {:x}", stats_mem_addr);

    pd_tcp_global_stats_get_args_t *args = pd_func_args->pd_tcp_global_stats_get;

    ret = sdk::asic::asic_mem_read(stats_mem_addr, (uint8_t *)args,
                                   sizeof(pd_tcp_global_stats_get_args_t));
    return hal_sdk_ret_to_hal_ret(ret);
}

//Short term fix. Bypasses state
hal_ret_t
p4pd_update_sesq_ci_addr (uint32_t qid, uint64_t ci_addr)
{
    hal_ret_t               ret = HAL_RET_OK;
    s4_t0_tcp_tx_xmit_d     data = {0};
    uint64_t                addr = 0;

    addr = lif_manager()->get_lif_qstate_addr(SERVICE_LIF_TCP_PROXY, 0,
            qid) + (P4PD_TCPCB_STAGE_ENTRY_OFFSET * P4PD_HWID_TCP_TX_TCP_XMIT);

    if(sdk::asic::asic_mem_read(addr,  (uint8_t *)&data, sizeof(data))){
        HAL_TRACE_ERR("Failed to get rx: read_tx2rx entry for TCP CB");
        return HAL_RET_HW_FAIL;
    }

    data.sesq_ci_addr = htonll(ci_addr);

    HAL_TRACE_DEBUG("Updating sesq_ci_addr for for TCP CB qid: {:#x} "
                    "sesq_ci_addr: {:#x} TCPCB write addr: {:#x}",
                    qid, ci_addr, addr);

    if(!p4plus_hbm_write(addr,  (uint8_t *)&data, sizeof(data),
                P4PLUS_CACHE_INVALIDATE_BOTH)){
        HAL_TRACE_ERR("Failed to update sesq_ci_addr for for TCP CB qid: {:#x} "
                      "sesq_ci_addr: {:#x} TCPCB write addr: {:#x}",
                      qid, ci_addr, addr);
        ret = HAL_RET_HW_FAIL;
    }
    return ret;
}

hal_ret_t
tcpcb_pd_serq_lif_qtype_qstate_ring_set (uint32_t tcp_qid, uint32_t lif,
                                         uint32_t qtype, uint32_t qid,
                                         uint32_t ring)
{
    hal_ret_t               ret = HAL_RET_OK;
    uint64_t                addr = 0;
    s6_t0_tcp_rx_dma_d      rx_dma_d = { 0 };
    wring_hw_id_t           serq_base;

    addr = lif_manager()->get_lif_qstate_addr(SERVICE_LIF_TCP_PROXY, 0,
            tcp_qid) + (P4PD_TCPCB_STAGE_ENTRY_OFFSET * P4PD_HWID_TCP_RX_DMA);

    if(sdk::asic::asic_mem_read(addr,  (uint8_t *)&rx_dma_d, sizeof(rx_dma_d))) {
        HAL_TRACE_ERR("Failed to get rx: read_tx2rx entry for TCP CB");
        return HAL_RET_HW_FAIL;
    }
    ret = wring_pd_get_base_addr(types::WRING_TYPE_SERQ,
                                 tcp_qid, &serq_base);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to receive serq base for qid: {}", qid);
    } else {
        HAL_TRACE_DEBUG("Serq base: {:#x}", serq_base);
        rx_dma_d.serq_base = htonll(serq_base);
    }

    rx_dma_d.consumer_lif = htons(lif);
    rx_dma_d.consumer_qtype = qtype;
    rx_dma_d.consumer_qid = htons(qid);
    rx_dma_d.consumer_ring = ring;

    HAL_TRACE_DEBUG("Updating consumer lif: {:#x}, qtype: {:#x}, qid: {:#x}, "
                    "ring: {:#x} for TCP CB qid: {:#x} TCPCB write addr: {:#x}",
                    lif, qtype, qid, ring, tcp_qid, addr);

    if(!p4plus_hbm_write(addr,  (uint8_t *)&rx_dma_d, sizeof(rx_dma_d),
                P4PLUS_CACHE_INVALIDATE_BOTH)){
        HAL_TRACE_ERR("Failed to update consumer lif,qtype,qid for for TCP CB qid: {:#x} "
                      "TCPCB write addr: {:#x}",
                      tcp_qid, addr);
        ret = HAL_RET_HW_FAIL;
    }
    return ret;
}

}    // namespace pd
}    // namespace hal
