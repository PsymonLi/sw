/*
 * Copyright (c) 2017, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/param.h>

#include "platform/pciehdevices/include/pci_ids.h"
#include "platform/misc/include/misc.h"
#include "platform/cfgspace/include/cfgspace.h"
#include "platform/pciemgrutils/include/pciemgrutils.h"
#include "platform/pciemgr/include/pciemgr.h"
#include "pciehcfg_impl.h"

/*
 * XXX
 * We need a copy of the params here to construct VPD info,
 * and we would like to be able to call pciehdev_get_params()
 * but that is in libpciemgr so we're creating a new dependency.
 * The layering needs to be improved here.  Might be time to
 * just combine libpciemgrutils and libpciemgr, the need for separation
 * has generally been removed in most areas.  Fix the rest.
 * XXX
 */
static pciemgr_params_t pciehcfg_params;

void
pciehcfg_set_params(pciemgr_params_t *params)
{
    if (params) pciehcfg_params = *params;
}

pciemgr_params_t *
pciehcfg_get_params(void)
{
    return &pciehcfg_params;
}

void
pciehcfg_setconf_cap_gen(pciehcfg_t *pcfg, const u_int8_t cap_gen)
{
    pcfg->cap_gen = cap_gen;
}

void
pciehcfg_setconf_cap_width(pciehcfg_t *pcfg, const u_int8_t cap_width)
{
    pcfg->cap_width = cap_width;
}

void
pciehcfg_setconf_vendorid(pciehcfg_t *pcfg, const u_int16_t vendorid)
{
    pcfg->vendorid = vendorid;
}

void
pciehcfg_setconf_deviceid(pciehcfg_t *pcfg, const u_int16_t deviceid)
{
    pcfg->deviceid = deviceid;
}

void
pciehcfg_setconf_subvendorid(pciehcfg_t *pcfg, const u_int16_t subvendorid)
{
    pcfg->subvendorid = subvendorid;
}

void
pciehcfg_setconf_subdeviceid(pciehcfg_t *pcfg, const u_int16_t subdeviceid)
{
    pcfg->subdeviceid = subdeviceid;
}

void
pciehcfg_setconf_classcode(pciehcfg_t *pcfg, const u_int32_t classcode)
{
    pcfg->classcode = classcode;
}

void
pciehcfg_setconf_revid(pciehcfg_t *pcfg, const u_int8_t revid)
{
    pcfg->revid = revid;
}

void
pciehcfg_setconf_intpin(pciehcfg_t *pcfg, const u_int8_t intpin)
{
    pcfg->intpin = intpin;
}

void
pciehcfg_setconf_nintrs(pciehcfg_t *pcfg, const u_int16_t nintrs)
{
    pcfg->nintrs = nintrs;
}

void
pciehcfg_setconf_msix_tblbir(pciehcfg_t *pcfg, const u_int8_t bir)
{
    pcfg->msix_tblbir = bir;
}

void
pciehcfg_setconf_msix_tbloff(pciehcfg_t *pcfg, const u_int32_t off)
{
    pcfg->msix_tbloff = off;
}

void
pciehcfg_setconf_msix_pbabir(pciehcfg_t *pcfg, const u_int8_t bir)
{
    pcfg->msix_pbabir = bir;
}

void
pciehcfg_setconf_msix_pbaoff(pciehcfg_t *pcfg, const u_int32_t off)
{
    pcfg->msix_pbaoff = off;
}

void
pciehcfg_setconf_dsn(pciehcfg_t *pcfg, const u_int64_t dsn)
{
    pcfg->dsn = dsn;
}

void
pciehcfg_setconf_flr(pciehcfg_t *pcfg, const int on)
{
    pcfg->flr = on;
}

void
pciehcfg_setconf_exttag(pciehcfg_t *pcfg, const int on)
{
    pcfg->exttag = on;
}

void
pciehcfg_setconf_exttag_en(pciehcfg_t *pcfg, const int on)
{
    pcfg->exttag_en = on;
}

void
pciehcfg_setconf_msicap(pciehcfg_t *pcfg, const int on)
{
    pcfg->msicap = on;
}

void
pciehcfg_setconf_msixcap(pciehcfg_t *pcfg, const int on)
{
    pcfg->msixcap = on;
}

