//------------------------------------------------------------------------------
// Copyright (c) 2019 Pensando Systems, Inc.
//------------------------------------------------------------------------------

#include <iostream>
#include <math.h>
#include "nic/apollo/test/scale/test.hpp"
#include "nic/apollo/test/scale/api.hpp"
#include "nic/apollo/test/utils/base.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_tep.hpp"
#include "nic/apollo/api/include/pds_vpc.hpp"
#include "nic/apollo/api/include/pds_subnet.hpp"
#include "nic/apollo/api/include/pds_vnic.hpp"
#include "nic/apollo/api/include/pds_mapping.hpp"
#include "nic/apollo/api/include/pds_policy.hpp"
#include "nic/apollo/api/include/pds_device.hpp"
#include "boost/foreach.hpp"
#include "boost/optional.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "nic/apollo/test/scale/test_common.hpp"

#define VNID_BASE                      1000

using std::string;

extern char *g_input_cfg_file;
extern std::string g_pipeline;
pds_device_spec_t g_device = {0};
test_params_t g_test_params;

#define PDS_SUBNET_ID(vpc_num, num_subnets_per_vpc, subnet_num)    \
            (((vpc_num) * (num_subnets_per_vpc)) + subnet_num)

//----------------------------------------------------------------------------
// create route tables
//------------------------------------------------------------------------------
sdk_ret_t
create_v6_route_tables (uint32_t num_teps, uint32_t num_vpcs,
                        uint32_t num_subnets, uint32_t num_routes,
                        ip_prefix_t *tep_pfx, ip_prefix_t *route_pfx,
                        ip_prefix_t *v6_route_pfx, uint32_t num_nh)
{
    uint32_t ntables = num_vpcs * num_subnets;
    uint32_t tep_offset = 3;
    uint32_t nh_id = 1;
    uint32_t v6rtnum;
    pds_route_table_spec_t v6route_table;
    sdk_ret_t rv = SDK_RET_OK;

    tep_offset = 3;
    v6route_table.af = IP_AF_IPV6;
    v6route_table.routes =
            (pds_route_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_ROUTE_TABLE,
                                      (num_routes * sizeof(pds_route_t)));
    v6route_table.num_routes = num_routes;
    for (uint32_t i = 1; i <= ntables; i++) {
        v6rtnum = 0;
        v6route_table.key.id = ntables + i;
        for (uint32_t j = 0; j < num_routes; j++) {
            compute_ipv6_prefix(&v6route_table.routes[j].prefix, v6_route_pfx,
                                v6rtnum++, 120);
            if (apollo()) {
                v6route_table.routes[j].nh_ip.af = IP_AF_IPV4;
                v6route_table.routes[j].nh_ip.addr.v4_addr =
                        tep_pfx->addr.addr.v4_addr + tep_offset++;

                tep_offset %= (num_teps + 3);
                if (tep_offset == 0) {
                    // skip MyTEP and gateway IPs
                    tep_offset += 3;
                }
                v6route_table.routes[j].nh_type = PDS_NH_TYPE_TEP;
            } else if (artemis()) {
                v6route_table.routes[j].nh_type = PDS_NH_TYPE_IP;
                v6route_table.routes[j].nh = nh_id++;
                if (nh_id > num_nh) {
                    nh_id = 1;
                }
            }
        }

        rv = create_route_table(&v6route_table);
        SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                "create route table %u failed, ret %u",
                                v6route_table.key.id, rv);
    }

    return rv;
}

sdk_ret_t
create_route_tables (uint32_t num_teps, uint32_t num_vpcs, uint32_t num_subnets,
                     uint32_t num_routes, ip_prefix_t *tep_pfx,
                     ip_prefix_t *route_pfx, ip_prefix_t *v6_route_pfx,
                     uint32_t num_nh)
{
    uint32_t ntables = num_vpcs * num_subnets;
    uint32_t tep_offset = 3;
    uint32_t nh_id = 1;
    uint32_t rtnum;
    pds_route_table_spec_t route_table;
    sdk_ret_t rv = SDK_RET_OK;

    route_table.af = IP_AF_IPV4;
    route_table.routes =
        (pds_route_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_ROUTE_TABLE,
                                  (num_routes * sizeof(pds_route_t)));
    route_table.num_routes = num_routes;
    for (uint32_t i = 1; i <= ntables; i++) {
        rtnum = 0;
        route_table.key.id = i;
        for (uint32_t j = 0; j < num_routes; j++) {
            route_table.routes[j].prefix.addr.af = IP_AF_IPV4;
            if (apollo()) {
                route_table.routes[j].prefix.len = 24;
                route_table.routes[j].prefix.addr.addr.v4_addr =
                        ((0xC << 28) | (rtnum++ << 8));
                route_table.routes[j].nh_type = PDS_NH_TYPE_TEP;
                route_table.routes[j].nh_ip.af = IP_AF_IPV4;
                route_table.routes[j].nh_ip.addr.v4_addr =
                    tep_pfx->addr.addr.v4_addr + tep_offset++;
                tep_offset %= (num_teps + 3);
                if (tep_offset == 0) {
                    // skip MyTEP and gateway IPs
                    tep_offset += 3;
                }
            } else if (artemis()) {
                route_table.routes[j].prefix.len = TESTAPP_V4ROUTE_PREFIX_LEN;
                route_table.routes[j].prefix.addr.addr.v4_addr =
                                    TESTAPP_V4ROUTE_PREFIX_VAL(rtnum);
                rtnum++;
                route_table.routes[j].nh_type = PDS_NH_TYPE_IP;
                route_table.routes[j].nh = nh_id++;
                if (nh_id > num_nh) {
                    nh_id = 1;
                }
            }
        }
        rv = create_route_table(&route_table);
        SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                "create route table %u failed, ret %u",
                                route_table.key.id, rv);
    }

    if (g_test_params.dual_stack) {
        rv = create_v6_route_tables(num_teps, num_vpcs, num_subnets, num_routes,
                                    tep_pfx, route_pfx, v6_route_pfx, num_nh);
    }
    return rv;
}

