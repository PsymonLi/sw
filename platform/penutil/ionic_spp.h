/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <stdint.h>

#ifndef IONIC_SPP_H
#define IONIC_SPP_H

/* SPP build OS for discovery file. */
#ifdef DSC_SPP_WIN
#define SPP_BUILD_OS "Windows"
#elif __linux__
#define SPP_BUILD_OS "Linux"
#elif __FreeBSD__
#define SPP_BUILD_OS "FreeBSD"
#else /* ! __linux__ */
#define SPP_BUILD_OS "ESX"
#endif /* __linux__ */

#ifndef DSC_SPP_VERSION
#define DSC_SPP_VERSION 	"1.0.0"
#endif
/*
* Firmware type as agreed with HPE.
*/
#define IONIC_SPP_FW_TYPE	"nvm"
#define IONIC_SPP_FW_TYPE_FILE	"nvmFile"

#ifdef DSC_SPP_WIN
typedef wchar_t	spp_char_t;

/* Add "L" for wide string */
#define W_STRING(X)	L ##X

#define PRIxHS		"%hs"
#define PRIxWS		"%ls"

#define STRNCPY(d, s, ds)	wcsncpy(d, s, ds)

#define STRCASECMP(s1, s2)	_stricmp(s1, s2)
/* Size is divide by 2 for wchar_t */
#define W_SNPRINTF(s, n, f, ...)_snwprintf_s(s, (n) / 2, _TRUNCATE, W_STRING(f), ##__VA_ARGS__)

#define strtok_r		strtok_s

#else /* ! WIN32 */
typedef char spp_char_t;

#define W_STRING(X)	X

#define PRIxHS		"%s"
#define PRIxWS		"%s"

#define STRNCPY(d, s, ds)	strncpy(d, s, ds)

#define STRCASECMP(s1, s2)	strcasecmp(s1, s2)
#define W_SNPRINTF(s, n, f, args...) snprintf(s, n, f, ##args)

#endif

/*
 * From HPE SPP spec REQ 019
 */
#define HPE_SPP_STATUS_SUCCESS		0
#define HPE_SPP_REBOOT_AFTER_INSTALL	1 /* Request reboot after firmware install */
#define HPE_SPP_FW_VER_SAME		2 /* Firmware version is same */
#define HPE_SPP_FW_VER_LATEST		3 /* Card firmware is latest */
#define HPE_SPP_HW_ACCESS		4 /* Couldn't identify or access the hardware */
#define HPE_SPP_USER_CANCELLED		5 /* User cancelled installation */
#define HPE_SPP_LIBRARY_DEP_FAILED	6 /* Library dependency or tool failure */
#define HPE_SPP_INSTALL_HW_ERROR	7 /* Firmware install operation failed */
#define HPE_SPP_FW_FILE_MISSING		8 /* Firmware file couldn't be found */
#define HPE_SPP_FW_FILE_PERM_ERR	9 /* Firmware file permission issue */
#define HPE_SPP_DIS_MISSING		10 /* Discovery file missing */
#define HPE_SPP_DIS_ACCESS		11 /* Discovery file permission issue */
#define HPE_SPP_DIS_CREATE_FAILED	12 /* Couldn't create discovery file */
#define HPE_SPP_VPD_CORRUPTED		13 /* VPD corruption was detected */
#define HPE_SPP_REST_FW_REBOOT		14 /* Install failed, original fw was restored, request reboot */
#define HPE_SPP_FW_CORRUPT		15 /* Inability to validate firmware integrity */

/*
 * File contains firmware version and file name details.
 */
#define HPE_NIC_FW_DATA_FILE 	"NICFWData.xml"

/* From HPE SPP spec, appendix C */
/*
 * The maximum number of independent firmware
 * components that a Vendor can provide (just an arbitrary large number
 * for now)
 */
#define MAX_FW			20

#define HPE_SPP_ADAPTER_BRAND_MAX_LEN	1024
#define HPE_SPP_ADAPTER_MAX_LEN		256
#define HPE_SPP_MAC_ADDR_MAX_LEN	20
#define HPE_SPP_SLOT_MAX_LEN		16
#define HPE_SPP_PATH_MAX_LEN		1024

typedef struct {
	spp_char_t fwType[256];
	spp_char_t fwFileName[HPE_SPP_PATH_MAX_LEN];
	// current firmware version on card
	uint32_t curFwMaj; // Major
	uint32_t curFwMin; // Minor
	uint32_t curFwRel; // Release
	// available firmware version for update
	uint32_t avlbFwMaj; // Major
	uint32_t avlbFwMin; // Minor
	uint32_t avlbFwRel; // Release
	// FW configuration flags reserved for HPE
	int32_t fwFlags;
} fwInfo;

typedef struct {
	spp_char_t adapterBrandingName[HPE_SPP_ADAPTER_BRAND_MAX_LEN];
	spp_char_t ethernetInterfaceName[HPE_SPP_ADAPTER_MAX_LEN];
	//PCID INFO
	uint32_t venId;
	uint32_t devId;
	uint32_t subVenId;
	uint32_t subDevId;
	//BUS INFO
	uint32_t segment;
	uint32_t busNumber;
	uint32_t deviceNumber;
	uint32_t funcNumber;
	spp_char_t slotNumber[HPE_SPP_SLOT_MAX_LEN];
	spp_char_t macAddress[HPE_SPP_MAC_ADDR_MAX_LEN];
	uint32_t nFW; // Independent FW Count
	fwInfo fwInfoData[MAX_FW];
	// Configuration flags reserved for HPE
	int32_t configFlags;
} ven_adapter_info;

/*
* Single image for Naples.
*/
typedef enum {
nvm
} dscFirmwareType;

#ifdef __cplusplus
extern "C" {
#endif
int dsc_do_discovery_with_files(spp_char_t *discover_file_xml, spp_char_t *firmware_file_path);
int dsc_do_full_flash_PCI(spp_char_t *firmware_file, int force, uint16_t domain,
uint8_t  bus, uint8_t dev, uint8_t func);
int dsc_do_flash_with_file(spp_char_t *dsicover_file_xml, spp_char_t *firmware_file_path);
int dsc_get_adapter_info(ven_adapter_info *adp, int *nEntries, spp_char_t *fimware_file_path);
spp_char_t *dsc_text_for_error_code(uint32_t erro_code);
int dsc_get_debug_info(spp_char_t *log_file);
int dsc_get_full_flash_dump_PCI(spp_char_t *flash_dump_file, dscFirmwareType firmware_type,
uint16_t domain, uint8_t bus, uint8_t dev, uint8_t func);
#ifdef __cplusplus
}
#endif
#endif /* IONIC_SPP_H */
