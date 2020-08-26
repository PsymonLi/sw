#ifndef __TCP_COMMON_H__
#define __TCP_COMMON_H__

// Fill common ipv4 header information
#define SET_COMMON_IP_HDR(_buf, _i) \
    _buf[_i++] = 0x08; \
    _buf[_i++] = 0x00; \
    _buf[_i++] = 0x45; \
    _buf[_i++] = 0x09; \
    _buf[_i++] = 0x00; \
    _buf[_i++] = 0x28; \
    _buf[_i++] = 0x00; \
    _buf[_i++] = 0x01; \
    _buf[_i++] = 0x00; \
    _buf[_i++] = 0x00; \
    _buf[_i++] = 0x40; \
    _buf[_i++] = 0x06; \
    _buf[_i++] = 0xfa; \
    _buf[_i++] = 0x71;

// Fill common ipv6 header information
#define SET_COMMON_IP6_HDR(_buf, _i) \
    _buf[_i++] = 0x86; \
    _buf[_i++] = 0xdd; \
    _buf[_i++] = 0x60; \
    _buf[_i++] = 0x01; \
    _buf[_i++] = 0x00; \
    _buf[_i++] = 0x00; \
    _buf[_i++] = 0x00; \
    _buf[_i++] = 0x28; \
    _buf[_i++] = 0x06; \
    _buf[_i++] = 0x40;

// MSBit/First bit at sesq_retx_ci_offset is used to indicate SESQ End Marker has been received on SESQ flag
#define SESQ_END_MARK_FLAG_BIT_SHIFT 15
#define SESQ_END_MARK_FLAG_BIT  (1 << SESQ_END_MARK_FLAG_BIT_SHIFT)

// NOTE : ** These offsets need to match the offsets in 
// p4pd_tcp_proxy_api.h **
#define TCP_TCB_RX2TX_SHARED_OFFSET     0

#define TCP_TCB_RX2TX_TX_CI_OFFSET \
                    (TCP_TCB_RX2TX_SHARED_OFFSET + 40)
#define TCP_TCB_RX2TX_RETX_CI_OFFSET \
                    (TCP_TCB_RX2TX_SHARED_OFFSET + 42)
#define TCP_TCB_RX2TX_RETX_PENDING_OFFSET \
                    (TCP_TCB_RX2TX_SHARED_OFFSET + 46)
#define TCP_TCB_RX2TX_ATO_OFFSET \
                    (TCP_TCB_RX2TX_SHARED_OFFSET + 48)
#define TCP_TCB_RX2TX_RTO_OFFSET \
                    (TCP_TCB_RX2TX_SHARED_OFFSET + 50)
#define TCP_TCB_RX2TX_KEEPA_TO_OFFSET \
                    (TCP_TCB_RX2TX_SHARED_OFFSET + 52)
#define TCP_TCB_RX2TX_TTYPERTO_TO_OFFSET \
                    (TCP_TCB_RX2TX_SHARED_OFFSET + 54)

#define TCP_TCB_TX2RX_SHARED_OFFSET     64
#define TCP_TCB_TX2RX_SHARED_WRITE_OFFSET \
                    (TCP_TCB_TX2RX_SHARED_OFFSET + 32)  // skip intrinsic part etc

#define TCP_TCB_RX2TX_SHARED_EXTRA_OFFSET     128
#define TCP_TCB_RX2TX_EXTRA_SND_CWND_OFFSET \
                    (TCP_TCB_RX2TX_SHARED_EXTRA_OFFSET + 0)

#define TCP_TCB_RX_OFFSET                     192
#define TCP_TCB_RX_OOQ_NOT_EMPTY \
                    (TCP_TCB_RX_OFFSET + 0)
#define TCP_TCB_RTT_OFFSET                    256

#define TCP_TCB_CC_OFFSET                     320
#define TCP_TCB_CC_SND_CWND_OFFSET \
                    (TCP_TCB_CC_OFFSET + 0)

