// {C} Copyright 2017 Pensando Systems Inc. All rights reserved

#ifndef __PDS_SOCKET_H__
#define __PDS_SOCKET_H__

// TCP App type
#define TCP_APP_TYPE_BYPASS                 1
#define TCP_APP_TYPE_TLS                    2
#define TCP_APP_TYPE_NVME                   3
#define TCP_APP_TYPE_ARM_SOCKET             4

typedef struct pds_sockopt_hw_offload_ {
    // input
    uint8_t app_type;
    uint8_t app_qtype;
    uint8_t app_ring;
    uint8_t serq_desc_size;;
    uint64_t serq_base;
    uint64_t sesq_prod_ci_addr;
    uint32_t app_qid; // for proxy, set to conn2 qid for conn2
    uint32_t serq_ring_size;
    uint16_t app_lif;
    uint16_t serq_ci;

    // output
    uint64_t sesq_base;
    uint64_t serq_prod_ci_addr;
    uint32_t hw_qid;
    uint32_t sesq_ring_size;
    uint16_t tcp_lif;
    uint8_t tcp_qtype;
    uint8_t tcp_ring;
    uint8_t sesq_desc_size;;
    uint16_t sesq_pi;
} pds_sockopt_hw_offload_t;

#endif      /* __PDS_SOCKET_H__ */
