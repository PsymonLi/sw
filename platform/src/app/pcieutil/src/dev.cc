/*
 * Copyright (c) 2018, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>

#include "pciehw.h"
#include "pciehw_dev.h"
#include "cmd.h"

static void
dev(int argc, char *argv[])
{
    int opt;

    optind = 0;
    while ((opt = getopt(argc, argv, "")) != -1) {
        switch (opt) {
        default:
            return;
        }
    }

    if (pciehdev_open(NULL) < 0) {
        printf("pciehdev_open failed\n");
        exit(1);
    }

    pciehw_dev_show();

    pciehdev_close();
}
CMDFUNC(dev, "dev");
