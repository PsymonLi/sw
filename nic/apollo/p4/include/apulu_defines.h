#include "nic/p4/common/defines.h"

/*****************************************************************************/
/* IP types                                                                  */
/*****************************************************************************/
#define IPTYPE_IPV4                     0
#define IPTYPE_IPV6                     1

/*****************************************************************************/
/* key types                                                                 */
/*****************************************************************************/
#define KEY_TYPE_NONE                   0
#define KEY_TYPE_IPV4                   1
#define KEY_TYPE_IPV6                   2
#define KEY_TYPE_MAC                    3
#define KEY_TYPE_IPSEC                  4

/*****************************************************************************/
/* lif_vlan table lookup modes                                               */
/*****************************************************************************/
#define APULU_LIF_VLAN_MODE_VLAN        0
#define APULU_LIF_VLAN_MODE_LIF_VLAN    2
#define APULU_LIF_VLAN_MODE_QINQ        3

/*****************************************************************************/
/* Lifs                                                                      */
/*****************************************************************************/
#define APULU_SERVICE_LIF               34
#define APULU_IPSEC_LIF                 35

/*****************************************************************************/
/* Direction (lif connected to host or uplink)                               */
/*****************************************************************************/
#define P4_LIF_DIR_HOST                 0
#define P4_LIF_DIR_UPLINK               1

/*****************************************************************************/
/* drop reasons - these are bit positions to be used in ASM                  */
/*****************************************************************************/
#define P4I_DROP_REASON_MIN                         0
#define P4I_DROP_VNI_INVALID                        0
#define P4I_DROP_TAGGED_PKT_FROM_VNIC               1
#define P4I_DROP_IP_FRAGMENT                        2
#define P4I_DROP_IPV6                               3
#define P4I_DROP_DHCP_SERVER_SPOOFING               4
#define P4I_DROP_L2_MULTICAST                       5
#define P4I_DROP_IP_MULTICAST                       6
#define P4I_DROP_PF_SUBNET_BINDING                  7
#define P4I_DROP_PROTOCOL_NOT_SUPPORTED             8
#define P4I_DROP_NACL                               9
#define P4I_DROP_REASON_MAX                         P4I_DROP_NACL

#define P4E_DROP_REASON_MIN                         0
#define P4E_DROP_SESSION_INVALID                    0
#define P4E_DROP_SESSION_HIT                        1
#define P4E_DROP_NEXTHOP_INVALID                    2
#define P4E_DROP_VNIC_POLICER_RX                    3
#define P4E_DROP_VNIC_POLICER_TX                    4
#define P4E_DROP_TCP_OUT_OF_WINDOW                  5
#define P4E_DROP_TCP_WIN_ZERO                       6
#define P4E_DROP_TCP_UNEXPECTED_PKT                 7
#define P4E_DROP_TCP_NON_RST_PKT_AFTER_RST          8
#define P4E_DROP_TCP_RST_WITH_INVALID_ACK_NUM       9
#define P4E_DROP_TCP_INVALID_RESPONDER_FIRST_PKT    10
#define P4E_DROP_TCP_DATA_AFTER_FIN                 11
#define P4E_DROP_MAC_IP_BINDING_FAIL                12
#define P4E_DROP_COPP_MIN                           13
#define P4E_DROP_COPP_MAX                           28
#define P4E_DROP_REASON_MAX                         P4E_DROP_COPP_MAX

/*****************************************************************************/
/* nexthop types                                                            */
/*****************************************************************************/
#define NEXTHOP_TYPE_VPC                0
#define NEXTHOP_TYPE_ECMP               1
#define NEXTHOP_TYPE_TUNNEL             2
#define NEXTHOP_TYPE_NEXTHOP            3
#define NEXTHOP_TYPE_NAT                4
#define NEXTHOP_TYPE_MAX                5

/*****************************************************************************/
/* flow role                                                                 */
/*****************************************************************************/
#define TCP_FLOW_INITIATOR              0
#define TCP_FLOW_RESPONDER              1

