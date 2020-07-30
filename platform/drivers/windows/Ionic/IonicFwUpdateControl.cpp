#include "common.h"
#include <wdmsec.h>


PIONIC_ADAPTER GetAdapterFromDevObject(PDEVICE_OBJECT pDeviceObject) {
    PIONIC_ADAPTER* pExtension = (PIONIC_ADAPTER*)NdisGetDeviceReservedExtension(pDeviceObject);
    PIONIC_ADAPTER  Adapter;

    if (!pExtension) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR, "%s - Failed to get device object extension\n", __FUNCTION__));
        return NULL;
    }
    Adapter = *pExtension;
    if (NULL == Adapter) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR, "%s - Failed to get adapter object\n", __FUNCTION__));
    }
    return Adapter;
}


NTSTATUS
FwControlDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION  pIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIONIC_ADAPTER      Adapter = GetAdapterFromDevObject(DeviceObject);

    if (NULL == Adapter) {
        ntStatus = STATUS_UNSUCCESSFUL;
        goto DISPATCH_EXIT;
    }
    switch (pIrpStack->MajorFunction) {
    case IRP_MJ_CREATE:
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE, "%s(IRP_MJ_CREATE) - mnic%u\n", __FUNCTION__, Adapter->nameIdx));
        break;
    case IRP_MJ_CLEANUP:
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE, "%s(IRP_MJ_CLEANUP) - mnic%u\n", __FUNCTION__, Adapter->nameIdx));
        break;
    case IRP_MJ_CLOSE:
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE, "%s(IRP_MJ_CLOSE) - mnic%u\n", __FUNCTION__, Adapter->nameIdx));
        break;
    default:
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE, "%s(Maj:%u) - mnic%u\n", __FUNCTION__, pIrpStack->MajorFunction, Adapter->nameIdx));
        break;
    }

DISPATCH_EXIT:
    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
}


NTSTATUS FwCtrlDevIoControl(IN PDEVICE_OBJECT pDeviceObject, PIRP Irp) {
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IoStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG               IOCTLCode = IoStack->Parameters.DeviceIoControl.IoControlCode;
    PIONIC_ADAPTER      Adapter = GetAdapterFromDevObject(pDeviceObject);
    ULONG               BytesReturned = 0;

    if (NULL == Adapter) {
        ntStatus = STATUS_UNSUCCESSFUL;
        goto COMPLETE_IOCTL;
    }
    
    DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE, "%s(IOCTLCode: %x) - mnic%u\n", __FUNCTION__, IOCTLCode, Adapter->nameIdx));

    switch (IOCTLCode) {
    case IOCTL_IONIC_FWCTRLUPDATE: {
        ntStatus = IoctlFwControlUpdate(Adapter, Irp, &BytesReturned);
        break;
    }
    default:
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }

COMPLETE_IOCTL:

    if (ntStatus != STATUS_PENDING) {
        Irp->IoStatus.Information = (ULONG_PTR)BytesReturned;
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE, "%s <== mnic%u, status: 0x%08x\n", __FUNCTION__, (NULL != Adapter) ? Adapter->nameIdx : 0, ntStatus));

    return ntStatus;
}


