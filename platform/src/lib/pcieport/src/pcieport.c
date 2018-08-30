/*
 * Copyright (c) 2017-2018, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <inttypes.h>

#include "pal.h"
#include "pciehsys.h"
#include "pcieport.h"
#include "pcieport_impl.h"

pcieport_info_t pcieport_info;

static void
pcieport_rx_credit_init(const int nports)
{
    int base, ncredits, i;

    if (pal_is_asic()) {
        assert(nports == 1); /* XXX tailored for 1 active port */
        /* port 0 gets all credits for now */
        pcieport_rx_credit_bfr(0, 0, 1023);
    } else {
        assert(nports == 4); /* XXX tailored for 4 active ports */
        ncredits = 1024 / nports;
        for (base = 0, i = 0; i < 8; i += 2, base += ncredits) {
            const int limit = base + ncredits - 1;

            pcieport_rx_credit_bfr(i, base, limit);
            pcieport_rx_credit_bfr(i + 1, 0, 0);
        }
    }
}

static void
pcieport_macfifo_thres(const int thres)
{
    union {
        struct {
            u_int32_t macfifo_thres:5;
            u_int32_t rd_sgl_pnd:1;
            u_int32_t tag_avl_guardband:3;
            u_int32_t cnxt_avl_guardband:3;
        } __attribute__((packed));
        u_int32_t w;
    } v;
    const u_int64_t itr_tx_req =
        (CAP_ADDR_BASE_PXB_PXB_OFFSET + 
         CAP_PXB_CSR_CFG_ITR_TX_REQ_BYTE_ADDRESS);

    v.w = pal_reg_rd32(itr_tx_req);
    v.macfifo_thres = thres;
    pal_reg_wr32(itr_tx_req, v.w);
}

static void
pcieport_link_init_asic(void)
{
    pal_reg_wr32(PP_(CFG_PP_LINKWIDTH), 0x0); /* 1 port x16 linkwidth mode */
    pcieport_rx_credit_init(1);
    pcieport_macfifo_thres(5); /* match late-stage ECO */

    pcieport_serdes_init();
}

static void
pcieport_link_init_haps(void)
{
    pal_reg_wr32(PP_(CFG_PP_LINKWIDTH), 0x2222); /* 4 port x4 linkwidth mode */
    pcieport_rx_credit_init(4);
    pcieport_macfifo_thres(5); /* match late-stage ECO */
}

static void
pcieport_link_init(void)
{
    if (pal_is_asic()) {
        pcieport_link_init_asic();
    } else {
        pcieport_link_init_haps();
    }
}

static int
pcieport_onetime_init(void)
{
    pcieport_info_t *pi = &pcieport_info;

    if (pi->init) {
        /* already initialized */
        return 0;
    }
    pcieport_link_init();
    pi->init = 1;
    return 0;
}

pcieport_t *
pcieport_open(const int port)
{
    pcieport_info_t *pi = &pcieport_info;
    pcieport_t *p;
    int otrace;

    otrace = pal_reg_trace_control(getenv("PCIEPORT_INIT_TRACE") != NULL);
    pal_reg_trace("================ pcieport_open %d start\n", port);

    assert(port < PCIEPORT_NPORTS);
    if (pcieport_onetime_init() < 0) {
        return NULL;
    }
    p = &pi->pcieport[port];
    if (p->open) {
        return NULL;
    }
    p->port = port;
    p->open = 1;
    p->host = 0;
    p->config = 0;
    pal_reg_trace("================ pcieport_open %d end\n", port);
    pal_reg_trace_control(otrace);
    return p;
}

void
pcieport_close(pcieport_t *p)
{
    if (p->open) {
        p->open = 0;
    }
}

static void
cmd_fsm(int argc, char *argv[])
{
    pcieport_fsm_dbg(argc, argv);
}

int vga_support;

static void
cmd_vga_support(int argc, char *argv[])
{
    if (argc < 2) {
        pciehsys_log("vga_support %d\n", vga_support);
        return;
    }
    vga_support = strtoul(argv[1], NULL, 0);
}

static void
cmd_port(int argc, char *argv[])
{
    pcieport_info_t *pi = &pcieport_info;
    pcieport_t *p;
    const int w = 20;
    int port;

    if (argc <= 1) {
        pciehsys_log("Usage: port <n>\n");
        return;
    }
    port = strtoul(argv[1], NULL, 0);
    if (port < 0 || port > PCIEPORT_NPORTS) {
        pciehsys_log("port %d out of range\n", port);
        return;
    }

    p = &pi->pcieport[port];
    pciehsys_log("%-*s: cap gen%dx%d\n", w, "config", p->cap_gen,p->cap_width);
    pciehsys_log("%-*s: cur gen%dx%d\n", w, "current",p->cur_gen,p->cur_width);
    pciehsys_log("%-*s: 0x%04x\n", w, "lanemask", p->lanemask);
    pciehsys_log("%-*s: 0x%04x\n", w, "subvendorid", p->subvendorid);
    pciehsys_log("%-*s: 0x%04x\n", w, "subdeviceid", p->subdeviceid);
    pciehsys_log("%-*s: %d\n", w, "open", p->open);
    pciehsys_log("%-*s: %d\n", w, "config", p->open);
    pciehsys_log("%-*s: %d\n", w, "crs", p->crs);
    pciehsys_log("%-*s: %d\n", w, "state", p->state);
    pciehsys_log("%-*s: %d\n", w, "event", p->event);
    pciehsys_log("%-*s: %s\n", w, "fault_reason", p->fault_reason);
    pciehsys_log("%-*s: %s\n", w, "last_fault_reason", p->last_fault_reason);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "hostup", p->hostup);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "phypolllast", p->phypolllast);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "phypollmax", p->phypollmax);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "phypollperstn", p->phypollperstn);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "phypollfail", p->phypollfail);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "gatepolllast", p->gatepolllast);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "gatepollmax", p->gatepollmax);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "markerpolllast", p->markerpolllast);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "markerpollmax", p->markerpollmax);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "axipendpolllast",p->axipendpolllast);
    pciehsys_log("%-*s: %"PRIu64"\n", w, "axipendpollmax", p->axipendpollmax);
}

typedef struct cmd_s {
    const char *name;
    void (*f)(int argc, char *argv[]);
    const char *desc;
    const char *helpstr;
} cmd_t;

static cmd_t cmdtab[] = {
#define CMDENT(name, desc, helpstr) \
    { #name, cmd_##name, desc, helpstr }
    CMDENT(fsm, "fsm", ""),
    CMDENT(port, "port", ""),
    CMDENT(vga_support, "vga_support", ""),
    { NULL, NULL }
};

static cmd_t *
cmd_lookup(cmd_t *cmdtab, const char *name)
{
    cmd_t *c;

    for (c = cmdtab; c->name; c++) {
        if (strcmp(c->name, name) == 0) {
            return c;
        }
    }
    return NULL;
}

void
pcieport_dbg(int argc, char *argv[])
{
    cmd_t *c;

    if (argc < 2) {
        pciehsys_log("Usage: pcieport <subcmd>\n");
        return;
    }

    c = cmd_lookup(cmdtab, argv[1]);
    if (c == NULL) {
        pciehsys_log("%s: %s not found\n", argv[0], argv[1]);
        return;
    }
    c->f(argc - 1, argv + 1);
}
