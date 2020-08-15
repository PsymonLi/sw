#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "ionic_os.h"
#include "ionic_spp.h"
#include "../common/include/ionic_en_mgmt_interface.h"

vmk_MgmtUserHandle ionic_en_mgmt_handle;
extern vmk_MgmtApiSignature ionic_en_mgmt_sig;

static inline int
ionic_en_allocate_adapter_list(int num_mgmt_infs,
                               vmk_MgmtVectorParm *adapter_list)
{
        adapter_list->dataPtr = calloc(num_mgmt_infs, sizeof(ionic_en_adapter_info));
        if (adapter_list->dataPtr == NULL) {
                return -1;
        }
        adapter_list->length = num_mgmt_infs * sizeof(ionic_en_adapter_info);

        return 0;
}

static inline void
ionic_en_free_adapter_list(vmk_MgmtVectorParm *adapter_list)
{
        free(adapter_list->dataPtr);
        adapter_list->dataPtr = NULL;
}

static int
ionic_platform_init()
{
        int error;
        vmk_uint64 ionic_en_cookie = 555;
        error = vmk_MgmtUserInit(&ionic_en_mgmt_sig,
                                 ionic_en_cookie,
                                 &ionic_en_mgmt_handle);
        if (error) {
                fprintf(stderr, "Failed to initialize management interface,"
                        " (rcUser=0x%x)\n", error);
                error = HPE_SPP_DSC_DRV_INCOMPATIBLE;
        }

        return error;
}

static void
ionic_platform_cleanup()
{
        vmk_MgmtUserDestroy(ionic_en_mgmt_handle);
}

int
ionic_find_devices(FILE *fstream, struct ionic ionic_devs[], int *count)
{
        int error, i, intf_name_len;
        vmk_MgmtVectorParm adapter_list;
        ionic_en_adapter_info *adapter_info_list;
        struct ionic *ionic;

        ionic_print_info(fstream, "all", "Discovering Naples mgmt devices on ESXI\n");

        error = ionic_platform_init();
        if (error) {
                ionic_print_error(fstream, "all", "ionic_platform_init() failed");
                return error;
        }

        error = vmk_MgmtUserCallbackInvoke(ionic_en_mgmt_handle,
                                           VMK_MGMT_NO_INSTANCE_ID,
                                           IONIC_EN_MGMT_INTERFACE_CB_COUNT_NUM_DSC,
                                           count);
        if (error) {
                ionic_print_error(fstream, "all", "Failed to call ionic_en_mgmt_inf_count_num_dsc_cb(),"
                                  " (rcUser=0x%x)\n", error);
                error =  HPE_SPP_DSC_DRV_CHANNEL_ERR;
                goto out;
        }

        error = ionic_en_allocate_adapter_list(*count,
                                               &adapter_list);
        if (error) {
                ionic_print_error(fstream, "all", "Failed to allocate memory for adapter list\n");
                error =  HPE_SPP_LIBRARY_DEP_FAILED;
                goto out;
        }

        error = vmk_MgmtUserCallbackInvoke(ionic_en_mgmt_handle,
                                           VMK_MGMT_NO_INSTANCE_ID,
                                           IONIC_EN_MGMT_INTERFACE_CB_GET_ADPT_INFO,
                                           &adapter_list);
        if (error) {
                ionic_print_error(fstream, "all", "Failed to call ionic_en_mgmt_inf_get_adpt_info_cb(),"
                                  " (rcUser=0x%x)\n", error);
                error = HPE_SPP_DSC_DRV_CHANNEL_ERR;
                goto out1;
        }

        adapter_info_list = adapter_list.dataPtr;

        for (i = 0; i < *count; i++) {
                ionic = &ionic_devs[i];

                ionic->domain = adapter_info_list[i].segment;
                ionic->bus = adapter_info_list[i].bus_number;
                ionic->dev = adapter_info_list[i].device_number;
                ionic->func = adapter_info_list[i].func_number;

                ionic->venId = adapter_info_list[i].ven_id;
                ionic->devId = adapter_info_list[i].dev_id;
                ionic->subVenId = adapter_info_list[i].sub_ven_id;
                ionic->subDevId = adapter_info_list[i].sub_dev_id;

                intf_name_len = strlen(vmk_NameToString(&adapter_info_list[i].ethernet_interface_name));
                strncpy(&ionic->intfName[0],
                        vmk_NameToString(&adapter_info_list[i].ethernet_interface_name),
                        intf_name_len);
                memcpy(&ionic->curFwVer[0],
                       vmk_NameToString(&adapter_info_list[i].fw_ver),
                       VMK_MISC_NAME_MAX);
                memcpy(ionic->macAddr,
                       adapter_info_list[i].mac_address,
                       HPE_SPP_MAC_ADDR_MAX_LEN);
        }

        goto out;

out1:
        ionic_en_free_adapter_list(&adapter_list);

out:
        ionic_platform_cleanup();
        return error;
}

