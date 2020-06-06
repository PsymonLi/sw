//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// HALS timer utlity
//---------------------------------------------------------------

#include "nic/metaswitch/stubs/hals/pds_ms_hals_timer.hpp"

extern NBB_ULONG hals_proc_id;
namespace pds_ms {

bool is_timer_running(NTL_TIMER_FIXED_CB *timer, NBB_ULONG corr)
{
    return (NTL_TIMER_IN_LIST(timer) &&
            (timer->tcb.action_correlator == corr));
}

void timer_start(NTL_TIMER_LIST_CB *timer_list, NTL_TIMER_FIXED_CB *timer,
                 int timer_val, NBB_ULONG corr)
{
    
    NBB_CREATE_THREAD_CONTEXT
    NBS_ENTER_SHARED_CONTEXT(hals_proc_id);
    
    if (!is_timer_running(timer, corr)) {
        PDS_TRACE_DEBUG("Starting timer, correlator %d", corr);
        timer->tcb.action_correlator = corr;
        NTL_TIMER_START(timer_list, timer, timer_val);
    }

    NBS_EXIT_SHARED_CONTEXT();
    NBB_DESTROY_THREAD_CONTEXT

    return;
}

void timer_stop(NTL_TIMER_LIST_CB *timer_list, NTL_TIMER_FIXED_CB *timer)
{
    
    NBB_CREATE_THREAD_CONTEXT
    NBS_ENTER_SHARED_CONTEXT(hals_proc_id);
    
    if (NTL_TIMER_IN_LIST(timer)) {
        PDS_TRACE_DEBUG("Stopping timer");
        NTL_TIMER_CANCEL(timer_list, timer);
    }

    NBS_EXIT_SHARED_CONTEXT();
    NBB_DESTROY_THREAD_CONTEXT

    return;
}

}