sdk_ret_t
create_svc_mappings (uint32_t num_vpcs, uint32_t num_subnets,
                     uint32_t num_vnics, uint32_t num_ip_per_vnic,
                     ip_prefix_t *v4_vip_pfx, ip_prefix_t *v6_vip_pfx,
                     ip_prefix_t *v4_provider_pfx,
                     ip_prefix_t *v6_provider_pfx)
{
    sdk_ret_t rv;
    uint32_t ip_offset = 0;
    pds_svc_mapping_spec_t svc_mapping;
    pds_svc_mapping_spec_t svc_v6_mapping;

    // create local vnic IP mappings first
    for (uint32_t i = 1; i <= num_vpcs; i++) {
        for (uint32_t j = 1; j <= num_subnets; j++) {
            for (uint32_t k = 1; k <= num_vnics; k++) {
                for (uint32_t l = 1; l <= num_ip_per_vnic; l++) {
                    // VIP is always in public/substrate/infra vpc
                    svc_mapping.key.vpc.id = num_vpcs + 1;
                    svc_mapping.key.vip.af = IP_AF_IPV4;
                    svc_mapping.key.vip.addr.v4_addr =
                        v4_vip_pfx->addr.addr.v4_addr + ip_offset;
                    svc_mapping.key.svc_port = TEST_APP_VIP_PORT;
                    svc_mapping.vpc.id = i;
                    svc_mapping.backend_ip.af = IP_AF_IPV4;
                    svc_mapping.backend_ip.addr.v4_addr =
                        (g_test_params.vpc_pfx.addr.addr.v4_addr | ((j - 1) << 14)) |
                            (((k - 1) * num_ip_per_vnic) + l);
                    svc_mapping.svc_port = TEST_APP_DIP_PORT;
                    svc_mapping.backend_provider_ip.af = IP_AF_IPV4;
                    svc_mapping.backend_provider_ip.addr.v4_addr =
                        v4_provider_pfx->addr.addr.v4_addr + ip_offset;
                    ip_offset++;
                    rv = create_svc_mapping(&svc_mapping);
                    SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                            "create v4 svc mapping failed, vpc %u, rv %u",
                                            i, rv);
                    if (g_test_params.dual_stack) {
                        svc_v6_mapping = svc_mapping;
                        svc_v6_mapping.key.vip.af = IP_AF_IPV6;
                        svc_v6_mapping.key.vip.addr.v6_addr =
                            v6_vip_pfx->addr.addr.v6_addr;
                        CONVERT_TO_V4_MAPPED_V6_ADDRESS(svc_v6_mapping.key.vip.addr.v6_addr,
                                                       svc_mapping.key.vip.addr.v4_addr);
                        svc_v6_mapping.backend_ip.af = IP_AF_IPV6;
                        svc_v6_mapping.backend_ip.addr.v6_addr =
                            g_test_params.v6_vpc_pfx.addr.addr.v6_addr;
                        CONVERT_TO_V4_MAPPED_V6_ADDRESS(svc_v6_mapping.backend_ip.addr.v6_addr,
                                                        svc_mapping.backend_ip.addr.v4_addr);
                        // TODO: fix the provider to be IPv4, not v6
                        svc_v6_mapping.backend_provider_ip.af = IP_AF_IPV6;
                        svc_v6_mapping.backend_provider_ip.addr.v6_addr =
                            v6_provider_pfx->addr.addr.v6_addr;
                        CONVERT_TO_V4_MAPPED_V6_ADDRESS(svc_v6_mapping.backend_provider_ip.addr.v6_addr,
                                                        svc_mapping.backend_provider_ip.addr.v4_addr);
                        rv = create_svc_mapping(&svc_v6_mapping);
                        SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                                "create v6 svc mapping failed, vpc %u, rv %u",
                                                i, rv);
                    }
                }
            }
        }
    }
    // push leftover objects
    rv = create_svc_mapping(NULL);
    SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                            "create svc mapping failed, rv %u", rv);
    return rv;
}

