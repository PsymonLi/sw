/*
 * Copyright (c) 2020 Pensando Systems. All rights reserved.
 */

/*
 * ionic_en_mgmt.c --
 *
 * Implement the kernel part of the ionic management utility
 */

 #include "ionic.h"

#define IONIC_EN_UPLINK_NAME_LEN                  8

static inline vmk_HashKeyIteratorCmd
ionic_hash_table_count_num_dsc_iterator(vmk_HashTable hash_table,
                                        vmk_HashKey key,
                                        vmk_HashValue value,
                                        vmk_AddrCookie addr_cookie)
{
        vmk_uint32 *cnt = (vmk_uint32 *) addr_cookie.ptr;
        struct ionic_en_priv_data *priv_data = (struct ionic_en_priv_data *) value;

        if (priv_data->uplink_handle.is_mgmt_nic) {
                (*cnt)++;
        }

        return VMK_HASH_KEY_ITER_CMD_CONTINUE;
}


VMK_ReturnStatus
ionic_en_mgmt_inf_count_num_dsc_cb(vmk_MgmtCookies *cookies,
                                   vmk_MgmtEnvelope *envelope,
                                   vmk_uint32 *num_dscs)
{
        VMK_ReturnStatus status;
        vmk_AddrCookie addr_cookie;
        vmk_uint32 cnt = 0;

        addr_cookie.ptr = &cnt;

        vmk_SemaLock(&ionic_driver.uplink_dev_list.lock);
        status = vmk_HashKeyIterate(ionic_driver.uplink_dev_list.list,
                                    ionic_hash_table_count_num_dsc_iterator,
                                    addr_cookie);
        vmk_SemaUnlock(&ionic_driver.uplink_dev_list.lock);
        if (status != VMK_OK) {
                ionic_en_err("vmk_HashKeyIterate() failed, status: %s",
                          vmk_StatusToString(status));
        }

        *num_dscs = cnt;

        return VMK_OK;
}

        
static inline vmk_HashKeyIteratorCmd
ionic_hash_table_get_adpt_info_iterator(vmk_HashTable hash_table,
                                        vmk_HashKey key,
                                        vmk_HashValue value,
                                        vmk_AddrCookie addr_cookie)
{
        vmk_uint32 i;
        struct ionic_en_priv_data *priv_data;
        vmk_MgmtVectorParm *adapter_list;
        ionic_en_adapter_info *adapter_info_list;
        int num_mgmt_infs;

        adapter_list = (vmk_MgmtVectorParm *) addr_cookie.ptr;
        adapter_info_list = adapter_list->dataPtr;
        priv_data = (struct ionic_en_priv_data *) value;
        num_mgmt_infs = adapter_list->length / sizeof(ionic_en_adapter_info); 

        if (!priv_data->uplink_handle.is_mgmt_nic) {
                return VMK_HASH_KEY_ITER_CMD_CONTINUE;
        }

        for (i = 0; i < num_mgmt_infs; i++) {
                if (adapter_info_list[i].is_used == VMK_FALSE) {
                        adapter_info_list[i].is_used = VMK_TRUE;
                        break;
                }
        }

        vmk_NameCopy(&adapter_info_list[i].ethernet_interface_name,
                     &priv_data->uplink_name);
        vmk_NameInitialize(&adapter_info_list[i].fw_ver,
                           priv_data->ionic.en_dev.idev.dev_info_regs->fw_version);

        /* Get PCI device ID info */
        adapter_info_list[i].ven_id = priv_data->ionic.en_dev.pci_device_id.vendorID;
        adapter_info_list[i].dev_id = priv_data->ionic.en_dev.pci_device_id.deviceID;
        adapter_info_list[i].sub_ven_id = priv_data->ionic.en_dev.pci_device_id.subVendorID;
        adapter_info_list[i].sub_dev_id = priv_data->ionic.en_dev.pci_device_id.subDeviceID;

        /* Get BUS info */
        adapter_info_list[i].segment = priv_data->ionic.en_dev.sbdf.seg;
        adapter_info_list[i].bus_number = priv_data->ionic.en_dev.sbdf.bus;
        adapter_info_list[i].device_number = priv_data->ionic.en_dev.sbdf.dev;
        adapter_info_list[i].func_number = priv_data->ionic.en_dev.sbdf.fn;

        vmk_Snprintf(adapter_info_list[i].mac_address,
                     IONIC_EN_MAC_ADDR_MAX_LEN,
                     VMK_ETH_ADDR_FMT_STR, VMK_ETH_ADDR_FMT_ARGS(priv_data->uplink_handle.vmk_mac_addr)); 

        return VMK_HASH_KEY_ITER_CMD_CONTINUE;
}



VMK_ReturnStatus
ionic_en_mgmt_inf_get_adpt_info_cb(vmk_MgmtCookies *cookies,
                                   vmk_MgmtEnvelope *envelope,
                                   vmk_MgmtVectorParm *adapter_list)
{
        VMK_ReturnStatus status;
        vmk_AddrCookie addr_cookie;

        addr_cookie.ptr = adapter_list;

        vmk_SemaLock(&ionic_driver.uplink_dev_list.lock);
        status = vmk_HashKeyIterate(ionic_driver.uplink_dev_list.list,
                                    ionic_hash_table_get_adpt_info_iterator,
                                    addr_cookie);
        vmk_SemaUnlock(&ionic_driver.uplink_dev_list.lock);
        if (status != VMK_OK) {
                ionic_en_err("vmk_HashKeyIterate() failed, status: %s",
                          vmk_StatusToString(status));
        }
       
        return VMK_OK;
}


VMK_ReturnStatus
ionic_en_mgmt_inf_flush_fw_cb(vmk_MgmtCookies *cookies,
                              vmk_MgmtEnvelope *envelope,
                              ionic_en_fw_flush_params *fw_flush_params)
{
        VMK_ReturnStatus status;
        vmk_Name uplink_name;
        char *fw_img_name;
        char *buf;
        struct ionic_en_priv_data *priv_data = NULL;
        
        fw_img_name = (char *)(fw_flush_params->fw_img_name_addr);
        buf = (char *)(fw_flush_params->fw_img_data_addr);

        vmk_StringCopy(uplink_name.string,
                       (char *)(fw_flush_params->uplink_name_addr),
                       IONIC_EN_UPLINK_NAME_LEN);

        status = ionic_device_list_get(uplink_name,
                                       &ionic_driver.uplink_dev_list,
                                       &priv_data);
        if (status != VMK_OK) {
                ionic_en_err("Uplink: %s is not found", vmk_NameToString(&uplink_name));
                return VMK_NOT_FOUND;
        }

        status = ionic_firmware_update(priv_data,
                                       buf,
                                       fw_flush_params->fw_img_size,
                                       fw_img_name,
                                       fw_flush_params->is_adminq_based);
        if (status != VMK_OK) {
                ionic_en_err("ionic_firmware_update() failed, status: %s",
                             vmk_StatusToString(status));
        }

        return VMK_OK;
}