#define TCP_TCB_RETX_OFFSET                   384
#define TCP_TCB_RETX_READ_NOTIFY_OFFSET \
                    (TCP_TCB_RETX_OFFSET + 0)
#define TCP_TCB_FC_OFFSET                     448
#define TCP_TCB_RX_DMA_OFFSET                 512
#define TCP_TCB_XMIT_OFFSET                   576
#define TCP_TCB_TSO_OFFSET                    640
#define TCP_TCB_HEADER_TEMPLATE_OFFSET        704
#define TCP_TCB_UNUSED1                       768
#define TCP_TCB_UNUSED2                       832
#define TCP_TCB_OOO_BOOK_KEEPING_OFFSET0      896

#define TCP_TCB_OOO_QADDR_OFFSET              960
// needs to match offset in p4pd_tcp_proxy_api.h
#define TCP_TCB_OOO_QADDR_CI_OFFSET           (TCP_TCB_OOO_QADDR_OFFSET + 0)

#define TCP_TCB_CLEANUP_BMAPS_BYTE_OFFSET       63
#define TCP_TCB_CLEANUP_BMAP_BIT_SHIFT          6

// TCP TCB cleanup cond1: SESQ cleanup completed until SESQ End Marker
#define TCP_CLEANUP_COND1_BIT   (1 << 0)

// TCP TCB cleanup cond2: The TCP state has moved to CLOSED
#define TCP_CLEANUP_COND2_BIT   (1 << 1)

#define TCP_TX_QTYPE                    0
#define TCP_OOO_RX2TX_QTYPE             1

#define TCP_SCHED_RING_SESQ             0x0
#define TCP_SCHED_RING_SEND_ACK         0x1
#define TCP_SCHED_RING_FAST_TIMER       0x2
#define TCP_SCHED_RING_DEL_ACK          0x3
#define TCP_SCHED_RING_ASESQ            0x4
#define TCP_SCHED_RING_PENDING_TX       0x5
#define TCP_SCHED_RING_FAST_RETRANS     0x6
#define TCP_SCHED_RING_CLEAN_RETX       0x7

#define TCP_TT_PERSIST                  1
#define TCP_TT_TW_FW2                   2

#define TCP_PROXY_TX_TOTAL_RINGS        8
#define TCP_PROXY_OOO_RX2TX_TOTAL_RINGS 5

#define TCP_OOO_RX2TX_RING0             0
#define TCP_WINDOW_UPDATE_EVENT_RING    1
#define TCP_CCQ_SESQ_CLEANED_IND_RING   2
#define TCP_CCQ_CONN_CLOSED_IND_RING    3
#define TCP_GEN_FEEDB_RST_RING          4

#define TCP_MAX_OOQ_DESCR_MASK 255

#define TCP_RX_PER_FLOW_STATS_SIZE       64
#define TCP_TX_PER_FLOW_STATS_SIZE       64
#define TCP_PER_FLOW_STATS_SIZE          (TCP_RX_PER_FLOW_STATS_SIZE + \
                                          TCP_TX_PER_FLOW_STATS_SIZE)
#define TCP_RX_PER_FLOW_STATS_OFFSET     0
#define TCP_TX_PER_FLOW_STATS_OFFSET     64

#define TCP_ESTABLISHED   1
#define TCP_SYN_SENT      2
#define TCP_SYN_RECV      3
#define TCP_FIN_WAIT1     4
#define TCP_FIN_WAIT2     5
#define TCP_TIME_WAIT     6
#define TCP_CLOSE         7
#define TCP_CLOSE_WAIT    8
#define TCP_LAST_ACK      9
#define TCP_LISTEN        10
#define TCP_CLOSING       11
#define TCP_NEW_SYN_RECV  12
/* We've received a rst */
#define TCP_RST           13
#define TCP_MAX_STATES    14 /* Leave at the end! */

#define TCP_PARSED_STATE_HANDLE_IN_CPU      0x01


