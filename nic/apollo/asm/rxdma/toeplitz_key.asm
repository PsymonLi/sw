#include "apollo_rxdma.h"
#include "INGRESS_p.h"
#include "ingress.h"
#include "INGRESS_toeplitz_key_k.h"

struct toeplitz_key_k_      k;
struct phv_                 p;

%%

toeplitz_key_init:
    // copy key fields from packet
    // key (320bits) is constructed from msb to lsb as -
    // key0: flow_src
    // key1: flow_dst
    // key2: sport, dport, proto (whichever order provided by key-maker)
    phvwr           p.toeplitz_key0_data, k.p4_to_rxdma_header_flow_src
    phvwr           p.toeplitz_key1_data, k.p4_to_rxdma_header_flow_dst
    add             r1, k.p4_to_rxdma_header_flow_sport, \
                        k.p4_to_rxdma_header_flow_dport, 16
    add.e           r1, r1, k.p4_to_rxdma_header_flow_proto, 32
    phvwr           p.toeplitz_key2_data[63:24], r1

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
toeplitz_key_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
