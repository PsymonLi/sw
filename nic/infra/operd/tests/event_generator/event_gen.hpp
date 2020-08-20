//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines events generator
///
//----------------------------------------------------------------------------

#ifndef __EVENT_GEN_HPP__
#define __EVENT_GEN_HPP__

#include "nic/infra/operd/event/event.hpp"
#include "gen/event/event_defs.h"

using namespace std;
using namespace operd::event;

namespace test {

class event_generator {
public:

    /// \brief factory method to allocate & initialize event_generator instance
    /// \return new instance of event_generator or NULL, in case of error
    static event_generator *factory(int rps=1, int total=1,
                                    int type=-1, string pfx="GEN");

    /// \brief free memory allocated to event_generator instance
    /// \param[in] event_generator pointer to event_generator instance
    static void destroy(event_generator *event_generator);

    /// \brief generates event
    void generate_event(void);

    /// \brief displays the configuration
    void show_config(void);

private:
    /// \brief parameterized constructor
    event_generator(int rps, int total, int type, string pfx) {
        rps_ = rps;
        total_ = total;
        type_ = type;
        msg_pfx_ = pfx;
        recorder_ = event_recorder::get();
    }

    /// \brief destructor
    ~event_generator() {}

    /// \brief returns the event type to be generated
    /// \return event type if configured else random supported event type
    int get_type_(void);

    /// \brief raise single event based on config
    void raise_event_(void);

public:
    static constexpr int k_max_event_types = OPERD_EVENT_TYPE_MAX;

private:
    int rps_;                        ///< genaration rate per second
    int total_;                      ///< number of event to be generated
    int type_;                       ///< type of event to be generated
    string msg_pfx_;                 ///< string to be prefixed with event msg
    event_recorder_ptr recorder_;    ///< operd event recorder

};

}    // namespace test
#endif   // __EVENT_GEN_HPP__