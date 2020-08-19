//---------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
// PDS MS User exit callbacks for BGP ORF propagation at RR
//---------------------------------------------------------------

#ifndef __PDS_MS_BGP_ORF_UE__
#define __PDS_MS_BGP_ORF_UE__

#include <nbase.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <a0spec.h>
#include <o0mac.h>
#include <a0cust.h>
#include <a0mib.h>
#include <qbrmincl.h>

// user exits from MS BGP
void pen_qbrm_user_orf_ext_comm_type(const ATG_INET_ADDRESS *local_addr,
                                     const ATG_INET_ADDRESS *remote_addr,
                                     const QB_EXTENDED_COMMUNITY* ext_comm_val,
                                     NBB_BYTE add_or_remove);
void pen_qbrm_user_orf_apply(const ATG_INET_ADDRESS *local_addr,
                             const ATG_INET_ADDRESS *remote_addr);
void pen_qbrm_user_nbr_session_down(const ATG_INET_ADDRESS *local_addr,
                                    const ATG_INET_ADDRESS *remote_addr);
void pen_qbrm_user_orf_remove_all(const ATG_INET_ADDRESS *local_addr,
                                  const ATG_INET_ADDRESS *remote_addr);

#ifdef __cplusplus
}
#endif

#endif
