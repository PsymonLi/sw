/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#ifdef DSC_SPP_CHECK_MANIFEST
#include <libtar.h>
#endif
#ifndef _WIN32
#include <pthread.h>
#else
#include <Windows.h>
#include <process.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "ionic_os.h"
//#include "ionic_fw_meta.h"
#include "ionic_spp_util.h"

/*
 * For encoding firmware version string to 64 bit.
 * Example: 1.2.3-E-4
 * Major(16):	    Bit[63:48] (1)
 * Minor(16):	    Bit[47:32] (2)
 * Release(32):     Bit[31:0]
 * ** RelMSB(10):   Bit[31:22] (3)
 * ** RelType(10):  Bit[21:12] (E)
 * ** RelBuild(12): Bit[11:0]  (4)
 */
#define IONIC_VER_ENCODE_MAJOR_SHIFT	48
#define IONIC_VER_ENCODE_MINOR_SHIFT	32
#define IONIC_VER_ENCODE_REL_MSB_SHIFT	22
#define IONIC_VER_ENCODE_REL_TYPE_SHIFT	12

#define IONIC_VER_ENCODE_REL_MSB_MASK	0x3FF /* Bit[31:22] */
#define IONIC_VER_ENCODE_REL_TYPE_MASK	0x3FF /* Bit[22:12] */
#define IONIC_VER_ENCODE_REL_BUILD_MASK	0xFFF /* Bit[11:0] */


/*
 * List of Naples device for firmware update and the number of
 * devices. This will include only Naples mgmt interfaces.
 */


#ifdef __cplusplus
extern "C" {
#endif
struct ionic ionic_devs[IONIC_DEVS_MAX];
int ionic_count;
#ifdef __cplusplus
}
#endif

/* Verbose level */
int ionic_verbose_level = 0;
/* For test, allow to override the device Id. */
uint16_t ionic_devid = PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT;
/* Mode flag to select devcmd(0)/adminQ(non-zero) for firmware update. */
int ionic_fw_update_type = 0;

/*
 * Naples PCI sub dev id table used to create NICFWData.xml
 * and also to populate discovery file nictype.
 */
static struct {
	uint16_t subDevId;
	/*  Only <type> of <VENDOR_NAME>_<TYPE>_1GB_NIC */
	char nictype[80];
	/* Full description of device. */
	char desc[256];
} naples_subid[] = {
	{0x4000, "MEM8G_100G", "Naples Dual 100Gb FHHL PCIe, 8GB HBM"},
	{0x4001, "MEM4G_100G", "Dual 100Gb FHHL PCIe, 4GB HBM"},
	{0x4002, "MEM4G_25G",  "Naples Dual 25Gb HHHL PCIe, 4GB HBM"},
	{0x4005, "MEM8G_25G",  "Naples Dual 25Gb HHHL PCIe, 8GB HBM"},
	{0x4007, "OCP_MEM4G_25G", "Naples Dual 25Gb OCP, 4GB HBM"},
	{0x4008, "SWM_MEM4G_25G", "Naples Dual 25Gb SWM, 4GB HBM"},
	{0x4009, "VOMERO_MEM8G_FLASH64G_25G", "Vomero Dual 100Gb HH3/4L PCIe, 8GB HBM, 64GB EMMC"},
	{0x400a, "MEM8G_FLASH16G_100G", "Naples Dual 100Gb HH3/4L PCIe, 8GB HBM, 16GB EMMC"},
	{0x400b, "OCP_MEM4G_FLASH8G_100G", "DSC-25-OCP 10/25G 2-port 4G RAM 8G eMMC G1"},
	{0x400c, "OCP_MEM4G_FLASH8G_10-25G", "DSC-25 10/25G 2-port 4G RAM 8G eMMC G1"},
	{0x400d, "QSFP28_40-100G", "DSP DSC-100 40/100G 2p QSFP28"},
};


