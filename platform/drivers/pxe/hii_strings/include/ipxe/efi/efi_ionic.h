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
#ifndef _EFI_IONIC_H_
#define _EFI_IONIC_H_


#define EFI_IONIC_HII_PACKAGE_PROTOCOL_GUID \
  { \
  0x76a91526, 0x6e6e,0x471d, {0x82, 0xc1, 0x9c, 0x81, 0xcb, 0xbe,0x1a, 0x6c } \
  }

#define EFI_EVENT_GROUP_READY_TO_BOOT \
  { 0x7ce88fb3, 0x4bd7, 0x4679, { 0x87, 0xa8, 0xa8, 0xd8, 0xde, 0xe5, 0x0d, 0x2b } }

#define SNP_BASE_KEY_OFFSET           0X100

#define EFI_STRING_ID	UINT16

#define IONIC_NIC_QUESTION_BASE_KEY   0xA000

#define IONIC_NIC_BASE_FORM_ID        0

// Customer Hii string definiton
#define IONIC_HII_NONE_VALUE            "N/A"
#define IONIC_HII_ENABLE_VALUE          "Enable"
#define IONIC_HII_DISABLE_VALUE         "Disable"

#define IONIC_NIC_DEV_FORM            IONIC_NIC_BASE_FORM_ID + 1
#define IONIC_NIC_DEV_NAME_INFO       "IONIC"

#define IONIC_FIRMWARE_IMAGE_FORM     IONIC_NIC_BASE_FORM_ID + 2
#define IONIC_FIRMWARE_IMAGE_INFO     "Firmware Image Properties"
#define IONIC_FIRMWARE_IMAGE_HELP     "Firmware Image Properties Help"

#define IONIC_FIRMWARE_VERSION_INFO    "Firmware Version"
#define IONIC_FIRMWARE_VERSION_HELP    "Firmware Version Information"
#define IONIC_FIRMWARE_VERSION_VALUE   IONIC_HII_NONE_VALUE

#define IONIC_EFI_VERSION_INFO          "EFI Image Version"
#define IONIC_EFI_VERSION_HELP          "EFI Image Information"
#define IONIC_EFI_VERSION_VALUE         IONIC_HII_NONE_VALUE

#define IONIC_NIC_CONFIG_FORM           IONIC_NIC_BASE_FORM_ID + 3
#define IONIC_NIC_CONFIG_INFO           "NIC Configuration"
#define IONIC_NIC_CONFIG_HELP           "NIC Configuration Help"

#define IONIC_VLAN_MODE_INFO            "Virtual LAN Mode"
#define IONIC_VLAN_MODE_HELP            "Virtual LAN Mode Information"
#define IONIC_VLAN_MODE_VALUE           IONIC_HII_NONE_VALUE
#define IONIC_VLAN_MODE_QUESTION        0x29

#define IONIC_VLAN_ID_INFO              "Virtual LAN ID"
#define IONIC_VLAN_ID_HELP              "Virtual LAN ID Information, Value can only be 0-4094."
#define IONIC_VLAN_ID_VALUE             IONIC_HII_NONE_VALUE
#define IONIC_VLAN_ID_QUESTION          0x30

#define IONIC_DEV_LEV_FORM              IONIC_NIC_BASE_FORM_ID + 4
#define IONIC_DEV_LEV_INFO              "Device Level Configuration"
#define IONIC_DEV_LEV_HELP              "Device Level Configuration Help"

#define IONIC_VIRTUAL_MODE_INFO         "Virtual Mode"
#define IONIC_VIRTUAL_MODE_HELP         "Virtual Mode Information"
#define IONIC_VIRTUAL_MODE_VALUE        IONIC_HII_NONE_VALUE
#define IONIC_VIRTUAL_MODE_QUESTION     0x31

#define IONIC_VIRTUAL_FUNC_INFO         "PCI Virtual Functions Advertised"
#define IONIC_VIRTUAL_FUNC_HELP         "PCI Virtual Functions Advertised Information"
#define IONIC_VIRTUAL_FUNC_VALUE        IONIC_HII_NONE_VALUE
#define IONIC_VIRTUAL_FUNC_QUESTION     0x32

#define IONIC_BMC_INTERFACE_INFO              "BMC Interface"
#define IONIC_BMC_INTERFACE_HELP              "BMC Interface Information"
#define IONIC_BMC_INTERFACE_QUESTION           0x33

