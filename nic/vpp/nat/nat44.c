//
//  {C} Copyright 2019 Pensando Systems Inc. All rights reserved.
//

#include <vlib/vlib.h>
#include <vnet/ip/ip.h>
#include <vnet/plugin/plugin.h>
#include <vppinfra/clib.h>
#include <nic/vpp/impl/nat.h>
#include <nic/vpp/impl/ftl_wrapper.h>
#include <mapping.h>
#include "nat_api.h"

#define IPV4_MASK 0xffffffff
#define PORT_MASK 0xffff
#define REF_COUNT_MASK 0xffff

typedef enum {
    NAT_PB_STATE_UNKNOWN,
    NAT_PB_STATE_ADDED,
    NAT_PB_STATE_DELETED,
    NAT_PB_STATE_UPDATED,
    NAT_PB_STATE_OK,
} nat_port_block_state_t;

typedef struct nat_port_block_s {
    // configured
    u8 id[PDS_MAX_KEY_LEN];
    ip4_address_t addr;
    u16 start_port;
    u16 end_port;

    // allocated
    nat_port_block_state_t state;
    u32 num_flow_alloc;
    u32 ports_in_use;
    u32 *ref_count;
    nat_hw_index_t *nat_tx_hw_index;
    uword *port_bitmap;
    u8 threshold;
    u8 alert_raised;
} nat_port_block_t;

typedef struct nat_vpc_config_s {
    clib_spinlock_t lock;

    u32 random_seed;

    // configured

    // number of port blocks
    u16 num_port_blocks;
    // pool of port blocks
    nat_port_block_t *nat_pb[NAT_ADDR_TYPE_NUM][NAT_PROTO_NUM];

    // allocated

    // hash table of public ip --> pb
    uword *nat_public_ip_ht;

    // hash table of allocated ports for flows
    uword *nat_flow_ht;

    // hash table of <pvt sip, pvt port> --> <public sip, public port>
    uword *nat_src_ht;

    // hash table of pvt_ip --> public_ip
    uword *nat_pvt_ip_ht;

    // stats
    u64 alloc_fail;
    u64 dealloc_fail;
} nat_vpc_config_t;

// Flow hash table maps <nat_flow_key_t> to <nat_rx_hw_index, addr_type, proto>
#define NAT_FLOW_HT_MAKE_VAL(nat_rx_hw_index, addr_type, nat_proto) \
    ((((uword)nat_rx_hw_index) << 32) | (((uword) addr_type << 16)) | ((uword) nat_proto))

#define NAT_FLOW_HT_GET_VAL(data, nat_rx_hw_index, addr_type, nat_proto) \
    nat_proto = (data) & 0xff; \
    addr_type = ((data) >> 16) & 0xff; \
    nat_rx_hw_index = ((data) >> 32) & 0xffffffff;

#define NAT_FLOW_HT_GET_HW_INDEX(data, nat_rx_hw_index) \
    nat_rx_hw_index = ((data) >> 32) & 0xffffffff;

// Endpoint hash table maps <pvt_ip, pvt_port> to <public_ip, public_port>
#define NAT_EP_SET_PUBLIC_IP_PORT(data, public_ip, public_port, ref_count) \
    data = (((uword) ref_count) << 48) | (((uword) public_port) << 32) | \
           (uword) public_ip.as_u32;

#define NAT_EP_SET_REF_COUNT(data, ref_count) \
    data = (((uword) ref_count) << 48) | ((data) & IPV4_MASK);

#define NAT_EP_INC_REF_COUNT(data, ref_count) \
    ref_count = ((data) >> 48) & REF_COUNT_MASK; \
    ref_count++; \
    data = (((uword) ref_count) << 48) | ((data) & IPV4_MASK);

#define NAT_EP_GET_PUBLIC_IP_PORT(data, public_ip, public_port, ref_count) \
    public_ip = (data) & IPV4_MASK; \
    public_port = ((data) >> 32) & PORT_MASK; \
    ref_count = ((data) >> 48) & REF_COUNT_MASK;

#define NAT_EP_GET_REF_COUNT(data, ref_count) \
    ref_count = ((data) >> 48) & REF_COUNT_MASK;

typedef CLIB_PACKED (union nat_src_key_s {
    struct {
        ip4_address_t pvt_ip;
        uint16_t pvt_port;
        uint8_t nat_proto;
        uint8_t service;
    };
    u64 as_u64;
}) nat_src_key_t;

// pvt ip --> public ip hash table
#define NAT_PVT_IP_HT_DATA(public_ip, ref_count) \
    ((((uword) ref_count) << 32) | public_ip)

#define NAT_PVT_IP_HT_PUBLIC_IP(data) \
    ((data) & IPV4_MASK)

#define NAT_PVT_IP_HT_INC_REF_COUNT(data) \
    u16 _ref_count = (((data) >> 32) & REF_COUNT_MASK); \
    _ref_count++; \
    (data) = ((uword)_ref_count << 32) | ((data) & IPV4_MASK);

#define NAT_PVT_IP_HT_DEC_REF_COUNT(data, ref_count) \
    (ref_count) = (((data) >> 32) & REF_COUNT_MASK); \
    (ref_count)--; \
    (data) = (((uword)ref_count) << 32) | ((data) & IPV4_MASK);

typedef CLIB_PACKED (union nat_pvt_ip_key_s {
    struct {
        ip4_address_t pvt_ip;
        uint8_t service;
    };
    u64 as_u64;
}) nat_pvt_ip_key_t;

// public ip --> port block
typedef CLIB_PACKED (union nat_pub_key_s {
    struct {
        u8 nat_addr_type;
        u8 nat_proto;
        ip4_address_t pub_ip;
    };
    u64 as_u64;
}) nat_pub_key_t;


//
// Globals
//
typedef struct pds_nat_main_s {
    clib_spinlock_t lock;
    uword *hw_index_bitmap;
    u32 num_hw_indices_allocated;
    nat_vpc_config_t **vpc_config;
    u8 proto_map[255];
    u8 nat_proto_map[NAT_PROTO_NUM + 1];
    u64 sync_stats[NAT_SYNC_STATS_MAX];
} pds_nat_main_t;

static pds_nat_main_t nat_main;

//
// init
//
void
nat_init(void)
{
    clib_spinlock_init(&nat_main.lock);
    clib_bitmap_alloc(nat_main.hw_index_bitmap, PDS_MAX_DYNAMIC_NAT);

    nat_main.proto_map[IP_PROTOCOL_UDP] = NAT_PROTO_UDP;
    nat_main.proto_map[IP_PROTOCOL_TCP] = NAT_PROTO_TCP;
    nat_main.proto_map[IP_PROTOCOL_ICMP] = NAT_PROTO_ICMP;

    nat_main.nat_proto_map[NAT_PROTO_TCP] = IP_PROTOCOL_TCP;
    nat_main.nat_proto_map[NAT_PROTO_UDP] = IP_PROTOCOL_UDP;
    nat_main.nat_proto_map[NAT_PROTO_ICMP] = IP_PROTOCOL_ICMP;

    ftl_reg_nat_dealloc_cb((nat_flow_dealloc_cb)nat_flow_dealloc);
}

