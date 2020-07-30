/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "ionic_os.h"


int
ionic_find_devices(FILE *fstream, struct ionic ionic_devs[], int *count)
{

	ionic_print_info(fstream, "all", "Discovering Naples mgmt devices on Windows\n");
	return (0);
}

int
ionic_get_details(FILE *fstream, struct ionic *ionic)
{
	char *intfName;

	intfName = ionic->intfName;
	ionic_print_info(fstream, intfName, "Getting various details\n");

	return (0);
}

int
ionic_flash_firmware( FILE *fstream, struct ionic *ionic, char *fw_file_name)
{
	char *intfName;

	intfName = ionic->intfName;
	ionic_print_info(fstream, intfName, "Enter firmware update using: %s\n",
		ionic_fw_update_type ? "adminQ" : "devcmd");

	return (0);
}
