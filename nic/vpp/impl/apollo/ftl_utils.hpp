//
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_IMPL_APOLLO_FTL_UTILS_H__
#define __VPP_IMPL_APOLLO_FTL_UTILS_H__

void
ftlv4_set_nexthop(ipv4_flow_hash_entry_t *entry,
                  uint32_t nexthop,
                  uint32_t nexthop_type,
                  uint8_t nexthop_valid,
                  uint8_t priority)
{
    return;
}

void
ftl_set_nexthop(flow_hash_entry_t *entry,
                uint32_t nexthop,
                uint32_t nexthop_type,
                uint8_t nexthop_valid,
                uint8_t priority)
{
    return;
}

void
ftlv4_set_lookup_id(ipv4_flow_hash_entry_t *entry, 
                    uint16_t lookup_id)
{
    entry->set_key_metadata_lkp_id(lookup_id);
}

void
ftl_set_lookup_id(flow_hash_entry_t *entry, 
                  uint16_t lookup_id)
{
    entry->set_key_metadata_lkp_id(lookup_id);
}

void
ftlv4_set_flow_miss_hit(ipv4_flow_hash_entry_t *entry, uint8_t val)
{
    return;
}

void
ftl_set_flow_miss_hit(flow_hash_entry_t *entry, uint8_t val)
{
    return;
}

void
ftlv4_set_l2l(ipv4_flow_hash_entry_t *entry, uint8_t val)
{
    return;
}

void
ftl_set_l2l(flow_hash_entry_t *entry, uint8_t val)
{
    return;
}

uint16_t
ftlv4_get_lookup_id(ipv4_flow_hash_entry_t *entry)
{
    return entry->get_key_metadata_lkp_id();
}

uint16_t
ftl_get_lookup_id(flow_hash_entry_t *entry)
{
    return entry->get_key_metadata_lkp_id();
}

uint8_t
ftlv4_get_epoch(ipv4_flow_hash_entry_t *entry)
{
    return 0;
}

uint8_t
ftl_get_epoch(flow_hash_entry_t *entry)
{
    return 0;
}

uint8_t
ftlv4_get_nexthop_valid(ipv4_flow_hash_entry_t *entry)
{
    return 0;
}

uint8_t
ftl_get_nexthop_valid(flow_hash_entry_t *entry)
{
    return 0;
}

uint8_t
ftlv4_get_nexthop_type(ipv4_flow_hash_entry_t *entry)
{
    return 0;
}

uint8_t
ftl_get_nexthop_type(flow_hash_entry_t *entry)
{
    return 0;
}

uint16_t
ftlv4_get_nexthop_id(ipv4_flow_hash_entry_t *entry)
{
    return 0;
}

uint16_t
ftl_get_nexthop_id(flow_hash_entry_t *entry)
{
    return 0;
}

uint8_t
ftlv4_get_nexthop_priority(ipv4_flow_hash_entry_t *entry)
{
    return 0;
}

uint8_t
ftl_get_nexthop_priority(flow_hash_entry_t *entry)
{
    return 0;
}

uint8_t
ftlv4_get_l2l(ipv4_flow_hash_entry_t *entry)
{
    return 0;
}

uint8_t
ftl_get_l2l(flow_hash_entry_t *entry)
{
    return 0;
}

#endif  // __VPP_IMPL_APOLLO_FTL_UTILS_H__
