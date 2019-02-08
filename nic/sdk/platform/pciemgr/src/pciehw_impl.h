/*
 * Copyright (c) 2017, Pensando Systems Inc.
 */

#ifndef __PCIEHW_IMPL_H__
#define __PCIEHW_IMPL_H__

#include "cap_top_csr_defines.h"
#include "cap_pxb_c_hdr.h"

#include "nic/sdk/platform/pciemgr/include/pciehw_dev.h"
#include "nic/sdk/platform/pciemgr/include/pciehw.h"
#include "nic/sdk/platform/pciemgrutils/include/pciemgrutils.h"

#include "pmt.h"
#include "prt.h"
#include "notify.h"
#include "indirect.h"
#include "req_int.h"
#include "event.h"

struct pciehw_s;
typedef struct pciehw_s pciehw_t;

#define PCIEHW_NPORTS   8
#define PCIEHW_NDEVS    48
#define PCIEHW_CFGSZ    1024
#define PCIEHW_NROMSK   128
#define PCIEHW_ROMSKSZ  (PCIEHW_CFGSZ / sizeof (u_int32_t))
#define PCIEHW_CFGHNDSZ (PCIEHW_CFGSZ / sizeof (u_int32_t))

enum pciehw_cfghnd_e {
    PCIEHW_CFGHND_NONE,
    PCIEHW_CFGHND_CMD,
    PCIEHW_CFGHND_BARS,
    PCIEHW_CFGHND_ROMBAR,
    PCIEHW_CFGHND_MSIX,
};
typedef enum pciehw_cfghnd_e pciehw_cfghnd_t;

#define PCIEHW_NPMT     PMT_COUNT
#define PCIEHW_NPRT     PRT_COUNT
#define PCIEHW_NBAR     6               /* 6 cfgspace BARs */
#define PCIEHW_NOTIFYSZ NOTIFYSZ

typedef u_int32_t pciehwdevh_t;

typedef enum pciehwbartype_e {
    PCIEHWBARTYPE_NONE,                 /* invalid bar type */
    PCIEHWBARTYPE_MEM,                  /* 32-bit memory bar */
    PCIEHWBARTYPE_MEM64,                /* 64-bit memory bar */
    PCIEHWBARTYPE_IO,                   /* 32-bit I/O bar */
} pciehwbartype_t;

typedef struct pciehwbar_s {
    u_int32_t valid:1;                  /* valid bar for this dev */
    pciehwbartype_t type;               /* PCIEHWBARTYPE_* */
    u_int32_t pmtb;                     /* pmt base  for bar */
    u_int32_t pmtc;                     /* pmt count for bar */
} pciehwbar_t;

typedef struct pciehwdev_s {
    char name[32];                      /* device name */
    int port;                           /* pcie port */
    void *pdev;                         /* pciehdev */
    u_int16_t bdf;                      /* bdf of this dev */
    u_int32_t lifc;                     /* lif count for this dev */
    u_int32_t lifb;                     /* lif base  for this dev */
    u_int32_t intrb;                    /* intr resource base */
    u_int32_t intrc;                    /* intr resource count */
    pciehwdevh_t parenth;               /* handle to parent */
    pciehwdevh_t childh;                /* handle to child */
    pciehwdevh_t peerh;                 /* handle to peer */
    u_int8_t intpin;                    /* legacy int pin */
    u_int8_t stridesel;                 /* vfstride table entry selector */
    u_int8_t romsksel[PCIEHW_ROMSKSZ];  /* cfg read-only mask selectors */
    u_int8_t cfgpmtf[PCIEHW_CFGHNDSZ];  /* cfg pmt flags */
    u_int8_t cfghnd[PCIEHW_CFGHNDSZ];   /* cfg indirect/notify handlers */
    pciehwbar_t bar[PCIEHW_NBAR];
    pciehwbar_t rombar;                 /* option rom bar */
} pciehwdev_t;

