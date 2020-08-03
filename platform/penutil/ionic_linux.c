/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "ionic_os.h"

#include <net/if.h>

#include <linux/ethtool.h>
#include <linux/sockios.h>

/*
 * Function to read individual sysfs entries.
 * Return null terminated string.
 */
static int
ionic_sysfs_entry(FILE *fstream, char *intf, char *dir, char *file, char *data, int data_len)
{
	char path[80];
	int fd, len;

	memset(data, 0, data_len);
	if (dir)
		snprintf(path, sizeof(path), "/sys/class/net/%s/%s/%s", intf, dir, file);
	else
		snprintf(path, sizeof(path), "/sys/class/net/%s/%s", intf, file);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		ionic_print_error(fstream, "", "Failed to read :%s, error: %s\n",
			path, strerror(errno));
		return (ENOENT);
	}

	len = read(fd, data, data_len -1);
	if (len >= 0)
		data[len] = 0;

	ionic_print_debug(fstream, "", "File :%s, data: %s\n", path, data);
	close(fd);

	return (0);
}

/*
 * Read PCI config address from sysfs.
 */
static int
ionic_pci_dbsf(FILE *fstream, char *intf, struct ionic *ionic)
{
	char path[80], buffer[80], *ptr;
	int i, len;

	memset(buffer, 0, sizeof(buffer));
	snprintf(path, sizeof(path), "/sys/class/net/%s/device", intf);

	len = readlink(path, buffer, sizeof(buffer) - 1);
	if (len < 0) {
		ionic_print_error(fstream, ionic->intfName, "Failed to read :%s, error: %s\n",
			path, strerror(errno));
		return (ENXIO);
	}

	buffer[len] = 0;
	ionic_print_info(fstream, ionic->intfName, "PCI config address is: %s\n", buffer);
	ptr = NULL;
	/* Find the filename which is based on PCI domain, bus, dev and func value */
	for (i = len - 1; i >= 0 && buffer[i]; i--) {
		if (buffer[i] == '/') {
			break;
		}
		ptr = &buffer[i];
	}

	if (ptr == NULL) {
		ionic_print_error(fstream, ionic->intfName, "Failed to parse :%s\n", buffer);
		return (ENXIO);
	}

	len = sscanf(ptr, "%hx:%hhx:%hhx.%hhx", &ionic->domain, &ionic->bus, &ionic->dev, &ionic->func);
	if (len <= 0) {
		ionic_print_error(fstream, ionic->intfName,
			"Failed to parse :%s, error: %s\n", ptr, strerror(errno));
		return (ENXIO);
	}

	return (0);
}
/*
 * Check if an interface is Naples device
 */
static int
ionic_find_one(FILE *fstream, char *intf, struct ionic *ionic)
{
	char buffer[32];
	int error;
	uint16_t venId, devId;
	uint32_t eth[6];

	/* Read PCI vendor ID. */
	error = ionic_sysfs_entry(fstream, intf, "device", "vendor", buffer, sizeof(buffer));
	if (error) {
		return (error);
	}
	venId = strtoul(buffer, NULL, 0);

	/* Read PCI device ID. */
	error = ionic_sysfs_entry(fstream, intf, "device", "device", buffer, sizeof(buffer));
	if (error) {
		return (error);
	}
	devId = strtoul(buffer, NULL, 0);

	if ((venId != PCI_VENDOR_ID_PENSANDO) || (devId != ionic_devid))
		return (EINVAL);

	ionic->venId = venId;
	ionic->devId = devId;
	strncpy(ionic->intfName, intf, sizeof(ionic->intfName));

	/* Read the PCI address. */
	error = ionic_pci_dbsf(fstream, intf, ionic);
	if (error) {
		return (error);
	}

	error = ionic_sysfs_entry(fstream, intf, "device", "subsystem_vendor", buffer, sizeof(buffer));
	if (error) {
		return (error);
	}
	ionic->subVenId = strtoul(buffer, NULL, 0);

	error = ionic_sysfs_entry(fstream, intf, "device", "subsystem_device", buffer, sizeof(buffer));
	if (error) {
		return (error);
	}
	ionic->subDevId = strtoul(buffer, NULL, 0);

	error = ionic_sysfs_entry(fstream, ionic->intfName, NULL, "address", buffer,
		sizeof(buffer));
	sscanf(buffer, "%x:%x:%x:%x:%x:%x",
		&eth[5], &eth[4], &eth[3], &eth[2], &eth[1], &eth[0]);
	snprintf(ionic->macAddr, sizeof(ionic->macAddr), "%02X:%02X:%02X:%02X:%02X:%02x",
		eth[5], eth[4], eth[3], eth[2], eth[1], eth[0]);
	if (error) {
		return (error);
	}

	return (0);
}

int
ionic_find_devices(FILE *fstream, struct ionic ionic_devs[], int *count)
{
	struct dirent *de;
	const char *sys_net = "/sys/class/net/";
	DIR *dr = opendir(sys_net);
	struct ionic *ionic;
	int error;

	*count = 0;
	ionic_print_info(fstream, "", "Scanning using sysfs\n");

	if (dr == NULL) {
		ionic_print_error(fstream, "", "Couldn't open: %s, error: %s\n",
			sys_net, strerror(errno));
		return (ENXIO);
	}

	while ((de = readdir(dr)) != NULL) {
		ionic = &ionic_devs[(*count)];
		if ((strcmp(de->d_name, ".") == 0) ||
			(strcmp(de->d_name, "..") == 0) ||
			(strcmp(de->d_name, "lo") == 0))
		continue;

		error = ionic_find_one(fstream, de->d_name, ionic);
		if (error == 0) {
			ionic_print_info(fstream, de->d_name, "Naples device is: %s\n", ionic->intfName);
			(*count)++;
		}
	}

	closedir(dr);
	return (0);
}

