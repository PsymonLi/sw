//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_INFRA_IPC_PDS_IPC_H__
#define __VPP_INFRA_IPC_PDS_IPC_H__

#ifdef __cplusplus
extern "C" {
#endif

// function prototypes
int pds_vpp_ipc_init(void);

// ipc log, to be used by CB functions
int ipc_log_notice(const char *fmt, ...);
int ipc_log_error(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif    // __VPP_INFRA_IPC_PDS_IPC_H__
