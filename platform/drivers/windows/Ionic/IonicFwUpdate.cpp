#include "common.h"


#define FW_INSTALL_ACTIVATE_TIMEOUT (25*60)  // sec.

NTSTATUS
ionic_firmware_download_adminq(struct lif* lif, uint64_t addr, uint32_t offset, uint32_t length)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    struct ionic_admin_ctx ctx = { 0 };
    //ctx.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work);
    ctx.cmd.fw_download.opcode = CMD_OPCODE_FW_DOWNLOAD;
    ctx.cmd.fw_download.offset = offset;
    ctx.cmd.fw_download.addr = addr;
    ctx.cmd.fw_download.length = length;


    status = ionic_adminq_post_wait(lif, &ctx);
    if (status != NDIS_STATUS_SUCCESS) {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s CMD_OPCODE_FW_DOWNLOAD failed: 0x%x\n", __FUNCTION__, status));
    }

    return status;
}

NTSTATUS
ionic_firmware_install_adminq(struct lif* lif, uint8_t* slot)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    struct ionic_admin_ctx ctx = { 0 };
    //ctx.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work);
    ctx.cmd.fw_control.opcode = CMD_OPCODE_FW_CONTROL;
    ctx.cmd.fw_control.oper = IONIC_FW_INSTALL;

    status = ionic_adminq_post_wait_timeout(lif, &ctx, FW_INSTALL_ACTIVATE_TIMEOUT);
    if (status != NDIS_STATUS_SUCCESS) {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s CMD_OPCODE_FW_CONTROL(IONIC_FW_INSTALL) failed: 0x%x\n", __FUNCTION__, status));
    }
    else {
        *slot = ctx.comp.fw_control.slot;
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s CMD_OPCODE_FW_CONTROL(IONIC_FW_INSTALL) succeeded. Slot: %u\n", __FUNCTION__, *slot));
    }

    return status;
}

NTSTATUS
ionic_firmware_activate_adminq(struct lif* lif, uint8_t slot)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    struct ionic_admin_ctx ctx = { 0 };
    //ctx.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work);
    ctx.cmd.fw_control.opcode = CMD_OPCODE_FW_CONTROL;
    ctx.cmd.fw_control.oper = IONIC_FW_ACTIVATE;
    ctx.cmd.fw_control.slot = slot;

    status = ionic_adminq_post_wait_timeout(lif, &ctx, FW_INSTALL_ACTIVATE_TIMEOUT);
    if (status != NDIS_STATUS_SUCCESS) {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s CMD_OPCODE_FW_CONTROL(IONIC_FW_ACTIVATE) failed: 0x%x\n", __FUNCTION__, status));
    }
    else {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s CMD_OPCODE_FW_CONTROL(IONIC_FW_ACTIVATE) succeeded. Slot: %u\n", __FUNCTION__, slot));
    }

    return status;
}