//
// Helper functions
//
always_inline nat_port_block_t *
nat_get_port_block_from_pub_ip(nat_vpc_config_t *vpc, nat_addr_type_t nat_addr_type,
                               nat_proto_t nat_proto, ip4_address_t addr)
{
    uword *data;

    nat_pub_key_t pub_key = { 0 };

    pub_key.nat_addr_type = (u8)nat_addr_type;
    pub_key.nat_proto = nat_proto;
    pub_key.pub_ip = addr;
    data = hash_get(vpc->nat_public_ip_ht, pub_key.as_u64);
    if (data) {
        return pool_elt_at_index(vpc->nat_pb[nat_addr_type][nat_proto - 1], *data);
    }

    return NULL;
}

always_inline nat_proto_t
get_nat_proto_from_proto(u8 protocol)
{
    return nat_main.proto_map[protocol];
}

nat_proto_t
get_proto_from_nat_proto(nat_proto_t nat_proto)
{
    return nat_main.nat_proto_map[nat_proto];
}

void
nat_flow_ht_get_fields(uint64_t data, nat_hw_index_t *hw_index,
                       nat_addr_type_t *addr_type, nat_proto_t *nat_proto)
{
    NAT_FLOW_HT_GET_VAL(data, *hw_index, *addr_type, *nat_proto)
}

nat_hw_index_t
nat_get_tx_hw_index_pub_ip_port(uint32_t vpc_id, nat_addr_type_t nat_addr_type,
                                nat_proto_t nat_proto, uint32_t addr,
                                uint16_t port)
{
    nat_port_block_t *pb;
    nat_vpc_config_t *vpc;
    u16 index;

    vpc = vec_elt(nat_main.vpc_config, vpc_id);
    if (vpc == NULL) {
        return -1;
    }

    pb = nat_get_port_block_from_pub_ip(vpc, nat_addr_type, nat_proto,
                                        (ip4_address_t)addr);
    if (!pb) {
        return -1;
    }
    index = port - pb->start_port;
    return vec_elt(pb->nat_tx_hw_index, index);
}

//
// Add SNAT port block
//
nat_err_t
nat_port_block_add(const u8 id[PDS_MAX_KEY_LEN], u32 vpc_id, ip4_address_t addr,
                   u8 protocol, u16 start_port, u16 end_port,
                   nat_addr_type_t nat_addr_type, u8 threshold)
{
    nat_port_block_t *pb;
    nat_vpc_config_t *vpc;
    nat_proto_t nat_proto;
    nat_pub_key_t pub_key = { 0 };

    ASSERT(nat_addr_type < NAT_ADDR_TYPE_NUM);

    vec_validate(nat_main.vpc_config, vpc_id);
    vpc = vec_elt(nat_main.vpc_config, vpc_id);
    if (vpc == NULL) {
        vpc = clib_mem_alloc_no_fail(sizeof(*vpc));
        clib_memset(vpc, 0, sizeof(*vpc));
        vec_elt(nat_main.vpc_config, vpc_id) = vpc;
    }

    nat_proto = get_nat_proto_from_proto(protocol);
    if (nat_proto == NAT_PROTO_UNKNOWN) {
        return NAT_ERR_INVALID_PROTOCOL;
    }

    if (nat_get_port_block_from_pub_ip(vpc, nat_addr_type, nat_proto, addr)) {
        // Entry already exists
        // This can happen in rollback case

        return NAT_ERR_EXISTS;
    }

    if (vpc->num_port_blocks == 0) {
        clib_spinlock_init(&vpc->lock);
        vpc->nat_flow_ht = hash_create_mem(0, sizeof(nat_flow_key_t),
                                           sizeof(uword));
        vpc->random_seed = random_default_seed();
    }

    clib_spinlock_lock(&vpc->lock);
    pool_get_zero(vpc->nat_pb[nat_addr_type][nat_proto - 1], pb);
    vpc->num_port_blocks++;
    pds_id_set(pb->id, id);
    pb->addr = addr;
    pb->start_port = start_port;
    pb->end_port = end_port;
    pb->threshold = threshold;
    pb->state = NAT_PB_STATE_ADDED;

    pub_key.nat_addr_type = (u8)nat_addr_type;
    pub_key.nat_proto = nat_proto;
    pub_key.pub_ip = addr;
    hash_set(vpc->nat_public_ip_ht, pub_key.as_u64,
             pb - vpc->nat_pb[nat_addr_type][nat_proto - 1]);
    clib_spinlock_unlock(&vpc->lock);

    return NAT_ERR_OK;
}

//
// Update SNAT port block
//
nat_err_t
nat_port_block_update(const u8 id[PDS_MAX_KEY_LEN], u32 vpc_id,
                      ip4_address_t addr, u8 protocol, u16 start_port,
                      u16 end_port, nat_addr_type_t nat_addr_type)
{
    nat_port_block_t *pb = NULL;
    nat_vpc_config_t *vpc;
    nat_proto_t nat_proto;

    ASSERT(nat_addr_type < NAT_ADDR_TYPE_NUM);

    vpc = vec_elt(nat_main.vpc_config, vpc_id);

    nat_proto = get_nat_proto_from_proto(protocol);
    if (nat_proto == NAT_PROTO_UNKNOWN) {
        return NAT_ERR_INVALID_PROTOCOL;
    }

    pb = nat_get_port_block_from_pub_ip(vpc, nat_addr_type, nat_proto, addr);
    if (!pb) {
        // Entry does not exist
        return NAT_ERR_NOT_FOUND;
    }

    if (pb->ports_in_use != 0) {
        // port block is in use, cannot update
        return NAT_ERR_IN_USE;
    }

    clib_spinlock_lock(&vpc->lock);
    pb->state = NAT_PB_STATE_UPDATED;
    clib_spinlock_unlock(&vpc->lock);

    return NAT_ERR_OK;
}

always_inline void
nat_port_block_del_inline(nat_vpc_config_t *vpc, nat_port_block_t *pb,
                          nat_proto_t nat_proto, nat_addr_type_t nat_addr_type)
{
    nat_pub_key_t pub_key = { 0 };

    pub_key.nat_addr_type = (u8)nat_addr_type;
    pub_key.nat_proto = (u8)nat_proto;
    pub_key.pub_ip = pb->addr;
    clib_spinlock_lock(&vpc->lock);
    hash_unset(vpc->nat_public_ip_ht, pub_key.as_u64);
    pool_put(vpc->nat_pb[nat_addr_type][nat_proto - 1], pb);
    vpc->num_port_blocks--;
    if (vpc->num_port_blocks == 0) {
        hash_free(vpc->nat_flow_ht);
    }
    clib_spinlock_unlock(&vpc->lock);
}

//
// Delete SNAT port block
//
nat_err_t
nat_port_block_del(const u8 id[PDS_MAX_KEY_LEN], u32 vpc_id,
                   ip4_address_t addr, u8 protocol, u16 start_port,
                   u16 end_port, nat_addr_type_t nat_addr_type)
{
    nat_port_block_t *pb;
    nat_vpc_config_t *vpc;
    nat_proto_t nat_proto;

    vpc = vec_elt(nat_main.vpc_config, vpc_id);

    nat_proto = get_nat_proto_from_proto(protocol);
    if (nat_proto == NAT_PROTO_UNKNOWN) {
        return NAT_ERR_INVALID_PROTOCOL;
    }

    pb = nat_get_port_block_from_pub_ip(vpc, nat_addr_type, nat_proto, addr);
    if (!pb) {
        // Entry does not exist.
        return NAT_ERR_NOT_FOUND;
    }

    if (pb->ports_in_use != 0) {
        // port block is in use, cannot update
        return NAT_ERR_IN_USE;
    }

    pb->state = NAT_PB_STATE_DELETED;

    return NAT_ERR_OK;
}

