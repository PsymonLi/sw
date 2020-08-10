/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#ifdef SPP_DSC_OLD_NICFWDATA
int ionic_parse_NICFWData(FILE *fstream, char *firmware_file_path);
int ionic_parse_NICFWData_nic(FILE *fstream, xmlNodePtr nic);
#endif
xmlNodePtr ionic_dis_add_new_node(FILE *fstream, const char *intfName, xmlNodePtr parent,
	const char *name, const char *attr, const char *value);
int ionic_dis_add_dev(FILE *fstream, struct ionic *ionic, char *fw_file, xmlNodePtr device);
int ionic_parse_discovery(FILE *fstream, const char *dis_file, const char *fw_Path);
int ionic_parse_discovery_devices(FILE *fstream, xmlNodePtr device);
