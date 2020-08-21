#ifndef __FTL_LIF_HPP__
#define __FTL_LIF_HPP__

#include <map>
#include <utility>
#include "nic/p4/ftl_dev/include/ftl_dev_shared.h"
#include "nic/sdk/asic/pd/db.hpp"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))
#endif

#define FTL_LIF_NAME                        "ftl_lif"

/*
 * Local doorbell address formation
 */
#define DB_QID_SHFT                     24
#define DB32_QID_SHFT                   16

#define FTL_LIF_DBDATA_SET(qid, pndx)                               \
    (((uint64_t)(qid) << DB_QID_SHFT) | (pndx))

#define FTL_LIF_DBDATA32_SET(qid, pndx)                             \
    (((uint64_t)(qid) << DB32_QID_SHFT) | (pndx))

using namespace ftl_dev_if;

class FtlLif;

static inline bool
is_power_of_2(uint64_t n)
{
    return n && !(n & (n - 1));
}

/**
 * Generic timestamp and expiry interval
 */
static inline uint64_t
timestamp(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (((uint64_t)tv.tv_sec * USEC_PER_SEC) +
            (uint64_t)tv.tv_usec);
}

class ftl_timestamp_t
{
public:
    ftl_timestamp_t() :
        ts(0),
        expiry(0) {}

    void time_expiry_set(uint64_t val)
    {
        ts = timestamp();
        expiry = val;
    }

    bool time_expiry_check(void)
    {
        return (expiry == 0) ||
               ((timestamp() - ts) > expiry);
    }

private:
    uint64_t                ts;
    uint64_t                expiry;
};

/**
 * Devcmd context
 */
class ftl_lif_devcmd_ctx_t {
public:
    ftl_lif_devcmd_ctx_t() :
        req(nullptr),
        req_data(nullptr),
        rsp(nullptr),
        rsp_data(nullptr),
        status(FTL_RC_SUCCESS)
    {
    }

    ftl_devcmd_t            *req;
    void                    *req_data;
    ftl_devcmd_cpl_t        *rsp;
    void                    *rsp_data;
    ftl_status_code_t       status;
};

/**
 * LIF State Machine
 */
typedef enum {
    FTL_LIF_ST_INITIAL,
    FTL_LIF_ST_WAIT_HAL,
    FTL_LIF_ST_PRE_INIT,
    FTL_LIF_ST_POST_INIT,
    FTL_LIF_ST_QUEUES_RESET,
    FTL_LIF_ST_QUEUES_PRE_INIT,
    FTL_LIF_ST_QUEUES_INIT_TRANSITION,
    FTL_LIF_ST_QUEUES_STOPPING,
    FTL_LIF_ST_SCANNERS_QUIESCE,
    FTL_LIF_ST_TIMERS_QUIESCE,
    FTL_LIF_ST_QUEUES_STOPPED,
    FTL_LIF_ST_QUEUES_STARTED,
    FTL_LIF_ST_MAX,

    FTL_LIF_ST_SAME
} ftl_lif_state_t;

#define FTL_LIF_STATE_STR_TABLE                                     \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_INITIAL),                    \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_WAIT_HAL),                   \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_PRE_INIT),                   \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_POST_INIT),                  \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_QUEUES_RESET),               \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_QUEUES_PRE_INIT),            \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_QUEUES_INIT_TRANSITION),     \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_QUEUES_STOPPING),            \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_SCANNERS_QUIESCE),           \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_TIMERS_QUIESCE),             \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_QUEUES_STOPPED),             \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_ST_QUEUES_STARTED),             \

typedef enum {
    FTL_LIF_EV_NULL,
    FTL_LIF_EV_ANY,
    FTL_LIF_EV_CREATE,
    FTL_LIF_EV_DESTROY,
    FTL_LIF_EV_HAL_UP,
    FTL_LIF_EV_IDENTIFY,
    FTL_LIF_EV_INIT,
    FTL_LIF_EV_SETATTR,
    FTL_LIF_EV_GETATTR,
    FTL_LIF_EV_RESET,
    FTL_LIF_EV_RESET_DESTROY,
    FTL_LIF_EV_PRE_INIT,
    FTL_LIF_EV_SCANNERS_QUIESCE,
    FTL_LIF_EV_TIMERS_QUIESCE,
    FTL_LIF_EV_POLLERS_INIT,
    FTL_LIF_EV_SCANNERS_INIT,
    FTL_LIF_EV_SCANNERS_START,
    FTL_LIF_EV_SCANNERS_START_SINGLE,
    FTL_LIF_EV_SCANNERS_STOP,
    FTL_LIF_EV_QUEUES_STOP_COMPLETE,
    FTL_LIF_EV_POLLERS_FLUSH,
    FTL_LIF_EV_POLLERS_DEQ_BURST,
    FTL_LIF_EV_MPU_TIMESTAMP_INIT,
    FTL_LIF_EV_MPU_TIMESTAMP_START,
    FTL_LIF_EV_MPU_TIMESTAMP_STOP,
    FTL_LIF_EV_ACCEL_AGING_CONTROL,

    FTL_LIF_EV_MAX
} ftl_lif_event_t;

