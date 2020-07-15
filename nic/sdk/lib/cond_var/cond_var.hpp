//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// condition variable using monotonic clock
///
//----------------------------------------------------------------------------

#ifndef __NIC_SDK_COND_VAR_HPP__
#define __NIC_SDK_COND_VAR_HPP__

#include <pthread.h>
#include <system_error>
#include <functional>

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

class cond_var_mutex_guard_t;
class cond_var_t;

// mutex to be used with the sdk condition variable
class cond_var_mutex_t {
public:
    cond_var_mutex_t(void);
    ~cond_var_mutex_t(void) {
        pthread_mutex_destroy(&mtx_);
    }
    void lock(void) {
        pthread_mutex_lock(&(mtx_));
    }
    void unlock(void) {
        pthread_mutex_unlock(&(mtx_));
    }
    friend class cond_var_t;

private:
    pthread_mutex_t mtx_;
};

// RAII guard for the mutex
class cond_var_mutex_guard_t {
public:
     // locks the underlying mutex on construction
     cond_var_mutex_guard_t(cond_var_mutex_t* cond_var_mutex)
         : mtx_ (cond_var_mutex) {
         mtx_->lock();
     }
     // releases the underlying mutex before going out of scope
     ~cond_var_mutex_guard_t(void) {
         if (mtx_ != nullptr) {
             mtx_->unlock();
         }
     }
private:
     cond_var_mutex_t* mtx_ = nullptr;
};

// condition variable with monotonic timed wait
class cond_var_t {
public:
    // constructor - initializes condition variable with montonic clock
    cond_var_t(void);

    // wait for the condition variable to be signaled when predicate function
    // evaluates to true.
    // throws std::system_error if invalid mutex is passed in or if mutex is
    // not already owned by calling thread
    void wait(cond_var_mutex_t& mutex, std::function<bool(void)> predicate_fn);

    // timed wait for the condition variable to be signaled when the predicate
    // function evaluates to true.
    // returns true if the predicate becomes true
    //         false if timer expires before predicate becomes true
    // throws std::system_error if invalid duration or mutex is passed in or
    // if mutex is not already owned by calling thread
    bool wait_for(cond_var_mutex_t& mutex, uint32_t dur_in_ms,
                  std::function<bool(void)> predicate_fn);

    void notify_all(void) noexcept {
        pthread_cond_broadcast(&cv_);
    }
    void notify_one(void) noexcept {
        pthread_cond_signal(&cv_);
    }
private:
    pthread_cond_t cv_;
};

}    // end namespace lib
}    // end namespace sdk

#endif