//----------------------------------------------------------------------------
// 1. create 1 primary + 32 secondary IP for each of 1K local vnics
// 2. create 1023 remote mappings per VPC
//------------------------------------------------------------------------------
sdk_ret_t
create_mappings (uint32_t num_teps, uint32_t num_vpcs, uint32_t num_subnets,
                 uint32_t num_vnics, uint32_t num_ip_per_vnic,
                 ip_prefix_t *teppfx, ip_prefix_t *natpfx,
                 ip_prefix_t *v6_natpfx,
                 uint32_t num_remote_mappings,
                 ip_prefix_t *provider_pfx,
                 ip_prefix_t *v6_provider_pfx)
{
    sdk_ret_t rv;
    pds_local_mapping_spec_t pds_local_mapping;
    pds_local_mapping_spec_t pds_local_v6_mapping;
    pds_remote_mapping_spec_t pds_remote_mapping;
    pds_remote_mapping_spec_t pds_remote_v6_mapping;
    uint16_t vnic_key = 1, ip_base;
    uint32_t ip_offset = 0, remote_slot = 1025;
    uint32_t tep_offset = 0, v6_tep_offset = 0;
    static uint32_t svc_tag = 1;

    // ensure a max. of 32 IPs per VNIC
    SDK_ASSERT(num_vpcs * num_subnets * num_vnics * num_ip_per_vnic <=
               (32 * PDS_MAX_VNIC));
    // create local vnic IP mappings first
    for (uint32_t i = 1; i <= num_vpcs; i++) {
        for (uint32_t j = 1; j <= num_subnets; j++) {
            for (uint32_t k = 1; k <= num_vnics; k++) {
                for (uint32_t l = 1; l <= num_ip_per_vnic; l++) {
                    memset(&pds_local_mapping, 0, sizeof(pds_local_mapping));
                    pds_local_mapping.key.vpc.id = i;
                    pds_local_mapping.key.ip_addr.af = IP_AF_IPV4;
                    pds_local_mapping.key.ip_addr.addr.v4_addr =
                        (g_test_params.vpc_pfx.addr.addr.v4_addr | ((j - 1) << 14)) |
                        (((k - 1) * num_ip_per_vnic) + l);
                    pds_local_mapping.subnet.id =
                        PDS_SUBNET_ID((i - 1), num_subnets, j);
                    if (g_test_params.fabric_encap.type ==
                            PDS_ENCAP_TYPE_VXLAN) {
                        pds_local_mapping.fabric_encap.type =
                            PDS_ENCAP_TYPE_VXLAN;
                        //pds_mapping.fabric_encap.val.vnid = VNID_BASE + pds_mapping.subnet.id;
                        pds_local_mapping.fabric_encap.val.vnid = vnic_key;
                    } else {
                        pds_local_mapping.fabric_encap.type =
                            PDS_ENCAP_TYPE_MPLSoUDP;
                        pds_local_mapping.fabric_encap.val.mpls_tag = vnic_key;
                    }
                    MAC_UINT64_TO_ADDR(pds_local_mapping.vnic_mac,
                                       (((((uint64_t)i & 0x7FF) << 22) |
                                         ((j & 0x7FF) << 11) | (k & 0x7FF))));
                    pds_local_mapping.vnic.id = vnic_key;
                    if (natpfx) {
                        pds_local_mapping.public_ip_valid = true;
                        pds_local_mapping.public_ip.af = IP_AF_IPV4;
                        pds_local_mapping.public_ip.addr.v4_addr =
                            natpfx->addr.addr.v4_addr + ip_offset;
                    }
                    if (artemis() && i == TEST_APP_S3_VNET_IN_OUT_V6_OUTER) {
                        pds_local_mapping.provider_ip_valid = true;
                        pds_local_mapping.provider_ip = v6_provider_pfx->addr;
                        CONVERT_TO_V4_MAPPED_V6_ADDRESS(pds_local_mapping.provider_ip.addr.v6_addr,
                                                        (provider_pfx->addr.addr.v4_addr + ip_offset));
                    } else {
                        pds_local_mapping.provider_ip_valid = true;
                        pds_local_mapping.provider_ip.af = IP_AF_IPV4;
                        pds_local_mapping.provider_ip.addr.v4_addr =
                                        provider_pfx->addr.addr.v4_addr + ip_offset;
                        pds_local_mapping.svc_tag = svc_tag++;
                    }
                    ip_offset++;

                    rv = create_local_mapping(&pds_local_mapping);
                    SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                            "create v4 local mapping failed, vpc %u, ret %u",
                                            i, rv);

                    if (g_test_params.dual_stack) {
                        // V6 mapping
                        pds_local_v6_mapping = pds_local_mapping;
                        pds_local_v6_mapping.key.ip_addr.af = IP_AF_IPV6;
                        pds_local_v6_mapping.key.ip_addr.addr.v6_addr =
                               g_test_params.v6_vpc_pfx.addr.addr.v6_addr;
                        CONVERT_TO_V4_MAPPED_V6_ADDRESS(pds_local_v6_mapping.key.ip_addr.addr.v6_addr,
                                                        pds_local_mapping.key.ip_addr.addr.v4_addr);
                        // no need of v6 to v6 NAT
                        pds_local_v6_mapping.public_ip_valid = false;
                        if (natpfx) {
                            pds_local_v6_mapping.public_ip.af = IP_AF_IPV6;
                            pds_local_v6_mapping.public_ip.addr.v6_addr = v6_natpfx->addr.addr.v6_addr;
                            CONVERT_TO_V4_MAPPED_V6_ADDRESS(pds_local_v6_mapping.public_ip.addr.v6_addr,
                                                            pds_local_mapping.public_ip.addr.v4_addr);
                        }
                        if (artemis() && i == TEST_APP_S3_VNET_IN_OUT_V6_OUTER) {
                            pds_local_v6_mapping.provider_ip_valid = true;
                            pds_local_v6_mapping.provider_ip = v6_provider_pfx->addr;
                            CONVERT_TO_V4_MAPPED_V6_ADDRESS(pds_local_v6_mapping.provider_ip.addr.v6_addr,
                                                            (provider_pfx->addr.addr.v4_addr + ip_offset));
                        } else {
                            pds_local_v6_mapping.provider_ip_valid = true;
                            pds_local_v6_mapping.provider_ip.af = IP_AF_IPV4;
                            pds_local_v6_mapping.provider_ip.addr.v4_addr =
                                provider_pfx->addr.addr.v4_addr + ip_offset;
                        }
                        ip_offset++;

                        rv = create_local_mapping(&pds_local_v6_mapping);
                        SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                                "create v4 local mapping failed, vpc %u, ret %u",
                                                i, rv);
                    }
                }
                vnic_key++;
            }
        }
    }
    // Batching: push leftover objects
    rv = create_local_mapping(NULL);
    SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                            "create v4 local mapping failed, ret %u", rv);

    // create remote mappings
    SDK_ASSERT(num_vpcs * num_remote_mappings <= (1 << 20));
    for (uint32_t i = 1; i <= num_vpcs; i++) {
        tep_offset = 3;
        v6_tep_offset = tep_offset + num_teps / 2;
        for (uint32_t j = 1; j <= num_subnets; j++) {
            ip_base = num_vnics * num_ip_per_vnic + 1;
            for (uint32_t k = 1; k <= num_remote_mappings; k++) {
                memset(&pds_remote_mapping, 0, sizeof(pds_remote_mapping));
                pds_remote_mapping.key.vpc.id = i;
                pds_remote_mapping.key.ip_addr.af = IP_AF_IPV4;
                pds_remote_mapping.key.ip_addr.addr.v4_addr =
                    (g_test_params.vpc_pfx.addr.addr.v4_addr | ((j - 1) << 14)) |
                    ip_base++;
                pds_remote_mapping.subnet.id =
                    PDS_SUBNET_ID((i - 1), num_subnets, j);
                if (g_test_params.fabric_encap.type == PDS_ENCAP_TYPE_VXLAN) {
                    pds_remote_mapping.fabric_encap.type = PDS_ENCAP_TYPE_VXLAN;
                    //pds_mapping.fabric_encap.val.vnid =
                        //VNID_BASE + pds_mapping.subnet.id;
                    pds_remote_mapping.fabric_encap.val.vnid = remote_slot++;
                } else {
                    pds_remote_mapping.fabric_encap.type =
                        PDS_ENCAP_TYPE_MPLSoUDP;
                    pds_remote_mapping.fabric_encap.val.mpls_tag =
                        remote_slot++;
                }
                pds_remote_mapping.tep.ip_addr.af = IP_AF_IPV4;
                pds_remote_mapping.tep.ip_addr.addr.v4_addr =
                    teppfx->addr.addr.v4_addr + tep_offset;
                MAC_UINT64_TO_ADDR(
                    pds_remote_mapping.vnic_mac,
                    (((((uint64_t)i & 0x7FF) << 22) | ((j & 0x7FF) << 11) |
                      ((num_vnics + k) & 0x7FF))));
                if (artemis() && i == TEST_APP_S3_VNET_IN_OUT_V6_OUTER) {
                    pds_remote_mapping.provider_ip_valid = true;
                    pds_remote_mapping.provider_ip = v6_provider_pfx->addr;
                    CONVERT_TO_V4_MAPPED_V6_ADDRESS(pds_remote_mapping.provider_ip.addr.v6_addr,
                                                    (provider_pfx->addr.addr.v4_addr + ip_offset));
                } else {
                    pds_remote_mapping.provider_ip_valid = true;
                    pds_remote_mapping.provider_ip.af = IP_AF_IPV4;
                    pds_remote_mapping.provider_ip.addr.v4_addr =
                                    provider_pfx->addr.addr.v4_addr + ip_offset;
                }
                ip_offset++;

                rv = create_remote_mapping(&pds_remote_mapping);
                SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                        "create v4 remote mapping failed, vpc %u, ret %u",
                                        i, rv);

                if (g_test_params.dual_stack) {
                    // V6 mapping
                    pds_remote_v6_mapping = pds_remote_mapping;
                    pds_remote_v6_mapping.key.ip_addr.af = IP_AF_IPV6;
                    pds_remote_v6_mapping.key.ip_addr.addr.v6_addr =
                          g_test_params.v6_vpc_pfx.addr.addr.v6_addr;
                    CONVERT_TO_V4_MAPPED_V6_ADDRESS(pds_remote_v6_mapping.key.ip_addr.addr.v6_addr,
                                                    pds_remote_mapping.key.ip_addr.addr.v4_addr);
                    pds_remote_v6_mapping.tep.ip_addr.af = IP_AF_IPV4;
                    pds_remote_v6_mapping.tep.ip_addr.addr.v4_addr =
                        teppfx->addr.addr.v4_addr + v6_tep_offset;
                    if (artemis() && i == TEST_APP_S3_VNET_IN_OUT_V6_OUTER) {
                        pds_remote_v6_mapping.provider_ip_valid = true;
                        pds_remote_v6_mapping.provider_ip = v6_provider_pfx->addr;
                        CONVERT_TO_V4_MAPPED_V6_ADDRESS(pds_remote_v6_mapping.provider_ip.addr.v6_addr,
                                                        (provider_pfx->addr.addr.v4_addr + ip_offset));
                    } else {
                        pds_remote_v6_mapping.provider_ip_valid = true;
                        pds_remote_v6_mapping.provider_ip.af = IP_AF_IPV4;
                        pds_remote_v6_mapping.provider_ip.addr.v4_addr =
                            provider_pfx->addr.addr.v4_addr + ip_offset;
                    }
                    ip_offset++;

                    rv = create_remote_mapping(&pds_remote_v6_mapping);
                    SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                            "create v6 remote mapping failed, vpc %u, ret %u",
                                            i, rv);
                }
                tep_offset++;
                tep_offset %= num_teps;
                tep_offset = tep_offset ? tep_offset : 3;
                v6_tep_offset++;
                v6_tep_offset %= num_teps;
                v6_tep_offset = v6_tep_offset ? v6_tep_offset : 3;
            }
        }
    }
    // Batching: push leftover objects
    rv = create_remote_mapping(NULL);
    SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                            "create v6 remote mapping failed, ret %u", rv);
    return SDK_RET_OK;
}