typedef struct pciehw_port_s {
    u_int8_t secbus;                    /* bridge secondary bus */
    u_int64_t indirect_cnt;             /* total count of indirect events */
    u_int64_t notify_intr;              /* notify interrupts */
    u_int64_t notify_cnt;               /* total count of notify events */
    u_int32_t notify_max;               /* largest pending notify events */
    u_int64_t notspurious;
    u_int64_t notcfgrd;
    u_int64_t notcfgwr;
    u_int64_t notmemrd;
    u_int64_t notmemwr;
    u_int64_t notiord;
    u_int64_t notiowr;
    u_int64_t notunknown;
    u_int64_t indspurious;
    u_int64_t indcfgrd;
    u_int64_t indcfgwr;
    u_int64_t indmemrd;
    u_int64_t indmemwr;
    u_int64_t indiord;
    u_int64_t indiowr;
    u_int64_t indunknown;
} pciehw_port_t;

typedef struct pciehw_sprt_s {
    prt_t prt;                          /* shadow copy of prt */
} pciehw_sprt_t;

typedef struct pciehw_spmt_s {
    u_int64_t baroff;                   /* bar addr offset */
    pciehwdevh_t owner;                 /* current owner of this entry */
    u_int8_t loaded:1;                  /* is loaded into hw */
    u_int8_t cfgidx;                    /* cfgidx for bar we belong to */
    pmt_t pmt;                          /* shadow copy of pmt */
} pciehw_spmt_t;

typedef struct pciehw_sromsk_s {
    u_int32_t entry;
    u_int32_t count;
} pciehw_sromsk_t;

#define PCIEHW_MAGIC    0x706d656d      /* 'pmem' */
#define PCIEHW_VERSION  0x1

typedef struct pciehw_shmem_s {
    u_int32_t magic;                    /* PCIEHW_MAGIC when initialized */
    u_int32_t version;                  /* PCIEHW_VERSION when initialized */
    u_int32_t hwinit:1;                 /* hw is initialized */
    u_int32_t notify_verbose:1;         /* notify logs all */
    u_int32_t skip_notify:1;            /* notify skips if ring full */
    u_int32_t allocdev;
    u_int32_t allocpmt;
    u_int32_t allocprt;
    pciehwdevh_t rooth[PCIEHW_NPORTS];
    pciehwdev_t dev[PCIEHW_NDEVS];
    pciehw_port_t port[PCIEHW_NPORTS];
    pciehw_sromsk_t sromsk[PCIEHW_NROMSK];
    pciehw_spmt_t spmt[PCIEHW_NPMT];
    pciehw_sprt_t sprt[PCIEHW_NPRT];
    u_int8_t cfgrst[PCIEHW_NDEVS][PCIEHW_CFGSZ];
    u_int8_t cfgmsk[PCIEHW_NDEVS][PCIEHW_CFGSZ];
    u_int32_t notify_ring_mask;
} pciehw_shmem_t;

typedef struct pciehw_mem_s {
    u_int8_t notify_area[PCIEHW_NPORTS][PCIEHW_NOTIFYSZ]
                                     __attribute__((aligned(PCIEHW_NOTIFYSZ)));
    u_int8_t cfgcur[PCIEHW_NDEVS][PCIEHW_CFGSZ] __attribute__((aligned(4096)));
    u_int32_t notify_intr_dest[PCIEHW_NPORTS];   /* notify   intr dest */
    u_int32_t indirect_intr_dest[PCIEHW_NPORTS]; /* indirect intr dest */
    u_int8_t zeros[0x1000];             /* page of zeros to back cfgspace */
    u_int32_t magic;                    /* PCIEHW_MAGIC when initialized */
    u_int32_t version;                  /* PCIEHW_VERSION when initialized */
} pciehw_mem_t;

typedef struct pciehw_s {
    u_int32_t open:1;                   /* hw is in use */
    u_int16_t clients;                  /* number of clients using us */
    pciehdev_params_t params;
    pciehw_mem_t *pciehwmem;
    pciehw_shmem_t *pcieshmem;
} pciehw_t;