int
ionic_init(FILE *fstream)
{
	struct ionic *ionic;
	int i, error;
	char *intfName;

	error = ionic_find_devices(fstream, ionic_devs, &ionic_count);
	if (error) {
		ionic_print_error(fstream, "all", "No Naples device was found\n");
		return (error);
	}

	ionic_print_info(fstream, "", "Number of DSC devices in system : %d\n", ionic_count);

	for (i = 0; i < ionic_count; i++) {
		ionic = &ionic_devs[i];
		intfName = ionic->intfName;
		/*
		 * If interface name is not set, use PCI value to form the name
		 * as specified in SPP spec.
		 */
		if (intfName[0] == 0) {
			snprintf(intfName, sizeof(ionic->intfName), "%04X.%04X.%04X.%04x",
				ionic->venId, ionic->devId, ionic->subVenId, ionic->subDevId);
			ionic_print_info(fstream, intfName, "Setting interface name using SPP format\n");
		}

		error = ionic_get_details(fstream, ionic);
		ionic_print_info(fstream, intfName, "DSC[%d] PCI(%d:%d:%d.%d)"
			" ASIC: %s firmware version: %s driver version: %s slot: %s mac: %s\n",
			i, ionic->domain, ionic->bus, ionic->dev, ionic->func, ionic->asicType,
			ionic->curFwVer, ionic->drvVer, ionic->slotInfo, ionic->macAddr);
		/*
		 * If slot information is not available, set as per SPP spec.
		 * which is "FFFFFFFF".
		 */
		if (ionic->slotInfo[0] == 0) {
			snprintf(ionic->slotInfo, sizeof(ionic->slotInfo), "FFFFFFFF");
		}
		if (ionic->macAddr[0] == 0) {
			snprintf(ionic->macAddr, sizeof(ionic->macAddr), "FFFFFFFF");
		}
	}

	return (error);
}

int
ionic_dump_list(void)
{
	struct ionic *ionic;
	int i;
	char *intfName;

	for (i = 0; i < ionic_count; i++) {
		ionic = &ionic_devs[i];
		intfName = ionic->intfName;
		ionic_print_info(stderr, intfName, "DSC[%d] PCI config addr: %d:%d.%d.%d"
			" current firmware version: %s driver version: %s slot: %s mac: %s\n",
			i, ionic->domain, ionic->bus, ionic->dev, ionic->func,
			ionic->curFwVer, ionic->drvVer, ionic->slotInfo, ionic->macAddr);
	}

	return (0);
}

/*
 * Conevrt release value of Maj.Min.Rel to string.
 */
int
ionic_decode_release(const uint32_t rel, char *buffer, int buf_len)
{
	uint32_t relMsb, relType, relBuild;
	int len = 0;

	relMsb = (rel >> IONIC_VER_ENCODE_REL_MSB_SHIFT) & IONIC_VER_ENCODE_REL_MSB_MASK;
	relType = (rel >> IONIC_VER_ENCODE_REL_TYPE_SHIFT) & IONIC_VER_ENCODE_REL_TYPE_MASK;
	relBuild = rel & IONIC_VER_ENCODE_REL_BUILD_MASK;

	len += snprintf(buffer, buf_len, "%d", relMsb);
	buffer[strlen(buffer)] = 0;

	if (!relType)
		return(0);

	len += snprintf(buffer + len, buf_len - len, "-");
	if(isalpha(relType)) {
		len += snprintf(buffer + len, buf_len - len, "%c", relType);
	} else {
		len += snprintf(buffer + len, buf_len - len, "%d", relType);
	}
	buffer[strlen(buffer)] = 0;

	if (!relBuild)
		return(0);

	len += snprintf(buffer + len, buf_len - len, "-");
	len += snprintf(buffer + len, buf_len - len, "%d", relBuild);

	return (0);
}
/*
 * Convert Version number in Maj.Min.Rel to Pensando format (A.B.C-E-<build>)
 */
int
ionic_spp2pen_verstr(const char *spp_ver, char *pen_ver, int buf_len)
{
	uint32_t maj, min, rel;
	char rel_str[32];

	sscanf(spp_ver, "%d.%d.%d", &maj, &min, &rel);
	ionic_decode_release(rel, rel_str, sizeof(rel_str));
	snprintf(pen_ver, buf_len, "%d.%d.%s", maj, min, rel_str);

	return (0);
}

