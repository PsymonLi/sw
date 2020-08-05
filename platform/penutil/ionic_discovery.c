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
 * Helper function to create discovery xml node.
 */
xmlNodePtr
ionic_dis_add_new_node(FILE *fstream, const char *intfName, xmlNodePtr parent, const char *name,
	const char *attr, const char *value)
{
	xmlNodePtr child;

	child = xmlNewChild(parent, NULL, BAD_CAST name, NULL);

	if (child) {
		xmlNewProp(child, BAD_CAST attr, BAD_CAST value);
		ionic_print_debug(fstream, intfName, "Added node: %s with attribute: %s(%s)\n",
			name, attr, value);
	} else {
		ionic_print_error(fstream, intfName,
			"Failed to add node: %s with attribute: %s value: %s\n",
			name, attr, value);
	}
	return (child);
}

/*
 * Helper function to create fw xml node.
 */
static int
ionic_dis_add_fw(FILE *fstream, struct ionic *ionic, const char *fw_file, xmlNodePtr fw)
{
 	xmlNodePtr child;
	char buffer[256];
	char *intfName;
	uint32_t new_maj, new_min, new_rel, cur_maj, cur_min, cur_rel;

	intfName = ionic->intfName;
	ionic_print_debug(fstream, intfName, "Adding fw entry in discovery\n");

	child = ionic_dis_add_new_node(fstream, intfName, fw, "type", "value", IONIC_SPP_FW_TYPE);
	child = ionic_dis_add_new_node(fstream, intfName, fw, "firmware_id", "value", "");
	child = ionic_dis_add_new_node(fstream, intfName, fw, "firmware_file", "value",
		fw_file);

	ionic_encode_fw_version(ionic->newFwVer, &new_maj, &new_min, &new_rel);
	snprintf(buffer, sizeof(buffer), "%d.%d.%d", new_maj, new_min, new_rel);
	ionic_print_info(fstream, intfName, "Adding entry for version: %s(%s)\n",
		buffer, ionic->newFwVer);
	child = ionic_dis_add_new_node(fstream, intfName, fw, "version", "value", buffer);

	ionic_encode_fw_version(ionic->curFwVer, &cur_maj, &cur_min, &cur_rel);
	snprintf(buffer, sizeof(buffer), "%d.%d.%d", cur_maj, cur_min, cur_rel);
	ionic_print_info(fstream, intfName, "Adding entry for active_version: %s(%s)\n",
		buffer, ionic->curFwVer);
	child = ionic_dis_add_new_node(fstream, intfName, fw, "active_version", "value", buffer);

	strncpy(buffer, ionic_fw_decision2(new_maj, new_min, new_rel, cur_maj,
		cur_min, cur_rel), sizeof(buffer));
	child = ionic_dis_add_new_node(fstream, intfName, fw, "action", "value", buffer);

	if (STRCASECMP(buffer, "skip") == 0) {
		child = ionic_dis_add_new_node(fstream, intfName, fw, "action_status", "value", "");
	} else {
		child = ionic_dis_add_new_node(fstream, intfName, fw, "action_status", "value", "1");
	}

	/* Max time for update in seconds, 30 mins for CPLD update */
	child = ionic_dis_add_new_node(fstream, intfName, fw, "duration", "value",
		NAPLES_FW_UPDATE_MAX_TIME_SECS);
	/* Reserved for HPE */
	child = ionic_dis_add_new_node(fstream, intfName, fw, "message", "value", "");
	child = ionic_dis_add_new_node(fstream, intfName, fw, "shared", "value", "no");

	return (0);
}

/*
 * Helper function to create dev xml node.
 */
