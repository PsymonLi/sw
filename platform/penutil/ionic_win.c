/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "ionic_os.h"
#include "ionic_spp.h"

#ifndef __cplusplus

#define bool  _Bool
#define false 0
#define true  1

#endif /* __cplusplus */

#include "windows.h"
#include "UserCommon.h"

DWORD
DoIoctlEx(FILE * fstream,
          LPCWSTR lpDeviceName,
          DWORD dwIoControlCode,
          LPVOID lpInBuffer,
          DWORD nInBufferSize,
          LPVOID lpOutBuffer,
          DWORD nOutBufferSize,
          PDWORD pnBytesReturned)
{
    DWORD nBytesReturned;
    HANDLE hDevice = NULL;
    DWORD dwError = ERROR_SUCCESS;

    hDevice = CreateFile(lpDeviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        dwError = GetLastError();
        ionic_print_info(fstream, "all", "IonicConfig Failed to open control device Error: %u\n", dwError);
        return dwError;
    }

    if (pnBytesReturned == NULL) {
        pnBytesReturned = &nBytesReturned;
    }

    if (!DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, pnBytesReturned, NULL)) {
        dwError = GetLastError();
        if (dwError != ERROR_MORE_DATA) {
            ionic_print_info(fstream, "all", "IonicConfig DeviceIoControl Failed Error: %u\n", dwError);
        }
    }

    CloseHandle(hDevice);

    return dwError;
}


DWORD
DoIoctl(FILE* fstream,
        DWORD dwIoControlCode,
        LPVOID lpInBuffer,
        DWORD nInBufferSize,
        LPVOID lpOutBuffer,
        DWORD nOutBufferSize,
        PDWORD pnBytesReturned)
{
    return DoIoctlEx(fstream,
                     IONIC_LINKNAME_STRING_USER,
                     dwIoControlCode,
                     lpInBuffer,
                     nInBufferSize,
                     lpOutBuffer,
                     nOutBufferSize,
                     pnBytesReturned);
}


#define IONIC_ASIC_TYPE_CAPRI   0

char* GetAsicTypeStr(UCHAR asic_type) {
    switch (asic_type) {
    case IONIC_ASIC_TYPE_CAPRI:
        return "CAPRI";
    default:
        return "Unknown";
    }
}

ULONG DumpAdapterInfo(FILE* fstream, void* info_buffer, ULONG Size, struct ionic ionic_devs[], int *ionic_devcount)
{
    struct _ADAPTER_INFO_HDR* adapter_info = (struct _ADAPTER_INFO_HDR*)info_buffer;
    struct _ADAPTER_INFO* info = NULL;
    ULONG	count = 0;
    ULONG   max_info = 0;

    if (Size > sizeof(*adapter_info)) {
        max_info = (Size - sizeof(*adapter_info)) / sizeof(*info);
        if (max_info >= adapter_info->count) {
            max_info = adapter_info->count;
        }
        else {
            ionic_print_info(fstream, "all", "Warning: bad adapter count\n\n");
        }
    }

    info = (struct _ADAPTER_INFO*)((char*)adapter_info + sizeof(struct _ADAPTER_INFO_HDR));

    for (count = 0; count < max_info; count++) {
        struct ionic* adapter;
        ULONG ulBus, ulDev, ulFun;
        fprintf(fstream, "\tScanning device Ven: 0x%04x Device: 0x%04x\n", info->vendor_id, info->product_id);

        if ((info->vendor_id != PCI_VENDOR_ID_PENSANDO) ||
            (info->product_id != PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT)) {
            info = (struct _ADAPTER_INFO*)((char*)info + sizeof(struct _ADAPTER_INFO));
            continue;
        }
        adapter = &ionic_devs[(*ionic_devcount)++];
        adapter->venId = info->vendor_id;
        adapter->devId = info->product_id;
        adapter->subVenId = info->sub_vendor_id;
        adapter->subDevId = info->sub_system_id;
        snprintf(adapter->asicType, sizeof(adapter->asicType), "%s", GetAsicTypeStr(info->asic_type));
        snprintf(adapter->macAddr, sizeof(adapter->macAddr), "%02X-%02X-%02X-%02X-%02X-%02X", info->perm_mac_addr[0], info->perm_mac_addr[1], info->perm_mac_addr[2], info->perm_mac_addr[3], info->perm_mac_addr[4], info->perm_mac_addr[5]);

        snprintf(adapter->intfName, sizeof(adapter->intfName), "%S", info->name);
        snprintf(adapter->curFwVer, sizeof(adapter->curFwVer), "%s", info->fw_version);
        _snwscanf(info->device_location, sizeof(info->device_location), L"PCI bus %u, device %u, function %u", &ulBus, &ulDev, &ulFun);
        adapter->bus = (uint8_t)ulBus;
        adapter->dev = (uint8_t)ulDev;
        adapter->func = (uint8_t)ulFun;

        fprintf(fstream, "%04x:%02x:%02x.%d vendor=%04x device=%04x\n",
            adapter->domain, adapter->bus, adapter->dev, adapter->func, adapter->venId, adapter->devId);
        fprintf(fstream, "\t%d (%s)(%s)\n", *ionic_devcount, adapter->intfName, adapter->curFwVer);

        info = (struct _ADAPTER_INFO*)((char*)info + sizeof(struct _ADAPTER_INFO));
    };


    return count;
}