/*****************************************************************************/
/* flow states                                                               */
/*****************************************************************************/
#define FLOW_STATE_INIT                 0
#define FLOW_STATE_TCP_SYN_RCVD         1
#define FLOW_STATE_TCP_ACK_RCVD         2
#define FLOW_STATE_TCP_SYN_ACK_RCVD     3
#define FLOW_STATE_ESTABLISHED          4
#define FLOW_STATE_FIN_RCVD             5
#define FLOW_STATE_BIDIR_FIN_RCVD       6
#define FLOW_STATE_RESET                7

/*****************************************************************************/
/* TCP exceptions                                                            */
/*****************************************************************************/
#define TCP_SYN_REXMIT                  0x0001
#define TCP_WIN_ZERO_DROP               0x0002
#define TCP_FULL_REXMIT                 0x0004
#define TCP_PARTIAL_OVERLAP             0x0008
#define TCP_PACKET_REORDER              0x0010
#define TCP_OUT_OF_WINDOW               0x0020
#define TCP_ACK_ERR                     0x0040
#define TCP_NORMALIZATION_DROP          0x0080
#define TCP_SPLIT_HANDSHAKE_DETECTED    0x0100
#define TCP_DATA_AFTER_FIN              0x0200
#define TCP_NON_RST_PKT_AFTER_RST       0x0400
#define TCP_INVALID_RESPONDER_FIRST_PKT 0x0800
#define TCP_UNEXPECTED_PKT              0x1000
#define TCP_RST_WITH_INVALID_ACK_NUM    0x2000

/*****************************************************************************/
/* rewrite flags                                                             */
/*****************************************************************************/
#define P4_REWRITE_DMAC_START           0
#define P4_REWRITE_DMAC_MASK            3
#define P4_REWRITE_DMAC_NONE            0
#define P4_REWRITE_DMAC_FROM_MAPPING    1
#define P4_REWRITE_DMAC_FROM_NEXTHOP    2
#define P4_REWRITE_DMAC_FROM_TUNNEL     3
#define P4_REWRITE_DMAC_BITS            1:0

#define P4_REWRITE_SMAC_START           2
#define P4_REWRITE_SMAC_MASK            3
#define P4_REWRITE_SMAC_NONE            0
#define P4_REWRITE_SMAC_FROM_VRMAC      1
#define P4_REWRITE_SMAC_FROM_NEXTHOP    2
#define P4_REWRITE_SMAC_BITS            3:2

#define P4_REWRITE_SIP_START            4
#define P4_REWRITE_SIP_MASK             1
#define P4_REWRITE_SIP_NONE             0
#define P4_REWRITE_SIP_FROM_NAT         1
#define P4_REWRITE_SIP_BITS             4:4

#define P4_REWRITE_DIP_START            5
#define P4_REWRITE_DIP_MASK             1
#define P4_REWRITE_DIP_NONE             0
#define P4_REWRITE_DIP_FROM_NAT         1
#define P4_REWRITE_DIP_BITS             5:5

#define P4_REWRITE_SPORT_START          6
#define P4_REWRITE_SPORT_MASK           1
#define P4_REWRITE_SPORT_NONE           0
#define P4_REWRITE_SPORT_FROM_NAT       1
#define P4_REWRITE_SPORT_BITS           6:6

#define P4_REWRITE_DPORT_START          7
#define P4_REWRITE_DPORT_MASK           1
#define P4_REWRITE_DPORT_NONE           0
#define P4_REWRITE_DPORT_FROM_NAT       1
#define P4_REWRITE_DPORT_BITS           7:7

#define P4_REWRITE_ENCAP_START          8
#define P4_REWRITE_ENCAP_MASK           3
#define P4_REWRITE_ENCAP_NONE           0
#define P4_REWRITE_ENCAP_VXLAN          1
#define P4_REWRITE_ENCAP_MPLSoUDP       2
#define P4_REWRITE_ENCAP_BITS           9:8