void
pciehcfg_setconf_bridgeup(pciehcfg_t *pcfg, const int on)
{
    assert(!pcfg->bridgedn);
    pcfg->bridgeup = on;
}

void
pciehcfg_setconf_bridgedn(pciehcfg_t *pcfg, const int on)
{
    assert(!pcfg->bridgeup);
    pcfg->bridgedn = on;
}

void
pciehcfg_setconf_fnn(pciehcfg_t *pcfg, const int on)
{
    pcfg->fnn = on;
}

void
pciehcfg_setconf_pf(pciehcfg_t *pcfg, const int on)
{
    pcfg->pf = on;
}

void
pciehcfg_setconf_vf(pciehcfg_t *pcfg, const int on)
{
    pcfg->vf = on;
}

void
pciehcfg_setconf_totalvfs(pciehcfg_t *pcfg, const u_int16_t totalvfs)
{
    assert(pcfg->pf);
    assert(!pcfg->vf);
    pcfg->totalvfs = totalvfs;
}

void
pciehcfg_setconf_vfdeviceid(pciehcfg_t *pcfg, const u_int16_t vfdeviceid)
{
    assert(pcfg->pf);
    assert(!pcfg->vf);
    pcfg->vfdeviceid = vfdeviceid;
}

static void
pciehcfg_setconf_defaults(pciehcfg_t *pcfg)
{
    pciemgr_params_t *params = pciehcfg_get_params();

    pcfg->vendorid = params->vendorid;
    /* pcfg->deviceid does not get a default */
    pcfg->subvendorid = params->subvendorid;
    pcfg->subdeviceid = params->subdeviceid;
    pcfg->cap_gen = params->cap_gen;
    pcfg->cap_width = params->cap_width;
    pcfg->flr = 1;
    pcfg->exttag = 1;
    pcfg->exttag_en = 1;
    pcfg->msixcap = 1;
}

static void
vpdtab_add_id(vpd_table_t *vpdtab, const char *id)
{
    free(vpdtab->id); /* free previous value, if any */
    vpdtab->id = strdup(id);
    assert(vpdtab->id);
}

static int
vpdtab_has_data(const vpd_table_t *vpdtab)
{
    return vpdtab->id || vpdtab->nentries;
}

static void
vpdtab_add(vpd_table_t *vpdtab, const char *key, const char *val)
{
    const size_t nsz = (vpdtab->nentries + 1) * sizeof(vpd_entry_t);
    vpd_entry_t *nentry;
    vpd_entry_t *vpde;
    int i;

    /*
     * Search for the key already in the table.  If found,
     * replace value with new value.  This let's clients override
     * default values if desired, or even cancel default values
     * by setting value to NULL.
     */
    for (vpde = vpdtab->entry, i = 0; i < vpdtab->nentries; i++, vpde++) {
        if (memcmp(vpde->key, key, sizeof(vpde->key)) == 0) {
            free(vpde->val);
            if (val) vpde->val = strdup(val);
            return;
        }
    }

    /*
     * Not found in table.  Extend table and append new entry.
     */
    nentry = pciesys_realloc(vpdtab->entry, nsz);
    vpdtab->entry = nentry;
    vpde = &vpdtab->entry[vpdtab->nentries++];
    memset(vpde, 0, sizeof(*vpde));
    memcpy(vpde->key, key, sizeof(vpde->key));
    if (val) vpde->val = strdup(val);
}

static void
vpdtab_free(vpd_table_t *vpdtab)
{
    int i;

    for (i = 0; i < vpdtab->nentries; i++) {
        free(vpdtab->entry[i].val);
    }
    free(vpdtab->id);
    pciesys_free(vpdtab->entry);
    vpdtab->id = NULL;
    vpdtab->entry = NULL;
    vpdtab->nentries = 0;
}

pciehcfg_t *
pciehcfg_new(void)
{
    pciehcfg_t *pcfg = pciesys_zalloc(sizeof(pciehcfg_t) + (2 * PCIEHCFGSZ));

    pcfg->cur = (u_int8_t *)(pcfg + 1);
    pcfg->msk = pcfg->cur + PCIEHCFGSZ;

    pcfg->cap_cursor = 0x40;
    pcfg->extcap_cursor = 0x100;

    pciehcfg_setconf_defaults(pcfg);

    return pcfg;
}

