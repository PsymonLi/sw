/*
 * Copyright (c) 2018, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>

#include "pal.h"
#include "pal_impl.h"

void *
pal_mem_map(const u_int64_t pa, const size_t sz)
{
    return pr_ptov(pa, sz);
}

void
pal_mem_unmap(void *va)
{
    /* XXX */
}

u_int64_t
pal_mem_vtop(const void *va)
{
    return pr_vtop(va, 4);
}

void *
pal_memcpy(void *dst, const void *src, size_t n)
{
    volatile u_int8_t *d = dst;
    const u_int8_t *s = src;
    int i;

    for (i = 0; i < n; i++) {
        *d++ = *s++;
    }
    return dst;
}

int
pal_mem_rd(const u_int64_t pa, void *buf, const size_t sz)
{
    pal_memcpy(buf, pr_ptov(pa, sz), sz);
    pal_reg_trace("mem_rd 0x%08"PRIx64" sz 0x%ld\n", pa, sz);
    return sz;
}

int
pal_mem_wr(const u_int64_t pa, const void *buf, const size_t sz)
{
    pal_reg_trace("mem_wr 0x%08"PRIx64" sz 0x%ld\n", pa, sz);
    pal_memcpy(pr_ptov(pa, sz), buf, sz);
    return sz;
}
