/*
 * Copyright (c) 2018-2019, Pensando Systems Inc.
 */

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <endian.h>
#include <sstream>
#include <sys/time.h>

// Tell accel_dev.hpp to emumerate definitions of all devcmds
#define ACCEL_DEV_CMD_ENUMERATE  1

#include "nic/include/base.hpp"
#include "nic/hal/pd/capri/capri_barco_crypto.hpp"

#include "gen/proto/nicmgr/accel_metrics.pb.h"
#include "gen/proto/nicmgr/accel_metrics.delphi.hpp"

#include "nic/sdk/platform/misc/include/misc.h"
#include "nic/sdk/platform/intrutils/include/intrutils.h"
#include "platform/src/lib/pciemgr_if/include/pciemgr_if.hpp"
#include "platform/src/app/nicmgrd/src/delphic.hpp"

#include "logger.hpp"
#include "accel_dev.hpp"
#include "accel_lif.hpp"
#include "pd_client.hpp"
#include "adminq.hpp"


#define GBPS_TO_BYTES_PER_SEC(gbps)                     \
    ((uint64_t)(gbps) * (1000000000ULL / 8))

#define STORAGE_SEQ_RATE_LIMIT_GBPS_SCALE(gbps)         \
    (GBPS_TO_BYTES_PER_SEC(gbps) >> STORAGE_SEQ_RL_UNITS_SCALE_SHFT)

using namespace nicmgr;

// Amount of time to wait for sequencer queues to be quiesced
#define ACCEL_DEV_SEQ_QUEUES_QUIESCE_TIME_US    5000000
#define ACCEL_DEV_RING_OP_QUIESCE_TIME_US       1000000
#define ACCEL_DEV_ALL_RINGS_MAX_QUIESCE_TIME_US (10 * ACCEL_DEV_RING_OP_QUIESCE_TIME_US)

/*
 * rounded up log2
 */
static uint32_t
log_2(uint32_t x)
{
  uint32_t log = 0;

  while ((uint64_t)(1 << log) < (uint64_t)x) {
      log++;
  }
  return log;
}

static inline uint64_t
timestamp(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000 + tv.tv_usec);
}

static inline void
time_expiry_set(accel_timestamp_t& ts,
                uint64_t expiry)
{
    ts.timestamp = timestamp();
    ts.expiry = expiry;
}

static inline bool
time_expiry_check(const accel_timestamp_t& ts)
{
    return (ts.expiry == 0) ||
           ((timestamp() - ts.timestamp) > ts.expiry);
}

static std::vector<std::pair<const std::string,uint32_t>> accel_ring_vec = {
    {"cp",      ACCEL_RING_CP},
    {"cp_hot",  ACCEL_RING_CP_HOT},
    {"dc",      ACCEL_RING_DC},
    {"dc_hot",  ACCEL_RING_DC_HOT},
    {"xts0",    ACCEL_RING_XTS0},
    {"xts1",    ACCEL_RING_XTS1},
    {"gcm0",    ACCEL_RING_GCM0},
    {"gcm1",    ACCEL_RING_GCM1},
};

static const std::map<uint32_t,const char*> accel_ring_id2name_map = {
    {ACCEL_RING_CP,      "cp"},
    {ACCEL_RING_CP_HOT,  "cp_hot"},
    {ACCEL_RING_DC,      "dc"},
    {ACCEL_RING_DC_HOT,  "dc_hot"},
    {ACCEL_RING_XTS0,    "xts_enc"},
    {ACCEL_RING_XTS1,    "xts_dec"},
    {ACCEL_RING_GCM0,    "gcm_enc"},
    {ACCEL_RING_GCM1,    "gcm_dec"},
};

static const char              *lif_state_str_table[] = {
    ACCEL_LIF_STATE_STR_TABLE
};

static const char              *lif_event_str_table[] = {
    ACCEL_LIF_EVENT_STR_TABLE
};

