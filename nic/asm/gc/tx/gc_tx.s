#include "INGRESS_p.h"
#include "ingress.h"
#include "capri-macros.h"

struct phv_ p;
struct gc_tx_initial_action_k k;
struct gc_tx_initial_action_initial_action_d d;

%%
    .param          gc_tx_read_rnmdr_addr
    .param          gc_tx_read_tnmdr_addr

/*
 * Each ring (RNMDR, TNMDR etc.) that needs to be garbage collected, gets its
 * own QID, and hence its own start offset.
 *
 * Each QID, can contain upto 8 rings.
 * Each ring stands for a producer (TCP, TLS, IPSEC etc.) producing into the GC
 * ring
 */

.align
gc_tx_rnmdr_s0_start:
    .brbegin
        br          r7[0:0]
        nop
            .brcase CAPRI_RNMDR_GC_TCP_RING_PRODUCER
                b gc_tx_handle_rnmdr_tcp
                nop
            .brcase CAPRI_RNMDR_GC_TCP_RING_PRODUCER + 1
                b gc_tx_handle_rnmdr_tcp
                nop
    .brend


.align
gc_tx_tnmdr_s0_start:
    .brbegin
        br          r7[0:0]
        nop
            .brcase CAPRI_TNMDR_GC_TCP_RING_PRODUCER
                b gc_tx_handle_tnmdr_tcp
                nop
            .brcase CAPRI_TNMDR_GC_TCP_RING_PRODUCER + 1
                b gc_tx_handle_tnmdr_tcp
                nop
    .brend


/*
 * TCP freeing a descriptor into RNMDR
 */
gc_tx_handle_rnmdr_tcp:
    CAPRI_OPERAND_DEBUG(d.ring_base)
    CAPRI_OPERAND_DEBUG(d.ring_shift)
    /*
     * Ring 0 is the ring that TCP writes to when freeing a descriptor
     *
     * r5 = Address to read from descriptor GC ring
     * 
     * r4 = doorbell address
     * r3 = doorbell data
     */
    add             r5, d.{ring_base}.dx, RNMDR_GC_PRODUCER_TCP, RNMDR_GC_PER_PRODUCER_SHIFT
    add             r5, r5, d.{ci_0}.hx, RNMDR_TABLE_ENTRY_SIZE_SHFT
    tbladd          d.{ci_0}.hx, 1
    addi            r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_CIDX_SET, DB_SCHED_UPD_EVAL, 0, LIF_GC)
    /* data will be in r3 */
    CAPRI_RING_DOORBELL_DATA(0, CAPRI_HBM_GC_RNMDR_QID, CAPRI_RNMDR_GC_TCP_RING_PRODUCER, d.{ci_0}.hx)
    memwr.dx        r4, r3
    CAPRI_NEXT_TABLE_READ(0, TABLE_LOCK_DIS, gc_tx_read_rnmdr_addr, r5, TABLE_SIZE_64_BITS)
    nop.e
    nop

/*
 * TCP freeing a descriptor into TNMDR
 */
gc_tx_handle_tnmdr_tcp:
    CAPRI_OPERAND_DEBUG(d.ring_base)
    CAPRI_OPERAND_DEBUG(d.ring_shift)
    /*
     * Ring 0 is the ring that TCP writes to when freeing a descriptor
     *
     * r5 = Address to read from descriptor GC ring
     * 
     * r4 = doorbell address
     * r3 = doorbell data
     */
    add             r5, d.{ring_base}.dx, TNMDR_GC_PRODUCER_TCP, TNMDR_GC_PER_PRODUCER_SHIFT
    add             r5, r5, d.{ci_0}.hx, TNMDR_TABLE_ENTRY_SIZE_SHFT
    tbladd          d.{ci_0}.hx, 1
    addi            r4, r0, CAPRI_DOORBELL_ADDR(0, DB_IDX_UPD_CIDX_SET, DB_SCHED_UPD_EVAL, 0, LIF_GC)
    /* data will be in r3 */
    CAPRI_RING_DOORBELL_DATA(0, CAPRI_HBM_GC_TNMDR_QID, CAPRI_TNMDR_GC_TCP_RING_PRODUCER, d.{ci_0}.hx)
    memwr.dx        r4, r3
    add             r3, d.{ring_base}.dx, TNMDR_GC_PRODUCER_TCP, TNMDR_GC_PER_PRODUCER_SHIFT
    CAPRI_NEXT_TABLE_READ(0, TABLE_LOCK_DIS, gc_tx_read_tnmdr_addr, r5, TABLE_SIZE_64_BITS)
    nop.e
    nop