sdk_ret_t
create_vnics (uint32_t num_vpcs, uint32_t num_subnets,
              uint32_t num_vnics, uint16_t vlan_start,
              uint32_t num_meter)
{
    sdk_ret_t rv = SDK_RET_OK;
    pds_vnic_spec_t pds_vnic;
    uint16_t vnic_key = 1;
    static pds_meter_id_t v4_meter_id = 1;
    static pds_meter_id_t v6_meter_id = num_meter+1;

    SDK_ASSERT(num_vpcs * num_subnets * num_vnics <= PDS_MAX_VNIC);
    for (uint32_t i = 1; i <= (uint64_t)num_vpcs; i++) {
        for (uint32_t j = 1; j <= num_subnets; j++) {
            for (uint32_t k = 1; k <= num_vnics; k++) {
                memset(&pds_vnic, 0, sizeof(pds_vnic));
                pds_vnic.vpc.id = i;
                pds_vnic.subnet.id = PDS_SUBNET_ID((i - 1), num_subnets, j);
                pds_vnic.key.id = vnic_key;
                if (g_test_params.tag_vnics) {
                    pds_vnic.vnic_encap.type = PDS_ENCAP_TYPE_DOT1Q;
                    pds_vnic.vnic_encap.val.vlan_tag =
                        vlan_start + vnic_key - 1;
                } else {
                    pds_vnic.vnic_encap.type = PDS_ENCAP_TYPE_NONE;
                    pds_vnic.vnic_encap.val.value = 0;
                }
                if (g_test_params.fabric_encap.type == PDS_ENCAP_TYPE_VXLAN) {
                    pds_vnic.fabric_encap.type = PDS_ENCAP_TYPE_VXLAN;
                    //pds_vnic.fabric_encap.val.vnid =
                        //VNID_BASE + pds_vnic.subnet.id;
                    pds_vnic.fabric_encap.val.vnid = vnic_key;
                } else {
                    pds_vnic.fabric_encap.type = PDS_ENCAP_TYPE_MPLSoUDP;
                    pds_vnic.fabric_encap.val.mpls_tag = vnic_key;
                }
                // MAC ADDR bits based as below
                // vnic:   bits  0 - 10
                // subnet: bits 11 - 21
                // vpc:    bits 22 - 32
                MAC_UINT64_TO_ADDR(pds_vnic.mac_addr,
                                   (((((uint64_t)i & 0x7FF) << 22) |
                                     ((j & 0x7FF) << 11) | (k & 0x7FF))));
#if 0
                // Provider MAC ADDR bits based as below
                // vnic:   bits  0 - 10
                // subnet: bits 11 - 21
                // vpc:    bits 22 - 32
                // 1:      bit 33
                MAC_UINT64_TO_ADDR(pds_vnic.provider_mac_addr,
                                   ((((uint64_t)1 << 33) |
                                     (((uint64_t)i & 0x7FF) << 22) |
                                     ((j & 0x7FF) << 11) | (k & 0x7FF))));
#endif
                pds_vnic.rsc_pool_id = 1;
                pds_vnic.src_dst_check = false; //(k & 0x1);
                pds_vnic.tx_mirror_session_bmap =
                    g_test_params.rspan_bmap | g_test_params.erspan_bmap;
                pds_vnic.rx_mirror_session_bmap =
                    g_test_params.rspan_bmap | g_test_params.erspan_bmap;
                pds_vnic.v4_meter.id = v4_meter_id++;
                if (v4_meter_id > num_meter) {
                    v4_meter_id = 1;
                }
                pds_vnic.v6_meter.id = v6_meter_id++;
                if (v6_meter_id > (2 * num_meter)) {
                    v6_meter_id = num_meter+1;
                }
                rv = create_vnic(&pds_vnic);
                SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                        "create vnic failed, vpc %u, ret %u",
                                        i, rv);
                vnic_key++;
            }
        }
    }

    if (artemis()) {
        // create a bridge vnic in the infra/provider/public vrf
        memset(&pds_vnic, 0, sizeof(pds_vnic));
        pds_vnic.vpc.id = num_vpcs + 1;
        pds_vnic.subnet.id = PDS_SUBNET_ID(((num_vpcs + 1) - 1), 1, 1);
        pds_vnic.key.id = vnic_key;
        pds_vnic.vnic_encap.type = PDS_ENCAP_TYPE_NONE;
        pds_vnic.vnic_encap.val.value = 0;
        pds_vnic.fabric_encap.type = PDS_ENCAP_TYPE_DOT1Q;
        pds_vnic.fabric_encap.val.value = TESTAPP_SWITCH_VNIC_VLAN;
        MAC_UINT64_TO_ADDR(pds_vnic.mac_addr,
                           (((((uint64_t)pds_vnic.vpc.id & 0x7FF) << 22) |
                             ((1 & 0x7FF) << 11) | (1 & 0x7FF))));
        pds_vnic.switch_vnic = true;
        rv = create_vnic(&pds_vnic);
        SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                "create vnic failed, vpc %u, ret %u",
                                num_vpcs + 1, rv);
    }

    // Batching: push leftover objects
    rv = create_vnic(NULL);
    SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                            "create vnic failed, ret %u", rv);
    return rv;
}

// VPC prefix is /8, subnet id is encoded in next 10 bits (making it /18 prefix)
// leaving LSB 14 bits for VNIC IPs
sdk_ret_t
create_subnets (uint32_t vpc_id, uint32_t num_vpcs,
                uint32_t num_subnets, ipv4_prefix_t *vpc_pfx,
                ip_prefix_t *v6_vpc_pfx)
{
    sdk_ret_t rv;
    pds_subnet_spec_t pds_subnet;
    static uint32_t route_table_id = 1;
    static uint32_t policy_id = 1;

    for (uint32_t i = 1; i <= num_subnets; i++) {
        memset(&pds_subnet, 0, sizeof(pds_subnet));
        pds_subnet.key.id = PDS_SUBNET_ID((vpc_id - 1), num_subnets, i);
        pds_subnet.vpc.id = vpc_id;
        pds_subnet.v4_prefix = *vpc_pfx;
        pds_subnet.v4_prefix.v4_addr =
            (pds_subnet.v4_prefix.v4_addr) | ((i - 1) << 14);
        pds_subnet.v4_prefix.len = 18;
        pds_subnet.v4_vr_ip = pds_subnet.v4_prefix.v4_addr;
        pds_subnet.v6_prefix = *v6_vpc_pfx;
        pds_subnet.v6_prefix.addr.addr.v6_addr.addr32[3] =
            (pds_subnet.v6_prefix.addr.addr.v6_addr.addr32[3]) | ((i - 1) << 14);
        pds_subnet.v6_prefix.len = 96;
        pds_subnet.v6_vr_ip = pds_subnet.v6_prefix.addr;
        MAC_UINT64_TO_ADDR(pds_subnet.vr_mac,
                           (uint64_t)pds_subnet.v4_vr_ip);
        pds_subnet.v6_route_table.id =
            route_table_id + (num_subnets * num_vpcs);
        pds_subnet.v4_route_table.id = route_table_id++;
        pds_subnet.egr_v4_policy.id = policy_id;
        pds_subnet.ing_v4_policy.id = policy_id + (num_subnets * num_vpcs);
        pds_subnet.egr_v6_policy.id = policy_id + (num_subnets * num_vpcs) * 2;
        pds_subnet.ing_v6_policy.id = policy_id + (num_subnets * num_vpcs) * 3;
        policy_id++;
        rv = create_subnet(&pds_subnet);
        SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                "create subnet %u failed, ret %u", i, rv);
    }
    return SDK_RET_OK;
}

sdk_ret_t
create_vpcs (uint32_t num_vpcs, ip_prefix_t *ipv4_prefix,
             ip_prefix_t *ipv6_prefix, ip_prefix_t *nat46_pfx,
             uint32_t num_subnets)
{
    sdk_ret_t rv;
    pds_vpc_spec_t pds_vpc;
    pds_subnet_spec_t pds_subnet;
    uint64_t substrate_vr_mac = 0x0000DEADBADEUL;

    SDK_ASSERT(num_vpcs <= PDS_MAX_VPC);
    for (uint32_t i = 1; i <= num_vpcs; i++) {
        memset(&pds_vpc, 0, sizeof(pds_vpc));
        pds_vpc.type = PDS_VPC_TYPE_TENANT;
        pds_vpc.key.id = i;
        pds_vpc.v4_prefix.v4_addr = ipv4_prefix->addr.addr.v4_addr & 0xFF000000;
        pds_vpc.v4_prefix.len = 8; // fix this to /8
        pds_vpc.v6_prefix = *ipv6_prefix;
        pds_vpc.fabric_encap.type = PDS_ENCAP_TYPE_VXLAN;
        pds_vpc.fabric_encap.val.vnid = i;
        pds_vpc.nat46_prefix = *nat46_pfx;
        rv = create_vpc(&pds_vpc);
        SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                "create vpc %u failed, rv %u", i, rv);
        rv = create_subnets(i, num_vpcs, num_subnets, &pds_vpc.v4_prefix,
                            &pds_vpc.v6_prefix);
        if (rv != SDK_RET_OK) {
            return rv;
        }
    }

    if (artemis()) {
        // create infra/substrate/provider VPC
        memset(&pds_vpc, 0, sizeof(pds_vpc));
        pds_vpc.type = PDS_VPC_TYPE_SUBSTRATE;
        pds_vpc.key.id = num_vpcs + 1;
        pds_vpc.fabric_encap.type = PDS_ENCAP_TYPE_VXLAN;
        pds_vpc.fabric_encap.val.vnid = TESTAPP_SUBSTRATE_VNID;
        rv = create_vpc(&pds_vpc);
        SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                "create vpc %u failed, rv %u", num_vpcs + 1, rv);
        // create a subnet under infra/substrate/provider VPC
        memset(&pds_subnet, 0, sizeof(pds_subnet));
        pds_subnet.key.id = PDS_SUBNET_ID(((num_vpcs + 1)- 1), 1, 1);
        pds_subnet.v4_vr_ip = g_test_params.device_ip;
        pds_subnet.vpc.id = num_vpcs + 1;
        MAC_UINT64_TO_ADDR(pds_subnet.vr_mac, substrate_vr_mac);
        rv = create_subnet(&pds_subnet);
        SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                "create subnet %u failed, ret %u",
                                pds_subnet.key.id, rv);
    }

    // Batching: push leftover vpc and subnet objects
    rv = create_vpc(NULL);
    SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                            "create vpc failed, rv %u", rv);
    rv = create_subnet(NULL);
    SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                            "create subnet failed, ret %u", rv);
    return SDK_RET_OK;
}