//
// Commit SNAT port block
//
nat_err_t
nat_port_block_commit(const u8 id[PDS_MAX_KEY_LEN], u32 vpc_id,
                      ip4_address_t addr, u8 protocol, u16 start_port,
                      u16 end_port, nat_addr_type_t nat_addr_type,
                      u8 threshold)
{
    nat_port_block_t *pb;
    nat_vpc_config_t *vpc;
    nat_proto_t nat_proto;

    ASSERT(nat_addr_type < NAT_ADDR_TYPE_NUM);

    vpc = vec_elt(nat_main.vpc_config, vpc_id);

    nat_proto = get_nat_proto_from_proto(protocol);
    if (nat_proto == NAT_PROTO_UNKNOWN) {
        return NAT_ERR_INVALID_PROTOCOL;
    }

    pb = nat_get_port_block_from_pub_ip(vpc, nat_addr_type, nat_proto, addr);
    if (!pb) {
        // This should not happen
        return NAT_ERR_NOT_FOUND;
    }
    if (pb->state == NAT_PB_STATE_ADDED) {
        pb->state = NAT_PB_STATE_OK;
        return NAT_ERR_OK;
    } else if (pb->state == NAT_PB_STATE_DELETED) {
        nat_port_block_del_inline(vpc, pb, nat_proto, nat_addr_type);
        return NAT_ERR_OK;
    } else if (pb->state == NAT_PB_STATE_UPDATED) {
        pb->addr = addr;
        pb->start_port = start_port;
        pb->end_port = end_port;
        pb->threshold = threshold;
        pb->state = NAT_PB_STATE_OK;
        return NAT_ERR_OK;
    }

    return NAT_ERR_OK;
}

//
// Read SNAT port block stats
//
nat_err_t
nat_port_block_get_stats(const u8 id[PDS_MAX_KEY_LEN], u32 vpc_id,
                         u8 protocol, nat_addr_type_t nat_addr_type,
                         pds_nat_port_block_export_t *export_pb)
{
    nat_port_block_t *pb;
    nat_vpc_config_t *vpc;
    nat_port_block_t *nat_pb;
    nat_proto_t nat_proto;
    u32 in_use_cnt = 0, session_cnt = 0;

    ASSERT(nat_addr_type < NAT_ADDR_TYPE_NUM);

    if (vpc_id >= vec_len(nat_main.vpc_config)) {
        export_pb->in_use_cnt = 0;
        export_pb->session_cnt = 0;
        return NAT_ERR_NOT_FOUND;
    }
    vpc = vec_elt(nat_main.vpc_config, vpc_id);
    if (vpc == NULL) {
        export_pb->in_use_cnt = 0;
        export_pb->session_cnt = 0;
        return NAT_ERR_NOT_FOUND;
    }

    nat_proto = get_nat_proto_from_proto(protocol);
    if (nat_proto == NAT_PROTO_UNKNOWN) {
        return NAT_ERR_INVALID_PROTOCOL;
    }

    nat_pb = vpc->nat_pb[nat_addr_type][nat_proto - 1];

    pool_foreach (pb, nat_pb,
    ({
        if (pds_id_equals(id, pb->id)) {
            in_use_cnt += pb->ports_in_use;
            session_cnt += pb->num_flow_alloc;
        }
    }));

    export_pb->in_use_cnt = in_use_cnt;
    export_pb->session_cnt = session_cnt;

    return NAT_ERR_OK;
}

always_inline uword
nat_alloc_hw_index_no_lock(void)
{
    uword hw_index;

    hw_index = clib_bitmap_first_clear(nat_main.hw_index_bitmap);
    clib_bitmap_set_no_check(nat_main.hw_index_bitmap, hw_index, 1);
    nat_main.num_hw_indices_allocated++;

    return hw_index + PDS_DYNAMIC_NAT_START_INDEX;
}

always_inline void
nat_free_hw_index_no_lock(uword hw_index)
{
    hw_index -= PDS_DYNAMIC_NAT_START_INDEX;
    clib_bitmap_set_no_check(nat_main.hw_index_bitmap, hw_index, 0);
    nat_main.num_hw_indices_allocated--;
}

always_inline void
nat_alloc_hw_index_at_index_no_lock(uword hw_index)
{
    hw_index -= PDS_DYNAMIC_NAT_START_INDEX;
    clib_bitmap_set_no_check(nat_main.hw_index_bitmap, hw_index, 1);
    nat_main.num_hw_indices_allocated++;
}

always_inline void
nat_flow_get_and_add_src_endpoint_mapping(nat_vpc_config_t *vpc,
                                          ip4_address_t pvt_ip, u16 pvt_port,
                                          ip4_address_t *public_ip,
                                          u16 *public_port, nat_proto_t nat_proto,
                                          nat_addr_type_t nat_addr_type)
{
    nat_src_key_t key = { 0 };
    uword *data;
    u16 ref_count;

    public_ip->as_u32 = 0;
    *public_port = 0;

    key.pvt_ip = pvt_ip;
    key.pvt_port = pvt_port;
    key.nat_proto = nat_proto;
    key.service = (nat_addr_type == NAT_ADDR_TYPE_INFRA);

    data = hash_get(vpc->nat_src_ht, key.as_u64);
    if (data) {
        NAT_EP_INC_REF_COUNT(*data, ref_count);
    }
}

always_inline void
nat_flow_set_src_endpoint_mapping(nat_vpc_config_t *vpc,
                                  ip4_address_t pvt_ip, u16 pvt_port,
                                  ip4_address_t public_ip, u16 public_port,
                                  nat_proto_t nat_proto,
                                  nat_addr_type_t nat_addr_type)
{
    nat_src_key_t key = { 0 };
    uword data;

    key.pvt_ip = pvt_ip;
    key.pvt_port = pvt_port;
    key.nat_proto = nat_proto;
    key.service = (nat_addr_type == NAT_ADDR_TYPE_INFRA);

    NAT_EP_SET_PUBLIC_IP_PORT(data, public_ip, public_port, 1);
    hash_set(vpc->nat_src_ht, key.as_u64, data);
}

always_inline void
nat_flow_get_and_del_src_endpoint_mapping(nat_vpc_config_t *vpc,
                                          ip4_address_t pvt_ip, u16 pvt_port,
                                          nat_proto_t nat_proto,
                                          nat_addr_type_t nat_addr_type)
{
    nat_src_key_t key = { 0 };
    uword *data;
    u16 ref_count;

    key.pvt_ip = pvt_ip;
    key.pvt_port = pvt_port;
    key.nat_proto = nat_proto;
    key.service = (nat_addr_type == NAT_ADDR_TYPE_INFRA);

    data = hash_get(vpc->nat_src_ht, key.as_u64);
    if (data) {
        NAT_EP_GET_REF_COUNT(*data, ref_count);
        ref_count--;
        if (ref_count == 0) {
            hash_unset(vpc->nat_src_ht, key.as_u64);
        }
    }
}