#define TCP_STATE_MASK    0xF

#define TCP_ACTION_FIN    (1 << 7)

#define TCPF_ESTABLISHED  (1 << 1)
#define TCPF_SYN_SENT     (1 << 2)
#define TCPF_SYN_RECV     (1 << 3)
#define TCPF_FIN_WAIT1    (1 << 4)
#define TCPF_FIN_WAIT2    (1 << 5)
#define TCPF_TIME_WAIT    (1 << 6)
#define TCPF_CLOSE        (1 << 7)
#define TCPF_CLOSE_WAIT   (1 << 8)
#define TCPF_LAST_ACK     (1 << 9)
#define TCPF_LISTEN       (1 << 10)
#define TCPF_CLOSING      (1 << 11)
#define TCPF_NEW_SYN_RECV (1 << 12)

/* debug_dol encodings - need to match defines in tcp_proxy.py */
//#define TCP_DDOL_PKT_TO_SERQ            0x1 UNUSED
#define TCP_DDOL_TEST_ATOMIC_STATS      0x2
//#define TCP_DDOL_DONT_QUEUE_TO_SERQ     0x4 UNUSED
#define TCP_DDOL_LEAVE_IN_ARQ           0x8
#define TCP_DDOL_DONT_SEND_ACK          0x10
#define TCP_DDOL_DEL_ACK_TIMER          0x20
#define TCP_DDOL_PKT_TO_L7Q             0x40
#define TCP_DDOL_BYPASS_BARCO           0x80
#define TCP_DDOL_TSOPT_SUPPORT          0x0100
// Tx encodings
#define TCP_TX_DDOL_DONT_SEND_ACK           0x1
#define TCP_TX_DDOL_DONT_TX                 0x2
#define TCP_TX_DDOL_BYPASS_BARCO            0x4
#define TCP_TX_DDOL_START_DEL_ACK_TIMER     0x8
#define TCP_TX_DDOL_FORCE_TIMER_FULL        0x10
#define TCP_TX_DDOL_FORCE_TBL_SETADDR       0x20
#define TCP_TX_DDOL_TSOPT_SUPPORT           0x40
#define TCP_TX_DDOL_START_RETX_TIMER        0x80

// Tx endcoding bit position
#define TCP_TX_DDOL_DONT_SEND_ACK_BIT           0
#define TCP_TX_DDOL_DONT_TX_BIT                 1
#define TCP_TX_DDOL_BYPASS_BARCO_BIT            2
#define TCP_TX_DDOL_START_DEL_ACK_TIMER_BIT     3
#define TCP_TX_DDOL_FORCE_TIMER_FULL_BIT        4
#define TCP_TX_DDOL_FORCE_TBL_SETADDR_BIT       5
#define TCP_TX_DDOL_TSOPT_SUPPORT_BIT           6
#define TCP_TX_DDOL_START_RETX_TIMER_BIT        7

#define TCP_DDOL_TBLADDR_SHIFT_OFFSET   (64 * 15)
#define TCP_DDOL_TBLADDR_VALUE          0x59

#define TCP_GC_CB_SW_PI_OFFSET              32

#define TCP_TX_INVALID_SESQ_TX_CI           0xffff

// TCP CB CFG flags
#define TCP_OPER_FLAG_DELACK_BIT            0
#define TCP_OPER_FLAG_OOO_QUEUE_BIT         1
#define TCP_OPER_FLAG_KEEPALIVES_BIT        2

#define TCP_OPER_FLAG_DELACK                (1 << TCP_OPER_FLAG_DELACK_BIT)
#define TCP_OPER_FLAG_OOO_QUEUE             (1 << TCP_OPER_FLAG_OOO_QUEUE_BIT)
#define TCP_OPER_FLAG_KEEPALIVES            (1 << TCP_OPER_FLAG_KEEPALIVES_BIT)

// TCP CB Option CFG flags
#define TCP_OPT_FLAG_SACK_PERM_BIT          0
#define TCP_OPT_FLAG_TIMESTAMPS_BIT         1