sdk_ret_t
create_vpc_peers (uint32_t num_vpcs)
{
    sdk_ret_t rv;
    pds_vpc_peer_spec_t pds_vpc_peer;
    uint32_t vpc_peer_id = 1;

    if (num_vpcs <= 1) {
        return SDK_RET_OK;
    } else if (num_vpcs & 0x1) {
        // last VPC will be left out w.r.t peering if we have odd number of VPCs
        num_vpcs -= 1;
    }

    SDK_ASSERT(num_vpcs <= PDS_MAX_VPC);
    for (uint32_t i = 1; i <= num_vpcs; i+=2) {
        memset(&pds_vpc_peer, 0, sizeof(pds_vpc_peer));
        pds_vpc_peer.key.id = vpc_peer_id++;
        pds_vpc_peer.vpc1.id = i;
        pds_vpc_peer.vpc2.id = i+1;
        rv = create_vpc_peer(&pds_vpc_peer);
        SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                                "create vpc peer %u failed, ret %u",
                                vpc_peer_id - 1, rv);
    }
    // Batching: push leftover vpc and subnet objects
    rv = create_vpc_peer(NULL);
    SDK_ASSERT_TRACE_RETURN((rv == SDK_RET_OK), rv,
                            "create vpc peer failed, ret %u", rv);
    return SDK_RET_OK;
}

static sdk_ret_t
populate_tags_api_rule_spec (uint32_t id, pds_tag_rule_t *api_rule_spec,
                             uint32_t tag_value, uint32_t num_prefixes,
                             uint32_t *tag_pfx_count, uint8_t ip_af,
                             ip_prefix_t *v6_route_pfx, uint32_t priority)
{
    api_rule_spec->tag = tag_value++;
    api_rule_spec->priority = priority;
    ip_prefix_t *prefixes =
            (ip_prefix_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_METER,
                           (num_prefixes * sizeof(ip_prefix_t)));
    memset(prefixes, 0, num_prefixes * sizeof(ip_prefix_t));
    api_rule_spec->num_prefixes = num_prefixes;
    api_rule_spec->prefixes = prefixes;
    for (uint32_t pfx = 0;
                  pfx < api_rule_spec->num_prefixes; pfx++) {
        prefixes[pfx].addr.af = ip_af;
        if (ip_af == IP_AF_IPV4) {
            prefixes[pfx].len = 24;
            prefixes[pfx].addr.addr.v4_addr =
                          ((0xC << 28) | ((*tag_pfx_count)++ << 8));
        } else {
            compute_ipv6_prefix(&prefixes[pfx], v6_route_pfx,
                                (*tag_pfx_count)++, 120);
        }
    }
    return SDK_RET_OK;
}

#define TESTAPP_TAGS_NUM_PREFIXES 4
#define TESTAPP_TAGS_PRIORITY_STEP 4
sdk_ret_t
create_tags (uint32_t num_tag_trees, uint32_t scale,
             uint8_t ip_af, ip_prefix_t *v6_route_pfx)
{
    sdk_ret_t ret;
    pds_tag_spec_t pds_tag;
    uint32_t num_prefixes = TESTAPP_TAGS_NUM_PREFIXES;
    uint32_t priority = 0;
    uint32_t step = TESTAPP_TAGS_PRIORITY_STEP;
    // unique IDs across tags
    static pds_tag_id_t id = 1;
    static uint32_t tag_pfx_count = 0;
    static uint32_t tag_value = 0;

    if (num_tag_trees == 0) {
        return SDK_RET_OK;
    }

    memset(&pds_tag, 0, sizeof(pds_tag));
    pds_tag.af = ip_af;

    // if scale < TESTAPP_TAGS_NUM_PREFIXES,
    // create num_rules=scale with num_prefixes=1
    if (scale < TESTAPP_TAGS_NUM_PREFIXES) {
        num_prefixes = 1;
    }

    // if the scale is not divisible by num_prefixes, then create
    // remainder rules with num_prefixes=1
    uint32_t num_rules = scale/num_prefixes;
    uint32_t num_rules_rem = scale % num_prefixes;

    pds_tag.num_rules = num_rules + num_rules_rem;

    // allocate (pds_tag.num_rules + num_rules_rem)
    pds_tag.rules =
            (pds_tag_rule_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_TAG,
                        (pds_tag.num_rules * sizeof(pds_tag_rule_t)));
    for (uint32_t i = 0; i < num_tag_trees; i++) {
        pds_tag.key.id = id++;
        priority = 0;
        step = TESTAPP_TAGS_PRIORITY_STEP;
        for (uint32_t rule = 0; rule < num_rules; rule++) {
            pds_tag_rule_t *api_rule_spec = &pds_tag.rules[rule];
            if (step == TESTAPP_TAGS_PRIORITY_STEP) {
                priority = rule + (step - 1);
            }
            populate_tags_api_rule_spec(id, api_rule_spec, tag_value,
                                        num_prefixes, &tag_pfx_count, ip_af,
                                        v6_route_pfx, priority);
            tag_value = (tag_value + 1) % 128;
            priority--;
            step--;
            if (step == 0) {
                step = TESTAPP_TAGS_PRIORITY_STEP;
            }
        }
        for (uint32_t rule = num_rules;
                      rule < (num_rules + num_rules_rem); rule++) {
            pds_tag_rule_t *api_rule_spec = &pds_tag.rules[rule];
            if (step == TESTAPP_TAGS_PRIORITY_STEP) {
                priority = rule + (step - 1);
            }
            populate_tags_api_rule_spec(
                    id, api_rule_spec, tag_value, 1, &tag_pfx_count,
                    ip_af, v6_route_pfx, priority);
            tag_value = (tag_value + 1) % 128;
            priority--;
            priority--;
            step--;
            if (step == 0) {
                step = TESTAPP_TAGS_PRIORITY_STEP;
            }
        }
        ret = create_tag(&pds_tag);
        SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                                "create tag %u failed, ret %u",
                                pds_tag.key.id, ret);
    }
    // Batching: push leftover objects
    ret = create_tag(NULL);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "create tag failed, ret %u", ret);
    return SDK_RET_OK;
}

