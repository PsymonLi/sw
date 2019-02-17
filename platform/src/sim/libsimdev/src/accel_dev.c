/*
 * Copyright (c) 2018, Pensando Systems Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/param.h>

#include "nic/sdk/platform/misc/include/maclib.h"
#include "nic/sdk/platform/misc/include/bdf.h"
#include "platform/src/sim/libsimlib/include/simserver.h"
#include "platform/src/sim/libsimdev/src/dev_utils.h"
#include "platform/src/sim/libsimdev/src/simdev_impl.h"

/* Supply these for accel_dev_if.h */
#define dma_addr_t uint64_t

#include "nic/sdk/storage/storage_seq_common.h"
#include "nic/include/accel_ring.h"
#include "nic/include/accel_dev_if.h"

typedef struct accelparams_s {
    int lif;
    int intr_base;
    u_int64_t cmb_base;
    int upd[8];
    u_int64_t devcmd_pa;
    u_int64_t devcmddb_pa;
} accelparams_t;

static simdev_t *current_sd;

static int
accel_lif(simdev_t *sd)
{
    accelparams_t *ep = sd->priv;
    return ep->lif;
}

static int
accel_intrb(simdev_t *sd)
{
    accelparams_t *ep = sd->priv;
    return ep->intr_base;
}

static u_int64_t
accel_cmb_base(simdev_t *sd)
{
    accelparams_t *ep = sd->priv;
    return ep->cmb_base;
}

/*
 * ================================================================
 * dev_cmd regs
 * ----------------------------------------------------------------
 */

struct dev_cmd_regs {
    u_int32_t signature;
    u_int32_t done;
    u_int32_t cmd[16];
    u_int32_t comp[4];
    uint8_t data[2048] __attribute__((aligned (2048)));
};

_Static_assert(sizeof(struct dev_cmd_regs) == 4096,
                "Devcmd region should be 4K bytes");
_Static_assert((offsetof(struct dev_cmd_regs, cmd)  % 4) == 0,
                "Devcmd cmd field should be word-aligned");
_Static_assert(sizeof(((struct dev_cmd_regs*)0)->cmd) == 64,
                "Devcmd cmd field should be 64 bytes");
_Static_assert((offsetof(struct dev_cmd_regs, comp) % 4) == 0,
                "Devcmd comp field should be word-aligned");
_Static_assert(sizeof(((struct dev_cmd_regs*)0)->comp) == 16,
                "Devcmd comp field should be 16 bytes");
_Static_assert((offsetof(struct dev_cmd_regs, data) % 4) == 0,
                "Devcmd data region should be word-aligned");

static struct dev_cmd_regs dev_cmd_regs = {
    .signature = DEV_CMD_SIGNATURE,
};

/*
 * ================================================================
 * bar region handlers
 * ----------------------------------------------------------------
 */

static int
bar_invalid_rd(int bar, int reg,
               u_int64_t offset, u_int8_t size, u_int64_t *valp)
{
    simdev_error("invalid_rd: bar %d reg %d off 0x%"PRIx64" size %d\n",
                 bar, reg, offset, size);
    return -1;
}

static int
bar_invalid_wr(int bar, int reg,
               u_int64_t offset, u_int8_t size, u_int64_t val)
{
    simdev_error("invalid_wr: bar %d reg %d off 0x%"PRIx64" "
                 "size %d = val 0x%"PRIx64"\n",
                 bar, reg, offset, size, val);
    return -1;
}

static int
bar_devcmd_rd(int bar, int reg,
              u_int64_t offset, u_int8_t size, u_int64_t *valp)
{
    if (offset + size > sizeof(dev_cmd_regs)) {
        simdev_error("devcmd_rd: invalid offset 0x%"PRIx64" size 0x%x\n",
                     offset, size);
        return -1;
    }

    accelparams_t *ep = current_sd->priv;
    simdev_read_mem(ep->devcmd_pa + offset, valp, size);

    return 0;
}

static int
bar_devcmd_wr(int bar, int reg,
              u_int64_t offset, u_int8_t size, u_int64_t val)
{
    if (offset + size >= sizeof(dev_cmd_regs)) {
        simdev_error("devcmd_rd: invalid offset 0x%"PRIx64" size 0x%x\n",
                     offset, size);
        return -1;
    }

    accelparams_t *ep = current_sd->priv;
    simdev_write_mem(ep->devcmd_pa + offset, &val, size);

    return 0;
}

