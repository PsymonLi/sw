/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <errno.h>

#include "platform/pal/include/pal.h"
#include "platform/pciemgrutils/include/pciesys.h"
#include "platform/pcieport/include/pcieport.h"
#include "platform/pcieport/src/pcieport_impl.h"
#include "pcieportpd.h"

static void
pcieport_mac_k_gen(pcieport_t *p)
{
    const int pn = p->port;
    u_int64_t gen;

    gen = pal_reg_rd64_safe(PXC_(CFG_C_MAC_K_GEN, pn));

    /* set defaults */
    gen &= 0xffffffff00000000ULL;
    gen |= 0x80e20050;

    if (p->sris) {
        gen |= (1 << 8); /* sris_mode */
    }

    /* select gen speed, and spec_version */
    switch (p->cap_gen) {
    case 1: gen |= (0x1 << 9) | 3; break;
    case 2: gen |= (0x3 << 9) | 3; break;
    case 3: gen |= (0x7 << 9) | 3; break;
    case 4: gen |= (0xf << 9) | 4; break;
    }

    switch (p->cap_width) {
    case 1: gen |= (0xf << 24); break;
    case 2: gen |= (0xe << 24); break;
    case 4: gen |= (0xc << 24); break;
    case 8: gen |= (0x8 << 24); break;
    case 16: /* 16 is default */ break;
    }

    pal_reg_wr64_safe(PXC_(CFG_C_MAC_K_GEN, pn), gen);
}

/*
 * k_pexconf is documented in
 *     PLDA XpressRICH4 Reference Manual
 *     Version 1.9.3 January 2018 - Pre-release draft
 */
static void
pcieport_mac_k_pexconf(pcieport_t *p)
{
    const int pn = p->port;
    u_int32_t pexconf[12];

    pal_reg_rd32w(PXC_(CFG_C_MAC_K_PEXCONF, pn), pexconf, 12);

    /*****************
     * word1
     */
    /* disable Completion Timeout Disable sup - endpoints only */
    pexconf[1] &= ~(1 << 4);  /* [36] Completion Timeout Disable sup */
    if (p->cap_gen < 4) {
        pexconf[1] &= ~(1 << 16); /* [48] 10-bit Completer Tag sup */
        pexconf[1] &= ~(1 << 17); /* [49] 10-bit Requester Tag sup */
    }

    /*****************
     * word2
     */
    /* [64:73] disable gen4-only capabilities if not gen4 */
    if (p->cap_gen < 4) {
        pexconf[2] &= ~0x1ff;
    }
    /* [74:75] claim ASPM L0s, L1 not supported (respectively) */
    pexconf[2] &= ~(0x3 << 10);

    /*****************
     * word5
     */
    /*
     * For compliance testing claim we don't have AER
     * so no surprises show up in the AER registers.
     */
    if (p->compliance) {
        pexconf[5] &= ~(1 << 31); /* [191] AER implemented */
    }

    /*
     * ELBA-TODO HV: pexconf[5][13]=PCIEPORT_HOST_CLOCK
     * This reflects as a slot clock setting in lspci on our device
     * 0=independent clock, 1=host refclk
     */

    pal_reg_wr32w(PXC_(CFG_C_MAC_K_PEXCONF, pn), pexconf, 12);
}

static void
pcieport_mac_k_pciconf(pcieport_t *p)
{
    const int pn = p->port;
    u_int32_t val;

    /* class code */
    val = 0x06040000;
    pal_reg_wr32(PXC_(CFG_C_MAC_K_PCICONF, pn) + 4, val);

    val = pal_reg_rd32(PXC_(CFG_C_MAC_K_PCICONF, pn) + 0xc);
    /* vga supported */
    val &= ~(1 << 4);
    val |= (p->vga_support << 4);
    /* legacy power management disabled */
    val &= ~(0x7ff << 21);
    pal_reg_wr32(PXC_(CFG_C_MAC_K_PCICONF, pn) + 0xc, val);
}

static void
pcieport_mac_k_rx_cred(pcieport_t *p)
{
    if (!pal_is_asic()) {
        u_int32_t val = 0x00200080;
        pal_reg_wr32(PXC_(CFG_C_MAC_K_RX_CRED, p->port), val);
    }
    if (p->reduce_rx_cred) {
        u_int32_t reg[3], r;
        const u_int64_t axi_rsp_order = PXB_(CFG_ITR_AXI_RESP_ORDER);

        pal_reg_rd32w(PXC_(CFG_C_MAC_K_RX_CRED, p->port), reg, 3);
        reg[0] = 0x00200080;
        reg[1] = 0x00100010;
        pal_reg_wr32w(PXC_(CFG_C_MAC_K_RX_CRED, p->port), reg, 3);

        r = pal_reg_rd32(axi_rsp_order);
        r &= 0x3;       /* preserve rd/wr_strict */
        r |= 0x20 << 2; /* reduce rd_id_limit */
        pal_reg_wr32(axi_rsp_order, r);
    }
}