struct pcie_stlp_s;
typedef struct pcie_stlp_s pcie_stlp_t;

pciehw_mem_t *pciehw_get_hwmem(void);
pciehw_shmem_t *pciehw_get_shmem(void);
pciehdev_params_t *pciehw_get_params(void);
pciehwdev_t *pciehwdev_get(pciehwdevh_t hwdevh);
pciehwdevh_t pciehwdev_geth(const pciehwdev_t *phwdev);
void pciehwdev_get_cfgspace(const pciehwdev_t *phwdev, cfgspace_t *cs);
const char *pciehwdev_get_name(const pciehwdev_t *phwdev);
u_int16_t pciehwdev_get_bdf(const pciehwdev_t *phwdev);
pciehwdev_t *pciehwdev_find_by_name(const char *name);

#include "hdrt.h"
#include "portmap.h"
#include "intr.h"
#include "reset.h"

int pciehw_cfg_init(void);
int pciehw_cfg_finalize(pciehdev_t *pdev);
int pciehw_cfg_finalize_done(pciehwdev_t *phwroot);
int pciehwdev_cfgrd(pciehwdev_t *phwdev,
                    const u_int16_t offset,
                    const u_int8_t size,
                    u_int32_t *valp);
int pciehwdev_cfgwr(pciehwdev_t *phwdev,
                    const u_int16_t offset,
                    const u_int8_t size,
                    const u_int32_t val);

void pciehw_cfgrd_notify(pciehwdev_t *phwdev,
                         const pcie_stlp_t *stlp,
                         const pciehw_spmt_t *spmt);
void pciehw_cfgwr_notify(pciehwdev_t *phwdev,
                         const pcie_stlp_t *stlp,
                         const pciehw_spmt_t *spmt);

struct indirect_entry_s;
typedef struct indirect_entry_s indirect_entry_t;
void pciehw_cfgrd_indirect(indirect_entry_t *ientry, const pcie_stlp_t *stlp);
void pciehw_cfgwr_indirect(indirect_entry_t *ientry, const pcie_stlp_t *stlp);

int pciehw_bar_init(void);
int pciehw_bars_finalize(pciehdev_t *pdev);
void pciehw_bar_setaddr(pciehwbar_t *phwbar, const u_int64_t addr);
void pciehw_bar_load(pciehwbar_t *phwbar);
void pciehw_bar_enable(pciehwbar_t *phwbar, const int on);
u_int64_t pciehw_bar_getsize(pciehwbar_t *phwbar);
void pciehw_barrd_notify(pciehwdev_t *phwdev,
                         const pcie_stlp_t *stlp,
                         const pciehw_spmt_t *spmt);
void pciehw_barwr_notify(pciehwdev_t *phwdev,
                         const pcie_stlp_t *stlp,
                         const pciehw_spmt_t *spmt);
void pciehw_barrd_indirect(indirect_entry_t *ientry, const pcie_stlp_t *stlp);
void pciehw_barwr_indirect(indirect_entry_t *ientry, const pcie_stlp_t *stlp);
void pciehw_bar_dbg(int argc, char *argv[]);

#define ROMSK_RDONLY 1

void pciehw_romsk_init(void);
int pciehw_romsk_load(pciehwdev_t *phwdev);
void pciehw_romsk_unload(pciehwdev_t *phwdev);
void pciehw_romsk_dbg(int argc, char *argv[]);

#define VFSTRIDE_IDX_DEV        0x0
#define VFSTRIDE_IDX_4K         0x1

void pciehw_vfstride_init(void);
int pciehw_vfstride_load(pciehwdev_t *phwdev);
void pciehw_vfstride_unload(pciehwdev_t *phwdev);

void pciehw_tgt_port_init(void);
void pciehw_tgt_port_skip_notify(const int port, const int on);

void pciehw_itr_port_init(void);

void pciehw_poll(void);

void *pciehw_memset(void *s, int c, size_t n);
void *pciehw_memcpy(void *dst, const void *src, size_t n);

#endif /* __PCIEHW_IMPL_H__ */
