/*
* Copyright (c) 2019, Pensando Systems Inc.
*/

#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__

#include <string>
#include "pkt-defs.h"

namespace sdk {
namespace platform {
namespace ncsi {


class transport {

protected:
    std::string xport_name;

public:
    virtual int Init() = 0;
    virtual ssize_t SendPkt(const void *buf, size_t len) = 0;
    virtual ssize_t RecvPkt(void *buf, size_t len, size_t& ncsi_hdr_offset) = 0;
    virtual int GetFd() { return -1; };
    virtual void *GetNcsiRspPkt(ssize_t sz) = 0;
    std::string name() { return xport_name;};
};

} // namespace ncsi
} // namespace platform
} // namespace sdk

#endif //__TRANSPORT_H__

