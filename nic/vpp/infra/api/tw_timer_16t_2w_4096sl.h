//
//  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
//
//  timer template for 16 Timers, 2 Wheels and 4096 slots

#ifndef __included_tw_timer_16t_2w_4096sl_h__
#define __included_tw_timer_16t_2w_4096sl_h__

#undef TW_TIMER_WHEELS
#undef TW_SLOTS_PER_RING
#undef TW_RING_SHIFT
#undef TW_RING_MASK
#undef TW_TIMERS_PER_OBJECT
#undef LOG2_TW_TIMERS_PER_OBJECT
#undef TW_SUFFIX
#undef TW_OVERFLOW_VECTOR
#undef TW_FAST_WHEEL_BITMAP
#undef TW_TIMER_ALLOW_DUPLICATE_STOP
#undef TW_START_STOP_TRACE_SIZE

#define TW_TIMER_WHEELS 2
#define TW_SLOTS_PER_RING 4096
#define TW_RING_SHIFT 12 
#define TW_RING_MASK (TW_SLOTS_PER_RING - 1)
#define TW_TIMERS_PER_OBJECT 16
#define LOG2_TW_TIMERS_PER_OBJECT 4
#define TW_SUFFIX _16t_2w_4096sl
#define TW_FAST_WHEEL_BITMAP 0
#define TW_TIMER_ALLOW_DUPLICATE_STOP 1

#include <vppinfra/tw_timer_template.h>

#endif /* __included_tw_timer_16t_2w_4096sl_h__ */

