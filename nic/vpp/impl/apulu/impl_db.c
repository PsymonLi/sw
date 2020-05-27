//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#include "impl_db.h"

pds_impl_db_ctx_t impl_db_ctx;

#define POOL_IMPL_DB_ADD(obj, hw_id)                            \
    pds_impl_db_##obj##_entry_t *obj##_info;                    \
    u16 offset;                                                 \
    if (hw_id >= vec_len(impl_db_ctx.obj##_pool_idx))           \
        return -1;                                              \
    offset = vec_elt(impl_db_ctx.obj##_pool_idx, hw_id);        \
    if (offset == 0xffff) {                                     \
    pool_get(impl_db_ctx.obj##_pool_base, obj##_info);          \
    offset = obj##_info - impl_db_ctx.obj##_pool_base;          \
    vec_elt(impl_db_ctx.obj##_pool_idx, hw_id) = offset;        \
    clib_memset(obj##_info, 0, sizeof(*obj##_info));            \
    } else {                                                    \
    obj##_info =  pool_elt_at_index(impl_db_ctx.obj##_pool_base,\
                                    offset);                    \
    }                                                           \

#define POOL_IMPL_DB_GET(obj, hw_id)                            \
    pds_impl_db_##obj##_entry_t *obj##_info;                    \
    if (hw_id >= vec_len(impl_db_ctx.obj##_pool_idx))           \
        return NULL;                                            \
    u16 _offset = vec_elt(impl_db_ctx.obj##_pool_idx, hw_id);   \
    if (_offset == 0xffff) return NULL;                         \
    obj##_info = pool_elt_at_index(impl_db_ctx.obj##_pool_base, \
                                   _offset);                    \

#define IMPL_DB_ENTRY_DEL(type, obj)                            \
int                                                             \
pds_impl_db_##obj##_del (type hw_id)                            \
{                                                               \
    u16 offset;                                                 \
                                                                \
    if (hw_id >= vec_len(impl_db_ctx.obj##_pool_idx))           \
        return -1;                                              \
    offset = vec_elt(impl_db_ctx.obj##_pool_idx, hw_id);        \
    if (offset == 0xffff) return -1;                            \
    pool_put_index(impl_db_ctx.obj##_pool_base, offset);        \
    vec_elt(impl_db_ctx.obj##_pool_idx, hw_id) = 0xffff;        \
    return 0;                                                   \
}                                                               \

#define IMPL_DB_INIT(obj, len, def_val)                         \
void                                                            \
pds_impl_db_##obj##_init (void)                                 \
{                                                               \
    vec_validate_init_empty(impl_db_ctx.obj##_pool_idx,         \
                            (len - 1), def_val);                \
    impl_db_ctx.obj##_pool_base = NULL;                         \
    return;                                                     \
}                                                               \

#define IMPL_DB_ENTRY_GET(type, obj)                            \
pds_impl_db_##obj##_entry_t *                                   \
pds_impl_db_##obj##_get (type hw_id)                            \
{                                                               \
    POOL_IMPL_DB_GET(obj, hw_id);                               \
    return obj##_info;                                          \
}                                                               \

int
pds_impl_db_vnic_set (uint8_t *mac,
                      uint32_t max_sessions,
                      uint16_t vnic_hw_id,
                      uint16_t subnet_hw_id,
                      uint8_t flow_log_en,
                      uint8_t dot1q,
                      uint8_t dot1ad,
                      uint16_t vlan_id,
                      uint16_t nh_hw_id,
                      uint16_t host_lif_hw_id)
{
    ethernet_header_t *eth = NULL;
    ethernet_vlan_header_t *vlan = NULL;
    pds_impl_db_subnet_entry_t *subnet_info = NULL;

    subnet_info = pds_impl_db_subnet_get(subnet_hw_id);
    if(subnet_info == NULL) {
        return -1;
    }

    POOL_IMPL_DB_ADD(vnic, vnic_hw_id);

    clib_memcpy(vnic_info->mac, mac, ETH_ADDR_LEN);
    vnic_info->max_sessions = max_sessions;
    vnic_info->subnet_hw_id = subnet_hw_id;
    if (flow_log_en) {
        vnic_info->flow_log_en = 1;
    } else {
        vnic_info->flow_log_en = 0;
    }
    vnic_info->l2_encap_len = sizeof(ethernet_header_t);
    vnic_info->vlan_id = vlan_id;
    if (dot1q) {
        vnic_info->encap_type = PDS_ETH_ENCAP_DOT1Q;
        vnic_info->l2_encap_len += sizeof(ethernet_vlan_header_t);
    } else if (dot1ad) {
        vnic_info->encap_type = PDS_ETH_ENCAP_DOT1AD;
        vnic_info->l2_encap_len += (2 * sizeof(ethernet_vlan_header_t));
    } else {
        vnic_info->encap_type = PDS_ETH_ENCAP_NO_VLAN;
    }
    vnic_info->nh_hw_id = nh_hw_id;
    vnic_info->host_lif_hw_id = host_lif_hw_id;

    vec_free(vnic_info->rewrite);
    vec_validate(vnic_info->rewrite, vnic_info->l2_encap_len - 1);
    eth = (ethernet_header_t *) vnic_info->rewrite;

    clib_memcpy(eth->src_address, subnet_info->mac, ETH_ADDR_LEN);
    clib_memcpy(eth->dst_address, mac, ETH_ADDR_LEN);
    eth->type = clib_host_to_net_u16(ETHERNET_TYPE_IP4); // default
    if (dot1q) {
        eth->type = clib_host_to_net_u16(ETHERNET_TYPE_VLAN);
        vlan = (ethernet_vlan_header_t *) (eth + 1);
        vlan->priority_cfi_and_id = clib_host_to_net_u16(vlan_id);
        vlan->type = clib_host_to_net_u16(ETHERNET_TYPE_IP4); // default
    }
    return 0;
}

int
pds_impl_db_vnic_del (uint16_t hw_id)
{
    u16 offset;
    pds_impl_db_vnic_entry_t *vnic_info = NULL;

    if (hw_id >= vec_len(impl_db_ctx.vnic_pool_idx)) {
        return -1;
    }
    offset = vec_elt(impl_db_ctx.vnic_pool_idx, hw_id);
    if (offset == 0xffff) {
        return -1;
    }

    vnic_info = pool_elt_at_index(impl_db_ctx.vnic_pool_base, offset);
    vec_free(vnic_info->rewrite);
    pool_put_index(impl_db_ctx.vnic_pool_base, offset);
    vec_elt(impl_db_ctx.vnic_pool_idx, hw_id) = 0xffff;
    return 0;
}

IMPL_DB_ENTRY_GET(uint16_t, vnic);
IMPL_DB_INIT(vnic, PDS_VPP_MAX_VNIC, 0xffff);

int
pds_impl_db_subnet_set (uint8_t pfx_len,
                        uint32_t vr_ip,
                        uint8_t *mac,
                        uint16_t subnet_hw_id,
                        uint32_t vnid,
                        uint16_t vpc_id)
{
    POOL_IMPL_DB_ADD(subnet, subnet_hw_id);

    subnet_info->prefix_len = pfx_len;
    ip46_address_set_ip4(&subnet_info->vr_ip, (ip4_address_t *) &vr_ip);
    clib_memcpy(subnet_info->mac, mac, ETH_ADDR_LEN);
    // store vnid in format required for vxlan packet so that no
    // conversion required in packet path
    vnid = vnid << 8;
    subnet_info->vnid = clib_host_to_net_u32(vnid);
    subnet_info->vpc_id = vpc_id;
    return 0;
}

IMPL_DB_ENTRY_DEL(uint16_t, subnet);
IMPL_DB_ENTRY_GET(uint16_t, subnet);
IMPL_DB_INIT(subnet, PDS_VPP_MAX_SUBNET, 0xffff);

int
pds_impl_db_vpc_set (uint16_t vpc_hw_id, uint16_t bd_hw_id, uint16_t flags)
{
    POOL_IMPL_DB_ADD(vpc, vpc_hw_id);

    vpc_info->hw_bd_id = bd_hw_id;
    vpc_info->flags = flags;
    return 0;
}

uint8_t
pds_impl_db_vpc_is_control_vpc (uint16_t vpc_hw_id)
{
    pds_impl_db_vpc_entry_t *entry = pds_impl_db_vpc_get(vpc_hw_id);

    return (entry->flags & PDS_VPP_VPC_FLAGS_CONTROL_VPC);
}

IMPL_DB_ENTRY_DEL(uint16_t, vpc);
IMPL_DB_ENTRY_GET(uint16_t, vpc);
IMPL_DB_INIT(vpc, PDS_VPP_MAX_VPC, 0xffff);

int
pds_impl_db_vr_ip_mac_get (uint16_t subnet, uint32_t *vr_ip, uint8_t **vr_mac)
{
    pds_impl_db_subnet_entry_t *entry = pds_impl_db_subnet_get(subnet);
    if (PREDICT_FALSE(!entry)) {
        return -1;
    }
    *vr_ip = entry->vr_ip.ip4.data_u32;
    *vr_mac = entry->mac;
    return 0;
}

int
pds_impl_db_vr_ip_get (uint16_t subnet, uint32_t *vr_ip)
{
    pds_impl_db_subnet_entry_t *entry = pds_impl_db_subnet_get(subnet);
    if (PREDICT_FALSE(!entry)) {
        return -1;
    }
    *vr_ip = entry->vr_ip.ip4.data_u32;
    return 0;
}

int
pds_impl_db_device_set (const u8 *mac, const u8 *ip, u8 ip4,
                        u8 overlay_routing_en, u8 symmetric_routing_en,
                        u16 mapping_prio)
{
    pds_impl_db_device_entry_t *dev = &impl_db_ctx.device;

    dev->overlay_routing_en = overlay_routing_en;
    dev->symmetric_routing_en = symmetric_routing_en;
    dev->mapping_prio = mapping_prio;
    clib_memcpy(dev->device_mac, mac, ETH_ADDR_LEN);
    if (ip4) {
        ip46_address_set_ip4(&dev->device_ip, (ip4_address_t *) ip);
    } else {
        ip46_address_set_ip6(&dev->device_ip, (ip6_address_t *) ip);
    }
    return 0;
}

pds_impl_db_device_entry_t *
pds_impl_db_device_get (void)
{
    return &impl_db_ctx.device;
}

int
pds_impl_db_device_del (void)
{
    clib_memset(&impl_db_ctx.device, 0, sizeof(pds_impl_db_device_entry_t));
    return 0;
}

int
pds_impl_db_init (void)
{
    clib_memset(&impl_db_ctx, 0, sizeof(impl_db_ctx));
    pds_impl_db_vnic_init();
    pds_impl_db_subnet_init();
    pds_impl_db_vpc_init();

    return 0;
}
