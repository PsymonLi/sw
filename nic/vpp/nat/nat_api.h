//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_NAT_API_H__
#define __VPP_NAT_API_H__

#ifndef __cplusplus
#include <vlib/vlib.h>
#include <vnet/ip/ip4.h>
#endif
#include "nic/vpp/infra/ipc/pdsa_vpp_hdlr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Datastructures
typedef uint32_t nat_hw_index_t;

// NAT Port Block returned in get
typedef struct pds_nat_port_block_export_s {
    uint8_t id[PDS_MAX_KEY_LEN + 1];
    uint32_t addr;
    uint16_t start_port;
    uint16_t end_port;
    uint8_t protocol;
    uint8_t address_type;

    // stats
    uint32_t in_use_cnt;
    uint32_t session_cnt;
} pds_nat_port_block_export_t;

typedef enum {
    NAT_ERR_OK = 0,
    NAT_ERR_EXISTS = 1,
    NAT_ERR_NOT_FOUND = 2,
    NAT_ERR_IN_USE = 3,
    NAT_ERR_HW_TABLE_FULL = 4,
    NAT_ERR_ALLOC_FAIL = 5,
    NAT_ERR_NO_RESOURCE = 6,
    NAT_ERR_INVALID_PROTOCOL = 7,
    NAT_ERR_FAIL = 8,
} nat_err_t;

typedef enum {
    NAT_ADDR_TYPE_INTERNET = 0,
    NAT_ADDR_TYPE_INFRA = 1,

    NAT_ADDR_TYPE_NUM = 2
} nat_addr_type_t;

typedef enum {
    NAT_PROTO_UNKNOWN = 0,
    NAT_PROTO_UDP = 1,
    NAT_PROTO_TCP = 2,
    NAT_PROTO_ICMP = 3,

    NAT_PROTO_NUM = 3
} nat_proto_t;

typedef struct nat_flow_key_s {
    uint32_t dip;
    uint32_t sip;
    uint16_t sport;
    uint16_t dport;
    uint8_t protocol;
    uint8_t pad[3];
} nat_flow_key_t;

// sync datastructure used in upgrade
typedef struct nat44_sync_info_ {
    uint32_t vpc_id;
    uint32_t public_sip;
    uint32_t dip;
    uint32_t pvt_sip;
    uint32_t rx_xlate_id;
    uint32_t tx_xlate_id;
    uint16_t public_sport;
    uint16_t dport;
    uint16_t pvt_sport;
    uint8_t proto;
    nat_addr_type_t nat_addr_type;
} nat44_sync_info_t;

typedef struct nat_global_stats_ {
    uint32_t hw_index_total;
    uint32_t hw_index_alloc;
} nat_global_stats_t;


typedef void *(*nat_flow_iter_cb_t)(void *ctxt, uint32_t vpc_id,
                                    nat_flow_key_t *key, uint64_t val);


// API
nat_err_t nat_port_block_get_stats(const uint8_t id[PDS_MAX_KEY_LEN],
                                   uint32_t vpc_hw_id, uint8_t protocol,
                                   nat_addr_type_t nat_addr_type,
                                   pds_nat_port_block_export_t *export_pb);

void *nat_flow_iterate(void *ctxt, nat_flow_iter_cb_t iter_cb);

void nat_flow_ht_get_fields(uint64_t data, nat_hw_index_t *hw_index,
                            nat_addr_type_t *nat_addr_type,
                            nat_proto_t *nat_proto);

nat_proto_t get_proto_from_nat_proto(nat_proto_t nat_proto);

nat_hw_index_t nat_get_tx_hw_index_pub_ip_port(uint32_t vpc_id,
                                               nat_addr_type_t nat_addr_type,
                                               nat_proto_t nat_proto,
                                               uint32_t addr, uint16_t port);

void nat_sync_start(void);
void nat_sync_stop(void);
nat_err_t nat_sync_restore(nat44_sync_info_t *sync);
void nat_get_global_stats(nat_global_stats_t *stats);