static int
bar_devcmddb_rd(int bar, int reg, 
                u_int64_t offset, u_int8_t size, u_int64_t *valp)
{
    /* devcmddb is write-only */
    return bar_invalid_rd(bar, reg, offset, size, valp);
}

static int
bar_devcmddb_wr(int bar, int reg, 
                u_int64_t offset, u_int8_t size, u_int64_t val)
{
    if (size != 4) {
        simdev_error("doorbell: write size %d != 4, ignoring\n", size);
        return -1;
    }
    if (offset != 0) {
        simdev_error("doorbell: write offset 0x%x, ignoring\n", size);
        return -1;
    }
    if (val != 1) {
        simdev_error("doorbell: write data 0x%"PRIx64", ignoring\n", val);
        return -1;
    }

    accelparams_t *ep = current_sd->priv;
    simdev_write_mem(ep->devcmddb_pa, &val, size);

    return 0;
}

static int
bar_intrctrl_rd(int bar, int reg,
                u_int64_t offset, u_int8_t size, u_int64_t *valp)
{
    u_int32_t idx = accel_intrb(current_sd);
    u_int64_t base = intr_drvcfg_addr(idx);
    u_int32_t val;

    simdev_log("intrctrl read offset 0x%"PRIx64"\n", offset);
    if (size != 4) {
        simdev_error("intrctrl read size %d != 4, ignoring\n", size);
        return -1;
    }
    if (simdev_read_reg(base + offset, &val) < 0) {
        return -1;
    }
    *valp = val;
    return 0;
}

static int
bar_intrctrl_wr(int bar, int reg,
                u_int64_t offset, u_int8_t size, u_int64_t val)
{
    u_int32_t idx = accel_intrb(current_sd);
    u_int64_t base = intr_drvcfg_addr(idx);

    simdev_log("intrctrl write offset 0x%"PRIx64"\n", offset);
    if (size != 4) {
        simdev_error("intrctrl write size %d != 4, ignoring\n", size);
        return -1;
    }
    if (simdev_write_reg(base + offset, val) < 0) {
        return -1;
    }
    return 0;
}

static int
bar_intrstatus_rd(int bar, int reg,
                  u_int64_t offset, u_int8_t size, u_int64_t *valp)
{
    u_int32_t idx = accel_intrb(current_sd);
    u_int64_t base = intr_pba_addr(idx);
    u_int32_t val;

    simdev_log("intrstatus read offset 0x%"PRIx64"\n", offset);
    if (size != 4 && size != 8) {
        simdev_error("intrctrl read size %d invalid, ignoring\n", size);
        return -1;
    }
    if (size == 8) { /* XXX */
        simdev_error("intrctrl read size %d unimplemented\n", size);
        assert(0);
    }
    if (simdev_read_reg(base + offset, &val) < 0) {
        return -1;
    }
    *valp = val;
    return 0;
}

static int
bar_intrstatus_wr(int bar, int reg,
                  u_int64_t offset, u_int8_t size, u_int64_t val)
{
    /* intrstatus reg is read-only */
    return bar_invalid_wr(bar, reg, offset, size, val);
}

static int
bar_msixtbl_rd(int bar, int reg,
               u_int64_t offset, u_int8_t size, u_int64_t *valp)
{
    u_int32_t idx = accel_intrb(current_sd);
    u_int64_t base = intr_msixcfg_addr(idx);
    u_int32_t val;

    simdev_log("msixtbl read offset 0x%"PRIx64"\n", offset);
    if (size != 4 && size != 8) {
        simdev_error("msixtbl read size %d invalid, ignoring\n", size);
        return -1;
    }
    if (size == 8) { /* XXX */
        simdev_error("msixtbl read size %d unimplemented\n", size);
        assert(0);
    }
    if (simdev_read_reg(base + offset, &val) < 0) {
        return -1;
    }
    *valp = val;
    return 0;
}