static accel_lif_state_event_t  lif_initial_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_reject_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_CREATE,
        &AccelLif::accel_lif_create_action,
        ACCEL_LIF_ST_WAIT_HAL,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  lif_wait_hal_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_eagain_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_HAL_UP,
        &AccelLif::accel_lif_hal_up_action,
        ACCEL_LIF_ST_PRE_INIT,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_HANG_NOTIFY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  lif_pre_init_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_reject_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_INIT,
        &AccelLif::accel_lif_init_action,
        ACCEL_LIF_ST_POST_INIT,
    },
    {
        ACCEL_LIF_EV_RESET,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET_DESTROY,
        &AccelLif::accel_lif_ring_info_get_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_HANG_NOTIFY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  lif_post_init_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_reject_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET,
        &AccelLif::accel_lif_reset_action,
        ACCEL_LIF_ST_SEQ_QUEUE_RESET,
    },
    {
        ACCEL_LIF_EV_RESET_DESTROY,
        &AccelLif::accel_lif_reset_action,
        ACCEL_LIF_ST_SEQ_QUEUE_RESET,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_destroy_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_HANG_NOTIFY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  lif_seq_reset_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_reject_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET,
        &AccelLif::accel_lif_seq_quiesce_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET_DESTROY,
        &AccelLif::accel_lif_seq_quiesce_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_destroy_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_WAIT_SEQ_QUEUE_QUIESCE,
        &AccelLif::accel_lif_seq_quiesce_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_WAIT_RGROUP_QUIESCE,
        &AccelLif::accel_lif_rgroup_quiesce_action,
        ACCEL_LIF_ST_RGROUP_QUIESCE,
    },
    {
        ACCEL_LIF_EV_RGROUP_RESET,
        &AccelLif::accel_lif_rgroup_reset_action,
        ACCEL_LIF_ST_RGROUP_RESET,
    },
    {
        ACCEL_LIF_EV_HANG_NOTIFY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  lif_rgroup_quiesce_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_reject_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET,
        &AccelLif::accel_lif_rgroup_quiesce_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET_DESTROY,
        &AccelLif::accel_lif_rgroup_quiesce_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_destroy_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RGROUP_RESET,
        &AccelLif::accel_lif_rgroup_reset_action,
        ACCEL_LIF_ST_RGROUP_RESET,
    },
    {
        ACCEL_LIF_EV_HANG_NOTIFY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  lif_rgroup_reset_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_reject_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET,
        &AccelLif::accel_lif_rgroup_reset_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET_DESTROY,
        &AccelLif::accel_lif_rgroup_reset_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_destroy_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_PRE_INIT,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SEQ_QUEUE_PRE_INIT,
    },
    {
        ACCEL_LIF_EV_PRE_INIT,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_PRE_INIT,
    },
    {
        ACCEL_LIF_EV_HANG_NOTIFY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  lif_seq_pre_init_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_reject_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET_DESTROY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_destroy_action,
        ACCEL_LIF_ST_POST_INIT,
    },
    {
        ACCEL_LIF_EV_INIT,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_POST_INIT_POST_RESET,
    },
    {
        ACCEL_LIF_EV_ADMINQ_INIT,
        &AccelLif::accel_lif_adminq_init_action,
        ACCEL_LIF_ST_SEQ_QUEUE_INIT,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_INIT,
        &AccelLif::accel_lif_seq_queue_init_action,
        ACCEL_LIF_ST_SEQ_QUEUE_INIT,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_BATCH_INIT,
        &AccelLif::accel_lif_seq_queue_batch_init_action,
        ACCEL_LIF_ST_SEQ_QUEUE_INIT,
    },
    {
        ACCEL_LIF_EV_CRYPTO_KEY_UPDATE,
        &AccelLif::accel_lif_crypto_key_update_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_HANG_NOTIFY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  lif_post_init_post_reset_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_reject_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SEQ_QUEUE_PRE_INIT,
    },
    {
        ACCEL_LIF_EV_RESET_DESTROY,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_SEQ_QUEUE_PRE_INIT,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_destroy_action,
        ACCEL_LIF_ST_POST_INIT,
    },
    {
        ACCEL_LIF_EV_HANG_NOTIFY,
        &AccelLif::accel_lif_hang_notify_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  lif_seq_init_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_reject_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET,
        &AccelLif::accel_lif_reset_action,
        ACCEL_LIF_ST_SEQ_QUEUE_RESET,
    },
    {
        ACCEL_LIF_EV_RESET_DESTROY,
        &AccelLif::accel_lif_reset_action,
        ACCEL_LIF_ST_SEQ_QUEUE_RESET,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_destroy_action,
        ACCEL_LIF_ST_POST_INIT,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_INIT,
        &AccelLif::accel_lif_seq_queue_init_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_BATCH_INIT,
        &AccelLif::accel_lif_seq_queue_batch_init_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_ENABLE,
        &AccelLif::accel_lif_seq_queue_control_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_DISABLE,
        &AccelLif::accel_lif_seq_queue_control_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_BATCH_ENABLE,
        &AccelLif::accel_lif_seq_queue_batch_control_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_BATCH_DISABLE,
        &AccelLif::accel_lif_seq_queue_batch_control_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_INIT_COMPLETE,
        &AccelLif::accel_lif_null_action,
        ACCEL_LIF_ST_IDLE,
    },
    {
        ACCEL_LIF_EV_CRYPTO_KEY_UPDATE,
        &AccelLif::accel_lif_crypto_key_update_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_HANG_NOTIFY,
        &AccelLif::accel_lif_hang_notify_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  lif_idle_ev_table[] = {
    {
        ACCEL_LIF_EV_ANY,
        &AccelLif::accel_lif_reject_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_RESET,
        &AccelLif::accel_lif_reset_action,
        ACCEL_LIF_ST_SEQ_QUEUE_RESET,
    },
    {
        ACCEL_LIF_EV_RESET_DESTROY,
        &AccelLif::accel_lif_reset_action,
        ACCEL_LIF_ST_SEQ_QUEUE_RESET,
    },
    {
        ACCEL_LIF_EV_DESTROY,
        &AccelLif::accel_lif_destroy_action,
        ACCEL_LIF_ST_POST_INIT,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_ENABLE,
        &AccelLif::accel_lif_seq_queue_control_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_DISABLE,
        &AccelLif::accel_lif_seq_queue_control_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_BATCH_ENABLE,
        &AccelLif::accel_lif_seq_queue_batch_control_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_SEQ_QUEUE_BATCH_DISABLE,
        &AccelLif::accel_lif_seq_queue_batch_control_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_CRYPTO_KEY_UPDATE,
        &AccelLif::accel_lif_crypto_key_update_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_HANG_NOTIFY,
        &AccelLif::accel_lif_hang_notify_action,
        ACCEL_LIF_ST_SAME,
    },
    {
        ACCEL_LIF_EV_NULL
    },
};

static accel_lif_state_event_t  *lif_fsm_table[ACCEL_LIF_ST_MAX] = {
    [ACCEL_LIF_ST_INITIAL]              = lif_initial_ev_table,
    [ACCEL_LIF_ST_WAIT_HAL]             = lif_wait_hal_ev_table,
    [ACCEL_LIF_ST_PRE_INIT]             = lif_pre_init_ev_table,
    [ACCEL_LIF_ST_POST_INIT]            = lif_post_init_ev_table,
    [ACCEL_LIF_ST_SEQ_QUEUE_RESET]      = lif_seq_reset_ev_table,
    [ACCEL_LIF_ST_RGROUP_QUIESCE]       = lif_rgroup_quiesce_ev_table,
    [ACCEL_LIF_ST_RGROUP_RESET]         = lif_rgroup_reset_ev_table,
    [ACCEL_LIF_ST_POST_INIT_POST_RESET] = lif_post_init_post_reset_ev_table,
    [ACCEL_LIF_ST_SEQ_QUEUE_PRE_INIT]   = lif_seq_pre_init_ev_table,
    [ACCEL_LIF_ST_SEQ_QUEUE_INIT]       = lif_seq_init_ev_table,
    [ACCEL_LIF_ST_IDLE]                 = lif_idle_ev_table,
};

static accel_lif_ordered_event_t lif_ordered_ev_table[ACCEL_LIF_ST_MAX][ACCEL_LIF_EV_MAX];

static bool accel_lif_fsm_verbose;

static void accel_lif_state_machine_build(void);
static const char *lif_state_str(accel_lif_state_t state);
static const char *lif_event_str(accel_lif_event_t event);

/*
 * devcmd opcodes to state machine events
 */
typedef std::map<uint32_t,accel_lif_event_t> opcode2event_map_t;

static const opcode2event_map_t opcode2event_map = {
    {CMD_OPCODE_NOP,                     ACCEL_LIF_EV_NULL},
    {CMD_OPCODE_LIF_INIT,                ACCEL_LIF_EV_INIT},
    {CMD_OPCODE_LIF_RESET,               ACCEL_LIF_EV_RESET},
    {CMD_OPCODE_ADMINQ_INIT,             ACCEL_LIF_EV_ADMINQ_INIT},
    {CMD_OPCODE_SEQ_QUEUE_INIT,          ACCEL_LIF_EV_SEQ_QUEUE_INIT},
    {CMD_OPCODE_SEQ_QUEUE_BATCH_INIT,    ACCEL_LIF_EV_SEQ_QUEUE_BATCH_INIT},
    {CMD_OPCODE_SEQ_QUEUE_ENABLE,        ACCEL_LIF_EV_SEQ_QUEUE_ENABLE},
    {CMD_OPCODE_SEQ_QUEUE_DISABLE,       ACCEL_LIF_EV_SEQ_QUEUE_DISABLE},
    {CMD_OPCODE_SEQ_QUEUE_BATCH_ENABLE,  ACCEL_LIF_EV_SEQ_QUEUE_BATCH_ENABLE},
    {CMD_OPCODE_SEQ_QUEUE_BATCH_DISABLE, ACCEL_LIF_EV_SEQ_QUEUE_BATCH_DISABLE},
    {CMD_OPCODE_SEQ_QUEUE_INIT_COMPLETE, ACCEL_LIF_EV_SEQ_QUEUE_INIT_COMPLETE},
    {CMD_OPCODE_SEQ_QUEUE_DUMP,          ACCEL_LIF_EV_NULL},
    {CMD_OPCODE_CRYPTO_KEY_UPDATE,       ACCEL_LIF_EV_CRYPTO_KEY_UPDATE},
    {CMD_OPCODE_HANG_NOTIFY,             ACCEL_LIF_EV_HANG_NOTIFY},
};

#define ACCEL_LIF_FSM_LOG()                                                     \
    NIC_LOG_DEBUG("{}: state {} event {}: ",                                    \
                  LifNameGet(),                                                 \
                  lif_state_str(fsm_ctx.enter_state), lif_event_str(event))

#define ACCEL_LIF_FSM_VERBOSE_LOG()                                             \
    if (accel_lif_fsm_verbose) ACCEL_LIF_FSM_LOG()

#define ACCEL_LIF_FSM_ERR_LOG()                                                 \
    NIC_LOG_ERR("{}: state {} invalid event {}: ",                              \
                LifNameGet(),                                                   \
                lif_state_str(fsm_ctx.enter_state), lif_event_str(event))

static inline const char *
accel_ring_id2name_get(uint32_t ring_handle)
{
    auto iter = accel_ring_id2name_map.find(ring_handle);
    if (iter != accel_ring_id2name_map.end()) {
        return iter->second;
    }
    return "unknown";
}

/*
 * Common accel ring group API return error checking
 */
#define ACCEL_RGROUP_GET_CB_HANDLE_CHECK(_handle)                               \
    do {                                                                        \
        if (_handle >= ACCEL_RING_ID_MAX) {                                     \
            NIC_LOG_ERR("{}: unrecognized ring_handle {}",                      \
                        lif->LifNameGet(), _handle);                            \
        }                                                                       \
    } while (false)

#define ACCEL_RGROUP_GET_RET_CHECK(_ret_val, _num_rings, _name)                 \
    do {                                                                        \
        if (_ret_val != SDK_RET_OK) {                                           \
            NIC_LOG_ERR("{}: failed to get " _name " for ring group",           \
                        LifNameGet());                                          \
            return _ret_val;                                                    \
        }                                                                       \
        if (_num_rings < accel_ring_vec.size()) {                               \
            NIC_LOG_ERR("{}: too few num_rings {} expected at least {}",        \
                        LifNameGet(), _num_rings, accel_ring_vec.size());       \
            return HAL_RET_ERR;                                                 \
        }                                                                       \
    } while (false)


AccelLif::AccelLif(AccelDev& accel_dev,
                   accel_lif_res_t& lif_res) :
    accel_dev(accel_dev),
    spec(accel_dev.DevSpecGet()),
    pd(accel_dev.PdClientGet()),
    dev_api(accel_dev.DevApiGet()),
    seq_created_count(0),
    seq_qid_init_high(0)
{
    accel_lif_state_machine_build();

    memset(&hal_lif_info_, 0, sizeof(hal_lif_info_));
    hal_lif_info_.lif_id = lif_res.lif_id;
    lif_name = spec->name + std::string("/lif") +
               std::to_string(hal_lif_info_.lif_id);
    strncpy0(hal_lif_info_.name, lif_name.c_str(), sizeof(hal_lif_info_.name));
    intr_base = lif_res.intr_base;

    /*
     * If multiple accel LIFs are supported in the future, make an
     * allocator for allocating PAL memory for qinfo/rmetrics below.
     */
    cmb_qinfo_addr = pd->mp_->start_addr(STORAGE_QINFO_HBM_HANDLE);
    cmb_qinfo_size = pd->mp_->size(STORAGE_QINFO_HBM_HANDLE);
    if ((cmb_qinfo_addr == INVALID_MEM_ADDRESS) || (cmb_qinfo_size == 0)) {
        NIC_LOG_ERR("{}: Failed to get HBM memory for {}",
                    LifNameGet(), STORAGE_QINFO_HBM_HANDLE);
        throw;
    }

    NIC_LOG_DEBUG("{}: cmb_qinfo_addr: {:#x} cmb_qinfo_size: {}",
                  LifNameGet(), cmb_qinfo_addr, cmb_qinfo_size);

    cmb_rmetrics_addr = pd->mp_->start_addr(STORAGE_RMETRICS_HBM_HANDLE);
    cmb_rmetrics_size = pd->mp_->size(STORAGE_RMETRICS_HBM_HANDLE);
    if ((cmb_rmetrics_addr == INVALID_MEM_ADDRESS) || (cmb_rmetrics_size == 0)) {
        NIC_LOG_ERR("{}: Failed to get HBM memory for {}",
                    LifNameGet(), STORAGE_RMETRICS_HBM_HANDLE);
        throw;
    }

    NIC_LOG_DEBUG("{}: cmb_rmetrics_addr: {:#x} cmb_rmetrics_size: {}",
                  LifNameGet(), cmb_rmetrics_addr, cmb_rmetrics_size);

    memset(&fsm_ctx, 0, sizeof(fsm_ctx));
    fsm_ctx.state = ACCEL_LIF_ST_INITIAL;
    accel_lif_state_machine(ACCEL_LIF_EV_CREATE);
}

AccelLif::~AccelLif()
{
    /*
     * Host driver would have already performed graceful reset-destroy, in
     * which case, the following FSM event would result in very quick work,
     * i.e., no delay.
     */
    fsm_ctx.devcmd.status = ACCEL_RC_EAGAIN;
    while (fsm_ctx.devcmd.status == ACCEL_RC_EAGAIN) {
        accel_lif_state_machine(ACCEL_LIF_EV_DESTROY);
    }
}

void
AccelLif::SetHalClient(devapi *dapi)
{
    dev_api = dapi;
}

void
AccelLif::HalEventHandler(bool status)
{
    if (status) {
        accel_lif_state_machine(ACCEL_LIF_EV_HAL_UP);
    }
}

void
AdminCmdHandler(void *obj,
                void *req,
                void *req_data,
                void *resp,
                void *resp_data)
{
    AccelLif *lif = (AccelLif *)obj;
    lif->CmdHandler(req, req_data, resp, resp_data);
}

accel_status_code_t
AccelLif::CmdHandler(void *req,
                     void *req_data,
                     void *resp,
                     void *resp_data)
{
    dev_cmd_t           *cmd = (dev_cmd_t *)req;
    dev_cmd_cpl_t       *cpl = (dev_cmd_cpl_t *)resp;
    accel_lif_event_t   event;

    NIC_LOG_DEBUG("{}: Handling cmd: {}", LifNameGet(),
                  accel_dev_opcode_str(cmd->cmd.opcode));
    fsm_ctx.devcmd.req = req;
    fsm_ctx.devcmd.req_data = req_data;
    fsm_ctx.devcmd.resp = resp;
    fsm_ctx.devcmd.resp_data = resp_data;
    fsm_ctx.devcmd.status = ACCEL_RC_SUCCESS;

    auto iter = opcode2event_map.find(cmd->cmd.opcode);
    if (iter != opcode2event_map.end()) {
        event = iter->second;
        if (event != ACCEL_LIF_EV_NULL) {
            accel_lif_state_machine(event);
        }
    } else {
        NIC_LOG_ERR("{}: Unknown Opcode {}", LifNameGet(), cmd->cmd.opcode);
        fsm_ctx.devcmd.status = ACCEL_RC_EOPCODE;
    }

    cpl->cpl.status = fsm_ctx.devcmd.status;
    cpl->cpl.rsvd = 0xff;

    NIC_LOG_DEBUG("{}: Done cmd: {}, status: {}", LifNameGet(),
                  accel_dev_opcode_str(cmd->cmd.opcode), fsm_ctx.devcmd.status);
    return fsm_ctx.devcmd.status;
}

accel_status_code_t
AccelLif::reset(bool destroy)
{
    fsm_ctx.devcmd.status = ACCEL_RC_SUCCESS;
    accel_lif_state_machine(destroy ? ACCEL_LIF_EV_RESET_DESTROY :
                                      ACCEL_LIF_EV_RESET);
    return fsm_ctx.devcmd.status;
}

/*
 * Find or create an accelerator ring group ring
 */
accel_rgroup_ring_t&
AccelLif::accel_rgroup_find_create(uint32_t ring_handle,
                                   uint32_t sub_ring)
{
    accel_rgroup_ring_key_t         key;
    accel_metrics::AccelHwRingKey   delphi_key;

    key = accel_rgroup_ring_key_make(ring_handle, sub_ring);
    auto iter = rgroup_map.find(key);
    if (iter != rgroup_map.end()) {
        return iter->second;
    }

    accel_rgroup_ring_t& rgroup_ring = rgroup_map[key];
    if (g_nicmgr_svc) {
        delphi_key.set_rid(accel_ring_id2name_get(ring_handle));
        delphi_key.set_subrid(std::to_string(sub_ring));
        rgroup_ring.delphi_metrics = delphi::objects::AccelHwRingMetrics::
                           NewAccelHwRingMetrics(delphi_key,
                              rmetrics_addr_get(ring_handle, sub_ring));
    }
    return rgroup_ring;
}

/*
 * LIF State Machine Actions
 */
accel_lif_event_t
AccelLif::accel_lif_null_action(accel_lif_event_t event)
{
    ACCEL_LIF_FSM_LOG();
    fsm_ctx.devcmd.status = ACCEL_RC_SUCCESS;
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_eagain_action(accel_lif_event_t event)
{
    ACCEL_LIF_FSM_LOG();
    fsm_ctx.devcmd.status = ACCEL_RC_EAGAIN;
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_reject_action(accel_lif_event_t event)
{
    ACCEL_LIF_FSM_ERR_LOG();
    fsm_ctx.devcmd.status = ACCEL_RC_EPERM;
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_create_action(accel_lif_event_t event)
{
    ACCEL_LIF_FSM_LOG();
    memset(qinfo, 0, sizeof(qinfo));

    qinfo[STORAGE_SEQ_QTYPE_SQ] = {
        .type_num = STORAGE_SEQ_QTYPE_SQ,
        .size = HW_CB_MULTIPLE(STORAGE_SEQ_CB_SIZE_SHFT),
        .entries = log_2(spec->seq_queue_count),
    };

    qinfo[STORAGE_SEQ_QTYPE_UNUSED] = {
        .type_num = STORAGE_SEQ_QTYPE_UNUSED,
        .size = HW_CB_MIN_MULTIPLE,
        .entries = 0,
    };

    qinfo[STORAGE_SEQ_QTYPE_ADMIN] = {
        .type_num = STORAGE_SEQ_QTYPE_ADMIN,
        .size = HW_CB_MULTIPLE(ADMIN_QSTATE_SIZE_SHFT),
        .entries = 2,
    };

    seq_created_count = spec->seq_queue_count;

    hal_lif_info_.type = sdk::platform::LIF_TYPE_NONE;
    hal_lif_info_.rx_limit_bytes =
            STORAGE_SEQ_RATE_LIMIT_GBPS_SCALE(spec->rx_limit_gbps);
    hal_lif_info_.rx_burst_bytes =
            STORAGE_SEQ_RATE_LIMIT_GBPS_SCALE(spec->rx_burst_gb);
    hal_lif_info_.tx_limit_bytes =
            STORAGE_SEQ_RATE_LIMIT_GBPS_SCALE(spec->tx_limit_gbps);
    hal_lif_info_.tx_burst_bytes =
            STORAGE_SEQ_RATE_LIMIT_GBPS_SCALE(spec->tx_burst_gb);
    NIC_LOG_DEBUG("{}: rx_limit_gbps {} rx_burst_gb {} "
                  "tx_limit_gbps {} tx_burst_gb {}", LifNameGet(),
                  spec->rx_limit_gbps, spec->rx_burst_gb,
                  spec->tx_limit_gbps, spec->tx_burst_gb);
    memcpy(hal_lif_info_.queue_info, qinfo, sizeof(hal_lif_info_.queue_info));

    adminq = new AdminQ(LifNameGet().c_str(),
                        pd, LifIdGet(),
                        ACCEL_ADMINQ_REQ_QTYPE, ACCEL_ADMINQ_REQ_QID,
                        ACCEL_ADMINQ_REQ_RING_SIZE, ACCEL_ADMINQ_RESP_QTYPE,
                        ACCEL_ADMINQ_RESP_QID, ACCEL_ADMINQ_RESP_RING_SIZE,
                        AdminCmdHandler, this);
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_destroy_action(accel_lif_event_t event)
{
    ACCEL_LIF_FSM_LOG();
    fsm_ctx.devcmd.status = ACCEL_RC_SUCCESS;
    return ACCEL_LIF_EV_RESET_DESTROY;
}

accel_lif_event_t
AccelLif::accel_lif_hal_up_action(accel_lif_event_t event)
{
    ACCEL_LIF_FSM_LOG();
    cosA = 1;
    cosB = 0;
    ctl_cosA = 1;
    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        fsm_ctx.devcmd.status = ACCEL_RC_ERROR;
        return ACCEL_LIF_EV_NULL;
    }
    dev_api->qos_get_txtc_cos("CONTROL", 1, &ctl_cosB);
    if (ctl_cosB < 0) {
        NIC_LOG_ERR("{}: Failed to get cosB for group {}, uplink {}",
                    LifNameGet(), "CONTROL", 1);
        ctl_cosB = 0;
        fsm_ctx.devcmd.status = ACCEL_RC_ERROR;
    }
    return ACCEL_LIF_EV_NULL;
}


accel_lif_event_t
AccelLif::accel_lif_ring_info_get_action(accel_lif_event_t event)
{
    ACCEL_LIF_FSM_LOG();

    // acquire rings info as initialized by HAL
    if (rgroup_map.empty()) {
        if (accel_ring_info_get_all() != SDK_RET_OK) {
            NIC_LOG_ERR("{}: failed to acquire ring group info",
                        LifNameGet());
            fsm_ctx.devcmd.status = ACCEL_RC_ERROR;
        }
    }
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_init_action(accel_lif_event_t event)
{
    ACCEL_LIF_FSM_LOG();

    fsm_ctx.reset_destroy = false;
    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        fsm_ctx.devcmd.status = ACCEL_RC_ERROR;
        return ACCEL_LIF_EV_NULL;
    }
    if (dev_api->lif_create(&hal_lif_info_) != SDK_RET_OK) {
        NIC_LOG_ERR("{}: Failed to create LIF", LifNameGet());
        fsm_ctx.devcmd.status = ACCEL_RC_ERROR;
    }

    // program the queue state
    pd->program_qstate((struct queue_info*)hal_lif_info_.queue_info,
                       &hal_lif_info_, 0x0);
    NIC_LOG_INFO("{}: created", LifNameGet());

    // establish sequencer queues metrics with Delphi
    if (qmetrics_init()) {
        NIC_LOG_ERR("{}: Failed to establish qmetrics", LifNameGet());
        fsm_ctx.devcmd.status = ACCEL_RC_ERROR;
    }

    // Initialize AdminQ service
    if (!adminq->Init(0, ctl_cosA, ctl_cosB)) {
        NIC_LOG_ERR("{}: Failed to initialize AdminQ service",
                    LifNameGet());
        fsm_ctx.devcmd.status = ACCEL_RC_ERROR;
    }
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_reset_action(accel_lif_event_t event)
{
    int64_t                 qstate_addr;
    uint32_t                qid;
    uint8_t                 enable = 0;
    uint8_t                 abort = 1;

    ACCEL_LIF_FSM_LOG();
    fsm_ctx.reset_destroy = (event == ACCEL_LIF_EV_RESET_DESTROY) ||
                            (event == ACCEL_LIF_EV_DESTROY);
    // Disable all sequencer queues
    for (qid = 0; qid < seq_created_count; qid++) {
        qstate_addr = pd->lm_->get_lif_qstate_addr(LifIdGet(),
                                                   STORAGE_SEQ_QTYPE_SQ, qid);
        if (qstate_addr < 0) {
            NIC_LOG_ERR("{}: Failed to get qstate address for SEQ qid {}",
                        LifNameGet(), qid);
            continue;
        }
        WRITE_MEM(qstate_addr + offsetof(storage_seq_qstate_t, abort),
                  (uint8_t *)&abort, sizeof(abort), 0);
        WRITE_MEM(qstate_addr + offsetof(storage_seq_qstate_t, enable),
                  (uint8_t *)&enable, sizeof(enable), 0);
        PAL_barrier();
        p4plus_invalidate_cache(qstate_addr, sizeof(storage_seq_qstate_t),
                                P4PLUS_CACHE_INVALIDATE_TXDMA);
    }

    // Disable all adminq
    for (qid = 0; qid < spec->adminq_count; qid++) {
        qstate_addr = pd->lm_->get_lif_qstate_addr(LifIdGet(),
                                                   STORAGE_SEQ_QTYPE_ADMIN, qid);
        if (qstate_addr < 0) {
            NIC_LOG_ERR("{}: Failed to get qstate address for ADMIN qid {}",
                        LifNameGet(), qid);
            continue;
        }
        MEM_SET(qstate_addr + offsetof(admin_qstate_t, p_index0), 0,
                sizeof(admin_qstate_t) - offsetof(admin_qstate_t, p_index0), 0);
        PAL_barrier();
        p4plus_invalidate_cache(qstate_addr, sizeof(admin_qstate_t),
                                P4PLUS_CACHE_INVALIDATE_TXDMA);
    }

    time_expiry_set(fsm_ctx.ts, ACCEL_DEV_SEQ_QUEUES_QUIESCE_TIME_US);
    fsm_ctx.quiesce_qid = 0;
    fsm_ctx.devcmd.status = ACCEL_RC_EAGAIN;
    return ACCEL_LIF_EV_WAIT_SEQ_QUEUE_QUIESCE;
}

accel_lif_event_t
AccelLif::accel_lif_seq_quiesce_action(accel_lif_event_t event)
{
    int64_t                 qstate_addr;
    uint64_t                expiry;
    storage_seq_qstate_t    seq_qstate;
    uint32_t                max_pendings;
    int                     ret_val;

    ACCEL_LIF_FSM_VERBOSE_LOG();

    static_assert((offsetof(storage_seq_qstate_t, c_ndx) -
                   offsetof(storage_seq_qstate_t, p_ndx)) > 0, "");
    static_assert((offsetof(storage_seq_qstate_t, abort) -
                   offsetof(storage_seq_qstate_t, p_ndx)) > 0, "");

    fsm_ctx.devcmd.status = ACCEL_RC_EAGAIN;
    if (!time_expiry_check(fsm_ctx.ts)) {

        while (seq_qid_init_high &&
               (fsm_ctx.quiesce_qid <= seq_qid_init_high)) {

            qstate_addr = pd->lm_->get_lif_qstate_addr(LifIdGet(),
                                       STORAGE_SEQ_QTYPE_SQ, fsm_ctx.quiesce_qid);
            if (qstate_addr < 0) {
                NIC_LOG_ERR("{}: Failed to get qstate address for qid {}",
                            LifNameGet(), fsm_ctx.quiesce_qid);
                continue;
            }
            READ_MEM(qstate_addr + offsetof(storage_seq_qstate_t, p_ndx),
                     (uint8_t *)&seq_qstate.p_ndx,
                     (offsetof(storage_seq_qstate_t, abort) -
                      offsetof(storage_seq_qstate_t, p_ndx) +
                      sizeof(seq_qstate.abort)),
                     0);

            // As part of abort, P4+ would set c_ndx = p_ndx
            if (seq_qstate.c_ndx != seq_qstate.p_ndx) {
                return ACCEL_LIF_EV_NULL;
            }
            fsm_ctx.quiesce_qid++;
        }
    }

    NIC_LOG_DEBUG("{}: last qid quiesced: {} seq_qid_init_high: {}",
                 LifNameGet(), fsm_ctx.quiesce_qid, seq_qid_init_high);
    /*
     * Reset requires rings to be disabled which helps with convergence
     * to the quiesce state. For those rings that do not support disablement
     * (such as XTS/GCM rings), we'll wait for them to go empty. The expectation
     * here is that the Accel driver has already stopped submitting I/O requests
     * prior to asking for rings to be reset.
     */
    accel_rgroup_enable_set(false);
    fsm_ctx.devcmd.status = ACCEL_RC_SUCCESS;

    ret_val = accel_ring_max_pendings_get(max_pendings);
    if (ret_val == 0) {
        NIC_LOG_DEBUG("{}: max requests pending {}",
                      LifNameGet(), max_pendings);
        expiry = max_pendings ?
                 (uint64_t)max_pendings * ACCEL_DEV_RING_OP_QUIESCE_TIME_US :
                 ACCEL_DEV_RING_OP_QUIESCE_TIME_US;
        time_expiry_set(fsm_ctx.ts,
                        std::min(expiry,
                                 (uint64_t)ACCEL_DEV_ALL_RINGS_MAX_QUIESCE_TIME_US));
        return ACCEL_LIF_EV_WAIT_RGROUP_QUIESCE;
    }

    return ACCEL_LIF_EV_RGROUP_RESET;
}

accel_lif_event_t
AccelLif::accel_lif_rgroup_quiesce_action(accel_lif_event_t event)
{
    uint64_t    quiesce_time;
    int         ret_val;

    ACCEL_LIF_FSM_VERBOSE_LOG();

    fsm_ctx.devcmd.status = ACCEL_RC_EAGAIN;
    quiesce_time = platform_is_hw(pd->platform_) ?
                   ACCEL_DEV_RING_OP_QUIESCE_TIME_US : 0;
    if (!time_expiry_check(fsm_ctx.ts)) {
        ret_val = accel_rgroup_rindices_get();
        if (ret_val == 0) {

            for (auto iter = rgroup_map.begin();
                 iter != rgroup_map.end();
                 iter++) {

                if (accel_ring_num_pendings_get(iter->second)) {
                    return ACCEL_LIF_EV_NULL;
                }
            }

            NIC_LOG_DEBUG("{}: all rings quiesced", LifNameGet());
            time_expiry_set(fsm_ctx.ts, quiesce_time);

        } else {
            NIC_LOG_ERR("{}: unable to wait for rings to quiesce", LifNameGet());
            time_expiry_set(fsm_ctx.ts, 0);
        }

    } else {
        time_expiry_set(fsm_ctx.ts, quiesce_time);
    }

    fsm_ctx.devcmd.status = ACCEL_RC_SUCCESS;
    return ACCEL_LIF_EV_RGROUP_RESET;
}

accel_lif_event_t
AccelLif::accel_lif_rgroup_reset_action(accel_lif_event_t event)
{
    int     ret_val;

    ACCEL_LIF_FSM_VERBOSE_LOG();

    fsm_ctx.devcmd.status = ACCEL_RC_EAGAIN;
    if (!time_expiry_check(fsm_ctx.ts)) {
        return ACCEL_LIF_EV_NULL;
    }

    ret_val = accel_rgroup_reset_set(true);

    /*
     * Note that model does not have support for ring reset/disablement
     * i.e., cndx would not get cleared as we would expect.
     * Consequently, pndx should never be overwritten (particularly once
     * the ring has been active). To avoid any issues, we only
     * write pndx conditionally.
     */
    if (ret_val == 0) {
        ret_val = accel_rgroup_pndx_set(0, true);
    }

    /*
     * Bring out of reset
     */
    accel_rgroup_reset_set(false);

    /*
     * Reenable rings
     */
    accel_rgroup_enable_set(true);

    accel_dev.IntrClear();

    /*
     * It's now safe to destroy the LIF if applicable.
     */
    fsm_ctx.devcmd.status = ACCEL_RC_SUCCESS;
    if (fsm_ctx.reset_destroy) {
        if (!dev_api) {
            NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
            fsm_ctx.devcmd.status = ACCEL_RC_ERROR;
            return ACCEL_LIF_EV_NULL;
        }
        if (dev_api->lif_destroy(LifIdGet()) == SDK_RET_OK) {
            qmetrics_fini();
            accel_ring_info_del_all();
            NIC_LOG_DEBUG("{}: destroyed", LifNameGet());
            return ACCEL_LIF_EV_PRE_INIT;
        }

        NIC_LOG_ERR("{}: failed to destroy LIF", LifNameGet());
        fsm_ctx.devcmd.status = ACCEL_RC_ERROR;
    }

    return ACCEL_LIF_EV_SEQ_QUEUE_PRE_INIT;
}

accel_lif_event_t
AccelLif::accel_lif_adminq_init_action(accel_lif_event_t event)
{
    adminq_init_cmd_t   *cmd = (adminq_init_cmd_t *)fsm_ctx.devcmd.req;
    adminq_init_cpl_t   *cpl = (adminq_init_cpl_t *)fsm_ctx.devcmd.resp;
    admin_qstate_t      qstate;
    int64_t             addr, nicmgr_qstate_addr;

    ACCEL_LIF_FSM_LOG();
    NIC_LOG_DEBUG("{}: queue_index {} ring_base {:#x} ring_size {} "
                  "intr_index {}",  LifNameGet(), cmd->index, cmd->ring_base,
                  cmd->ring_size, cmd->intr_index);

    if (cmd->index >= spec->adminq_count) {
        NIC_LOG_ERR("{}: bad qid {}", LifNameGet(), cmd->index);
        fsm_ctx.devcmd.status = ACCEL_RC_EQID;
        return ACCEL_LIF_EV_NULL;
    }

    if (cmd->intr_index >= spec->intr_count) {
        NIC_LOG_ERR("{}: bad intr {}", LifNameGet(), cmd->intr_index);
        fsm_ctx.devcmd.status = ACCEL_RC_ERANGE;
        return ACCEL_LIF_EV_NULL;
    }

    if (cmd->ring_size < 2 || cmd->ring_size > 16) {
        NIC_LOG_ERR("{}: bad ring size {}", LifNameGet(), cmd->ring_size);
        fsm_ctx.devcmd.status = ACCEL_RC_ERANGE;
        return ACCEL_LIF_EV_NULL;
    }

    fsm_ctx.devcmd.status = ACCEL_RC_ERROR;
    addr = pd->lm_->get_lif_qstate_addr(LifIdGet(),
                            STORAGE_SEQ_QTYPE_ADMIN, cmd->index);
    if (addr < 0) {
        NIC_LOG_ERR("{}: Failed to get qstate address for SEQ qid {}",
                    LifNameGet(), cmd->index);
        return ACCEL_LIF_EV_NULL;
    }

    nicmgr_qstate_addr = pd->lm_->get_lif_qstate_addr(LifIdGet(),
                                  ACCEL_ADMINQ_REQ_QTYPE, ACCEL_ADMINQ_REQ_QID);
    if (nicmgr_qstate_addr < 0) {
        NIC_LOG_ERR("{}: Failed to get request qstate address for ADMIN qid {}",
                    LifIdGet(), cmd->index);
        return ACCEL_LIF_EV_NULL;
    }

    uint8_t off;
    if (pd->get_pc_offset("txdma_stage0.bin", "adminq_stage0", &off, NULL) < 0) {
        NIC_LOG_ERR("Failed to get PC offset of program: txdma_stage0.bin "
                    "label: adminq_stage0");
        return ACCEL_LIF_EV_NULL;
    }
    qstate.pc_offset = off;
    qstate.cos_sel = 0;
    qstate.cosA = ctl_cosA;
    qstate.cosB = ctl_cosB;
    qstate.host = 1;
    qstate.total = 1;
    qstate.pid = cmd->pid;
    qstate.p_index0 = 0;
    qstate.c_index0 = 0;
    qstate.comp_index = 0;
    qstate.ci_fetch = 0;
    qstate.sta.color = 1;
    qstate.cfg.enable = 1;
    qstate.cfg.host_queue = ACCEL_PHYS_ADDR_HOST_GET(cmd->ring_base);
    qstate.cfg.intr_enable = 1;
    qstate.ring_base = cmd->ring_base;
    if (qstate.cfg.host_queue && !ACCEL_PHYS_ADDR_LIF_GET(cmd->ring_base)) {
        qstate.ring_base |= ACCEL_PHYS_ADDR_LIF_SET(LifIdGet());
    }
    qstate.ring_size = cmd->ring_size;
    qstate.cq_ring_base = roundup(qstate.ring_base + (64 << cmd->ring_size), 4096);
    qstate.intr_assert_index = intr_base + cmd->intr_index;
    qstate.nicmgr_qstate_addr = nicmgr_qstate_addr;

    WRITE_MEM(addr, (uint8_t *)&qstate, sizeof(qstate), 0);

    PAL_barrier();
    p4plus_invalidate_cache(addr, sizeof(qstate), P4PLUS_CACHE_INVALIDATE_TXDMA);

    cpl->qid = cmd->index;
    cpl->qtype = STORAGE_SEQ_QTYPE_ADMIN;
    NIC_LOG_DEBUG("{}: qid {} qtype {}", LifNameGet(), cpl->qid, cpl->qtype);

    fsm_ctx.devcmd.status = ACCEL_RC_SUCCESS;
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_seq_queue_init_action(accel_lif_event_t event)
{
    seq_queue_init_cmd_t   *cmd = (seq_queue_init_cmd_t *)fsm_ctx.devcmd.req;
    seq_queue_init_cpl_t   *cpl = (seq_queue_init_cpl_t *)fsm_ctx.devcmd.resp;

    ACCEL_LIF_FSM_LOG();
    NIC_LOG_DEBUG("{}: qid {} qgroup {} core_id {} wring_base {:#x} wring_size {} "
                  "entry_size {}", LifNameGet(), cmd->index, cmd->qgroup,
                  cmd->core_id, cmd->wring_base, cmd->wring_size, cmd->entry_size);
    fsm_ctx.devcmd.status = _DevcmdSeqQueueSingleInit(cmd);
    cpl->qid = cmd->index;
    cpl->qtype = STORAGE_SEQ_QTYPE_SQ;
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_seq_queue_batch_init_action(accel_lif_event_t event)
{
    seq_queue_batch_init_cmd_t *batch_cmd =
                               (seq_queue_batch_init_cmd_t *)fsm_ctx.devcmd.req;
    seq_queue_init_cmd_t   cmd = {0};
    uint64_t               seq_q_size;
    uint32_t               i;

    ACCEL_LIF_FSM_LOG();
    NIC_LOG_DEBUG("{}: qid {} qgroup {} num_queues {} wring_base {:#x} "
                  "wring_size {} entry_size {}", LifNameGet(),
                  batch_cmd->index, batch_cmd->qgroup, batch_cmd->num_queues,
                  batch_cmd->wring_base, batch_cmd->wring_size,
                  batch_cmd->entry_size);
    cmd.opcode = CMD_OPCODE_SEQ_QUEUE_INIT;
    cmd.index = batch_cmd->index;
    cmd.pid = batch_cmd->pid;
    cmd.qgroup = batch_cmd->qgroup;
    cmd.enable = batch_cmd->enable;
    cmd.cos = batch_cmd->cos;
    cmd.total_wrings = batch_cmd->total_wrings;
    cmd.host_wrings = batch_cmd->host_wrings;
    cmd.entry_size = batch_cmd->entry_size;
    cmd.wring_size = batch_cmd->wring_size;
    cmd.wring_base = batch_cmd->wring_base;

    seq_q_size = (1ULL << batch_cmd->entry_size) *
                 (1ULL << batch_cmd->wring_size);
    for (i = 0; i < batch_cmd->num_queues; i++) {
        fsm_ctx.devcmd.status = _DevcmdSeqQueueSingleInit(&cmd);
        if (fsm_ctx.devcmd.status != ACCEL_RC_SUCCESS) {
            break;
        }
        cmd.index++;
        cmd.wring_base += seq_q_size;
    }
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_seq_queue_control_action(accel_lif_event_t event)
{
    seq_queue_control_cmd_t *cmd = (seq_queue_control_cmd_t *)fsm_ctx.devcmd.req;
    bool                    enable_sense;

    ACCEL_LIF_FSM_LOG();
    enable_sense = event == ACCEL_LIF_EV_SEQ_QUEUE_ENABLE;
    NIC_LOG_DEBUG("{}: qtype {} qid {} enable {}", LifNameGet(),
                  cmd->qtype, cmd->qid, enable_sense);

    fsm_ctx.devcmd.status = _DevcmdSeqQueueSingleControl(cmd, enable_sense);
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_seq_queue_batch_control_action(accel_lif_event_t event)
{
    seq_queue_batch_control_cmd_t *batch_cmd =
                            (seq_queue_batch_control_cmd_t *)fsm_ctx.devcmd.req;
    seq_queue_control_cmd_t cmd = {0};
    bool                    enable_sense;
    uint32_t                i;

    enable_sense = event == ACCEL_LIF_EV_SEQ_QUEUE_BATCH_ENABLE;
    ACCEL_LIF_FSM_LOG();
    NIC_LOG_DEBUG("{}: qtype {} qid {} mum_queues {} enable {}",
                  LifNameGet(), batch_cmd->qtype, batch_cmd->qid,
                  batch_cmd->num_queues, enable_sense);
    cmd.opcode = enable_sense ? CMD_OPCODE_SEQ_QUEUE_ENABLE :
                                CMD_OPCODE_SEQ_QUEUE_DISABLE;
    cmd.qid = batch_cmd->qid;
    cmd.qtype = batch_cmd->qtype;

    for (i = 0; i < batch_cmd->num_queues; i++) {
        fsm_ctx.devcmd.status =
                _DevcmdSeqQueueSingleControl(&cmd, enable_sense);
        if (fsm_ctx.devcmd.status != ACCEL_RC_SUCCESS) {
            break;
        }
        cmd.qid++;
    }
    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_hang_notify_action(accel_lif_event_t event)
{
    int64_t                 qstate_addr;
    storage_seq_qstate_t    seq_qstate;
    admin_qstate_t          admin_qstate;
    uint32_t                qid;

    ACCEL_LIF_FSM_LOG();

    /*
     * Dump everything about HW rings to log, to be collected by show-tech-support
     */
    fsm_ctx.info_dump = true;
    accel_rgroup_rindices_get();
    accel_rgroup_rmisc_get();
    accel_rgroup_rmetrics_get();
    fsm_ctx.info_dump = false;

    /*
     * Dump sequencer queue states
     */
    NIC_LOG_DEBUG("{}: seq_created_count {}", LifNameGet(), seq_created_count);
    for (qid = 0; qid < seq_created_count; qid++) {
        qstate_addr = pd->lm_->get_lif_qstate_addr(LifIdGet(),
                                                   STORAGE_SEQ_QTYPE_SQ, qid);
        if (qstate_addr < 0) {
            NIC_LOG_ERR("{}: Failed to get qstate address for SEQ qid {}",
                        LifNameGet(), qid);
            continue;
        }
        READ_MEM(qstate_addr, (uint8_t *)&seq_qstate, sizeof(seq_qstate), 0);

        NIC_LOG_DEBUG("    seq_queue {}: pndx {} c_ndx {} enable {} abort {}",
                      qid, seq_qstate.p_ndx, seq_qstate.c_ndx,
                      seq_qstate.enable, seq_qstate.abort);
    }

    /*
     * Dump admin queue states
     */
    NIC_LOG_DEBUG("{}: adminq_count {}", LifNameGet(), spec->adminq_count);
    for (qid = 0; qid < spec->adminq_count; qid++) {
        qstate_addr = pd->lm_->get_lif_qstate_addr(LifIdGet(),
                                                   STORAGE_SEQ_QTYPE_ADMIN, qid);
        if (qstate_addr < 0) {
            NIC_LOG_ERR("{}: Failed to get qstate address for ADMIN qid {}",
                        LifNameGet(), qid);
            continue;
        }
        READ_MEM(qstate_addr, (uint8_t *)&admin_qstate, sizeof(admin_qstate), 0);

        NIC_LOG_DEBUG("    admin_queue {}: pndx {} c_ndx {} comp {} intr {}",
                      qid, admin_qstate.p_index0, admin_qstate.c_index0,
                      admin_qstate.comp_index, admin_qstate.intr_assert_index);
    }

    return ACCEL_LIF_EV_NULL;
}

accel_lif_event_t
AccelLif::accel_lif_crypto_key_update_action(accel_lif_event_t event)
{
    ACCEL_LIF_FSM_LOG();
    fsm_ctx.devcmd.status =
        accel_dev.CmdHandler(fsm_ctx.devcmd.req, fsm_ctx.devcmd.req_data,
                             fsm_ctx.devcmd.resp, fsm_ctx.devcmd.resp_data);
    return ACCEL_LIF_EV_NULL;
}

void
AccelLif::accel_lif_state_machine(accel_lif_event_t event)
{
    accel_lif_ordered_event_t   *ordered_event;
    accel_lif_action_t          action;

    while (event != ACCEL_LIF_EV_NULL) {

        if ((fsm_ctx.state < ACCEL_LIF_ST_MAX) &&
            (event < ACCEL_LIF_EV_MAX)) {

            ordered_event = &lif_ordered_ev_table[fsm_ctx.state][event];
            fsm_ctx.enter_state = fsm_ctx.state;
            if (ordered_event->next_state != ACCEL_LIF_ST_SAME) {
                fsm_ctx.state = ordered_event->next_state;
            }
            action = ordered_event->action;
            if (!action) {
                NIC_LOG_ERR("Null action for state {} event {}",
                            lif_state_str(fsm_ctx.enter_state),
                            lif_event_str(event));
                throw;
            }
            event = (this->*action)(event);

        } else {
            NIC_LOG_ERR("Unknown state {} or event {}",
                        fsm_ctx.state, event);
            throw;
        }
    }
}

static void
accel_lif_state_machine_build(void)
{
    accel_lif_state_event_t    **fsm_entry;
    accel_lif_state_event_t    *state_event;
    accel_lif_state_event_t    *any_event;
    accel_lif_ordered_event_t  *ordered_event;
    uint32_t                   state;

    static bool lif_ordered_event_table_built;
    if (lif_ordered_event_table_built) {
        return;
    }
    lif_ordered_event_table_built = true;

    for (fsm_entry = &lif_fsm_table[0], state = 0;
         fsm_entry < &lif_fsm_table[ACCEL_LIF_ST_MAX];
         fsm_entry++, state++) {

        state_event = *fsm_entry;
        if (state_event) {
            any_event = nullptr;
            while (state_event->event != ACCEL_LIF_EV_NULL) {
                if (state_event->event < ACCEL_LIF_EV_MAX) {
                    ordered_event = &lif_ordered_ev_table[state]
                                                         [state_event->event];

                    ordered_event->action = state_event->action;
                    ordered_event->next_state = state_event->next_state;

                    if (state_event->event == ACCEL_LIF_EV_ANY) {
                        any_event = state_event;
                    }

                } else {
                    NIC_LOG_ERR("Unknown event {} for state {}", state_event->event,
                                lif_state_str((accel_lif_state_t)state));
                    throw;
                }
                state_event++;
            }

            if (!any_event) {
                NIC_LOG_ERR("Missing 'any' event for state {}",
                            lif_state_str((accel_lif_state_t)state));
                throw;
            }

            for (ordered_event = &lif_ordered_ev_table[state][0];
                 ordered_event < &lif_ordered_ev_table[state][ACCEL_LIF_EV_MAX];
                 ordered_event++) {

                if (!ordered_event->action) {
                    ordered_event->action  = any_event->action;
                    ordered_event->next_state = any_event->next_state;
                }
            }
        }
    }
}

accel_status_code_t
AccelLif::_DevcmdSeqQueueSingleInit(const seq_queue_init_cmd_t *cmd)
{
    uint64_t                qstate_addr;
    uint64_t                desc0_pgm_pc = 0;
    uint64_t                desc1_pgm_pc = 0;
    uint32_t                qid = cmd->index;
    storage_seq_qstate_t    qstate = {0};
    const char              *desc0_pgm_name = nullptr;
    const char              *desc1_pgm_name = nullptr;
    uint64_t                wring_base = 0;

    switch (cmd->qgroup) {

    case STORAGE_SEQ_QGROUP_CPDC:
        desc0_pgm_name = STORAGE_SEQ_PGM_NAME_SQ_GEN;
        break;

    case STORAGE_SEQ_QGROUP_CPDC_STATUS:
        desc0_pgm_name = STORAGE_SEQ_PGM_NAME_CPDC_STATUS0;
        desc1_pgm_name = STORAGE_SEQ_PGM_NAME_CPDC_STATUS1;
        break;

    case STORAGE_SEQ_QGROUP_CRYPTO:
        desc0_pgm_name = STORAGE_SEQ_PGM_NAME_SQ_GEN;
        break;

    case STORAGE_SEQ_QGROUP_CRYPTO_STATUS:
        desc0_pgm_name = STORAGE_SEQ_PGM_NAME_CRYPTO_STATUS0;
        desc1_pgm_name = STORAGE_SEQ_PGM_NAME_CRYPTO_STATUS1;
        break;

    default:
        break;
    }

    if (cmd->index >= seq_created_count) {
        NIC_LOG_ERR("{}: qgroup {} index {} exceeds max {}", LifNameGet(),
                    cmd->qgroup, cmd->index, seq_created_count);
        return ACCEL_RC_EQID;
    }

    qstate_addr = pd->lm_->get_lif_qstate_addr(LifIdGet(),
                                               STORAGE_SEQ_QTYPE_SQ, qid);
    if (qstate_addr < 0) {
        NIC_LOG_ERR("{}: Failed to get qstate address for SEQ qid {}",
                    LifNameGet(), cmd->index);
        return ACCEL_RC_ERROR;
    }

    if (desc0_pgm_name &&
        pd->get_pc_offset(desc0_pgm_name, NULL, NULL, &desc0_pgm_pc)) {
        NIC_LOG_ERR("Failed to get base for program {}", desc0_pgm_name);
        return ACCEL_RC_ERROR;
    }

    if (desc1_pgm_name &&
        pd->get_pc_offset(desc1_pgm_name, NULL, NULL, &desc1_pgm_pc)) {
        NIC_LOG_ERR("Failed to get base for program {}", desc1_pgm_name);
        return ACCEL_RC_ERROR;
    }

    wring_base = cmd->wring_base;
    if (ACCEL_PHYS_ADDR_HOST_GET(cmd->wring_base) &&
        !ACCEL_PHYS_ADDR_LIF_GET(cmd->wring_base)) {
        wring_base |= ACCEL_PHYS_ADDR_LIF_SET(LifIdGet());
    }

    seq_qid_init_high = std::max(seq_qid_init_high, qid);

    uint8_t off;
    if (pd->get_pc_offset("txdma_stage0.bin", "storage_seq_stage0",
                          &off, NULL) < 0) {
        NIC_LOG_ERR("Failed to get PC offset of program: txdma_stage0.bin "
                    "label: storage_seq_stage0");
        return ACCEL_RC_ERROR;
    }
    qstate.pc_offset = off;
    qstate.cos_sel = 0;
    qstate.cosA = 0;
    qstate.cosB = cmd->cos;
    qstate.host_wrings = cmd->host_wrings;
    qstate.total_wrings = cmd->total_wrings;
    qstate.pid = cmd->pid;
    qstate.p_ndx = 0;
    qstate.c_ndx = 0;
    qstate.enable = cmd->enable;
    qstate.abort = 0;
    qstate.wring_base = htonll(wring_base);
    qstate.wring_size = htons(cmd->wring_size);
    qstate.entry_size = htons(cmd->entry_size);

    if (desc0_pgm_name) {
        qstate.desc0_next_pc = htonl(desc0_pgm_pc >> 6);
    }

    if (desc1_pgm_name) {
        qstate.desc1_next_pc = htonl(desc1_pgm_pc >> 6);
        qstate.desc1_next_pc_valid = 1;
    }

    qstate.qgroup = cmd->qgroup;
    qstate.core_id = cmd->core_id;

    WRITE_MEM(qstate_addr, (uint8_t *)&qstate, sizeof(qstate), 0);

    PAL_barrier();
    p4plus_invalidate_cache(qstate_addr, sizeof(qstate),
                            P4PLUS_CACHE_INVALIDATE_TXDMA);
    qinfo_metrics_update(qid, qstate_addr, qstate);
    return ACCEL_RC_SUCCESS;
}

accel_status_code_t
AccelLif::_DevcmdSeqQueueSingleControl(const seq_queue_control_cmd_t *cmd,
                                       bool enable)
{
    int64_t                 qstate_addr;
    uint8_t                 value;
    struct admin_cfg_qstate admin_cfg = {0};

    if (cmd->qtype >= STORAGE_SEQ_QTYPE_MAX) {
        NIC_LOG_ERR("{}: bad qtype {}", LifNameGet(), cmd->qtype);
        return ACCEL_RC_EQTYPE;
    }

    value = enable;
    switch (cmd->qtype) {
    case STORAGE_SEQ_QTYPE_SQ:
        if (cmd->qid >= seq_created_count) {
            NIC_LOG_ERR("{}: qtype {} qid {} exceeds count {}", LifNameGet(),
                        cmd->qtype, cmd->qid, seq_created_count);
            return ACCEL_RC_EQID;
        }
        qstate_addr = pd->lm_->get_lif_qstate_addr(LifIdGet(), cmd->qtype,
                                                   cmd->qid);
        if (qstate_addr < 0) {
            NIC_LOG_ERR("{}: Failed to get qstate address for ADMIN qid {}",
                        LifNameGet(), cmd->qid);
            return ACCEL_RC_ERROR;
        }
        WRITE_MEM(qstate_addr + offsetof(storage_seq_qstate_t, enable),
                  (uint8_t *)&value, sizeof(value), 0);
        PAL_barrier();
        p4plus_invalidate_cache(qstate_addr, sizeof(storage_seq_qstate_t),
                                P4PLUS_CACHE_INVALIDATE_TXDMA);
        break;
    case STORAGE_SEQ_QTYPE_ADMIN:
        if (cmd->qid >= spec->adminq_count) {
            NIC_LOG_ERR("{}: qtype {} qid {} exceeds count {}", LifNameGet(),
                        cmd->qtype, cmd->qid, spec->adminq_count);
            return ACCEL_RC_EQID;
        }
        qstate_addr = pd->lm_->get_lif_qstate_addr(LifIdGet(), cmd->qtype,
                                                   cmd->qid);
        if (qstate_addr < 0) {
            NIC_LOG_ERR("{}: Failed to get qstate address for ADMIN qid {}",
                        LifNameGet(), cmd->qid);
            return ACCEL_RC_ERROR;
        }
        admin_cfg.enable = enable;
        admin_cfg.host_queue = 0x1;
        WRITE_MEM(qstate_addr + offsetof(admin_qstate_t, cfg),
                  (uint8_t *)&admin_cfg, sizeof(admin_cfg), 0);
        PAL_barrier();
        p4plus_invalidate_cache(qstate_addr, sizeof(storage_seq_qstate_t),
                                P4PLUS_CACHE_INVALIDATE_TXDMA);
        break;
    default:
        return ACCEL_RC_EQTYPE;
        break;
    }

    return ACCEL_RC_SUCCESS;
}

/*
 * Populate accel_ring_tbl with info read from HW
 */
int
AccelLif::accel_ring_info_get_all(void)
{
    int         ret_val;

    ret_val = accel_rgroup_add();
    if (ret_val == 0) {
        ret_val = accel_rgroup_rings_add();
    }
    if (ret_val == 0) {
        ret_val = accel_rgroup_rinfo_get();
    }

    if (ret_val == 0) {
        for (auto iter = rgroup_map.begin();
             iter != rgroup_map.end();
             iter++) {

            const accel_rgroup_rinfo_rsp_t& info = iter->second.info;
            if (info.ring_handle < ACCEL_RING_ID_MAX) {
                accel_ring_t& spec_ring = accel_ring_tbl[info.ring_handle];
                spec_ring.ring_base_pa = info.base_pa;
                spec_ring.ring_pndx_pa = info.pndx_pa;
                spec_ring.ring_shadow_pndx_pa = info.shadow_pndx_pa;
                spec_ring.ring_opaque_tag_pa = info.opaque_tag_pa;
                spec_ring.ring_opaque_tag_size = info.opaque_tag_size;
                spec_ring.ring_desc_size = info.desc_size;
                spec_ring.ring_pndx_size = info.pndx_size;
                spec_ring.ring_size = info.ring_size;

                NIC_LOG_DEBUG("ring {} ring_base_pa {:#x} ring_pndx_pa {:#x} "
                             "ring_shadow_pndx_pa {:#x} ring_opaque_tag_pa {:#x} "
                             "ring_size {}",
                             accel_ring_id2name_get(info.ring_handle),
                             spec_ring.ring_base_pa, spec_ring.ring_pndx_pa,
                             spec_ring.ring_shadow_pndx_pa,
                             spec_ring.ring_opaque_tag_pa, spec_ring.ring_size);
            }
        }
    }

    return ret_val;
}

/*
 * Delete accel_ring_tbl
 */
void
AccelLif::accel_ring_info_del_all(void)
{
    uint32_t    ring_handle;
    uint32_t    sub_ring;

    auto iter = rgroup_map.begin();
    while (iter != rgroup_map.end()) {
         accel_rgroup_ring_t& rgroup_ring = iter->second;
         if (rgroup_ring.delphi_metrics) {
             accel_rgroup_ring_key_extract(iter->first, ring_handle, sub_ring);
             NIC_LOG_DEBUG("{}: deleting delphi_metrics ring {} "
                           "sub_ring {}", LifNameGet(),
                           accel_ring_id2name_get(ring_handle), sub_ring);
             rgroup_ring.delphi_metrics->Delete();
             rgroup_ring.delphi_metrics.reset();
         }

         iter = rgroup_map.erase(iter);
    }
    accel_rgroup_rings_del();
    accel_rgroup_del();
}

/*
 * Create accelerator ring group
 */
int
AccelLif::accel_rgroup_add(void)
{
    int     ret_val;

    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return -1;
    }
    ret_val = dev_api->accel_rgroup_add(LifNameGet(), cmb_rmetrics_addr,
                                        cmb_rmetrics_size);
    if (ret_val != SDK_RET_OK) {
        NIC_LOG_ERR("{}: failed to add ring group", LifNameGet());
    }
    return ret_val;
}

/*
 * Delete accelerator ring group
 */
void
AccelLif::accel_rgroup_del(void)
{
    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return;
    }
    if (dev_api->accel_rgroup_del(LifNameGet()) != SDK_RET_OK) {
        NIC_LOG_ERR("{}: failed to delete ring group", LifNameGet());
    }
}

/*
 * Add relevant rings to accelerator ring group
 */
int
AccelLif::accel_rgroup_rings_add(void)
{
    int     ret_val;

    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return -1;
    }
    ret_val = dev_api->accel_rgroup_ring_add(LifNameGet(), accel_ring_vec);
    if (ret_val != SDK_RET_OK) {
        NIC_LOG_ERR("{}: failed to add ring vector", LifNameGet());
    }
    return ret_val;
}

/*
 * Delete relevant rings from accelerator ring group
 */
void
AccelLif::accel_rgroup_rings_del(void)
{
    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return;
    }
    if (dev_api->accel_rgroup_ring_del(LifNameGet(),
                                       accel_ring_vec) != SDK_RET_OK) {
        NIC_LOG_ERR("{}: failed to delete ring vector", LifNameGet());
    }
}

/*
 * Reset/Un-reset a HW ring
 */
int
AccelLif::accel_rgroup_reset_set(bool reset_sense)
{
    int     ret_val;

    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return -1;
    }
    ret_val = dev_api->accel_rgroup_reset_set(LifNameGet(),
                                    ACCEL_SUB_RING_ALL, reset_sense);
    if (ret_val != SDK_RET_OK) {
        NIC_LOG_ERR("{}: failed to reset ring group sense {}",
                    LifNameGet(), reset_sense);
    }
    return ret_val;
}

/*
 * Enable/disable a HW ring
 */
int
AccelLif::accel_rgroup_enable_set(bool enable_sense)
{
    int     ret_val;

    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return -1;
    }
    ret_val = dev_api->accel_rgroup_enable_set(LifNameGet(),
                                    ACCEL_SUB_RING_ALL, enable_sense);
    if (ret_val != SDK_RET_OK) {
        NIC_LOG_ERR("{}: failed to enable ring group sense {}",
                    LifNameGet(), enable_sense);
    }
    return ret_val;
}

/*
 * Set producer index for a HW ring
 */
int
AccelLif::accel_rgroup_pndx_set(uint32_t val,
                                bool conditional)
{
    int     ret_val;

    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return -1;
    }
    ret_val = dev_api->accel_rgroup_pndx_set(LifNameGet(),
                                    ACCEL_SUB_RING_ALL, val, conditional);
    if (ret_val != SDK_RET_OK) {
        NIC_LOG_ERR("{}: failed to set pndx val {}", LifNameGet(), val);
    }
    return ret_val;
}

/*
 * Accelerator ring group ring info response callback handler
 */
void
accel_rgroup_rinfo_rsp_cb(void *user_ctx,
                          const accel_rgroup_rinfo_rsp_t& info)
{
    AccelLif    *lif = (AccelLif *)user_ctx;

    ACCEL_RGROUP_GET_CB_HANDLE_CHECK(info.ring_handle);
    accel_rgroup_ring_t& rgroup_ring =
            lif->accel_rgroup_find_create(info.ring_handle, info.sub_ring);
    rgroup_ring.info = info;
}

/*
 * Get ring group info
 */
int
AccelLif::accel_rgroup_rinfo_get(void)
{
    uint32_t    num_rings;
    int         ret_val;

    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return -1;
    }
    ret_val = dev_api->accel_rgroup_info_get(LifNameGet(), ACCEL_SUB_RING_ALL,
                             accel_rgroup_rinfo_rsp_cb, this, &num_rings);
    ACCEL_RGROUP_GET_RET_CHECK(ret_val, num_rings, "rinfo");
    return ret_val;
}

/*
 * Accelerator ring group ring indices response callback handler
 */
void
accel_rgroup_rindices_rsp_cb(void *user_ctx,
                             const accel_rgroup_rindices_rsp_t& indices)
{
    AccelLif    *lif = (AccelLif *)user_ctx;

    ACCEL_RGROUP_GET_CB_HANDLE_CHECK(indices.ring_handle);
    accel_rgroup_ring_t& rgroup_ring =
            lif->accel_rgroup_find_create(indices.ring_handle, indices.sub_ring);
    rgroup_ring.indices = indices;

    if (lif->fsm_ctx.info_dump) {
        NIC_LOG_DEBUG("{}: ring {} sub_ring {}", lif->LifNameGet(),
                accel_ring_id2name_get(indices.ring_handle), indices.sub_ring);
        NIC_LOG_DEBUG("    pndx {} cndx {}", indices.pndx, indices.cndx);
    }
}

/*
 * Get ring group indices
 */
int
AccelLif::accel_rgroup_rindices_get(void)
{
    uint32_t    num_rings;
    int         ret_val;

    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return -1;
    }
    ret_val = dev_api->accel_rgroup_indices_get(LifNameGet(), ACCEL_SUB_RING_ALL,
                             accel_rgroup_rindices_rsp_cb, this, &num_rings);
    ACCEL_RGROUP_GET_RET_CHECK(ret_val, num_rings, "indices");
    return ret_val;
}

/*
 * Accelerator ring group ring metrics response callback handler
 */
void
accel_rgroup_rmetrics_rsp_cb(void *user_ctx,
                             const accel_rgroup_rmetrics_rsp_t& metrics)
{
    AccelLif    *lif = (AccelLif *)user_ctx;

    ACCEL_RGROUP_GET_CB_HANDLE_CHECK(metrics.ring_handle);
    accel_rgroup_ring_t& rgroup_ring =
            lif->accel_rgroup_find_create(metrics.ring_handle, metrics.sub_ring);
    rgroup_ring.metrics = metrics;

    if (lif->fsm_ctx.info_dump) {
        NIC_LOG_DEBUG("{}: ring {} sub_ring {}", lif->LifNameGet(),
                accel_ring_id2name_get(metrics.ring_handle), metrics.sub_ring);
        NIC_LOG_DEBUG("    input_bytes {} output_bytes {} soft_resets {}",
                      metrics.input_bytes, metrics.output_bytes,
                      metrics.soft_resets);
    }
}

/*
 * Get ring group metrics
 */
int
AccelLif::accel_rgroup_rmetrics_get(void)
{
    uint32_t    num_rings;
    int         ret_val;

    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return -1;
    }
    ret_val = dev_api->accel_rgroup_metrics_get(LifNameGet(), ACCEL_SUB_RING_ALL,
                             accel_rgroup_rmetrics_rsp_cb, this, &num_rings);
    ACCEL_RGROUP_GET_RET_CHECK(ret_val, num_rings, "metrics");
    return ret_val;
}

