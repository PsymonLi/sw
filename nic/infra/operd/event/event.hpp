//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines event APIs for producers
///
//----------------------------------------------------------------------------

#ifndef __OPERD_EVENT_HPP__
#define __OPERD_EVENT_HPP__

#include <stdarg.h>
#include <memory>
#include <string>
#include "nic/sdk/lib/operd/operd.hpp"
#include "nic/sdk/lib/operd/region.hpp"
#include "gen/event/event_defs.h"

namespace operd {
namespace event {

class event_recorder {
public:
    static std::shared_ptr<event_recorder> get(void);
    void event(operd_event_type_t event, const char *fmt, ...)
        __attribute__ ((format (printf, 3, 4)));
private:
    static std::shared_ptr<event_recorder> instance_;
    sdk::operd::region_ptr region_;
};
typedef std::shared_ptr<event_recorder> event_recorder_ptr;

}    // namespace event
}    // namespace operd

#endif    // __OPERD_EVENT_HPP__
