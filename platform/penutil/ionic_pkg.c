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
/*
 * Parsing of NICFWData.xml as per new schema.
 * NICFWData.xml is meta file for the firmware package.
 */

xmlNodePtr
ionic_find_child(FILE *fstream, xmlNodePtr parent, const char *name, const char *childName)
{
	xmlNodePtr node, node1;

	ionic_print_debug(fstream, "all", "Searching for %s under %s(%s) in %s\n",
		childName, name, parent->name, HPE_NIC_FW_DATA_FILE);
	for (node = parent; node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;

		if (xmlStrcasecmp(node->name, (const xmlChar *)name)) {
			ionic_print_debug(fstream, "all", "Unrecognized node: %s"
				" under %s\n", node->name, name);
			continue;
		}
		for (node1 = node->children; node1; node1 = node1->next) {
			ionic_print_debug(fstream, "all", "Child %s under %s\n", node1->name, name);
			if (xmlStrcasecmp(node1->name, (const xmlChar *)childName) == 0) {
				ionic_print_info(fstream, "all", "Found %s node under %s\n",
					childName, name);
				return (node1);
			}
		}
	}

	return (NULL);
}

/*
 * Parse input file to get version number and file name.
 */
int
ionic_parse_nicfwdata_for_fwfile(FILE *fstream, xmlNodePtr root, char *file, int buf_len,
	char *ver_str, int ver_len)
{
	xmlNodePtr payload, devices, device, fwimages, fwfile, version;
	int error = 0;
	const char *payload_name = "payload";
	const char *devices_name = "Devices";
	const char *device_name = "Device";
	const char *firmwareimages_name = "FirmwareImages";
	const char *fwfile_name = "FileName";
	const char *version_name = "Version";
	xmlChar *temp;

	payload = ionic_find_child(fstream, root, "cpq_package", payload_name);
	if (payload == NULL){
		ionic_print_error(fstream, "all", "%s not under %s\n",
			payload_name, "cpq_package");
		return (ENXIO);
	}

	devices = ionic_find_child(fstream, payload, payload_name, devices_name);
	if (devices == NULL){
		ionic_print_error(fstream, "all", "%s not under %s\n",
			devices_name, payload_name);
		return (ENXIO);
	}

	device = ionic_find_child(fstream, devices, devices_name, device_name);
	if (device == NULL) {
		ionic_print_error(fstream, "all", "%s not under %s\n",
			device_name, devices_name);
		return (ENXIO);
	}

	version = ionic_find_child(fstream, device, device_name, version_name);
	if (version == NULL) {
		ionic_print_error(fstream, "all", "%s not under %s\n",
			version_name, device_name);
		return (ENXIO);
	}

	temp = xmlNodeGetContent(version);
	strncpy(ver_str, (char *)temp, ver_len);
	xmlFree(temp);

	fwimages = ionic_find_child(fstream, device, device_name, firmwareimages_name);
	if (fwimages == NULL) {
		ionic_print_error(fstream, "all", "%s not under %s\n",
			firmwareimages_name, device_name);
		return (ENXIO);
	}

	fwfile = ionic_find_child(fstream, fwimages, firmwareimages_name, fwfile_name);
	if (fwimages == NULL) {
		ionic_print_error(fstream, "all", "%s not under %s\n",
			fwfile_name, firmwareimages_name);
		return (ENXIO);
	}

	temp = xmlNodeGetContent(fwfile);
	strncpy(file, (char *)temp, buf_len);
	xmlFree(temp);

	return (error);
}
/*
 * Parse the NICFWData.xml and update ionic with available firmware version details.
 */
int
ionic_parse_NICFWData(FILE *fstream, char *firmware_file_path)
{
	xmlDoc *doc = NULL;
	xmlNodePtr root;
	struct ionic *ionic;
	char nic_file[512], fw_file[256], version[256];
	char *intfName;
	int i, error = 0;

	snprintf(nic_file, sizeof(nic_file), "%s/%s", firmware_file_path, HPE_NIC_FW_DATA_FILE);
	doc = xmlReadFile(nic_file, NULL, 0);
	if (doc == NULL) {
		ionic_print_error(fstream, "all", "%s file not found\n", nic_file);
		return (HPE_SPP_DSC_NICFWDATA_ERROR);
	}

	ionic_print_info(fstream, "all", "Parsing %s\n", nic_file);
	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		ionic_print_error(fstream, "all", "Couldn't parse %s\n", nic_file);
		error = HPE_SPP_DSC_NICFWDATA_ERROR;
		goto err_exit;
	}
	error = ionic_parse_nicfwdata_for_fwfile(fstream, root, fw_file, sizeof(fw_file),
			version, sizeof(version));

	/*
	 * Make sure that we have list of firmware for all the devices in system.
	 * XXX: should we skip device if matching firmware is not found?
	 */
	for (i = 0; i < ionic_count; i++) {
		ionic = &ionic_devs[i];
		intfName = ionic->intfName;
		strncpy(ionic->fwFile, fw_file, sizeof(ionic->fwFile));
		strncpy(ionic->newFwVer, version, sizeof(ionic->newFwVer));
#if 0 /* XXX */
		if (ionic->fwFile[0] == 0) {
			ionic_print_error(fstream, intfName,
				"No firmware file specified in %s for (0x%X 0x%x 0x%x 0x%x)\n",
				HPE_NIC_FW_DATA_FILE, ionic->venId, ionic->devId, ionic->subVenId,
				ionic->subDevId);
			error = HPE_SPP_DSC_NICFWDATA_ERROR;
			goto err_exit;
		}
#endif
	}

err_exit:
	//xmlMemoryDump();
	xmlCleanupParser();
	if (doc)
		xmlFreeDoc(doc);
	return (error);
}