#ifdef __cplusplus
nat_err_t nat_port_block_add(const uint8_t key[PDS_MAX_KEY_LEN],
                             uint32_t vpc_hw_id,
                             uint32_t addr, uint8_t protocol,
                             uint16_t start_port, uint16_t end_port,
                             nat_addr_type_t nat_addr_type, uint8_t threshold);
nat_err_t nat_port_block_update(const uint8_t key[PDS_MAX_KEY_LEN],
                                uint32_t vpc_hw_id,
                                uint32_t addr, uint8_t protocol,
                                uint16_t start_port, uint16_t end_port,
                                nat_addr_type_t nat_addr_type);
nat_err_t nat_port_block_commit(const uint8_t key[PDS_MAX_KEY_LEN],
                                uint32_t vpc_hw_id,
                                uint32_t addr, uint8_t protocol,
                                uint16_t start_port, uint16_t end_port,
                                nat_addr_type_t nat_addr_type,
                                uint8_t threshold);
nat_err_t nat_port_block_del(const uint8_t key[PDS_MAX_KEY_LEN],
                             uint32_t vpc_hw_id,
                             uint32_t addr, uint8_t protocol,
                             uint16_t start_port, uint16_t end_port,
                             nat_addr_type_t nat_addr_type);
#else

typedef int (*nat_vendor_invalidate_cb)(vlib_buffer_t *b, u16 *next);

void nat_init(void);
nat_err_t nat_port_block_add(const u8 id[PDS_MAX_KEY_LEN], u32 vpc_hw_id,
                             ip4_address_t addr, u8 protocol, u16 start_port,
                             u16 end_port, nat_addr_type_t nat_addr_type,
                             u8 threshold);
nat_err_t nat_port_block_update(const u8 id[PDS_MAX_KEY_LEN], u32 vpc_hw_id,
                                ip4_address_t addr, u8 protocol, u16 start_port,
                                u16 end_port, nat_addr_type_t nat_addr_type);
nat_err_t nat_port_block_commit(const u8 id[PDS_MAX_KEY_LEN], u32 vpc_hw_id,
                                ip4_address_t addr, u8 protocol, u16 start_port,
                                u16 end_port, nat_addr_type_t nat_addr_type,
                                u8 threshold);
nat_err_t nat_port_block_del(const u8 id[PDS_MAX_KEY_LEN], u32 vpc_hw_id,
                             ip4_address_t addr, u8 protocol, u16 start_port,
                             u16 end_port, nat_addr_type_t nat_addr_type);

nat_err_t nat_flow_alloc(u32 vpc_hw_id, ip4_address_t dip, u16 dport,
                         u8 protocol, ip4_address_t pvt_ip, u16 pvt_port,
                         nat_addr_type_t nat_addr_type, ip4_address_t *sip, u16 *sport,
                         nat_hw_index_t *xlate_idx,
                         nat_hw_index_t *xlate_idx_rflow);
nat_err_t nat_flow_dealloc(u32 vpc_hw_id, ip4_address_t dip, u16 dport, u8 protocol,
                           ip4_address_t sip, u16 sport);
nat_err_t nat_flow_xlate(u32 vpc_id, ip4_address_t dip, u16 dport,
                         u8 protocol, ip4_address_t sip, u16 sport,
                         ip4_address_t *pvt_ip, u16 *pvt_port);
void nat_flow_is_dst_valid(u32 vpc_id, ip4_address_t dip, u16 dport,
                           u8 protocol, nat_addr_type_t nat_addr_type,
                           bool *dstip_valid, bool *dstport_valid);
nat_err_t nat_usage(u32 vpc_hw_id, u8 protocol, nat_addr_type_t nat_addr_type,
                    u32 *num_ports_total, u32 *num_ports_alloc,
                    u32 *num_flows_alloc);
nat_err_t nat_hw_usage(u32 *total_hw_indices, u32 *total_alloc_indices);

void nat_register_vendor_invalidate_cb(nat_vendor_invalidate_cb cb);

#endif // __cplusplus

#ifdef __cplusplus
}
#endif

#endif    // __VPP_NAT_API_H__