/*
 * Convert Pensando version string to SPP format.
 */
int
ionic_pen2spp_verstr(const char *pen_ver, char *spp_ver, int buf_len)
{
	uint32_t maj, min, rel;

	ionic_encode_fw_version(pen_ver, &maj, &min, &rel);
	snprintf(spp_ver, buf_len, "%d.%d.%d", maj, min, rel);

	return (0);
}
/*
 * Encode version string release part (<num>-E/C-<build>) to release number.
 */
static int
ionic_encode_release(char *saveptr, uint32_t *rel)
{
	uint32_t relMsb, relType, relBuild;
	char *sub;

	*rel = 0;
	sub = strtok_r(NULL, "-", &saveptr);
	if (sub == NULL) {
		return (0);
	}
	relMsb = strtoul(sub, NULL, 0);
	*rel = (relMsb << IONIC_VER_ENCODE_REL_MSB_SHIFT);

	sub = strtok_r(NULL, "-", &saveptr);
	if (sub == NULL) {
		return (0);
	}
	/*
	 * Handle different type string it could be
	 * 1.13.0-E-7 (use one character -'E' here).
	 * 1.13.0-136-7-xxxxx (Use '136' as '1' in this case)
	 */
	if (isalpha(sub[0]))
		relType = sub[0];
	else
		relType = strtoul(sub, NULL, 0);

	*rel |= (relType << IONIC_VER_ENCODE_REL_TYPE_SHIFT);

	sub = strtok_r(NULL, "-", &saveptr);
	if (sub == NULL) {
		return (0);
	}
	relBuild = strtoul(sub, NULL, 0);
	*rel |= relBuild;

	return (0);
}
/*
 * Convert Naples firmware version to Major, minor and release number.
 */
int
ionic_encode_fw_version(const char *fw_vers, uint32_t *major, uint32_t *minor, uint32_t *rel)
{
	char *string, *string1, *sub, *saveptr = NULL;
	*major = *minor = *rel = 0;

	if (fw_vers == NULL)
		return (EINVAL);

#ifdef _WIN32
	string = _strdup(fw_vers);
#else
	string = strdup(fw_vers);
#endif
	if (string == NULL) {
		return (EINVAL);
	}

	string1 = string;
	/* Find the major number. */
	sub = strtok_r(string, ".", &saveptr);
	if (sub == NULL) {
		free(string1);
		return (EINVAL);
	}
	*major = strtoul(sub, NULL, 0);

	/* Find the minor number. */
	sub = strtok_r(NULL, ".", &saveptr);
	if (sub == NULL) {
		free(string1);
		return (0);
	}
	*minor = strtoul(sub, NULL, 0);

	ionic_encode_release(saveptr, rel);

	free(string1);
	return (0);
}

/*
 * Main function to determine the firmware upgrade/downgrade action.
 * Returns as per SPP spec:
 * 1)upgrade - new version > cur_version
 * 2)rewrite - new version = cur version
 * 3)downgrade - new version < cur version
 * 4)skip - not sure where we need this? could be for 3)
 */
char *
ionic_fw_decision2(uint32_t new_maj, uint32_t new_min, uint32_t new_rel, uint32_t cur_maj,
	uint32_t cur_min, uint32_t cur_rel)
{
	uint64_t new, cur;

	new = ((uint64_t)new_maj << IONIC_VER_ENCODE_MAJOR_SHIFT) +
		((uint64_t)new_min << IONIC_VER_ENCODE_MINOR_SHIFT) +
		new_rel;
	cur = ((uint64_t)cur_maj << IONIC_VER_ENCODE_MAJOR_SHIFT) +
		((uint64_t)cur_min << IONIC_VER_ENCODE_MINOR_SHIFT) +
		cur_rel;

	if (new > cur)
		return "upgrade";
	else if (new < cur)
		return "downgrade";
	else
		return "rewrite";
}

