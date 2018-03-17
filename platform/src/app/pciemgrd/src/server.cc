/*
 * Copyright (c) 2017-2018, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "evutils.h"
#include "pciemgr.h"
#include "pciemgrutils.h"
#include "pcieport.h"
#include "pciehdev_impl.h"
#include "pciehcfg_impl.h"
#include "pciehbar_impl.h"
#include "pmserver.h"
#include "pciemgrd_impl.hpp"

static void
do_open(pmmsg_t *m)
{
    pciesys_loginfo("open: %s\n", m->open.name);
}

static void
do_initialize(pmmsg_t *m)
{
    const int port = m->initialize.port;
    int r;

    pciesys_loginfo("initialize: port %d\n", port);
    r = pciehdev_initialize(port);
    if (r < 0) {
        pciesys_logerror("pciehdev_initialize(%d) failed %d\n", port, r);
    }
}

static void
do_finalize(pmmsg_t *m)
{
    const int port = m->finalize.port;
    int r;

    pciesys_loginfo("finalize: port %d\n", port);
    r = pciehdev_finalize(port);
    if (r < 0) {
        pciesys_logerror("pciehdev_finalize(%d) failed %d\n", port, r);
    }

    // log some info about the final config
    pciehw_dev_show();
    pciehw_bar_show();
}

static size_t
msgdata_left(pmmsg_t *m, char *mp)
{
    return m->hdr.msglen - (mp - &m->msgdata);
}

static void
do_dev_add(pmmsg_t *m)
{
    pciehdev_t *pdev = pciehdev_new("", NULL);
    pciehcfg_t *pcfg = pciehcfg_new();
    pciehbars_t *pbars = pciehbars_new();
    char *mp;
    int b, r, p, nbars;

    if (m->hdr.msglen < (sizeof(pmmsg_dev_add_t) +
                         sizeof(pciehdev_t) +
                         sizeof(pciehcfg_t) +
                         PCIEHCFGSZ * 2 +
                         sizeof(pciehbars_t))) {
        pciesys_logerror("malformed dev_add msg, len %d\n", m->hdr.msglen);
        goto out;
    }

    mp = (char *)&m->dev_add + sizeof(pmmsg_dev_add_t);
    /* pciehdev_t */
    memcpy(pdev, mp, sizeof(pciehdev_t));
    mp += sizeof(pciehdev_t);
    pdev->pbars = NULL;
    pdev->pcfg = NULL;
    /* pciehcfg_t */
    memcpy(pcfg, mp, sizeof(pciehcfg_t));
    mp += sizeof(pciehcfg_t);

    pcfg->cur = (u_int8_t *)(pcfg + 1);
    pcfg->msk = pcfg->cur + PCIEHCFGSZ;

    memcpy(pcfg->cur, mp, PCIEHCFGSZ);
    mp += PCIEHCFGSZ;
    memcpy(pcfg->msk, mp, PCIEHCFGSZ);
    mp += PCIEHCFGSZ;

    pciehdev_set_cfg(pdev, pcfg);
    pcfg = NULL; /* now owned by dev */

    /* bars */
    memcpy(pbars, mp, sizeof(pciehbars_t));
    mp += sizeof(pciehbars_t);

    nbars = pbars->nbars;
    pbars->nbars = 0;
    pbars->bars = NULL;
    pbars->rombar = NULL;

    for (b = 0; b < nbars; b++) {
        pciehbar_t bar;
        int nregs;

        if (msgdata_left(m, mp) < sizeof(pciehbar_t)) {
            pciesys_logerror("malformed dev_add msg, %s bar %d len %ld\n",
                             pciehdev_get_name(pdev),
                             b, msgdata_left(m, mp));
            goto out;
        }

        /* bar */
        memcpy(&bar, mp, sizeof(pciehbar_t));
        mp += sizeof(pciehbar_t);

        nregs = bar.nregs;
        bar.nregs = 0;
        bar.regs = NULL;

        for (r = 0; r < nregs; r++) {
            pciehbarreg_t reg;
            int nprts;

            /* reg */
            if (msgdata_left(m, mp) < sizeof(pciehbarreg_t)) {
                pciesys_logerror("malformed dev_add msg, "
                                 "%s bar %d reg %d len %ld\n",
                                 pciehdev_get_name(pdev),
                                 b, r, msgdata_left(m, mp));
                goto out;
            }
            memcpy(&reg, mp, sizeof(pciehbarreg_t));
            mp += sizeof(pciehbarreg_t);

            /* prts */
            nprts = reg.nprts;
            reg.nprts = 0;
            reg.prts = NULL;

            if (msgdata_left(m, mp) < sizeof(prt_t) * nprts) {
                pciesys_logerror("malformed dev_add msg, "
                                 "%s bar %d reg %d nprts %d len %ld\n",
                                 pciehdev_get_name(pdev),
                                 b, r, nprts, msgdata_left(m, mp));
                goto out;
            }
            for (p = 0; p < nprts; p++) {
                pciehbarreg_add_prt(&reg, (prt_t *)mp);
                mp += sizeof(prt_t);
            }
            pciehbar_add_reg(&bar, &reg);
        }
        pciehbars_add_bar(pbars, &bar);
    }

    pciehdev_set_bars(pdev, pbars);
    pbars = NULL; /* now owned by dev */

    {
        pciehbar_t *pbar = pdev->pbars->bars;
        pciesys_loginfo("dev_add: port %d %s lif %d\n",
                        pdev->port, pdev->name, pdev->lif);
        for (b = 0; b < pdev->pbars->nbars; b++, pbar++) {
            pciesys_logdebug("  bar %d nregs %d\n", b, pbar->nregs);
            pciehbarreg_t *preg = pbar->regs;
            for (r = 0; r < pbar->nregs; r++, preg++) {
                pciesys_logdebug("    reg %d baroff 0x%09" PRIx64 
                                 " nprts %d\n",
                                 r, preg->baroff,
                                 preg->nprts);
            }
        }
    }

    pciehdev_add(pdev);
    return;

 out:
    if (pbars) pciehbars_delete(pbars);
    if (pcfg) pciehcfg_delete(pcfg);
    if (pdev) pciehdev_delete(pdev);
}

