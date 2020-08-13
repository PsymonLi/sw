/*
 * Copyright (c) 2019, Pensando Systems Inc.
 */

#ifndef __COUNTER_DEFS_H__
#define __COUNTER_DEFS_H__

#define PXB_SAT_ITR_GENERATOR(mac) \
    mac(32f, CPL_ERR, ecrc,                0, 8) \
    mac(32f, CPL_ERR, rxbuf_ecc,           8, 8) \
    mac(32f, CPL_ERR, cpl_stat,           16, 8) \
    mac(32f, CPL_ERR, unexpected,         24, 8) \
    mac(32,  RDLAT0,                    x, 0, 0) \
    mac(32,  RDLAT1,                    x, 0, 0) \
    mac(32,  RDLAT2,                    x, 0, 0) \
    mac(32,  RDLAT3,                    x, 0, 0) \
    mac(32,  WRLAT,                     x, 0, 0) \
    mac(32f, REQ_ERR, unsupp_wr,           0, 8) \
    mac(32f, REQ_ERR, unsupp_rd,           8, 8) \
    mac(32f, REQ_ERR, pcihdrt_miss,       16, 8) \
    mac(32f, REQ_ERR, bus_master_dis,     24, 8) \
    mac(32,  REQ_PORTGATE,              x, 0, 0) \
    mac(32f, RSP_ERR, axi,                 0, 8) \
    mac(32f, RSP_ERR, cpl_timeout,         8, 8) \
    mac(32f, XFER_UNEXPECTED, wr256x,      0, 8) \
    mac(32f, XFER_UNEXPECTED, rd256x,      8, 8) \
    mac(32f, XFER_UNEXPECTED, wr_narrow,  16, 8) \
    mac(32f, XFER_UNEXPECTED, rd_narrow,  24, 8)

#define PXB_SAT_TGT_GENERATOR(mac) \
    mac(by_name, IND_REASON,            x, 0, 0) \
    mac(32f, RSP_ERR, ind_cnxt_mismatch,   0, 8) \
    mac(32f, RSP_ERR, rresp_err,           8, 8) \
    mac(32f, RSP_ERR, bresp_err,          16, 8) \
    mac(64f, RX_DROP, port0,               0, 8) \
    mac(64f, RX_DROP, port1,               8, 8) \
    mac(64f, RX_DROP, port2,              16, 8) \
    mac(64f, RX_DROP, port3,              24, 8) \
    mac(64f, RX_DROP, port4,              32, 8) \
    mac(64f, RX_DROP, port5,              40, 8) \
    mac(64f, RX_DROP, port6,              48, 8) \
    mac(64f, RX_DROP, port7,              56, 8)

#define PXB_CNT_ITR_GENERATOR(mac) \
    mac(64,  AXI_RD256,                 x, 0, 0) \
    mac(64,  AXI_RD64,                  x, 0, 0) \
    mac(64,  AXI_WR256,                 x, 0, 0) \
    mac(64,  AXI_WR64,                  x, 0, 0) \
    mac(32,  AXI_WR_ID_UID_OVERFLOW,    x, 0, 0) \
    mac(32,  RDCOMBINE_1K,              x, 0, 0) \
    mac(32,  RDCOMBINE_768,             x, 0, 0) \
    mac(32,  RDCOMBINE_512,             x, 0, 0) \
    mac(32,  RDCOMBINE_TMOUT,           x, 0, 0) \
    mac(64,  INTX_ASSERT_MSG,           x, 0, 0) \
    mac(64,  RDLAT_ACCUM0,              x, 0, 0) \
    mac(64,  RDLAT_ACCUM1,              x, 0, 0) \
    mac(64,  RDLAT_ACCUM2,              x, 0, 0) \
    mac(64,  RDLAT_ACCUM3,              x, 0, 0) \
    mac(64,  WRLAT_ACCUM,               x, 0, 0) \
    mac(64,  TOT_AXI_PAYLOAD_RD,        x, 0, 0) \
    mac(64,  TOT_AXI_PAYLOAD_WR,        x, 0, 0) \
    mac(64,  TOT_ATOMIC_REQ,            x, 0, 0) \
    mac(64,  TOT_AXI_RD,                x, 0, 0) \
    mac(64,  TOT_AXI_WR,                x, 0, 0) \
    mac(64,  TOT_MSG,                   x, 0, 0)

#define PXB_CNT_TGT_GENERATOR(mac) \
    mac(64,  AXI_RD64,                  x, 0, 0) \
    mac(64,  AXI_WR64,                  x, 0, 0) \
    mac(64,  DB32_AXI_WR,               x, 0, 0) \
    mac(64,  DB64_AXI_WR,               x, 0, 0) \
    mac(64,  DB_BYPASS,                 x, 0, 0) \
    mac(64,  WQE_BYPASS,                x, 0, 0) \
    mac(64,  RSP_CA_UR,                 x, 0, 0) \
    mac(64,  TOT_AXI_PAYLOAD_RD,        x, 0, 0) \
    mac(64,  TOT_AXI_PAYLOAD_WR,        x, 0, 0) \
    mac(64,  TOT_AXI_RD,                x, 0, 0) \
    mac(64,  TOT_AXI_WR,                x, 0, 0)

