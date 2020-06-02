// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include <sched.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <iostream>
#include <vector>

#include <nic/vpp/infra/shm_ipc/shm_ipc.h>
#include <types.pb.h>
#include <session_internal.pb.h>

//const int count = 128;
const int count = 2000000;
//const int count = 10;

struct Sinfo {
    uint32_t sessid;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint32_t src_port;
    uint32_t dst_port;
};

static bool first;
static std::vector<Sinfo>sessq;

static void
decode_one_session (Sinfo &sinfo, ::vppinternal::SessV4Info &info)
{
    sinfo.sessid = info.sessid();
    sinfo.src_ip = info.isrcip();
    sinfo.dst_ip = info.idstip();
    sinfo.src_port = info.isrcport();
    sinfo.dst_port = info.idstport();
}

static bool
q_rdcb (const uint8_t *data, const uint8_t len, void *opaq)
{
    static ::vppinternal::SessV4Info info;
    Sinfo sinfo;

    if (!first) {
        first = true;
        struct timespec *ts = (struct timespec *)opaq;
        clock_gettime(CLOCK_MONOTONIC_RAW, ts);
    }
    info.Clear();
    info.ParseFromArray(data, len);
    decode_one_session(sinfo, info);
    sessq.push_back(sinfo);
    return true;
}

static void
affine_cpu (bool odd_cpu)
{
    cpu_set_t set, target;

    CPU_ZERO(&set);
    CPU_ZERO(&target);
    sched_getaffinity(0, sizeof(set), &set);
    int i = odd_cpu ? 1 : 0;
    for (; i < CPU_SETSIZE; i += 2) {
        if (CPU_ISSET(i, &set)) {
            CPU_SET(i, &target);
            std::cout << "Affining to cpu " << i << "\n";
            break;
        }
    }
    sched_setaffinity(0, sizeof(target), &target);
}

int
main(int argc, char* argv[ ])
{
    struct timespec start, end, dtime;
    struct shm_ipcq *q;

    affine_cpu(false);
    shm_ipc_init();
    q = shm_ipc_attach("testq");

    sessq.reserve(count);
    // now receive messages from the queue
    while (shm_ipc_recv_start(q, q_rdcb, &start) != -1);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    assert(sessq.size() == count);

    if (end.tv_nsec < start.tv_nsec) {
        end.tv_sec--;
        end.tv_nsec += 1000000000;
    }
    dtime.tv_sec = end.tv_sec - start.tv_sec;
    dtime.tv_nsec = end.tv_nsec - start.tv_nsec;
    std::cout << "Reads " << sessq.size() << "\n";
    std::cout << "Time: " << dtime.tv_sec << " : " << dtime.tv_nsec << "\n";
#if 0
    for( auto i : sessq) {
        std::cout << "Session Id: " << i.sessid() << "\n";
    }
#endif
    return 0;
}