always_inline void
nat_flow_add_ip_mapping(nat_vpc_config_t *vpc,
                        ip4_address_t pvt_ip,
                        ip4_address_t public_ip,
                        nat_addr_type_t nat_addr_type)
{
    nat_pvt_ip_key_t key = { 0 };
    uword *data;

    key.pvt_ip = pvt_ip;
    key.service = (nat_addr_type == NAT_ADDR_TYPE_INFRA);
    data = hash_get(vpc->nat_pvt_ip_ht, key.as_u64);
    if (!data) {
        hash_set(vpc->nat_pvt_ip_ht, key.as_u64,
                 NAT_PVT_IP_HT_DATA(public_ip.as_u32, 1));
    } else {
        NAT_PVT_IP_HT_INC_REF_COUNT(*data);
    }
}

always_inline void
nat_flow_del_ip_mapping(nat_vpc_config_t *vpc,
                        ip4_address_t pvt_ip,
                        nat_addr_type_t nat_addr_type)
{
    nat_pvt_ip_key_t key = { 0 };
    uword *data;
    u16 ref_count;

    key.pvt_ip = pvt_ip;
    key.service = (nat_addr_type == NAT_ADDR_TYPE_INFRA);
    data = hash_get(vpc->nat_pvt_ip_ht, key.as_u64);
    if (data) {
        NAT_PVT_IP_HT_DEC_REF_COUNT(*data, ref_count);
        if (ref_count == 0) {
            hash_unset(vpc->nat_pvt_ip_ht, key.as_u64);
        }
    }
}

always_inline nat_err_t
nat_flow_alloc_for_pb(nat_vpc_config_t *vpc, nat_port_block_t *pb,
                      ip4_address_t sip, u16 sport, ip4_address_t pvt_ip,
                      u16 pvt_port, nat_flow_key_t *flow_key,
                      nat_hw_index_t *xlate_idx,
                      nat_hw_index_t *xlate_idx_rflow,
                      nat_addr_type_t nat_addr_type, nat_proto_t nat_proto)
{
    nat_hw_index_t rx_hw_index, tx_hw_index;
    u16 index = sport - pb->start_port;
    float port_use_percent;

    //
    // Allocate NAT hw indices
    // 
    clib_spinlock_lock(&nat_main.lock);
    if (PREDICT_FALSE(nat_main.num_hw_indices_allocated) >=
                      PDS_MAX_DYNAMIC_NAT - 2) {
        // no free nat hw indices;
        clib_spinlock_unlock(&nat_main.lock);
        return NAT_ERR_HW_TABLE_FULL;
    }
    if (vec_elt(pb->ref_count, index) == 0) {
        // Allocate HW NAT index for Tx direction
        tx_hw_index = nat_alloc_hw_index_no_lock();
        vec_elt(pb->nat_tx_hw_index, index) = tx_hw_index;
    }
    // Allocate HW NAT index for Rx direction
    rx_hw_index = nat_alloc_hw_index_no_lock();
    clib_spinlock_unlock(&nat_main.lock);

    if (vec_elt(pb->ref_count, index) == 0) {
        clib_bitmap_set_no_check(pb->port_bitmap, index, 1);
        pb->ports_in_use++;
        pds_snat_tbl_write_ip4(vec_elt(pb->nat_tx_hw_index, index),
                                       sip.as_u32, sport);
    }

    pds_snat_tbl_write_ip4(rx_hw_index, pvt_ip.as_u32, pvt_port);
    *xlate_idx = vec_elt(pb->nat_tx_hw_index, index);
    *xlate_idx_rflow = rx_hw_index;

    vec_elt(pb->ref_count, index)++;
    pb->num_flow_alloc++;

    // Add to the flow hash table
    hash_set_mem_alloc(&vpc->nat_flow_ht, flow_key,
                       NAT_FLOW_HT_MAKE_VAL(rx_hw_index, nat_addr_type, nat_proto));

    // Add to the src endpoint hash table
    nat_flow_set_src_endpoint_mapping(vpc, pvt_ip, pvt_port, sip, sport,
                                      nat_proto, nat_addr_type);

    // Add the pvt_ip to the pvt_ip hash table
    nat_flow_add_ip_mapping(vpc, pvt_ip, sip, nat_addr_type);

    if (pb->threshold && !pb->alert_raised) {
        port_use_percent = pb->ports_in_use * 100 /
                           (pb->end_port - pb->start_port + 1);
        if (port_use_percent > pb->threshold) {
            // TODO : raise alert event
            pb->alert_raised = 1;
        }
    }

    return NAT_ERR_OK;
}

static nat_err_t
nat_flow_alloc_for_pb_icmp(nat_vpc_config_t *vpc, nat_port_block_t *pb,
                          ip4_address_t sip, u16 sport, ip4_address_t pvt_ip,
                          u16 pvt_port, nat_flow_key_t *flow_key,
                          nat_hw_index_t *xlate_idx,
                          nat_hw_index_t *xlate_idx_rflow,
                          nat_addr_type_t nat_addr_type, nat_proto_t nat_proto)
{
    nat_hw_index_t rx_hw_index, tx_hw_index;
    u16 index = 0;

    //
    // Allocate NAT hw indices
    // 
    clib_spinlock_lock(&nat_main.lock);
    if (PREDICT_FALSE(nat_main.num_hw_indices_allocated) >=
                      PDS_MAX_DYNAMIC_NAT - 2) {
        // no free nat hw indices;
        clib_spinlock_unlock(&nat_main.lock);
        return NAT_ERR_HW_TABLE_FULL;
    }
    if (vec_elt(pb->ref_count, index) == 0) {
        // Allocate HW NAT index for Tx direction
        tx_hw_index = nat_alloc_hw_index_no_lock();
        vec_elt(pb->nat_tx_hw_index, index) = tx_hw_index;
    }
    // Allocate HW NAT index for Rx direction
    rx_hw_index = nat_alloc_hw_index_no_lock();
    clib_spinlock_unlock(&nat_main.lock);

    if (vec_elt(pb->ref_count, index) == 0) {
        clib_bitmap_set_no_check(pb->port_bitmap, index, 1);
        pds_snat_tbl_write_ip4(vec_elt(pb->nat_tx_hw_index, index),
                                       sip.as_u32, sport);
    }

    pds_snat_tbl_write_ip4(rx_hw_index, pvt_ip.as_u32, pvt_port);
    *xlate_idx = vec_elt(pb->nat_tx_hw_index, index);
    *xlate_idx_rflow = rx_hw_index;

    vec_elt(pb->ref_count, index)++;
    pb->num_flow_alloc++;

    // Add to the flow hash table
    hash_set_mem_alloc(&vpc->nat_flow_ht, flow_key,
                       NAT_FLOW_HT_MAKE_VAL(rx_hw_index, nat_addr_type, nat_proto));

    // Add to the src endpoint hash table
    nat_flow_set_src_endpoint_mapping(vpc, pvt_ip, pvt_port, sip, sport,
                                      nat_proto, nat_addr_type);

    // Add the pvt_ip to the pvt_ip hash table
    nat_flow_add_ip_mapping(vpc, pvt_ip, sip, nat_addr_type);

    return NAT_ERR_OK;
}