char *
ionic_fw_decision(char *newVer, char *curVer)
{
	uint32_t new_maj, new_min, new_rel, cur_maj, cur_min, cur_rel;
	char *str;

	ionic_encode_fw_version(newVer, &new_maj, &new_min, &new_rel);
	ionic_encode_fw_version(curVer, &cur_maj, &cur_min, &cur_rel);

	str = ionic_fw_decision2(new_maj, new_min, new_rel, cur_maj, cur_min, cur_rel);

	return (str);
}
/*
 * Provide descriptive name for Naples based on PCI subsystem Id.
 */
int
ionic_desc_name(struct ionic *ionic, char *desc, int len)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(naples_subid); i++) {
		if (naples_subid[i].subDevId == ionic->subDevId) {
			/*  Format <VENDOR_NAME>_<TYPE>_1GB_NIC */
			if (ionic->asicType[0])
				snprintf(desc, len, "PEN_DSC_%s_%s_NIC", ionic->asicType, naples_subid[i].nictype);
			else
				snprintf(desc, len, "PEN_DSC_%s_NIC", naples_subid[i].nictype);
			return (0);
		}
	}

	snprintf(desc, len, "PEN_DSC_%s_0x%x_NIC", ionic->asicType, ionic->subDevId);
	return (EINVAL);
}


#ifdef SPP_DSC_OLD_NICFWDATA
/*
 * Create NICFWData.xmll for version string
 */
int
ionic_create_NICFWData(const char *pen_ver, const char *fw_path)
{
	xmlDoc *doc = NULL;
	xmlNodePtr root, nics, nic;
	char file[256], buffer[320];
	int error = 0, len;
	uint32_t i;

	snprintf(file, sizeof(file), "%s/%s", fw_path, HPE_NIC_FW_DATA_FILE);

	doc = xmlNewDoc(BAD_CAST "1.0");
	root = xmlNewNode(NULL, BAD_CAST "nic_fw_package");
	xmlNewProp(root, BAD_CAST "version", BAD_CAST "1.0.0.0");
	xmlDocSetRootElement(doc, root);

	nics = xmlNewChild(root, NULL, BAD_CAST "nic", NULL);
	if (nics == NULL) {
		fprintf(stderr, "Failed to create nic node\n");
		error = ENXIO;
		goto err_out;
	}

	for (i = 0; i < ARRAY_SIZE(naples_subid); i++) {
		nic = xmlNewChild(nics, NULL, BAD_CAST "nic", NULL);
		if (nic == NULL) {
			fprintf(stderr, "Failed to add nic node\n");
			error = ENXIO;
			goto err_out;
		}
		snprintf(buffer, sizeof(buffer), "HPE %s Adapter", naples_subid[i].desc);
		xmlNewProp(nic, BAD_CAST "name", BAD_CAST buffer);

		snprintf(buffer, sizeof(buffer), "%x", PCI_VENDOR_ID_PENSANDO);
		xmlNewProp(nic, BAD_CAST "venid", BAD_CAST buffer);

		snprintf(buffer, sizeof(buffer), "%x", PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT);
		xmlNewProp(nic, BAD_CAST "devid", BAD_CAST buffer);

		snprintf(buffer, sizeof(buffer), "%x", PCI_VENDOR_ID_PENSANDO);
		xmlNewProp(nic, BAD_CAST "subvenid", BAD_CAST buffer);

		snprintf(buffer, sizeof(buffer), "%x", naples_subid[i].subDevId);
		xmlNewProp(nic, BAD_CAST "subdevid", BAD_CAST buffer);

		memset(buffer, 0, sizeof(buffer));
		ionic_pen2spp_verstr(pen_ver, buffer, sizeof(buffer));
		xmlNewProp(nic, BAD_CAST IONIC_SPP_FW_TYPE, BAD_CAST buffer);

		snprintf(buffer, sizeof(buffer), "%s/dsc_fw_%s.tar", fw_path, pen_ver);
		xmlNewProp(nic, BAD_CAST IONIC_SPP_FW_TYPE_FILE, BAD_CAST buffer);
	}

	// To print on console.
	//xmlSaveFormatFileEnc("-", doc, "UTF-8", 1);
	len = xmlSaveFormatFileEnc(file, doc, "UTF-8", 1);
	if (len < 0) {
		fprintf(stderr, "Failed to save %s file, error: %s\n", file, strerror(errno));
		error = ENXIO;
	} else {
		fprintf(stderr, "Wrote %d bytes for %s file\n", len, file);
	}

err_out:
	//xmlMemoryDump();
	if (doc)
		xmlFreeDoc(doc);
	xmlCleanupParser();

	if (error)
		fprintf(stderr, "Failed to create %s\n", file);
	return (error);
}
#endif /* SPP_DSC_OLD_NICFWDATA */

