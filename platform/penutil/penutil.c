/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#ifndef _WIN32
#include <getopt.h>
#include <pthread.h>
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "ionic_spp.h"
#include "ionic_os.h"
#include "ionic_fw_meta.h"
#include "ionic_spp_util.h"

static void
print_usage (char **argv)
{

#ifdef _WIN32
	fprintf(stdout, "Usage : %s <update/discovery> <Firmware image directory> <log file> <Discovery file> <Update Type> <DevId> <Verbose>\n",
		argv[0]);
#else
	fprintf(stdout, "Usage : %s\n"
		"\t-p|--image <Firmware image directory>\n"
		"\t-d|--discovery <Discovery file>\n"
		"\t-l|--log <log file>\n"
		"\t[-u|--update <0/1>] Flash firmware using discovery file, 0 - single/1 - multi threaded\n",
		argv[0]);
#endif
}

int
main (int argc,  char **argv)
{
	char *log_file = NULL, *path_of_fw = NULL, *discovery_file = NULL;
	bool update = false, test_multi = false;
#ifndef _WIN32
	int error;
	struct option longopts[] = {
		{ "discovery_file", 	required_argument, 	NULL, 'd'},
		{ "help",  		no_argument,  		NULL, 'h'},
		{ "log",  		required_argument, 	NULL, 'l'},
		{ "mode", 		optional_argument, 	NULL, 'm'},
		{ "path of fw", 	required_argument, 	NULL, 'p'},
		{ "update", 		optional_argument,	NULL, 'u'},
		{ "devid", 		optional_argument, 	NULL, 'x'},
		{ 0,       		0,                 	0,     0}
	};

	while ((error = getopt_long(argc, argv, "d:hl:m:p:u:v:x:", longopts, NULL)) != -1) {
		switch (error) {
		case 'd':
			if (optarg) {
				discovery_file = optarg;
			}
			break;

		case 'h':
			print_usage(argv);
			exit(0);
			break;

		case 'l':
			if (optarg) {
				log_file = optarg;
			}
			break;

		case 'm':
			if (optarg) {
				ionic_fw_update_type = strtoul(optarg, NULL, 0);
			}
			break;

		case 'p':
			if (optarg) {
				path_of_fw = optarg;
			}
			break;

		case 'u':
			update = true;
			if (optarg) {
				test_multi = strtoul(optarg, NULL, 0);
			}
			break;

		case 'v':
			ionic_verbose_level = strtoul(optarg, NULL, 0);
			break;

		case 'x':
			ionic_devid = strtoul(optarg, NULL, 0);
			break;

		case '?':
		default:
			print_usage(argv);
			exit(1);
			break;
		}
	}

	if (optind < argc) {
		fprintf(stderr, "non-option ARGV-elements: ");
		while (optind < argc)
			fprintf(stderr, "%s ", argv[optind++]);
		fprintf(stderr, "\n");
		print_usage(argv);
		exit(1);
	}

#else /* For Windows */
	if (argc < 5) {
		print_usage(argv);
		exit(1);
	}

	if (strcmp(argv[1], "update") == 0)
		update = true;
	path_of_fw = argv[2];
	log_file = argv[3];
	discovery_file = argv[4];
	if (argv[5]) {
		ionic_fw_update_type = strtoul(argv[5], NULL, 0);
	}
	if (argv[6]) {
		ionic_devid = strtoul(argv[6], NULL, 0);
	}
	if (argv[7]) {
		ionic_verbose_level = strtoul(argv[6], NULL, 0);
	}
	//why test_multi?
	test_multi = true;
#endif

	if (log_file == NULL || discovery_file == NULL || path_of_fw == NULL) {
		print_usage(argv);
		exit(1);
	}

	if (update)
		ionic_test_update(log_file, path_of_fw, discovery_file, test_multi);
	else
		ionic_test_discovery(log_file, path_of_fw, discovery_file);

	return (0);
}
