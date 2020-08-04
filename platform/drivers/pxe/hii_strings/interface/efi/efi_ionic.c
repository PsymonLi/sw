/*
 * Copyright 2017-2020 Pensando Systems, Inc.  All rights reserved.
 *
 * This program is free software; you may redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include <string.h>
#include <errno.h>
#include <byteswap.h>
#include <ipxe/dhcp.h>
#include <ipxe/dhcpopts.h>
#include <ipxe/version.h>
#include <ipxe/list.h>
#include <ipxe/settings.h>
#include <ipxe/device.h>
#include <ipxe/netdevice.h>
#include <ipxe/pci.h>
#include <ipxe/init.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/efi_snp.h>
#include <ipxe/efi/efi_ionic.h>
#include <drivers/net/ionic.h>
#include <ipxe/pcivpd.h>
#include <ipxe/efi/efi_driver.h>
#include <ipxe/efi/efi_pci.h>
#include <ipxe/efi/Protocol/PciIo.h>
#include <ipxe/efi/Protocol/PciRootBridgeIo.h>

EFI_GUID gEfiionicHiiPackageInfoProtocol = EFI_IONIC_HII_PACKAGE_PROTOCOL_GUID;

NIC_HII_PACKAGE_INFO    *gNicHiiPackageInfo = NULL;
IONIC_NIC_HII_INFO        *gNicHiiInfo = NULL;

static CHAR8 FixPciId[] = "0x1002";
static CHAR8 LinkUp[] = "Link is up";
static CHAR8 LinkDown[] = "Link is down";
static CHAR8 VirMacAddr[] = "00:00:00:00:00:00";
static CHAR8 AscType0[] = "Capri";
static CHAR8 AscTypeUnknow[] = "Unknown";

UINT16 SnpMappedBaseKey[IONIC_MAX_NUMBER] = {
    IONIC_NIC_QUESTION_BASE_KEY ,
    IONIC_NIC_QUESTION_BASE_KEY + 0x100,
    IONIC_NIC_QUESTION_BASE_KEY + 0x200,
    IONIC_NIC_QUESTION_BASE_KEY + 0x300,
    IONIC_NIC_QUESTION_BASE_KEY + 0x400,
    IONIC_NIC_QUESTION_BASE_KEY + 0x500,
    IONIC_NIC_QUESTION_BASE_KEY + 0x600,
    IONIC_NIC_QUESTION_BASE_KEY + 0x700,
};

/** settings scope */
static const struct settings_scope ionic_settings_scope;

struct setting Firmware_Image __setting ( SETTING_IONIC, FirmwareImage ) = {
    .name = "FirmwareImage",
    .description = IONIC_FIRMWARE_IMAGE_INFO,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,    
};

