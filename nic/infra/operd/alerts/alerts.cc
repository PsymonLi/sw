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
#include "nic/infra/operd/alerts/alerts.hpp"
#include "nic/infra/operd/alerts/alert_type.hpp"

namespace operd {
namespace alerts {

const int MAX_PRINTF_SIZE = 1024;

alert_recorder_ptr alert_recorder::instance_ = nullptr;

alert_recorder_ptr
alert_recorder::get(void) {
    if (instance_ == nullptr) {
        instance_ = std::make_shared<alert_recorder>();
        instance_->region_ = std::make_shared<sdk::operd::region>("alerts");
    }

    return instance_;
}

void
alert_recorder::alert(operd_alerts_t alert, const char *fmt, ...) {
    va_list ap;
    char buffer[MAX_PRINTF_SIZE + sizeof(uint16_t)];
    operd_event_data_t *event_data = (operd_event_data_t *)buffer;
    int n;

    event_data->event_id = alert;
    va_start(ap, fmt);
    n = vsnprintf(event_data->message, MAX_PRINTF_SIZE, fmt, ap);
    va_end(ap);

    this->region_->write(OPERD_DECODER_ALERTS, 1, buffer,
                         n + 1 + sizeof(uint16_t));
}

}    // namespace alerts
}    // namespace operd
