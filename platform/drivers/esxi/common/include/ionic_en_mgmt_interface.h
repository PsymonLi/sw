/*
 * Copyright (c) 2020 Pensando Systems. All rights reserved.
 */

/*
 * ionic_en_mgmt_interface.h --
 *
 * Definitions for channel between user/kernel space 
 */

#ifndef _IONIC_EN_MGMT_INTERFACE_H_
#define _IONIC_EN_MGMT_INTERFACE_H_

#include <vmkapi.h>

/*************************************************************************************/
#define IONIC_EN_MGMT_INTERFACE_VSPHERE_VER_2016 VMK_REVISION_FROM_NUMBERS(2, 4, 0, 0)
#define IONIC_EN_MGMT_INTERFACE_VSPHERE_VER_2017 VMK_REVISION_FROM_NUMBERS(2, 5, 0, 0)
#define IONIC_EN_MGMT_INTERFACE_VSPHERE_VER_2020 VMK_REVISION_FROM_NUMBERS(2, 6, 0, 0)
/*************************************************************************************/

#define IONIC_EN_MGMT_INTERFACE_NAME            "ionic_en_mgmt_interface"
#define IONIC_EN_MGMT_INTERFACE_VENDOR          "Pensando Systems"

typedef enum ionic_en_mgmt_interface_cbs {
        IONIC_EN_MGMT_INTERFACE_CB_COUNT_NUM_DSC = (VMK_MGMT_RESERVED_CALLBACKS + 1),
        IONIC_EN_MGMT_INTERFACE_CB_GET_ADPT_INFO = (VMK_MGMT_RESERVED_CALLBACKS + 2),
        IONIC_EN_MGMT_INTERFACE_CB_FLASH_FW = (VMK_MGMT_RESERVED_CALLBACKS + 3),
        IONIC_EN_MGMT_INTERFACE_CB_MAX,
} ionic_en_mgmt_interface_cbs;

#define IONIC_EN_MGMT_INTERFACE_CB_NUM \
        (IONIC_EN_MGMT_INTERFACE_CB_MAX - IONIC_EN_MGMT_INTERFACE_CB_COUNT_NUM_DSC)

#define IONIC_EN_ADPT_BRANDING_NAME_MAX_LEN        8 
#define IONIC_EN_FW_TYPE_MAX_LEN                 256
#define IONIC_EN_FW_FILE_NAME_MAX_LEN           1024

typedef struct ionic_en_adapter_info {
        char adapter_branding_name[IONIC_EN_ADPT_BRANDING_NAME_MAX_LEN];
        vmk_Name ethernet_interface_name;

        // PCID INFO
        vmk_uint32 ven_id;
        vmk_uint32 dev_id;
        vmk_uint32 sub_ven_id;
        vmk_uint32 sub_dev_id;

        // BUS INFO
        vmk_uint32 segment;
        vmk_uint32 bus_number;
        vmk_uint32 device_number;
        vmk_uint32 func_number;
        char slot_number[16];
        char mac_address[20];

        // Independent FW Count
        vmk_uint32 n_fw;
        vmk_Name fw_ver;

        vmk_Bool is_used;

        // Configuration flags reserved for HPE
        vmk_int32 config_flags;
} __attribute__((__packed__)) ionic_en_adapter_info;


#ifdef VMKERNEL
VMK_ReturnStatus
ionic_en_mgmt_inf_count_num_dsc_cb(vmk_MgmtCookies *cookies,
                                   vmk_MgmtEnvelope *envelope,
                                   vmk_uint32 *num_dscs);
                                   
VMK_ReturnStatus
ionic_en_mgmt_inf_get_adpt_info_cb(vmk_MgmtCookies *cookies,
                                   vmk_MgmtEnvelope *envelope,
                                   vmk_MgmtVectorParm *adapter_list);
VMK_ReturnStatus
ionic_en_mgmt_inf_flush_fw_cb(vmk_MgmtCookies *cookies,
                              vmk_MgmtEnvelope *envelope,
                              vmk_uint64 *fw_img_name_addr,
                              vmk_uint64 *fw_img_data_addr,
                              vmk_uint64 *fw_img_size,
                              vmk_uint64 *uplink_name_addr);

#else
#define ionic_en_mgmt_inf_count_num_dsc_cb              NULL
#define ionic_en_mgmt_inf_get_adpt_info_cb              NULL
#define ionic_en_mgmt_inf_flush_fw_cb                   NULL
#endif  /* End of VMKERNEL */

#endif  /* End of _IONIC_EN_MGMT_INTERFACE_H_ */
