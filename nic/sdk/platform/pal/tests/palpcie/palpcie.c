/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "platform/pal/include/pal.h"
#include "platform/pal/include/penpcie_dev.h"

int
main(int argc, char *argv[])
{
    u_int64_t pa;
    u_int32_t v;
    int r;

    if (argc == 2) {
        pa = strtoull(argv[1], NULL, 0);
        r = pal_pciepreg_rd32(pa, &v);
        printf("pal_pciepreg_rd32(0x%" PRIx64 ") = 0x%x (%d)\n", pa, v, r);
    } else if (argc == 3) {
        pa = strtoull(argv[1], NULL, 0);
        v = strtoul(argv[2], NULL, 0);
        printf("pal_pciepreg_wr32(0x%" PRIx64 ", 0x%x)\n", pa, v);
        pal_pciepreg_wr32(pa, v);
    } else {
        printf("Usage: palpcie <pa> [<val>]\n");
        exit(1);
    }

    exit(0);
}
