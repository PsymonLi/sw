//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//
#ifndef __TCP_PROG_HW_HPP__
#define __TCP_PROG_HW_HPP__

#ifdef __cplusplus

#include <stddef.h>
#include "nic/sdk/platform/utils/program.hpp"
#include "nic/sdk/platform/utils/lif_mgr/lif_mgr.hpp"

using sdk::platform::utils::lif_mgr;

namespace vpp {
namespace tcp {

mpartition *pds_platform_get_mpartition(void);
void pds_platform_init(program_info *prog_info, asic_cfg_t *asic_cfg,
        catalog *platform_catalog, lif_mgr **tcp_lif, lif_mgr **gc_lif,
        lif_mgr **sock_lif);
const char * pds_get_platform_name(void);
uint32_t pds_get_platform_max_tcp_qid(void);
uint8_t pds_platform_get_pc_offset(program_info *prog_info,
        const char *prog,const char *label);
sdk_ret_t pds_platform_prog_ip4_flow(uint32_t qid, bool is_ipv4,
                                     uint32_t lcl_ip, uint32_t rmt_ip,
                                     uint16_t lcl_port, uint16_t rmt_port,
                                     bool proxy);
sdk_ret_t pds_platform_del_ip4_flow(uint32_t qid, bool is_ipv4,
                                    uint32_t lcl_ip, uint32_t rmt_ip,
                                    uint16_t lcl_port, uint16_t rmt_port);

} // namespace tcp
} // namespace vpp

extern "C" {
#endif // __cplusplus

#include "nic/utils/tcpcb_pd/tcpcb_pd.h"
#include "nic/include/pds_socket.h"

//#define VPP_TRACE_ENABLE 1

typedef struct ip4_proxy_params_s {
    uint32_t my_ip;
    uint32_t client_ip;
    uint32_t server_ip;
} ip4_proxy_params_t;

typedef struct pds_tcp_connection_info_ {
    uint32_t c_index;
    uint32_t thread_index;
    uint8_t  state;
    uint8_t  close_reason;
    uint8_t  app_type;
} pds_tcp_connection_info_t;

int pds_init(void);
int pds_get_max_num_tcp_flows();
void pds_tcp_fill_def_params(tcpcb_pd_t *tcpcb);
void pds_tcp_fill_app_params(tcpcb_pd_t *tcpcb,
        pds_sockopt_hw_offload_t *offload, uint32_t data_len);
void pds_tcp_fill_app_return_params(tcpcb_pd_t *tcpcb,
        pds_sockopt_hw_offload_t *offload, uint32_t data_len);
void pds_tcp_fill_proxy_return_params(tcpcb_pd_t *tcpcb,
        pds_sockopt_hw_offload_t *offload, uint32_t data_len);
int pds_tcp_prog_tcpcb(tcpcb_pd_t *tcpcb);
int pds_tcp_del_tcpcb(uint32_t qid);
int pds_tcp_prog_ip4_flow(uint32_t qid, bool is_ipv4, uint32_t lcl_ip,
                          uint32_t rmt_ip, uint16_t lcl_port, uint16_t rmt_port,
                          bool proxy);
int pds_tcp_del_ip4_flow(uint32_t qid, bool is_ipv4, uint32_t lcl_ip,
                         uint32_t rmt_ip, uint16_t lcl_port, uint16_t rmt_port);

void ip4_proxy_set_params(uint32_t my_ip,
                          uint32_t client_addr4,
                          uint32_t server_addr4);
ip4_proxy_params_t * ip4_proxy_get_params(void);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __TCP_PROG_HW_HPP__