#define FTL_LIF_EVENT_STR_TABLE                                     \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_NULL),                       \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_ANY),                        \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_CREATE),                     \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_DESTROY),                    \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_HAL_UP),                     \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_IDENTIFY),                   \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_INIT),                       \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_SETATTR),                    \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_GETATTR),                    \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_RESET),                      \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_RESET_DESTROY),              \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_PRE_INIT),                   \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_SCANNERS_QUIESCE),           \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_TIMERS_QUIESCE),             \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_POLLERS_INIT),               \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_SCANNERS_INIT),              \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_SCANNERS_START),             \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_SCANNERS_START_SINGLE),      \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_SCANNERS_STOP),              \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_QUEUES_STOP_COMPLETE),       \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_POLLERS_FLUSH),              \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_POLLERS_DEQ_BURST),          \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_MPU_TIMESTAMP_INIT),         \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_MPU_TIMESTAMP_START),        \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_MPU_TIMESTAMP_STOP),         \
    FTL_DEV_INDEX_STRINGIFY(FTL_LIF_EV_ACCEL_AGING_CONTROL),        \
 
typedef ftl_lif_event_t (FtlLif::*ftl_lif_action_t)(ftl_lif_event_t event,
                                                    ftl_lif_devcmd_ctx_t& devcmd_ctx);

typedef struct {
    ftl_lif_event_t         event;
    ftl_lif_action_t        action;
    ftl_lif_state_t         next_state;
} ftl_lif_state_event_t;

typedef struct {
    ftl_lif_state_event_t *state_event;
} ftl_lif_fsm_t;

typedef struct {
    ftl_lif_action_t        action;
    ftl_lif_state_t         next_state;
} ftl_lif_ordered_event_t;

/**
 * HW Scanners init - single queue init
 */
typedef struct {
    uint8_t                 cos;
    uint8_t                 qtype;
    uint16_t                lif_index;
    uint16_t                pid;
    uint32_t                index;
    uint64_t                poller_qstate_addr;
    uint64_t                scan_addr_base;
    uint32_t                scan_id_base;
    uint32_t                scan_range_sz;
    uint32_t                scan_burst_sz;
    uint32_t                scan_resched_ticks;
    uint8_t                 poller_qdepth_shft;
    uint8_t                 cos_override;
    uint8_t                 resched_uses_slow_timer;
    uint8_t                 pc_offset;
} scanner_init_single_cmd_t;

/**
 * SW Pollers init - single queue init
 */
typedef struct {
    uint16_t                lif_index;
    uint16_t                pid;
    uint32_t                index;
    uint64_t                wring_base_addr;
    uint32_t                wring_sz;
    uint8_t                 qdepth_shft;
} poller_init_single_cmd_t;

/*
 * Memory access wrapper
 */
class mem_access_t {
public:
    mem_access_t(FtlLif& lif) :
        lif(lif),
        paddr(0),
        vaddr(nullptr),
        total_sz(0),
        mmap_taken(false)
    {
    }

    ~mem_access_t()
    {
        clear();
    }

    void clear(void);
    void reset(uint64_t paddr,
               uint32_t total_sz,
               bool mmap_requested = true);
    void set(uint64_t new_paddr,
             volatile uint8_t *new_vaddr,
             uint32_t new_sz);
    void small_read(uint32_t offset,
                    uint8_t *buf,
                    uint32_t read_sz) const;
    void small_write(uint32_t offset,
                     const uint8_t *buf,
                     uint32_t write_sz) const;
    void large_read(uint32_t offset,
                    uint8_t *buf,
                    uint32_t read_sz) const;
    void large_write(uint32_t offset,
                     const uint8_t *buf,
                     uint32_t write_sz) const;
    void cache_invalidate(uint32_t offset = 0,
                          uint32_t sz = 0) const;
    uint64_t pa(void) const { return paddr; }
    volatile uint8_t *va(void) const { return vaddr; }


private:
    FtlLif&                 lif;
    int64_t                 paddr;
    volatile uint8_t        *vaddr;
    uint32_t                total_sz;
    bool                    mmap_taken;
};