int
ionic_get_details(FILE *fstream, struct ionic *ionic)
{
	struct ifreq ifr;
	struct ethtool_drvinfo drvinfo;
	int fd, error;
	char *intfName;

	intfName = ionic->intfName;
	ionic_print_info(fstream, intfName, "Getting various details\n");

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ionic_print_error(fstream, intfName, "Failed to open socket for ethtool, error: %s\n",
			strerror(errno));
		return (errno);
	}

	memset(&ifr, 0, sizeof(ifr));
	drvinfo.cmd = ETHTOOL_GDRVINFO;
	ifr.ifr_data = (void *)&drvinfo;
	strcpy(ifr.ifr_name, ionic->intfName);

	error = ioctl(fd, SIOCETHTOOL, &ifr);
	if (error < 0) {
		ionic_print_error(fstream, intfName, "ethtool cmd: %d failed for: %s, error: %s\n",
			drvinfo.cmd, ionic->intfName, strerror(errno));
		close (fd);
		return (errno);
	}

	ionic_print_info(fstream, intfName, "driver: %s version: %s fw-version: %s slot: %s\n",
		drvinfo.driver, drvinfo.version, drvinfo.fw_version, drvinfo.bus_info);

	strncpy(ionic->curFwVer, drvinfo.fw_version, sizeof(ionic->curFwVer));
	strncpy(ionic->drvVer, drvinfo.version, sizeof(ionic->drvVer));
	strncpy(ionic->slotInfo, drvinfo.bus_info, sizeof(ionic->slotInfo));

	close (fd);

	return (0);
}

/*
 * Copy firmware file to /lib/firmware
 */
static int
ionic_linux_copy_fw_file(FILE *fstream, const char *intfName, const char *src, const char *dest)
{
	FILE *f1, *f2;
	uint64_t count = 0;
	char buffer[1];

	f1 = fopen(src, "rb");
	if (f1 == NULL) {
		ionic_print_error(fstream, intfName, "Couldn't open file: %s, error: %s\n",
			src, strerror(errno));
		return (errno);
	}

	f2 = fopen(dest, "wb");
	if (f2 == NULL) {
		ionic_print_error(fstream, intfName, "Couldn't create file: %s, error: %s\n",
			dest, strerror(errno));
		fclose(f1);
		return (EIO);
	}

	while (fread(&buffer, 1, 1, f1) > 0) {
		count += fwrite(&buffer, 1, 1, f2);
	}

	ionic_print_info(fstream, intfName, "Copied Src: %s Dest: %s size: %ld MB\n",
		src, dest, SIZE_1MB(count));
	fflush(f2);
	fclose(f2);

	fclose(f1);
	return (0);
}
/*
 * Flash the firmware file as pointed by fw_file to device.
 * We use ethtool to flash, which require file to be present in
 * /lib/firmware, copy the file in the location and invoke ethtool.
 */
int
ionic_flash_firmware(FILE *fstream, struct ionic *ionic, char *fw_file_name)
{
	struct ifreq ifr;
	struct ethtool_flash efl;
	struct timeval start, end;
	int i, fd, error;
	char *intfName;
	const char LIB_FW_PATH[] = "/lib/firmware/ionic";
	char dest_file[256];
	char path[512];

	intfName = ionic->intfName;
	ionic_print_info(fstream, intfName, "Enter firmware update\n");

	error = mkdir(LIB_FW_PATH, 0);
	if (error && (errno != EEXIST)) {
		ionic_print_error(fstream, intfName, "Couldn't access %s, error: %s\n",
			LIB_FW_PATH, strerror(errno));
		return (error);
	}

	/* Extract the firmware file name, always have at least one '/'. */
	for (i = strlen(fw_file_name) - 1; i >= 0; i--) {
		if (fw_file_name[i] == '/') {
			strncpy(dest_file, &fw_file_name[i + 1], sizeof(dest_file));
			break;
		}
	}

	snprintf(path, sizeof(path), "%s/%s", LIB_FW_PATH, dest_file);
	ionic_print_debug(fstream, intfName, "Source fw file: %s destination file: %s\n",
		fw_file_name, path);

	error = ionic_linux_copy_fw_file(fstream, ionic->intfName, fw_file_name, path);
	if (error) {
		ionic_print_error(fstream, intfName, "Couldn't copy file: %s, error: %s\n",
			path, strerror(errno));
		return (error);
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ionic_print_error(fstream, intfName,
			"Failed to open socket for flash update, error: %s\n",
			strerror(errno));
		return (errno);
	}

	memset(&ifr, 0, sizeof(ifr));
	efl.cmd = ETHTOOL_FLASHDEV;
	snprintf(efl.data, sizeof(efl.data), "ionic/%s", dest_file);
	ifr.ifr_data = (void *)&efl;
	strncpy(ifr.ifr_name, intfName, sizeof(ifr.ifr_name));

	gettimeofday(&start, NULL);
	ionic_print_info(fstream, intfName, "Starting flash update using: %s with file: %s\n",
		ionic_fw_update_type ? "adminQ" : "devcmd", path);
	error = ioctl(fd, SIOCETHTOOL, &ifr);
	if (error < 0) {
		ionic_print_error(fstream, intfName, "flash update ioctl failed, error: %s\n",
			strerror(errno));
		close (fd);
		return (error);
	}

	gettimeofday(&end, NULL);

	ionic_print_info(fstream, intfName, "firmware update took %ld seconds\n",
		end.tv_sec - start.tv_sec);
	close (fd);

	return (0);
}