int
ionic_dis_add_dev(FILE *fstream, struct ionic *ionic, char *fw_file, xmlNodePtr device)
{
	xmlNodePtr child, fw;
	char buffer[256];
	char *intfName;

	intfName = ionic->intfName;
	ionic_print_debug(fstream, intfName, "Adding dev entry in discovery file\n");

	child = ionic_dis_add_new_node(fstream, intfName, device, "device_id", "value", intfName);

	ionic_desc_name(ionic, buffer, sizeof(buffer));
	child = ionic_dis_add_new_node(fstream, intfName, device, "nictype", "value", buffer);

	snprintf(buffer, sizeof(buffer), "%x", ionic->venId);
	child = ionic_dis_add_new_node(fstream, intfName, device, "venid", "value", buffer);

	snprintf(buffer, sizeof(buffer), "%x", ionic->devId);
	child = ionic_dis_add_new_node(fstream, intfName, device, "devid", "value", buffer);

	snprintf(buffer, sizeof(buffer), "%x", ionic->subVenId);
	child = ionic_dis_add_new_node(fstream, intfName, device, "subvenid", "value", buffer);

	snprintf(buffer, sizeof(buffer), "%x", ionic->subDevId);
	child = ionic_dis_add_new_node(fstream, intfName, device, "subdevid", "value", buffer);

	snprintf(buffer, sizeof(buffer), "HPE Pensando DSC adapter");
	child = ionic_dis_add_new_node(fstream, intfName, device, "product_id", "value", buffer);

	snprintf(buffer, sizeof(buffer), "%x", ionic->domain);
	child = ionic_dis_add_new_node(fstream, intfName, device, "segment", "value", buffer);

	snprintf(buffer, sizeof(buffer), "%x", ionic->bus);
	child = ionic_dis_add_new_node(fstream, intfName, device, "busnumber", "value", buffer);

	snprintf(buffer, sizeof(buffer), "%x", ionic->dev);
	child = ionic_dis_add_new_node(fstream, intfName, device, "devicenumber", "value", buffer);

	snprintf(buffer, sizeof(buffer), "%x", ionic->func);
	child = ionic_dis_add_new_node(fstream, intfName, device, "functionnumber", "value", buffer);

	child = ionic_dis_add_new_node(fstream, intfName, device, "slotnumber", "value", ionic->slotInfo);

	snprintf(buffer, sizeof(buffer), "%s", ionic->macAddr);
	child = ionic_dis_add_new_node(fstream, intfName, device, "macaddress", "value", buffer);
	fw = xmlNewChild(device, NULL, BAD_CAST "fw_item", NULL);
	if (fw)
		ionic_dis_add_fw(fstream, ionic, fw_file, fw);
	else
		ionic_print_error(fstream, intfName, "Failed to create device fw node\n");

	return (0);
}

/*
 * Parse the NICFWData.xml nic node and update ionic with
 * available firmware version details.
 */
int
ionic_parse_NICFWData_nic(FILE *fstream, xmlNodePtr nic)
{
	struct ionic *ionic;
	xmlNodePtr node;
	int i;
	char *intfName, *venid, *devid, *subvenid, *subdevid, *fw_ver, *fw_file;
	uint32_t venId, devId, subVenId, subDevId;

	ionic_print_debug(fstream, "all", "Parsing nic nodes from %s\n",
		HPE_NIC_FW_DATA_FILE);
	for (node = nic; node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;

		if (xmlStrcasecmp(node->name, (const xmlChar *)"nic")) {
			ionic_print_info(fstream, "all", "Unrecognized node: %s,"
				" expecting nic under nic\n", node->name);
			continue;
		}

		venid = (char *)xmlGetProp(node, BAD_CAST "venid");
		devid = (char *)xmlGetProp(node, BAD_CAST "devid");
		subvenid = (char *)xmlGetProp(node, BAD_CAST "subvenid");
		subdevid = (char *)xmlGetProp(node, BAD_CAST "subdevid");
		ionic_print_debug(fstream, "", "Found entry in %s for (%s %s %s %s)\n",
			HPE_NIC_FW_DATA_FILE, venid, devid, subvenid, subdevid);
		if ((venid == NULL) || (devid == NULL) || (subvenid == NULL) || (subdevid == NULL))
			continue;
		/* XXX: is format guaranteed at hex. */
		venId = strtoul(venid, 0, 16);
		devId = strtoul(devid, 0, 16);
		subVenId = strtoul(subvenid, 0, 16);
		subDevId = strtoul(subdevid, 0, 16);
		xmlFree(venid);
		xmlFree(devid);
		xmlFree(subvenid);
		xmlFree(subdevid);
		for (i = 0; i < ionic_count; i++) {
			ionic = &ionic_devs[i];
			intfName = ionic->intfName;
			/*
			 * We use only PCI venId and devId for now.
			 */
			if ((venId == ionic->venId) && (devId == ionic->devId) &&
				(subVenId == ionic->subVenId) && (subDevId == ionic->subDevId)) {
				ionic_print_debug(fstream, intfName, "%s has entry for (0x%x 0x%x 0x%x 0x%x\n",
					HPE_NIC_FW_DATA_FILE, venId, devId, subVenId, subDevId);
				fw_ver = (char *)xmlGetProp(node, BAD_CAST IONIC_SPP_FW_TYPE);
				fw_file = (char *)xmlGetProp(node, BAD_CAST IONIC_SPP_FW_TYPE_FILE);
				if ((fw_ver == NULL) || (fw_file == NULL)) {
					ionic_print_error(fstream, intfName,
						"%s or %s entry missing in %s\n",
						IONIC_SPP_FW_TYPE, IONIC_SPP_FW_TYPE_FILE,
						HPE_NIC_FW_DATA_FILE);

					return (HPE_SPP_DSC_NICFWDATA_ERROR);
				}
				ionic_spp2pen_verstr(fw_ver, ionic->newFwVer, sizeof(ionic->newFwVer));
				ionic_print_info(fstream, intfName, "Firmware from %s"
					" version: %s(%s) file: %s\n",
					HPE_NIC_FW_DATA_FILE, fw_ver, ionic->newFwVer, fw_file);
				strncpy(ionic->fwFile, fw_file, sizeof(ionic->fwFile));
				xmlFree(fw_ver);
				xmlFree(fw_file);
			}
		}
	}

	return (0);
}

