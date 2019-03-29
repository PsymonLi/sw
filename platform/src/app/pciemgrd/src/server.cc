/*
 * Copyright (c) 2017-2018, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "nic/sdk/platform/evutils/include/evutils.h"
#include "nic/sdk/platform/pciemgr/include/pciemgr.h"
#include "nic/sdk/platform/pcieport/include/pcieport.h"
#include "nic/sdk/platform/pciemgrutils/include/pciemgrutils.h"
#include "nic/sdk/platform/pciemgrutils/include/pciehdev_impl.h"
#include "nic/sdk/platform/pciemgrutils/include/pciehcfg_impl.h"
#include "nic/sdk/platform/pciemgrutils/include/pciehbar_impl.h"
#include "platform/src/app/pciemgrd/src/pciemgrd_impl.hpp"
#include "platform/src/lib/pciemgr_if/include/pmserver.h"

#include "delphic.h"

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

    r = pcieport_crs_off(port);
    if (r < 0) {
        pciesys_logerror("pcieport_crs_off(%d) failed %d\n", port, r);
    }

    // log some info about the final config
    pciehw_dev_show(0, NULL);
    pciehw_bar_show(0, NULL);
    pciesys_loginfo("finalize: port %d ready\n", port);
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
    pciehbar_t *pbar = NULL;
    char *mp;
    int b, r, p;

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

    pbar = pbars->bars;
    for (b = 0; b < PCIEHBAR_NBARS; b++, pbar++) {

        if (pbar->size == 0) continue;

        int nregs = pbar->nregs;
        pbar->nregs = 0;
        pbar->regs = NULL;

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
                prt_t prt;
                memcpy(&prt, mp, sizeof(prt_t));
                pciehbarreg_add_prt(&reg, &prt);
                mp += sizeof(prt_t);
            }
            pciehbar_add_reg(pbar, &reg);
        }
        pciehbars_add_bar(pbars, pbar);
    }

    /* rombar */
    pbar = &pbars->rombar;
    if (pbar->size) {
        int nregs = pbar->nregs;
        pbar->nregs = 0;
        pbar->regs = NULL;

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
                prt_t prt;
                memcpy(&prt, mp, sizeof(prt_t));
                pciehbarreg_add_prt(&reg, &prt);
                mp += sizeof(prt_t);
            }
            pciehbar_add_reg(pbar, &reg);
        }
        pciehbars_add_rombar(pbars, pbar);
    }

    pciehdev_set_bars(pdev, pbars);
    pbars = NULL; /* now owned by dev */

    /* log what we got */
    {
        pciehbar_t *pbar = pdev->pbars->bars;
        pciesys_loginfo("dev_add: port %d %s lifs %d-%d\n",
                        pdev->port, pdev->name, pdev->lifb,
                        pdev->lifb + pdev->lifc - 1);
        for (b = 0; b < PCIEHBAR_NBARS; b++, pbar++) {
            if (pbar->size == 0) continue;
            pciesys_logdebug("  bar %d nregs %d\n", b, pbar->nregs);
            pciehbarreg_t *preg = pbar->regs;
            for (r = 0; r < pbar->nregs; r++, preg++) {
                pciesys_logdebug("    reg %d baroff 0x%09" PRIx64 
                                 " nprts %d\n",
                                 r, preg->baroff,
                                 preg->nprts);
            }
        }
        pbar = &pdev->pbars->rombar;
        if (pbar->size) {
            pciesys_logdebug("  rom   nregs %d\n", pbar->nregs);
            pciehbarreg_t *preg = pbar->regs;
            for (r = 0; r < pbar->nregs; r++, preg++) {
                pciesys_logdebug("    reg %d baroff 0x%09" PRIx64 
                                 " nprts %d\n",
                                 r, preg->baroff,
                                 preg->nprts);
            }
        }
    }

    if ((r = pciehdev_add(pdev)) < 0) {
        pciesys_logerror("dev_add: port %d %s lif %d: failed %d\n",
                         pdev->port, pdev->name, pdev->lifb, r);
    }
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
dev_evhandler(const pciehdev_eventdata_t *evd)
{
    pmmsg_t *m;
    const size_t msglen = (sizeof(pmmsg_event_t) + 
                           sizeof(pciehdev_eventdata_t));
    pciemgrs_msgalloc(&m, msglen);

    m->hdr.msgtype = PMMSG_EVENT;

    char *mp = (char *)&m->event + sizeof(pmmsg_event_t);
    memcpy(mp, evd, sizeof(pciehdev_eventdata_t));

    // msg complete - send it
    pciemgrs_msgsend(m);
    pciemgrs_msgfree(m);
}

#ifdef __aarch64__
static void
update_stats(void *arg)
{
    pciemgrenv_t *pme = pciemgrenv_get();

    for (int port = 0; port < PCIEPORT_NPORTS; port++) {
        if (pme->enabled_ports & (1 << port)) {
            delphi_update_pcie_metrics(port);
        }
    }
}
#endif

int
server_loop(void)
{
    pciemgrenv_t *pme = pciemgrenv_get();
    int r = 0;

    logger_init();
    pciesys_loginfo("pciemgrd started\n");

#ifdef __aarch64__
    // connect to delphi
    delphi_client_start();
#endif

    pciemgrd_params(pme);

    pciesys_loginfo("---------------- config ----------------\n");
    pciesys_loginfo("enabled_ports 0x%x\n", pme->enabled_ports);
    pciesys_loginfo("gen%dx%d\n", pme->params.cap_gen, pme->params.cap_width);
    pciesys_loginfo("vendorid: %04x\n", pme->params.vendorid);
    pciesys_loginfo("subvendorid: %04x\n", pme->params.subvendorid);
    pciesys_loginfo("subdeviceid: %04x\n", pme->params.subdeviceid);
    pciesys_loginfo("---------------- config ----------------\n");

    if ((r = open_hostports()) < 0) {
        goto error_out;
    }
    if ((r = pciehdev_open(&pme->params)) < 0) {
        pciesys_logerror("pciehdev_open failed: %d\n", r);
        goto close_port_error_out;
    }
    if ((r = pciehdev_register_event_handler(dev_evhandler)) < 0) {
        pciesys_logerror("pciehdev_register_event_handler failed %d\n", r);
        goto close_dev_error_out;
    }
    intr_init();

#ifdef __aarch64__
    // initialize stats timer to update stats to delphi
    static evutil_timer stats_timer;
    evutil_timer_start(&stats_timer, update_stats, NULL, 10.0, 10.0);
#endif

    pciemgrs_open(NULL, pciemgr_msg_cb);
    pciesys_loginfo("pciemgrd ready\n");
    evutil_run();
    pciemgrs_close();

 close_dev_error_out:
    pciehdev_close();
 close_port_error_out:
    close_hostports();
 error_out:
    pciesys_loginfo("pciemgrd exit %d\n", r);
    return r;
}
