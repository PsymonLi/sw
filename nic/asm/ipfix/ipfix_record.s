#include "ipfix_defines.h"
#include "nic/hal/iris/datapath/p4/include/defines.h"
#include "ipfix/asm_out/INGRESS_p.h"
#include "ipfix/asm_out/ingress.h"
#include "../common-p4+/include/capri-macros.h"

struct ipfix_create_record_k k;
struct ipfix_create_record_d d;
struct phv_                  p;

%%

ipfix_create_record:
    // check if there is space available to fit a record
    add         r7, d.{u.ipfix_create_record_d.next_record_offset}.hx, \
                    IPFIX_MAX_RECORD_SIZE
    sle         c1, r7, d.{u.ipfix_create_record_d.pktsize}.hx
    bcf         [!c1], ipfix_record_exit

    // setup dma cmd ptr
    phvwr.c1    p.p4_txdma_intr_dma_cmd_ptr, \
                    CAPRI_PHV_START_OFFSET(phv2mem_cmd1_dma_cmd_pad) / 16

    bbeq        k.ipfix_metadata_scan_complete, TRUE, ipfix_header_fixups

    // setup record start address
    add         r1, d.{u.ipfix_create_record_d.pktaddr}.dx, \
                    d.{u.ipfix_create_record_d.next_record_offset}.hx

    seq         c1, k.ipfix_metadata_flow_type, FLOW_KEY_LOOKUP_TYPE_IPV4
    bcf         [c1], ipfix_create_ipv4_record
    seq         c1, k.ipfix_metadata_flow_type, FLOW_KEY_LOOKUP_TYPE_IPV6
    bcf         [c1], ipfix_create_ipv6_record

ipfix_create_non_ip_record:
    phvwr.!c1   p.ipfix_record_nonip_set_id, IPFIX_NON_IP_RECORD_ID
    phvwr       p.ipfix_record_nonip_len, IPFIX_NON_IP_RECORD_SIZE

    phvwr       p.phv2mem_cmd1_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwr       p.phv2mem_cmd1_dma_cmd_phv_start_addr, \
                    CAPRI_PHV_START_OFFSET(ipfix_record_nonip_set_id)
    phvwr       p.phv2mem_cmd1_dma_cmd_phv_end_addr, \
                    CAPRI_PHV_END_OFFSET(ipfix_record_nonip_ether_type)
    phvwr       p.phv2mem_cmd1_dma_cmd_addr, r1
    add         r1, r1, (CAPRI_PHV_END_OFFSET(ipfix_record_nonip_ether_type) - \
                         CAPRI_PHV_START_OFFSET(ipfix_record_nonip_set_id) + 1)

    phvwr       p.phv2mem_cmd2_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwr       p.phv2mem_cmd2_dma_cmd_phv_start_addr, \
                    CAPRI_PHV_START_OFFSET(ipfix_record_common_flow_id)
    phvwr       p.phv2mem_cmd2_dma_cmd_phv_end_addr, \
                    CAPRI_PHV_END_OFFSET(ipfix_record_common_drop_vector)
    phvwr       p.phv2mem_cmd2_dma_cmd_addr, r1

    b           ipfix_header_fixups
    tbladd      d.{u.ipfix_create_record_d.next_record_offset}.hx, \
                    IPFIX_NON_IP_RECORD_SIZE