class mem_access_vec_t {
public:
    mem_access_vec_t(FtlLif& lif) :
        lif(lif),
        mem_access_vec(lif),
        elem_count_(0),
        elem_sz_(0),
        total_sz(0)
    {
    }

    ~mem_access_vec_t()
    {
        clear();
    }

    void reset(uint64_t paddr_base,
               uint32_t elem_count,
               uint32_t elem_sz);
    bool empty(void)
    {
        return total_sz == 0;
    }

    void clear(void)
    {
        mem_access_vec.clear();
        total_sz = 0;
    }

    void get(uint32_t elem_id,
             mem_access_t *access);
    uint64_t pa(void)
    {
        return mem_access_vec.pa();
    }

private:
    FtlLif&                 lif;
    mem_access_t            mem_access_vec;
    uint32_t                elem_count_;
    uint32_t                elem_sz_;
    uint32_t                total_sz;
};

/*
 * Doorbell access wrapper
 */
class db_access_t {
public:
    db_access_t(FtlLif& lif) :
        lif(lif),
        db_access(lif)
    {
        db_addr = {0};
    }

    void reset(enum ftl_qtype qtype,
               uint32_t upd);
    void write32(uint32_t data);

    friend class ftl_lif_queues_ctl_t;

private:
    FtlLif&                 lif;
    asic_db_addr_t          db_addr;
    mem_access_t            db_access;
};

/**
 * Queues control class
 */
class ftl_lif_queues_ctl_t {
public:
    ftl_lif_queues_ctl_t(FtlLif& lif,
                        enum ftl_qtype qtype,
                        uint32_t qcount);
    ~ftl_lif_queues_ctl_t();

    ftl_status_code_t init(const scanners_init_cmd_t *cmd);
    ftl_status_code_t init(const pollers_init_cmd_t *cmd);
    ftl_status_code_t init(const mpu_timestamp_init_cmd_t *cmd);

    ftl_status_code_t start(void);
    ftl_status_code_t sched_start_single(uint32_t qid);
    ftl_status_code_t stop(void);
    ftl_status_code_t sched_stop_single(uint32_t qid);
    ftl_status_code_t dequeue_burst(uint32_t qid,
                                    uint32_t *burst_count,
                                    uint8_t *buf,
                                    uint32_t buf_sz);
    ftl_status_code_t metrics_get(lif_attr_metrics_t *metrics);

    bool quiesce(void);
    void quiesce_idle(void);

    enum ftl_qtype qtype(void) { return qtype_; }
    uint32_t qcount(void) { return qcount_; }
    uint32_t qcount_actual(void) { return qcount_actual_; }
    uint32_t quiesce_qid(void) { return quiesce_qid_; }
    bool queue_empty(uint32_t qid);

    void qstate_access_init(uint32_t elem_sz);
    void wring_access_init(uint64_t paddr_base,
                           uint32_t elem_count,
                           uint32_t elem_sz);
    uint64_t qstate_access_pa(void) { return qstate_access.pa();  }
    uint64_t wring_access_pa(void) { return wring_access.pa();  }
    bool empty_qstate_access(void) { return qstate_access.empty(); }
    bool empty_wring_access(void) { return wring_access.empty(); }
    ftl_status_code_t pgm_pc_offset_get(const char *pc_jump_label,
                                        uint8_t *pc_offset);

private:
    ftl_status_code_t scanner_init_single(const scanner_init_single_cmd_t *cmd);
    ftl_status_code_t poller_init_single(const poller_init_single_cmd_t *cmd);

    FtlLif&                 lif;
    enum ftl_qtype          qtype_;
    mem_access_vec_t        qstate_access;
    mem_access_vec_t        wring_access;
    db_access_t             db_pndx_inc;
    db_access_t             db_sched_clr;
    uint64_t                wrings_base_addr;
    uint32_t                wring_single_sz;
    uint32_t                slot_data_sz;
    uint32_t                qcount_;
    uint32_t                qdepth;
    uint32_t                qdepth_mask;
    uint32_t                qcount_actual_;
    uint32_t                quiesce_qid_;
    uint32_t                quiescing   : 1,
                            unused      : 31;
};

/**
 * MPU timestamp access class
 */