void
pciehcfg_delete(pciehcfg_t *pcfg)
{
    if (pcfg == NULL) return;
    vpdtab_free(&pcfg->vpdtab);
    pciesys_free(pcfg);
}

void
pciehcfg_get_cfgspace(pciehcfg_t *pcfg, cfgspace_t *cs)
{
    cs->cur = pcfg->cur;
    cs->msk = pcfg->msk;
    cs->size = PCIEHCFGSZ;
}

static cfgspace_bartype_t
bartype_to_cfgbartype(pciehbartype_t bt)
{
    cfgspace_bartype_t cfgbt;

    switch (bt) {
    case PCIEHBARTYPE_MEM:   cfgbt = CFGSPACE_BARTYPE_MEM;   break;
    case PCIEHBARTYPE_MEM64: cfgbt = CFGSPACE_BARTYPE_MEM64; break;
    case PCIEHBARTYPE_IO:    cfgbt = CFGSPACE_BARTYPE_IO;    break;
    default:                 cfgbt = CFGSPACE_BARTYPE_NONE;  break;
    }
    return cfgbt;
}

static int
bars_to_cfgbars(pciehbars_t *pbars, cfgspace_bar_t *cfgbars)
{
    cfgspace_bar_t *cpb;
    pciehbar_t *b;
    int nbars;

    nbars = 0;
    cpb = cfgbars;
    for (b = pciehbars_get_first(pbars);
         b;
         b = pciehbars_get_next(pbars, b), cpb++) {
        nbars++;
        cpb->type = bartype_to_cfgbartype(b->type);
        cpb->prefetch = b->prefetch;
        cpb->size = b->size;
        cpb->cfgidx = b->cfgidx;
    }
    return nbars;
}

static void
pciehcfg_get_cfgspace_header_params(pciehcfg_t *pcfg,
                                    pciehbars_t *pbars,
                                    cfgspace_header_params_t *cp)
{
    pciehbar_t *b;

    memset(cp, 0, sizeof(*cp));
    cp->vendorid = pcfg->vendorid;
    cp->deviceid = pcfg->deviceid;
    cp->subvendorid = pcfg->subvendorid;
    cp->subdeviceid = pcfg->subdeviceid;
    cp->classcode = pcfg->classcode;
    cp->revid = pcfg->revid;
    /* if setconf value use it, else derive based on nintrs */
    cp->intpin = pcfg->intpin ? pcfg->intpin : pcfg->nintrs ? 1 : 0;
    cp->vf = pcfg->vf;

    cp->nbars = bars_to_cfgbars(pbars, cp->bars);
    b = pciehbars_get_rombar(pbars);
    if (b != NULL) {
        cp->rombar.type = bartype_to_cfgbartype(b->type);
        cp->rombar.size = b->size;
    }
}

static void
pciehcfg_get_cfgspace_capparams(pciehcfg_t *pcfg,
                                pciehbars_t *vfbars,
                                cfgspace_capparams_t *cp)
{
    memset(cp, 0, sizeof(*cp));
    cp->bridgeup = pcfg->bridgeup;
    cp->bridgedn = pcfg->bridgedn;
    cp->flr = pcfg->flr;
    cp->exttag = pcfg->exttag;
    cp->exttag_en = pcfg->exttag_en;
    cp->fnn = pcfg->fnn;
    cp->subvendorid = pcfg->subvendorid;
    cp->subdeviceid = pcfg->subdeviceid;
    cp->nintrs = pcfg->nintrs;
    cp->vfdeviceid = pcfg->vfdeviceid;
    cp->totalvfs = pcfg->totalvfs;
    cp->cap_gen = pcfg->cap_gen;
    cp->cap_width = pcfg->cap_width;
    cp->msix_tblbir = pcfg->msix_tblbir;
    cp->msix_tbloff = pcfg->msix_tbloff;
    cp->msix_pbabir = pcfg->msix_pbabir;
    cp->msix_pbaoff = pcfg->msix_pbaoff;
    cp->dsn = pcfg->dsn;
    cp->nvfbars = bars_to_cfgbars(vfbars, cp->vfbars);
}