NTSTATUS
ionic_firmware_update_adminq(struct ionic* adapter, const void* const fw_data, uint32_t fw_sz)
{
    dma_addr_t buf_pa;
    void* buf;
    uint32_t buf_sz, copy_sz, offset;
    uint8_t fw_slot;
    NTSTATUS ntStatus;
    struct lif* lif = adapter->master_lif;
    LARGE_INTEGER StartTime1, EndTime1, StartTime2, EndTime2, StartToEndTime;

    KeQuerySystemTime(&StartTime1);
    StartTime2 = StartTime1;

    if (NULL == lif) {
        return STATUS_INVALID_PARAMETER;
    }

    buf_sz = (1 << 20);	// 1 MiB

    /* Allocate DMA'ble bounce buffer for sending firmware to card */
    buf = dma_alloc_coherent(lif->ionic, buf_sz, &buf_pa, 0);

    if (NULL == buf) {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s Failed to allocate DMA'ble firmware buffer, aborting.\n", __FUNCTION__));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        return ntStatus;
    }

    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Firmware dma buffer address 0x%I64x (PA: 0x%I64x)\n", __FUNCTION__, buf, buf_pa));

    offset = 0;
    while (offset < fw_sz) {
        copy_sz = min(buf_sz, fw_sz - offset);
        NdisMoveMemory(buf, (uint8_t*)fw_data + (size_t)offset, copy_sz);

        ntStatus = ionic_firmware_download_adminq(lif, buf_pa, offset, copy_sz);
        if (ntStatus != STATUS_SUCCESS) {
            DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s - Firmware upload failed at offset: 0x%x addr: %p len: %u\n", __FUNCTION__, offset, buf_pa, copy_sz));
            goto err_out_free_buf;
        }
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Firmware upload success at offset: 0x%x addr: %p len: %u\n", __FUNCTION__, offset, buf_pa, copy_sz));
        offset += copy_sz;
    }
    KeQuerySystemTime(&EndTime2);
    StartToEndTime.QuadPart = EndTime2.QuadPart - StartTime2.QuadPart;
    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Time to download: %I64u ms\n", __FUNCTION__, StartToEndTime.QuadPart / 10000));

    KeQuerySystemTime(&StartTime2);
    ntStatus = ionic_firmware_install_adminq(lif, &fw_slot);
    if (ntStatus != STATUS_SUCCESS) {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s - Firmware install failed.\n", __FUNCTION__));
        goto err_out_free_buf;
    }
    KeQuerySystemTime(&EndTime2);
    StartToEndTime.QuadPart = EndTime2.QuadPart - StartTime2.QuadPart;
    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Time to install: %I64u ms\n", __FUNCTION__, StartToEndTime.QuadPart / 10000));

    KeQuerySystemTime(&StartTime2);
    ntStatus = ionic_firmware_activate_adminq(lif, fw_slot);
    if (ntStatus != STATUS_SUCCESS) {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s - Firmware activate failed.\n", __FUNCTION__));
        goto err_out_free_buf;
    }
    KeQuerySystemTime(&EndTime2);
    EndTime1 = EndTime2;
    StartToEndTime.QuadPart = EndTime2.QuadPart - StartTime2.QuadPart;
    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Time to activate: %I64u ms\n", __FUNCTION__, StartToEndTime.QuadPart / 10000));
    StartToEndTime.QuadPart = EndTime1.QuadPart - StartTime1.QuadPart;
    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Total time: %I64u ms\n", __FUNCTION__, StartToEndTime.QuadPart / 10000));

err_out_free_buf:
    dma_free_coherent(lif->ionic, buf_sz, buf, buf_pa);

    return ntStatus;
}