#define P4_REWRITE_VNI_START            10
#define P4_REWRITE_VNI_MASK             1
#define P4_REWRITE_VNI_DEFAULT          0
#define P4_REWRITE_VNI_FROM_TUNNEL      1
#define P4_REWRITE_VNI_BITS             10:10

#define P4_REWRITE_TTL_START            11
#define P4_REWRITE_TTL_MASK             1
#define P4_REWRITE_TTL_NONE             0
#define P4_REWRITE_TTL_DEC              1
#define P4_REWRITE_TTL_BITS             11:11

#define P4_REWRITE_VLAN_START           12
#define P4_REWRITE_VLAN_MASK            3
#define P4_REWRITE_VLAN_NONE            0
#define P4_REWRITE_VLAN_ENCAP           1
#define P4_REWRITE_VLAN_DECAP           2
#define P4_REWRITE_VLAN_BITS            13:12

#define P4_REWRITE(a, attr, val) \
    ((((a) >> P4_REWRITE_ ## attr ## _START) & P4_REWRITE_ ## attr ## _MASK) == P4_REWRITE_ ## attr ## _ ## val)

#define P4_SET_REWRITE(attr, val) \
    ((P4_REWRITE_ ## attr ## _ ## val) << (P4_REWRITE_ ## attr ## _START))

/*****************************************************************************/
/* route result type and bit position                                        */
/*****************************************************************************/
#define ROUTE_RESULT_METER_EN_SIZE           1
#define ROUTE_RESULT_SNAT_TYPE_SIZE          2
#define ROUTE_RESULT_SNAT_TYPE_NONE          0
#define ROUTE_RESULT_SNAT_TYPE_STATIC        1
#define ROUTE_RESULT_SNAT_TYPE_NAPT_PUBLIC   2
#define ROUTE_RESULT_SNAT_TYPE_NAPT_SVC      3
#define ROUTE_RESULT_DNAT_EN_SIZE            1
#define ROUTE_RESULT_DNAT_IDX_SIZE           28         // DNAT_EN == 1
#define ROUTE_RESULT_NHTYPE_SIZE             2          // DNAT_EN == 0
#define ROUTE_RESULT_PRIORITY_SIZE           10         // DNAT_EN == 0
#define ROUTE_RESULT_NEXTHOP_SIZE            16         // DNAT_EN == 0

#define ROUTE_RESULT_METER_EN_MASK           0x80000000
#define ROUTE_RESULT_SNAT_TYPE_MASK          0x60000000
#define ROUTE_RESULT_DNAT_EN_MASK            0x10000000
#define ROUTE_RESULT_DNAT_IDX_MASK           0x0FFFFFFF
#define ROUTE_RESULT_NHTYPE_MASK             0x0C000000
#define ROUTE_RESULT_PRIORITY_MASK           0x03FF0000
#define ROUTE_RESULT_NEXTHOP_MASK            0x0000FFFF

#define ROUTE_RESULT_METER_EN_END_BIT       31
#define ROUTE_RESULT_METER_EN_START_BIT     (ROUTE_RESULT_METER_EN_END_BIT -\
                                             ROUTE_RESULT_METER_EN_SIZE + 1)
#define ROUTE_RESULT_METER_EN_SHIFT          ROUTE_RESULT_METER_EN_START_BIT

#define ROUTE_RESULT_SNAT_TYPE_END_BIT      (ROUTE_RESULT_METER_EN_START_BIT - 1)
#define ROUTE_RESULT_SNAT_TYPE_START_BIT    (ROUTE_RESULT_SNAT_TYPE_END_BIT -\
                                             ROUTE_RESULT_SNAT_TYPE_SIZE + 1)
#define ROUTE_RESULT_SNAT_TYPE_SHIFT         ROUTE_RESULT_SNAT_TYPE_START_BIT

#define ROUTE_RESULT_DNAT_EN_END_BIT        (ROUTE_RESULT_SNAT_TYPE_START_BIT - 1)
#define ROUTE_RESULT_DNAT_EN_START_BIT      (ROUTE_RESULT_DNAT_EN_END_BIT -\
                                             ROUTE_RESULT_DNAT_EN_SIZE + 1)
#define ROUTE_RESULT_DNAT_EN_SHIFT           ROUTE_RESULT_DNAT_EN_START_BIT

#define ROUTE_RESULT_DNAT_IDX_END_BIT       (ROUTE_RESULT_DNAT_EN_START_BIT - 1)
#define ROUTE_RESULT_DNAT_IDX_START_BIT     (ROUTE_RESULT_DNAT_IDX_END_BIT -\
                                             ROUTE_RESULT_DNAT_IDX_SIZE + 1)
#define ROUTE_RESULT_DNAT_IDX_SHIFT          ROUTE_RESULT_DNAT_IDX_START_BIT

#define ROUTE_RESULT_NHTYPE_END_BIT         (ROUTE_RESULT_DNAT_EN_START_BIT - 1)
#define ROUTE_RESULT_NHTYPE_START_BIT       (ROUTE_RESULT_NHTYPE_END_BIT -\
                                             ROUTE_RESULT_NHTYPE_SIZE + 1)