static nat_err_t
nat_flow_alloc_inline_icmp(u32 vpc_id, ip4_address_t dip, u16 dport,
                           ip4_address_t pvt_ip, u16 pvt_port,
                           ip4_address_t *sip, u16 *sport,
                           nat_port_block_t *pb, nat_vpc_config_t *vpc,
                           u8 protocol, nat_hw_index_t *xlate_idx,
                           nat_hw_index_t *xlate_idx_rflow,
                           nat_addr_type_t nat_addr_type, nat_proto_t nat_proto)
{
    u16 num_ports = 1;
    nat_flow_key_t key = { 0 };
    uword *v;

    if (pb->ref_count == NULL) {
        // No ports allocated
        pb->ref_count = vec_new(u32, num_ports);
        pb->nat_tx_hw_index = vec_new(u32, num_ports);
        clib_bitmap_alloc(pb->port_bitmap, num_ports);
    }

    key.dip = dip.as_u32;
    key.sip = pb->addr.as_u32;
    key.dport = dport;
    key.protocol = protocol;
    // No port allocation in ICMP, use pvt_port
    key.sport = pvt_port;

    v = hash_get_mem(vpc->nat_flow_ht, &key);
    if (!v) {
        // flow does not exist, this sip/sport can be allocated
        *sip = pb->addr;
        *sport = pvt_port;

        return nat_flow_alloc_for_pb_icmp(vpc, pb, *sip, *sport, pvt_ip,
                                          pvt_port, &key, xlate_idx,
                                          xlate_idx_rflow, nat_addr_type,
                                          nat_proto);

    }

    return NAT_ERR_NO_RESOURCE;
}

always_inline nat_err_t
nat_flow_alloc_inline(u32 vpc_id, ip4_address_t dip, u16 dport,
                      ip4_address_t pvt_ip, u16 pvt_port,
                      ip4_address_t *sip, u16 *sport,
                      nat_port_block_t *pb, nat_vpc_config_t *vpc,
                      u8 protocol, nat_hw_index_t *xlate_idx,
                      nat_hw_index_t *xlate_idx_rflow,
                      nat_addr_type_t nat_addr_type, nat_proto_t nat_proto)
{
    u16 num_ports;
    nat_flow_key_t key = { 0 };
    uword *v;
    u16 try_sport;

    if (PREDICT_FALSE(protocol == IP_PROTOCOL_ICMP)) {
        return nat_flow_alloc_inline_icmp(vpc_id, dip, dport, pvt_ip, pvt_port,
                                          sip, sport, pb, vpc, protocol,
                                          xlate_idx, xlate_idx_rflow,
                                          nat_addr_type, nat_proto);

    }

    num_ports = pb->end_port - pb->start_port + 1;
    if (num_ports == pb->ports_in_use) {
        // When all ports in port block are used up, don't bother to walk the
        // list of ports to see if we can reuse any of them
        return NAT_ERR_NO_RESOURCE;
    }

    if (pb->ref_count == NULL) {
        // No ports allocated
        pb->ref_count = vec_new(u32, num_ports);
        pb->nat_tx_hw_index = vec_new(u32, num_ports);
        clib_bitmap_alloc(pb->port_bitmap, num_ports);
    }

    key.dip = dip.as_u32;
    key.sip = pb->addr.as_u32;
    key.dport = dport;
    key.protocol = protocol;

    for (try_sport = pb->start_port; try_sport <= pb->end_port; try_sport++) {
        key.sport = try_sport;
        v = hash_get_mem(vpc->nat_flow_ht, &key);
        if (!v) {
            // flow does not exist, this sip/sport can be allocated
            *sip = pb->addr;
            *sport = try_sport;

            return nat_flow_alloc_for_pb(vpc, pb, *sip, *sport, pvt_ip,
                                         pvt_port, &key, xlate_idx,
                                         xlate_idx_rflow, nat_addr_type,
                                         nat_proto);
        }
    }

    return NAT_ERR_NO_RESOURCE;
}

always_inline nat_port_block_t *
nat_get_pool_for_pvt_ip(nat_vpc_config_t *vpc, nat_port_block_t *nat_pb,
                        ip4_address_t pvt_ip, nat_addr_type_t nat_addr_type,
                        nat_proto_t nat_proto)
{
    nat_pvt_ip_key_t key = { 0 };
    uword *data;
    ip4_address_t public_ip;
    nat_port_block_t *pb = NULL;

    key.pvt_ip = pvt_ip;
    key.service = (nat_addr_type == NAT_ADDR_TYPE_INFRA);
    data = hash_get(vpc->nat_pvt_ip_ht, key.as_u64);
    if (data) {
        public_ip.as_u32 = NAT_PVT_IP_HT_PUBLIC_IP(*data);
        pb = nat_get_port_block_from_pub_ip(vpc, nat_addr_type, nat_proto,
                                            public_ip);
    }

    return pb;
}

always_inline nat_port_block_t *
nat_get_random_pool(nat_vpc_config_t *vpc, nat_port_block_t *nat_pb)
{
    u32 num_elts = pool_elts(nat_pb);
    u32 elt = random_u32(&vpc->random_seed) % num_elts;
    u32 i;
    nat_port_block_t *pb = NULL;

    i = 0;
    pool_foreach (pb, nat_pb,
    ({
        if (i == elt && pb->state == NAT_PB_STATE_OK) {
            return pb;
        }
        i++;
    }));

    return NULL;
}

//
// Allocate SNAT port during flow creation (VPP datapath)
//
nat_err_t
nat_flow_alloc(u32 vpc_id, ip4_address_t dip, u16 dport,
               u8 protocol, ip4_address_t pvt_ip, u16 pvt_port,
               nat_addr_type_t nat_addr_type, ip4_address_t *sip, u16 *sport,
               nat_hw_index_t *xlate_idx, nat_hw_index_t *xlate_idx_rflow)
{
    nat_port_block_t *pb = NULL;
    nat_vpc_config_t *vpc;
    nat_err_t ret = NAT_ERR_OK;
    nat_proto_t nat_proto;
    nat_port_block_t *nat_pb;

    if (PREDICT_FALSE(vpc_id >= vec_len(nat_main.vpc_config))) {
        return NAT_ERR_NO_RESOURCE;
    }
    vpc = vec_elt(nat_main.vpc_config, vpc_id);
    if (PREDICT_FALSE(!vpc || vpc->num_port_blocks == 0)) {
        return NAT_ERR_NO_RESOURCE;
    }

    nat_proto = get_nat_proto_from_proto(protocol);
    if (PREDICT_FALSE(nat_proto == NAT_PROTO_UNKNOWN)) {
        return NAT_ERR_INVALID_PROTOCOL;
    }

    clib_spinlock_lock(&vpc->lock);

    nat_pb = vpc->nat_pb[nat_addr_type][nat_proto - 1];
    if (PREDICT_FALSE(pool_elts(nat_pb) == 0)) {
        clib_spinlock_unlock(&vpc->lock);
        return NAT_ERR_NO_RESOURCE;
    }

    // if we have already allocated <pvt_ip, pvt_port>, try to use it
    nat_flow_get_and_add_src_endpoint_mapping(vpc, pvt_ip, pvt_port, sip,
                                              sport, nat_proto, nat_addr_type);
    if (*sport == 0) {
        // endpoint not found
        // Check if pvt_ip has been used
        pb = nat_get_pool_for_pvt_ip(vpc, nat_pb, pvt_ip, nat_addr_type, nat_proto);
        if (!pb) {
            // Try random pool
            pb = nat_get_random_pool(vpc, nat_pb);
        }
        if (pb) {
            ret = nat_flow_alloc_inline(vpc_id, dip, dport, pvt_ip, pvt_port,
                    sip, sport, pb, vpc, protocol, xlate_idx, xlate_idx_rflow,
                    nat_addr_type, nat_proto);
        }
        if (!pb || ret != NAT_ERR_OK) {
            // random pool did not work, try all pools
            u8 allocated = 0;
            pool_foreach (pb, nat_pb,
            ({
                if (allocated || pb->state != NAT_PB_STATE_OK) continue;
                ret = nat_flow_alloc_inline(vpc_id, dip, dport, pvt_ip,
                                            pvt_port, sip, sport, pb, vpc,
                                            protocol, xlate_idx,
                                            xlate_idx_rflow, nat_addr_type,
                                            nat_proto);
                if (ret == NAT_ERR_OK) allocated = 1;
            }));
        }

    } else {
        // src endpoint found, reuse public ip, port mapping
        pb = nat_get_port_block_from_pub_ip(vpc, nat_addr_type, nat_proto, *sip);
        if (pb) {
            nat_flow_key_t key = { 0 };
            key.dip = dip.as_u32;
            key.sip = sip->as_u32;
            key.sport = *sport;
            key.dport = dport;
            key.protocol = protocol;
            nat_flow_alloc_for_pb(vpc, pb, *sip, *sport, pvt_ip, pvt_port,
                                  &key, xlate_idx, xlate_idx_rflow,
                                  nat_addr_type, nat_proto);
        }
    }
    clib_spinlock_unlock(&vpc->lock);

    if (ret != NAT_ERR_OK) {
        vpc->alloc_fail++;
    }

    return ret;
}

