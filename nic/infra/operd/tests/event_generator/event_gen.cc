//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains implementation of event generator
///
//----------------------------------------------------------------------------

#include <chrono>
#include <iostream>
#include "nic/infra/operd/tests/event_generator/event_gen.hpp"

using namespace std;

namespace test {

event_generator *
event_generator::factory(int rps, int total, int type, string pfx) {
    return new event_generator(rps, total, type, pfx);
}

void
event_generator::destroy(event_generator *event_generator_inst) {
    delete event_generator_inst;
}

static inline string
get_event_name (int event_type)
{
    string event_name;

    if (event_type == -1) {
        event_name = "random";
    } else {
        event_name = operd::event::event[event_type].name;
    }

    return event_name;
}

void
event_generator::show_config(void) {
    cout << "Types of event supported: " << k_max_event_types << endl;
    cout << "Events rate per second: " << rps_ << endl;
    cout << "Total no. of event to be generated: " << total_ << endl;
    cout << "Event msg prefix: " << msg_pfx_ << endl;
    cout << "Event type: " << get_event_name(type_) << endl;
}

void
event_generator::generate_event(void) {
    int num_event_raised = 0;
    int time_elapsed;
    using micros = chrono::microseconds;

    while (num_event_raised < total_) {
        auto start = chrono::steady_clock::now();
        for (int i = 0; i < rps_; ++i) {
            raise_event_();
        }
        num_event_raised += rps_;
        auto finish = chrono::steady_clock::now();
        time_elapsed = chrono::duration_cast<micros>(finish - start).count();
        usleep(1000000 - time_elapsed);
    }
    cout << "No. of event generated " << total_ << endl;
}

int
event_generator::get_type_(void) {
    if (type_ == -1) {
        return (rand() % k_max_event_types);
    }
    return type_;
}

static inline string
get_event_msg (void)
{
    time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    char timestring[30];

    strftime(timestring, 30, " : event %Y%m%d%H%M%S", localtime(&now));
    return string(timestring);
}

void
event_generator::raise_event_(void) {
    operd_event_type_t event_type;
    string event_msg;

    event_type = (operd_event_type_t)get_type_();
    event_msg = msg_pfx_ + get_event_msg();
#if 0
    cout << "Raising event " << get_event_name(event_type)
         << " with msg " <<  event_msg << endl;
#endif
    recorder_->event(event_type, event_msg.c_str());
    return;
}

}    // namespace test
