//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// Gtest for condition variable using monotonic clock
///
//----------------------------------------------------------------------------

#include "lib/cond_var/cond_var.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <vector>

sdk::lib::cond_var_t       g_cv_monotonic;
sdk::lib::cond_var_mutex_t g_cv_mtx;
bool                       g_simple_wait_pred = false;
std::vector<int>           g_batch_queue;
constexpr int              k_max_batch_sz = 30;
 // freq at which items are added to batch in milisec
constexpr int              k_freq = 100;

void
simple_wait_signal_thread_main (void)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::cout << "signal cond var when pred is false" << std::endl;
    g_cv_monotonic.notify_all();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::cout << "signal cond var when pred is true" << std::endl;
    {
        // acquire mutex before changing predicate condition
        sdk::lib::cond_var_mutex_guard_t lg(g_cv_mtx);
        g_simple_wait_pred = true;
    }
    g_cv_monotonic.notify_all();
}

bool
simple_wait_pred_chk (void)
{
    return g_simple_wait_pred;
}

// test cond var simple wait based on condition
TEST(cond_var, simple_wait) {
    // separate thread to signal condition
    std::thread thr(simple_wait_signal_thread_main);

    // main thread that blocks on condition
    try {
        {
            // acquire mutex before blocking for condition
            sdk::lib::cond_var_mutex_guard_t lg(g_cv_mtx);
            // wait until predicate becomes true
            g_cv_monotonic.wait(g_cv_mtx, simple_wait_pred_chk);
        }   // exit context to release mutex
        ASSERT_TRUE(g_simple_wait_pred) << "wait() returned when predicate is false";

        thr.join();
    } catch (const std::system_error& e) {
        ASSERT_TRUE(0) << "errcode " << e.code() << " errstr " << e.what();
    }
}

// separate thread to signal condition
void
batching_thread_main (int count)
{
    int i = 0;

    while (g_batch_queue.size() < count) {
        // push to batch queue and signal cond-var every 100 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(k_freq));
        std::cout << "pushed "<< g_batch_queue.size() << " items" << std::endl;
        {
            // acquire mutex before changing predicate condition
            sdk::lib::cond_var_mutex_guard_t lg(g_cv_mtx);
            g_batch_queue.push_back(++i);
        }
        g_cv_monotonic.notify_one();
    }
};

bool
batch_size_chk (void)
{
    return (g_batch_queue.size() > k_max_batch_sz);
}

// test cond var timed wait based on condition
// max batch size reached before batching timer expires
TEST(cond_var, batching_predicate_test) {
    // separate thread to batch and signal condition
    std::thread thr(batching_thread_main, k_max_batch_sz+5);

    // main thread that blocks on condition
    try {
        bool ret;
        {
            // acquire mutex before blocking for condition
            sdk::lib::cond_var_mutex_guard_t lg(g_cv_mtx);
            // allow sufficient time to reach max batch size
            int interval = (k_max_batch_sz + 5) * k_freq;
            // wait until either batch size exceeds limit or timer expires
            ret = g_cv_monotonic.wait_for(g_cv_mtx, interval, batch_size_chk);
            std::cout << "wait_for() returned when batch_sz = "
                      << g_batch_queue.size() << std::endl;
            // wait_for() should have returned before timer expired
            ASSERT_TRUE(g_batch_queue.size() == k_max_batch_sz+1)
                      << "wait_for() did not return when batch max was reached"
                         " - actual batch_sz = " << g_batch_queue.size();
        }   // exit context to release mutex
        // wait_for() should have returned true since batch condtion is
        // satisfied before timer expires
        ASSERT_TRUE(ret) << "wait_for() returned false when predicate is true";

        thr.join();
        g_batch_queue.clear();
    } catch (const std::system_error& e) {
        ASSERT_TRUE(0) << "errcode " << e.code() << " errstr " << e.what();
    }
}

// test cond var timed wait based on condition
// batching timer expires before max batch size is reached
TEST(cond_var, batching_timeout_test) {
    // separate thread to batch and signal condition
    std::thread thr(batching_thread_main, k_max_batch_sz+5);

    // main thread that blocks on condition
    try {
        bool ret;
        {
            // acquire mutex before blocking for condition
            sdk::lib::cond_var_mutex_guard_t lg(g_cv_mtx);
            // set interval to less than time taken to reach max batching size
            int interval = (k_max_batch_sz / 2) * k_freq;
            // wait until either batch size exceeds limit or timer expires
            ret = g_cv_monotonic.wait_for(g_cv_mtx, interval, batch_size_chk);
            std::cout << "wait_for() returned when batch_sz = "
                      << g_batch_queue.size() << std::endl;
            // wait_for() should have returned after timer expired
            ASSERT_TRUE(g_batch_queue.size() <= k_max_batch_sz/2)
                        << "wait_for() did not return when timer popped";
            ASSERT_TRUE(g_batch_queue.size() >= (k_max_batch_sz/2)-1)
                        << "wait_for() returned before timer popped";
        }   // exit context to release mutex
        // wait_for() should have returned false since condtion is still
        // not satisfied when timer expires
        ASSERT_FALSE(ret) << "wait_for() returned true even when predicate"
                             " is false count " << g_batch_queue.size();
        thr.join();
        g_batch_queue.clear();
    } catch (const std::system_error& e) {
        ASSERT_TRUE(0) << "errcode " << e.code() << " errstr " << e.what();
    }
}

int main (int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