always_inline nat_err_t
nat_flow_dealloc_inline_icmp(u32 vpc_id, ip4_address_t dip, u16 dport,
                             ip4_address_t sip, u16 sport,
                             nat_port_block_t *pb)
{
    u16 index = 0;

    pb->ref_count[index]--;
    if (pb->ref_count[index] == 0) {
        clib_bitmap_set_no_check(pb->port_bitmap, index, 0);

        clib_spinlock_lock(&nat_main.lock);
        nat_free_hw_index_no_lock(vec_elt(pb->nat_tx_hw_index, index));
        clib_spinlock_unlock(&nat_main.lock);
    }
    pb->num_flow_alloc--;
    if (pb->num_flow_alloc == 0) {
        vec_free(pb->ref_count);
        vec_free(pb->nat_tx_hw_index);
        clib_bitmap_free(pb->port_bitmap);
    }

    return NAT_ERR_OK;
}

always_inline nat_err_t
nat_flow_dealloc_inline(u32 vpc_id, ip4_address_t dip, u16 dport,
                        ip4_address_t sip, u16 sport,
                        nat_port_block_t *pb)
{
    u16 index;
    float port_use_percent;

    if (sport < pb->start_port || sport > pb->end_port) {
        // not in this port block
        return NAT_ERR_NOT_FOUND;
    }

    index = sport - pb->start_port;
    pb->ref_count[index]--;
    if (pb->ref_count[index] == 0) {
        clib_bitmap_set_no_check(pb->port_bitmap, index, 0);
        pb->ports_in_use--;

        clib_spinlock_lock(&nat_main.lock);
        nat_free_hw_index_no_lock(vec_elt(pb->nat_tx_hw_index, index));
        clib_spinlock_unlock(&nat_main.lock);
    }
    pb->num_flow_alloc--;
    if (pb->num_flow_alloc == 0) {
        vec_free(pb->ref_count);
        vec_free(pb->nat_tx_hw_index);
        clib_bitmap_free(pb->port_bitmap);
    }

    if (pb->threshold && pb->alert_raised) {
        port_use_percent = pb->ports_in_use * 100 /
                           (pb->end_port - pb->start_port + 1);
        // 10% hysteresis
        if (port_use_percent <= clib_max(pb->threshold - 10, 0)) {
            pb->alert_raised = 0;
        }
    }

    return NAT_ERR_OK;
}

//
// Deallocate SNAT port for flow
//
nat_err_t
nat_flow_dealloc(u32 vpc_id, ip4_address_t dip, u16 dport, u8 protocol,
                 ip4_address_t sip, u16 sport)
{
    nat_port_block_t *pb = NULL;
    nat_vpc_config_t *vpc;
    nat_flow_key_t key = { 0 };
    nat_err_t ret = NAT_ERR_NOT_FOUND;
    uword *data, hw_index;
    nat_proto_t nat_proto;
    nat_addr_type_t nat_addr_type;
    ip4_address_t pvt_ip;
    u16 pvt_port;

    if (PREDICT_FALSE(vpc_id >= vec_len(nat_main.vpc_config))) {
        return NAT_ERR_NOT_FOUND;
    }
    vpc = vec_elt(nat_main.vpc_config, vpc_id);
    if (PREDICT_FALSE(!vpc)) {
        return NAT_ERR_NOT_FOUND;
    }

    key.dip = dip.as_u32;
    key.sip = sip.as_u32;
    key.dport = dport;
    key.sport = sport;
    key.protocol = protocol;

    clib_spinlock_lock(&vpc->lock);

    data = hash_get_mem(vpc->nat_flow_ht, &key);
    if (PREDICT_FALSE(!data)) {
        clib_spinlock_unlock(&vpc->lock);
        return NAT_ERR_NOT_FOUND;
    }
    NAT_FLOW_HT_GET_VAL(*data, hw_index, nat_addr_type, nat_proto)

    pds_snat_tbl_read_ip4(hw_index, &pvt_ip.as_u32, &pvt_port);

    nat_flow_get_and_del_src_endpoint_mapping(vpc, pvt_ip, pvt_port, nat_proto,
                                              nat_addr_type);

    hash_unset_mem_free(&vpc->nat_flow_ht, &key);
    nat_flow_del_ip_mapping(vpc, pvt_ip, nat_addr_type);

    pb = nat_get_port_block_from_pub_ip(vpc, nat_addr_type, nat_proto, sip);
    if (pb) {
        if (PREDICT_TRUE(protocol != IP_PROTOCOL_ICMP)) {
            ret = nat_flow_dealloc_inline(vpc_id, dip, dport, sip,
                                          sport, pb);
        } else {
            ret = nat_flow_dealloc_inline_icmp(vpc_id, dip, dport, sip,
                                               sport, pb);
        }
    }
    clib_spinlock_unlock(&vpc->lock);

    // free HW NAT indices
    clib_spinlock_lock(&nat_main.lock);
    nat_free_hw_index_no_lock(hw_index);
    clib_spinlock_unlock(&nat_main.lock);

    if (ret != NAT_ERR_OK) {
        vpc->dealloc_fail++;
    }

    return NAT_ERR_OK;
}

//
// Check if destination is part of local NAT port blocks
//
void
nat_flow_is_dst_valid(u32 vpc_id, ip4_address_t dip, u16 dport,
                      u8 protocol, nat_addr_type_t nat_addr_type,
                      bool *dstip_valid, bool *dstport_valid)
{
    nat_port_block_t *pb = NULL;
    nat_vpc_config_t *vpc;
    nat_proto_t nat_proto;
    nat_port_block_t *nat_pb;

    *dstip_valid = false;
    *dstport_valid = false;

    if (PREDICT_FALSE(vpc_id >= vec_len(nat_main.vpc_config))) {
        return;
    }
    vpc = vec_elt(nat_main.vpc_config, vpc_id);
    if (PREDICT_FALSE(!vpc || vpc->num_port_blocks == 0)) {
        return;
    }

    nat_proto = get_nat_proto_from_proto(protocol);
    if (PREDICT_FALSE(nat_proto == NAT_PROTO_UNKNOWN)) {
        return;
    }

    clib_spinlock_lock(&vpc->lock);

    nat_pb = vpc->nat_pb[nat_addr_type][nat_proto - 1];
    if (PREDICT_FALSE(pool_elts(nat_pb) == 0)) {
        clib_spinlock_unlock(&vpc->lock);
        return;
    }

    pool_foreach (pb, nat_pb,
    ({
        if (pb->addr.as_u32 == dip.as_u32 &&
            dport >= pb->start_port &&
            dport <= pb->end_port) {
            *dstip_valid = true;
            *dstport_valid = true;
            clib_spinlock_unlock(&vpc->lock);
            return;
        } else if (pb->addr.as_u32 == dip.as_u32) {
            *dstip_valid = true;
        }
    }));

    clib_spinlock_unlock(&vpc->lock);
    return;
}