NDIS_STATUS FwCtrlDevRegister(PIONIC_ADAPTER Adapter) {
    NTSTATUS                        Status = NDIS_STATUS_SUCCESS;
    UNICODE_STRING                  DeviceName;
    UNICODE_STRING                  DeviceLinkUnicodeString;
    PDRIVER_DISPATCH                DispatchTable[IRP_MJ_MAXIMUM_FUNCTION + 1];
    NDIS_DEVICE_OBJECT_ATTRIBUTES   DeviceAttribute;
    WCHAR pwstrDevName[64];
    WCHAR pwstrLinkName[64];
    PUCHAR pMAC = (PUCHAR)&Adapter->perm_addr[0];

    DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE, "%s - mnic%u ==>\n", __FUNCTION__, Adapter->nameIdx));

    if (Adapter->pci_config.DeviceID != PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR, "%s - Not a management interface -> no control device.\n", __FUNCTION__, Status));
        Status = NDIS_STATUS_NOT_ACCEPTED;
        goto DEVREGISTER_EXIT;
    }

    NdisZeroMemory(DispatchTable, (IRP_MJ_MAXIMUM_FUNCTION + 1) * sizeof(PDRIVER_DISPATCH));

    DispatchTable[IRP_MJ_CREATE] = FwControlDispatch;
    DispatchTable[IRP_MJ_CLEANUP] = FwControlDispatch;
    DispatchTable[IRP_MJ_CLOSE] = FwControlDispatch;
    DispatchTable[IRP_MJ_DEVICE_CONTROL] = FwCtrlDevIoControl;

    Status = RtlStringCchPrintfW(pwstrDevName, 64, L"%ws-%02x-%02x-%02x-%02x-%02x-%02x", FWUPDATE_CTRLDEV_DEVICENAME, pMAC[0], pMAC[1], pMAC[2], pMAC[3], pMAC[4], pMAC[5]);
    if (!NT_SUCCESS(Status)) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR, "%s - RtlStringCchPrintfW failed for DevName %x\n", __FUNCTION__, Status));
        goto DEVREGISTER_EXIT;
    }
    Status = RtlStringCchPrintfW(pwstrLinkName, 64, L"%ws-%02x-%02x-%02x-%02x-%02x-%02x", FWUPDATE_CTRLDEV_LINKNAME, pMAC[0], pMAC[1], pMAC[2], pMAC[3], pMAC[4], pMAC[5]);
    if (!NT_SUCCESS(Status)) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR, "%s - RtlStringCchPrintfW failed for LinkName %x\n", __FUNCTION__, Status));
        goto DEVREGISTER_EXIT;
    }
    NdisInitUnicodeString(&DeviceName, pwstrDevName);
    NdisInitUnicodeString(&DeviceLinkUnicodeString, pwstrLinkName);

    NdisZeroMemory(&DeviceAttribute, sizeof(NDIS_DEVICE_OBJECT_ATTRIBUTES));

    DeviceAttribute.Header.Type = NDIS_OBJECT_TYPE_DEVICE_OBJECT_ATTRIBUTES;
    DeviceAttribute.Header.Revision = NDIS_DEVICE_OBJECT_ATTRIBUTES_REVISION_1;
    DeviceAttribute.Header.Size = sizeof(NDIS_DEVICE_OBJECT_ATTRIBUTES);

    DeviceAttribute.DeviceName = &DeviceName;
    DeviceAttribute.SymbolicName = &DeviceLinkUnicodeString;
    DeviceAttribute.MajorFunctions = &DispatchTable[0];
    DeviceAttribute.ExtensionSize = sizeof(struct ionic*);
    DeviceAttribute.DefaultSDDLString = &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R;

    if (NDIS_STATUS_SUCCESS == (Status = NdisRegisterDeviceEx(IonicDriver, &DeviceAttribute, &Adapter->FwUpdateCntrlDevObj, &Adapter->FwUpdateControlHandle))) {
        PIONIC_ADAPTER* pExtension = (PIONIC_ADAPTER*)NdisGetDeviceReservedExtension(Adapter->FwUpdateCntrlDevObj);
        if (pExtension) {
            *pExtension = Adapter;
        }
        else {
            NdisDeregisterDeviceEx(Adapter->FwUpdateControlHandle);
            Adapter->FwUpdateControlHandle = NULL;
            Adapter->FwUpdateCntrlDevObj = NULL;
            Status = STATUS_UNSUCCESSFUL;
            DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR, "%s - Failed to create device object.\n", __FUNCTION__));
            goto DEVREGISTER_EXIT;
        }
    }
    else {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR, "%s - Failed to create device object.\n", __FUNCTION__));
        goto DEVREGISTER_EXIT;
    }

DEVREGISTER_EXIT:
    DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR, "%s - <== Status: 0x%08x\n", __FUNCTION__, Status));
    return Status;
}


VOID FwCtrlDevUnregister(PIONIC_ADAPTER Adapter) {

    DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE, "%s - mnic%u ==>\n", __FUNCTION__, Adapter->nameIdx));

    if (Adapter->FwUpdateControlHandle != NULL) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE, "%s - NdisMDeregisterDevice device.\n", __FUNCTION__));
        NdisDeregisterDeviceEx(Adapter->FwUpdateControlHandle);
        Adapter->FwUpdateControlHandle = NULL;
        Adapter->FwUpdateCntrlDevObj = NULL;
    }
    DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE, "%s - <== mnic%u\n", __FUNCTION__, Adapter->nameIdx));
}
