//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//------------------------------------------------------------------------------
#include "nic/sdk/include/sdk/eth.hpp"
#include "nic/sdk/include/sdk/ip.hpp"
#include "ftltest_utils.hpp"

void
fill_key (uint32_t index, pds_flow_key_t *key)
{
    uint8_t src_ip[IP6_ADDR8_LEN] = {0};
    uint8_t dst_ip[IP6_ADDR8_LEN] = {0};

    uint8_t src_ip_var;
    uint8_t dst_ip_var;
    uint8_t src_ip_0;
    uint8_t dst_ip_0;


    memset(key, 0, sizeof(pds_flow_key_t));
    key->ip_proto = IP_PROTO_UDP;
    key->vnic_id = ((index >> 13) % 256) + 2;
    src_ip_var = (index % 2) + 1;
    dst_ip_var = (index >> 1) % 2 + 33;
    key->l4.tcp_udp.sport = ((index >> 2) % 256) + 100;
    key->l4.tcp_udp.dport = ((index >> 10) % 8) + 201;
    //key->l4.tcp_udp.dport = ((index >> 2) % 256) + 201;
    //key->l4.tcp_udp.sport = ((index >> 10) % 8) + 100;

    key->key_type = KEY_TYPE_IPV4;
    src_ip[0] = src_ip_var;
    src_ip[1] = 0x0;
    src_ip[2] = 0x0;
    src_ip[3] = 0xb;
    memcpy(key->ip_saddr, src_ip, IP6_ADDR8_LEN);

    dst_ip[0] = dst_ip_var;
    dst_ip[1] = 0x0;
    dst_ip[2] = 0x0;
    dst_ip[3] = 0xa;
    memcpy(key->ip_daddr, dst_ip, IP6_ADDR8_LEN);

    return;
}

void
fill_data (uint32_t index, pds_flow_spec_index_type_t index_type,
           pds_flow_data_t *data)
{
    memset(data, 0, sizeof(pds_flow_data_t));
    data->index = index;
    data->index_type = index_type;
    return;
}

void
dump_flow (pds_flow_iter_cb_arg_t *iter_cb_arg)
{
    pds_flow_key_t *key = &iter_cb_arg->flow_key;
    pds_flow_data_t *data = &iter_cb_arg->flow_appdata;

    printf("SrcIP:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x "
           "DstIP:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x "
           "Dport:%u Sport:%u Proto:%u "
           "Ktype:%u VNICID:%u "
           "index:%u index_type:%u\n\n",
           key->ip_saddr[0], key->ip_saddr[1], key->ip_saddr[2], key->ip_saddr[3],
           key->ip_saddr[4], key->ip_saddr[5], key->ip_saddr[6], key->ip_saddr[7],
           key->ip_saddr[8], key->ip_saddr[9], key->ip_saddr[10], key->ip_saddr[11],
           key->ip_saddr[12], key->ip_saddr[13], key->ip_saddr[14], key->ip_saddr[15],
           key->ip_daddr[0], key->ip_daddr[1], key->ip_daddr[2], key->ip_daddr[3],
           key->ip_daddr[4], key->ip_daddr[5], key->ip_daddr[6], key->ip_daddr[7],
           key->ip_daddr[8], key->ip_daddr[9], key->ip_daddr[10], key->ip_daddr[11],
           key->ip_daddr[12], key->ip_daddr[13], key->ip_daddr[14], key->ip_daddr[15],
           key->l4.tcp_udp.dport, key->l4.tcp_udp.sport,
           key->ip_proto, (uint8_t)key->key_type, key->vnic_id,
           data->index, (uint8_t)data->index_type);
    return;
}

void
dump_stats (pds_flow_stats_t *stats)
{
    printf("\nPrinting Flow cache statistics\n");
    printf("Insert %lu, Insert_fail_dupl %lu, Insert_fail %lu, "
           "Insert_fail_recirc %lu\n"
           "Remove %lu, Remove_not_found %lu, Remove_fail %lu\n"
           "Update %lu, Update_fail %lu\n"
           "Get %lu, Get_fail %lu\n"
           "Reserve %lu, reserve_fail %lu\n"
           "Release %lu, Release_fail %lu\n"
           "Tbl_entries %lu, Tbl_collision %lu\n"
           "Tbl_insert %lu, Tbl_remove %lu, Tbl_read %lu, Tbl_write %lu\n",
           stats->api_insert,
           stats->api_insert_duplicate,
           stats->api_insert_fail,
           stats->api_insert_recirc_fail,
           stats->api_remove,
           stats->api_remove_not_found,
           stats->api_remove_fail,
           stats->api_update,
           stats->api_update_fail,
           stats->api_get,
           stats->api_get_fail,
           stats->api_reserve,
           stats->api_reserve_fail,
           stats->api_release,
           stats->api_release_fail,
           stats->table_entries, stats->table_collisions,
           stats->table_insert, stats->table_remove,
           stats->table_read, stats->table_write);
    for (int i= 0; i < PDS_FLOW_TABLE_MAX_RECIRC; i++) {
         printf("Tbl_lvl %u, Tbl_insert %lu, Tbl_remove %lu\n",
                 i, stats->table_insert_lvl[i], stats->table_remove_lvl[i]);
    }
    return;
}