static int
bar_msixtbl_wr(int bar, int reg,
               u_int64_t offset, u_int8_t size, u_int64_t val)
{
    u_int32_t idx = accel_intrb(current_sd);
    u_int64_t base = intr_msixcfg_addr(idx);

    simdev_log("msixtbl write offset 0x%"PRIx64"\n", offset);
    if (size != 4 && size != 8) {
        simdev_error("msixtbl write size %d invalid, ignoring\n", size);
        return -1;
    }
    if (size == 8) { /* XXX */
        simdev_error("msixtbl write size %d unimplemented\n", size);
        assert(0);
    }
    if (simdev_write_reg(base + offset, val) < 0) {
        return -1;
    }
    return 0;
}

static int
bar_msixpba_rd(int bar, int reg,
               u_int64_t offset, u_int8_t size, u_int64_t *valp)
{
    u_int32_t idx = accel_lif(current_sd);
    u_int64_t base = intr_pba_addr(idx);
    u_int32_t val;

    simdev_log("msixpba read offset 0x%"PRIx64" pba_base 0x%"PRIx64"\n",
               offset, base);
    if (size != 4 && size != 8) {
        simdev_error("msixpba read size %d invalid, ignoring\n", size);
        return -1;
    }
    if (size == 8) { /* XXX */
        simdev_error("msixpba read size %d unimplemented\n", size);
        assert(0);
    }
    if (simdev_read_reg(base + offset, &val) < 0) {
        return -1;
    }
    *valp = val;
    return 0;
}

static int
bar_msixpba_wr(int bar, int reg,
               u_int64_t offset, u_int8_t size, u_int64_t val)
{
    /* msixpba is read-only */
    return bar_invalid_wr(bar, reg, offset, size, val);
}

static int
bar_db_rd(int bar, int reg,
          u_int64_t offset, u_int8_t size, u_int64_t *valp)
{
    simdev_error("doorbell read: undefined!\n");
    return -1;
}

static int
bar_db_wr(int bar, int reg,
          u_int64_t offset, u_int8_t size, u_int64_t val)
{
    simdev_t *sd = current_sd;
    accelparams_t *ep = sd->priv;
    u_int32_t idx = accel_lif(sd);
    u_int64_t base = db_host_addr(idx);
    struct doorbell {
        u_int16_t p_index;
        u_int8_t ring:3;
        u_int8_t rsvd:5;
        u_int8_t qid_lo;
        u_int16_t qid_hi;
        u_int16_t rsvd2;
    } PACKED db;
    u_int32_t qid;
    u_int8_t qtype, upd;
    u_int32_t pid = offset >> 12;
    
    if (size != 8) {
        simdev_error("doorbell: write size %d != 8, ignoring\n", size);
        return -1;
    }
    if (((offset & (8-1)) != 0) ||
        (offset & 0xFFF) >= sizeof(db) * 8) {
        simdev_error("doorbell: write offset 0x%"PRIx64", ignoring\n", offset);
        return -1;
    }

    *(u_int64_t *)&db = val;
    qid = (db.qid_hi << 8) | db.qid_lo;

    /* set UPD bits on doorbell based on qtype */
    qtype = (offset >> 3) & 0x7;
    upd = ep->upd[qtype];
    offset = (offset & 0xFFF)|(upd << 17);

    simdev_log("doorbell: offset %lx upd 0x%x pid %d qtype %d "
               "qid %d ring %d index %d\n",
               offset, upd, pid, qtype, qid, db.ring, db.p_index);
    simdev_doorbell(base + offset, val);
    return 0;
}

static int
bar4_rd(int bar, int reg,
        u_int64_t offset, u_int8_t size, u_int64_t *valp)
{
    const u_int64_t cmb_base = accel_cmb_base(current_sd);
    const u_int64_t addr = cmb_base + offset;

    *valp = 0;
    return simdev_read_mem(addr, valp, size);
}

static int
bar4_wr(int bar, int reg,
        u_int64_t offset, u_int8_t size, u_int64_t val)
{
    const u_int64_t cmb_base = accel_cmb_base(current_sd);
    const u_int64_t addr = cmb_base + offset;

    return simdev_write_mem(addr, &val, size);
}

#define NREGS_PER_BAR   16
#define NBARS           6