#define TCP_OPT_FLAG_SACK_PERM              (1 << TCP_OPT_FLAG_SACK_PERM_BIT)
#define TCP_OPT_FLAG_TIMESTAMPS             (1 << TCP_OPT_FLAG_TIMESTAMPS_BIT)

#ifdef IRIS
// TCP App type
#define TCP_APP_TYPE_BYPASS                 1
#define TCP_APP_TYPE_TLS                    2
#define TCP_APP_TYPE_NVME                   3
#endif
#define TCP_QUICKACKS                       8

#ifdef HW
#define HW_TIMER 1
#endif

#ifdef HW_TIMER
// Timeout values
#define TCP_TIMER_TICK                      1000 // 1 ms 1000 us

#define TCP_ATO_USEC                        4000  // 4 ms
#define TCP_ATO_MIN                         4000  // 4 ms
#define TCP_ATO_TICKS                       (TCP_ATO_USEC / TCP_TIMER_TICK)

#define TCP_TICK_US                         1000           
#define TCP_RTO_MAX                         1000000  //1 s    
#define TCP_RTO_MIN                         200000   //200ms
#define TCP_RTO_MAX_TICK                    (TCP_RTO_MAX / TCP_TICK_US)
#define TCP_RTO_MIN_TICK                    (TCP_RTO_MIN / TCP_TICK_US)
#define TCP_MAX_PERSIST_TICK                ((500000) / TCP_TICK_US)  //500ms
#define TCP_DEFAULT_FW2_TICK                ((5000000) / TCP_TICK_US)  //5000ms
#define TCP_DEFAULT_TW_TICK                 ((5000000) / TCP_TICK_US)  //5000ms
#define TCP_DEFAULT_KEEPA_TICK              ((60000000) / TCP_TICK_US)  //60000ms
#define TCP_DEFAULT_KEEPA_RESET_TICK        (TCP_DEFAULT_KEEPA_TICK + 1) 
#define TCP_DEFAULT_KEEPA_COUNT             3    // count of expires (w/o response) before RST 
#define TCP_DIVS_TC_TS                      100  // ratio  between TS resolution, and timer_tick
#else

#define TCP_TIMER_TICK                      10 // 10 us
 
#define TCP_ATO_USEC                        100 // 100 us
#define TCP_ATO_MIN                         100 // 100 us
#define TCP_ATO_TICKS                       (TCP_ATO_USEC / TCP_TIMER_TICK)
 
#define TCP_RTO_MAX                         40000
#define TCP_RTO_MIN                         4000
#define TCP_RTO_MAX_TICK                    (TCP_RTO_MAX / TCP_TIMER_TICK)
#define TCP_RTO_MIN_TICK                    (TCP_RTO_MIN / TCP_TIMER_TICK)
#define TCP_MAX_PERSIST_TICK                ((50000) / TCP_TIMER_TICK)  //500ms
#define TCP_DEFAULT_FW2_TICK                ((500000) / TCP_TIMER_TICK)  //5000ms
#define TCP_DEFAULT_TW_TICK                 ((500000) / TCP_TIMER_TICK)  //5000ms
#define TCP_DEFAULT_KEEPA_TICK              ((60000) / TCP_TICK_US)  //6ms
#define TCP_DEFAULT_KEEPA_RESET_TICK        (TCP_DEFAULT_KEEPA_TICK + 1) 
#define TCP_DEFAULT_KEEPA_COUNT             3
#endif

#define TCP_MAX_RTO_COUNT                   30
/*
 * CC related
 */
#define TCP_MAX_WIN                         65535

// CC algo
#define TCP_CC_ALGO_NONE                    0
#define TCP_CC_ALGO_NEW_RENO                1
#define TCP_CC_ALGO_CUBIC                   2

