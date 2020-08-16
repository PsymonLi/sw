//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// This file defines class for file logging
///
//----------------------------------------------------------------------------

#ifndef __OPERD_DAEMON_ROTATED_FILE_HPP__
#define __OPERD_DAEMON_ROTATED_FILE_HPP__

#include <memory>
#include <string>
#include "nic/sdk/lib/operd/operd.hpp"
#include "nic/infra/operd/daemon/output.hpp"

class rotated_file : public output {
public:
    static std::shared_ptr<rotated_file> factory(std::string path,
                                                 uint8_t level,
                                                 off_t max_size,
                                                 int max_files);
    rotated_file(std::string path, uint8_t level, off_t max_size,
                 int max_files);
    ~rotated_file();
    void handle(sdk::operd::log_ptr entry) override;

private:
    void open_file_(void);
    void close_file_(void);
    void rotate_files_(void);

private:
    uint8_t level_;
    std::string path_;
    off_t max_size_;
    int max_files_;
    off_t current_size_;
    int fd_;
};
typedef std::shared_ptr<rotated_file> rotated_file_ptr;

#endif    // __OPERD_DAEMON_ROTATED_FILE_HPP__
