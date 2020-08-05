/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <errno.h>
#include "ionic_os.h"

#ifdef PCIUTILS /* For accessing device directly */
#include <pci/pci.h>

#include "ionic_pmd.h"

#define IONIC_BAR0_SIZE		0x8000

/*
 * User space access to ionic.
 */

/*
 * Scan the PCI using pciutil package and find Naples
 * management interfaces.
 */
int
ionic_pci_scan(FILE *fstream, struct ionic ionic_devs[], int *count)
{
	struct pci_access *pacc;
	struct pci_dev *dev;
	struct ionic *ionic;
	uint64_t bar0;
	uint16_t devid;
	char namebuf[1024], *name;

	pacc = pci_alloc();	/* Get the pci_access structure */
	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);		/* Initialize the PCI library */
	pci_scan_bus(pacc);	/* We want to get the list of devices */

	/* Look for mgmt ionic interface. */
	devid = ionic_devid ? ionic_devid : PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT;

	*count = 0;
	/* Iterate over all devices */
	for (dev = pacc->devices; dev; dev = dev->next) {
		/* Fill in header info we need */
		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);
		if ((dev->vendor_id != PCI_VENDOR_ID_PENSANDO) ||
			(dev->device_id != devid))
			continue;

		bar0 = dev->base_addr[0] & PCI_BASE_ADDRESS_MEM_MASK;
		ionic = &ionic_devs[(*count)++];
		ionic->venId = dev->vendor_id;
		ionic->devId = dev->device_id;
		ionic->domain = dev->domain;
		ionic->bus = dev->bus;
		ionic->dev = dev->dev;
		ionic->func = dev->func;
		ionic->bar0_pa = bar0;
		ionic->subVenId = pci_read_word(dev, PCI_SUBSYSTEM_VENDOR_ID);
		ionic->subDevId = pci_read_word(dev, PCI_SUBSYSTEM_ID);
		ionic_print_info(fstream, ionic->intfName,
			"%04x:%02x:%02x.%d vendor=%04x device=%04x class=%04x irq=%d base0=%lx\n",
			dev->domain, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id,
			dev->device_class, dev->irq, bar0);
		/* Look up and print the full name of the device */
		name = pci_lookup_name(pacc, namebuf, sizeof(namebuf), PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
		ionic_print_info(fstream, ionic->intfName, "%d (%s)\n", *count, name);

	}

	pci_cleanup(pacc);	/* Close everything */

	return 0;
}

/*
 * Find device by PCI BDF
*/
int
ionic_find_pci_by_dbdf(FILE *fstream, struct ionic *ionic)
{
	struct pci_access *pacc;
	struct pci_dev *dev;
	uint64_t bar0;
	char namebuf[1024], *name;
	int error = EINVAL;

	pacc = pci_alloc();	/* Get the pci_access structure */
	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);		/* Initialize the PCI library */
	pci_scan_bus(pacc);	/* We want to get the list of devices */

	/* Iterate over all devices */
	for (dev = pacc->devices; dev; dev = dev->next) {
		/* Fill in header info we need */
		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);

		if ((dev->domain != ionic->domain) || (dev->bus != ionic->bus) ||
			(dev->dev != ionic->dev) || (dev->func != ionic->func))
			continue;

		error = 0;
		bar0 = dev->base_addr[0] & PCI_BASE_ADDRESS_MEM_MASK;
		ionic->venId = dev->vendor_id;
		ionic->devId = dev->device_id;
		ionic->bar0_pa = bar0;
		ionic->subVenId = pci_read_word(dev, PCI_SUBSYSTEM_VENDOR_ID);
		ionic->subDevId = pci_read_word(dev, PCI_SUBSYSTEM_ID);
		ionic_print_info(fstream, ionic->intfName,
			"%04x:%02x:%02x.%d vendor=%04x device=%04x class=%04x irq=%d base0=%lx\n",
			dev->domain, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id,
			dev->device_class, dev->irq, bar0);
		/* Look up and print the full name of the device */
		name = pci_lookup_name(pacc, namebuf, sizeof(namebuf), PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
		ionic_print_info(fstream, ionic->intfName, "(%s)\n", name);

	}

	pci_cleanup(pacc);	/* Close everything */

	return (error);
}

int
ionic_find_devices(FILE *fstream, struct ionic ionic_devs[], int *count)
{

	return (ionic_pci_scan(fstream, ionic_devs, count));
}
#else /* ! PCIUTILS */
int
ionic_find_devices(FILE *fstream, uint16_t devid, struct ionic (*ionic_devs)[], int *count)
{

	return (0);
}
#endif /* PCIUTILS */

/*
 * Open the ionic device interface.
 */
void *
ionic_mem_open(FILE *fstream, uint64_t bar0_pa)
{
	const char *path = "/dev/mem";
	void *ptr;
	int fd;

	fd = open(path, O_RDWR | O_SYNC);
	if (fd == -1) {
		ionic_print_error(fstream, "", "Failed to open: %s, error: %s\n",
			path, strerror(errno));
		return (NULL);
	}


	ptr = mmap(NULL, IONIC_BAR0_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, bar0_pa);
	if (ptr == MAP_FAILED) {
		ionic_print_error(fstream, "", "Failed to mmap: %s, for addr: 0x%lx error: %s\n",
			path, bar0_pa, strerror(errno));
		close(fd);
		return (NULL);
	}

	close(fd);
	return (ptr);
}

static void
ionic_mem_close(FILE *fstream, void *bar0_va)
{

	if (bar0_va == NULL) {
		ionic_print_info(fstream, "", "device alreaday closed.\n");
	} else {
		munmap(bar0_va, IONIC_BAR0_SIZE);
	}

}

int
ionic_get_details(FILE *fstream, struct ionic *ionic)
{
#ifndef PCIUTILS
	return (0);
#else
	void *ptr;
	int error;

	ptr = ionic_mem_open(fstream, ionic->bar0_pa);
	if (ptr == NULL) {
		return (EIO);
	}

	snprintf(ionic->intfName, sizeof(ionic->intfName), "ionic-%d:%d:%d.%d",
		ionic->domain, ionic->bus, ionic->dev, ionic->func);
	error = ionic_pmd_identify(fstream, ptr, ionic->curFwVer, ionic->asicType);
	if (error) {
		ionic_print_error(fstream, ionic->intfName, "Couldn't get details, error: %d\n",
			error);
	}

	ionic_mem_close(fstream, ptr);
	return (error);
#endif
}

int
ionic_flash_firmware(FILE *fstream, struct ionic *ionic, char *fw_file)
{
#ifndef PCIUTILS
	return (0);
#else
	void *ptr;
	int error;

	error = ionic_find_pci_by_dbdf(fstream, ionic);

	if (error) {
		ionic_print_error(fstream, ionic->intfName,
			"Couldn't find device(%d:%d:%d.%d)\n",
			ionic->domain, ionic->bus, ionic->dev, ionic->func);
		return (EIO);
	}

	if (!ionic->bar0_pa) {
		ionic_print_error(fstream, ionic->intfName,
			"BAR0 address is not set\n");
		return (EIO);
	}

	ptr = ionic_mem_open(fstream, ionic->bar0_pa);
	if (ptr == NULL) {
		return (EIO);
	}

	ionic_print_info(fstream, ionic->intfName, "Starting pmd flash update\n");
	error = ionic_pmd_flash_firmware(fstream, ptr, fw_file);
	ionic_print_info(fstream, ionic->intfName, "pmd flash update done, status: %d\n",
		error);

	ionic_mem_close(fstream, ptr);

	return (error);
#endif
}