static inline void
populate_meter_api_rule_spec (uint32_t id, pds_meter_rule_t *api_rule_spec,
                              pds_meter_type_t type, uint64_t pps_bps,
                              uint64_t burst, uint32_t num_prefixes,
                              uint32_t *meter_pfx_count,
                              uint8_t ip_af, ip_prefix_t *v6_route_pfx,
                              uint32_t priority)
{
    api_rule_spec->type = type;
    api_rule_spec->priority = priority;
    switch (type) {
    case PDS_METER_TYPE_PPS_POLICER:
        api_rule_spec->pps = pps_bps;
        api_rule_spec->pkt_burst = burst;
        break;

    case PDS_METER_TYPE_BPS_POLICER:
        api_rule_spec->bps = pps_bps;
        api_rule_spec->byte_burst = burst;
        break;

    case PDS_METER_TYPE_ACCOUNTING:
        break;

    case PDS_METER_TYPE_NONE:
    default:
        break;
    }
    ip_prefix_t *prefixes =
            (ip_prefix_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_METER,
                           (num_prefixes * sizeof(ip_prefix_t)));
    memset(prefixes, 0, num_prefixes * sizeof(ip_prefix_t));
    api_rule_spec->num_prefixes = num_prefixes;
    api_rule_spec->prefixes = prefixes;
    for (uint32_t pfx = 0;
            pfx < api_rule_spec->num_prefixes; pfx++) {
        if (ip_af == IP_AF_IPV4) {
            prefixes[pfx].len = 24;
            prefixes[pfx].addr.af = IP_AF_IPV4;
            prefixes[pfx].addr.addr.v4_addr =
                          ((0xC << 28) | ((*meter_pfx_count)++ << 8));
        } else {
            compute_ipv6_prefix(&prefixes[pfx], v6_route_pfx,
                                (*meter_pfx_count)++, 120);
        }
    }
}

#define TESTAPP_METER_PRIORITY_STEP 4
sdk_ret_t
create_meter (uint32_t num_meter, uint32_t scale, pds_meter_type_t type,
              uint64_t pps_bps, uint64_t burst, uint32_t ip_af,
              ip_prefix_t *v6_route_pfx)
{
    sdk_ret_t ret;
    pds_meter_spec_t pds_meter;
    uint32_t num_prefixes = TESTAPP_METER_NUM_PREFIXES;
    uint32_t priority = 0;
    uint32_t step = 0;

    // unique IDs across meters
    static pds_meter_id_t id = 1;
    static uint32_t meter_pfx_count = 0;

    if (num_meter == 0) {
        return SDK_RET_OK;
    }

    memset(&pds_meter, 0, sizeof(pds_meter_spec_t));
    pds_meter.af = ip_af;

    // if scale < TESTAPP_METER_NUM_PREFIXES,
    // create num_rules=scale with num_prefixes=1
    if (scale < TESTAPP_METER_NUM_PREFIXES) {
        num_prefixes = 1;
    }

    // if the scale is not divisible by num_prefixes, then create
    // remainder rules with num_prefixes=1
    uint32_t num_rules = scale/num_prefixes;
    uint32_t num_rules_rem = scale % num_prefixes;

    pds_meter.num_rules = num_rules + num_rules_rem;

    pds_meter.rules =
            (pds_meter_rule_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_METER,
                            (pds_meter.num_rules * sizeof(pds_meter_rule_t)));

    for (uint32_t i = 0; i < num_meter; i++) {
        pds_meter.key.id = id++;

        // priority is per meter
        priority = 0;
        step = TESTAPP_METER_PRIORITY_STEP;
        for (uint32_t rule = 0; rule < num_rules; rule++) {
            pds_meter_rule_t *api_rule_spec = &pds_meter.rules[rule];
            if (step == TESTAPP_METER_PRIORITY_STEP) {
                priority = rule + (step - 1);
            }
            populate_meter_api_rule_spec(
                pds_meter.key.id, api_rule_spec, type, pps_bps,
                burst, num_prefixes, &meter_pfx_count, ip_af,
                v6_route_pfx, priority);
            priority--;
            step--;
            if (step == 0) {
                step = TESTAPP_METER_PRIORITY_STEP;
            }
        }
        for (uint32_t rule = num_rules;
                      rule < (num_rules + num_rules_rem); rule++) {
            pds_meter_rule_t *api_rule_spec = &pds_meter.rules[rule];
            if (step == TESTAPP_METER_PRIORITY_STEP) {
                priority = rule + (step - 1);
            }
            populate_meter_api_rule_spec(
                pds_meter.key.id, api_rule_spec, type, pps_bps,
                burst, 1, &meter_pfx_count, ip_af,
                v6_route_pfx, priority);
            priority--;
            step--;
            if (step == 0) {
                step = TESTAPP_METER_PRIORITY_STEP;
            }
        }
        ret = create_meter(&pds_meter);
        SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                                "create meter %u failed, ret %u",
                                pds_meter.key.id, ret);
    }
    // Batching: push leftover objects
    ret = create_meter(NULL);
    SDK_ASSERT_TRACE_RETURN((ret == SDK_RET_OK), ret,
                            "create meter failed, ret %u", ret);
    return SDK_RET_OK;
}

static inline sdk_ret_t
create_nexthops (uint32_t num_nh, ip_prefix_t *ip_pfx, uint32_t num_vpcs)
{
    sdk_ret_t ret;
    pds_nexthop_spec_t pds_nh;
    uint32_t id = 1;
    uint32_t vpc_id = 0;

    for (uint32_t nh = 1; nh <= num_nh; nh++) {
        memset(&pds_nh, 0, sizeof(pds_nexthop_spec_t));
        pds_nh.key = id++;
        pds_nh.type = PDS_NH_TYPE_IP;
        pds_nh.ip.af = IP_AF_IPV4;
        // 1st IP in the TEP prefix is gateway IP, 2nd is MyTEP IP,
        // so skip the first 2 IPs
        pds_nh.ip.addr.v4_addr = ip_pfx->addr.addr.v4_addr + 2 + nh;
#if 0
        pds_nh.vpc.id = vpc_id++;
        if (vpc_id > num_vpcs) {
            vpc_id = 1;
        }
#endif
        pds_nh.vlan = TESTAPP_SWITCH_VNIC_VLAN;
        MAC_UINT64_TO_ADDR(pds_nh.mac,
                           (((((uint64_t)vpc_id & 0x7FF) << 22) |
                             ((1 & 0x7FF) << 11) | (id & 0x7FF))));
        ret = create_nexthop(&pds_nh);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }
    // Batching: push leftover objects
    ret = create_nexthop(NULL);
    if (ret != SDK_RET_OK) {
        SDK_ASSERT(0);
        return ret;
    }
    return SDK_RET_OK;
}

// NOTE: all service TEPs are remote TEPs
sdk_ret_t
create_service_teps (uint32_t num_teps, ip_prefix_t *svc_tep_pfx,
                     ip_prefix_t *tep_pfx)
{
    sdk_ret_t      rv;
    pds_tep_spec_t pds_tep;
    uint32_t tep_vnid = g_test_params.svc_tep_vnid_base;

    if (!num_teps) {
        return SDK_RET_OK;
    }
    for (uint32_t i = 1; i <= num_teps; i++) {
        memset(&pds_tep, 0, sizeof(pds_tep));
        compute_local46_addr(&pds_tep.key.ip_addr, svc_tep_pfx,
                             tep_pfx->addr.addr.v4_addr + 2 + i, i);
        pds_tep.type = PDS_TEP_TYPE_SERVICE;
        pds_tep.encap.type = PDS_ENCAP_TYPE_VXLAN;
        pds_tep.encap.val.vnid = tep_vnid++;
        rv = create_tunnel(g_test_params.num_teps + i + 1, &pds_tep);
        if (rv != SDK_RET_OK) {
            return rv;
        }
    }
    // push leftover objects
    rv = create_tunnel(0, NULL);
    if (rv != SDK_RET_OK) {
        SDK_ASSERT(0);
        return rv;
    }
    return SDK_RET_OK;
}

