/*
 *	Implements the tnmdr ring desc of the TxDMA P4+ tls  pipeline
 */

#include "tls-phv.h"
#include "tls-shared-state.h"
#include "tls-macros.h"
#include "tls-table.h"
#include "ingress.h"
#include "INGRESS_p.h"

 /* d is the data returned by lookup result */
struct d_struct {
	odesc			 : DESC_ADDRESS_WIDTH ;
    pad              : 448               ;
};

/* Readonly Parsed packet header info for the current packet */
struct tx_table_s2_t2_k                 k;

struct phv_ p;
struct d_struct d;

%%
	
        .param          tls_queue_brq_start
        .param          TNMDR_TABLE_BASE
	    .align
tls_tdesc_alloc_start:

        CAPRI_CLEAR_TABLE2_VALID

	    phvwr		p.to_s5_odesc, d.odesc
        
	    nop.e
	    nop