//
// Get pvt_ip, pvt_port for flow
//
nat_err_t
nat_flow_xlate(u32 vpc_id, ip4_address_t dip, u16 dport,
               u8 protocol, ip4_address_t sip, u16 sport,
               ip4_address_t *pvt_ip, u16 *pvt_port)
{
    nat_flow_key_t key = { 0 };
    uword *data, hw_index;
    nat_vpc_config_t *vpc;

    if (PREDICT_FALSE(vpc_id >= vec_len(nat_main.vpc_config))) {
        return NAT_ERR_NOT_FOUND;
    }
    vpc = vec_elt(nat_main.vpc_config, vpc_id);
    if (PREDICT_FALSE(!vpc)) {
        return NAT_ERR_NOT_FOUND;
    }

    key.dip = dip.as_u32;
    key.sip = sip.as_u32;
    key.dport = dport;
    key.sport = sport;
    key.protocol = protocol;

    clib_spinlock_lock(&vpc->lock);

    data = hash_get_mem(vpc->nat_flow_ht, &key);
    if (PREDICT_FALSE(!data)) {
        clib_spinlock_unlock(&vpc->lock);
        return NAT_ERR_NOT_FOUND;
    }
    clib_spinlock_unlock(&vpc->lock);

    NAT_FLOW_HT_GET_HW_INDEX(*data, hw_index)

    pds_snat_tbl_read_ip4(hw_index, &pvt_ip->as_u32, pvt_port);

    return NAT_ERR_OK;
}

//
// Get SNAT usage
//
nat_err_t
nat_usage(u32 vpc_id, u8 protocol, nat_addr_type_t nat_addr_type,
          u32 *num_ports_total, u32 *num_ports_alloc,
          u32 *num_flows_alloc)
{
    nat_port_block_t *pb;
    nat_vpc_config_t *vpc;
    nat_proto_t nat_proto;
    nat_port_block_t *nat_pb;

    nat_proto = get_nat_proto_from_proto(protocol);
    if (nat_proto == NAT_PROTO_UNKNOWN) {
        return NAT_ERR_INVALID_PROTOCOL;
    }

    *num_ports_total = 0;
    *num_ports_alloc = 0;
    *num_flows_alloc = 0;

    vpc = vec_elt(nat_main.vpc_config, vpc_id);

    nat_pb = vpc->nat_pb[nat_addr_type][nat_proto - 1];

    pool_foreach (pb, nat_pb,
    ({
        *num_ports_total += pb->end_port - pb->start_port + 1;
        *num_ports_alloc += pb->ports_in_use;
        *num_flows_alloc += pb->num_flow_alloc;
    }));

    return NAT_ERR_OK;
}

//
// Get NAT hw usage
//
nat_err_t
nat_hw_usage(u32 *total_hw_indices, u32 *total_alloc_indices)
{
    *total_hw_indices = PDS_MAX_DYNAMIC_NAT;
    *total_alloc_indices = nat_main.num_hw_indices_allocated;

    return NAT_ERR_OK;
}

void
nat_get_global_stats(nat_global_stats_t *stats)
{
    stats->hw_index_total = PDS_MAX_DYNAMIC_NAT;
    stats->hw_index_alloc = nat_main.num_hw_indices_allocated;
}

uint16_t
nat_pb_count() {
    uint16_t count = 0;
    for (int i = 0; i < vec_len(nat_main.vpc_config); i++) {
        count += nat_main.vpc_config[i]->num_port_blocks;
    }
    return count;
}

//
// Iterate over NAT flows
//
void *
nat_flow_iterate(void *ctxt, nat_flow_iter_cb_t iter_cb)
{
    nat_vpc_config_t *vpc;
    nat_flow_key_t *key;
    u64 value;

    for (int vpc_id = 0; vpc_id < vec_len(nat_main.vpc_config); vpc_id++) {
        vpc = vec_elt(nat_main.vpc_config, vpc_id);
        if (!vpc || vpc->num_port_blocks == 0) {
            continue;
        }
        hash_foreach_mem(key, value, vpc->nat_flow_ht,
        ({
            ctxt = iter_cb(ctxt, vpc_id, key, value);
        }));
    }

    return ctxt;
}

static void *
nat_cli_show_flow_cb(void *ctxt, u32 vpc_id, nat_flow_key_t *key, u64 val)
{
    u32 hw_index;
    nat_proto_t nat_proto;
    nat_addr_type_t nat_addr_type;
    ip4_address_t sip, dip, pvt_ip;
    u16 sport, dport, pvt_port;
    vlib_main_t *vm = (vlib_main_t *)ctxt;

    NAT_FLOW_HT_GET_VAL(val, hw_index, nat_addr_type, nat_proto)
    (void)nat_proto;

    pds_snat_tbl_read_ip4(hw_index, &pvt_ip.as_u32, &pvt_port);

    sip.as_u32 = clib_host_to_net_u32(key->sip);
    dip.as_u32 = clib_host_to_net_u32(key->dip);
    sport = clib_host_to_net_u16(key->sport);
    dport = clib_host_to_net_u16(key->dport);
    pvt_ip.as_u32 = clib_host_to_net_u32(pvt_ip.as_u32);
    pvt_port = clib_host_to_net_u16(pvt_port);
    vlib_cli_output(vm, "%-14U%-14U%-10U%-10U%-10U%-10d%-10d%-10U%-10U\n",
                    format_ip4_address, &sip,
                    format_ip4_address, &dip,
                    format_tcp_udp_port, sport,
                    format_tcp_udp_port, dport,
                    format_ip_protocol, key->protocol,
                    hw_index, nat_addr_type,
                    format_ip4_address, &pvt_ip,
                    format_tcp_udp_port, pvt_port);

    return vm;
}

typedef struct nat_clear_cb_s {
    nat_flow_key_t key;
    u64 val;
    u32 vpc_id;
} nat_clear_cb_t;

static void *
nat_cli_clear_flow_cb(void *ctxt, u32 vpc_id, nat_flow_key_t *key, u64 val)
{
    nat_clear_cb_t *clear_flow_pool = (nat_clear_cb_t *)ctxt;
    nat_clear_cb_t *elem;

    pool_get(clear_flow_pool, elem);
    elem->key = *key;
    elem->val = val;
    elem->vpc_id = vpc_id;

    return clear_flow_pool;
}