class mpu_timestamp_access_t {
public:
    mpu_timestamp_access_t(FtlLif& lif) :
        lif(lif),
        v_qstate(nullptr)
    {
    }

    void reset(volatile uint8_t *qstate_vaddr)
    {
        v_qstate = (volatile mpu_timestamp_qstate_t *)qstate_vaddr;
    }

    uint64_t curr_timestamp(void)
    {
        /*
         * Only HW platform has VA access to qstate;
         * SIM would not, but then SIM doesn't require timestamp anyway.
         */
        return v_qstate ? v_qstate->timestamp : 0;
    }

private:
    FtlLif&                 lif;
    volatile mpu_timestamp_qstate_t *v_qstate;
};

/**
 * LIF State Machine Context
 */
class ftl_lif_fsm_ctx_t {
public:
    ftl_lif_fsm_ctx_t() :
        state(FTL_LIF_ST_INITIAL),
        enter_state(FTL_LIF_ST_INITIAL),
        initing(0),
        reset(0),
        reset_destroy(0),
        scanners_stopping(0)
    {
    }

    ftl_lif_state_t         state;
    ftl_lif_state_t         enter_state;
    ftl_timestamp_t         ts;
    uint32_t                initing             : 1,
                            reset               : 1,
                            reset_destroy       : 1,
                            scanners_stopping   : 1;
};

/**
 * LIF Resource structure
 */
typedef struct {
    uint64_t                lif_id;
    uint32_t                index;
} ftl_lif_res_t;

typedef struct ftllif_pstate_s {
    ftllif_pstate_s () {
        memset(queue_info, 0, sizeof(queue_info));
        memset(qstate_addr, 0, sizeof(qstate_addr));
        qstate_mem_address = 0;
        qstate_mem_size = 0;
        scanner_pc_offset = 0;
        mpu_timestamp_pc_offset = 0;
        qstate_created_ = false;
    }
    lif_queue_info_t queue_info[NUM_QUEUE_TYPES];
    uint64_t qstate_addr[NUM_QUEUE_TYPES];
    uint64_t qstate_mem_address;
    uint64_t qstate_mem_size;
    uint8_t  scanner_pc_offset;
    uint8_t  mpu_timestamp_pc_offset;
    bool     qstate_created_;

    void qstate_created_set(bool yesno)
    {
        SDK_ATOMIC_STORE_BOOL(&qstate_created_, yesno);
    }

    bool qstate_created(void)
    {
        return SDK_ATOMIC_LOAD_BOOL(&qstate_created_);
    }
} __PACK__ ftllif_pstate_t;

/**
 * Ftl LIF
 */
class FtlLif {
public:
    FtlLif(FtlDev& ftl_dev,
           ftl_lif_res_t& lif_res,
           bool is_soft_init,
           EV_P);
    ~FtlLif();

    ftl_status_code_t CmdHandler(ftl_devcmd_t *req,
                                 void *req_data,
                                 ftl_devcmd_cpl_t *rsp,
                                 void *rsp_data);
    ftl_status_code_t reset(bool destroy);
    void SetHalClient(devapi *dapi);
    void HalEventHandler(bool status);


    uint64_t LifIdGet(void) { return hal_lif_info_.lif_id; }
    const std::string& LifNameGet(void) { return lif_name; }
    uint64_t mpu_timestamp(void) { return mpu_timestamp_access.curr_timestamp(); }
    bool queue_empty(enum ftl_qtype qtype,
                     uint32_t qid);
    void qstate_created_set(bool yesno)
    {
        lif_pstate->qstate_created_set(yesno);
    }

    lif_info_t                  hal_lif_info_;

    friend class ftl_lif_queues_ctl_t;
    friend class mem_access_t;

    static ftl_lif_state_event_t lif_initial_ev_table[];
    static ftl_lif_state_event_t lif_wait_hal_ev_table[];
    static ftl_lif_state_event_t lif_pre_init_ev_table[];
    static ftl_lif_state_event_t lif_post_init_ev_table[];
    static ftl_lif_state_event_t lif_queues_reset_ev_table[];
    static ftl_lif_state_event_t lif_queues_pre_init_ev_table[];
    static ftl_lif_state_event_t lif_queues_init_transition_ev_table[];
    static ftl_lif_state_event_t lif_queues_stopping_ev_table[];
    static ftl_lif_state_event_t lif_scanners_quiesce_ev_table[];
    static ftl_lif_state_event_t lif_timers_quiesce_ev_table[];
    static ftl_lif_state_event_t lif_queues_stopped_ev_table[];
    static ftl_lif_state_event_t lif_queues_started_ev_table[];

private:
    std::string                 lif_name;
    FtlDev&                     ftl_dev;
    const ftl_devspec_t         *spec;
    struct queue_info           pd_qinfo[NUM_QUEUE_TYPES];
    ftl_lif_queues_ctl_t        session_scanners_ctl;
    ftl_lif_queues_ctl_t        conntrack_scanners_ctl;
    ftl_lif_queues_ctl_t        pollers_ctl;
    ftl_lif_queues_ctl_t        mpu_timestamp_ctl;
    mpu_timestamp_access_t      mpu_timestamp_access;

