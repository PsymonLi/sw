/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#ifdef __cplusplus
extern "C" {
#endif
int ionic_read_from_manifest(FILE *fstream, const char buffer[], int buf_len, char *ver_str,
	char *asic_type);
#ifdef __cplusplus
}
#endif