int
ionic_find_devices(FILE *fstream, struct ionic ionic_devs[], int *count)
{
    DWORD Size = 1024 * 1024;
    DWORD error = ERROR_SUCCESS;
    DWORD BytesReturned = 0;
    AdapterCB cb = {0};
    DWORD dumped = 0;
    int iRet = 0;


    ionic_print_info(fstream, "all", "Discovering Naples mgmt devices on Windows\n");
    *count = 0;

    void* pBuffer = malloc(Size);
    if (pBuffer == NULL) {
        return HPE_SPP_LIBRARY_DEP_FAILED;
    }

    do {
        memset(pBuffer, 0, Size);

        BytesReturned = 0;
        error = DoIoctl(fstream, IOCTL_IONIC_GET_ADAPTER_INFO, &cb, sizeof(cb), pBuffer, Size, &BytesReturned);

        ionic_print_info(fstream, "all", "DoIoctl returned: %u, bytes: %u\n", error, BytesReturned);

        if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
            iRet = HPE_SPP_HW_ACCESS;
            break;
        }
        dumped = DumpAdapterInfo(fstream, pBuffer, BytesReturned, ionic_devs, count);

        ionic_print_info(fstream, "all", "DumpAdapterInfo returned: %u, count: %u\n", dumped, *count);
        cb.Skip += dumped;

    } while (error == ERROR_MORE_DATA);

    free(pBuffer);

    return iRet;
}

int
ionic_get_details(FILE *fstream, struct ionic *ionic)
{
    char *intfName;

    intfName = ionic->intfName;
    ionic_print_info(fstream, intfName, "Getting various details\n");

    return (0);
}


typedef struct _ADAPTER_INFO_HDR    ADAPTER_INFO_HDR, * PADAPTER_INFO_HDR;
typedef struct _ADAPTER_INFO        ADAPTER_INFO, * PADAPTER_INFO;