/*
 * Set these values to perform better on pcie compliance tests.
 * k_lmr is documented in
 *     PLDA XpressRICH4 Reference Manual
 *     Version 1.9.3 January 2018 - Pre-release draft
 */
static void
pcieport_mac_k_lmr(pcieport_t *p)
{
    const int pn = p->port;
    u_int32_t val[3];

    pal_reg_rd32w(PXC_(CFG_C_MAC_K_LMR, pn), val, 3);
    val[0] = 0x140620df;
    val[1] = 0x800000;
    val[2] = 0xf;
    pal_reg_wr32w(PXC_(CFG_C_MAC_K_LMR, pn), val, 3);
}

static void
pcieport_set_sw_reset(pcieport_t *p, const int on)
{
    u_int32_t val = pal_reg_rd32(PP_(CFG_PP_SW_RESET, p->port));
    if (on) {
        val |= 0x3 << ((p->port % 4) << 1);
    } else {
        val &= ~(0x3 << ((p->port % 4) << 1));
    }
    pal_reg_wr32(PP_(CFG_PP_SW_RESET, p->port), val);
}

/*
 * Note this function sets reset on/off, but the register
 * is inverted logic for PCIE_PLL_RST_N.  If on=1 then put
 * the serdes in reset by clearing reset_n bits;
 */
static void
pcieport_set_pcie_pll_rst(pcieport_t *p, const int on, const int host_clk)
{
    /*
     * host_clk mode uses pll1
     * local_clk mode uses pll0
     */
    const int pll = host_clk;
    u_int32_t pll_cfg[3];

#define CFG_PLL_PCIE(pll) \
    (MS_SOC_(SCA_CFG_PLL_PCIE_0) + \
     ((pll) * ELB_SOCA_CSR_CFG_PLL_PCIE_0_BYTE_SIZE))

    pal_reg_rd32w(CFG_PLL_PCIE(pll), pll_cfg, 3);

    /*****************
     * word2
     */
    /* set resetb and post_resetb according to input value of on */

    if (on) {
        /* assert post_resetb, resetb */
        pll_cfg[2] = ELB_SOCA_CSR_CFG_PLL_PCIE_0_CFG_PLL_PCIE_0_2_3_POST_RESETB_MODIFY(pll_cfg[2], 0);
        pll_cfg[2] = ELB_SOCA_CSR_CFG_PLL_PCIE_0_CFG_PLL_PCIE_0_2_3_RESETB_MODIFY(pll_cfg[2], 0);
        pll_cfg[1] = ELB_SOCA_CSR_CFG_PLL_PCIE_0_CFG_PLL_PCIE_0_1_3_BYP_EN_MODIFY(pll_cfg[1], 1);
    } else {
        /* deassert post_resetb, resetb */
        pll_cfg[2] = ELB_SOCA_CSR_CFG_PLL_PCIE_0_CFG_PLL_PCIE_0_2_3_POST_RESETB_MODIFY(pll_cfg[2], 1);
        pll_cfg[2] = ELB_SOCA_CSR_CFG_PLL_PCIE_0_CFG_PLL_PCIE_0_2_3_RESETB_MODIFY(pll_cfg[2], 1);
        pll_cfg[1] = ELB_SOCA_CSR_CFG_PLL_PCIE_0_CFG_PLL_PCIE_0_1_3_BYP_EN_MODIFY(pll_cfg[1], 0);
    }

    pal_reg_wr32w(CFG_PLL_PCIE(pll), pll_cfg, 3);
}

/*
 * Note this function sets reset on/off, but the register
 * is inverted logic for _RESET_N.  If on=1 then put the serdes
 * in reset by clearing reset_n bits;
 */
static void
pcieport_set_pcs_reset(const int port,
                       const u_int16_t lanemask,
                       const int on)
{
    u_int32_t v = pal_reg_rd32(PP_(CFG_PP_PCS_RESET_N, port));
    if (on) {
        v &= ~(1 << (port % 4));
    } else {
        v |=  (1 << (port % 4));
    }
    pal_reg_wr32(PP_(CFG_PP_PCS_RESET_N, port), v);
}