int
ionic_add_spp_entry(FILE *fstream, ven_adapter_info *ionic_info, struct ionic *ionic,
	const char *firmware_file_path)
{
	fwInfo *fw;
	int error;
	char buffer[256];

	ionic_desc_name(ionic, buffer, sizeof(buffer));
	W_SNPRINTF(ionic_info->adapterBrandingName, sizeof(ionic_info->adapterBrandingName), PRIxHS, buffer);
	W_SNPRINTF(ionic_info->ethernetInterfaceName, sizeof(ionic_info->ethernetInterfaceName),
		PRIxHS, ionic->intfName);
	ionic_info->venId = ionic->venId;
	ionic_info->devId = ionic->devId;
	ionic_info->subVenId = ionic->subVenId;
	ionic_info->subDevId = ionic->subDevId;
	ionic_info->segment = ionic->domain;
	ionic_info->busNumber = ionic->bus;
	ionic_info->deviceNumber = ionic->dev;
	ionic_info->funcNumber = ionic->func;
	W_SNPRINTF(ionic_info->macAddress, sizeof(ionic_info->macAddress), PRIxHS, ionic->macAddr);
	W_SNPRINTF(ionic_info->slotNumber, sizeof(ionic_info->slotNumber), PRIxHS, ionic->slotInfo);

	/* Naples single monolithic image. */
	ionic_info->nFW = 1;
	fw = &ionic_info->fwInfoData[0];
	W_SNPRINTF(fw->fwType, sizeof_field(fwInfo, fwType), PRIxHS, IONIC_SPP_FW_TYPE);
	W_SNPRINTF(fw->fwFileName, sizeof_field(fwInfo, fwFileName), PRIxHS, firmware_file_path);

	error = ionic_encode_fw_version(ionic->curFwVer, &fw->curFwMaj,
				&fw->curFwMin, &fw->curFwRel);
	if (error) {
		ionic_print_error(fstream, ionic->intfName, "Couldn't decode current fw version: %s\n",
			ionic->curFwVer);
		return (error);
	}

	error = ionic_encode_fw_version(ionic->newFwVer, &fw->avlbFwMaj,
				&fw->avlbFwMin, &fw->avlbFwRel);
#if 0
	if (error) {
		ionic_print_error(fstream, ionic->intfName, "Couldn't decode new fw version: %s\n",
			ionic->newFwVer);
		return (error);
	}
#endif
	return (0);
}

#ifdef DSC_SPP_CHECK_MANIFEST
static int
ionic_read_file_from_tar(FILE *fstream, struct ionic *ionic, TAR *tar)
{
	const int max_size = 8192;
	char *buffer, ver_str[256], asicType[32];
	int tar_size, buf_size, i, error, read;

	buffer = (char *)malloc(max_size);
	if (buffer == NULL) {
		ionic_print_error(fstream, "", "Malloc failed, error: %s\n",
			strerror(errno));
		return (ENOMEM);
	}

	tar_size = th_get_size(tar);
	for (i = tar_size, buf_size = 0; i > 0 && buf_size < max_size;
		i -= T_BLOCKSIZE, buf_size += read) {
		read = tar_block_read(tar, buffer + buf_size);
		if (read == EOF) {
			break;
		}
	}

	error = ionic_read_from_manifest(fstream, buffer, buf_size, ver_str, asicType);
	ionic_print_info(fstream, "", "buf_size: %d Version str: %s ASIC type: %s\n",
		buf_size, ver_str, asicType);

	if (STRCASECMP(ionic->asicType, asicType) == 0) {
		ionic_print_error(fstream, ionic->intfName, "Firmware image ASIC mismatch,"
			"expected: %s, found: %s\n", ionic->asicType, asicType);
//XXX:
//		error = EINVAL;
	}

	if (STRCASECMP(ionic->newFwVer, ver_str) == 0) {
		ionic_print_error(fstream, ionic->intfName, "Firmware image version mismatch,"
			"expected: %s, found: %s\n", ionic->newFwVer, ver_str);
//		error = EINVAL;
	}
	free(buffer);

	return (error);
}

