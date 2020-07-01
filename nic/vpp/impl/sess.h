//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_SESS_H__
#define __VPP_IMPL_SESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct session_info_s {
    uint16_t tx_rewrite_flags;
    uint32_t tx_xlate_id;
    uint16_t tx_xlate_id2;
    uint16_t rx_rewrite_flags;
    uint32_t rx_xlate_id;
    uint16_t rx_xlate_id2;
    uint16_t meter_id;
    uint64_t timestamp;
    uint8_t session_tracking_en : 1;
    uint8_t drop : 1;
    uint8_t qid_en : 1;
    uint8_t qid;
} session_info_t;

typedef struct session_track_info_s {
    uint8_t iflow_tcp_state;
    uint32_t iflow_tcp_seq_num;
    uint32_t iflow_tcp_ack_num;
    uint16_t iflow_tcp_win_size;
    uint8_t iflow_tcp_win_scale;
    uint16_t iflow_tcp_mss;
    uint16_t iflow_tcp_exceptions;
    uint8_t iflow_tcp_win_scale_option_sent;
    uint8_t rflow_tcp_state;
    uint32_t rflow_tcp_seq_num;
    uint32_t rflow_tcp_ack_num;
    uint16_t rflow_tcp_win_size;
    uint8_t rflow_tcp_win_scale;
    uint16_t rflow_tcp_mss;
    uint16_t rflow_tcp_exceptions;
} session_track_info_t;

typedef struct pds_session_stats {
    uint64_t iflow_bytes_count;
    uint64_t iflow_packets_count;
    uint64_t rflow_bytes_count;
    uint64_t rflow_packets_count;
} pds_session_stats_t;

void set_skip_session_program(int val);

int initialize_session(void);

int session_track_program(uint32_t ses_id, void *action);

void session_insert(uint32_t ses_id, void *ses_info);

void session_get_addr(uint32_t ses_id, uint8_t **ses_addr,
                      uint32_t *entry_size);

int pds_session_program(uint32_t ses_id, void *action);

uint64_t pds_session_get_timestamp(uint32_t session_id);

void pds_session_get_session_state(uint32_t session_id, uint8_t *iflow_state,
                                   uint8_t *rflow_state);

void pds_session_get_info(uint32_t session_id, session_info_t *info);

bool pds_session_state_established(uint8_t state);

int session_track_program(uint32_t ses_id, void *action);

void pds_session_track_get_info(uint32_t session_id, session_track_info_t *info);

int pds_session_stats_read(uint32_t session_id, pds_session_stats_t *stats);

int pds_session_stats_clear(uint32_t session_id);

uint64_t pds_session_get_timestamp(uint32_t ses);

bool pds_session_get_xlate_ids(uint32_t ses, uint16_t *rx_xlate_id,
                               uint16_t *tx_xlate_id, uint16_t *rx_xlate_id2,
                               uint16_t *tx_xlate_id2);

int pds_session_update_rewrite_flags(uint32_t session_id, uint16_t tx_rewrite,
                                     uint16_t rx_rewrite);

#ifdef __cplusplus
}
#endif

#endif  // __VPP_IMPL_SESS_H__