#define ROUTE_RESULT_NHTYPE_SHIFT            ROUTE_RESULT_NHTYPE_START_BIT

#define ROUTE_RESULT_PRIORITY_END_BIT       (ROUTE_RESULT_NHTYPE_START_BIT - 1)
#define ROUTE_RESULT_PRIORITY_START_BIT     (ROUTE_RESULT_PRIORITY_END_BIT -\
                                              ROUTE_RESULT_PRIORITY_SIZE + 1)
#define ROUTE_RESULT_PRIORITY_SHIFT          ROUTE_RESULT_PRIORITY_START_BIT

#define ROUTE_RESULT_NEXTHOP_END_BIT        (ROUTE_RESULT_PRIORITY_START_BIT - 1)
#define ROUTE_RESULT_NEXTHOP_START_BIT      (ROUTE_RESULT_NEXTHOP_END_BIT -\
                                             ROUTE_RESULT_NEXTHOP_SIZE + 1)
#define ROUTE_RESULT_NEXTHOP_SHIFT           ROUTE_RESULT_NEXTHOP_START_BIT

/*****************************************************************************/
/* policy result computation schemes                                         */
/*****************************************************************************/
#define FW_ACTION_XPOSN_GLOBAL_PRIORTY       0
#define FW_ACTION_XPOSN_ANY_DENY             1

/*****************************************************************************/
/* IP mapping types                                                          */
/*****************************************************************************/
#define MAPPING_TYPE_OVERLAY                 0
#define MAPPING_TYPE_PUBLIC                  1

/*****************************************************************************/
/* number of hints in various HBM hash tables                                */
/*****************************************************************************/
#define P4_LOCAL_MAPPING_NUM_HINTS_PER_ENTRY    8
#define P4_MAPPING_NUM_HINTS_PER_ENTRY          8
#define P4_RXDMA_MAPPING_NUM_HINTS_PER_ENTRY    2

/*****************************************************************************/
/* cpu flags and bit positions                                               */
/*****************************************************************************/
#define APULU_CPU_FLAGS_VLAN_VALID_BIT_POS     0
#define APULU_CPU_FLAGS_IPV4_1_VALID_BIT_POS   1
#define APULU_CPU_FLAGS_IPV6_1_VALID_BIT_POS   2
#define APULU_CPU_FLAGS_ETH_2_VALID_BIT_POS    3
#define APULU_CPU_FLAGS_IPV4_2_VALID_BIT_POS   4
#define APULU_CPU_FLAGS_IPV6_2_VALID_BIT_POS   5