/*
 * Parse the NICFWData.xml and update ionic with available firmware version details.
 */
int
ionic_parse_NICFWData(FILE *fstream, char *firmware_file_path)
{
	xmlDoc *doc = NULL;
	xmlNodePtr root, node, child;
	struct ionic *ionic;
	char nic_file[512];
	char *ver, *intfName;
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

	for (node = root; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE) {
			if (xmlStrcasecmp(node->name, (const xmlChar *)"nic_fw_package") == 0) {
				ver = (char *)xmlGetProp(node, BAD_CAST "version");
				if (ver) {
					ionic_print_info(fstream, "all", "nic_fw_type with version: %s\n", ver);
					xmlFree(ver);
				}
			}
		}
	}


	for (node = root->children; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE) {
			if (xmlStrcasecmp(node->name, (const xmlChar *)"nic") == 0) {
				child = node->children;
				if (child) {
					ionic_print_debug(fstream, "all", "Found nic node\n");
					error = ionic_parse_NICFWData_nic(fstream, child);
					if (error)
						goto err_exit;
				}
			}
		}
	}

	for (i = 0; i < ionic_count; i++) {
		ionic = &ionic_devs[i];
		intfName = ionic->intfName;
		if (ionic->fwFile[0] == 0) {
			ionic_print_error(fstream, intfName,
				"No firmware file specified in %s for (0x%X 0x%x 0x%x 0x%x)\n",
				HPE_NIC_FW_DATA_FILE, ionic->venId, ionic->devId, ionic->subVenId,
				ionic->subDevId);
			error = HPE_SPP_DSC_NICFWDATA_ERROR;
			goto err_exit;
		}
	}

err_exit:
	//xmlMemoryDump();
	xmlCleanupParser();
	if (doc)
		xmlFreeDoc(doc);
	return (error);
}

/*
 * Parse the fw_items in discovery file.
 */