#define IONIC_OUT_OF_BAND_MANAGEMENT_INFO      "Out of Band Management"
#define IONIC_OUT_OF_BAND_MANAGEMENT_HELP      "Out of Band Management Information"
#define IONIC_OUT_OF_BAND_MANAGEMENT_VALUE     0

#define IONIC_SIDE_BAND_INTERFACE_INFO        "Side-band interface to BMC"
#define IONIC_SIDE_BAND_INTERFACE_HELP        "Side-band interface to BMC Information"
#define IONIC_SIDE_BAND_INTERFACE_VALUE       1

#define IONIC_ENABLE_INFO                     "Enabled"
#define IONIC_ENABLE_HELP                     "Enabled Information"
#define IONIC_ENABLE_VALUE                    1

#define IONIC_DISABLE_INFO                    "Disabled"
#define IONIC_DISABLE_HELP                    "Disable Information"
#define IONIC_DISABLE_VALUE                   0

#define IONIC_BLINK_LED_INFO          "Blink LEDs"
#define IONIC_BLINK_LED_HELP          "Blink LEDs Help"
#define IONIC_BLINK_LED_VALUE         IONIC_HII_NONE_VALUE
#define IONIC_BLINK_LED_QUESTION      0x34

#define IONIC_DEV_NAME_INFO           "Device Name"
#define IONIC_DEV_NAME_HELP           "Device Name Information"
#define IONIC_DEV_NAME_VALUE           IONIC_HII_NONE_VALUE

#define IONIC_CHIP_TYPE_INFO          "Chip Type"
#define IONIC_CHIP_TYPE_HELP          "Chip Type Information"
#define IONIC_CHIP_TYPE_VALUE         IONIC_HII_NONE_VALUE

#define IONIC_PCI_DEV_ID_INFO         "PCI Device ID"
#define IONIC_PCI_DEV_ID_HELP         "PCI Device ID Information"
#define IONIC_PCI_DEV_ID_VALUE        IONIC_HII_NONE_VALUE

#define IONIC_PCI_VENDOR_ID_INFO      "PCI Vendor ID"
#define IONIC_PCI_VENDOR_ID_HELP      "PCI Vendor ID Information"
#define IONIC_PCI_VENDOR_ID_VALUE     IONIC_HII_NONE_VALUE

#define IONIC_PCI_SUB_ID_INFO         "PCI Sub System ID"
#define IONIC_PCI_SUB_ID_HELP         "PCI Sub System Information"
#define IONIC_PCI_SUB_ID_VALUE        IONIC_HII_NONE_VALUE

#define IONIC_PCI_ADDR_INFO           "PCI Address"
#define IONIC_PCI_ADDR_HELP           "PCI Address Information"
#define IONIC_PCI_ADDR_VALUE          IONIC_HII_NONE_VALUE

#define IONIC_LINK_STA_INFO           "Link Status"
#define IONIC_LINK_STA_HELP           "Link Status Information"
#define IONIC_LINK_STA_VALUE          IONIC_HII_NONE_VALUE

#define IONIC_MAC_ADDR_INFO           "MAC Address"
#define IONIC_MAC_ADDR_HELP           "MAC Address Information"
#define IONIC_MAC_ADDR_VALUE          IONIC_HII_NONE_VALUE
#define IONIC_MAC_ADDR_QUESTION       0x35

#define IONIC_VIRTUAL_MAC_INFO        "Virtual MAC Address"
#define IONIC_VIRTUAL_MAC_HELP        "Virtual MAC Address Information"
#define IONIC_VIRTUAL_MAC_VALUE       IONIC_HII_NONE_VALUE
#define IONIC_VIRTUAL_MAC_QUESTION    0x36

#define IONIC_BMC_SUPPORT_INFO        "BMC SUPPORT"
#define IONIC_BMC_SUPPORT_HELP         "BMC SUPPORT Information"
#define IONIC_BMC_SUPPORT_VALUE       IONIC_HII_NONE_VALUE
#define IONIC_BMC_SUPPORT_QUESTION    0x37

#define HII_BUILD_STRING    1
#define HII_BUILD_VARSTORE  2
#define HII_BUILD_FORM      3
#define HII_BUILD_ITEM      4

#pragma pack(1)

typedef struct _IONIC_NIC_HII_INFO IONIC_NIC_HII_INFO;