#define PP_SAT_PP_PIPE_GENERATOR(mac) \
    mac(32,  DECODE_DISPARITY_ERR,         x, 0, 0) \
    mac(32,  ELASTIC_BUFFER_OVERFLOW_ERR,  x, 0, 0) \
    mac(32,  ELASTIC_BUFFER_UNDERFLOW_ERR, x, 0, 0) \
    mac(32,  SKP_ADD,                      x, 0, 0) \
    mac(32,  SKP_REMOVE,                   x, 0, 0)

#define PP_PORT_C_CNT_C_GENERATOR(mac) \
    mac(32,  PORT_RX_CFG0_REQ,          x, 0, 0) \
    mac(64,  TL_RX_REQ_POSTED,          x, 0, 0) \
    mac(64,  TL_RX_REQ_NON_POSTED,      x, 0, 0) \
    mac(64,  TL_RX_CPL,                 x, 0, 0) \
    mac(32,  TL_RX_CPL_SPLIT,           x, 0, 0) \
    mac(64,  TL_RX_TOT_CPL_PAYLOAD,     x, 0, 0) \
    mac(64,  TL_RX_TOT_WR_PAYLOAD,      x, 0, 0) \
    mac(64,  TL_TX_TOT_WR_PAYLOAD,      x, 0, 0) \
    mac(64,  TL_TX_TOT_CPL_PAYLOAD,     x, 0, 0) \
    mac(64,  TL_TX_REQ_POSTED,          x, 0, 0) \
    mac(64,  TL_TX_REQ_NON_POSTED,      x, 0, 0) \
    mac(64,  TL_TX_CPL,                 x, 0, 0)

#define PP_PORT_C_SAT_C_PORT_CNT_GENERATOR(mac) \
    mac(32,  RX_PORTGATE,               x, 0, 0) \
    mac(32,  RX_WATCHDOG_NULLIFY,       x, 0, 0) \
    mac(32f, RX_EVENTS, nullify,           0, 8) \
    mac(32f, RX_EVENTS, tlrxerr_unsupp,    8, 8) \
    mac(32f, RX_EVENTS, rc_legacy_ints,   16,16) \
    mac(32f, RX_ERRS, framing,             0, 8) \
    mac(32f, RX_ERRS, ecrc,                8, 8) \
    mac(32f, RX_ERRS, rxbuf_ecc,          16, 8) \
    mac(32f, RX_ERRS, split_cpl_eoperr,   24, 8) \
    mac(32f, RX_TLP_ERRS, requests,        0, 8) \
    mac(32f, RX_TLP_ERRS, completions,     8, 8) \
    mac(32f, RX_TLP_ERRS, bar_miss,       16, 8) \
    mac(32f, RX_TLP_ERRS, unsupp,         24, 8) \
    mac(32f, TX_DROP, nullified,           0, 8) \
    mac(32f, TX_DROP, portgate,            8, 8) \
    mac(32,  TXBFR_OVERFLOW,            x, 0, 0)

#define PP_PORT_P_SAT_P_PORT_CNT_GENERATOR(mac) \
    mac(32,  8B10B_128B130B_SKP_OS_ERR, x, 0, 0) \
    mac(32,  CORE_INITIATED_RECOVERY,   x, 0, 0) \
    mac(32,  DESKEW_ERR,                x, 0, 0) \
    mac(32,  FC_TIMEOUT,                x, 0, 0) \
    mac(32,  FCPE,                      x, 0, 0) \
    mac(32,  LTSSM_STATE_CHANGED,       x, 0, 0) \
    mac(32,  PHYSTATUS_ERR,             x, 0, 0) \
    mac(32,  REPLAY_NUM_ERR,            x, 0, 0) \
    mac(32,  REPLAY_TIMER_ERR,          x, 0, 0) \
    mac(32,  RX_BAD_DLLP,               x, 0, 0) \
    mac(32,  RX_BAD_TLP,                x, 0, 0) \
    mac(32,  RX_NAK_RECEIVED,           x, 0, 0) \
    mac(32,  RX_NULLIFIED,              x, 0, 0) \
    mac(32,  RXBFR_OVERFLOW,            x, 0, 0) \
    mac(32,  RXBFR_MON,                 x, 0, 0) \
    mac(32,  TX_NAK_SENT,               x, 0, 0) \
    mac(32,  TXBUF_ECC_ERR,             x, 0, 0)

#define DB_WA_SAT_WA_GENERATOR(mac) \
    mac(32f, AXI_ERR, rresp,               0, 8) \
    mac(32f, AXI_ERR, bresp,               8, 8) \
    mac(32f, AXI_ERR, wqe_bresp,          16, 8) \
    mac(32,  HOST_ACCESS_ERR,           x, 0, 0) \
    mac(32,  MERGED_INFLIGHT,           x, 0, 0) \
    mac(32,  MERGED_PRE_AXI_READ,       x, 0, 0) \
    mac(32,  PID_CHKFAIL,               x, 0, 0) \
    mac(32,  WQE_DB_RINGADDR_CHKFAIL,   x, 0, 0) \
    mac(32,  WQE_DB_WQESIZE_CHKFAIL,    x, 0, 0) \
    mac(32,  WQE_DB_UPD_CHKFAIL,        x, 0, 0) \
    mac(32,  QADDR_CAM_CONFLICT,        x, 0, 0) \
    mac(32,  QID_OVERFLOW,              x, 0, 0) \
    mac(32,  RING_ACCESS_ERR,           x, 0, 0)

#endif /* __COUNTER_DEFS_H__ */