struct setting Firmware_Version __setting ( SETTING_IONIC, FirmwareVer ) = {
    .name = "FirmwareVer",
    .description = IONIC_FIRMWARE_VERSION_INFO,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting Efi_Version __setting ( SETTING_IONIC, EfiVer ) = {
    .name = "EfiVer",
    .description = IONIC_EFI_VERSION_INFO,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting Nic_Config __setting    (SETTING_IONIC, NicConfig ) = {
    .name = "NicConfig",
    .description = IONIC_NIC_CONFIG_INFO,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,    
};

struct setting Vlan_Mode __setting ( SETTING_IONIC, VlanMode ) = {
    .name = "VlanMode",
    .description = IONIC_VLAN_MODE_INFO,
    .tag = IONIC_VLAN_MODE_QUESTION,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting Vlan_Id __setting ( SETTING_IONIC, VlanId ) = {
    .name = "VlanId",
    .description = IONIC_VLAN_ID_INFO,
    .tag = IONIC_VLAN_ID_QUESTION,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting Dev_Config __setting ( SETTING_IONIC, DevConfig ) = {
    .name = "DevConfig",
    .description = IONIC_DEV_LEV_INFO,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting Virtual_Mode __setting ( SETTING_IONIC, VirtualMode ) = {
    .name = "VirtualMode",
    .description = IONIC_VIRTUAL_MODE_INFO,
    .tag = IONIC_VIRTUAL_MODE_QUESTION,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting Virtual_Func __setting ( SETTING_IONIC, VirtualFunc ) = {
    .name = "VirtualFunc",
    .description = IONIC_VIRTUAL_FUNC_INFO,
    .tag = IONIC_VIRTUAL_FUNC_QUESTION,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting Bmc_Interface __setting ( SETTING_IONIC, BmcInterface ) = {
    .name = "BmcInterface",
    .description = IONIC_BMC_INTERFACE_INFO,
    .tag = IONIC_BMC_INTERFACE_QUESTION,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting Blink_Led __setting ( SETTING_IONIC, BlinkLed ) = {
    .name = "BlinkLed",
    .description = IONIC_BLINK_LED_INFO,
    .tag = IONIC_BLINK_LED_QUESTION,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting device_name __setting ( SETTING_IONIC, DevName ) = {
    .name = "DevName",
    .description = IONIC_DEV_NAME_INFO,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting chip_type __setting ( SETTING_IONIC, ChipType ) = {
    .name = "ChipType",
    .description = IONIC_CHIP_TYPE_INFO,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting pci_dev_id __setting ( SETTING_IONIC, PciDevId) = {
    .name = "PciDevId",
    .description = IONIC_PCI_DEV_ID_INFO,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting pci_addr __setting ( SETTING_IONIC, PciAddr ) = {
    .name = "PciAddr",
    .description = IONIC_PCI_ADDR_INFO,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting link_status __setting ( SETTING_IONIC, LinkStatus ) = {
    .name = "LinkStatus",
    .description = IONIC_LINK_STA_INFO,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting mac_addr __setting ( SETTING_IONIC, MacAddr ) = {
    .name = "MacAddr",
    .description = IONIC_MAC_ADDR_INFO,
    .tag = IONIC_MAC_ADDR_QUESTION,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

struct setting virtual_mac_addr __setting ( SETTING_IONIC, VirtualMacAddr ) = {
    .name = "VirtualMacAddr",
    .description = IONIC_VIRTUAL_MAC_INFO,
    .tag = IONIC_VIRTUAL_MAC_QUESTION,
    .type = &setting_type_uint8,
    .scope = &ionic_settings_scope,
};

/**
 * Check applicability of setting
 *
 * @v settings        Settings block
 * @v setting        Setting
 * @ret applies        Setting applies within this settings block
 */
static int ionic_applies ( struct settings *settings __unused,
            const struct setting *setting ) {

    return ( setting->scope == &ionic_settings_scope );
}

static int ionic_fetch_vlan_mode ( void *data, size_t len ) {

    static UINT8 buffer[10];

    buffer[0] = 1;

    memcpy ( data, &buffer[0], len );

    return sizeof(UINT8);
}

static int ionic_fetch_vlan_id ( void *data, size_t len ) {

    static UINT8 buffer[10];

    buffer[0] = 4;

    memcpy ( data, &buffer[0], len );

    return sizeof(UINT8);
}

static int ionic_fetch_virtual_mode ( void *data, size_t len ) {

    static UINT8 buffer[10];

    buffer[0] = 5;

    memcpy ( data, &buffer[0], len );

    return sizeof(UINT8);

}

static int ionic_fetch_virtual_func ( void *data, size_t len ) {

    static UINT8 buffer[10];

    buffer[0] = 6;

    memcpy ( data, &buffer[0], len );

    return sizeof(UINT8);
}

static int ionic_fetch_bmc_interface ( void *data, size_t len ) {

    static UINT8 buffer[10];

    buffer[0] = 7;
    memcpy ( data, &buffer[0], len );
    return sizeof(UINT8);
}

static int ionic_fetch_blink_led ( void *data, size_t len ) {

    static UINT8 buffer[10];

    buffer[0] = 9;
    memcpy ( data, &buffer[0], len );
    return sizeof(UINT8);
}

/** A network device setting operation */
struct ionic_setting_operation {
    /** Setting */
    const struct setting *setting;
    int ( * store ) (const void *data,size_t len );
    int ( * fetch ) (void *data, size_t len );
};

/** ionic settings */
static struct ionic_setting_operation ionic_setting_operations[] = {
    { &Vlan_Mode, NULL, ionic_fetch_vlan_mode },
    { &Vlan_Id, NULL, ionic_fetch_vlan_id },
    { &Virtual_Mode, NULL, ionic_fetch_virtual_mode },
    { &Virtual_Func, NULL, ionic_fetch_virtual_func },
    { &Bmc_Interface, NULL, ionic_fetch_bmc_interface },
    { &Blink_Led, NULL, ionic_fetch_blink_led },
};


/**
 * Fetch value of network device setting
 *
 * @v settings        Settings block
 * @v setting        Setting to fetch
 * @v data        Buffer to fill with setting data
 * @v len        Length of buffer
 * @ret len        Length of setting data, or negative error
 */
static int ionic_fetch ( struct settings *settings __unused,
            struct setting *setting,
            void *data, size_t len ) {

    struct ionic_setting_operation *op;
    unsigned int i;

    for ( i = 0 ; i < ( sizeof ( ionic_setting_operations ) /sizeof ( ionic_setting_operations[0] ) ) ; i++ )
    {
        op = &ionic_setting_operations[i];
        if ( setting_cmp ( setting, op->setting ) == 0 ){
            return op->fetch ( data, len );
        }
    }    

    return 0;
}

static struct settings_operations ionic_settings_operations = {
    .applies = ionic_applies,
    .fetch = ionic_fetch,
};


/** "ionic" settings */
static struct settings ionic_settings = {
    .refcnt = NULL,
    .siblings = LIST_HEAD_INIT ( ionic_settings.siblings ),
    .children = LIST_HEAD_INIT ( ionic_settings.children ),
    .op = &ionic_settings_operations,
    .default_scope = &ionic_settings_scope,
};

void *
GetMatchedSnpDev(UINT16 SnpIndexVar){
    EFI_STATUS     Status = EFI_SUCCESS;
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_HANDLE    *HandleBuffer = NULL;
    UINTN        HandleNum = 0;
    UINT32        Index;
    UINTN        i;
    NIC_HII_PACKAGE_INFO    *NicPackage = NULL;

    for(i=0 ; i <IONIC_MAX_NUMBER; i++) {
        if( SnpMappedBaseKey[i] == SnpIndexVar) break;
    }

    if(i == IONIC_MAX_NUMBER) return NULL;

    Status = bs->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiionicHiiPackageInfoProtocol,
                    NULL,
                    &HandleNum,
                    &HandleBuffer
                    );

    if (EFI_ERROR (Status)) {
        return NULL;
    }

    for(Index = 0; Index < HandleNum; Index++) {
        Status = bs->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiionicHiiPackageInfoProtocol,
                    (VOID **) &NicPackage
                );
        if (!EFI_ERROR (Status)) {
            if(NicPackage->SnpIndex == i){
                return (void *)NicPackage->SnpDev;
            }
        }
    }

    if (HandleBuffer != NULL) {
        bs->FreePool(HandleBuffer);
    }
    return NULL;
}

UINT8 GetHiiPackageInfoIndex(){
    EFI_STATUS     Status = EFI_SUCCESS;
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_HANDLE    *HandleBuffer = NULL;
    UINTN        HandleNum = 0;

    Status = bs->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiionicHiiPackageInfoProtocol,
                    NULL,
                    &HandleNum,
                    &HandleBuffer
                    );

    if (EFI_ERROR (Status)) {
        return 0;//no handle , mean this is the fist index
    }

    if (HandleBuffer != NULL) {
        bs->FreePool(HandleBuffer);
    }
    return HandleNum;
}


EFI_STATUS EFIAPI
firmware_version_callback (
    IN        UINT16        SnpIndex __unused,
    IN OUT    CHAR8        **Buffer,
    IN OUT    UINT8        *BufferLen
) {
    EFI_STATUS Status = EFI_NOT_FOUND;
    unsigned int i, j;
    struct efi_snp_device *snpdev = (struct efi_snp_device *)gNicHiiPackageInfo->SnpDev;
    struct net_device *netdev = snpdev->netdev;
    struct ionic *ionic = netdev->priv;
    struct ionic_device_bar *bars = ionic->bars;

    if(snpdev == NULL) return Status;

    for (i = 0, j = 0; i < IONIC_IPXE_BARS_MAX; i++) {
        if(((DEV_INFO_REGS *)(bars[j].vaddr))->signature == IONIC_DEV_INFO_SIGNATURE)
        {
            *Buffer = &(((DEV_INFO_REGS *)(bars[j].vaddr))->fw_version[0]);
            *BufferLen = 32;
            return EFI_SUCCESS;
        }
        j++;
    }
    return Status;
}

EFI_STATUS EFIAPI
efi_version_callback (
    IN        UINT16        SnpIndex __unused,
    IN OUT    CHAR8        **Buffer,
    IN OUT    UINT8        *BufferLen
) {

    static char buf[16];

    snprintf ( buf, sizeof ( buf ), "iPXE %s", product_version);

    *Buffer = &buf[0];
    *BufferLen = sizeof(buf);
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI
device_name_callback (    
    IN        UINT16    SnpIndex __unused,
    IN OUT    CHAR8    **Buffer,
    IN OUT    UINT8    *BufferLen
) {
    int    rc;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    unsigned int rw_address;
    size_t rw_len;
    static char dev_buf[100];
    struct efi_snp_device *snpdev = (struct efi_snp_device *)gNicHiiPackageInfo->SnpDev;
    struct net_device *netdev = snpdev->netdev;
    struct device *dev = netdev->dev;
    struct pci_vpd vpd;
    struct pci_device *pci     =
    container_of ( dev, struct pci_device, dev );

    if(snpdev == NULL) return Status;

    bs->SetMem(&dev_buf[0],sizeof ( dev_buf ),0);

    rc = pci_vpd_init ( &vpd, pci );

    /* Initialise VPD device */
    if ( rc != 0 ) {
        return rc;
    }

    rc = pci_vpd_find ( &vpd, PCI_VPD_FIELD_NAME, &rw_address,&rw_len );
    if( rc == 0 ){
            rc = pci_vpd_read ( &vpd, rw_address, dev_buf,sizeof ( dev_buf ) );
            if ( rc != 0 ) return EFI_NOT_FOUND;
            bs->SetMem(&dev_buf[rw_len],sizeof ( dev_buf )-rw_len,0);
            *Buffer = &dev_buf[0];
            *BufferLen = rw_len;
    }
    return Status;
}

EFI_STATUS EFIAPI
chip_type_callback (
    IN        UINT16    SnpIndex __unused,
    IN OUT    CHAR8    **Buffer,
    IN OUT    UINT8    *BufferLen
) {
    EFI_STATUS Status = EFI_NOT_FOUND;
    unsigned int i, j;
    struct efi_snp_device *snpdev = (struct efi_snp_device *)gNicHiiPackageInfo->SnpDev;
    struct net_device *netdev = snpdev->netdev;
    struct ionic *ionic = netdev->priv;
    struct ionic_device_bar *bars = ionic->bars;

    if(snpdev == NULL) return Status;

    for (i = 0, j = 0; i < IONIC_IPXE_BARS_MAX; i++) {
        if(((DEV_INFO_REGS *)(bars[j].vaddr))->signature == IONIC_DEV_INFO_SIGNATURE)
        {
            if(((DEV_INFO_REGS *)(bars[j].vaddr))->asic_type == IONIC_ASIC_TYPE_CAPRI){
                *Buffer = &AscType0[0];
                *BufferLen = sizeof(AscType0);
                Status = EFI_SUCCESS;
            }else{
                *Buffer = &AscTypeUnknow[0];
                *BufferLen = sizeof(AscTypeUnknow);
                Status = EFI_SUCCESS;
            }
            break;
        }
        j++;
    }

    return Status;
}

EFI_STATUS EFIAPI
pci_dev_id_callback (
    IN        UINT16    SnpIndex __unused,
    IN OUT    CHAR8    **Buffer,
    IN OUT    UINT8    *BufferLen
) {
    *Buffer = &FixPciId[0];
    *BufferLen = sizeof(FixPciId);
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI
pci_addr_callback (
    IN        UINT16    SnpIndex __unused,
    IN OUT    CHAR8    **Buffer,
    IN OUT    UINT8    *BufferLen
) {
    EFI_STATUS Status = EFI_NOT_FOUND;
    struct efi_snp_device *snpdev = (struct efi_snp_device *)gNicHiiPackageInfo->SnpDev;
    struct net_device *netdev = snpdev->netdev;
    struct device *dev = netdev->dev;

    if(snpdev == NULL) return Status;

    *Buffer = &dev->name[0];
    *BufferLen = sizeof(dev->name);

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI
link_status_callback (
    IN        UINT16    SnpIndex __unused,
    IN OUT    CHAR8    **Buffer,
    IN OUT    UINT8    *BufferLen
) {
    EFI_STATUS Status = EFI_NOT_FOUND;
    struct efi_snp_device *snpdev = (struct efi_snp_device *)gNicHiiPackageInfo->SnpDev;
    struct net_device *netdev = snpdev->netdev;

    if(snpdev == NULL) return Status;

    if(netdev->link_rc == 0){
        *Buffer = &LinkUp[0];
        *BufferLen = sizeof(LinkUp);
    }else{
        *Buffer = &LinkDown[0];
        *BufferLen = sizeof(LinkUp);
    }
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI
mac_addr_callback (
    IN        UINT16    SnpIndex __unused,
    IN OUT    CHAR8    **Buffer,
    IN OUT    UINT8    *BufferLen
) {
    EFI_STATUS Status = EFI_NOT_FOUND;
    struct efi_snp_device *snpdev = (struct efi_snp_device *)gNicHiiPackageInfo->SnpDev;
    struct net_device *netdev = snpdev->netdev;

    if(snpdev == NULL) return Status;

    *Buffer = (CHAR8 *)netdev_addr ( netdev );
    *BufferLen = 18;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI
virtual_mac_addr_callback (
    IN        UINT16    SnpIndex __unused,
    IN OUT    CHAR8    **Buffer,
    IN OUT    UINT8    *BufferLen
) {

    *Buffer = &VirMacAddr[0];
    *BufferLen = sizeof(VirMacAddr);
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI
vlan_mode_callback (
    IN        UINT16    SnpIndex __unused,
    IN OUT    CHAR8    **Buffer __unused,
    IN OUT    UINT8    *BufferLen __unused
) {
    EFI_STATUS Status = EFI_NOT_FOUND;
    return Status;
}

EFI_STATUS EFIAPI
vlan_id_callback (
    IN        UINT16    SnpIndex,
    IN OUT    CHAR8    **Buffer __unused,
    IN OUT    UINT8    *BufferLen __unused
) {
    EFI_STATUS Status = EFI_SUCCESS;
    struct efi_snp_device *snpdev = (struct efi_snp_device *)GetMatchedSnpDev(SnpIndex);
    struct net_device *netdev;

    if(snpdev == NULL) return Status;

    netdev = snpdev->netdev;

    //<TOD> Workaound for grayout Vlan mode
    if(gNicHiiPackageInfo->VlanModeVar == 1){
        ionic_add_vlan_cb(netdev, (u32)gNicHiiPackageInfo->VlanIdVar,1);
    }

    return Status;
}

EFI_STATUS EFIAPI
virtual_mode_callback (
    IN        UINT16    SnpIndex __unused,
    IN OUT    CHAR8    **Buffer __unused,
    IN OUT    UINT8    *BufferLen __unused
) {
    EFI_STATUS Status = EFI_NOT_FOUND;
    return Status;
}

EFI_STATUS EFIAPI
virtual_func_callback (
    IN        UINT16    SnpIndex __unused,
    IN OUT    CHAR8    **Buffer __unused,
    IN OUT    UINT8    *BufferLen __unused
) {
    EFI_STATUS Status = EFI_NOT_FOUND;
    return Status;
}

EFI_STATUS EFIAPI
bmc_interface_callback (
    IN        UINT16    SnpIndex,
    IN OUT    CHAR8    **Buffer __unused,
    IN OUT    UINT8    *BufferLen __unused
) {
    EFI_STATUS Status = EFI_SUCCESS;
    struct efi_snp_device *snpdev = (struct efi_snp_device *)GetMatchedSnpDev(SnpIndex);
    struct net_device *netdev;

    if(snpdev == NULL) return Status;

    netdev = snpdev->netdev;

    ionic_oob_en_cb(netdev, gNicHiiPackageInfo->BmcInterfaceVar);
    return Status;
}

EFI_STATUS EFIAPI
blink_led_callback (
    IN        UINT16    SnpIndex,
    IN OUT    CHAR8    **Buffer __unused,
    IN OUT    UINT8    *BufferLen __unused
) {
    EFI_STATUS Status = EFI_SUCCESS;
    struct efi_snp_device *snpdev = (struct efi_snp_device *)GetMatchedSnpDev(SnpIndex);
    struct net_device *netdev;

    if(snpdev == NULL) return Status;

    netdev = snpdev->netdev;
    ionic_set_system_led_cb(netdev, (bool) gNicHiiPackageInfo->BlinkLedVar);
    return Status;
}

void *
ionic_hii_init (void *snp) {

    EFI_STATUS     Status = EFI_SUCCESS;
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_HANDLE    Handle = NULL;
    UINT8    PackageIndex;

    PackageIndex = GetHiiPackageInfoIndex();

    Status = bs->AllocatePool (EfiBootServicesData,
                             sizeof(NIC_HII_PACKAGE_INFO),
                             (void **)&gNicHiiPackageInfo );

    if(Status != EFI_SUCCESS) return NULL;

    Status = bs->AllocatePool (EfiBootServicesData,
                             sizeof(IONIC_NIC_HII_INFO),
                             (void **)&gNicHiiInfo );

    if(Status != EFI_SUCCESS) {
        bs->FreePool(gNicHiiPackageInfo);
        return NULL;
    }

    // clear allocated memory
    bs->SetMem(gNicHiiPackageInfo,sizeof ( NIC_HII_PACKAGE_INFO ),0);
    bs->SetMem(gNicHiiInfo,sizeof ( IONIC_NIC_HII_INFO ),0);

    gNicHiiPackageInfo->SnpIndex = PackageIndex;

    gNicHiiPackageInfo->NicHiiInfo = (void *)gNicHiiInfo;

    gNicHiiPackageInfo->SnpDev = snp;

    gNicHiiInfo->NicName.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->NicName.Setting = NULL;
    gNicHiiInfo->NicName.PromptStr = IONIC_NIC_DEV_NAME_INFO;
    gNicHiiInfo->NicName.HelpStr = NULL;
    gNicHiiInfo->NicName.ValueStr = NULL;
    gNicHiiInfo->NicName.Callback = NULL;
    gNicHiiInfo->NicName.Show = TRUE;

    //go-to-item
    gNicHiiInfo->FirmwareInfo.FormId = IONIC_FIRMWARE_IMAGE_FORM;
    gNicHiiInfo->FirmwareInfo.Type = EFI_IFR_REF_OP;
    gNicHiiInfo->FirmwareInfo.Setting = &Firmware_Image;
    gNicHiiInfo->FirmwareInfo.PromptStr = IONIC_FIRMWARE_IMAGE_INFO;
    gNicHiiInfo->FirmwareInfo.HelpStr = IONIC_FIRMWARE_IMAGE_HELP;
    gNicHiiInfo->FirmwareInfo.ValueStr = NULL;
    gNicHiiInfo->FirmwareInfo.Callback = NULL;
    gNicHiiInfo->FirmwareInfo.Show = TRUE;

    gNicHiiInfo->FirmwareVer.FormId = IONIC_FIRMWARE_IMAGE_FORM;
    gNicHiiInfo->FirmwareVer.Type = EFI_IFR_TEXT_OP;
    gNicHiiInfo->FirmwareVer.Setting = &Firmware_Version;
    gNicHiiInfo->FirmwareVer.PromptStr = IONIC_FIRMWARE_VERSION_INFO;
    gNicHiiInfo->FirmwareVer.HelpStr = IONIC_FIRMWARE_VERSION_HELP;
    gNicHiiInfo->FirmwareVer.ValueStr = IONIC_HII_NONE_VALUE;
    gNicHiiInfo->FirmwareVer.Callback = firmware_version_callback;
    gNicHiiInfo->FirmwareVer.Show = TRUE;

    gNicHiiInfo->EfiVer.FormId = IONIC_FIRMWARE_IMAGE_FORM;
    gNicHiiInfo->EfiVer.Type = EFI_IFR_TEXT_OP;
    gNicHiiInfo->EfiVer.Setting = &Efi_Version;
    gNicHiiInfo->EfiVer.PromptStr = IONIC_EFI_VERSION_INFO;
    gNicHiiInfo->EfiVer.HelpStr = IONIC_EFI_VERSION_HELP;
    gNicHiiInfo->EfiVer.ValueStr = IONIC_HII_NONE_VALUE;
    gNicHiiInfo->EfiVer.Callback = efi_version_callback;
    gNicHiiInfo->EfiVer.Show = TRUE;

    //go-to-item
    gNicHiiInfo->NicConfig.FormId = IONIC_NIC_CONFIG_FORM;
    gNicHiiInfo->NicConfig.Type = EFI_IFR_REF_OP;
    gNicHiiInfo->NicConfig.Setting = &Nic_Config;
    gNicHiiInfo->NicConfig.PromptStr = IONIC_NIC_CONFIG_INFO;
    gNicHiiInfo->NicConfig.HelpStr = IONIC_NIC_CONFIG_HELP;
    gNicHiiInfo->NicConfig.ValueStr = NULL;
    gNicHiiInfo->NicConfig.Callback = NULL;
    gNicHiiInfo->NicConfig.Show = TRUE;

    gNicHiiInfo->VlanMode.FormId = IONIC_NIC_CONFIG_FORM;
    gNicHiiInfo->VlanMode.Type = EFI_IFR_ONE_OF_OP;
    gNicHiiInfo->VlanMode.Setting = &Vlan_Mode;
    gNicHiiInfo->VlanMode.PromptStr = IONIC_VLAN_MODE_INFO;
    gNicHiiInfo->VlanMode.HelpStr = IONIC_VLAN_MODE_HELP;
    gNicHiiInfo->VlanMode.ValueStr = NULL;
    gNicHiiInfo->VlanMode.Callback = vlan_mode_callback;
    gNicHiiInfo->VlanMode.Show = TRUE;

    gNicHiiInfo->VlanModeEnable.FormId = IONIC_NIC_CONFIG_FORM;
    gNicHiiInfo->VlanModeEnable.Type = EFI_IFR_ONE_OF_OPTION_OP;
    gNicHiiInfo->VlanModeEnable.Setting = NULL;
    gNicHiiInfo->VlanModeEnable.PromptStr = IONIC_ENABLE_INFO;
    gNicHiiInfo->VlanModeEnable.HelpStr = IONIC_ENABLE_HELP;
    gNicHiiInfo->VlanModeEnable.ValueStr = NULL;
    gNicHiiInfo->VlanModeEnable.Callback = NULL;
    gNicHiiInfo->VlanModeEnable.Flag = EFI_IFR_OPTION_DEFAULT;
    gNicHiiInfo->VlanModeEnable.Value = IONIC_ENABLE_VALUE;
    gNicHiiInfo->VlanModeEnable.Show = TRUE;

    gNicHiiInfo->VlanModeDisable.FormId = IONIC_NIC_CONFIG_FORM;
    gNicHiiInfo->VlanModeDisable.Type = EFI_IFR_ONE_OF_OPTION_OP;
    gNicHiiInfo->VlanModeDisable.Setting = NULL;
    gNicHiiInfo->VlanModeDisable.PromptStr = IONIC_DISABLE_INFO;
    gNicHiiInfo->VlanModeDisable.HelpStr = IONIC_DISABLE_HELP;
    gNicHiiInfo->VlanModeDisable.ValueStr = NULL;
    gNicHiiInfo->VlanModeDisable.Callback = NULL;
    gNicHiiInfo->VlanModeDisable.Flag = 0;
    gNicHiiInfo->VlanModeDisable.Value = IONIC_DISABLE_VALUE;
    gNicHiiInfo->VlanModeDisable.Show = TRUE;

    gNicHiiInfo->VlanId.FormId = IONIC_NIC_CONFIG_FORM;
    gNicHiiInfo->VlanId.Type = EFI_IFR_NUMERIC_OP;
    gNicHiiInfo->VlanId.Setting = &Vlan_Id;
    gNicHiiInfo->VlanId.PromptStr = IONIC_VLAN_ID_INFO;
    gNicHiiInfo->VlanId.HelpStr = IONIC_VLAN_ID_HELP;
    gNicHiiInfo->VlanId.ValueStr = NULL;
    gNicHiiInfo->VlanId.Callback = vlan_id_callback;
    gNicHiiInfo->VlanId.Show = TRUE;

    //go-to-item
    gNicHiiInfo->DevLevel.FormId = IONIC_DEV_LEV_FORM;
    gNicHiiInfo->DevLevel.Type = EFI_IFR_REF_OP;
    gNicHiiInfo->DevLevel.Setting = &Dev_Config;
    gNicHiiInfo->DevLevel.PromptStr = IONIC_DEV_LEV_INFO;
    gNicHiiInfo->DevLevel.HelpStr = IONIC_DEV_LEV_HELP;
    gNicHiiInfo->DevLevel.ValueStr = NULL;
    gNicHiiInfo->DevLevel.Callback = NULL;
    gNicHiiInfo->DevLevel.Show = TRUE;

    gNicHiiInfo->VirtualMode.FormId = IONIC_DEV_LEV_FORM;
    gNicHiiInfo->VirtualMode.Type = EFI_IFR_CHECKBOX_OP;
    gNicHiiInfo->VirtualMode.Setting = &Virtual_Mode;
    gNicHiiInfo->VirtualMode.PromptStr = IONIC_VIRTUAL_MODE_INFO;
    gNicHiiInfo->VirtualMode.HelpStr = IONIC_VIRTUAL_MODE_HELP;
    gNicHiiInfo->VirtualMode.ValueStr = NULL;
    gNicHiiInfo->VirtualMode.Callback = virtual_mode_callback;
    gNicHiiInfo->VirtualMode.Show = FALSE;

    gNicHiiInfo->VirtualFunc.FormId = IONIC_DEV_LEV_FORM;
    gNicHiiInfo->VirtualFunc.Type = EFI_IFR_NUMERIC_OP;
    gNicHiiInfo->VirtualFunc.Setting = &Virtual_Func;
    gNicHiiInfo->VirtualFunc.PromptStr = IONIC_VIRTUAL_FUNC_INFO;
    gNicHiiInfo->VirtualFunc.HelpStr = IONIC_VIRTUAL_FUNC_HELP;
    gNicHiiInfo->VirtualFunc.ValueStr = NULL;
    gNicHiiInfo->VirtualFunc.Callback = virtual_func_callback;
    gNicHiiInfo->VirtualFunc.Show = FALSE;

    gNicHiiInfo->BmcInterface.FormId = IONIC_DEV_LEV_FORM;
    gNicHiiInfo->BmcInterface.Type = EFI_IFR_ONE_OF_OP;
    gNicHiiInfo->BmcInterface.Setting = &Bmc_Interface;
    gNicHiiInfo->BmcInterface.PromptStr = IONIC_BMC_INTERFACE_INFO;
    gNicHiiInfo->BmcInterface.HelpStr = IONIC_BMC_INTERFACE_HELP;
    gNicHiiInfo->BmcInterface.ValueStr = NULL;
    gNicHiiInfo->BmcInterface.Callback = bmc_interface_callback;
    gNicHiiInfo->BmcInterface.Show = TRUE;

    gNicHiiInfo->OutofBandManage.FormId = IONIC_DEV_LEV_FORM;
    gNicHiiInfo->OutofBandManage.Type = EFI_IFR_ONE_OF_OPTION_OP;
    gNicHiiInfo->OutofBandManage.Setting = NULL;
    gNicHiiInfo->OutofBandManage.PromptStr = IONIC_OUT_OF_BAND_MANAGEMENT_INFO;
    gNicHiiInfo->OutofBandManage.HelpStr = IONIC_OUT_OF_BAND_MANAGEMENT_HELP;
    gNicHiiInfo->OutofBandManage.ValueStr = NULL;
    gNicHiiInfo->OutofBandManage.Callback = NULL;
    gNicHiiInfo->OutofBandManage.Flag = EFI_IFR_OPTION_DEFAULT;
    gNicHiiInfo->OutofBandManage.Value = IONIC_OUT_OF_BAND_MANAGEMENT_VALUE;
    gNicHiiInfo->OutofBandManage.Show = TRUE;

    gNicHiiInfo->SideBandInterface.FormId = IONIC_DEV_LEV_FORM;
    gNicHiiInfo->SideBandInterface.Type = EFI_IFR_ONE_OF_OPTION_OP;
    gNicHiiInfo->SideBandInterface.Setting = NULL;
    gNicHiiInfo->SideBandInterface.PromptStr = IONIC_SIDE_BAND_INTERFACE_INFO;
    gNicHiiInfo->SideBandInterface.HelpStr = IONIC_SIDE_BAND_INTERFACE_HELP;
    gNicHiiInfo->SideBandInterface.ValueStr = NULL;
    gNicHiiInfo->SideBandInterface.Callback = NULL;
    gNicHiiInfo->SideBandInterface.Flag = 0;
    gNicHiiInfo->SideBandInterface.Value = IONIC_SIDE_BAND_INTERFACE_VALUE;
    gNicHiiInfo->SideBandInterface.Show = TRUE;

    gNicHiiInfo->BLed.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->BLed.Type = EFI_IFR_ONE_OF_OP;
    gNicHiiInfo->BLed.Setting = &Blink_Led;
    gNicHiiInfo->BLed.PromptStr = IONIC_BLINK_LED_INFO;
    gNicHiiInfo->BLed.HelpStr = IONIC_BLINK_LED_HELP;
    gNicHiiInfo->BLed.ValueStr = NULL;
    gNicHiiInfo->BLed.Callback = blink_led_callback;
    gNicHiiInfo->BLed.Show = TRUE;

    gNicHiiInfo->BLedEnable.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->BLedEnable.Type = EFI_IFR_ONE_OF_OPTION_OP;
    gNicHiiInfo->BLedEnable.Setting = NULL;
    gNicHiiInfo->BLedEnable.PromptStr = IONIC_ENABLE_INFO;
    gNicHiiInfo->BLedEnable.HelpStr = IONIC_ENABLE_HELP;
    gNicHiiInfo->BLedEnable.ValueStr = NULL;
    gNicHiiInfo->BLedEnable.Callback = NULL;
    gNicHiiInfo->BLedEnable.Flag = EFI_IFR_OPTION_DEFAULT;
    gNicHiiInfo->BLedEnable.Value = IONIC_ENABLE_VALUE;
    gNicHiiInfo->BLedEnable.Show = TRUE;

    gNicHiiInfo->BLedDisable.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->BLedDisable.Type = EFI_IFR_ONE_OF_OPTION_OP;
    gNicHiiInfo->BLedDisable.Setting = NULL;
    gNicHiiInfo->BLedDisable.PromptStr = IONIC_DISABLE_INFO;
    gNicHiiInfo->BLedDisable.HelpStr = IONIC_DISABLE_HELP;
    gNicHiiInfo->BLedDisable.ValueStr = NULL;
    gNicHiiInfo->BLedDisable.Callback = NULL;
    gNicHiiInfo->BLedDisable.Flag = 0;
    gNicHiiInfo->BLedDisable.Value = IONIC_DISABLE_VALUE;
    gNicHiiInfo->BLedDisable.Show = TRUE;

    gNicHiiInfo->DevName.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->DevName.Type = EFI_IFR_TEXT_OP;
    gNicHiiInfo->DevName.Setting = &device_name;
    gNicHiiInfo->DevName.PromptStr = IONIC_DEV_NAME_INFO;
    gNicHiiInfo->DevName.HelpStr = IONIC_DEV_NAME_HELP;
    gNicHiiInfo->DevName.ValueStr = IONIC_DEV_NAME_VALUE;
    gNicHiiInfo->DevName.Callback = device_name_callback;
    gNicHiiInfo->DevName.Show = TRUE;

    gNicHiiInfo->ChipType.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->ChipType.Type = EFI_IFR_TEXT_OP;
    gNicHiiInfo->ChipType.Setting = &chip_type;
    gNicHiiInfo->ChipType.PromptStr = IONIC_CHIP_TYPE_INFO;
    gNicHiiInfo->ChipType.HelpStr = IONIC_CHIP_TYPE_HELP;
    gNicHiiInfo->ChipType.ValueStr = IONIC_CHIP_TYPE_VALUE;
    gNicHiiInfo->ChipType.Callback = chip_type_callback;
    gNicHiiInfo->ChipType.Show = TRUE;

    gNicHiiInfo->PciDevId.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->PciDevId.Type = EFI_IFR_TEXT_OP;
    gNicHiiInfo->PciDevId.Setting = &pci_dev_id;
    gNicHiiInfo->PciDevId.PromptStr = IONIC_PCI_DEV_ID_INFO;
    gNicHiiInfo->PciDevId.HelpStr = IONIC_PCI_DEV_ID_HELP;
    gNicHiiInfo->PciDevId.ValueStr = IONIC_PCI_DEV_ID_VALUE;
    gNicHiiInfo->PciDevId.Callback = pci_dev_id_callback;
    gNicHiiInfo->PciDevId.Show = TRUE;

    gNicHiiInfo->PciAddr.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->PciAddr.Type = EFI_IFR_TEXT_OP;
    gNicHiiInfo->PciAddr.Setting = &pci_addr;
    gNicHiiInfo->PciAddr.PromptStr = IONIC_PCI_ADDR_INFO;
    gNicHiiInfo->PciAddr.HelpStr = IONIC_PCI_ADDR_HELP;
    gNicHiiInfo->PciAddr.ValueStr = IONIC_PCI_ADDR_VALUE;
    gNicHiiInfo->PciAddr.Callback = pci_addr_callback;;
    gNicHiiInfo->PciAddr.Show = TRUE;

    gNicHiiInfo->LinkStatus.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->LinkStatus.Type = EFI_IFR_TEXT_OP;
    gNicHiiInfo->LinkStatus.Setting = &link_status;
    gNicHiiInfo->LinkStatus.PromptStr = IONIC_LINK_STA_INFO;
    gNicHiiInfo->LinkStatus.HelpStr = IONIC_LINK_STA_HELP;
    gNicHiiInfo->LinkStatus.ValueStr = IONIC_LINK_STA_VALUE;
    gNicHiiInfo->LinkStatus.Callback = link_status_callback;;
    gNicHiiInfo->LinkStatus.Show = TRUE;

    gNicHiiInfo->MacAddr.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->MacAddr.Type = EFI_IFR_TEXT_OP;
    gNicHiiInfo->MacAddr.Setting = &mac_addr;
    gNicHiiInfo->MacAddr.PromptStr = IONIC_MAC_ADDR_INFO;
    gNicHiiInfo->MacAddr.HelpStr = IONIC_MAC_ADDR_HELP;
    gNicHiiInfo->MacAddr.ValueStr = IONIC_MAC_ADDR_VALUE;
    gNicHiiInfo->MacAddr.Callback = mac_addr_callback;;
    gNicHiiInfo->MacAddr.Show = TRUE;

    gNicHiiInfo->VirtualMacAddr.FormId = IONIC_NIC_DEV_FORM;
    gNicHiiInfo->VirtualMacAddr.Type = EFI_IFR_TEXT_OP;
    gNicHiiInfo->VirtualMacAddr.Setting = &virtual_mac_addr;
    gNicHiiInfo->VirtualMacAddr.PromptStr = IONIC_VIRTUAL_MAC_INFO;
    gNicHiiInfo->VirtualMacAddr.HelpStr = IONIC_VIRTUAL_MAC_HELP;
    gNicHiiInfo->VirtualMacAddr.ValueStr = IONIC_VIRTUAL_MAC_VALUE;
    gNicHiiInfo->VirtualMacAddr.Callback = virtual_mac_addr_callback;;
    gNicHiiInfo->VirtualMacAddr.Show = TRUE;

    Status = bs->InstallMultipleProtocolInterfaces (
            &Handle,
            &gEfiionicHiiPackageInfoProtocol,
            gNicHiiPackageInfo,
            NULL);

    if ( Status != EFI_SUCCESS ) {
        bs->FreePool(gNicHiiPackageInfo);
        bs->FreePool(gNicHiiInfo);
        return NULL;
    }

    gNicHiiPackageInfo->Handle = Handle;
    return (void *)gNicHiiPackageInfo;
}

/** Initialise "ionic" settings */
static void ionic_settings_init ( void ) {
    register_settings ( &ionic_settings, NULL,"ionic" );
}

/** "ionic" settings initialiser */
struct init_fn ionic_settings_init_fn __init_fn ( INIT_NORMAL ) = {
    .initialise = ionic_settings_init,
};

static int ionic_create_routes ( void ) {
    return 0;
}

struct settings_applicator ionic_setting_applicator __settings_applicator = {
    .apply = ionic_create_routes,
};

void ionic_test(){
}