NTSTATUS
ionic_firmware_update_devcmd(struct ionic* adapter, const void* const fw_data, uint32_t fw_sz)
{
    NTSTATUS ntStatus;
    uint8_t fw_slot;
    uint32_t buf_sz, copy_sz, offset;
    u64 addr = FIELD_OFFSET(union dev_cmd_regs, data);
    union dev_cmd_comp comp = {};
    struct fw_control_comp* fw_control_comp;
    struct ionic_dev* idev = &adapter->idev;
    LARGE_INTEGER StartTime1, EndTime1, StartTime2, EndTime2, StartToEndTime;

    KeQuerySystemTime(&StartTime1);
    StartTime2 = StartTime1;

    buf_sz = sizeof(idev->dev_cmd_regs->data);
    offset = 0;
    while (offset < fw_sz) {
        copy_sz = min(buf_sz, fw_sz - offset);
        memcpy_toio(&idev->dev_cmd_regs->data, (uint8_t*)fw_data + (size_t)offset, copy_sz);

        ionic_dev_cmd_fw_download(idev, offset, addr, copy_sz);

        ntStatus = ionic_dev_cmd_wait(adapter, devcmd_timeout);
        if (ntStatus != STATUS_SUCCESS) {
            DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s - Firmware upload failed at offset: 0x%x len: %u\n", __FUNCTION__, offset, copy_sz));
            goto fwdownload_exit;
        }
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Firmware upload success at offset: 0x%x len: %u\n", __FUNCTION__, offset, copy_sz));
        offset += copy_sz;
    }
    KeQuerySystemTime(&EndTime2);
    StartToEndTime.QuadPart = EndTime2.QuadPart - StartTime2.QuadPart;
    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Time to download: %I64u ms\n", __FUNCTION__, StartToEndTime.QuadPart / 10000));

    KeQuerySystemTime(&StartTime2);
    ionic_dev_cmd_fw_install(idev);
    ntStatus = ionic_dev_cmd_wait(adapter, FW_INSTALL_ACTIVATE_TIMEOUT);
    if (ntStatus != STATUS_SUCCESS) {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s - Firmware install failed.\n", __FUNCTION__));
        goto fwdownload_exit;
    }
    ionic_dev_cmd_comp(idev, &comp);
    fw_control_comp = (struct fw_control_comp*) & comp.comp;
    fw_slot = fw_control_comp->slot;
    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Firmware install succeeded. Slot: %u\n", __FUNCTION__, fw_slot));
    KeQuerySystemTime(&EndTime2);
    StartToEndTime.QuadPart = EndTime2.QuadPart - StartTime2.QuadPart;
    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Time to install: %I64u ms\n", __FUNCTION__, StartToEndTime.QuadPart / 10000));

    KeQuerySystemTime(&StartTime2);
    ionic_dev_cmd_fw_activate(idev, fw_slot);
    ntStatus = ionic_dev_cmd_wait(adapter, FW_INSTALL_ACTIVATE_TIMEOUT);
    if (ntStatus != STATUS_SUCCESS) {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s - Firmware activate failed.\n", __FUNCTION__));
        goto fwdownload_exit;
    }
    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Firmware activate succeeded. Slot: %u\n", __FUNCTION__, fw_slot));
    KeQuerySystemTime(&EndTime2);
    EndTime1 = EndTime2;
    StartToEndTime.QuadPart = EndTime2.QuadPart - StartTime2.QuadPart;
    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Time to activate: %I64u ms\n", __FUNCTION__, StartToEndTime.QuadPart / 10000));
    StartToEndTime.QuadPart = EndTime1.QuadPart - StartTime1.QuadPart;
    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s - Total time: %I64u ms\n", __FUNCTION__, StartToEndTime.QuadPart / 10000));

fwdownload_exit:

    return ntStatus;

}

NTSTATUS 
ionic_firmware_update(struct ionic* adapter, PVOID FwBuf, ULONG FwBufLen, ULONG DownloadMode, PULONG pReadBytes)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pReadBytes) {
        *pReadBytes = 0;
    }
    if (DownloadMode == FW_DOWNLOAD_DEVCMD) {
        ntStatus = ionic_firmware_update_devcmd(adapter, FwBuf, FwBufLen);
    }
    else {
        ntStatus = ionic_firmware_update_adminq(adapter, FwBuf, FwBufLen);
    }
    if (STATUS_SUCCESS == ntStatus) {
        if (pReadBytes) {
            *pReadBytes = FwBufLen;
        }
    }

    return ntStatus;
}

NTSTATUS IoctlFwUpdateSynchronous(struct ionic *adapter, PVOID pFwBuf, ULONG FwBufLen, ULONG DownloadMode, PULONG outbytes) {
    NDIS_OPER_STATE state;
    NTSTATUS ntStatus;

    state.Header.Revision = NDIS_OPER_STATE_REVISION_1;
    state.Header.Size = NDIS_SIZEOF_OPER_STATE_REVISION_1;
    state.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;

    state.OperationalStatus = NET_IF_OPER_STATUS_TESTING;
    state.OperationalStatusFlags = 0;

    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s IOCTL_IONIC_FWUPDATE  Setting status to TESTING\n", __FUNCTION__));

    ionic_indicate_status(adapter, NDIS_STATUS_OPER_STATUS, &state, sizeof(NDIS_OPER_STATE));

    ntStatus = ionic_firmware_update(adapter, pFwBuf, FwBufLen, DownloadMode, outbytes);

    state.OperationalStatus = NET_IF_OPER_STATUS_UP;

    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s IOCTL_IONIC_FWUPDATE  Setting status to UP\n", __FUNCTION__));

    ionic_indicate_status(adapter, NDIS_STATUS_OPER_STATUS, &state, sizeof(NDIS_OPER_STATE));

    KeReleaseSemaphore(&adapter->FwUpdateData.FwUpdateSemaphore, IO_NO_INCREMENT, 1, FALSE);

    return ntStatus;
}

