//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_NAT_PDSA_UDS_HDLR_H__
#define __VPP_NAT_PDSA_UDS_HDLR_H__

#include <nic/vpp/infra/ipc/pdsa_vpp_hdlr.h>
#include "nat_api.h"

#ifdef __cplusplus
extern "C" {
#endif
//
// data structures
//

// pdsa_uds_hdlr.cc
void pds_nat_uds_init(void);


#ifdef __cplusplus
}
#endif

#endif    // __VPP_NAT_PDSA_UDS_HDLR_H__
