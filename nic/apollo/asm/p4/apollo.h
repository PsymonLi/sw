#include "../../p4/include/defines.h"
#include "../../p4/include/sacl_defines.h"

#define ASM_INSTRUCTION_OFFSET_MAX     (64 * 256)

// Uses c1, c2, r7
#define LOCAL_VNIC_INFO_COMMON_END(local_vnic_tag, vcn_id, skip_src_dst_check, \
                                   resource_group_1, lpm_v4addr_1,             \
                                   lpm_v6addr_1, sacl_v4addr_1, sacl_v6addr_1, \
                                   epoch1, resource_group_2, lpm_v4addr_2,     \
                                   lpm_v6addr_2, sacl_v4addr_2, sacl_v6addr_2, \
                                   epoch2)                                     \
    phvwr           p.vnic_metadata_local_vnic_tag, local_vnic_tag;            \
    phvwr           p.vnic_metadata_skip_src_dst_check, skip_src_dst_check;    \
    /* c2 will be set if using epoch1, else will be reset */;                  \
    seq             c1, k.service_header_valid, TRUE;                          \
    seq.c1          c2, k.service_header_epoch, epoch1;                        \
    sle.!c1         c2, epoch2, epoch1;                                        \
    bcf             [!c2], __use_epoch2;                                       \
    phvwr           p.vnic_metadata_vcn_id, vcn_id;                            \
__use_epoch1:;                                                                 \
    phvwr           p.policer_metadata_resource_group, resource_group_1;       \
    phvwr           p.p4_to_txdma_header_lpm_addr, lpm_v4addr_1;               \
    phvwr           p.p4_to_rxdma_header_sacl_base_addr, sacl_v4addr_1;        \
    phvwr           p.control_metadata_lpm_v6addr, lpm_v6addr_1;               \
    phvwr.e         p.control_metadata_sacl_v6addr, sacl_v6addr_1;             \
    phvwr           p.service_header_epoch, epoch1;                            \
__use_epoch2: ;                                                                \
    phvwr           p.policer_metadata_resource_group, resource_group_2;       \
    phvwr           p.p4_to_txdma_header_lpm_addr, lpm_v4addr_2;               \
    phvwr           p.p4_to_rxdma_header_sacl_base_addr, sacl_v4addr_2;        \
    phvwr           p.control_metadata_lpm_v6addr, lpm_v6addr_2;               \
    phvwr.e         p.control_metadata_sacl_v6addr, sacl_v6addr_2;             \
    phvwr           p.service_header_epoch, epoch2;