typedef struct barreg_handler_s {
    int (*rd)(int bar, int reg,
              u_int64_t offset, u_int8_t size, u_int64_t *valp);
    int (*wr)(int bar, int reg,
              u_int64_t offset, u_int8_t size, u_int64_t val);
} barreg_handler_t;

static barreg_handler_t invalid_reg = { bar_invalid_rd, bar_invalid_wr };
static barreg_handler_t devcmd_reg = { bar_devcmd_rd, bar_devcmd_wr };
static barreg_handler_t devcmddb_reg = { bar_devcmddb_rd, bar_devcmddb_wr };
static barreg_handler_t intrstatus_reg = { bar_intrstatus_rd,
                                           bar_intrstatus_wr };
static barreg_handler_t intrctrl_reg = { bar_intrctrl_rd, bar_intrctrl_wr };
static barreg_handler_t msixtbl_reg = { bar_msixtbl_rd, bar_msixtbl_wr };
static barreg_handler_t msixpba_reg = { bar_msixpba_rd, bar_msixpba_wr };
static barreg_handler_t db_reg = { bar_db_rd, bar_db_wr };
static barreg_handler_t bar4_reg = { bar4_rd, bar4_wr };

typedef struct bar_handler_s {
    u_int32_t regsz;
    barreg_handler_t *regs[NREGS_PER_BAR];
} bar_handler_t;

static bar_handler_t invalid_bar = {
    .regs = {
        &invalid_reg,
    },
};

static bar_handler_t bar0_handler = {
    .regsz = 4096,
    .regs = {
        &devcmd_reg,
        &devcmddb_reg,
        &intrstatus_reg,
        &intrctrl_reg,
        &invalid_reg,
        &invalid_reg,
        &msixtbl_reg,
        &msixpba_reg,
    },
};

static bar_handler_t bar2_handler = {
    .regsz = 0,
    .regs = {
        &db_reg,
    },
};

static bar_handler_t bar4_handler = {
    .regsz = 0,
    .regs = {
        &bar4_reg,
    },
};

static bar_handler_t *bar_handlers[NBARS] = {
    &bar0_handler,
    &invalid_bar,
    &bar2_handler,
    &invalid_bar,
    &bar4_handler,
};

static bar_handler_t *
bar_handler(int bar)
{
    return (bar < NBARS) ? bar_handlers[bar] : NULL;
}

static int
barreg(bar_handler_t *b, u_int64_t offset)
{
    return b->regsz ? offset / b->regsz : 0;
}

static int
barreg_offset(bar_handler_t *b, u_int64_t offset)
{
    return offset - (barreg(b, offset) * b->regsz);
}

static barreg_handler_t *
barreg_handler(bar_handler_t *b, int reg)
{
    return (b && reg < NREGS_PER_BAR) ? b->regs[reg] : NULL;
}

static int
bar_rd(int bar, u_int64_t offset, u_int8_t size, u_int64_t *valp)
{
    bar_handler_t *b;
    barreg_handler_t *breg;
    u_int64_t regoff;
    int reg;

    b = bar_handler(bar);
    if (b == NULL) {
        simdev_error("bar_rd: unhandled bar %d\n", bar);
        return -1;
    }

    reg = barreg(b, offset);
    regoff = barreg_offset(b, offset);
    breg = barreg_handler(b, reg);
    if (breg == NULL || breg->rd == NULL) {
        simdev_error("bar_rd: unhandled reg %d\n", reg);
        return -1;
    }

    return breg->rd(bar, reg, regoff, size, valp);
}

static int
bar_wr(int bar, u_int64_t offset, u_int8_t size, u_int64_t val)
{
    bar_handler_t *b;
    barreg_handler_t *breg;
    u_int64_t regoff;
    int reg;

    b = bar_handler(bar);
    if (b == NULL) {
        simdev_error("bar_rd: unhandled bar %d\n", bar);
        return -1;
    }

    reg = barreg(b, offset);
    regoff = barreg_offset(b, offset);
    breg = barreg_handler(b, reg);
    if (breg == NULL || breg->wr == NULL) {
        simdev_error("bar_wr: unhandled reg %d\n", reg);
        return -1;
    }

    return breg->wr(bar, reg, regoff, size, val);
}

/*
 * ================================================================
 * process messages
 * ----------------------------------------------------------------
 */

