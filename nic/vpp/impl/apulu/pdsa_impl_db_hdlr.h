//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_APULU_PDSA_IMPL_DB_HDLR_H__
#define __VPP_IMPL_APULU_PDSA_IMPL_DB_HDLR_H__

#include <stdint.h>
#include "nic/vpp/infra/ipc/pdsa_vpp_hdlr.h"

#define PDS_VPP_VPC_FLAGS_CONTROL_VPC           0x0001

#ifdef __cplusplus
extern "C" {
#endif

bool pds_session_active_on_vnic_get(uint16_t vnic_id, uint32_t *sess_count);

int pds_impl_db_vnic_set(uint8_t *key,
                         uint8_t *mac,
                         uint32_t max_sessions,
                         uint16_t vnic_hw_id,
                         uint16_t subnet_hw_id,
                         bool flow_log_en,
                         bool con_track_en,
                         bool dot1q,
                         bool dot1ad,
                         uint16_t vlan_id,
                         uint16_t nh_hw_id,
                         uint16_t host_lif_hw_id);

int pds_impl_db_vnic_del(uint16_t vnic_hw_id);

int pds_impl_db_subnet_set(uint8_t pfx_len,
                           uint32_t vr_ip,
                           uint8_t *mac,
                           uint16_t subnet_hw_id,
                           uint32_t vnid,
                           uint16_t vpc_id);

int pds_impl_db_subnet_del(uint16_t subnet_hw_id);

int pds_impl_db_vpc_set(uint16_t vpc_hw_id, uint16_t bd_hw_id, uint16_t flags);

uint8_t pds_impl_db_vpc_is_control_vpc (uint16_t vpc_hw_id);

int pds_impl_db_vpc_del(uint16_t vpc_hw_id);

int pds_impl_db_device_set(const uint8_t *mac, const uint8_t *ip,
                           uint8_t ip4, uint8_t overlay_routing_en,
                           uint8_t symmetric_routing_en,
                           uint16_t mapping_prio,
                           bool host, bool bitw_switch,
                           bool bitw_svc, bool bitw_classic);

int pds_impl_db_device_del(void);

int pds_cfg_db_init(void);

int pds_impl_db_init(void);

#ifdef __cplusplus
}
#endif

#endif    // __VPP_IMPL_APULU_PDSA_IMPL_DB_HDLR_H__