ipfix_create_ipv4_record:
    phvwr       p.ipfix_record_ipv4_set_id, IPFIX_IPv4_RECORD_ID
    phvwr       p.ipfix_record_ipv4_len, IPFIX_IPv4_RECORD_SIZE

    phvwr       p.phv2mem_cmd1_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwr       p.phv2mem_cmd1_dma_cmd_phv_start_addr, \
                    CAPRI_PHV_START_OFFSET(ipfix_record_ipv4_set_id)
    phvwr       p.phv2mem_cmd1_dma_cmd_phv_end_addr, \
                    CAPRI_PHV_END_OFFSET(ipfix_record_ipv4_dport)
    phvwr       p.phv2mem_cmd1_dma_cmd_addr, r1
    add         r1, r1, (CAPRI_PHV_END_OFFSET(ipfix_record_ipv4_dport) - \
                         CAPRI_PHV_START_OFFSET(ipfix_record_ipv4_set_id) + 1)

    phvwr       p.phv2mem_cmd2_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwr       p.phv2mem_cmd2_dma_cmd_phv_start_addr, \
                    CAPRI_PHV_START_OFFSET(ipfix_record_ip_flow_state_index)
    phvwr       p.phv2mem_cmd2_dma_cmd_phv_end_addr, \
                    CAPRI_PHV_END_OFFSET(ipfix_record_common_drop_vector)
    phvwr       p.phv2mem_cmd2_dma_cmd_addr, r1

    b           ipfix_header_fixups
    tbladd      d.{u.ipfix_create_record_d.next_record_offset}.hx, \
                    IPFIX_IPv4_RECORD_SIZE

ipfix_create_ipv6_record:
    phvwr       p.ipfix_record_ipv6_set_id, IPFIX_IPv6_RECORD_ID
    phvwr       p.ipfix_record_ipv6_len, IPFIX_IPv6_RECORD_SIZE

    phvwr       p.phv2mem_cmd1_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwr       p.phv2mem_cmd1_dma_cmd_phv_start_addr, \
                    CAPRI_PHV_START_OFFSET(ipfix_record_ipv6_set_id)
    phvwr       p.phv2mem_cmd1_dma_cmd_phv_end_addr, \
                    CAPRI_PHV_END_OFFSET(ipfix_record_ipv6_dport)
    phvwr       p.phv2mem_cmd1_dma_cmd_addr, r1
    add         r1, r1, (CAPRI_PHV_END_OFFSET(ipfix_record_ipv6_dport) - \
                         CAPRI_PHV_START_OFFSET(ipfix_record_ipv6_set_id) + 1)

    phvwr       p.phv2mem_cmd2_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwr       p.phv2mem_cmd2_dma_cmd_phv_start_addr, \
                    CAPRI_PHV_START_OFFSET(ipfix_record_ip_flow_state_index)
    phvwr       p.phv2mem_cmd2_dma_cmd_phv_end_addr, \
                    CAPRI_PHV_END_OFFSET(ipfix_record_common_drop_vector)
    phvwr       p.phv2mem_cmd2_dma_cmd_addr, r1

    tbladd      d.{u.ipfix_create_record_d.next_record_offset}.hx, \
                    IPFIX_IPv6_RECORD_SIZE

