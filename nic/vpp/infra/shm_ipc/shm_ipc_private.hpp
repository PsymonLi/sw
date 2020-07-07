// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
// This file is shared between both C++ and C code.

#ifndef __SHM_IPC_PRIVATE_HPP__
#define __SHM_IPC_PRIVATE_HPP__

#ifdef __cplusplus

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/lockfree/spsc_queue.hpp>

// num_qs, change this to the number of threads if we have to create
// shm queue per thread, in the sender and receiver process.
const uint32_t num_qs = 1;

// Number of buffers that can be enqueued to the message queue
const uint32_t NUM_BUFFERS = 8192;

// message queue attr. We keep the queue size the same as
// number of buffers. This is to ensure that if we are able to
// allocate a buffer, we must be able to enqueue it
const uint32_t ipc_qsize = NUM_BUFFERS;

// shm region to allocate 'NUM_BUFFERS' buffers per num_qs.
const size_t BUFFER_MEM_SIZE = ((VPP_SHMIPC_BUFFER_SIZE + 1) *
                                 NUM_BUFFERS) + 1024;

// shm region to allocate a spsc ring in shared memory.
// spsc ring contains 'ipc_qsize' elements, each 8 bytes in size
const size_t RING_MEM_SIZE = (ipc_qsize * 8) + 1024;

// Total size of shared memory
const size_t ALL_BUFFER_MEM_SIZE = BUFFER_MEM_SIZE * num_qs;
const size_t ALL_RINGS_MEM_SIZE = RING_MEM_SIZE * 2 * num_qs;
const size_t SHM_IPC_MEMSTORE_SIZE = ALL_BUFFER_MEM_SIZE + ALL_RINGS_MEM_SIZE;

// Session object serialize to / de-serialize from Array
// We use a SHM allocator, allocate this array, serialize the
// session proto buf object to this array and queue the pointer
// for the reader to take it, de-serialize and update his tables
struct BufferObj {
    uint8_t len;
    uint8_t data[VPP_SHMIPC_BUFFER_SIZE];
};

typedef boost::interprocess::managed_mapped_file::handle_t ptr_handle_t;
typedef boost::lockfree::spsc_queue<ptr_handle_t,
                                    boost::lockfree::capacity<ipc_qsize>
                                    > spsc_q;

#endif  /* __cplusplus */
#endif  /* __SHM_IPC_PRIVATE_HPP__ */

