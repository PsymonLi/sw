/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
  extern "C" {
#endif

/* For GTEST */
int ionic_init(FILE *fstream);
int ionic_dump_list(void);

char *ionic_fw_decision(char *newVer, char *curVer);
char *ionic_fw_decision2(uint32_t new_maj, uint32_t new_min, uint32_t new_rel, uint32_t cur_maj,
	uint32_t cur_min, uint32_t cur_rel);

int ionic_spp2pen_verstr(const char *spp_ver, char *pen_ver, int buf_len);
int ionic_decode_release(const uint32_t rel, char *buffer, int buf_len);
int ionic_encode_fw_version(const char *fw_vers, uint32_t *major, uint32_t *minor, uint32_t *rel);

int ionic_desc_name(struct ionic *ionic, char *desc, int len);

int ionic_add_spp_entry(FILE *fstream, ven_adapter_info *ionic_info, struct ionic *ionic,
	const char *firmware_file_path);

int ionic_find_version_from_tar(FILE *fstream, char *tar_file, const char *find_file);

int ionic_test_discovery(const char *log_file, const char *fw_path, const char *discovery_file);
int ionic_test_update(const char *log_file, const char *fw_path, const char *discovery_file, bool test_multi);
#ifndef DSC_SPP_WIN
void * ionic_test_flash_thread(void *arg);
#endif
#ifdef __cplusplus
}
#endif
