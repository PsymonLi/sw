//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines class for consumers
///
//----------------------------------------------------------------------------

#ifndef __OPERD_DAEMON_OUTPUT_HPP__
#define __OPERD_DAEMON_OUTPUT_HPP__

#include <memory>
#include "nic/sdk/lib/operd/operd.hpp"

class output {
public:
    virtual void handle(sdk::operd::log_ptr entry) = 0;
};
typedef std::shared_ptr<output> output_ptr;

#endif    // __OPERD_DAEMON_OUTPUT_HPP__
