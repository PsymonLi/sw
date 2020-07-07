// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include <string>
#include <vector>

#include "include/sdk/base.hpp"
#include "include/sdk/mem.hpp"
#include "lib/shmmgr/shmmgr.hpp"
#include "shm_ipc.h"
#include "shm_ipc_private.hpp"

static const int BATCH_SIZE = 16;
using mshm = boost::interprocess::managed_mapped_file;

struct shm_ipcq {
    shm_ipcq(const char *name, bool is_producer, mshm *shmp);
    void destroy();

    spsc_q *aq;
    spsc_q *sq;
    uint64_t reads, writes;
    uint32_t old_entries;
    std::vector<BufferObj *>freeq;
    boost::interprocess::managed_mapped_file *shm;
    std::string qname;
    std::string sq_name;
    std::string aq_name;
    bool producer;
};

shm_ipcq::shm_ipcq (const char *name, bool is_producer, mshm *shmp) :
    shm(shmp), qname(name), sq_name(qname + "_syncq"), aq_name(qname + "_ackq"),
    producer(is_producer)
{
    if (producer) {
        freeq.reserve(ipc_qsize);
        // Create the free list
        BufferObj *ptr;
        std::pair<BufferObj *, size_t> pret;
        pret = shm->find<BufferObj>(qname.c_str());
        if (pret.second == NUM_BUFFERS) {
            ptr = pret.first;
        } else {
            // cleanup remnant of the past - This will trip the reader if a reader was
            // active at this instance :(
            shm->destroy<BufferObj>(qname.c_str());
            ptr = shm->construct<BufferObj>(qname.c_str(),std::nothrow)[NUM_BUFFERS]();
        }
        SDK_ASSERT(ptr);
        for (uint32_t i = 0; i < ipc_qsize; i++) {
            freeq.push_back(ptr + i);
        }
    }
    // SPSC rings in the shared memory
    sq = shm->find_or_construct<spsc_q>(sq_name.c_str(), std::nothrow)();
    SDK_ASSERT(sq);
    aq = shm->find_or_construct<spsc_q>(aq_name.c_str(), std::nothrow)();
    SDK_ASSERT(aq);
    // Drain the ackq now as Buffer objects are always constructed afresh.
    // For the syncq, we can't drain, but we note its current size.
    // So when we handle the ackq, we won't call free on the first 'old_entries'
    // entry we dequeue.
    reads = writes = 0;
    old_entries = 0;
    if (producer) {
        // if there was a reader while we are here, the loop below will
        // help arrive at the right 'old_entries' to ignore.
        do {
            old_entries = ipc_qsize - sq->write_available();
            ptr_handle_t dummy;
            while(aq->pop(dummy));
        } while (old_entries != ipc_qsize - sq->write_available());
        printf("Found %u remnant entries yet to be read\n", old_entries);
    }
}

void
shm_ipcq::destroy (void)
{
    if (producer) {
        shm->destroy<BufferObj>(qname.c_str());
        shm->destroy_ptr(sq);
        shm->destroy_ptr(aq);
    }
}

// A simple wrapper class over sdk::lib::shmmgr and to manage a collection
// of shm_ipcq objects using the shmmgr
class shm_ipcq_mgr {
public:
    static shm_ipcq_mgr *factory();
    static void destroy(shm_ipcq_mgr *mgr);
    shm_ipcq * findq(const char *name);
    shm_ipcq * createq(const char *name, bool is_producer);
    void deleteq(const char *name);
    
private:
    shm_ipcq_mgr() : shm(nullptr) { ipcqs.reserve(num_qs); };
    ~shm_ipcq_mgr(){};
    bool init();

    sdk::lib::shmmgr *shm;
    std::vector<shm_ipcq> ipcqs;
};

// Singleton
static shm_ipcq_mgr *qmgr;

shm_ipcq *
shm_ipcq_mgr::findq (const char *qname)
{
    for (auto iter = ipcqs.begin(); iter != ipcqs.end(); ++iter) {
        if (iter->qname == qname) {
            return ipcqs.data() + std::distance(iter, ipcqs.begin());
        }
    }
    return nullptr;
}

shm_ipcq *
shm_ipcq_mgr::createq (const char *qname, bool is_producer)
{
    shm_ipcq *ret = findq(qname);
    if (ret) {
        return ret;
    }
    // Prevent the vector from growing beyond its initial size (num_qs).
    // We return the address and therefore we never want the vector to
    // resize.
    if (ipcqs.size() == num_qs) {
        return nullptr;
    }
    ipcqs.emplace_back(qname, is_producer, static_cast<mshm *>(shm->mmgr()));
    ret = findq(qname);
    SDK_ASSERT(ret);
    return ret;
}

void
shm_ipcq_mgr::deleteq (const char *qname)
{
    for (auto iter = ipcqs.begin(); iter != ipcqs.end(); ++iter) {
        if (iter->qname == qname) {
            ipcqs.erase(iter);
            break;
        }
    }
}

bool
shm_ipcq_mgr::init (void)
{
    shm = sdk::lib::shmmgr::factory("/share/vpp_ipc_memstore", SHM_IPC_MEMSTORE_SIZE,
                                    sdk::lib::SHM_OPEN_OR_CREATE);
    return shm ? true : false;
}

