#include "apollo.h"
#include "ingress.h"
#include "INGRESS_p.h"

struct ingress_to_rxdma_k k;
struct phv_ p;

%%

ingress_to_rxdma:
    seq             c1, k.service_header_local_ip_mapping_done, FALSE
    seq.!c1         c1, k.service_header_flow_done, FALSE
    bcf             [c1], recirc_packet
    phvwr.!c1       p.capri_intrinsic_tm_oport, TM_PORT_DMA
    phvwr           p.capri_intrinsic_lif, APOLLO_SERVICE_LIF
    phvwr           p.capri_rxdma_intrinsic_rx_splitter_offset, \
                        (CAPRI_GLOBAL_INTRINSIC_HDR_SZ + \
                         CAPRI_RXDMA_INTRINSIC_HDR_SZ + \
                         APOLLO_P4_TO_ARM_HDR_SZ + \
                         APOLLO_P4_TO_RXDMA_HDR_SZ)
    phvwr           p.capri_p4_intrinsic_valid, TRUE
    phvwr           p.capri_rxdma_intrinsic_valid, TRUE
    phvwr           p.p4_to_arm_header_valid, TRUE
    phvwr           p.p4_to_rxdma_header_valid, TRUE
    phvwr           p.predicate_header_valid, TRUE
    phvwr           p.p4_to_txdma_header_valid, TRUE
    phvwr           p.p4i_apollo_i2e_valid, TRUE
    add             r1, k.{capri_p4_intrinsic_packet_len_sbit0_ebit5,\
                        capri_p4_intrinsic_packet_len_sbit6_ebit13}, \
                        APOLLO_I2E_HDR_SZ
    phvwr           p.p4_to_rxdma_header_table3_valid, TRUE
    phvwr           p.p4_to_rxdma_header_direction, k.control_metadata_direction
    phvwr           p.p4_to_txdma_header_payload_len, r1
    seq             c1, k.control_metadata_direction, RX_FROM_SWITCH
    phvwr.c1        p.predicate_header_lpm_bypass, TRUE
    phvwr.e         p.service_header_valid, FALSE
    phvwr           p.predicate_header_direction, k.control_metadata_direction

recirc_packet:
    phvwr.e         p.capri_intrinsic_tm_oport, TM_PORT_INGRESS
    phvwr           p.service_header_valid, TRUE

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
ingress_to_rxdma_error:
    phvwr.e         p.capri_intrinsic_drop, 1
    nop