// Feedback packet rx2tx
#define TCP_TX2RX_FEEDBACK_OOO_PKT          1
#define TCP_TX2RX_FEEDBACK_WIN_UPD          2
#define TCP_TX2RX_FEEDBACK_LAST_OOO_PKT     3
#define TCP_TX2RX_FEEDBACK_RST_PKT          4


// Global stats
#define TCP_PROXY_STATS_RNMDR_FULL              (0 << 3)
#define TCP_PROXY_STATS_INVALID_SESQ_DESCR      (1 << 3)
#define TCP_PROXY_STATS_INVALID_RETX_SESQ_DESCR (2 << 3)
#define TCP_PROXY_STATS_RETX_PARTIAL_PKT_ACK    (3 << 3)
#define TCP_PROXY_STATS_RETX_NOP_SCHEDULE       (4 << 3)
#define TCP_PROXY_STATS_GC_FULL                 (5 << 3)
#define TCP_PROXY_STATS_TLS_GC_FULL             (6 << 3)
#define TCP_PROXY_STATS_OOQ_FULL                (7 << 3)
#define TCP_PROXY_STATS_INVALID_NMDR_DESCR      (8 << 3)
#define TCP_PROXY_STATS_RCVD_CE_PKTS            (9 << 3)
#define TCP_PROXY_STATS_ECN_REDUCED_CONG        (10 << 3)
#define TCP_PROXY_STATS_RETX_PKTS               (11 << 3)
#define TCP_PROXY_STATS_OOQ_RX2TX_FULL          (12 << 3)
#define TCP_PROXY_STATS_RCVD_ACK_FOR_UNSENT     (13 << 3)
#define TCP_PROXY_STATS_FIN_SENT                (14 << 3)
#define TCP_PROXY_STATS_RST_SENT                (15 << 3)
#define TCP_PROXY_STATS_RCV_WIN_PROBE           (16 << 3)
#define TCP_PROXY_STATS_RCVD_KEEP_ALIVE         (17 << 3)
#define TCP_PROXY_STATS_RCVD_PKT_AFTER_WIN      (18 << 3)
#define TCP_PROXY_STATS_RCVD_PURE_WIN_UPD       (19 << 3)
#define TCP_PROXY_STATS_DUMMY1                  (20 << 3)
#define TCP_PROXY_STATS_DUMMY2                  (21 << 3)
#define TCP_PROXY_STATS_DUMMY3                  (22 << 3)

#define TCP_PROXY_STATS_DEBUG1                  (23 << 3)
#define TCP_PROXY_STATS_DEBUG2                  (24 << 3)
#define TCP_PROXY_STATS_DEBUG3                  (25 << 3)
#define TCP_PROXY_STATS_DEBUG4                  (26 << 3)
#define TCP_PROXY_STATS_DEBUG5                  (27 << 3)
#define TCP_PROXY_STATS_DEBUG6                  (28 << 3)
#define TCP_PROXY_STATS_DEBUG7                  (29 << 3)
#define TCP_PROXY_STATS_DEBUG8                  (30 << 3)
#define TCP_PROXY_STATS_DEBUG9                  (31 << 3)
#define TCP_PROXY_STATS_DEBUG10                 (32 << 3)

// TCP Global region offsets
#define TCP_PROXY_MEM_GC_OFFSET                 0
#define TCP_PROXY_MEM_OOO_RX2TX_FREE_PI_OFFSET  8

/*
 * TCP connection close reason
 * Passed from P4+ to ARM via ACTL msg
 */
#define CLOSE_REASON_NONE       0
#define ACTIVE_GRACEFUL         1
#define PASSIVE_GRACEFUL        2
#define LOCAL_RST               3
#define REMOTE_RST              4
#define LOCAL_RST_FW2_TO        5
#define INTERNAL_ERROR_START    LOCAL_RST_FW2_TO
#define LOCAL_RST_KA_TO         6
#define LOCAL_RST_RTO           7
#define NUM_CLOSE_RSN           8
#endif /* #ifndef __TCP_COMMON_H__ */