ipfix_header_fixups:
    // if scan is not complete and there is space for another record,
    // don't flush the packet out
    seq         c1, k.ipfix_metadata_scan_complete, FALSE
    cmov        r6, c1, IPFIX_SCAN_INCOMPLETE, IPFIX_SCAN_COMPLETE
    add         r7, d.{u.ipfix_create_record_d.next_record_offset}.hx, \
                    IPFIX_MAX_RECORD_SIZE
    sle.c1      c1, r7, d.{u.ipfix_create_record_d.pktsize}.hx
    bcf         [c1], ipfix_record_done
    phvwr.c1    p.phv2mem_cmd2_dma_cmd_eop, 1

    // IPFIX header fixups before sending the packet out
    phvwr       p.ipfix_record_header_version, IPFIX_VERSION
    sub         r1, d.{u.ipfix_create_record_d.next_record_offset}.hx, \
                    d.{u.ipfix_create_record_d.ipfix_hdr_offset}.hx
    phvwr       p.ipfix_record_header_len, r1
    phvwr       p.ipfix_record_header_export_time, 0
    phvwr       p.ipfix_record_header_seq_num, \
                    d.{u.ipfix_create_record_d.seq_no}.wx
    phvwr       p.ipfix_record_header_domain_id, \
                    d.{u.ipfix_create_record_d.domain_id}.wx

    phvwr       p.phv2mem_cmd3_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwr       p.phv2mem_cmd3_dma_cmd_phv_start_addr, \
                    CAPRI_PHV_START_OFFSET(ipfix_record_header_version)
    phvwr       p.phv2mem_cmd3_dma_cmd_phv_end_addr, \
                    CAPRI_PHV_END_OFFSET(ipfix_record_header_domain_id)
    add         r1, d.{u.ipfix_create_record_d.pktaddr}.dx, \
                    d.{u.ipfix_create_record_d.ipfix_hdr_offset}.hx
    phvwr       p.phv2mem_cmd3_dma_cmd_addr, r1

    // increment sequence number
    tbladd      d.{u.ipfix_create_record_d.seq_no}.wx, 1

    // disable doorbell (self)
    addi        r1, r0, DB_ADDR_BASE
    or          r1, r1, 0x2, DB_UPD_SHFT
    or          r1, r1, 1005, DB_LIF_SHFT
    add         r2, r0, k.ipfix_metadata_export_id, 24
    phvwr       p.ipfix_s5_metadata_doorbell1_data, r2.dx
    phvwr       p.phv2mem_cmd4_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwr       p.phv2mem_cmd4_dma_cmd_phv_start_addr, \
                    CAPRI_PHV_START_OFFSET(ipfix_s5_metadata_doorbell1_data)
    phvwr       p.phv2mem_cmd4_dma_cmd_phv_end_addr, \
                    CAPRI_PHV_END_OFFSET(ipfix_s5_metadata_doorbell1_data)
    phvwr       p.phv2mem_cmd4_dma_cmd_addr, r1

    // update table type and next index in qstate of (self+16)
    phvwr       p.ipfix_t0_metadata_next_record_offset, r6
    add         r1, k.{ipfix_metadata_qstate_addr_sbit0_ebit1,\
                       ipfix_metadata_qstate_addr_sbit2_ebit33}, ((64*16)+32)
    phvwr       p.phv2mem_cmd5_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwr       p.phv2mem_cmd5_dma_cmd_phv_start_addr, \
                    CAPRI_PHV_START_OFFSET(ipfix_t0_metadata_next_record_offset)
    phvwr       p.phv2mem_cmd5_dma_cmd_phv_end_addr, \
                    CAPRI_PHV_END_OFFSET(ipfix_t0_metadata_flow_hash_index_next)
    phvwr       p.phv2mem_cmd5_dma_cmd_addr, r1

    // ring doorbell (self+16)
    addi        r1, r0, DB_ADDR_BASE
    or          r1, r1, 0x3, DB_UPD_SHFT
    or          r1, r1, 1005, DB_LIF_SHFT
    add         r2, k.ipfix_metadata_export_id, IPFIX_EXPORT_ID_MAX
    add         r2, r0, r2, 24
    phvwr       p.ipfix_s5_metadata_doorbell2_data, r2.dx
    phvwr       p.phv2mem_cmd6_dma_cmd_type, CAPRI_DMA_COMMAND_PHV_TO_MEM
    phvwr       p.phv2mem_cmd6_dma_cmd_phv_start_addr, \
                    CAPRI_PHV_START_OFFSET(ipfix_s5_metadata_doorbell2_data)
    phvwr       p.phv2mem_cmd6_dma_cmd_phv_end_addr, \
                    CAPRI_PHV_END_OFFSET(ipfix_s5_metadata_doorbell2_data)
    phvwr       p.phv2mem_cmd6_dma_cmd_addr, r1
    phvwr       p.phv2mem_cmd6_dma_cmd_wr_fence, 1
    phvwr       p.phv2mem_cmd6_dma_cmd_eop, 1

ipfix_record_done:
    phvwr.e     p.{app_header_table0_valid...app_header_table3_valid}, 0
    nop

ipfix_record_exit:
    phvwr.e     p.{app_header_table0_valid...app_header_table3_valid}, 0
    phvwr       p.p4_intr_global_drop, 1
