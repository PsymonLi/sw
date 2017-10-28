// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <atomic>
#include "nic/hal/hal.hpp"
#include "hal_control.hpp"
#include "port.hpp"
#include "nic/include/asic_pd.hpp"

namespace hal {
namespace pd {

// per producer request queues
hal_ctrl_queue_t g_hal_ctrl_workq[HAL_THREAD_ID_MAX];

//------------------------------------------------------------------------------
// hal-control thread's forever loop
//------------------------------------------------------------------------------
static void
hal_control_loop (void)
{
    uint32_t           qid;
    uint16_t           cindx;
    bool               work_done = false;
    bool               rv = true;
    hal_ctrl_entry_t   *rw_entry;

    while (TRUE) {
        work_done = false;
        for (qid = 0; qid < HAL_THREAD_ID_MAX; qid++) {
            if (!g_hal_ctrl_workq[qid].nentries) {
                // no read/write requests
                continue;
            }

            // found a read/write request to serve
            cindx = g_hal_ctrl_workq[qid].cindx;
            rw_entry = &g_hal_ctrl_workq[qid].entries[cindx];
            switch (rw_entry->opn) {
                case HAL_CONTROL_OPERATION_PORT_TIMER:
                case HAL_CONTROL_OPERATION_PORT_ENABLE:
                case HAL_CONTROL_OPERATION_PORT_DISABLE:
                    port::port_event_notify(rw_entry->opn, rw_entry->data);
                    break;

                default:
                    HAL_TRACE_ERR("Invalid operation {}", rw_entry->opn);
                    rv = false;
                    break;
            }

            // populate the results
            rw_entry->status =  rv ? HAL_RET_OK : HAL_RET_ERR;
            rw_entry->done.store(true);

            // advance to next entry in the queue
            g_hal_ctrl_workq[qid].cindx++;
            if (g_hal_ctrl_workq[qid].cindx >= HAL_CONTROL_Q_SIZE) {
                g_hal_ctrl_workq[qid].cindx = 0;
            }
            g_hal_ctrl_workq[qid].nentries--;
            work_done = true;
        }

        // all queues scanned once, check if any work was found
        if (!work_done) {
            // didn't find any work, yield and give chance to other threads
            pthread_yield();
        }
    }
}

//------------------------------------------------------------------------------
// hal-control thread notification by other threads
//------------------------------------------------------------------------------
hal_ret_t
hal_control_notify (uint8_t operation, void *ctxt)
{
    uint16_t           pindx;
    hal::utils::thread *curr_thread = hal::utils::thread::current_thread();
    uint32_t           curr_tid = curr_thread->thread_id();
    hal_ctrl_entry_t   *rw_entry;

    if (g_hal_ctrl_workq[curr_tid].nentries >= HAL_CONTROL_Q_SIZE) {
        HAL_TRACE_ERR("Error: operation {} for thread {}, tid {} full",
                      operation, curr_thread->name(), curr_tid);
        return HAL_RET_HW_PROG_ERR;
    }
    pindx = g_hal_ctrl_workq[curr_tid].pindx;

    rw_entry = &g_hal_ctrl_workq[curr_tid].entries[pindx];
    rw_entry->opn = operation;
    rw_entry->status = HAL_RET_ERR;
    rw_entry->data = ctxt;
    rw_entry->done.store(false);

    g_hal_ctrl_workq[curr_tid].nentries++;

    while (rw_entry->done.load() == false) {
        if (curr_thread->can_yield()) {
            pthread_yield();
        }
    }

    // move the producer index to next slot.
    // consumer is unaware of the blocking/non-blocking call and always
    // moves to the next slot.
    g_hal_ctrl_workq[curr_tid].pindx++;
    if (g_hal_ctrl_workq[curr_tid].pindx >= HAL_CONTROL_Q_SIZE) {
        g_hal_ctrl_workq[curr_tid].pindx = 0;
    }

    return rw_entry->status;
}

//------------------------------------------------------------------------------
// hal-control thread's entry point
//------------------------------------------------------------------------------
void*
hal_control_start (void *ctxt)
{
    HAL_THREAD_INIT(ctxt);

    hal_cfg_t *hal_cfg =
                (hal_cfg_t *)hal::utils::thread::current_thread()->data();
    if (hal_cfg == NULL) {
        return NULL;
    }

    // start the asic-rw thread
    HAL_TRACE_DEBUG("Starting asic-rw thread ...");
    g_hal_threads[HAL_THREAD_ID_ASIC_RW] =
        thread::factory(std::string("asic-rw").c_str(),
                HAL_THREAD_ID_ASIC_RW, HAL_CONTROL_CORE_ID,
                hal::pd::asic_rw_start,
                sched_get_priority_max(SCHED_RR), SCHED_RR, true);
    HAL_ABORT(g_hal_threads[HAL_THREAD_ID_ASIC_RW] != NULL);

    // set custom data
    g_hal_threads[HAL_THREAD_ID_ASIC_RW]->set_data(hal_cfg);

    // invoke with thread instance reference
    g_hal_threads[HAL_THREAD_ID_ASIC_RW]->start(
                            g_hal_threads[HAL_THREAD_ID_ASIC_RW]);

    HAL_TRACE_DEBUG("Waiting for asic-rw thread to be ready ...");
    // wait for ASIC RW thread to be ready
    while (!is_asic_rw_ready()) {
        pthread_yield();
    }

    // initialize the port mac and serdes functions
    hal::pd::port::port_init(true /* is_sim */);

    // keep polling the queue and serve requests
    hal_control_loop();

    HAL_TRACE_DEBUG("hal-control thread done");

    return NULL;
}

}    // namespace pd
}    // namespace hal
