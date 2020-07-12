#include "apulu.h"
#include "ingress.h"
#include "INGRESS_p.h"
#include "INGRESS_local_mapping_tag_k.h"

struct local_mapping_tag_k_ k;
struct local_mapping_tag_d  d;
struct phv_                 p;

%%

local_mapping_tag_info:
    seq             c1, k.p4_to_rxdma_rx_packet, FALSE
    phvwr.c1        p.{lpm_metadata_stag0...lpm_metadata_stag4},\
                        d.{local_mapping_tag_info_d.tag0...local_mapping_tag_info_d.tag4}
    nop.e
    phvwr.!c1       p.{lpm_metadata_dtag0...lpm_metadata_dtag4},\
                        d.{local_mapping_tag_info_d.tag0...local_mapping_tag_info_d.tag4}

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
local_mapping_tag_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