static void
pciemgr_msg_cb(pmmsg_t *m)
{
    switch (m->hdr.msgtype) {
    case PMMSG_OPEN:
        do_open(m);
        break;
    case PMMSG_INITIALIZE:
        do_initialize(m);
        break;
    case PMMSG_FINALIZE:
        do_finalize(m);
        break;
    case PMMSG_DEV_ADD:
        do_dev_add(m);
        break;
    default:
        break;
    }

    pciemgr_msgfree(m);
}

static void
server_poll(void *arg)
{
    pciehdev_poll();
}

static void
server_evhandler(const pciehdev_eventdata_t *evd)
{
    pmmsg_t *m;
    const size_t msglen = (sizeof(pmmsg_event_t) + 
                           sizeof(pciehdev_eventdata_t));
    pciemgrs_msgalloc(&m, msglen);

    m->hdr.msgtype = PMMSG_EVENT;

    char *mp = (char *)&m->event + sizeof(pmmsg_event_t);

    //
    // If this event comes with a pdev, extract the client's pdev
    // that was stashed in pdev->priv for just this purpose.
    // This will map to the client's pdev in the domain on the other
    // end of the socket.
    //
    if (evd->pdev) {
        pciehdev_eventdata_t levd;
        levd = *evd;
        pciehdev_t *client_pdev = (pciehdev_t *)pciehdev_get_priv(evd->pdev);
        levd.pdev = client_pdev;
        memcpy(mp, &levd, sizeof(pciehdev_eventdata_t));
    } else {
        memcpy(mp, evd, sizeof(pciehdev_eventdata_t));
    }

    // msg complete - send it
    pciemgrs_msgsend(m);
    pciemgrs_msgfree(m);
}

void
server_loop(void)
{
    evutil_timer timer;

    logger_init();

    if (pciehdev_register_event_handler(server_evhandler) < 0) {
        pciesys_logerror("pciehdev_register_event_handler failed\n");
        return;
    }

    pciemgrs_open(NULL, pciemgr_msg_cb);
    evutil_timer_start(&timer, server_poll, NULL, 0.1, 0.1);
    pciesys_loginfo("pciemgrd started\n");
    evutil_run();
    /* NOTREACHED */
    pciemgrs_close();
    pciesys_loginfo("pciemgrd exit\n");
}