static void
pcieport_set_pcs_interrupt_disable(const int port, const u_int16_t lanemask)
{
    pal_reg_wr32(PP_(CFG_PP_PCS_INTERRUPT_DISABLE, port), lanemask);
}

static void
pcieport_pcs_int_disable(const int port, const u_int16_t lanemask)
{
    pcieport_set_pcs_interrupt_disable(port, lanemask);
}

static int
pcieport_mac_unreset(pcieport_t *p)
{
    u_int16_t phystatus;
    int perstn;
    const int maxpolls = 2000; /* 2 seconds */
    int polls = 0;

    do {
        usleep(1000);
        phystatus = pcieport_get_phystatus(p);
        perstn = pcieport_get_perstn(p);
    } while (phystatus && perstn && ++polls < maxpolls);

    p->stats.phypolllast = polls;
    if (polls > p->stats.phypollmax) {
        p->stats.phypollmax = polls;
    }

    if (!perstn) {
        /*
         * perstn went away - we went back into reset
         */
        p->stats.phypollperstn++;
        return -1;
    }

    if (phystatus != 0) {
        /*
         * PHY didn't come out of reset as expected?
         */
        p->stats.phypollfail++;
        return -1;
    }

    pcieport_set_mac_reset(p, 0); /* mac unreset */
    return 0;
}

static void
pcieport_system(const char *path)
{
    int r;

    pciesys_loginfo("running %s:\n", path);
    r = system(path);
    if (r) pciesys_logerror("%s: failed %d\n", path, r);
}

static void
pcieport_run_script(const char *name, const int port)
{
    char path[80];

#ifdef __aarch64__
    snprintf(path, sizeof(path), "/sysconfig/config0/%s", name);
    if (access(path, X_OK) == 0) pcieport_system(path);

    snprintf(path, sizeof(path), "/sysconfig/config0/%s-%d", name, port);
    if (access(path, X_OK) == 0) pcieport_system(path);

    snprintf(path, sizeof(path), "/tmp/%s", name);
    if (access(path, X_OK) == 0) pcieport_system(path);

    snprintf(path, sizeof(path), "/tmp/%s-%d", name, port);
    if (access(path, X_OK) == 0) pcieport_system(path);
#else
    snprintf(path, sizeof(path), "./%s", name);
    if (access(path, X_OK) == 0) pcieport_system(path);

    snprintf(path, sizeof(path), "./%s-%d", name, port);
    if (access(path, X_OK) == 0) pcieport_system(path);
#endif
}

static void
pcieport_poweron_script(pcieport_t *p)
{
    pcieport_run_script("pcie-poweron", p->port);
}

int
pcieportpd_config_host(pcieport_t *p)
{
    if (!pal_is_asic()) {
        /* toggle these resets */
        pcieport_set_serdes_reset(p->port, p->lanemask, 1);
        pcieport_set_pcs_reset(p->port, p->lanemask, 1);
        pcieport_set_serdes_reset(p->port, p->lanemask, 0);
        pcieport_set_pcs_reset(p->port, p->lanemask, 0);
    } else {
        pcieport_info_t *pi = pcieport_info_get();

        if (!pi->serdes_init || pi->serdes_init_always) {
            int host_clock = 1;
            /* ELBA-TODO per port configurablity for future will be great */
            char *env = getenv("PCIEPORT_HOST_CLOCK");

            if (env) {
                host_clock = strtoul(env, NULL, 0);
                pciesys_loginfo("host_clock override %d\n", host_clock);
            }

            pcieportpd_select_pcie_refclk(p->port, host_clock);
            pcieport_set_pcie_pll_rst(p, 0, host_clock);
            pcieport_set_serdes_reset(p->port, p->lanemask, 0);
            if (pcieportpd_serdes_init(p) < 0) {
                return -1;
            }
            /*
             * Make pp_cfg_pp_pcsd_control.sris_en_grp_X follow
             * pp_port_c_cfg_c_mac_k_gen.sris_mode.  When
             * sris_mode=0 (default) we set sris_en_grp_X to 0.
             * Saves about 20ns rx latency.
             */

            /*
             * ELBA-TODO pcieport_pcsd_control_sris needs update for ELBA
             * 4 bits per group instead of 2 per group in capri
             */
            pcieport_pcsd_control_sris(p->sris);
            pi->serdes_init = 1;
        }
        pcieport_set_pcs_reset(p->port, p->lanemask, 1);
        pcieport_set_pcs_reset(p->port, p->lanemask, 0);
    }

    pcieport_set_sw_reset(p, 0);

    pcieport_mac_k_gen(p);
    pcieport_mac_k_rx_cred(p);
    pcieport_mac_k_pexconf(p);
    pcieport_mac_k_pciconf(p);
    pcieport_mac_k_lmr(p);

    pcieport_poweron_script(p);

    /* now ready to unreset mac */
    if (pcieport_mac_unreset(p) < 0) {
        return -1;
    }

    /*
     * Set "margining ready" for better pcie compliance test results.
     * Margining is Gen4 or later.
     */
    pcieport_set_margining_ready(p, 1);

    /*
     * "AER common" controls whether we capture errors
     * from any downstream device and report them on the
     * upstream port bridge.  We ran into a BIOS (HPE Proliant)
     * that didn't like seeing "unsupported request" errors reported
     * in the bridge after the bus scan, and would report CATERR
     * and power off the host.  Safer to disable this, although
     * some might want to see all our errors reported on AER.
     */
    pcieport_set_aer_common_en(p, p->aer_common);

    if (!pal_is_asic()) {
        /* reduce clock frequency for fpga */
        pcieport_set_clock_freq(p, 8);
    } if (p->clock_freq) {
        pcieport_set_clock_freq(p, p->clock_freq / 1000000);
    }
    pcieport_set_ltssm_en(p, 1);  /* ready for ltssm */
    return 0;
}

