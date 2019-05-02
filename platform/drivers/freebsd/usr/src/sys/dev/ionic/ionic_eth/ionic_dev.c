/*
 * Copyright (c) 2017-2019 Pensando Systems, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include "ionic_dev.h"
#include "ionic.h"

static void ionic_init_devinfo(struct ionic_dev *idev)
{
	idev->dev_info.asic_type = ioread8(&idev->dev_info_regs->asic_type);
	idev->dev_info.asic_rev = ioread8(&idev->dev_info_regs->asic_rev);

	memcpy_fromio(idev->dev_info.fw_version,
		      idev->dev_info_regs->fw_version,
		      IONIC_DEVINFO_FWVERS_BUFLEN);

	memcpy_fromio(idev->dev_info.serial_num,
		      idev->dev_info_regs->serial_num,
		      IONIC_DEVINFO_SERIAL_BUFLEN);

	idev->dev_info.fw_version[IONIC_DEVINFO_FWVERS_BUFLEN] = 0;
	idev->dev_info.serial_num[IONIC_DEVINFO_SERIAL_BUFLEN] = 0;
}

int ionic_dev_setup(struct ionic* ionic)
{
	struct ionic_dev_bar *bar = ionic->bars;
	unsigned int num_bars = ionic->num_bars;
	struct ionic_dev *idev = &ionic->idev;
	struct device *dev = ionic->dev;
	u32 sig;

	/*
	 * BAR0 resources
	 */

	if (num_bars < 1) {
		dev_info(dev, "No bars found, aborting\n");
		return -EFAULT;
	}

	if (bar->len < BAR0_SIZE) {
		dev_info(dev, "Resource bar size %lu too small, aborting\n",
			 bar->len);
		return -EFAULT;
	}

	idev->dev_info_regs = bar->vaddr + BAR0_DEV_INFO_REGS_OFFSET;
	idev->dev_cmd = bar->vaddr + BAR0_DEV_CMD_REGS_OFFSET;
	idev->intr_status = bar->vaddr + BAR0_INTR_STATUS_OFFSET;
	idev->intr_ctrl = bar->vaddr + BAR0_INTR_CTRL_OFFSET;

	sig = ioread32(&idev->dev_info_regs->signature);
	if (sig != IONIC_DEV_INFO_SIGNATURE) {
		dev_err(dev, "Incompatible firmware signature %x", sig);
		return -EFAULT;
	}

	ionic_init_devinfo(idev);

	dev_info(dev, "ASIC: %s rev: 0x%X serial num: %s fw_ver: %s\n",
		 ionic_dev_asic_name(idev->dev_info.asic_type),
		 idev->dev_info.asic_rev,
		 idev->dev_info.serial_num,
		 idev->dev_info.fw_version);

	/*
	 * BAR1 resources
	 */
	bar++;

	idev->db_pages = bar->vaddr;
	idev->phy_db_pages = bar->bus_addr;

	/* 
	 * BAR2 resources
	 */

	mutex_init(&idev->cmb_inuse_lock);

	bar++;
	if (num_bars < 3) {
		idev->phy_cmb_pages = 0;
		idev->cmb_npages = 0;
		idev->cmb_inuse = NULL;
		return 0;
	}

	idev->phy_cmb_pages = bar->bus_addr;
	idev->cmb_npages = bar->len / PAGE_SIZE;
	idev->cmb_inuse = malloc(BITS_TO_LONGS(idev->cmb_npages) * sizeof(long),
				  M_IONIC, M_WAITOK | M_ZERO);
	if (!idev->cmb_inuse) {
		idev->phy_cmb_pages = 0;
		idev->cmb_npages = 0;
	}

	return 0;
}

/* Devcmd Interface */
u8 ionic_dev_cmd_status(struct ionic_dev *idev)
{
	return ioread8(&idev->dev_cmd->comp.status);
}

bool ionic_dev_cmd_done(struct ionic_dev *idev)
{
	return ioread32(&idev->dev_cmd->done) & DEV_CMD_DONE;
}