static int
accel_memrd(simdev_t *sd, simmsg_t *m, u_int64_t *valp)
{
    const u_int16_t bdf  = m->u.read.bdf;
    const u_int8_t  bar  = m->u.read.bar;
    const u_int64_t addr = m->u.read.addr;
    const u_int8_t  size = m->u.read.size;

    current_sd = sd;

    if (bar_rd(bar, addr, size, valp) < 0) {
        sims_readres(sd->fd, bdf, bar, addr, size, 0, EFAULT);
        return -1;
    }
    sims_readres(sd->fd, bdf, bar, addr, size, *valp, 0);
    return 0;
}

static void
accel_memwr(simdev_t *sd, simmsg_t *m)
{
    const u_int16_t bdf  = m->u.write.bdf;
    const u_int8_t  bar  = m->u.write.bar;
    const u_int64_t addr = m->u.write.addr;
    const u_int8_t  size = m->u.write.size;
    const u_int64_t val  = m->u.write.val;

    current_sd = sd;
    bar_wr(bar, addr, size, val);
    sims_writeres(sd->fd, bdf, bar, addr, size, 0);
}

static int
accel_iord(simdev_t *sd, simmsg_t *m, u_int64_t *valp)
{
    const u_int16_t bdf  = m->u.read.bdf;
    const u_int8_t  bar  = m->u.read.bar;
    const u_int64_t addr = m->u.read.addr;
    const u_int8_t  size = m->u.read.size;

    current_sd = sd;

    if (bar_rd(bar, addr, size, valp) < 0) {
        sims_readres(sd->fd, bdf, bar, addr, size, 0, EINVAL);
        return -1;
    }
    sims_readres(sd->fd, bdf, bar, addr, size, *valp, 0);
    return 0;
}

static void
accel_iowr(simdev_t *sd, simmsg_t *m)
{
    const u_int16_t bdf  = m->u.write.bdf;
    const int       bar  = m->u.write.bar;
    const u_int64_t addr = m->u.write.addr;
    const u_int8_t  size = m->u.write.size;
    const u_int64_t val  = m->u.write.val;

    current_sd = sd;

    bar_wr(bar, addr, size, val);
    sims_writeres(sd->fd, bdf, bar, addr, size, 0);
}

static int
accel_init(simdev_t *sd, const char *devparams)
{
    accelparams_t *ep;

    if (devparam_str(devparams, "help", NULL, 0) == 0) {
        simdev_error("accel params:\n"
                     "    lif=<lif>\n"
                     "    intr_base=<intr_base>\n"
                     "    cmb_base=<cmb_base>\n"
                     "    devcmd_pa=<devcmd_pa>\n"
                     "    devcmddb_pa=<devcmddb_pa>\n"
                     );
        return -1;
    }

    ep = calloc(1, sizeof(accelparams_t));
    if (ep == NULL) {
        simdev_error("accelparams alloc failed: no mem\n");
        return -1;
    }
    sd->priv = ep;

#define GET_PARAM(P, TYP) \
    devparam_##TYP(devparams, #P, &ep->P)

    GET_PARAM(lif, int);
    GET_PARAM(intr_base, int);
    GET_PARAM(cmb_base, u64);
    GET_PARAM(devcmd_pa, u64);
    GET_PARAM(devcmddb_pa, u64);

    enum storage_seq_qtype {
        STORAGE_SEQ_QTYPE_SQ        = 0,
        STORAGE_SEQ_QTYPE_UNUSED    = 1,
        STORAGE_SEQ_QTYPE_ADMIN     = 2,
        STORAGE_SEQ_QTYPE_MAX
    };

    ep->upd[STORAGE_SEQ_QTYPE_SQ] = 0xb;
    ep->upd[STORAGE_SEQ_QTYPE_ADMIN] = 0xb;

    simdev_set_lif(ep->lif);

    return 0;
}

static void
accel_free(simdev_t *sd)
{
    free(sd->priv);
    sd->priv = NULL;
}

dev_ops_t accel_ops = {
    .init  = accel_init,
    .free  = accel_free,
    .cfgrd = generic_cfgrd,
    .cfgwr = generic_cfgwr,
    .memrd = accel_memrd,
    .memwr = accel_memwr,
    .iord  = accel_iord,
    .iowr  = accel_iowr,
};