/*
 * Accelerator ring group misc info response callback handler
 */
void
accel_rgroup_rmisc_rsp_cb(void *user_ctx,
                          const accel_rgroup_rmisc_rsp_t& misc)
{
    AccelLif    *lif = (AccelLif *)user_ctx;
    uint32_t    num_reg_vals;

    ACCEL_RGROUP_GET_CB_HANDLE_CHECK(misc.ring_handle);
    if (lif->fsm_ctx.info_dump) {
        NIC_LOG_DEBUG("{}: ring {} sub_ring {} num_reg_vals {}",
                      lif->LifNameGet(), accel_ring_id2name_get(misc.ring_handle),
                      misc.sub_ring, misc.num_reg_vals);
        num_reg_vals = std::min(misc.num_reg_vals,
                                (uint32_t)ACCEL_RING_NUM_REGS_MAX);
        for (uint32_t i = 0; i < num_reg_vals; i++) {
            NIC_LOG_DEBUG("    {} {:#x} ({})", misc.reg_val[i].name,
                          misc.reg_val[i].val, misc.reg_val[i].val);
        }
    }
}

/*
 * Get ring group misc info
 */
int
AccelLif::accel_rgroup_rmisc_get(void)
{
    uint32_t    num_rings;
    int         ret_val;

    if (!dev_api) {
        NIC_LOG_ERR("{}: Uninitialzed devapi.", LifNameGet());
        return -1;
    }
    ret_val = dev_api->accel_rgroup_misc_get(LifNameGet(), ACCEL_SUB_RING_ALL,
                             accel_rgroup_rmisc_rsp_cb, this, &num_rings);
    ACCEL_RGROUP_GET_RET_CHECK(ret_val, num_rings, "misc");
    return ret_val;
}

