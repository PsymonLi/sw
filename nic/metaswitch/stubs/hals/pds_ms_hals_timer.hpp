//---------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// HALS timer utlity
//---------------------------------------------------------------

#ifndef __PDS_MS_HALS_TIMER_HPP__
#define __PDS_MS_HALS_TIMER_HPP__

#include <nbase.h>
extern "C"
{
#include <ntltimer.h>
}
#include "nic/apollo/api/include/pds.hpp"
#include "nic/metaswitch/stubs/common/pds_ms_state.hpp"

namespace pds_ms {

bool is_timer_running(NTL_TIMER_FIXED_CB *timer, NBB_ULONG corr);
void timer_start(NTL_TIMER_LIST_CB *timer_list, NTL_TIMER_FIXED_CB *timer,
                 int timer_val, NBB_ULONG corr);
void timer_stop(NTL_TIMER_LIST_CB *timer_list, NTL_TIMER_FIXED_CB *timer);

}

#endif