typedef struct {
    UINT32    signature;
    UINT8     version;
    UINT8     asic_type;
    UINT8     asic_rev;
    UINT8     fw_status;
    UINT32    fw_heartbeat;
    CHAR8     fw_version[32];
} DEV_INFO_REGS;

typedef
EFI_STATUS
(EFIAPI *IONIC_CALLBACK_FUNC)(
  IN        UINT16     SnpIndex,
  IN OUT    CHAR8     **Buffer,
  IN OUT    UINT8     *BufferLen
  );

typedef struct {
    EFI_STRING_ID     Prompt;
    EFI_STRING_ID     Help;
    EFI_STRING_ID     TextTwo;
} HII_TEXT_ITEM;

typedef struct {
    struct setting              *Setting;
    UINT16                      FormId;
    UINT8                       Type;
    EFI_GUID                    VarstoreGuid;
    UINT16                      VarStoreId;
    UINT16                      VarStoreSize;
    UINT16                      ControlId;
    const char                  *PromptStr;
    const char                  *HelpStr;
    const char                  *ValueStr;
    IONIC_CALLBACK_FUNC         Callback;
    HII_TEXT_ITEM               Hii;
    UINT8                       Flag;
    UINT8                       Value;
    BOOLEAN                     Show;
    BOOLEAN                     Suppress;
} NIC_HII_STRUC;

struct _IONIC_NIC_HII_INFO {
  NIC_HII_STRUC             NicName;
  NIC_HII_STRUC             FirmwareInfo;
  NIC_HII_STRUC             FirmwareVer;
  NIC_HII_STRUC             EfiVer;
  NIC_HII_STRUC             NicConfig;
  NIC_HII_STRUC             VlanMode;
  NIC_HII_STRUC             VlanModeEnable;
  NIC_HII_STRUC             VlanModeDisable;
  NIC_HII_STRUC             VlanId;
  NIC_HII_STRUC             DevLevel;
  NIC_HII_STRUC             VirtualMode;
  NIC_HII_STRUC             VirtualFunc;
  NIC_HII_STRUC             BmcSupport;
  NIC_HII_STRUC             BmcInterface;
  NIC_HII_STRUC             OutofBandManage;
  NIC_HII_STRUC             SideBandInterface;
  NIC_HII_STRUC             BLed;
  NIC_HII_STRUC             BLedEnable;
  NIC_HII_STRUC             BLedDisable;
  NIC_HII_STRUC             DevName;
  NIC_HII_STRUC             ChipType;
  NIC_HII_STRUC             PciDevId;
  NIC_HII_STRUC             VendorId;
  NIC_HII_STRUC             SubSysId;
  NIC_HII_STRUC             PciAddr;
  NIC_HII_STRUC             LinkStatus;
  NIC_HII_STRUC             MacAddr;
  NIC_HII_STRUC             VirtualMacAddr;
} ;

typedef struct {
  EFI_GUID                    PackageGuid;
  EFI_GUID                    FormsetGuid;
  EFI_HANDLE                  Handle;
  UINT16                      SnpIndexVar;
  BOOLEAN                     VlanModeVar;
  UINT16                      VlanIdVar;
  BOOLEAN                     VirtualModeVar;
  UINT8                       VirtualFuncVar;
  UINT8                       BmcInterfaceVar;
  UINT8                       BlinkLedVar;
  UINT8                       ChipTypeVar;
  UINT16                      PciIdVar;
  UINT16                      PciAddrVar;
  UINT16                      LinkStatusVar;
  CHAR16                      NicMacVar[50];
  CHAR16                      NicVirMacVar[50];
  UINT8                       SnpIndex;
  UINT16                      SnpQuestionBaseKey;
  void                        *NicHiiInfo;
  void                        *SnpDev;
  UINT8                       ReadyToBoot;
} NIC_HII_PACKAGE_INFO;

#pragma pack()

extern char vlan_mode_tse_string[];
extern char vlan_id_tes_string[];
extern char blink_led_tes_string[];
extern char bmc_support_tes_string[];
extern char bmc_interface_tes_string[];
extern char vis_mode_tes_string[];
extern char vis_func_tes_string[];

void *
ionic_hii_init (void *snp);

int
ionic_hii_valid(void *snp);

extern EFI_GUID gEfiionicHiiPackageInfoProtocol;

#endif