NTSTATUS IoctlFwUpdate(PIRP Irp, PULONG outbytes) {
    NTSTATUS ntStatus;
    PVOID pInBuf1 = Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG ulInBuf1Len = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    PVOID pInBuf2 = NULL;
    ULONG ulInBuf2Len = 0;
    ULONG ulMdlBufLen = 0;
    struct ionic* adapter = NULL;
    LIST_ENTRY MgmtInterfacesList;
    PFW_UPDATE_DATA pFwUpdateData;
    ULONG FwCrtDownloadMode;

    NDIS_STRING AdapterNameString = {};
    FwDownloadCB cb = {};

    *outbytes = 0;

    if (ulInBuf1Len < sizeof(cb)) {
        ntStatus = NDIS_STATUS_BUFFER_TOO_SHORT;
        goto CLEAN_AND_EXIT;
    }

    cb = *(FwDownloadCB*)pInBuf1;
    InitAdapterNameString(&AdapterNameString, cb.AdapterName);
    if (cb.Flags & FWDOWNLOAD_FLAG_ADMINQ) {
        FwCrtDownloadMode = FW_DOWNLOAD_ADMINQ;
    }
    else {
        FwCrtDownloadMode = FwDownloadMode;
    }

    if (NULL == Irp->MdlAddress) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto CLEAN_AND_EXIT;
    }
    pInBuf2 = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority | MdlMappingNoExecute);
    ulInBuf2Len = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ulMdlBufLen = MmGetMdlByteCount(Irp->MdlAddress);
    if (ulMdlBufLen < ulInBuf2Len) {
        ntStatus = STATUS_UNSUCCESSFUL;
        goto CLEAN_AND_EXIT;
    }

    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s IOCTL_IONIC_FWUPDATE  pInBuf1: %p(%u) pInBuf2: %p(%u, %u)\n", __FUNCTION__, pInBuf1, ulInBuf1Len, pInBuf2, ulInBuf2Len, ulMdlBufLen));

    // Search for mnics (all/name matching) and acquire fw update mutex on each
    // Insert all found mnics in a temporary list
    ntStatus = STATUS_NOT_FOUND;
    InitializeListHead(&MgmtInterfacesList);

    NDIS_WAIT_FOR_MUTEX(&AdapterListLock);
    ListForEachEntry(adapter, &AdapterList, struct ionic, list_entry) {

        if (!MatchesAdapterNameIonic(&AdapterNameString, adapter)) {
            continue;
        }
        if (adapter->pci_config.DeviceID != PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT) {
            DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s IOCTL_IONIC_FWUPDATE - adapter is not a management interface. Continue\n", __FUNCTION__));
            if (AdapterNameString.Length != 0) {
                ntStatus = STATUS_INVALID_PARAMETER;
                break;
            }
            continue;
        }
        if (adapter->hardware_status != NdisHardwareStatusReady || BooleanFlagOn(adapter->Flags, IONIC_FLAG_PAUSED)) {
            DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s IOCTL_IONIC_FWUPDATE - not accepted due to device not ready\n", __FUNCTION__));
            if (AdapterNameString.Length != 0) {
                ntStatus = NDIS_STATUS_NOT_ACCEPTED;
                break;
            }
            continue;
        }

        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s IOCTL_IONIC_FWUPDATE  Found management interface: %p\n", __FUNCTION__, adapter));

        KeWaitForSingleObject(&adapter->FwUpdateData.FwUpdateSemaphore, Executive, KernelMode, FALSE, NULL);
        adapter->FwUpdateData.FwUpdateIrp = Irp;
        InsertTailList(&MgmtInterfacesList, &adapter->FwUpdateData.FwUpdateListEntry);
        ntStatus = STATUS_SUCCESS;
    }
    NDIS_RELEASE_MUTEX(&AdapterListLock);

    ListForEachEntry(pFwUpdateData, &MgmtInterfacesList, FW_UPDATE_DATA, FwUpdateListEntry) {
        adapter = CONTAINING_RECORD(pFwUpdateData, struct ionic, FwUpdateData);
        if (STATUS_SUCCESS == ntStatus) {
#ifdef FW_UPDATE_ASYNC
            ntStatus = IoctlFwUpdateAsynchronous(adapter, pInBuf2, ulInBuf2Len, FwCrtDownloadMode, outbytes);
#else
            ntStatus = IoctlFwUpdateSynchronous(adapter, pInBuf2, ulInBuf2Len, FwCrtDownloadMode, outbytes);
#endif
        }
        else {
            KeReleaseSemaphore(&adapter->FwUpdateData.FwUpdateSemaphore, IO_NO_INCREMENT, 1, FALSE);
        }
    }

