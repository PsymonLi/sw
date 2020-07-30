/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
int ionic_pmd_flash_firmware(FILE *fstream, void *bar0_ptr, char *file_path);
int ionic_pmd_identify(FILE *fstream, void *bar0_ptr, char *fw_vers, char *asic_type);
