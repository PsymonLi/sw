/*
 * Copyright (c) 2018-2019, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>

#include "platform/misc/include/misc.h"
#include "platform/evutils/include/evutils.h"
#include "platform/cfgspace/include/cfgspace.h"
#include "platform/pciehdevices/include/pciehdevices.h"
#include "platform/pciemgr/include/pciemgr.h"
#include "platform/pciemgrutils/include/pciemgrutils.h"
#include "platform/pciemgrutils/include/pciehdev_impl.h"
#include "platform/pciemgrutils/include/pciehcfg_impl.h"
#include "platform/pciemgrutils/include/pciehbar_impl.h"

#include "pciesvc.h"
#include "pciemgr_if.hpp"

static pciemgr::evhandler default_evhandler;

pciemgr::pciemgr(const char *name, EV_P) :
    pciemgr(name, default_evhandler, loop) {}

pciemgr::pciemgr(const char *name, evhandler &evhandlercb, EV_P) :
        evhandlercb (evhandlercb)
{
    serverfd = pciemgrc_open(name, NULL);
    if (serverfd < 0) {
        // XXX retry connecting to server?
        fprintf(stderr, "No pciemgr server found - exiting\n");
        exit(1);
    }
    this->loop = loop;
    evutil_add_fd(EV_A_ serverfd, msgrecv, NULL, this);
}

pciemgr::~pciemgr(void)
{
    evutil_remove_fd(EV_A_ serverfd);
    pciemgrc_close();
}

int
pciemgr::initialize(const int port)
{
    pmmsg_t m = {
        .hdr = {
            .msgtype = PMMSG_INITIALIZE,
            .msglen = sizeof(pmmsg_initialize_t),
        },
    };

    m.initialize.port = port;
    return pciemgrc_msgsend(&m);
}

int
pciemgr::finalize(const int port)
{
    pmmsg_t m = {
        .hdr = {
            .msgtype = PMMSG_FINALIZE,
            .msglen = sizeof(pmmsg_finalize_t),
        },
    };

    m.finalize.port = port;
    return pciemgrc_msgsend(&m);
}

int
pciemgr::add_devres(pciehdevice_resources_t *pres)
{
    pmmsg_t *m;
    char *mp;

    const size_t msglen = (sizeof(pmmsg_devres_add_t) +
                           sizeof(pciehdevice_resources_t));
    pciemgrc_msgalloc(&m, msglen);
    // msg header
    m->hdr.msgtype = PMMSG_DEVRES_ADD;
    mp = (char *)&m->devres_add + sizeof(pmmsg_devres_add_t);

    //
    // Now copy our data to the msg.
    //
    memcpy(mp, pres, sizeof(*pres));

    // msg complete - send it
    pciemgrc_msgsend(m);
    pciemgrc_msgfree(m);
    return 0;
}

void
pciemgr::msgrecv(void *arg)
{
    pciemgr *pciemgr = (class pciemgr *)arg;
    pmmsg_t *m;

    pciemgrc_msgrecv(&m);
    pciemgr->msghandler(m);
    pciemgrc_msgfree(m);
}

void
pciemgr::msghandler(pmmsg_t *m)
{
    switch (m->hdr.msgtype) {
    case PMMSG_EVENT: {
        char *msgdata = (char *)&m->event + sizeof(pmmsg_event_t);
        pciehdev_eventdata_t *evd = (pciehdev_eventdata_t *)msgdata;
        handle_event(evd);
        break;
    }
    default:
        // we can ignore this event
        break;
    }
}

void
pciemgr::handle_event(const pciehdev_eventdata_t *evd)
{
    switch (evd->evtype) {
    case PCIEHDEV_EV_MEMRD_NOTIFY:
        evhandlercb.memrd(evd->port, evd->lif, &evd->memrw_notify);
        break;
    case PCIEHDEV_EV_MEMWR_NOTIFY:
        evhandlercb.memwr(evd->port, evd->lif, &evd->memrw_notify);
        break;
    case PCIEHDEV_EV_SRIOV_NUMVFS:
        evhandlercb.sriov_numvfs(evd->port, evd->lif,
                                 evd->sriov_numvfs.numvfs);
        break;
    default:
        break;
    }
}
