/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <libtar.h>
#include <string.h>
#ifdef __FreeBSD__
#include <json/json.h>
#else
#include <jsoncpp/json/json.h>
#endif

#include "ionic_spp.h"
#include "ionic_spp_util.h"

using namespace std;

extern "C" int ionic_read_from_manifest(FILE *fstream, const char buffer[], int buf_len, char *ver_str,
	char *asic_type);

int
ionic_read_from_manifest(FILE *fstream, const char buffer[], int buf_len, char *ver_str,
	char *asic_type)
{
	Json::CharReaderBuilder rb;
	Json::CharReader *reader;
	Json::Value obj;
	string version, asic, errstr;
	bool status;

	reader = rb.newCharReader();
	status = reader->parse(buffer, buffer + buf_len, &obj, &errstr);
	if (!status) {
		fprintf(fstream, "Failed to parse MANIFEST, not a valid json[ %s ], error: %s \n",
			buffer, errstr.c_str());
		return (ENXIO);
	}

	version =  obj["software_version"].asString();
	asic =  obj["asic_compat"].asString();

	strcpy(ver_str, version.c_str());
	strcpy(asic_type, asic.c_str());
	fprintf(fstream, "Version: %s ASIC: %s from MANIFEST file\n",
		ver_str, asic_type);

	return (0);
}
