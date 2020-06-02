/*
 *  {C} Copyright 2020 Pensando Systems Inc. All rights reserved.
 */

#include <stdio.h>

namespace pds_ms {

int pds_ms_init(void) __attribute__ ((weak));
void * pds_ms_thread_init(void *ctxt) __attribute__ ((weak));

int
pds_ms_init (void)
{
    return 0;
}

void *
pds_ms_thread_init (void *ctxt)
{
    return nullptr;
}

}
