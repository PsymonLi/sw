#define INPUT_MAPPING_TABLE_SIZE                           64
#define INPUT_PROPERTIES_TABLE_SIZE                        8192
#define INPUT_PROPERTIES_OTCAM_TABLE_SIZE                  1024
#define INPUT_PROPERTIES_MAC_VLAN_TABLE_SIZE               1024
#define DECODE_ROCE_OPCODE_TABLE_SIZE                      256
#define L4_PROFILE_TABLE_SIZE                              256
#define DDOS_SRC_VF_TABLE_SIZE                             256
#define DDOS_SRC_DST_TABLE_SIZE                            512
#define DDOS_SERVICE_TABLE_SIZE                            256
#define FLOW_HASH_TABLE_SIZE                               2097152
#define FLOW_HASH_OVERFLOW_TABLE_SIZE                      262144
#define FLOW_TABLE_SIZE                                    1048576
#define FLOW_STATE_TABLE_SIZE                              1048576
#define REGISTERED_MACS_TABLE_SIZE                         8192
#define REGISTERED_MACS_OTCAM_TABLE_SIZE                   256
#define NACL_TABLE_SIZE                                    512
#define QOS_TABLE_SIZE                                     32
#define COPP_TABLE_SIZE                                    32
#define TUNNEL_DECAP_TABLE_SIZE                            32
#define TUNNEL_ENCAP_UPDATE_INNER_TABLE_SIZE               32
#define DROP_STATS_TABLE_SIZE                              64
#define IPSG_TABLE_SIZE                                    1024
#define OUTPUT_MAPPING_TABLE_SIZE                          2048
#define TX_STATS_TABLE_SIZE                                2048

#define TWICE_NAT_TABLE_SIZE                               2048
#define REWRITE_TABLE_SIZE                                 4096
#define TUNNEL_REWRITE_TABLE_SIZE                          1024
#define RX_POLICER_TABLE_SIZE                              2048
#define MIRROR_SESSION_TABLE_SIZE                          256
#define P4PLUS_APP_TABLE_SIZE                              256
#define CHECKSUM_COMPUTE_TABLE_SIZE                        64

#ifdef PLATFORM_HAPS
#undef INPUT_PROPERTIES_OTCAM_TABLE_SIZE
#undef INPUT_PROPERTIES_MAC_VLAN_TABLE_SIZE
#undef REGISTERED_MACS_OTCAM_TABLE_SIZE
#undef NACL_TABLE_SIZE
#undef IPSG_TABLE_SIZE

#define INPUT_PROPERTIES_OTCAM_TABLE_SIZE                  64
#define INPUT_PROPERTIES_MAC_VLAN_TABLE_SIZE               64
#define REGISTERED_MACS_OTCAM_TABLE_SIZE                   64
#define NACL_TABLE_SIZE                                    64
#define IPSG_TABLE_SIZE                                    64
#endif