int
ionic_find_version_from_tar(FILE *fstream, struct ionic *ionic, char *tar_file, const char *find_file)
{
	TAR *tar = NULL;
	char *file;
	int status = 0;

	status = tar_open(&tar, tar_file, NULL, O_RDONLY, 0, 0);
	if (status != 0) {
		fprintf(fstream, "Fail to open file: %s, error: %s\n",
			tar_file, strerror(errno));
			return (ENXIO);
	}

	fprintf(fstream, "Find version from firmware file: %s\n",
			tar_file);
	while (th_read(tar) == 0) {
		file = th_get_pathname(tar);
		if (strcmp(file, find_file) == 0) {
			free(file);
			ionic_read_file_from_tar(fstream, ionic, tar);
			break;
		}
		free(file);

		status = tar_skip_regfile(tar);
		if (TH_ISREG(tar) && status) {
			fprintf(fstream, "tar_skip_regfile() failed, error: %d\n",
				status);
			break;
		}
	}

	status = tar_close(tar);

	return (status);
}
#endif

/*
 * Functions for penutil and GTEST
 */
#ifndef _WIN32
void *
ionic_test_flash_thread(void *arg)
{
	struct ionic *ionic = (struct ionic *)arg;
	int error;

	error = oem_do_full_flash_PCI(ionic->fwFile, true, ionic->domain, ionic->bus,
		ionic->dev, ionic->func);
	fprintf(stderr, "Flash update status: " PRIxWS "\n", oem_text_for_error_code(error));
	pthread_exit(NULL);
}
#else
unsigned __stdcall ionic_test_flash_thread(void* arg)
{
	struct ionic* ionic = (struct ionic*)arg;
	int error;
	spp_char_t w_fw_file[256];

	W_SNPRINTF(w_fw_file, sizeof(w_fw_file), PRIxHS, ionic->fwFile);
	fprintf(stderr, "Starting Flash update...\n");
	error = oem_do_full_flash_PCI(w_fw_file, true, ionic->domain, ionic->bus, ionic->dev, ionic->func);
	fprintf(stderr, "Flash update, SPP status: " PRIxWS "\n", oem_text_for_error_code(error));

	_endthreadex(0);
	return 0;
}
#endif

/*
 * XXX: not clear how flash_with_PCI() will be used in multi-threaded.
 */
int
ionic_test_multi_update(const char *log_file, const char *fw_path, const char *discovery_file)
{
	struct ionic *ionic;
	int i, error = 0;
	char fw_full_path[512];
#ifndef _WIN32
	pthread_t thr[IONIC_DEVS_MAX];
#else
	HANDLE thr[IONIC_DEVS_MAX];
	unsigned int ThreadId = 0;
	DWORD WaitStatus;
#endif

	error = ionic_test_discovery(log_file, fw_path, discovery_file);
	if (error)
		return (error);

	fprintf(stderr, "Starting muti-threaded flash update, number of cards: %u\n", ionic_count);
	for (i = 0; i < ionic_count; i++) {
		ionic = &ionic_devs[i];
		/* Fix the firmware file path based on input. */
		sprintf(fw_full_path, "%s/%s", fw_path, ionic->fwFile);
		strncpy(ionic->fwFile, fw_full_path, sizeof(ionic->fwFile));
#ifndef _WIN32
		error = pthread_create(&thr[i], NULL, ionic_test_flash_thread, ionic);
		if (error) {
			perror("pthread_create");
			return (error);
		}
#else
		thr[i] = (HANDLE)_beginthreadex(NULL, 0, &ionic_test_flash_thread, ionic, 0, &ThreadId);
		if (0 == thr[i]) {
			error = GetLastError();
			fprintf(stderr, "Failed to create flash thread. Err: %u\n", error);
			break;
		}
#endif
	}
	fprintf(stderr, "%u flashing threads started.\n", i);

#ifndef _WIN32
	for (i = 0; i < ionic_count; i++) {
		error = pthread_join(thr[i], NULL);
		if (error)
			perror("pthread_join");
	}
#else
	WaitStatus = WaitForMultipleObjects(i, thr, TRUE, INFINITE);
	if (WAIT_OBJECT_0 == WaitStatus) {
		for (i = 0; i < ionic_count; i++) {
			CloseHandle(thr[i]);
		}
	}
	else {
		error = GetLastError();
		fprintf(stderr, "Failed to wait for flash thread. Err: %u\n", error);
	}
#endif
	fprintf(stderr, "SPP status:" PRIxWS "\n", oem_text_for_error_code(error));

	return (error);
}

