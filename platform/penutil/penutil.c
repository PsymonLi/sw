/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#ifndef _WIN32
#include <getopt.h>
#include <pthread.h>
#else
#include "getopt_win.h"
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

	fprintf(stdout, "Usage : %s\n"
		"\t-p|--image <Firmware image directory>\n"
		"\t-d|--discovery <Discovery file>\n"
		"\t-l|--log <log file>\n"
		"\t[-u|--update <0/1>] Flash firmware using discovery file, 0 - single/1 - multi threaded\n"
		"\t[-m|--mode <0/1>] 0 - use devcmd/ 1 - adminQ\n"
		"\t[-x|--devid] Naples Device ID(default is 0x1004 for management)\n"
		"\t[-h|--help] This help\n",
		argv[0]);
}

int
main (int argc,  char **argv)
{
	char *log_file = NULL, *path_of_fw = NULL, *discovery_file = NULL;
	bool update = false, test_multi = false;
	int error;

	struct option longopts[] = {
	//	{ "create NICFWData.xml", required_argument,	NULL, 'c'},
		{ "discovery_file", 	required_argument, 	NULL, 'd'},
		{ "help",  		no_argument,  		NULL, 'h'},
		{ "log",  		required_argument, 	NULL, 'l'},
		{ "mode", 		optional_argument, 	NULL, 'm'},
		{ "path of fw", 	required_argument, 	NULL, 'p'},
		{ "update", 		optional_argument,	NULL, 'u'},
		{ "devid", 		optional_argument, 	NULL, 'x'},
		{ 0,       		0,                 	0,     0}
	};

	while ((error = getopt_long(argc, argv, "c:d:hl:m:p:u:v:x:", longopts, NULL)) != -1) {
		switch (error) {
		case 'd':
			if (optarg) {
				discovery_file = optarg;
			}
			break;

#ifdef SPP_DSC_OLD_NICFWDATA
		case 'c': /* Option to create NICFWData.xml based on version string. */
			if (optarg) {
				pen_ver = optarg;
				create = true;
			}
			break;
#endif
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
			ionic_devid = (uint16_t)strtoul(optarg, NULL, 0);
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

	if (log_file == NULL || discovery_file == NULL || path_of_fw == NULL) {
		print_usage(argv);
		exit(1);
	}

#ifdef __ESXI__
        error = ionic_platform_init();
        if (error) {
                fprintf(stderr, "Failed to initialize platform for OS: %s "
                        " (rcUser=0x%x)\n", SPP_BUILD_OS, error);
                exit(1);
        }
#endif

	if (update) {
		if (test_multi)
			error = ionic_test_multi_update(log_file, path_of_fw, discovery_file);
		else {
			fprintf(stderr, "Running firmware update\n");
			error = ionic_test_update(log_file, path_of_fw, discovery_file);
		}
	} else {
		fprintf(stderr, "Creating discovery file\n");
		error = ionic_test_discovery(log_file, path_of_fw, discovery_file);
	}
#ifdef __ESXI__
        ionic_platform_cleanup();
#endif

	return (error);
}
