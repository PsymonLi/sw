// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
// This file is shared between both C++ and C code.

#ifndef __SHM_IPC_H__
#define __SHM_IPC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

// Maximum size of the buffer in which data needs to be encoded
#define VPP_SHMIPC_BUFFER_SIZE 191

// C interfaces begin
// Forward Decl.
struct shm_ipcq;
typedef bool (*encode_cb)(uint8_t *dest, uint8_t *len, void *opaq);
typedef bool (*decode_cb)(const uint8_t *src, const uint8_t len, void *opaq);

// Common API that needs to be invoked from both the producer and consumer
void shm_ipc_init(void);

// Producer side API's
struct shm_ipcq * shm_ipc_create(const char *qname);
// As long as there is availability in the transport layer, shm_ipc_send_start
// will repeatedly call encode_cb with 'opaq', 'data' and 'len'.
// encode_cb has to fill 'data' and set 'len' to the actual length of data to 
// be transmitted. 'opaq' is a passthrough for this layer and can be used for
// bookkeeping.
// encode_cb can return 'true' if it has more data to send and false otherwise.
// shm_ipc_send_start will return 
//     false the transport cannot send any more data
//     true if it can take send more data
bool shm_ipc_send_start(struct shm_ipcq *q, encode_cb enq_cb, void *opaq);
// Try send a end-of-record marker over the channel.
//     true if eor was sent
//     false transport cannot send data. Caller has to re-try in this case.
bool shm_ipc_send_eor(struct shm_ipcq *q);
void shm_ipc_destroy(const char *qname);

// Consumer side API's
struct shm_ipcq * shm_ipc_attach(const char *name);
// As long as there is availability in the transport layer, shm_ipc_recv_start 
// will repeatedly get the data from the transport layer and call decode_cb with
// 'data' and 'len' for each item that was received. 'opaq' is a passthrough for
// this layer and can be used for bookkeeping.
// decode_cb can return true if it can process more data and false otherwise.
// shm_ipc_recv_start will return 
//     0 the transport doesn't have any more data
//     >0 if the transport has more data 
//     -1 if eor was received
// NOTE:
// decode_cb will be called with 'len' field set to 0, if an 'end-of-record'
// marker was received over the channel. See shmi_ipc_send_eor()
int shm_ipc_recv_start(struct shm_ipcq *q, decode_cb deq_cb, void* opaq);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __SHM_IPC_H__ */
