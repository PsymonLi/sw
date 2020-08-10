#define DEVICE_INFO_TABLE_SIZE                              16
#define LIF_TABLE_SIZE                                      2048
#define LIF_VLAN_HASH_TABLE_SIZE                            8192
#define LIF_VLAN_OTCAM_TABLE_SIZE                           1024
#define VNI_HASH_TABLE_SIZE                                 8192
#define VNI_OTCAM_TABLE_SIZE                                1024
#define LOCAL_MAPPING_TABLE_SIZE                            131072      // 128K
#define LOCAL_MAPPING_OHASH_TABLE_SIZE                      16384       // 16K
#define SERVICE_MAPPING_TABLE_SIZE                          4096
#define SERVICE_MAPPING_OTCAM_TABLE_SIZE                    1024
#define IPV4_FLOW_TABLE_SIZE                                8388608     // 8M
#define IPV4_FLOW_OHASH_TABLE_SIZE                          1048576     // 1M
#define FLOW_TABLE_SIZE                                     2097152     // 2M
#define FLOW_OHASH_TABLE_SIZE                               262144      // 256K
#define FLOW_INFO_TABLE_SIZE                                4194304     // 4M
#define IP_MAC_BINDING_TABLE_SIZE                           32768       // 32K
#define NACL_TABLE_SIZE                                     512
#define QOS_TABLE_SIZE                                      32
#define VNIC_TABLE_SIZE                                     1024
#define DROP_STATS_TABLE_SIZE                               64

#define MAPPING_TABLE_SIZE                                  4194304     // 4M
#define MAPPING_OHASH_TABLE_SIZE                            524288      // 512K
#define SESSION_TABLE_SIZE                                  2097152     // 2M
#define MIRROR_SESSION_TABLE_SIZE                           256
#define NAT_TABLE_SIZE                                      1048576     // 1M
#define NAT_NUM_STATIC_ENTRIES                              131072      // 128K carved out of NAT_TABLE_SIZE
#define NAT2_TABLE_SIZE                                     8192        // 8K
#define ECMP_TABLE_SIZE                                     2048
#define TUNNEL_TABLE_SIZE                                   2048
#define TUNNEL2_TABLE_SIZE                                  2048
#define NEXTHOP_TABLE_SIZE                                  65536
#define CHECKSUM_TABLE_SIZE                                 32
#define VPC_TABLE_SIZE                                      1024
#define BD_TABLE_SIZE                                       2048
#define COPP_TABLE_SIZE                                     1024
#define METER_TABLE_SIZE                                    2048
#define POLICER_TABLE_SIZE                                  1024
#define LIF_STATS_TABLE_SIZE                                16384
#define APP_TABLE_SIZE                                      16

#define VNIC_INFO_TABLE_SIZE                                1024
#define VNIC_INFO_RXDMA_TABLE_SIZE                          4096
#define DNAT_TABLE_SIZE                                     4096        // NAT2_TABLE_SIZE / 2
#define LOCAL_MAPPING_TAG_TABLE_SIZE                        65536       // 64K
#define MAPPING_TAG_TABLE_SIZE                              1048576     // 1M