/*
 * Return number of requests outstanding on a HW ring.
 */
uint32_t
AccelLif::accel_ring_num_pendings_get(const accel_rgroup_ring_t& rgroup_ring)
{
    uint32_t        pndx = 0;
    uint32_t        cndx = 0;
    uint32_t        num_pendings = 0;

    if (rgroup_ring.info.ring_size) {
        pndx = rgroup_ring.indices.pndx % rgroup_ring.info.ring_size;
        cndx = rgroup_ring.indices.cndx % rgroup_ring.info.ring_size;
        num_pendings = pndx < cndx ?
                       (pndx + rgroup_ring.info.ring_size) - cndx :
                       pndx - cndx;

        /*
         * If ring had disablement capability it would have been disabled
         * so we can consider the ring as empty.
         */
        if (rgroup_ring.info.sw_enable_capable) {
            num_pendings = 0;
        }
    }

    if (num_pendings == 0) {
        NIC_LOG_DEBUG("{} ring {} pndx {} cndx {}", LifNameGet(),
                     accel_ring_id2name_get(rgroup_ring.info.ring_handle),
                     pndx, cndx);
    }
    return num_pendings;
}

/*
 * Calculate the largest number of requests outstanding
 * of all the rings.
 */
int
AccelLif::accel_ring_max_pendings_get(uint32_t& max_pendings)
{
    uint32_t    num_pendings;
    int         ret_val;

    max_pendings = 0;
    ret_val = accel_rgroup_rindices_get();
    if (ret_val == 0) {

        for (auto iter  = rgroup_map.begin();
             iter != rgroup_map.end();
             iter++) {

            num_pendings = accel_ring_num_pendings_get(iter->second);
            max_pendings = std::max(max_pendings, num_pendings);
        }
    }

    return ret_val;
}