sdk_ret_t
create_teps (uint32_t num_teps, ip_prefix_t *ip_pfx)
{
    sdk_ret_t      rv;
    pds_tep_spec_t pds_tep;

    // leave the 1st IP in this prefix for MyTEP
    for (uint32_t i = 1; i <= num_teps; i++) {
        memset(&pds_tep, 0, sizeof(pds_tep));
        // 1st IP in the TEP prefix is gateway IP, 2nd is MyTEP IP,
        // so we skip the 1st (even for MyTEP we create a TEP)
        pds_tep.key.ip_addr.af = IP_AF_IPV4;
        pds_tep.key.ip_addr.addr.v4_addr = ip_pfx->addr.addr.v4_addr + 1 + i;
        MAC_UINT64_TO_ADDR(pds_tep.mac, (((uint64_t)0x0303 << 22) | i));
        if (g_test_params.fabric_encap.type == PDS_ENCAP_TYPE_VXLAN) {
            pds_tep.encap.type = PDS_ENCAP_TYPE_VXLAN;
            pds_tep.type = PDS_TEP_TYPE_WORKLOAD;
        } else {
            pds_tep.encap.type = PDS_ENCAP_TYPE_MPLSoUDP;
            pds_tep.type = PDS_TEP_TYPE_WORKLOAD;
        }
        rv = create_tunnel(i, &pds_tep);
        if (rv != SDK_RET_OK) {
            return rv;
        }
    }
    // Batching: push leftover objects
    rv = create_tunnel(0, NULL);
    if (rv != SDK_RET_OK) {
        SDK_ASSERT(0);
        return rv;
    }
    return SDK_RET_OK;
}

sdk_ret_t
create_device_cfg (ipv4_addr_t ipaddr, uint64_t macaddr, ipv4_addr_t gwip)
{
    sdk_ret_t            rv;

    memset(&g_device, 0, sizeof(g_device));
    g_device.device_ip_addr = ipaddr;
    MAC_UINT64_TO_ADDR(g_device.device_mac_addr, macaddr);
    g_device.gateway_ip_addr = gwip;

    rv = create_device(&g_device);

    return rv;
}