void
pciehcfg_sethdr_type0(pciehcfg_t *pcfg, pciehbars_t *pbars)
{
    cfgspace_t cs;
    cfgspace_header_params_t cp;

    pciehcfg_get_cfgspace(pcfg, &cs);
    pciehcfg_get_cfgspace_header_params(pcfg, pbars, &cp);
    cfgspace_sethdr_type0(&cs, &cp);
}

void
pciehcfg_sethdr_type1(pciehcfg_t *pcfg, pciehbars_t *pbars)
{
    cfgspace_t cs;
    cfgspace_header_params_t cp;

    pciehcfg_get_cfgspace(pcfg, &cs);
    pciehcfg_get_cfgspace_header_params(pcfg, pbars, &cp);
    cfgspace_sethdr_type1(&cs, &cp);
}

static void
addcap(pciehcfg_t *pcfg,
       cfgspace_t *cs,
       const cfgspace_capparams_t *cp,
       const char *capname)
{
    u_int8_t capaddr;

    /*
     * Add requested cap at cursor (rounded up to 0x20 for
     * convenience--"lspci -xxx" shows 0x10 bytes per line).
     */
    capaddr = roundup(pcfg->cap_cursor, 0x20);
    capaddr += cfgspace_setcap(cs, capname, cp, capaddr);
    pcfg->cap_cursor = capaddr;
}

static void
addextcap(pciehcfg_t *pcfg,
          cfgspace_t *cs,
          const cfgspace_capparams_t *cp,
          const char *capname)
{
    u_int16_t capaddr;

    /*
     * Add requested cap at cursor (rounded up to 0x20 for
     * convenience--"lspci -xxx" shows 0x10 bytes per line).
     */
    capaddr = roundup(pcfg->extcap_cursor, 0x20);
    capaddr += cfgspace_setextcap(cs, capname, cp, capaddr);
    pcfg->extcap_cursor = capaddr;
}

void
pciehcfg_addcap(pciehcfg_t *pcfg, const char *capname)
{
    cfgspace_t cfgspace, *cs = &cfgspace;
    cfgspace_capparams_t capparams, *cp = &capparams;

    pciehcfg_get_cfgspace(pcfg, cs);
    pciehcfg_get_cfgspace_capparams(pcfg, NULL, cp);
    addcap(pcfg, cs, cp, capname);
}

void
pciehcfg_addextcap(pciehcfg_t *pcfg, const char *capname)
{
    cfgspace_t cfgspace, *cs = &cfgspace;
    cfgspace_capparams_t capparams, *cp = &capparams;

    pciehcfg_get_cfgspace(pcfg, cs);
    pciehcfg_get_cfgspace_capparams(pcfg, NULL, cp);
    addextcap(pcfg, cs, cp, capname);
}

void
pciehcfg_add_standard_pfcaps(pciehcfg_t *pcfg, pciehbars_t *vfbars)
{
    cfgspace_t cfgspace, *cs = &cfgspace;
    cfgspace_capparams_t capparams, *cp = &capparams;

    pciehcfg_get_cfgspace(pcfg, cs);
    pciehcfg_get_cfgspace_capparams(pcfg, vfbars, cp);

    /*****************
     * Capabilities.
     */
    addcap(pcfg, cs, cp, "pcie");
    addcap(pcfg, cs, cp, "pm");
    if (pcfg->nintrs) {
        if (pcfg->msicap) {
            addcap(pcfg, cs, cp, "msi");
        }
        if (pcfg->msixcap) {
            addcap(pcfg, cs, cp, "msix");
        }
    }
    if (cfgspace_is_bridge(cp)) {
        addcap(pcfg, cs, cp, "subsys");
    }
    if (vpdtab_has_data(&pcfg->vpdtab)) {
        addcap(pcfg, cs, cp, "vpd");
    }

    /*****************
     * Extended capabilities.
     */
    addextcap(pcfg, cs, cp, "aer");
    if (pcfg->pf) {
        addextcap(pcfg, cs, cp, "sriov");
    }
    if ((pcfg->pf || pcfg->vf) && !cfgspace_is_bridge(cp)) {
        /* only endpoints need ARI */
        addextcap(pcfg, cs, cp, "ari");
    }
    if (pcfg->dsn) {
        addextcap(pcfg, cs, cp, "dsn");
    }
    if (cfgspace_is_bridgedn(cp)) {
        addextcap(pcfg, cs, cp, "acs");
    }

    /* Gen3 ext caps */
    if (pcfg->cap_gen >= 3) {
        if (!pcfg->fnn) {
            addextcap(pcfg, cs, cp, "spcie");
        }
    }

    /* Gen4 ext caps */
    if (pcfg->cap_gen >= 4) {
        /* All Gen4 Downstream Ports require this */
        if (pcfg->bridgedn) {
            addextcap(pcfg, cs, cp, "datalink");
        }
        /* All Gen4 function 0's require this */
        if (!pcfg->fnn) {
            addextcap(pcfg, cs, cp, "physlayer");
            addextcap(pcfg, cs, cp, "lanemargin");
        }
    }
}