shm_ipcq_mgr *
shm_ipcq_mgr::factory (void)
{
    void *mem;
    shm_ipcq_mgr *new_q;

    mem = SDK_CALLOC(SDK_MEM_ALLOC_LIB_SHM, sizeof(shm_ipcq_mgr));
    if (!mem) {
        return nullptr;
    }
    new_q = new (mem) shm_ipcq_mgr();
    if (new_q->init() == false) {
        new_q->~shm_ipcq_mgr();
        SDK_FREE(SDK_MEM_ALLOC_LIB_SHM, mem);
        new_q = nullptr;
    }
    return new_q;
}

void
shm_ipcq_mgr::destroy (shm_ipcq_mgr *mgr)
{
    if (mgr) {
        if (mgr->shm) {
            sdk::lib::shmmgr::destroy(mgr->shm);
        }
        mgr->~shm_ipcq_mgr();
        SDK_FREE(SDK_MEM_ALLOC_LIB_SHM, mgr);
    }
}

// C API implementation

void
shm_ipc_init (void)
{
    if (!qmgr) {
        qmgr = shm_ipcq_mgr::factory();
    }
}

void
shm_ipc_destroy (const char *qname)
{
    shm_ipcq *q;
    if (qmgr && (q = qmgr->findq(qname))) {
        q->destroy();
        qmgr->deleteq(qname);
    }
}

shm_ipcq *
shm_ipc_create (const char *qname)
{
    if (qmgr) {
        return qmgr->createq(qname, true);
    }
    return nullptr;
}

shm_ipcq *
shm_ipc_attach (const char *qname)
{
    if (qmgr) {
        return qmgr->createq(qname, false);
    }
    return nullptr;
}

bool
shm_ipc_send_start (shm_ipcq *q, encode_cb enq_cb, void *opaq)
{
    ptr_handle_t handle[BATCH_SIZE];
    spsc_q *sq = q->sq;
    spsc_q *aq = q->aq;
    BufferObj *ptr;

    // Note: write_available() can return different values each times its called
    // So we dont want to use the MIN macro here.
    uint32_t count = sq->write_available();
    if (count > q->freeq.size()) {
        count = q->freeq.size();
    }
    register int j = 0;
    for (uint32_t i = 0; i < count; i++) {
        ptr = q->freeq.back();
        q->freeq.pop_back();
        bool send_more = enq_cb(ptr->data, &ptr->len, opaq);
        handle[j++] = q->shm->get_handle_from_address(ptr);
        if (j == BATCH_SIZE) {
            sq->push(handle, BATCH_SIZE);
            q->writes += BATCH_SIZE;
            j = 0;
        }
        if (!send_more) {
            break;
        }
    }
    if (j) {
        // push remnant entries
        sq->push(handle, j);
        q->writes += j;
    }
    // Process the freeq in batches
    while ((j = aq->pop(handle, BATCH_SIZE))) {
        for (auto i = 0; i < j; i++) {
            if (!q->old_entries) {
                ptr = (BufferObj *)q->shm->get_address_from_handle(handle[i]);
                q->freeq.push_back(ptr);
            } else {
                q->old_entries--;
            }
        }
    }
    return q->freeq.size() != 0;
}

bool
shm_ipc_try_send (shm_ipcq *q, encode_cb enq_cb, void *opaq)
{
    bool ret = false;
    ptr_handle_t handle;
    spsc_q *sq = q->sq;
    spsc_q *aq = q->aq;
    BufferObj *ptr;

    if (sq->write_available() && q->freeq.size()) {
        ptr = q->freeq.back();
        q->freeq.pop_back();
        enq_cb(ptr->data, &ptr->len, opaq);
        handle = q->shm->get_handle_from_address(ptr);
        // NOTE: ret can only be true below.
        ret = sq->push(handle);
    }
    while (aq->pop(handle)) {
        if (!q->old_entries) {
            ptr = (BufferObj *)q->shm->get_address_from_handle(handle);
            q->freeq.push_back(ptr);
        } else {
            q->old_entries--;
        }
    }
    return ret;
}

bool
shm_ipc_send_eor (shm_ipcq *q)
{
    bool ret = false;
    ptr_handle_t handle;
    spsc_q *sq = q->sq;
    spsc_q *aq = q->aq;
    BufferObj *ptr;

    if (sq->write_available() && q->freeq.size()) {
        ptr = q->freeq.back();
        q->freeq.pop_back();
        ptr->data[0] = '\0';
        ptr->len = 0;
        handle = q->shm->get_handle_from_address(ptr);
        // NOTE: ret can only be true below.
        ret = sq->push(handle);
    }
    while (aq->pop(handle)) {
        if (!q->old_entries) {
            ptr = (BufferObj *)q->shm->get_address_from_handle(handle);
            q->freeq.push_back(ptr);
        } else {
            q->old_entries--;
        }
    }
    return ret;
}

int
shm_ipc_recv_start (shm_ipcq *q, decode_cb deq_cb, void* opaq)
{
    int ret;
    ptr_handle_t to_free[BATCH_SIZE];
    spsc_q *sq = q->sq;
    spsc_q *aq = q->aq;
    ptr_handle_t handle;

    register int j = 0;
    while (sq->pop(handle)) {
        q->reads++;
        to_free[j++] = handle;
        BufferObj *ptr = (BufferObj *)q->shm->get_address_from_handle(handle);
        if (ptr->len) {
            ret = deq_cb(ptr->data, ptr->len, opaq);
        } else {
            ret = -1;
        }
        if (j == BATCH_SIZE) {
            aq->push(to_free, BATCH_SIZE);
            j = 0;
        }
        if (ret <= 0) {
            break;
        }
    }
    if (j) {
        aq->push(to_free, j);
    }
    if (ret != -1) {
        ret = sq->read_available();
    }
    return ret;
}

