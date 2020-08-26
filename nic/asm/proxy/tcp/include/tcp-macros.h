#ifndef _TCP_MACROS_H_
#define _TCP_MACROS_H_

#include "capri-macros.h"
#include "cpu-macros.h"
#include "tcp_common.h"

//XXXX To be Enabled after ACTL-Q verification in Apollo
#if !(defined(APOLLO) || defined(ARTEMIS))
#define TCP_RXPKT_2_ACTL_Q
#endif

#define TCP_NEXT_TABLE_READ  CAPRI_NEXT_TABLE_READ

#define TCP_END_CWND_REDUCTION                                          \
        sne		c1, d.ca_state, TCP_CA_CWR;			\
	seq		c2, d.undo_marker, r0;		                \
	addi		r1, r0,TCP_INFINITE_SSTHRESH;	                \
	slt		c3, r1, d.snd_ssthresh;				\
	setcf		c4, [c1 & c2 & c3];                             \
	phvwr.!c4	p.rx2tx_snd_wnd, d.snd_ssthresh;	        \
    //phvwri		p.common_phv_ca_event, CA_EVENT_COMPLETE_CWR;



#define TCP_READ_IDX CAPRI_READ_IDX

#define TCP_ACTL_Q_BASE          tcp_actl_q_base

// MUST match CAPRI_TCP_ACTL_Q_RING_SIZE
#define TCP_ACTL_Q_TABLE_SIZE             (1 << TCP_ACTL_Q_TABLE_SHIFT)
#define TCP_ACTL_Q_TABLE_SHIFT            14

#define TCP_ACTL_Q_ENTRY_SIZE_SHIFT       3          /* for 8B */
#define TCP_ACTL_Q_ENTRY_SIZE             (1 << TCP_ACTL_Q_ENTRY_SIZE_SHIFT)

#define TCP_ACTL_Q_SEM_ENTRY_SHIFT        3

#define TCP_ACTL_Q_BASE_FOR_ID(_dest_r, \
                                _tcp_actl_q_base, \
                                _k_cpu_id) \
    add    _dest_r, r0, _k_cpu_id; \
    sll    _dest_r, _dest_r, TCP_ACTL_Q_TABLE_SHIFT; \
    sll    _dest_r, _dest_r, TCP_ACTL_Q_ENTRY_SIZE_SHIFT; \
    add    _dest_r, _dest_r, _tcp_actl_q_base

#define CPU_TCP_ACTL_Q_SEM_INF_ADDR(_k_cpu_id, _dest_r) \
    addi   _dest_r, r0, ASIC_SEM_TCP_ACTL_Q_0_ADDR;                \
    add    _dest_r, _dest_r, _k_cpu_id, TCP_ACTL_Q_SEM_ENTRY_SHIFT; \
    addi   _dest_r, _dest_r, ASIC_SEM_INF_OFFSET;

#define TCP_ACTL_Q_ENQUEUE(_dest_r, \
                       _ring_elem, \
                       _actl_q_pindex, \
                       _actl_q_base, \
                       _phv_desc_sfield_name, \
                       _phv_desc_efield_name, \
                       _dma_cmd_prefix, \
                       _eop_, \
                       _wr_fence_, \
                       _debug_dol_cr) \
    andi    _dest_r, _actl_q_pindex, ((1 << TCP_ACTL_Q_TABLE_SHIFT) - 1);  \
    add     _dest_r, _actl_q_base, _dest_r, TCP_ACTL_Q_ENTRY_SIZE_SHIFT;   \
    phvwri  p.##_dma_cmd_prefix##_type, CAPRI_DMA_COMMAND_PHV_TO_MEM;   \
    phvwr   p.##_dma_cmd_prefix##_addr, _dest_r;                          \
    andi    _dest_r, _actl_q_pindex, (1 << TCP_ACTL_Q_TABLE_SHIFT);          \
    add     _dest_r, _ring_elem, _dest_r, (CPU_VALID_BIT_SHIFT - TCP_ACTL_Q_TABLE_SHIFT); \
    phvwr   p._phv_desc_efield_name, _dest_r;                            \
    phvwri  p.##_dma_cmd_prefix##_phv_start_addr, CAPRI_PHV_START_OFFSET(_phv_desc_sfield_name); \
    phvwri  p.##_dma_cmd_prefix##_phv_end_addr, CAPRI_PHV_END_OFFSET(_phv_desc_efield_name); \
    phvwri  p.##_dma_cmd_prefix##_eop, _eop_;                           \
    phvwri  p.##_dma_cmd_prefix##_wr_fence, _wr_fence_

#define TCP_ACTL_MSG_TYPE_PKT               0
#define TCP_ACTL_MSG_TYPE_CLEANUP           1
#define TCP_ACTL_MSG_TYPE_SHIFT             0
#define TCP_ACTL_MSG_CLOSE_REASON_SHIFT     24
#define TCP_ACTL_MSG_QID_SHIFT              32
#endif /* #ifndef _TCP_MACROS_H_ */