void ionic_dev_cmd_comp(struct ionic_dev *idev, void *mem)
{
	union dev_cmd_comp *comp = mem;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(comp->words); i++)
		comp->words[i] = ioread32(&idev->dev_cmd->comp.words[i]);
}

void ionic_dev_cmd_go(struct ionic_dev *idev, union dev_cmd *cmd)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(cmd->words); i++)
		iowrite32(cmd->words[i], &idev->dev_cmd->cmd.words[i]);
	iowrite32(0, &idev->dev_cmd->done);
	iowrite32(1, &idev->dev_cmd->doorbell);
}

/* Device commands */
void ionic_dev_cmd_identify(struct ionic_dev *idev, u16 ver)
{
	union dev_cmd cmd = {
		.identify.opcode = CMD_OPCODE_IDENTIFY,
		.identify.ver = ver,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_init(struct ionic_dev *idev)
{
	union dev_cmd cmd = {
		.init.opcode = CMD_OPCODE_INIT,
		.init.type = 0,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_reset(struct ionic_dev *idev)
{
	union dev_cmd cmd = {
		.reset.opcode = CMD_OPCODE_RESET,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

/* Port commands */
void ionic_dev_cmd_port_identify(struct ionic_dev *idev)
{
	union dev_cmd cmd = {
		.port_init.opcode = CMD_OPCODE_PORT_IDENTIFY,
		.port_init.index = 0,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_port_init(struct ionic_dev *idev)
{
	union dev_cmd cmd = {
		.port_init.opcode = CMD_OPCODE_PORT_INIT,
		.port_init.index = 0,
		.port_init.info_pa = idev->port_info_pa,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_port_reset(struct ionic_dev *idev)
{
	union dev_cmd cmd = {
		.port_reset.opcode = CMD_OPCODE_PORT_RESET,
		.port_reset.index = 0,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_port_state(struct ionic_dev *idev, uint8_t state)
{
	union dev_cmd cmd = {
		.port_setattr.opcode = CMD_OPCODE_PORT_SETATTR,
		.port_setattr.index = 0,
		.port_setattr.attr = IONIC_PORT_ATTR_STATE,
		.port_setattr.state = state,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_port_speed(struct ionic_dev *idev, uint32_t speed)
{
	union dev_cmd cmd = {
		.port_setattr.opcode = CMD_OPCODE_PORT_SETATTR,
		.port_setattr.index = 0,
		.port_setattr.attr = IONIC_PORT_ATTR_SPEED,
		.port_setattr.speed = speed,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_port_mtu(struct ionic_dev *idev, uint32_t mtu)
{
	union dev_cmd cmd = {
		.port_setattr.opcode = CMD_OPCODE_PORT_SETATTR,
		.port_setattr.index = 0,
		.port_setattr.attr = IONIC_PORT_ATTR_MTU,
		.port_setattr.mtu = mtu,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_port_autoneg(struct ionic_dev *idev, uint8_t an_enable)
{
	union dev_cmd cmd = {
		.port_setattr.opcode = CMD_OPCODE_PORT_SETATTR,
		.port_setattr.index = 0,
		.port_setattr.attr = IONIC_PORT_ATTR_AUTONEG,
		.port_setattr.an_enable = an_enable,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_port_fec(struct ionic_dev *idev, uint8_t fec_type)
{
	union dev_cmd cmd = {
		.port_setattr.opcode = CMD_OPCODE_PORT_SETATTR,
		.port_setattr.index = 0,
		.port_setattr.attr = IONIC_PORT_ATTR_FEC,
		.port_setattr.fec_type = fec_type,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_port_pause(struct ionic_dev *idev, uint8_t pause_type)
{
	union dev_cmd cmd = {
		.port_setattr.opcode = CMD_OPCODE_PORT_SETATTR,
		.port_setattr.index = 0,
		.port_setattr.attr = IONIC_PORT_ATTR_PAUSE,
		.port_setattr.pause_type = pause_type,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_port_loopback(struct ionic_dev *idev, uint8_t loopback_mode)
{
	union dev_cmd cmd = {
		.port_setattr.opcode = CMD_OPCODE_PORT_SETATTR,
		.port_setattr.index = 0,
		.port_setattr.attr = IONIC_PORT_ATTR_LOOPBACK,
		.port_setattr.loopback_mode = loopback_mode,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

/* LIF commands */
void ionic_dev_cmd_lif_identify(struct ionic_dev *idev, u8 type, u8 ver)
{
	union dev_cmd cmd = {
		.lif_identify.opcode = CMD_OPCODE_LIF_IDENTIFY,
		.lif_identify.type = type,
		.lif_identify.ver = ver,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_lif_init(struct ionic_dev *idev, u32 index, dma_addr_t addr)
{
	union dev_cmd cmd = {
		.lif_init.opcode = CMD_OPCODE_LIF_INIT,
		.lif_init.index = index,
		.lif_init.info_pa = addr,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_lif_reset(struct ionic_dev *idev, u32 index)
{
	union dev_cmd cmd = {
		.lif_init.opcode = CMD_OPCODE_LIF_RESET,
		.lif_init.index = index,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

char *ionic_dev_asic_name(u8 asic_type)
{
	switch (asic_type) {
	case ASIC_TYPE_CAPRI:
		return "Capri";
	default:
		return "Unknown";
	}
}

int ionic_db_page_num(struct ionic *ionic, int lif_id, int pid)
{
	return lif_id * ionic->ident.dev.ndbpgs_per_lif + pid;
}

int ionic_intr_init(struct ionic_dev *idev, struct intr *intr,
		    unsigned long index)
{
	intr->index = index;
	intr->ctrl = idev->intr_ctrl + index;

	return 0;
}

void ionic_intr_mask_on_assertion(struct intr *intr)
{
	struct intr_ctrl ctrl = {
		.mask_on_assert = 1,
	};

	KASSERT(intr->ctrl, ("%s intr->ctrl is NULL", intr->name));
	iowrite32(*(u32 *)intr_to_mask_on_assert(&ctrl),
		  intr_to_mask_on_assert(intr->ctrl));
}

void ionic_intr_return_credits(struct intr *intr, unsigned int credits,
			       bool unmask, bool reset_timer)
{
	struct intr_ctrl ctrl = {
		.int_credits = credits,
	};

	ctrl.flags |= unmask ? INTR_F_UNMASK : 0;
	ctrl.flags |= reset_timer ? INTR_F_TIMER_RESET : 0;

	KASSERT(intr->ctrl, ("%s intr->ctrl is NULL", intr->name));
	iowrite32(*(u32 *)intr_to_credits(&ctrl),
		  intr_to_credits(intr->ctrl));
}

void ionic_intr_mask(struct intr *intr, bool mask)
{
	struct intr_ctrl ctrl = {
		.mask = mask ? 1 : 0,
	};

	iowrite32(*(u32 *)intr_to_mask(&ctrl),
		  intr_to_mask(intr->ctrl));
	(void)ioread32(intr_to_mask(intr->ctrl)); /* flush write */
}

void ionic_intr_coal_set(struct intr *intr, u32 intr_coal)
{
	struct intr_ctrl ctrl = {
		.coalescing_init = intr_coal > INTR_CTRL_COAL_MAX ?
			INTR_CTRL_COAL_MAX : intr_coal,
	};

	KASSERT(intr->ctrl, ("%s intr->ctrl is NULL", intr->name));

	iowrite32(*(u32 *)intr_to_coal(&ctrl), intr_to_coal(intr->ctrl));
	(void)ioread32(intr_to_coal(intr->ctrl)); /* flush write */
}

void ionic_ring_doorbell(struct doorbell *db_addr, uint32_t qid, uint16_t p_index)
{
	struct doorbell db_data = {
		.qid_lo = qid,
		.qid_hi = qid >> 8,
		.ring = 0,
		.p_index = p_index,
	};

	writeq(*(u64 *)&db_data, db_addr);
}

int ionic_desc_avail(int ndescs, int head, int tail) 
{
	int avail = tail;

	if (head >= tail)
		avail += ndescs - head - 1;
	else
		avail -= head + 1;

	return avail;
}