int
AccelLif::qmetrics_init(void)
{
    accel_metrics::AccelSeqQueueKey     seq_qkey;
    uint64_t                            qmetrics_addr;
    int64_t                             qstate_addr;
    uint32_t                            qid;
    storage_seq_qstate_t                qstate = {0};

    if (g_nicmgr_svc) {
        seq_qkey.set_lifid(std::to_string(LifIdGet()));
        for (qid = 0; qid < seq_created_count; qid++) {
            seq_qkey.set_qid(std::to_string(qid));
            qstate_addr = pd->lm_->get_lif_qstate_addr(LifIdGet(),
                                                       STORAGE_SEQ_QTYPE_SQ, qid);
            if (qstate_addr < 0) {
                NIC_LOG_ERR("{}: Failed to get qstate address for SEQ qid {}",
                            LifNameGet(), qid);
                return -1;
            }

            qmetrics_addr = qstate_addr +
                            offsetof(storage_seq_qstate_t, metrics);
            auto qmetrics = delphi::objects::AccelSeqQueueMetrics::
                            NewAccelSeqQueueMetrics(seq_qkey, qmetrics_addr);
            delphi_qmetrics_vec.push_back(std::move(qmetrics));

            /*
             * Some of the fields below will be updated when driver
             * issues _DevcmdSeqQueueInit().
             */
            qstate.qgroup = STORAGE_SEQ_QGROUP_CPDC;
            qinfo_metrics_update(qid, qstate_addr, qstate);

            auto qinfo = delphi::objects::AccelSeqQueueInfoMetrics::
                         NewAccelSeqQueueInfoMetrics(seq_qkey,
                                                     qinfo_metrics_addr_get(qid));
            delphi_qinfo_vec.push_back(std::move(qinfo));
        }

        NIC_LOG_DEBUG("{}: created qmetrics_vec size {} qinfo_vec size {}",
                      LifNameGet(), delphi_qmetrics_vec.size(),
                      delphi_qinfo_vec.size());
    }

    return SDK_RET_OK;
}