void
pciehcfg_add_standard_caps(pciehcfg_t *pcfg)
{
    pciehcfg_add_standard_pfcaps(pcfg, NULL);
}

void
pciehcfg_add_vpd_id(pciehcfg_t *pcfg, const char *id)
{
    vpd_table_t *vpdtab = &pcfg->vpdtab;
    vpdtab_add_id(vpdtab, id);
}

void
pciehcfg_add_vpd(pciehcfg_t *pcfg, const char *key, const char *val)
{
    vpd_table_t *vpdtab = &pcfg->vpdtab;
    vpdtab_add(vpdtab, key, val);
}

/*****************************************************************
 * Implement HPE VPD as documented in
 *
 *     Common Requirements Appendix C
 *     HP BCS VPD Mezzanine Card Adapter
 *     Requirements
 *     Version 1.7
 */

/* HPE specified vendor-specific keys V0-V9 */
#define HPE_VPD_KEY_MISC        "V0"    /* freq, power, etc */
#define HPE_VPD_KEY_UEFIVERS    "V1"    /* uefi driver version a.b.c.d */
#define HPE_VPD_KEY_MFGDATE     "V2"    /* mfg date code yyww */
#define HPE_VPD_KEY_FWVERS      "V3"    /* rom fw version a.b.c.d */
#define HPE_VPD_KEY_MAC         "V4"    /* mac address */
#define HPE_VPD_KEY_PCAREV      "V5"    /* pca revision info  */
#define HPE_VPD_KEY_PXEVERS     "V6"    /* pxe version a.b.c.d */
/* VA-VJ available for OEM Vendor use */
/* VK-VZ reserved for HPE future use */
/* Yx reserved for HPE future use */
/* YA asset tag (optional) */

static void
pciehcfg_add_vpd_hpe(pciehcfg_t *pcfg, const pciemgr_params_t *params)
{
    if (params->id[0] != '\0') {
        pciehcfg_add_vpd_id(pcfg, params->id);
    }

#define S(key, field) \
    if (params->field[0] != '\0') \
        pciehcfg_add_vpd(pcfg, key, params->field);

    /* HPE VPD spec says "PN" first */
    S("PN", partnum);
    S("SN", serialnum);
    S("EC", engdate);
    S(HPE_VPD_KEY_MFGDATE, mfgdate);
    S(HPE_VPD_KEY_PCAREV, pcarev);
    S(HPE_VPD_KEY_MISC, misc);
    S(HPE_VPD_KEY_FWVERS, fwvers);
#undef S
}

static void
pciehcfg_add_macaddr_vpd_hpe(pciehcfg_t *pcfg,
                             const pciemgr_params_t *params,
                             const u_int64_t macaddr)
{
    if (macaddr) {
        char macstr[16];
        snprintf(macstr, sizeof(macstr), "%012" PRIx64, macaddr);
        pciehcfg_add_vpd(pcfg, HPE_VPD_KEY_MAC, macstr);
    }
}

/*****************************************************************
 * Implements Dell VPD contents as specified in
 *
 *     Dell Specific PCI Device VPD Data Content Requirements
 *     Revision A04-01
 */