int
ionic_flash_firmware( FILE *fstream, struct ionic *ionic, char *fw_file_name)
{
    char *intfName;
    LARGE_INTEGER FileSize = { 0 };
    HANDLE file;
    DWORD error;
    int iret = 0;
    DWORD cbBytes = 0;
    WCHAR wszDeviceName[64];
    DWORD FwDownloadMode = ionic_fw_update_type;
    AdapterCB cb = {0};
    PVOID pBuffer = NULL;
    DWORD Size = sizeof(ADAPTER_INFO_HDR) + sizeof(ADAPTER_INFO);
    PADAPTER_INFO_HDR pHeader = NULL;
    PADAPTER_INFO pAdapterInfo = NULL;

    intfName = ionic->intfName;

    ionic_print_info(fstream, intfName, "Enter firmware update\n");

    pBuffer = malloc(Size);
    if (NULL == pBuffer) {
        ionic_print_info(fstream, intfName, "Couldn't allocate memory for adapter info buffer.\n");
        iret = HPE_SPP_LIBRARY_DEP_FAILED;
        goto exit;
    }
    memset(pBuffer, 0, Size);

    cbBytes = 0;
    _snwprintf_s(cb.AdapterName, sizeof(cb.AdapterName)/sizeof(WCHAR), _TRUNCATE, L"%S", intfName);

    error = DoIoctl(fstream, IOCTL_IONIC_GET_ADAPTER_INFO, &cb, sizeof(cb), pBuffer, Size, &cbBytes);

    ionic_print_info(fstream, intfName, "DoIoctl returned: %u, bytes returned: %u\n", error, cbBytes);
    if (error != ERROR_SUCCESS) {
        iret = HPE_SPP_HW_ACCESS;
        goto exit;
    }
    pHeader = (PADAPTER_INFO_HDR)pBuffer;
    if (pHeader->count != 1) {
        iret = HPE_SPP_HW_ACCESS;
        goto exit;
    }
    pAdapterInfo = (PADAPTER_INFO)(pHeader + 1);

    file = CreateFileA(fw_file_name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        ionic_print_info(fstream, intfName, "Couldn't open file %s. Err: %u\n", fw_file_name, error);
        iret = HPE_SPP_FW_FILE_MISSING;
        goto exit;
    };
    if ((!GetFileSizeEx(file, &FileSize)) || (0LL == FileSize.QuadPart)) {
        error = GetLastError();
        ionic_print_info(fstream, intfName, "Couldn't get file size for file %s. Err: %u\n", fw_file_name, error);
        CloseHandle(file);
        iret = HPE_SPP_FW_FILE_PERM_ERR;
        goto exit;
    }

    HANDLE mapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (NULL == mapping) {
        error = GetLastError();
        ionic_print_info(fstream, intfName, "Couldn't map file %s. Err: %u\n", fw_file_name, error);
        CloseHandle(file);
        iret = HPE_SPP_FW_FILE_PERM_ERR;
        goto exit;
    };

    DWORD FwBufLen = (DWORD)FileSize.QuadPart;
    PVOID pFwBuf = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (NULL == pFwBuf) {
        error = GetLastError();
        ionic_print_info(fstream, intfName, "Couldn't map view of file %s. Err: %u\n", fw_file_name, error);
        CloseHandle(file);
        CloseHandle(mapping);
        iret = HPE_SPP_FW_FILE_PERM_ERR;
        goto exit;
    }

    ionic_print_info(fstream, intfName, "FileName: %s size: %u  address: %p\n", fw_file_name, FwBufLen, pFwBuf);

    cbBytes = 0;
    PUCHAR pMAC = (PUCHAR)&pAdapterInfo->perm_mac_addr[0];

    _snwprintf_s(wszDeviceName, 64, _TRUNCATE, L"%ws-%02x-%02x-%02x-%02x-%02x-%02x", FWUPDATE_CTRLDEV_USERMODE, pMAC[0], pMAC[1], pMAC[2], pMAC[3], pMAC[4], pMAC[5]);

    error = DoIoctlEx(fstream, wszDeviceName, IOCTL_IONIC_FWCTRLUPDATE, &FwDownloadMode, sizeof(FwDownloadMode), pFwBuf, FwBufLen, &cbBytes);

    ionic_print_info(fstream, intfName, "DoIoctl returned: %u, bytes read: %u\n", error, cbBytes);

    if (error != ERROR_SUCCESS) {
        iret = HPE_SPP_INSTALL_HW_ERROR;
    }

    UnmapViewOfFile(pFwBuf);
    CloseHandle(mapping);
    CloseHandle(file);

exit:
    if (pBuffer) {
        free(pBuffer);
    }
    return iret;
}
