//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_INFRA_IPC_VPP_HDLR_H__
#define __VPP_INFRA_IPC_VPP_HDLR_H__

#define PDS_MAX_KEY_LEN 16

#ifdef __cplusplus
extern "C" {
#endif

#define PDS_VPP_THREAD_SLEEP_INTERVAL (100 * 1000) // Sleep for 100ms
#define PDS_VPP_SYNC_CORE (2)
#define PDS_VPP_RESTORE_CORE (3)

// function prototypes
int pds_vpp_ipc_init(void);
void pds_vpp_worker_thread_barrier(void);
void pds_vpp_worker_thread_barrier_release(void);
void pds_vpp_set_suspend_resume_worker_threads(int suspend);

// ipc log, to be used by CB functions
int ipc_log_notice(const char *fmt, ...);
int ipc_log_error(const char *fmt, ...);

// ipc infra internal functions
void pds_vpp_fd_register(int fd);
void pds_ipc_init(void);
void pds_ipc_read_fd(int fd);

#ifndef __cplusplus

// inline functions
always_inline u8
pds_id_equals(const u8 id1[PDS_MAX_KEY_LEN], const u8 id2[PDS_MAX_KEY_LEN])
{
    return !memcmp(id1, id2, PDS_MAX_KEY_LEN);
}

always_inline void
pds_id_set(u8 id1[PDS_MAX_KEY_LEN], const u8 id2[PDS_MAX_KEY_LEN])
{
    clib_memcpy(id1, id2, PDS_MAX_KEY_LEN);
}

#endif


#ifdef __cplusplus
}
#endif

#endif    // __VPP_INFRA_IPC_VPP_HDLR_H__
