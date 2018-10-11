/*
 * 	Doorbell write to clear the sched bit for the BSQ having
 *      finished the consumption processing.
 *  Stage 5, Table 0
 */

#include "tls-constants.h"
#include "tls-phv.h"
#include "tls-shared-state.h"
#include "tls-macros.h"
#include "tls-table.h"
#include "tls_common.h"
#include "ingress.h"
#include "INGRESS_p.h"
	

        
struct tx_table_s5_t0_k k                  ;
struct phv_ p	;

%%
	.param      tls_dec_post_read_odesc
        
tls_dec_bsq_consume_process:
    CAPRI_SET_DEBUG_STAGE4_7(p.stats_debug_stage4_7_thread, CAPRI_MPU_STAGE_5, CAPRI_MPU_TABLE_0)
    CAPRI_CLEAR_TABLE0_VALID


    /* For now, if we have a Barco Op error, bail out right here */
    sne     c1, r0, k.tls_global_phv_flags_barco_op_failed
    bcf     [c1], tls_dec_bsq_consume_process_done
    nop

table_read_QUEUE_SESQ:
    add     r1, r0, k.to_s5_odesc
    addi    r1, r1, PKT_DESC_AOL_OFFSET

    CAPRI_NEXT_TABLE_READ(0, TABLE_LOCK_DIS, tls_dec_post_read_odesc,
                          r1, TABLE_SIZE_512_BITS)

tls_dec_bsq_consume_process_done:
	nop.e
	nop