/*
 * Dell specified vendor-specific keys
 * Dell is flexible, use any vendor-specific keys V0-V9,VA-VZ.
 * Dell uses the "Dell Subtype Keyword" (DSK) to find the key.
 */
/* pensando keys */
#define DELL_VPD_KEY_FWV        "V3"    /* FW Version - V3 matches HPE */
#define DELL_VPD_KEY_MAC        "V4"    /* MAC address - V4 matches HPE */
/* Dell keys */
#define DELL_VPD_KEY_DSV        "VA"    /* Dell Signature and Version */
#define DELL_VPD_KEY_NMV        "VB"    /* NaMe of Vendor */
#define DELL_VPD_KEY_FFV        "VC"    /* Family Firmware Version (NIC) */
#define DELL_VPD_KEY_DTI        "VD"    /* Device Type Identifier (NIC) */
#define DELL_VPD_KEY_NPY        "VE"    /* Number of PhYsical ports (NIC) */
#define DELL_VPD_KEY_PMT        "VF"    /* Physical Media Type */
#define DELL_VPD_KEY_DCM        "VG"    /* Device Capability Mapping */

static char *
dsk_concat(const char *dsk, const char *val)
{
    static char buf[80];

    assert(strlen(dsk) + strlen(val) < sizeof(buf) - 1);
    snprintf(buf, sizeof(buf), "%s%s", dsk, val);
    return buf;
}

/*
 * DSK - Dell Subtype Keyword
 * prepend DSK to the value string.
 */
static void
vpd_dell_dsk(pciehcfg_t *pcfg,
             const char *key, const char *dsk, const char *val)
{
    pciehcfg_add_vpd(pcfg, key, dsk_concat(dsk, val));
}

static void
vpd_dell_ffv_vers(pciehcfg_t *pcfg,
                  const unsigned int maj,
                  const unsigned int min,
                  const unsigned int bld,
                  const unsigned int sub)
{
    char ffv[16];

    if (maj < 100 && min < 100) {
        if (bld < 100) {
            if (sub < 100) {
                snprintf(ffv, sizeof(ffv),
                         "%02u.%02u.%02u.%02u", maj, min, bld, sub);
            } else {
                snprintf(ffv, sizeof(ffv),
                         "%02u.%02u.%02u", maj, min, bld);
            }
        } else {
            snprintf(ffv, sizeof(ffv),
                     "%02u.%02u", maj, min);
        }
        vpd_dell_dsk(pcfg, DELL_VPD_KEY_FFV, "FFV", ffv);
    } else {
        pciesys_logwarn("vpd_dell_ffv_vers encode failed %u %u %u %u\n",
                        maj, min, bld, sub);
    }
}

/*
 * Dell Family Firmware Version
 *
 * Major.Minor.Build.Sub-build (Build, Sub-build optional)
 * Each part is exactly 2 digits, 0-padded
 */
static void
vpd_dell_ffv(pciehcfg_t *pcfg, const pciemgr_params_t *params)
{
    unsigned int maj, min, bld, sub, n;

    n = sscanf(params->fwvers, "%u.%u.%u-%u", &maj, &min, &bld, &sub);
    if (n == 4) {
        vpd_dell_ffv_vers(pcfg, maj, min, bld, sub);
        return;
    }
    n = sscanf(params->fwvers, "%u.%u.%u.%u", &maj, &min, &bld, &sub);
    if (n == 4) {
        vpd_dell_ffv_vers(pcfg, maj, min, bld, sub);
        return;
    }
    n = sscanf(params->fwvers, "%u.%u.%u-%*c-%u", &maj, &min, &bld, &sub);
    if (n == 4) {
        vpd_dell_ffv_vers(pcfg, maj, min, bld, sub);
        return;
    }
    n = sscanf(params->fwvers, "%u.%u.%u", &maj, &min, &bld);
    if (n == 3) {
        vpd_dell_ffv_vers(pcfg, maj, min, bld, 100);
        return;
    }
    n = sscanf(params->fwvers, "%u.%u", &maj, &min);
    if (n == 2) {
        vpd_dell_ffv_vers(pcfg, maj, min, 100, 100);
        return;
    }
    pciesys_logwarn("vpd_dell_ffv encode failed %s\n", params->fwvers);
}