CLEAN_AND_EXIT:

    return ntStatus;
}

NTSTATUS
IoctlFwControlUpdate(struct ionic* adapter, PIRP Irp, PULONG outbytes) {
    NTSTATUS ntStatus;
    PVOID pInBuf1 = Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG ulInBuf1Len = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    PVOID pInBuf2 = NULL;
    ULONG ulInBuf2Len = 0;
    ULONG ulMdlBufLen = 0;
    ULONG FwCrtDownloadMode;

    *outbytes = 0;

    if (ulInBuf1Len < sizeof(FwCrtDownloadMode)) {
        FwCrtDownloadMode = FwDownloadMode;
    }
    else {
        FwCrtDownloadMode = *(PULONG)pInBuf1;
    }

    if (NULL == Irp->MdlAddress) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto CLEAN_AND_EXIT;
    }
    pInBuf2 = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority | MdlMappingNoExecute);
    ulInBuf2Len = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ulMdlBufLen = MmGetMdlByteCount(Irp->MdlAddress);
    if (ulMdlBufLen < ulInBuf2Len) {
        ntStatus = STATUS_UNSUCCESSFUL;
        goto CLEAN_AND_EXIT;
    }

    DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_VERBOSE, "%s IOCTL_IONIC_FWCTRLUPDATE  pInBuf1: %p(%u) pInBuf2: %p(%u, %u)\n", __FUNCTION__, pInBuf1, ulInBuf1Len, pInBuf2, ulInBuf2Len, ulMdlBufLen));

    // acquire semaphore before checking hardware_status or Pause state
    KeWaitForSingleObject(&adapter->FwUpdateData.FwUpdateSemaphore, Executive, KernelMode, FALSE, NULL);

    if (adapter->pci_config.DeviceID != PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT) {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s IOCTL_IONIC_FWCTRLUPDATE - adapter is not a management interface.\n", __FUNCTION__));
        KeReleaseSemaphore(&adapter->FwUpdateData.FwUpdateSemaphore, IO_NO_INCREMENT, 1, FALSE);
        ntStatus = STATUS_NOT_FOUND;
        goto CLEAN_AND_EXIT;
    }
    if (adapter->hardware_status != NdisHardwareStatusReady || BooleanFlagOn(adapter->Flags, IONIC_FLAG_PAUSED)) {
        DbgTrace((TRACE_COMPONENT_IOCTL, TRACE_LEVEL_ERROR, "%s IOCTL_IONIC_FWCTRLUPDATE - not accepted due to device not ready\n", __FUNCTION__));
        KeReleaseSemaphore(&adapter->FwUpdateData.FwUpdateSemaphore, IO_NO_INCREMENT, 1, FALSE);
        ntStatus = NDIS_STATUS_NOT_ACCEPTED;
        goto CLEAN_AND_EXIT;
    }
    adapter->FwUpdateData.FwUpdateIrp = Irp;
#ifdef FW_UPDATE_ASYNC
    ntStatus = IoctlFwUpdateAsynchronous(adapter, pInBuf2, ulInBuf2Len, FwCrtDownloadMode, outbytes);
#else
    ntStatus = IoctlFwUpdateSynchronous(adapter, pInBuf2, ulInBuf2Len, FwCrtDownloadMode, outbytes);
#endif

CLEAN_AND_EXIT:

    return ntStatus;

}
