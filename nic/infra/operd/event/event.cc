//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file contains definitions of event APIs for producers
///
//----------------------------------------------------------------------------

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "nic/sdk/lib/operd/decoder.h"
#include "nic/sdk/lib/operd/operd.hpp"
#include "nic/infra/operd/event/event.hpp"
#include "nic/infra/operd/event/event_type.hpp"

namespace operd {
namespace event {

const int MAX_PRINTF_SIZE = 1024;

event_recorder_ptr event_recorder::instance_ = nullptr;

event_recorder_ptr
event_recorder::get(void) {
    if (instance_ == nullptr) {
        instance_ = std::make_shared<event_recorder>();
        instance_->region_ = std::make_shared<sdk::operd::region>("event");
    }

    return instance_;
}

void
event_recorder::event(operd_event_type_t event_type, const char *fmt, ...) {
    va_list ap;
    char buffer[MAX_PRINTF_SIZE + sizeof(uint16_t)];
    operd_event_data_t *event_data = (operd_event_data_t *)buffer;
    int n;

    event_data->event_id = event_type;
    va_start(ap, fmt);
    n = vsnprintf(event_data->message, MAX_PRINTF_SIZE, fmt, ap);
    va_end(ap);

    this->region_->write(OPERD_DECODER_EVENT, 1, buffer,
                         n + 1 + sizeof(uint16_t));
}

}    // namespace event
}    // namespace operd