#define APULU_CPU_FLAGS_VLAN_VALID     (1 << APULU_CPU_FLAGS_VLAN_VALID_BIT_POS)
#define APULU_CPU_FLAGS_IPV4_1_VALID   (1 << APULU_CPU_FLAGS_IPV4_1_VALID_BIT_POS)
#define APULU_CPU_FLAGS_IPV6_1_VALID   (1 << APULU_CPU_FLAGS_IPV6_1_VALID_BIT_POS)
#define APULU_CPU_FLAGS_ETH_2_VALID    (1 << APULU_CPU_FLAGS_ETH_2_VALID_BIT_POS)
#define APULU_CPU_FLAGS_IPV4_2_VALID   (1 << APULU_CPU_FLAGS_IPV4_2_VALID_BIT_POS)
#define APULU_CPU_FLAGS_IPV6_2_VALID   (1 << APULU_CPU_FLAGS_IPV6_2_VALID_BIT_POS)

/*****************************************************************************/
/* Header sizes                                                              */
/*****************************************************************************/
#define APULU_P4I_TO_RXDMA_HDR_SZ       52
#define APULU_I2E_HDR_SZ                44
#define APULU_P4_TO_ARM_HDR_SZ          56
#define APULU_ARM_TO_P4_HDR_SZ          11

#define APULU_INGRESS_MIRROR_BLOB_SZ    (APULU_I2E_HDR_SZ)

#define PKTQ_PAGE_SIZE                  10240

/* Qstate definition for packet Q - RxDMA to TxDMA */
#define PKTQ_QSTATE                                                            \
    pc, rsvd, cosA, cosB, cos_sel, eval_last, host_rings, total_rings, pid,    \
    p_index0, c_index0, sw_pindex0, sw_cindex0, ring0_base, ring1_base,        \
    ring_size, rxdma_cindex_addr

#define PKTQ_QSTATE_DVEC_SCRATCH(_scratch_qstate_hdr, _scratch_qstate_txdma_q) \
    modify_field(_scratch_qstate_hdr.pc, pc);                                  \
    modify_field(_scratch_qstate_hdr.rsvd, rsvd);                              \
    modify_field(_scratch_qstate_hdr.cosA, cosA);                              \
    modify_field(_scratch_qstate_hdr.cosB, cosB);                              \
    modify_field(_scratch_qstate_hdr.cos_sel, cos_sel);                        \
    modify_field(_scratch_qstate_hdr.eval_last, eval_last);                    \
    modify_field(_scratch_qstate_hdr.host_rings, host_rings);                  \
    modify_field(_scratch_qstate_hdr.total_rings, total_rings);                \
    modify_field(_scratch_qstate_hdr.pid, pid);                                \
    modify_field(_scratch_qstate_hdr.p_index0, p_index0);                      \
    modify_field(_scratch_qstate_hdr.c_index0, c_index0);                      \
                                                                               \
    modify_field(_scratch_qstate_txdma_q.sw_pindex0, sw_pindex0);              \
    modify_field(_scratch_qstate_txdma_q.sw_cindex0, sw_cindex0);              \
    modify_field(_scratch_qstate_txdma_q.ring0_base, ring0_base);              \
    modify_field(_scratch_qstate_txdma_q.ring1_base, ring1_base);              \
    modify_field(_scratch_qstate_txdma_q.ring_size, ring_size);                \
    modify_field(_scratch_qstate_txdma_q.rxdma_cindex_addr, rxdma_cindex_addr)

/*****************************************************************************/
/* Apulu Pkt memory                                                          */
/*****************************************************************************/
#define APULU_PKT_DESC_SHIFT            7
#define APULU_PKT_DESC_SIZE             (1 << APULU_PKT_DESC_SHIFT)

/*****************************************************************************/
/* Checksum bits from parser (for compiling P4 code only, don't use in ASM   */
/*****************************************************************************/
#define CSUM_HDR_IPV4_1                 0
#define CSUM_HDR_IPV4_2                 0
#define CSUM_HDR_UDP_1                  0
#define CSUM_HDR_UDP_2                  0
#define CSUM_HDR_TCP                    0

/*****************************************************************************/
/* IPSEC types                                                               */
/*****************************************************************************/
#define IPSEC_HEADER_AH                1
#define IPSEC_HEADER_ESP               2
