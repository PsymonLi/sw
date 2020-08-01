/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "ionic_spp.h"
#include "ionic_os.h"
#include "ionic_spp_util.h"
#include "ionic_discovery.h"

/* Log file name */
static char dsc_log_file[HPE_SPP_PATH_MAX_LEN];

/**************************************************************************
 * Naples SPP public APIs.
 **************************************************************************/
/*
 * Convert SPP error code to error string.
 */
spp_char_t *
#ifdef _WIN32
__stdcall
#endif
dsc_text_for_error_code(uint32_t err_code)
{

	switch (err_code) {
		case HPE_SPP_STATUS_SUCCESS:
			return W_STRING("spp success");
		case HPE_SPP_REBOOT_AFTER_INSTALL:
			return W_STRING("reboot requested after firmware install");
		case HPE_SPP_FW_VER_SAME:
			return W_STRING("firmware version same");
		case HPE_SPP_FW_VER_LATEST:
			return W_STRING("firmware version latest");
		case HPE_SPP_HW_ACCESS:
			return W_STRING("couldn't find DSC card");
		case HPE_SPP_USER_CANCELLED:
			return W_STRING("user cancelled");
		case HPE_SPP_LIBRARY_DEP_FAILED:
			return W_STRING("miscellaneous error");
		case HPE_SPP_INSTALL_HW_ERROR:
			return W_STRING("firmware install failed");
		case HPE_SPP_FW_FILE_MISSING:
			return W_STRING("firmware file missing");
		case HPE_SPP_FW_FILE_PERM_ERR:
			return W_STRING("firmware file permission error");
		case HPE_SPP_DIS_MISSING:
			return W_STRING("Discovery file missing");
		case HPE_SPP_DIS_ACCESS:
			return W_STRING("Discovery file access error");
		case HPE_SPP_DIS_CREATE_FAILED:
			return W_STRING("Discovery file couldn't be created");
		case HPE_SPP_VPD_CORRUPTED:
			return W_STRING("VPD is corrupted");
		case HPE_SPP_REST_FW_REBOOT:
			return W_STRING("Install failed, old firmware was restored");
		case HPE_SPP_FW_CORRUPT:
			return W_STRING("Firmware image corrupted");
		default:
			return W_STRING("Unknown");
	}
}

int
#ifdef _WIN32
__stdcall
#endif
dsc_get_debug_info(spp_char_t *log_file)
{
	FILE *fstream;
	char buffer[256];
	int error;
	time_t t;

	t = time(NULL);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&t));

	snprintf(dsc_log_file, sizeof(dsc_log_file), PRIxWS, log_file);
	fstream = fopen(dsc_log_file, "a");
	if (fstream == NULL) {
		error = HPE_SPP_LIBRARY_DEP_FAILED;
		ionic_print_error(stderr, "all", "failed to open file" PRIxHS ", SPP error: " PRIxWS "\n",
			dsc_log_file, dsc_text_for_error_code(error));
		return (error);
	}

	fprintf(fstream, "\n\n***********************************************\n");
	fprintf(fstream, "DSC SPP version: %s Build(OS:%s date: %s %s)\n",
		DSC_SPP_VERSION, SPP_BUILD_OS, __DATE__, __TIME__);
	fprintf(fstream, "Start time: %s log file: %s\n",
		buffer, dsc_log_file);

	fflush(fstream);
	fclose(fstream);

	return (HPE_SPP_STATUS_SUCCESS);
}
/*
 * Fill in ionic_info for SPP for *count instances of card.
 * If ionic_info is NULL, return only the count.
 * Return: 0,1,3,4,6,8-11
 */
