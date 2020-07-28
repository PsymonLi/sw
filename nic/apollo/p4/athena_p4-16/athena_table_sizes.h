#define KEY_MAPPING_TABLE_SIZE                              64

#define FLOW_TABLE_SIZE                                     2097152     // 2M

#define FLOW_OHASH_TABLE_SIZE                               1048576     // 2M/2 

#define DNAT_TABLE_SIZE                                     16384       // 16K
#define DNAT_OHASH_TABLE_SIZE                               4096        // 16K/4

#define L2_FLOW_TABLE_SIZE                                     1048576     // 1M
#define L2_FLOW_OHASH_TABLE_SIZE                               262144      // 1M/4 
//#Define L2_FLOW_OHASH_TABLE_SIZE                               524416      // 1M/2 

//Dummy IPv4 table for compilation
#define IPV4_FLOW_TABLE_SIZE                                64
#define IPV4_FLOW_OHASH_TABLE_SIZE                          64

#define NACL_TABLE_SIZE                                     512
#define MIRRORING_NACL_TABLE_SIZE                           32

#define CHECKSUM_TABLE_SIZE                                 32

#define VNIC_TABLE_SIZE                                     1024
#define VLAN_VNIC_MAP_TABLE_SIZE                            4096
//#define MPLS_LABEL_VNIC_MAP_TABLE_SIZE                      1048576
//FIXME: To workaround the NCC limitation to upgrade the key size from 20b to 24b, bumping up the table size
#define MPLS_LABEL_VNIC_MAP_TABLE_SIZE                      1048576
//#define MPLS_LABEL_VNIC_MAP_TABLE_SIZE                      1048576

#define SESSION_TABLE_SIZE                                  4194304 // 4M

#define POLICER_BW1_SIZE                                    2560
#define POLICER_BW2_SIZE                                    2560
#define POLICER_PPS_SIZE                                    512  //VNIC#

#define CONFIG_TABLE_SIZE                                   1048576 // 1M

#define CONNTRACK_TABLE_SIZE                                4194304     // 4M: used 3.6M

#define P4E_REDIR_TABLE_SIZE                                2
#define FLOW_LOG_TABLE_SIZE                                 2097152 // 2M
