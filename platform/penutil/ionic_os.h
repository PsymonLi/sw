/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <stdint.h>
#include <stdarg.h>

#include "ionic_spp.h"

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))

/* Sizeof of field f in type t */
#define sizeof_field(t, f)	sizeof(((t *)0)->f)

/* Naples specific definitions. */
#define IONIC_DEVS_MAX		10 /* Arbitrary number of devices. */

/*
 * Naples specific hardware definitions.
 */
#define PCI_VENDOR_ID_PENSANDO			0x1dd8

#define PCI_DEVICE_ID_PENSANDO_BRIDGE_UPSTREAM	0x1000
#define PCI_DEVICE_ID_PENSANDO_BRIDGE_DOWNSTREAM 0x1001
#define PCI_DEVICE_ID_PENSANDO_IONIC_ETH_PF	0x1002
#define PCI_DEVICE_ID_PENSANDO_IONIC_ETH_VF	0x1003
#define PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT	0x1004

/*
 * Max time to wait for Naples firmware update
 * include CPLD update time for discovery.xml
 */
#define NAPLES_FW_UPDATE_MAX_TIME_SECS		"1800"

#ifdef _WIN32
#define ionic_print_debug(fs, intf, fmt, ...)  do {				\
		if (ionic_verbose_level)					\
			fprintf(fs, "DEBUG[%s:%d](%s)" fmt,			\
				__func__, __LINE__, intf , ##__VA_ARGS__);	\
		} while(0)

#define ionic_print_info(fs, intf, fmt, ...)  fprintf(fs, "INFO[%s:%d](%s)" fmt ,\
	__func__, __LINE__, intf , ##__VA_ARGS__)
#define ionic_print_error(fs, intf, fmt, ...)  fprintf(fs, "ERR[%s:%d](%s)" fmt ,\
	__func__, __LINE__, intf , ##__VA_ARGS__)
#else
#define ionic_print_debug(fs, intf, fmt, args...)  do {				\
		if (ionic_verbose_level)					\
			fprintf(fs, "DEBUG[%s:%d](%s)" fmt,			\
				__func__, __LINE__, intf , ##args);		\
		} while(0)

#define ionic_print_info(fs, intf, fmt, args...)  fprintf(fs, "INFO[%s:%d](%s)" fmt ,\
	__func__, __LINE__, intf , ##args)
#define ionic_print_error(fs, intf, fmt, args...)  fprintf(fs, "ERR[%s:%d](%s)" fmt ,\
	__func__, __LINE__, intf , ##args)
#endif
/* From ionic_if.h */
#define IONIC_FWVERS_BUFLEN		32

/*
 * Private definition for DSC SPP.
 * Has following fields.
 *  * Mandatory - Updated by OS SPP by find_devices() or get_details().
 *                Like PCI detils, interface name etc.
 *  * Optional - For debugging like driver version, mac adddress
 *  * SPP core fields - Managed by DSC SPP core.
 *  * Driver optional - in case driver want to add some fields.
 */
struct ionic {
	/*
	 * Common OS definitions for SPP ven_adappter_info
	 */

	/* Driver must populate below field. */
	/* PCI related. */
	uint16_t domain;
	uint8_t bus;
	uint8_t dev;
	uint8_t func;

	uint16_t venId;
	uint16_t devId;
	uint16_t subVenId;
	uint16_t subDevId;

	/* ASIC type is used to validated firmware image. */
	char asicType[32];
	/* OS interface name. */
	char intfName[HPE_SPP_ADAPTER_MAX_LEN];
	/* Current firmware version string, e.g. 1.14.0-E-xxx */
	char curFwVer[IONIC_FWVERS_BUFLEN];
	/* New firmware version string. */
	char newFwVer[IONIC_FWVERS_BUFLEN];

	/* Optional physical slot information. */
	char slotInfo[HPE_SPP_SLOT_MAX_LEN];
	/* Optional MAC address in string format. */
	char macAddr[HPE_SPP_MAC_ADDR_MAX_LEN];

	/* For debugging, driver can populate below fields. */
	/* Current driver version string. */
	char drvVer[IONIC_FWVERS_BUFLEN];

	/*
	 * Following are managed by DSC SPP, OS SPP shouldn't
	 * touch this fields.
	 */
	/* Firmware file name read from NICDataFw xml file */
	char fwFile[256];
	/*
	 * Following are read from discovery file.
	 */
	/* Action - upgrade, downgrade, rewrite, skip */
	char action[32];

	/*
	 * OS specific definitions, add if you need it.
	 */
#ifdef PCIUTILS
	/* PCI BAR0 physical address. */
	uint64_t bar0_pa;
#endif
};

#if defined (_USRDLL) || ! defined (_WIN32) 
extern struct ionic ionic_devs[];
extern int ionic_count;
extern uint16_t ionic_devid;
extern int ionic_verbose_level;
extern int ionic_fw_update_type;
#else
__declspec(dllimport) struct ionic ionic_devs[];
__declspec(dllimport) int ionic_count;
__declspec(dllimport) uint16_t ionic_devid;
__declspec(dllimport) int ionic_verbose_level;
__declspec(dllimport) int ionic_fw_update_type;
#endif

/*
 * DSC OS 3 APIs.
 */
/* 1. For discovery of Naples management devices in system */
int ionic_find_devices(FILE *fstream, struct ionic ionic_devs[], int *count);
/* 2. Get details of the device that was partially update in discovery. */
int ionic_get_details(FILE *fstream, struct ionic *ionic);
/* 3. Called to do the actual flash update. */
int ionic_flash_firmware(FILE *fstream, struct ionic *ionic, char *fw_file);