int
#ifdef _WIN32
__stdcall
#endif
dsc_get_adapter_info(ven_adapter_info *ionic_info, int *count, spp_char_t *firmware_file_path)
{
	struct ionic *ionic;
	FILE *fstream;
	int i, error = HPE_SPP_STATUS_SUCCESS;
	char fw_file_path[HPE_SPP_PATH_MAX_LEN];
	char *intfName;

	snprintf(fw_file_path, sizeof(fw_file_path), PRIxWS, firmware_file_path);
	fstream = fopen(dsc_log_file, "a");
	if (fstream == NULL) {
		error = HPE_SPP_LIBRARY_DEP_FAILED;
		ionic_print_error(stderr, "all", "failed to open file" PRIxHS ", SPP error: " PRIxWS "\n",
			dsc_log_file, dsc_text_for_error_code(error));
		return (error);
	}

	ionic_print_info(fstream, "all", "Enter get_adapter_info\n");
	if (count == NULL) {
		ionic_print_error(fstream, "all", "count is NULL\n");
		error = HPE_SPP_LIBRARY_DEP_FAILED;
		goto err_out;
	}

	if (ionic_info == NULL) {
		error = ionic_init(fstream);
		error = error ? HPE_SPP_HW_ACCESS : HPE_SPP_STATUS_SUCCESS;
		*count = ionic_count;
		ionic_print_info(fstream, "all", "Count of devices: %d\n", *count);
	} else {
		for (i = 0; i < *count; i++) {
			ionic = &ionic_devs[i];
			intfName = ionic->intfName;
			error = ionic_add_spp_entry(fstream, &ionic_info[i], ionic, fw_file_path);
			ionic_print_info(fstream, intfName, "Added entry in ven_adapter_info, error: %d\n",
				error);
			if (error) {
				ionic_print_error(fstream, intfName, "Couldn't add entry, error: %d\n",
					error);
				error =  HPE_SPP_HW_ACCESS;
			}
		}
	}

err_out:
	ionic_print_info(fstream, "all", "Exit get_adapter_info, SPP status: " PRIxWS "\n",
		dsc_text_for_error_code(error));
	fflush(fstream);
	fclose(fstream);

	return (error);
}

int
#ifdef _WIN32
__stdcall
#endif
dsc_do_full_flash_PCI(spp_char_t *firmware_file, int force, uint16_t domain, uint8_t  bus, uint8_t dev,
	uint8_t func)
{
	struct ionic *ionic;
	FILE* fstream, * fw;
	int i, error, secs;
	char fw_file[256];
	char *intfName, *sugg;
#ifndef _WIN32
	struct timeval start, end;
#else
	clock_t start, end;
#endif

	fstream = fopen(dsc_log_file, "a");
	if (fstream == NULL) {
		error = HPE_SPP_LIBRARY_DEP_FAILED;
		ionic_print_error(stderr, "all", "failed to open file" PRIxHS ", SPP error: " PRIxWS "\n",
			dsc_log_file, dsc_text_for_error_code(error));
		return (error);
	}

	snprintf(fw_file, sizeof(fw_file), PRIxWS, firmware_file);

	ionic_print_info(fstream, "all", "Enter flash update for PCI(%d:%d:%d.%d) firmware image: %s\n",
		domain, bus, dev, func, fw_file);
	/* Check firmware file for permission. */
	fw = fopen(fw_file, "rb");
	if (fw == NULL) {
		ionic_print_error(fstream, "all", "failed to access firmware file: %s\n",
			fw_file);
		error = (errno == EACCES) ? HPE_SPP_FW_FILE_PERM_ERR : HPE_SPP_FW_FILE_MISSING;
		goto err_out;
	}
	fclose(fw);

	error = HPE_SPP_STATUS_SUCCESS;
	for (i = 0; i < ionic_count; i++) {
		ionic = &ionic_devs[i];
		intfName = ionic->intfName;
		if ((ionic->domain == domain) && (ionic->bus == bus) &&
			(ionic->dev == dev) && (ionic->func == func)) {
			sugg = ionic_fw_decision(ionic->newFwVer, ionic->curFwVer);
			ionic_print_info(fstream, intfName, "Current firmware: %s, new: %s force: %d,"
				"suggestion: %s\n", ionic->curFwVer, ionic->newFwVer, force, sugg);
			if (!force && (STRCASECMP(sugg, "rewrite") == 0)) {
				error = HPE_SPP_FW_VER_SAME;
				continue;
			}
#ifndef _WIN32
			gettimeofday(&start, NULL);
#else
			start = clock();
#endif
			error = ionic_flash_firmware(fstream, ionic, fw_file);
#ifndef _WIN32
			gettimeofday(&end, NULL);

			secs = end.tv_sec - start.tv_sec;
#else
			end = clock();
			secs = (end-start)/CLOCKS_PER_SEC;
#endif
			if (error) {
				ionic_print_error(fstream, intfName, "Firmware update failed after: %d"
					" seconds, error: %d\n", secs, error);
				error = HPE_SPP_INSTALL_HW_ERROR;
			} else {
				ionic_print_info(fstream, intfName, "Firmware update(upload + install)"
					" took %d seconds\n", secs);
				error = HPE_SPP_REBOOT_AFTER_INSTALL;
			}
		}
	}


err_out:
	ionic_print_info(fstream, "all", "Exit flash update PCI(%d:%d:%d.%d)"
		"firmware image: %s, SPP status: " PRIxWS "\n",
		domain, bus, dev, func, fw_file, dsc_text_for_error_code(error));
	fflush(fstream);
	fclose(fstream);
	return (error);
}
/*
 * Get the flash dump.
 * NOTE: DSC doesn't support flash dump.
 * Return HPE_SPP_FW_VER_LATEST(3) as suggested by HPE team
 */