int
pcieportpd_config_rc(pcieport_t *p)
{
    /* XXX */
    assert(0);
    return 0;
}

void
pcieportpd_mac_set_ids(pcieport_t *p)
{
    const int pn = p->port;
    u_int32_t val;

    /* set subvendor/deviceid */
    val = (p->subdeviceid << 16) | p->subvendorid;
    pal_reg_wr32(PXC_(CFG_C_MAC_SSVID_ELB, pn), val);
}

/*
 * Select local or host as source for pcie refclk.
 * Hw default is local refclk.
 */
void
pcieportpd_select_pcie_refclk(const int port, const int host_clock)
{
    /*
     * Select pll0 source from local osc
     * Select pll1 source from host refclk
     */

    pal_reg_wr32(MS_SOC_(CFG_REFCLK_SEL), 0x2);

    /*
     * XXX ELBA-TODO
     * Currently assuming all 4 ports within a PP use same pcie source clk
     * This can be changed at per-port configuration granularity
     */

    if (host_clock) {
        pal_reg_wr32(PP_(CFG_PP_PCIE_PLL_REFCLK_SEL, port), 0xff);
    } else {
        pal_reg_wr32(PP_(CFG_PP_PCIE_PLL_REFCLK_SEL, port), 0x00);
    }
}

/*
 * Park the pcie mac and serdes hw blocks in reset to match
 * the hardware reset state.  Then, when bringing the pcie link
 * out of reset on the next poweron cycle the normal asic sequence
 * will find the hardware in the expected state.
 *
 * This sequence is the reverse order of the init sequence when
 * powering up and downloading serdes.
 */
static int
pcieport_reset(pcieport_t *p)
{
    /*
     * The serdes init sequence in cap_pcie_serdes_setup()
     * brings all 16 lanes out of reset and downloads serdesfw
     * to all 16 lanes.  To keep that setup sequence happy
     * we park all 16 lanes in reset here, rather than use
     * the port-specific p->lanemask.
     */
    const u_int16_t lanemask = 0xffff;

    pcieport_set_ltssm_en(p, 0);
    pcieport_set_mac_reset(p, 1);
    pcieport_set_sw_reset(p, 1);
    pcieport_pcs_int_disable(p->port, lanemask);
    pcieport_set_pcs_reset(p->port, lanemask, 1);
    pcieport_set_serdes_reset(p->port, lanemask, 1);
    pcieport_set_pcie_pll_rst(p, 1, 1);  /* reset host clock pll1 */
    pcieportpd_select_pcie_refclk(p->port, 0); /* select local refclk */
    return 0;
}

/*
 * On SWM/OCP cards we enter the LOW power state and want
 * to shut off the pcie serdes to conserve power while running
 * in this reduced power state.
 */
int
pcieportpd_config_powerdown(pcieport_t *p)
{
    pcieport_info_t *pi = pcieport_info_get();

    if (!p->config) return -EIO;
    if (!p->host)   return -EINVAL;

    pcieport_reset(p);

    pi->serdes_init = 0;
    p->stats.powerdown++;
    return 0;
}
