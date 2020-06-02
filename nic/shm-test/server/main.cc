// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include <sched.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <iostream>
#include <vector>

#include <nic/p4/common/defines.h>
#include <nic/vpp/infra/shm_ipc/shm_ipc.h>
#include <types.pb.h>
#include <session_internal.pb.h>

//const int count = 128;
const int count = 2000000;
//const int count = 10;
const char *dst_ip_str = "20.0.0.1";
const char *src_ip_str = "10.0.0.1";
const uint32_t dst_port = 5555;
const uint32_t src_port = 4444;

struct Sinfo {
    uint32_t sessid;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint32_t src_port;
    uint32_t dst_port;
};

uint32_t dst_ip;
uint32_t src_ip;
uint32_t sessid;

static int encodes;
static int qfull;
static std::vector<Sinfo>sessq;

static void
create_one_session (Sinfo &sinfo)
{
    sinfo.sessid = ++sessid;
    sinfo.src_ip = src_ip;
    sinfo.dst_ip = dst_ip;
    sinfo.src_port = src_port;
    sinfo.dst_port = dst_port;

    src_ip++;
}

static void
encode_one_session (Sinfo &sinfo, ::vppinternal::SessV4Info &info)
{
    info.set_sessid(sinfo.sessid);
    info.set_proto(IP_PROTO_UDP);
    info.set_state(::vppinternal::EST);
    info.set_subtype(::vppinternal::L2R_INTER_SUBNET);
    info.set_svctype(::vppinternal::NONE);
    info.set_ingressbd(10);
    info.set_vnicid(15);
    info.set_islocaltolocal(false);
    info.set_ismisshit(false);
    info.set_isflowdrop(false);
    info.set_isiflowrx(false);
    // don't encode NAT related fields

    // initiator flow attributes - client to server
    info.set_isrcip(sinfo.src_ip);
    info.set_idstip(sinfo.dst_ip);
    info.set_isrcport(sinfo.src_port);
    info.set_idstport(sinfo.dst_port);
    info.set_inhid(1);
    info.set_inhtype(3);

    // responder flow attributes - server to client
    info.set_rsrcip(sinfo.dst_ip);
    info.set_rdstip(sinfo.src_ip);
    info.set_rsrcport(sinfo.dst_port);
    info.set_rdstport(sinfo.src_port);
    info.set_rnhid(2);
    info.set_rnhtype(4);
    info.set_egressbd(20);
}

static void
create_sessions (void)
{
    // Create 'count' number of  session, each sesion has two flow entries for 
    // either direction.
    for (auto i = 0; i < count; i++) {
        Sinfo info;
        create_one_session(info);
        sessq.push_back(info);
    }
    std::cout << "Created " << sessq.size() << " session objects\n";
}

struct bookmark {
    std::vector<Sinfo>::iterator iter;
    std::vector<Sinfo>::iterator end;
};

// Encode the current session into the passed buffer and update
// current session pointer.
static bool
q_wrcb (uint8_t *data, uint8_t *len, void *opaq)
{
    bookmark *bmark = static_cast<bookmark *>(opaq);

    //assert(bmark->iter != bmark->end);
    static ::vppinternal::SessV4Info info;
    info.Clear();
    encode_one_session(*bmark->iter, info);
    size_t req_sz = info.ByteSizeLong(); 
    //assert(req_sz < 192);
    *len = req_sz;
    info.SerializeWithCachedSizesToArray(data);

    encodes++;
    bmark->iter++;
    return bmark->iter != bmark->end ? true : false;
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

    affine_cpu(true);
    shm_ipc_init();
    q = shm_ipc_create("testq");

    // initalize
    sessq.reserve(count);
    dst_ip = inet_addr(dst_ip_str);
    src_ip = inet_addr(src_ip_str);
    
    // Create the sessions
    create_sessions();

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    // now encode ther sessions to the queue
    struct bookmark bmark;
    bmark.iter = sessq.begin();
    bmark.end = sessq.end();
    do {
        if (shm_ipc_send_start(q, q_wrcb, &bmark) == false) {
            qfull++;
        }
    } while(bmark.iter != bmark.end);
    while(shm_ipc_send_eor(q) == false);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    if (end.tv_nsec < start.tv_nsec) {
        end.tv_sec--;
        end.tv_nsec += 1000000000;
    }
    dtime.tv_sec = end.tv_sec - start.tv_sec;
    dtime.tv_nsec = end.tv_nsec - start.tv_nsec;
    std::cout << "Writes " << encodes << " Qfull: " << qfull << "\n";
    std::cout << "Time: " << dtime.tv_sec << " : " << dtime.tv_nsec << "\n";
    return 0;
}
