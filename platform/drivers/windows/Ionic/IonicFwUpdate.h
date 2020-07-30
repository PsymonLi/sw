#ifndef _IONICFWUPDATE_H_
#define _IONICFWUPDATE_H_

typedef struct _FW_UPDATE_DATA {
    LIST_ENTRY          FwUpdateListEntry;
    IONIC_WORKER_THREAD FwUpdateThread;
    KSEMAPHORE          FwUpdateSemaphore;
    PIRP                FwUpdateIrp;
} FW_UPDATE_DATA, *PFW_UPDATE_DATA;

NTSTATUS
ionic_firmware_download_adminq(struct lif* lif, uint64_t addr, uint32_t offset, uint32_t length);

NTSTATUS
ionic_firmware_install_adminq(struct lif* lif, uint8_t* slot);

NTSTATUS
ionic_firmware_activate_adminq(struct lif* lif, uint8_t slot);

NTSTATUS
ionic_firmware_update_adminq(struct ionic* adapter, const void* const fw_data, uint32_t fw_sz);

NTSTATUS
ionic_firmware_update_devcmd(struct ionic* adapter, const void* const fw_data, uint32_t fw_sz);

NTSTATUS
ionic_firmware_update(struct ionic* adapter, PVOID FwBuf, ULONG FwBufLen, PULONG pReadBytes);

NTSTATUS
IoctlFwUpdate(PIRP Irp, PULONG outbytes);

NTSTATUS
IoctlFwControlUpdate(struct ionic* adapter, PIRP Irp, PULONG outbytes);

#endif