    // PD Info
    PdClient                    *pd;
    // HAL Info
    devapi                      *dev_api;
    uint32_t                    index;
    uint8_t                     cosA, cosB, ctl_cosA, ctl_cosB;

    // Other states
    ftllif_pstate_t             *lif_pstate;
    ftl_lif_fsm_ctx_t           fsm_ctx;
    age_tmo_cb_t                normal_age_tmo_cb;
    age_tmo_cb_t                accel_age_tmo_cb;
    mem_access_t                normal_age_access_;
    mem_access_t                accel_age_access_;

    EV_P;

    void ftl_lif_state_machine(ftl_lif_event_t event,
                               ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_null_action(ftl_lif_event_t event,
                                        ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_null_no_log_action(ftl_lif_event_t event,
                                               ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_same_event_action(ftl_lif_event_t event,
                                              ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_eagain_action(ftl_lif_event_t event,
                                          ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_reject_action(ftl_lif_event_t event,
                                          ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_create_action(ftl_lif_event_t event,
                                          ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_destroy_action(ftl_lif_event_t event,
                                           ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_hal_up_action(ftl_lif_event_t event,
                                          ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_init_action(ftl_lif_event_t event,
                                        ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_setattr_action(ftl_lif_event_t event,
                                           ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_getattr_action(ftl_lif_event_t event,
                                           ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_identify_action(ftl_lif_event_t event,
                                            ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_reset_action(ftl_lif_event_t event,
                                         ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_pollers_init_action(ftl_lif_event_t event,
                                                ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_pollers_flush_action(ftl_lif_event_t event,
                                                 ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_pollers_deq_burst_action(ftl_lif_event_t event,
                                                     ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_scanners_init_action(ftl_lif_event_t event,
                                                 ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_scanners_start_action(ftl_lif_event_t event,
                                                  ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_scanners_start_single_action(ftl_lif_event_t event,
                                                         ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_scanners_stop_action(ftl_lif_event_t event,
                                                 ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_scanners_stop_complete_action(ftl_lif_event_t event,
                                                          ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_scanners_quiesce_action(ftl_lif_event_t event,
                                                    ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_timers_quiesce_action(ftl_lif_event_t event,
                                                  ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_mpu_timestamp_init_action(ftl_lif_event_t event,
                                                      ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_mpu_timestamp_start_action(ftl_lif_event_t event,
                                                       ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_mpu_timestamp_stop_action(ftl_lif_event_t event,
                                                      ftl_lif_devcmd_ctx_t& devcmd_ctx);
    ftl_lif_event_t ftl_lif_accel_aging_ctl_action(ftl_lif_event_t event,
                                                   ftl_lif_devcmd_ctx_t& devcmd_ctx);

    void age_tmo_init(void);
    void normal_age_tmo_cb_set(const lif_attr_age_tmo_t *attr_age_tmo);
    void normal_age_tmo_cb_get(lif_attr_age_tmo_t *attr_age_tmo = nullptr);
    ftl_status_code_t normal_age_tmo_cb_select(void);
    void accel_age_tmo_cb_set(const lif_attr_age_accel_tmo_t *attr_age_tmo);
    void accel_age_tmo_cb_sync(void);
    void accel_age_tmo_cb_get(lif_attr_age_accel_tmo_t *attr_age_tmo = nullptr);
    ftl_status_code_t accel_age_tmo_cb_select(void);
    void force_session_expired_ts_set(uint8_t force_expired_ts);
    void force_conntrack_expired_ts_set(uint8_t force_expired_ts);
    uint8_t force_session_expired_ts_get(void);
    uint8_t force_conntrack_expired_ts_get(void);
    uint64_t timers_quiesce_time_us(void);

    const mem_access_t& normal_age_access(void) { return normal_age_access_; }
    const mem_access_t& accel_age_access(void) { return accel_age_access_; }
};

#endif