static clib_error_t *
nat_cli_show_flow(vlib_main_t *vm,
                  unformat_input_t *input,
                  vlib_cli_command_t *cmd)
{
    vlib_cli_output(vm, "%-14s%-14s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\n",
                    "SIP", "DIP", "SPORT", "DPORT", "PROTO", "HW INDEX", "ADDR TYPE", "PVT IP", "PVT PORT");
    (void)nat_flow_iterate((void *)vm, &nat_cli_show_flow_cb);

    return 0;
}

static clib_error_t *
nat_clear_flow_helper(bool vpp_cli, vlib_main_t *vm)
{
    nat_clear_cb_t *clear_flow_pool = NULL;
    nat_clear_cb_t *elem;
    nat_err_t nat_ret;

    clear_flow_pool = nat_flow_iterate((void *)clear_flow_pool, &nat_cli_clear_flow_cb);

    pool_foreach (elem, clear_flow_pool,
    ({
        nat_ret = nat_flow_dealloc(elem->vpc_id, (ip4_address_t) elem->key.dip,
                                   elem->key.dport, elem->key.protocol,
                                   (ip4_address_t) elem->key.sip,
                                   elem->key.sport);
        if (nat_ret != NAT_ERR_OK) {
            if (vpp_cli) {
                vlib_cli_output(vm, "NAT flow delete failed %d", nat_ret);
            }
        }
    }));

    pool_free(clear_flow_pool);
    return 0;
}

static clib_error_t *
nat_cli_clear_all_flows(vlib_main_t *vm,
                        unformat_input_t *input,
                        vlib_cli_command_t *cmd)
{
    return nat_clear_flow_helper(true, vm);
}

int
nat_clear_all_flows(void)
{
    nat_clear_flow_helper(false, NULL);
    return 0;
}

VLIB_CLI_COMMAND(nat_cli_show_flow_command, static) =
{
    .path = "show nat flow",
    .short_help = "show nat flow",
    .function = nat_cli_show_flow,
};

VLIB_CLI_COMMAND(nat_cli_clear_flow_command, static) =
{
    .path = "clear nat flow",
    .short_help = "clear nat flow",
    .function = nat_cli_clear_all_flows,
};

void
nat_sync_start(void)
{
    clib_spinlock_lock(&nat_main.lock);
}

void
nat_sync_stop(void)
{
    clib_spinlock_unlock(&nat_main.lock);
}

void
nat_sync_stats_incr(nat_sync_stats_t stat)
{
    nat_main.sync_stats[stat]++;
}

nat_err_t
nat_sync_restore(nat44_sync_info_t *sync)
{
    nat_port_block_t *pb = NULL, *pb_iter;
    nat_vpc_config_t *vpc;
    nat_proto_t nat_proto;
    nat_port_block_t *nat_pb;
    u16 num_ports, index;
    nat_flow_key_t flow_key;

    if (PREDICT_FALSE(sync->vpc_id >= vec_len(nat_main.vpc_config))) {
        return NAT_ERR_NO_RESOURCE;
    }
    vpc = vec_elt(nat_main.vpc_config, sync->vpc_id);
    if (PREDICT_FALSE(!vpc || vpc->num_port_blocks == 0)) {
        return NAT_ERR_NO_RESOURCE;
    }

    nat_proto = get_nat_proto_from_proto(sync->proto);
    if (PREDICT_FALSE(nat_proto == NAT_PROTO_UNKNOWN)) {
        return NAT_ERR_INVALID_PROTOCOL;
    }

    nat_pb = vpc->nat_pb[sync->nat_addr_type][nat_proto - 1];
    if (PREDICT_FALSE(pool_elts(nat_pb) == 0)) {
        clib_spinlock_unlock(&vpc->lock);
        return NAT_ERR_NO_RESOURCE;
    }
    pool_foreach (pb_iter, nat_pb,
    ({
        if (pb_iter->addr.as_u32 == sync->public_sip)
             pb = pb_iter;
    }));
    if (!pb) {
        return NAT_ERR_NO_RESOURCE;
    }

    num_ports = pb->end_port - pb->start_port + 1;
    if (pb->ref_count == NULL) {
        // No ports allocated
        pb->ref_count = vec_new(u32, num_ports);
        pb->nat_tx_hw_index = vec_new(u32, num_ports);
        clib_bitmap_alloc(pb->port_bitmap, num_ports);
    }
    index = sync->public_sport - pb->start_port;
    if (PREDICT_FALSE(nat_main.num_hw_indices_allocated) >=
                      PDS_MAX_DYNAMIC_NAT - 2) {
        // no free nat hw indeices
        return NAT_ERR_HW_TABLE_FULL;
    }
    if (vec_elt(pb->ref_count, index) == 0) {
        // Allocate HW NAT index for Tx direction
        nat_alloc_hw_index_at_index_no_lock(sync->tx_xlate_id);
    } else {
        // assert sync->tx_xlate_id == vec_elt(pb->nat_tx_hw_index, index)
    }
    // Allocate HW NAT index for Rx direction
    nat_alloc_hw_index_at_index_no_lock(sync->rx_xlate_id);

    if (vec_elt(pb->ref_count, index) == 0) {
        clib_bitmap_set_no_check(pb->port_bitmap, index, 1);
        pb->ports_in_use++;
        vec_elt(pb->nat_tx_hw_index, index) = sync->tx_xlate_id;
        pds_snat_tbl_write_ip4(sync->tx_xlate_id, sync->public_sip, sync->public_sport);
    }

    pds_snat_tbl_write_ip4(sync->rx_xlate_id, sync->pvt_sip, sync->pvt_sport);

    vec_elt(pb->ref_count, index)++;
    pb->num_flow_alloc++;

    // Add to the flow hash table
    flow_key.dip = sync->dip;
    flow_key.sip = sync->public_sip;
    flow_key.sport = sync->public_sport;
    flow_key.dport = sync->dport;
    flow_key.protocol = sync->proto;
    hash_set_mem_alloc(&vpc->nat_flow_ht, &flow_key,
                       NAT_FLOW_HT_MAKE_VAL(sync->rx_xlate_id,
                                            sync->nat_addr_type, nat_proto));

    // Add to the src endpoint hash table
    nat_flow_set_src_endpoint_mapping(vpc, (ip4_address_t)sync->pvt_sip,
                                      sync->pvt_sport,
                                      (ip4_address_t)sync->public_sip,
                                      sync->public_sport,
                                      nat_proto, sync->nat_addr_type);

    // Add the pvt_ip to the pvt_ip hash table
    nat_flow_add_ip_mapping(vpc, (ip4_address_t)sync->pvt_sip,
                            (ip4_address_t)sync->public_sip,
                            sync->nat_addr_type);


    return NAT_ERR_OK;
}

static clib_error_t *
nat_cli_show_sync_stats(vlib_main_t *vm,
                        unformat_input_t *input,
                        vlib_cli_command_t *cmd)
{
    vlib_cli_output(vm, "NAT flow sync statistics\n");
    vlib_cli_output(vm, "========================\n");
    vlib_cli_output(vm, "%-40s: %lu", "NAT flow encode",
                    nat_main.sync_stats[NAT_SYNC_STATS_ENCODE]);
    vlib_cli_output(vm, "%-40s: %lu", "NAT flow decode",
                    nat_main.sync_stats[NAT_SYNC_STATS_DECODE]);
    return 0;
}

VLIB_CLI_COMMAND(nat_cli_show_nat_sync_stats_command, static) =
{
    .path = "show nat sync",
    .short_help = "show nat sync",
    .function = nat_cli_show_sync_stats,
};
