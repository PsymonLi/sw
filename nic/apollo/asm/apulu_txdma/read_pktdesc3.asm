#include "../../p4/include/apulu_sacl_defines.h"
#include "nic/apollo/asm/include/apulu.h"
#include "INGRESS_p.h"
#include "ingress.h"

struct read_pktdesc3_k   k;
struct read_pktdesc3_d   d;
struct phv_              p;

%%

read_pktdesc3:
    /* Initialize rx_to_tx_hdr */
    phvwr.e    p.{rx_to_tx_hdr_stag0_classid3, \
                  rx_to_tx_hdr_stag1_classid3, \
                  rx_to_tx_hdr_stag2_classid3, \
                  rx_to_tx_hdr_stag3_classid3, \
                  rx_to_tx_hdr_stag4_classid3, \
                  rx_to_tx_hdr_dtag0_classid3, \
                  rx_to_tx_hdr_dtag1_classid3, \
                  rx_to_tx_hdr_dtag2_classid3, \
                  rx_to_tx_hdr_dtag3_classid3, \
                  rx_to_tx_hdr_dtag4_classid3, \
                  rx_to_tx_hdr_stag0_classid4, \
                  rx_to_tx_hdr_stag1_classid4, \
                  rx_to_tx_hdr_stag2_classid4, \
                  rx_to_tx_hdr_stag3_classid4, \
                  rx_to_tx_hdr_stag4_classid4, \
                  rx_to_tx_hdr_dtag0_classid4, \
                  rx_to_tx_hdr_dtag1_classid4, \
                  rx_to_tx_hdr_dtag2_classid4, \
                  rx_to_tx_hdr_dtag3_classid4, \
                  rx_to_tx_hdr_dtag4_classid4, \
                  rx_to_tx_hdr_stag0_classid5, \
                  rx_to_tx_hdr_stag1_classid5, \
                  rx_to_tx_hdr_stag2_classid5, \
                  rx_to_tx_hdr_stag3_classid5, \
                  rx_to_tx_hdr_stag4_classid5, \
                  rx_to_tx_hdr_dtag0_classid5, \
                  rx_to_tx_hdr_dtag1_classid5, \
                  rx_to_tx_hdr_dtag2_classid5, \
                  rx_to_tx_hdr_dtag3_classid5, \
                  rx_to_tx_hdr_dtag4_classid5, \
                  rx_to_tx_hdr_pad_end}, \
               d[511:(512-(offsetof(p,rx_to_tx_hdr_stag0_classid3) + \
                   sizeof(p.rx_to_tx_hdr_stag0_classid3) - \
                   offsetof(p, rx_to_tx_hdr_pad_end)))]
    nop

/*****************************************************************************/
/* error function                                                            */
/*****************************************************************************/
.align
.assert $ < ASM_INSTRUCTION_OFFSET_MAX
read_pktdesc3_error:
    phvwr.e         p.capri_intr_drop, 1
    nop