int
ionic_get_details(FILE *fstream,
                  struct ionic *ionic)
{
        ionic_print_info(fstream, ionic->intfName, "Getting device details\n");
        return (0);
}

int
ionic_flash_firmware( FILE *fstream, struct ionic *ionic, char *fw_file_name)
{
        char *intfName;
        char *fw_data = NULL;
        int rc = 0;
        unsigned long fw_sz = 0;
        unsigned long rd_size = 0;
        FILE *fd;
        ionic_en_fw_flush_params fw_flush_params;

        intfName = ionic->intfName;
        ionic_print_info(fstream, intfName, "Enter firmware update\n");

        rc = ionic_platform_init();
        if (rc) {
                ionic_print_error(fstream, "all", "ionic_platform_init() failed");
                return rc;
        }

        fd = fopen(fw_file_name,"rb");
        if(fd == NULL) {
                ionic_print_info(fstream, intfName, "Failed to open firmware file: %s", fw_file_name);
                rc = HPE_SPP_FW_FILE_MISSING;
                goto out;
        }

        fseek(fd, 0, SEEK_END);
        fw_sz = ftell(fd);
        fseek(fd, 0, SEEK_SET);

        fw_data = calloc(fw_sz + 1, 1);
        if (!fw_data) {
                ionic_print_info(fstream, intfName, "Failed to allocate memory");
                rc = HPE_SPP_LIBRARY_DEP_FAILED;
                fclose(fd);
                goto out;
        }

        rd_size = fread(fw_data, 1, fw_sz, fd);
        fclose(fd);
        fw_data[fw_sz] = 0;

        if (fw_sz != rd_size) {
                ionic_print_info(fstream, intfName, "Firmware file was not read properly");
                rc = HPE_SPP_FW_FILE_PERM_ERR;
                goto out1;
        }

        fw_flush_params.fw_img_name_addr = (vmk_uint64) fw_file_name;
        fw_flush_params.fw_img_data_addr = (vmk_uint64) fw_data;
        fw_flush_params.fw_img_size = fw_sz;
        fw_flush_params.uplink_name_addr = (vmk_uint64) intfName;
        fw_flush_params.is_adminq_based = ionic_fw_update_type;

        rc = vmk_MgmtUserCallbackInvoke(ionic_en_mgmt_handle,
                                        VMK_MGMT_NO_INSTANCE_ID,
                                        IONIC_EN_MGMT_INTERFACE_CB_FLASH_FW,
                                        &fw_flush_params);
        if (rc) {
                ionic_print_info(fstream, intfName,"Failed to call ionic_en_mgmt_inf_flush_fw_cb(),"
                                 " (rcUser=0x%x)", rc);
                rc = HPE_SPP_INSTALL_HW_ERROR;
        }

out1:
        free(fw_data);

out:
        ionic_platform_cleanup();
        return rc;
}