#define TESTAPP_POLICY_PRIORITY_STEP 4
sdk_ret_t
create_security_policy (uint32_t num_vpcs, uint32_t num_subnets,
                        uint32_t num_rules, uint32_t ip_af, bool ingress)
{
    sdk_ret_t            rv;
    pds_policy_spec_t    policy;
    static uint32_t      policy_id = 1, num_pfx;
    rule_t               *rule;
    uint32_t             num_sub_rules = 10;
    uint16_t             step = 4;
    bool                 done;
    uint32_t             priority = 0;
    uint32_t             priority_step = TESTAPP_POLICY_PRIORITY_STEP;

    if (num_rules == 0) {
        return SDK_RET_OK;
    }

    policy.policy_type = POLICY_TYPE_FIREWALL;
    policy.af = ip_af;
    policy.direction = ingress ? RULE_DIR_INGRESS : RULE_DIR_EGRESS;
    policy.num_rules = num_rules;
    policy.rules = (rule_t *)SDK_CALLOC(PDS_MEM_ALLOC_ID_POLICY_RULES,
                                        num_rules * sizeof(rule_t));
    if (num_rules < num_sub_rules) {
        num_sub_rules = 1;
    }
    num_pfx = (uint32_t)ceilf((float)num_rules/(float)num_sub_rules);
    for (uint32_t i = 1; i <= num_vpcs; i++) {
        for (uint32_t j = 1, idx = 0; j <= num_subnets; idx = 0, j++) {
            memset(policy.rules, 0, num_rules * sizeof(rule_t));
            policy.key.id = policy_id++;
            done = false;
            priority = 0;
            priority_step = TESTAPP_POLICY_PRIORITY_STEP;
            for (uint32_t k = 0; k < num_pfx; k++) {
                uint16_t dport_base = 1024;
                for (uint32_t l = 0; l < num_sub_rules; l++) {
                    rule = &policy.rules[idx];
                    if (priority_step == TESTAPP_POLICY_PRIORITY_STEP) {
                        priority = idx + (priority_step - 1);
                    }
                    rule->priority = priority;
                    priority--;
                    priority_step--;
                    if (priority_step == 0) {
                        priority_step = TESTAPP_POLICY_PRIORITY_STEP;
                    }
                    rule->action_data.fw_action.action =
                        SECURITY_RULE_ACTION_ALLOW;
                    rule->stateful = g_test_params.stateful;
                    rule->match.l3_match.ip_proto = 17;    // UDP
                    rule->match.l4_match.sport_range.port_lo = 100;
                    rule->match.l4_match.sport_range.port_hi = 10000;
                    if (idx < (num_rules - 3)) {
                        if (policy.af == IP_AF_IPV4) {
                            if (ingress) {
                                rule->match.l3_match.src_ip_pfx.addr.af =
                                    policy.af;
                                rule->match.l3_match.src_ip_pfx =
                                    g_test_params.vpc_pfx;
                                rule->match.l3_match.src_ip_pfx.addr.addr.v4_addr =
                                        rule->match.l3_match.src_ip_pfx.addr.addr.v4_addr |
                                        ((j - 1) << 14) | ((k + 2) << 4);
                                rule->match.l3_match.src_ip_pfx.len = 28;
                            } else {
                                rule->match.l3_match.dst_ip_pfx.addr.af = policy.af;
                                rule->match.l3_match.dst_ip_pfx = g_test_params.vpc_pfx;
                                rule->match.l3_match.dst_ip_pfx.addr.addr.v4_addr =
                                        rule->match.l3_match.dst_ip_pfx.addr.addr.v4_addr |
                                        ((j - 1) << 14) | ((k + 2) << 4);
                                rule->match.l3_match.dst_ip_pfx.len = 28;
                            }
                        }
                        else {
                            if (ingress) {
                                rule->match.l3_match.src_ip_pfx.addr.af = policy.af;
                                rule->match.l3_match.src_ip_pfx = g_test_params.v6_vpc_pfx;
                                rule->match.l3_match.src_ip_pfx.addr.addr.v6_addr.addr32[3] =
                                        rule->match.l3_match.src_ip_pfx.addr.addr.v6_addr.addr32[3] |
                                        htonl(((j - 1) << 14) | ((k + 2) << 4));
                                rule->match.l3_match.src_ip_pfx.len = 124;
                            } else {
                                rule->match.l3_match.dst_ip_pfx.addr.af = policy.af;
                                rule->match.l3_match.dst_ip_pfx = g_test_params.v6_vpc_pfx;
                                rule->match.l3_match.dst_ip_pfx.addr.addr.v6_addr.addr32[3] =
                                        rule->match.l3_match.dst_ip_pfx.addr.addr.v6_addr.addr32[3] |
                                        htonl(((j - 1) << 14) | ((k + 2) << 4));
                                rule->match.l3_match.dst_ip_pfx.len = 124;
                            }
                        }
                        rule->match.l4_match.dport_range.port_lo = dport_base;
                        rule->match.l4_match.dport_range.port_hi =
                            dport_base + step - 1;
                        dport_base += step;
                        idx++;
                    } else if (idx < (num_rules - 2)) {
                        // catch-all policy within the vpc for UDP traffic
                        rule->match.l4_match.dport_range.port_lo = 100;
                        rule->match.l4_match.dport_range.port_hi = 20000;
                        idx++;
                    } else if (idx < (num_rules - 1)) {
                        // catch-all policy within the vpc for TCP traffic
                        rule->match.l3_match.ip_proto = 6;
                        rule->match.l4_match.dport_range.port_lo = 0;
                        rule->match.l4_match.dport_range.port_hi = 65535;
                        idx++;
                    } else {
                        // catch-all policy for LPM routes + UDP
                        if (policy.af == IP_AF_IPV4) {
                            if (ingress) {
                                rule->match.l3_match.src_ip_pfx.addr.af = policy.af;
                                rule->match.l3_match.src_ip_pfx.addr.addr.v4_addr = (0xC << 28);
                                rule->match.l3_match.src_ip_pfx.len = 8;
                            } else {
                                rule->match.l3_match.dst_ip_pfx.addr.af = policy.af;
                                rule->match.l3_match.dst_ip_pfx.addr.addr.v4_addr = (0xC << 28);
                                rule->match.l3_match.dst_ip_pfx.len = 8;
                            }
                        } else {
                            if (ingress) {
                                rule->match.l3_match.src_ip_pfx.addr.af = policy.af;
                                rule->match.l3_match.src_ip_pfx.addr.addr.v6_addr.addr32[0] = htonl(0x20210000);
                                rule->match.l3_match.src_ip_pfx.addr.addr.v6_addr.addr32[1] = htonl(0x00000000);
                                rule->match.l3_match.src_ip_pfx.addr.addr.v6_addr.addr32[2] = htonl(0xF1D0D1D0);
                                rule->match.l3_match.src_ip_pfx.addr.addr.v6_addr.addr32[3] = htonl(0x00000000);
                                rule->match.l3_match.src_ip_pfx.len = 96;
                            } else {
                                rule->match.l3_match.dst_ip_pfx.addr.af = policy.af;
                                rule->match.l3_match.dst_ip_pfx.addr.addr.v6_addr.addr32[0] = htonl(0x20210000);
                                rule->match.l3_match.dst_ip_pfx.addr.addr.v6_addr.addr32[1] = htonl(0x00000000);
                                rule->match.l3_match.dst_ip_pfx.addr.addr.v6_addr.addr32[2] = htonl(0xF1D0D1D0);
                                rule->match.l3_match.dst_ip_pfx.addr.addr.v6_addr.addr32[3] = htonl(0x00000000);
                                rule->match.l3_match.dst_ip_pfx.len = 96;
                            }
                        }
                        rule->match.l4_match.dport_range.port_lo = 1000;
                        rule->match.l4_match.dport_range.port_hi = 20000;
                        done = true;
                        break;
                    }
                }
                if (done) {
                    break;
                }
            }
            rv = create_policy(&policy);
            if (rv != SDK_RET_OK) {
                printf("Failed to create security policy, vpc %u, subnet %u, "
                       "err %u\n", i, j, rv);
                return rv;
            }
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
create_rspan_mirror_sessions (uint32_t num_sessions)
{
    sdk_ret_t rv;
    static uint16_t rspan_vlan_start = 4094;
    static uint32_t msid = 1, i;
    pds_mirror_session_spec_t ms;

    for (i = 0; i < num_sessions; i++) {
        ms.key.id = msid++;
        ms.type = PDS_MIRROR_SESSION_TYPE_RSPAN;
        ms.snap_len = 128;
        ms.rspan_spec.interface = 0x11010001;  // eth 1/1
        ms.rspan_spec.encap.type = PDS_ENCAP_TYPE_DOT1Q;
        ms.rspan_spec.encap.val.vlan_tag = rspan_vlan_start--;
        rv = create_mirror_session(&ms);
        if (rv != SDK_RET_OK) {
            printf("Failed to create mirror session %u, err %u\n",
                   ms.key.id, rv);
            return rv;
        }
    }
    return SDK_RET_OK;
}

sdk_ret_t
create_objects (void)
{
    sdk_ret_t ret;

    ret = parse_test_cfg(g_input_cfg_file, &g_test_params);
    if (ret != SDK_RET_OK) {
        exit(1);
    }

    ret = create_objects_init(&g_test_params);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // create device config
    ret = create_device_cfg(g_test_params.device_ip, g_test_params.device_mac,
                            g_test_params.device_gw_ip);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    if (artemis()) {
        // create v4 tags
        ret = create_tags(g_test_params.num_tag_trees/2,
                          g_test_params.tags_v4_scale, IP_AF_IPV4,
                          &g_test_params.v6_route_pfx);
        if (ret != SDK_RET_OK) {
            return ret;
        }
        // create v6 tags
        ret = create_tags(g_test_params.num_tag_trees/2,
                          g_test_params.tags_v6_scale, IP_AF_IPV6,
                          &g_test_params.v6_route_pfx);
        if (ret != SDK_RET_OK) {
            return ret;
        }

        // create v4 meter
        // meter ids: 1 to num_meter
        ret = create_meter(g_test_params.num_meter, g_test_params.meter_scale,
                           g_test_params.meter_type, g_test_params.pps_bps,
                           g_test_params.burst, IP_AF_IPV4,
                           &g_test_params.v6_route_pfx);
        if (ret != SDK_RET_OK) {
            return ret;
        }
        // create v6 meter
        // meter ids: num_meter+1 to 2*num_meter
        ret = create_meter(g_test_params.num_meter, g_test_params.meter_scale,
                           g_test_params.meter_type, g_test_params.pps_bps,
                           g_test_params.burst, IP_AF_IPV6,
                           &g_test_params.v6_route_pfx);
        if (ret != SDK_RET_OK) {
            return ret;
        }

        // create nexthops
        ret = create_nexthops(g_test_params.num_nh, &g_test_params.tep_pfx,
                              g_test_params.num_vpcs);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    // create TEPs including MyTEP
    ret = create_teps(g_test_params.num_teps + 1, &g_test_params.tep_pfx);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    if (artemis()) {
        // create service TEPs
        ret = create_service_teps(TESTAPP_MAX_SERVICE_TEP,
                                  &g_test_params.svc_tep_pfx,
                                  &g_test_params.tep_pfx);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    // create route tables
    ret = create_route_tables(g_test_params.num_teps, g_test_params.num_vpcs,
                              g_test_params.num_subnets,
                              g_test_params.num_routes,
                              &g_test_params.tep_pfx, &g_test_params.route_pfx,
                              &g_test_params.v6_route_pfx,
                              g_test_params.num_nh);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // create security policies
    ret = create_security_policy(g_test_params.num_vpcs,
                                 g_test_params.num_subnets,
                                 g_test_params.num_ipv4_rules,
                                 IP_AF_IPV4, false);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ret = create_security_policy(g_test_params.num_vpcs,
                                 g_test_params.num_subnets,
                                 g_test_params.num_ipv4_rules,
                                 IP_AF_IPV4, true);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ret = create_security_policy(g_test_params.num_vpcs,
                                 g_test_params.num_subnets,
                                 g_test_params.num_ipv6_rules,
                                 IP_AF_IPV6, false);
    if (ret != SDK_RET_OK) {
        return ret;
    }
    ret = create_security_policy(g_test_params.num_vpcs,
                                 g_test_params.num_subnets,
                                 g_test_params.num_ipv6_rules,
                                 IP_AF_IPV6, true);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // create vpcs and subnets
    ret = create_vpcs(g_test_params.num_vpcs, &g_test_params.vpc_pfx,
                      &g_test_params.v6_vpc_pfx, &g_test_params.nat46_vpc_pfx,
                      g_test_params.num_subnets);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    if (artemis()) {
        // create vpc peers
        ret = create_vpc_peers(g_test_params.num_vpcs);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    if (apollo()) {
        // create RSPAN mirror sessions
        if (g_test_params.mirror_en && (g_test_params.num_rspan > 0)) {
            ret = create_rspan_mirror_sessions(g_test_params.num_rspan);
            if (ret != SDK_RET_OK) {
                return ret;
            }
        }
    }

    // create vnics
    ret = create_vnics(g_test_params.num_vpcs, g_test_params.num_subnets,
                       g_test_params.num_vnics, g_test_params.vlan_start,
                       g_test_params.num_meter);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // create mappings
    ret = create_mappings(g_test_params.num_teps, g_test_params.num_vpcs,
                          g_test_params.num_subnets, g_test_params.num_vnics,
                          g_test_params.num_ip_per_vnic, &g_test_params.tep_pfx,
                          &g_test_params.nat_pfx, &g_test_params.v6_nat_pfx,
                          g_test_params.num_remote_mappings,
                          &g_test_params.provider_pfx,
                          &g_test_params.v6_provider_pfx);
    if (ret != SDK_RET_OK) {
        return ret;
    }

    // create service mappings
    if (artemis()) {
        ret = create_svc_mappings(g_test_params.num_vpcs,
                                  g_test_params.num_subnets,
                                  g_test_params.num_vnics,
                                  g_test_params.num_ip_per_vnic,
                                  &g_test_params.v4_vip_pfx,
                                  &g_test_params.v6_vip_pfx,
                                  &g_test_params.provider_pfx,
                                  &g_test_params.v6_provider_pfx);
        if (ret != SDK_RET_OK) {
            return ret;
        }
    }

    ret = create_objects_end();
    if (ret != SDK_RET_OK) {
        return SDK_RET_ERR;
    }

    return ret;
}
