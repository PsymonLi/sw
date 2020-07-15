//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// condition variable using monotonic clock
///
//----------------------------------------------------------------------------


#include "lib/cond_var/cond_var.hpp"
#include "include/sdk/timestamp.hpp"

//------------------------------------------------------------------------
// GCC implementation for C++ std::condition_variable does not use
// monotonic clock so the timed wait (wait_xxx()) has a bug in that it can
// block forever if the system date were to be modified while the condition
// variable was blocked in a timed wait.
// Ref - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=41861
//
// This is an alternative to std::condition_variable that uses
// pthread monotonic clock underneath
//---------------------------------------------------------------------

namespace sdk {
namespace lib {

// add duration to current monotonic clocktime
static struct timespec
monotonic_time_add_ (uint64_t dur_in_ms)
{
    struct timespec clock_time;
    struct timespec add_ts;

    clock_gettime(CLOCK_MONOTONIC, &clock_time);
    if (dur_in_ms == 0) {
        return clock_time;
    }
    timestamp_from_nsecs (&add_ts, dur_in_ms * TIME_NSECS_PER_MSEC);
    timestamp_add_timespecs(&clock_time, &add_ts);
    return clock_time;
}

cond_var_mutex_t::cond_var_mutex_t(void) {
    pthread_mutexattr_t attr;

    auto rc = pthread_mutexattr_init(&attr);
    if (rc != 0) {
        throw std::system_error(rc, std::system_category(),
                                "condition variable mutex attr init failed");
    }
    do {
        rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        if (rc != 0) {
            break;
        }
        rc = pthread_mutex_init(&mtx_, &attr);
        if (rc != 0) {
            break;
        }
    } while (0);

    pthread_mutexattr_destroy(&attr);

    if (rc != 0) {
        throw std::system_error(rc, std::system_category(),
                                "condition variable mutex init failed");
    }
}

cond_var_t::cond_var_t(void) {
    pthread_condattr_t attr;

    pthread_condattr_init(&attr);
    // set the clock to be monotonic
    // we dont want the timedwait to be affected by change in clock
    auto rc = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    if (rc != 0) {
        throw std::system_error(rc, std::system_category(),
                                "condition variable init setting monotonic clock failed");
    }

    rc = pthread_cond_init(&cv_, &attr);
    if (rc != 0) {
        throw std::system_error(rc, std::system_category(),
                                "condition variable init failed");
    }
}

void cond_var_t::wait(cond_var_mutex_t& mutex, std::function<bool(void)> pred) {
    // continue waiting in case of spurious wake-ups
    while (!pred()) {
        auto rc = pthread_cond_wait(&cv_, &(mutex.mtx_));
        if (rc != 0) {
            // error
            throw std::system_error(rc, std::system_category(),
                "condition variable wait failed");
        }
    }
}

bool cond_var_t::wait_for(cond_var_mutex_t& mutex, uint32_t dur_in_ms,
                          std::function<bool(void)> pred) {
    auto future_time = monotonic_time_add_(dur_in_ms);

    // continue waiting in case of spurious wake-ups
    while (!pred()) {
        int rc = pthread_cond_timedwait(&cv_, &(mutex.mtx_), &future_time);
        if (rc == ETIMEDOUT) {
            // timeout - check if condition satisfied
            return pred();
        }
        if (rc != 0) {
            // error
            throw std::system_error(rc, std::system_category(),
                "condition variable timed wait failed");
        }
    }
    return true;
}

}    // end namespace lib
}    // end namespace sdk