static int
ionic_parse_disc_fw_items(FILE *fstream, struct ionic *ionic, xmlNodePtr device)
{
	xmlNodePtr node;
	char *intfName, *value;
	char buffer[32];
	int error = 0;

	intfName = ionic->intfName;
	ionic_print_debug(fstream, intfName, "Parsing discovery file for fw_items\n");
	for (node = device; node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;

		if (xmlStrcasecmp(node->name, (const xmlChar *)"type") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic_print_debug(fstream, intfName, "%s: %s\n", node->name, value);
				if (STRCASECMP(value, IONIC_SPP_FW_TYPE) != 0) {
					ionic_print_error(fstream, intfName,
						"Unknown type for fw_items: %s\n", value);
					xmlFree(value);
					return (EINVAL);
				}
			} else {
				ionic_print_error(fstream, intfName, "Missing type value for %s\n", node->name);
				return (EINVAL);
			}
		}

		if (xmlStrcasecmp(node->name, (const xmlChar *)"firmware_file") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic_print_debug(fstream, intfName, "%s: %s\n", node->name, value);
				strncpy(ionic->fwFile, value, sizeof(ionic->fwFile));
				xmlFree(value);
			}
		}
		if (xmlStrcasecmp(node->name, (const xmlChar *)"version") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic_print_debug(fstream, intfName, "%s: %s\n", node->name, value);
				ionic_spp2pen_verstr(value, ionic->newFwVer, sizeof(ionic->newFwVer));
				ionic_print_debug(fstream, intfName, "SPP ver: %s -> Pensando ver: %s\n",
					value, ionic->newFwVer);
				//strncpy(ionic->newFwVer, value, sizeof(ionic->newFwVer));
				xmlFree(value);
			}
		}
		if (xmlStrcasecmp(node->name, (const xmlChar *)"active_version") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic_print_debug(fstream, intfName, "%s: %s\n", node->name, value);
				ionic_spp2pen_verstr(value, ionic->curFwVer, sizeof(ionic->curFwVer));
				//strncpy(ionic->curFwVer, value, sizeof(ionic->curFwVer));
				ionic_print_info(fstream, intfName, "Active SPP ver: %s Pensando ver: %s\n",
					value, ionic->curFwVer);
				xmlFree(value);
			}
		}
		if (xmlStrcasecmp(node->name, (const xmlChar *)"action") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic_print_debug(fstream, intfName, "%s: %s\n", node->name, value);
				strncpy(ionic->action, value, sizeof(ionic->action));
				xmlFree(value);
			}
		}

		if (STRCASECMP(ionic->action, "Skip") == 0)
			continue;
		/* We update action status after running the actual firmware update. */
		if (xmlStrcasecmp(node->name, (const xmlChar *)"action_status") == 0) {
			/* If firmware file name is missing, set the value "" as per spec. */
			if (strlen(ionic->fwFile) == 0) {
				xmlSetProp(node, BAD_CAST "value", BAD_CAST "");
			} else {
				error = ionic_flash_firmware(fstream, ionic, ionic->fwFile);
				/* Set '1' to indicate we need reboot. */
				if (error == 0) {
					snprintf(buffer, sizeof(buffer), "%d",
						HPE_SPP_REBOOT_AFTER_INSTALL);
				} else {
					ionic_print_error(fstream, intfName, "Flash update failed, error: %d\n",
						error);
					snprintf(buffer, sizeof(buffer), "%d", error);
					break;
				}
				xmlSetProp(node, BAD_CAST "value", BAD_CAST buffer);
			}
		}
	}

	return (error);
}

/*
 * Parse the discovery file device node which is per management device.
 */
static int
ionic_parse_dis_device(FILE *fstream, struct ionic *ionic, xmlNodePtr device)
{
	xmlNodePtr node, child;
	char *intfName, *value;
	int error = 0;

	intfName = ionic->intfName;
	ionic_print_debug(fstream, intfName, "Parsing discovery for device\n");
	for (node = device; node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;

		if (xmlStrcasecmp(node->name, (const xmlChar *)"device_id") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				strncpy(ionic->intfName, value, sizeof(ionic->intfName));
				ionic_print_debug(fstream, intfName, "%s: %s\n",
					node->name, value);
				xmlFree(value);
			}
		}
		if (xmlStrcasecmp(node->name, (const xmlChar *)"segment") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic->domain = (uint16_t)strtoul(value, 0, 16);
				ionic_print_debug(fstream, intfName, "%s: %s\n",
					node->name, value);
				xmlFree(value);
			}
		}
		if (xmlStrcasecmp(node->name, (const xmlChar *)"busnumber") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic->bus = (uint8_t)strtoul(value, 0, 16);
				ionic_print_debug(fstream, intfName, "%s: %s\n",
					node->name, value);
				xmlFree(value);
			}
		}
		if (xmlStrcasecmp(node->name, (const xmlChar *)"devicenumber") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic->dev = (uint8_t)strtoul(value, 0, 16);
				ionic_print_debug(fstream, intfName, "%s: %s\n",
					node->name, value);
				xmlFree(value);
			}
		}
		if (xmlStrcasecmp(node->name, (const xmlChar *)"funcnumber") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic->func = (uint8_t)strtoul(value, 0, 16);
				ionic_print_debug(fstream, intfName, "%s: %s\n",
					node->name, value);
				xmlFree(value);
			}
		}
		if (xmlStrcasecmp(node->name, (const xmlChar *)"macaddress") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				strncpy(ionic->macAddr, value, sizeof(ionic->macAddr));
				ionic_print_debug(fstream, intfName, "%s: %s\n",
					node->name, value);
				xmlFree(value);
			}
		}
		if (xmlStrcasecmp(node->name, (const xmlChar *)"slotnumber") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				strncpy(ionic->slotInfo, value, sizeof(ionic->slotInfo));
				ionic_print_debug(fstream, intfName, "%s: %s\n",
					node->name, value);
				xmlFree(value);
			}
		}
		if (xmlStrcasecmp(node->name, (const xmlChar *)"fw_item") == 0) {
			child = node->children;
			if (child) {
				error = ionic_parse_disc_fw_items(fstream, ionic, child);
				if (error) {
					ionic_print_error(fstream, intfName,
						"Failed to parse fw_items, error: %d\n",
						error);
					break;
				}
			}
		}
	}

	return (error);
}