void
AccelLif::qmetrics_fini(void)
{
    uint32_t    qid;

    if (delphi_qmetrics_vec.size() == delphi_qinfo_vec.size()) {
        NIC_LOG_DEBUG("{}: destroying qmetrics_vec size {} qinfo_vec size {}",
                      LifNameGet(), delphi_qmetrics_vec.size(),
                      delphi_qinfo_vec.size());
        for (qid = 0; qid < delphi_qmetrics_vec.size(); qid++) {
            auto qmetrics = delphi_qmetrics_vec.at(qid);
            qmetrics->Delete();
            qmetrics.reset();

            auto qinfo = delphi_qinfo_vec.at(qid);
            qinfo->Delete();
            qinfo.reset();
        }

        delphi_qmetrics_vec.clear();
        delphi_qinfo_vec.clear();

    } else {
        NIC_LOG_ERR("{}: unexpected qmetrics_vec size {} != qinfo_vec size {}",
                    LifNameGet(), delphi_qmetrics_vec.size(),
                    delphi_qinfo_vec.size());
        throw;
    }
}

/*
 * Get address into Delphi HW ring metrics memory for update.
 */
uint64_t
AccelLif::rmetrics_addr_get(uint32_t ring_handle,
                            uint32_t sub_ring)
{
    uint64_t    offs;

    offs = (ring_handle * (ACCEL_SUB_RING_MAX * sizeof(accel_ring_metrics_t))) +
           (sub_ring * sizeof(accel_ring_metrics_t));
    if ((offs + sizeof(accel_ring_metrics_t)) > cmb_rmetrics_size) {
        NIC_LOG_ERR("{}: out of rmetrics memory for ring {} sub_ring {}",
                    LifNameGet(), ring_handle, sub_ring);
        throw;
    }
    return cmb_rmetrics_addr + offs;
}