static void
pciehcfg_add_vpd_dell(pciehcfg_t *pcfg, const pciemgr_params_t *params)
{
    if (params->id[0] != '\0') {
        pciehcfg_add_vpd_id(pcfg, params->id);
    }

#define S(key, field) \
    if (params->field[0] != '\0') \
        pciehcfg_add_vpd(pcfg, key, params->field);

    S("PN", partnum);
    S("SN", serialnum);
    S("EC", engdate);
    S(DELL_VPD_KEY_FWV, fwvers);
#undef S

    pciehcfg_add_vpd(pcfg, "MN", "1028"); /* Dell Vendor Id */

    vpd_dell_dsk(pcfg, DELL_VPD_KEY_DSV, "DSV", "1028VPDR.VER2.2");
    vpd_dell_dsk(pcfg, DELL_VPD_KEY_NMV, "NMV", "Pensando Systems");
}

static void
pciehcfg_add_macaddr_vpd_dell(pciehcfg_t *pcfg,
                              const pciemgr_params_t *params,
                              const u_int64_t macaddr)
{
    if (macaddr) {
        char macstr[16];
        snprintf(macstr, sizeof(macstr), "%012" PRIx64, macaddr);
        pciehcfg_add_vpd(pcfg, DELL_VPD_KEY_MAC, macstr);
    }

    /*
     * macaddr comes from our network devices, and
     * Dell gets this additional info for network devices.
     */
    vpd_dell_ffv(pcfg, params);
    vpd_dell_dsk(pcfg, DELL_VPD_KEY_DTI, "DTI", "NIC");
    vpd_dell_dsk(pcfg, DELL_VPD_KEY_NPY, "NPY", "2");
    vpd_dell_dsk(pcfg, DELL_VPD_KEY_PMT, "PMT", "D"); /* SFF Cage */
    vpd_dell_dsk(pcfg, DELL_VPD_KEY_DCM, "DCM", "1001FFFFFF2001FFFFFF");
}

void
pciehcfg_add_standard_vpd(pciehcfg_t *pcfg)
{
    const pciemgr_params_t *params = pciehcfg_get_params();
    const pciemgr_vpd_format_t vpd_format = params->vpd_format;

    switch (vpd_format) {
    case VPD_FORMAT_DELL:
        pciehcfg_add_vpd_dell(pcfg, params);
        break;
    case VPD_FORMAT_HPE:
        pciehcfg_add_vpd_hpe(pcfg, params);
        break;
    default:
        pciesys_logwarn("vpd: unknown vpd_format %d\n", vpd_format);
        break;
    }
}

void
pciehcfg_add_macaddr_vpd(pciehcfg_t *pcfg, const u_int64_t macaddr)
{
    const pciemgr_params_t *params = pciehcfg_get_params();
    const pciemgr_vpd_format_t vpd_format = params->vpd_format;

    switch (vpd_format) {
    case VPD_FORMAT_DELL:
        pciehcfg_add_macaddr_vpd_dell(pcfg, params, macaddr);
        break;
    case VPD_FORMAT_HPE:
        pciehcfg_add_macaddr_vpd_hpe(pcfg, params, macaddr);
        break;
    default:
        pciesys_logwarn("macaddr_vpd: unknown vpd_format %d\n", vpd_format);
        break;
    }
}

void
pciehcfg_make_fn0(pciehcfg_t *pcfg)
{
    cfgspace_t cfgspace, *cs = &cfgspace;
    u_int32_t v;

    /* set multi-function bit in header */
    pciehcfg_get_cfgspace(pcfg, cs);
    v = cfgspace_get_headertype(cs);
    if ((v & 0x80) == 0) {
        v |= 0x80;
        cfgspace_set_headertype(cs, v);
    }
}

void
pciehcfg_make_fnn(pciehcfg_t *pcfg, const int fnn)
{
    cfgspace_t cfgspace, *cs = &cfgspace;
    u_int32_t v;

    /* adjust intpin value (if set) */
    pciehcfg_get_cfgspace(pcfg, cs);
    v = cfgspace_get_intpin(cs);
    if (v) {
        v = (fnn % 4) + 1;
        cfgspace_set_intpin(cs, v);
    }
}