/*
 * Parse enough of device node to confirm if its Naples.
 */
static int
ionic_parse_dis_device_is_naples(FILE *fstream, struct ionic *ionic, xmlNodePtr device)
{
	xmlNodePtr node;
	char *intfName, *value;
	int error = EINVAL;

	intfName = ionic->intfName;
	ionic_print_debug(fstream, intfName, "Parsing discovery device for venid and devid\n");
	for (node = device; node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;

		if (xmlStrcasecmp(node->name, (const xmlChar *)"venid") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic->venId = (uint16_t)strtoul(value, 0, 16);
				ionic_print_debug(fstream, intfName, "%s: %s\n",
					node->name, value);
				xmlFree(value);
			}
		}

		if (xmlStrcasecmp(node->name, (const xmlChar *)"devid") == 0) {
			value = (char *)xmlGetProp(node, BAD_CAST "value");
			if (value) {
				ionic->devId = (uint16_t)strtoul(value, 0, 16);
				ionic_print_debug(fstream, intfName, "%s: %s\n",
					node->name, value);
				xmlFree(value);
			}
		}

		if ((ionic->venId == PCI_VENDOR_ID_PENSANDO) && (ionic->devId == ionic_devid)) {
			error = 0;
			break;
		}
	}

	return (error);
}
/*
 * Helper function to parse discovery device nodes.
 */
int
ionic_parse_discovery_devices(FILE *fstream, xmlNodePtr device)
{
	xmlNodePtr node, child;
	struct ionic *ionic;
	int error = 0;

	ionic_print_debug(fstream, "all", "Parsing discovery devices\n");
	for (node = device; node; node = node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;

		if (node && xmlStrcasecmp(node->name, (const xmlChar *)"device") == 0) {
			ionic_print_debug(fstream, "all", "devices child: %s\n", node->name);
			child = node->children;
			ionic_print_debug(fstream, "all", "device child: %s\n", child->name);
			ionic = &ionic_devs[ionic_count++];
			if (child) {
				/*
				 * We scan two times, first for ven and dev Id and next for rest
				 * of the attributes since its possible that order of nodes is
				 * not guaranteed and venid and devid can show in the end of list.
				 */
				error = ionic_parse_dis_device_is_naples(fstream, ionic, child);
				if (error)
					continue;
				error = ionic_parse_dis_device(fstream, ionic, child);
				if (error)
					break;
				ionic_print_info(fstream, ionic->intfName, "From discovery file"
					" PCI config: %d:%d.%d.%d"
					" fw version: %s slot: %s mac: %s\n",
					ionic->domain, ionic->bus, ionic->dev, ionic->func,
					ionic->curFwVer, ionic->slotInfo, ionic->macAddr);
			}
		}
	}

	return (error);
}
/*
 * Read the discovery file and populate the ionic tables.
 */
int
ionic_parse_discovery(FILE *fstream, const char *dis_file)
{
	xmlDoc *doc = NULL;
	xmlNodePtr root, node, child;
	char *ver;
	int error = 0;

	doc = xmlReadFile(dis_file, NULL, 0);
	if (doc == NULL) {
		ionic_print_error(fstream, "all", "%s file not found\n", dis_file);
		return (HPE_SPP_DIS_MISSING);
	}

	ionic_print_debug(fstream, "all", "Parsing %s\n", dis_file);
	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		ionic_print_error(fstream, "all", "Couldn't parse %s\n", dis_file);
		error = HPE_SPP_DIS_ACCESS;
		goto err_exit;
	}

	for (node = root; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE) {
			if (xmlStrcasecmp(node->name, (const xmlChar *)"hp_rom_discovery") == 0) {
				ver = (char *)xmlGetProp(node, BAD_CAST "version");
				if (ver) {
					ionic_print_info(fstream, "all",
						"hp_rom_discovery with version: %s\n", ver);
					xmlFree(ver);
				}
			}
		}
	}

	for (node = root->children; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE) {
			if (xmlStrcasecmp(node->name, (const xmlChar *)"devices") == 0) {
				ionic_print_debug(fstream, "all", "Found devices node\n");
				child = node->children;
				if (child) {
					error = ionic_parse_discovery_devices(fstream, child);
					if (error)
						break;
				}
			}
		}
	}

err_exit:
	xmlMemoryDump();
	xmlFreeDoc(doc);
	xmlCleanupParser();
	return (error);
}