int
#ifdef _WIN32
__stdcall
#endif
dsc_get_full_flash_dump_PCI(spp_char_t *flash_dump_file, dscFirmwareType fwType,
	uint16_t domain, uint8_t bus, uint8_t dev, uint8_t func)
{
	FILE *fstream;
	int error = HPE_SPP_FW_VER_LATEST;

	fstream = fopen(dsc_log_file, "a");
	if (fstream == NULL) {
		error = HPE_SPP_LIBRARY_DEP_FAILED;
		ionic_print_error(stderr, "all", "failed to open file" PRIxHS ", SPP error: " PRIxWS "\n",
			dsc_log_file, dsc_text_for_error_code(error));
		return (error);
	}

	ionic_print_info(fstream, "all", "PCI(%d:%d:%d.%d), type: %d, file: "
		PRIxWS "SPP status : " PRIxWS "\n", domain, bus, dev, func, fwType, flash_dump_file,
		dsc_text_for_error_code(error));

	fflush(fstream);
	fclose(fstream);

	/* Following return code is as suggested by HPE. */
	return (error);
}

/*
 * Main function to create discovery xml file.
 */
int
#ifdef _WIN32
__stdcall
#endif
dsc_do_discovery_with_files(spp_char_t *discovery_file, spp_char_t *firmware_file_path)
{
	xmlDocPtr doc;
 	xmlNodePtr root, child, devices, device;
	struct ionic *ionic;
	FILE *fstream;
	int i, len, error = HPE_SPP_STATUS_SUCCESS;
	char buffer[256], dis_file[256], fw_file[256];
	char *intfName;

	fstream = fopen(dsc_log_file, "a");
	if (fstream == NULL) {
		error = HPE_SPP_LIBRARY_DEP_FAILED;
		ionic_print_error(stderr, "all", "failed to open file" PRIxHS ", SPP error: " PRIxWS "\n",
			dsc_log_file, dsc_text_for_error_code(error));
		return (error);
	}

	snprintf(dis_file, sizeof(dis_file), PRIxWS, discovery_file);
	snprintf(fw_file, sizeof(fw_file), PRIxWS, firmware_file_path);

	ionic_print_info(fstream, "all", "Enter create discovery file: %s\n", dis_file);

	if (ionic_count == 0) {
		ionic_print_error(fstream, "all", "No Naples devices detected\n");
		error = HPE_SPP_HW_ACCESS;
		goto err_out;
	}

	error = ionic_parse_NICFWData(fstream, fw_file);
	if (error) {
		ionic_print_error(fstream, "all", "%s error\n", HPE_NIC_FW_DATA_FILE);
		error = HPE_SPP_FW_FILE_MISSING;
		goto err_out;
	}

	doc = xmlNewDoc(BAD_CAST "1.0");
	root = xmlNewNode(NULL, BAD_CAST "hp_rom_discovery");
	xmlNewProp(root, BAD_CAST "version", BAD_CAST "2.0.0.0");
	xmlDocSetRootElement(doc, root);

	child = ionic_dis_add_new_node(fstream, "all", root, "type", "value", "");

	snprintf(buffer, sizeof(buffer), "HPE Pensando Online Firmware Upgrade Utility for %s", SPP_BUILD_OS);
	child = ionic_dis_add_new_node(fstream, "all", root, "alt_name", "value", buffer);
	child = ionic_dis_add_new_node(fstream, "all", root, "take_effect", "value", "deferred");

	devices = xmlNewChild(root, NULL, BAD_CAST "devices", NULL);
	if (devices) {
		for (i = 0; i < ionic_count; i++) {
			ionic = &ionic_devs[i];
			intfName = ionic->intfName;
			device = xmlNewChild(devices, NULL, BAD_CAST "device", NULL);
			if (device) {
				ionic_dis_add_dev(fstream, ionic, ionic->fwFile, device);
			} else {
				error = HPE_SPP_DIS_CREATE_FAILED;
				ionic_print_error(fstream, intfName, "Failed to create device node\n");
			}
		}
	} else {
		error = HPE_SPP_DIS_CREATE_FAILED;
		ionic_print_error(fstream, "all", "Failed to create devices node\n");
	}

	// To print on console.
	//xmlSaveFormatFileEnc("-", doc, "UTF-8", 1);
	len = xmlSaveFormatFileEnc(dis_file, doc, "UTF-8", 1);
	if (len < 0) {
		ionic_print_error(fstream, "all", "Failed to save discovery file: %s\n",
			dis_file);
		error = HPE_SPP_DIS_CREATE_FAILED;
	} else {
		ionic_print_info(fstream, "all", "Wrote %d bytes for discovery file: %s\n",
			len, dis_file);
	}
	//xmlMemoryDump();
	if (doc)
		xmlFreeDoc(doc);
	xmlCleanupParser();

err_out:
	ionic_print_info(fstream, "all", "Exit create discovery file: %s, SPP status: " PRIxWS "\n",
		dis_file, dsc_text_for_error_code(error));
	fflush(fstream);
	fclose(fstream);
	return (error);
}