/*
 * Get address into Delphi qinfo_metrics memory for update.
 */
uint64_t
AccelLif::qinfo_metrics_addr_get(uint32_t qid)
{
    uint64_t    offs = qid * sizeof(seq_queue_info_metrics_t);

    if ((offs + sizeof(seq_queue_info_metrics_t)) > cmb_qinfo_size) {
        NIC_LOG_ERR("{}: out of qinfo memory for qid {}", LifNameGet(), qid);
        throw;
    }
    return cmb_qinfo_addr + offs;
}

/*
 * Direct update Delphi qinfo_metrics for a qid.
 */
void
AccelLif::qinfo_metrics_update(uint32_t qid,
                               uint64_t qstate_addr,
                               const storage_seq_qstate_t& qstate)
{
    seq_queue_info_metrics_t    info = {0};

    info.qstate_addr = qstate_addr;
    info.qgroup = qstate.qgroup;
    info.core_id = qstate.core_id;
    WRITE_MEM(qinfo_metrics_addr_get(qid), (uint8_t *)&info, sizeof(info), 0);
}

static const char *
lif_state_str(accel_lif_state_t state)
{
    if (state < ACCEL_LIF_ST_MAX) {
        return lif_state_str_table[state];
    }
    return "unknown_state";
}

static const char *
lif_event_str(accel_lif_event_t event)
{
    if (event < ACCEL_LIF_EV_MAX) {
        return lif_event_str_table[event];
    }
    return "unknown_event";
}

