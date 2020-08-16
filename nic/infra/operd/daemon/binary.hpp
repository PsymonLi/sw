//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines class for binary based consumers
///
//----------------------------------------------------------------------------

#ifndef __OPERD_DAEMON_BINARY_HPP__
#define __OPERD_DAEMON_BINARY_HPP__

#include <memory>
#include <string>
#include "nic/sdk/lib/operd/operd.hpp"
#include "nic/infra/operd/daemon/output.hpp"

class binary : public output {
public:
    static std::shared_ptr<binary> factory(std::string path);
    binary(std::string path);
    ~binary();
    void handle(sdk::operd::log_ptr entry) override;

private:
    int fork_exec(void);

private:
    std::string path_;
    int fd_;
    pid_t pid_;
};
typedef std::shared_ptr<binary> binary_ptr;

#endif    // __OPERD_DAEMON_BINARY_HPP__
