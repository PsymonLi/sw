#include "../../p4/include/apulu_sacl_defines.h"
#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "ingress.h"

struct read_pktdesc2_k   k;
struct read_pktdesc2_d   d;
struct phv_              p;

%%

read_pktdesc2:
    /* Initialize rx_to_tx_hdr */
    phvwr     p.{rx_to_tx_hdr_sip_classid2, \
                 rx_to_tx_hdr_dip_classid2, \
                 rx_to_tx_hdr_pad_classid2, \
                 rx_to_tx_hdr_sport_classid2, \
                 rx_to_tx_hdr_dport_classid2, \
                 rx_to_tx_hdr_sip_classid3, \
                 rx_to_tx_hdr_dip_classid3, \
                 rx_to_tx_hdr_pad_classid3, \
                 rx_to_tx_hdr_sport_classid3, \
                 rx_to_tx_hdr_dport_classid3, \
                 rx_to_tx_hdr_sip_classid4, \
                 rx_to_tx_hdr_dip_classid4, \
                 rx_to_tx_hdr_pad_classid4, \
                 rx_to_tx_hdr_sport_classid4, \
                 rx_to_tx_hdr_dport_classid4, \
                 rx_to_tx_hdr_sip_classid5, \
                 rx_to_tx_hdr_dip_classid5, \
                 rx_to_tx_hdr_pad_classid5, \
                 rx_to_tx_hdr_sport_classid5, \
                 rx_to_tx_hdr_dport_classid5, \
                 rx_to_tx_hdr_vpc_id, \
                 rx_to_tx_hdr_vnic_id, \
                 rx_to_tx_hdr_iptype, \
                 rx_to_tx_hdr_rx_packet, \
                 rx_to_tx_hdr_payload_len, \
                 rx_to_tx_hdr_stag0_classid0, \
                 rx_to_tx_hdr_stag1_classid0, \
                 rx_to_tx_hdr_stag2_classid0, \
                 rx_to_tx_hdr_stag3_classid0, \
                 rx_to_tx_hdr_stag4_classid0, \
                 rx_to_tx_hdr_dtag0_classid0, \
                 rx_to_tx_hdr_dtag1_classid0, \
                 rx_to_tx_hdr_dtag2_classid0, \
                 rx_to_tx_hdr_dtag3_classid0, \
                 rx_to_tx_hdr_dtag4_classid0, \
                 rx_to_tx_hdr_stag0_classid1, \
                 rx_to_tx_hdr_stag1_classid1, \
                 rx_to_tx_hdr_stag2_classid1, \
                 rx_to_tx_hdr_stag3_classid1, \
                 rx_to_tx_hdr_stag4_classid1, \
                 rx_to_tx_hdr_dtag0_classid1, \
                 rx_to_tx_hdr_dtag1_classid1, \
                 rx_to_tx_hdr_dtag2_classid1, \
                 rx_to_tx_hdr_dtag3_classid1, \
                 rx_to_tx_hdr_dtag4_classid1, \
                 rx_to_tx_hdr_stag0_classid2, \
                 rx_to_tx_hdr_stag1_classid2, \
                 rx_to_tx_hdr_stag2_classid2, \
                 rx_to_tx_hdr_stag3_classid2, \
                 rx_to_tx_hdr_stag4_classid2, \
                 rx_to_tx_hdr_dtag0_classid2, \
                 rx_to_tx_hdr_dtag1_classid2, \
                 rx_to_tx_hdr_dtag2_classid2, \
                 rx_to_tx_hdr_dtag3_classid2, \
                 rx_to_tx_hdr_dtag4_classid2, \
                 rx_to_tx_hdr_pad2}, \
                 d[511:0]

    // Copy the first set of TAGs
    phvwr.e    p.txdma_control_stag_classid, d.read_pktdesc2_d.stag0_classid0
    phvwr      p.txdma_control_dtag_classid, d.read_pktdesc2_d.dtag0_classid0

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
read_pktdesc2_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
