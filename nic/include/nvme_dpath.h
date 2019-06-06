//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#ifndef _NVME_DPATH_H
#define _NVME_DPATH_H

#include "nic/sdk/platform/capri/capri_common.hpp"

#define HOSTMEM_PAGE_SIZE  (1 << 12)  //4096 Bytes
#define MAX_LIFS           2048

#define PACKED __attribute__((__packed__))

#define HBM_PAGE_SIZE 4096
#define HBM_PAGE_SIZE_SHIFT 12
#define HBM_PAGE_ALIGN(x) (((x) + (HBM_PAGE_SIZE - 1)) & \
                           ~(uint64_t)(HBM_PAGE_SIZE - 1))

typedef struct lif_init_attr_s {
    uint32_t max_sq;
    uint32_t max_cq;
    uint32_t max_ns;
    uint32_t max_sess;
} lif_init_attr_t;

/*====================  TYPES.H ===================*/

#define BYTES_TO_BITS(__B) ((__B)*8)
#define BITS_TO_BYTES(__b) ((__b)/8)

#endif // _NVME_DPATH_H