/*
 * HPE SPP API call for flash update.
 */
int
#ifdef _WIN32
__stdcall
#endif
dsc_do_flash_with_file(spp_char_t *discovery_file, spp_char_t *firmware_file_path)
{
	FILE *fstream;
	int error = HPE_SPP_STATUS_SUCCESS;
	char dis_file[256], fw_file[256];

	snprintf(dis_file, sizeof(dis_file), PRIxWS, discovery_file);
	snprintf(fw_file, sizeof(fw_file), PRIxWS, firmware_file_path);
	fstream = fopen(dsc_log_file, "a");
	if (fstream == NULL) {
		error = HPE_SPP_LIBRARY_DEP_FAILED;
		ionic_print_error(stderr, "all", "failed to open file" PRIxHS ", SPP error: " PRIxWS "\n",
			dsc_log_file, dsc_text_for_error_code(error));
		return (error);
	}

	ionic_print_info(fstream, "all", "Flash update with discovery file: %s\n", dis_file);
	error = ionic_parse_discovery(fstream, dis_file);
	if (error) {
		ionic_print_error(fstream, "all", "%s error\n", dis_file);
		error =  (error == EEXIST) ? HPE_SPP_DIS_MISSING : HPE_SPP_DIS_ACCESS;
		goto err_exit;
	}
#if 0
	/*
	 * XXX: not clear if we need to read NICFWData now or its read only
	 * once at discovery time.
	 */
	error = ionic_parse_NICFWData(fstream, fw_file);
	if (error) {
		ionic_print_error(fstream, "all", "%s error\n", HPE_NIC_FW_DATA_FILE);
		error = HPE_SPP_LIBRARY_DEP_FAILED;
	}
#endif
err_exit:
	ionic_print_info(fstream, "all", "Exit, SPP status: " PRIxWS "\n",
		dsc_text_for_error_code(error));
	fflush(fstream);
	fclose(fstream);
	return (error);
}
