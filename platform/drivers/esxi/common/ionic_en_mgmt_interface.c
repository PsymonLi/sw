/*
 * Copyright (c) 2020 Pensando Systems. All rights reserved.
 */

/*
 * ionic_en_mgmt_interface.c --
 *
 * Functions used for channel between user/kernel space 
 */

#include "include/ionic_en_mgmt_interface.h"

vmk_MgmtCallbackInfo ionic_en_mgmt_cbs[IONIC_EN_MGMT_INTERFACE_CB_NUM] = {
        {
                .location = VMK_MGMT_CALLBACK_KERNEL,
                .callback = ionic_en_mgmt_inf_count_num_dsc_cb,
                .synchronous = 1,
                .numParms = 1,
                .parmSizes = {
                                 sizeof(vmk_uint32),
                             },
                .parmTypes = {
                                 VMK_MGMT_PARMTYPE_INOUT,
                             },
                .callbackId = IONIC_EN_MGMT_INTERFACE_CB_COUNT_NUM_DSC,
        },
        {
                .location = VMK_MGMT_CALLBACK_KERNEL,
                .callback = ionic_en_mgmt_inf_get_adpt_info_cb,
                .synchronous = 1,
                .numParms = 1,
                .parmSizes = {
                                 sizeof(ionic_en_adapter_info),
                             },
                .parmTypes = {
                                 VMK_MGMT_PARMTYPE_VECTOR_INOUT,
                             },
                .callbackId = IONIC_EN_MGMT_INTERFACE_CB_GET_ADPT_INFO,
        },
        {
                .location = VMK_MGMT_CALLBACK_KERNEL,
                .callback = ionic_en_mgmt_inf_flush_fw_cb,
                .synchronous = 1,
                .numParms = 4,
                .parmSizes = {
                                 sizeof(vmk_uint64),
                                 sizeof(vmk_uint64),
                                 sizeof(vmk_uint64),
                                 sizeof(vmk_uint64),
                             },
                .parmTypes = {
                                 VMK_MGMT_PARMTYPE_IN,
                                 VMK_MGMT_PARMTYPE_IN,
                                 VMK_MGMT_PARMTYPE_IN,
                                 VMK_MGMT_PARMTYPE_IN,
                             },
                .callbackId = IONIC_EN_MGMT_INTERFACE_CB_FLASH_FW,
        },
};

vmk_MgmtApiSignature ionic_en_mgmt_sig = {
        .version       = VMK_REVISION_FROM_NUMBERS(1, 1, 0, 0),
        .name          = {IONIC_EN_MGMT_INTERFACE_NAME},
        .vendor        = {IONIC_EN_MGMT_INTERFACE_VENDOR},
        .numCallbacks  = IONIC_EN_MGMT_INTERFACE_CB_NUM,
        .callbacks     = ionic_en_mgmt_cbs,
};