int
ionic_test_discovery(const char *log_file, const char *fw_path, const char *discovery_file)
{
	int error, count;
	ven_adapter_info *ionic_info;
	spp_char_t w_log_file[256], w_fw_path[256], w_discovery_file[256];

	if (log_file)
		W_SNPRINTF(w_log_file, sizeof(w_log_file), PRIxHS, log_file);

	if (fw_path)
		W_SNPRINTF(w_fw_path, sizeof(w_fw_path), PRIxHS, fw_path);

	if (discovery_file)
		W_SNPRINTF(w_discovery_file, sizeof(w_discovery_file), PRIxHS, discovery_file);

	count = 0;
	error = oem_get_debug_info(w_log_file);
	if (error) {
		fprintf(stderr, "SPP status: " PRIxWS "\n", oem_text_for_error_code(error));
		return (error);
	}

	error = oem_get_adapter_info(NULL, &count, w_fw_path);
	if (error) {
		fprintf(stderr, "Couldn't get count, SPP status: " PRIxWS "\n",
			oem_text_for_error_code(error));
		return (error);
	}

	ionic_info = malloc(count * sizeof(ven_adapter_info));
	if (ionic_info == NULL) {
		fprintf(stderr, "malloc failed\n");
		perror("malloc");
		return (ENOMEM);
	}

	memset(ionic_info, 0, count * sizeof(ven_adapter_info));
	error = oem_get_adapter_info(ionic_info, &count, w_fw_path);
	if (error) {
		fprintf(stderr, "Couldn't get adapter info, SPP status: " PRIxWS "\n",
			oem_text_for_error_code(error));
		free(ionic_info);
		return (error);
	}

	error = oem_do_discovery_with_files(w_discovery_file, w_fw_path);
	fprintf(stderr, "SPP status:" PRIxWS "\n", oem_text_for_error_code(error));

	//ionic_dump_list();

	free(ionic_info);

	return (error);
}

int
ionic_test_update(const char *log_file, const char *fw_path, const char *discovery_file)
{
	int error;
	spp_char_t w_log_file[256], w_fw_path[256], w_discovery_file[256];

	if (log_file)
		W_SNPRINTF(w_log_file, sizeof(w_log_file)/2, PRIxHS, log_file);

	if (fw_path)
		W_SNPRINTF(w_fw_path, sizeof(w_fw_path)/2, PRIxHS, fw_path);

	if (discovery_file)
		W_SNPRINTF(w_discovery_file, sizeof(w_discovery_file)/2, PRIxHS, discovery_file);

	error = oem_get_debug_info(w_log_file);
	if (error) {
		fprintf(stderr, "Failed to open log file, SPP status: " PRIxWS "\n",
			oem_text_for_error_code(error));
		return (error);
	}

	error = oem_do_flash_with_file(w_discovery_file, w_fw_path);
	fprintf(stderr, "SPP status: " PRIxWS "\n",
			oem_text_for_error_code(error));

	return (error);
}
