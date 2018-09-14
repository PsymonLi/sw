/*
 * Copyright 2017-2018 Pensando Systems, Inc.  All rights reserved.
 *
 * This program is free software; you may redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/utsname.h>

#include "ionic.h"
#include "ionic_bus.h"
#include "ionic_lif.h"
#include "ionic_debugfs.h"

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR("Scott Feldman <sfeldma@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

unsigned int ntxq_descs = 1024;
unsigned int nrxq_descs = 1024;
module_param(ntxq_descs, uint, 0);
module_param(nrxq_descs, uint, 0);
MODULE_PARM_DESC(ntxq_descs, "Descriptors per Tx queue, must be power of 2");
MODULE_PARM_DESC(nrxq_descs, "Descriptors per Rx queue, must be power of 2");

unsigned int ntxqs = 0;
unsigned int nrxqs = 0;
module_param(ntxqs, uint, 0);
module_param(nrxqs, uint, 0);
MODULE_PARM_DESC(ntxqs, "Hard set the number of Tx queues per LIF");
MODULE_PARM_DESC(nrxqs, "Hard set the number of Rx queues per LIF");

unsigned int devcmd_timeout = 30;
module_param(devcmd_timeout, uint, 0);
MODULE_PARM_DESC(devcmd_timeout, "Devcmd timeout in seconds (default 30 secs)");

int ionic_adminq_check_err(struct lif *lif, struct ionic_admin_ctx *ctx)
{
	struct net_device *netdev = lif->netdev;
	static struct cmds {
		unsigned int cmd;
		char *name;
	} cmds[] = {
		{ CMD_OPCODE_TXQ_INIT, "CMD_OPCODE_TXQ_INIT" },
		{ CMD_OPCODE_RXQ_INIT, "CMD_OPCODE_RXQ_INIT" },
		{ CMD_OPCODE_FEATURES, "CMD_OPCODE_FEATURES" },
		{ CMD_OPCODE_Q_ENABLE, "CMD_OPCODE_Q_ENABLE" },
		{ CMD_OPCODE_Q_DISABLE, "CMD_OPCODE_Q_DISABLE" },
		{ CMD_OPCODE_STATION_MAC_ADDR_GET,
			"CMD_OPCODE_STATION_MAC_ADDR_GET" },
		{ CMD_OPCODE_MTU_SET, "CMD_OPCODE_MTU_SET" },
		{ CMD_OPCODE_RX_MODE_SET, "CMD_OPCODE_RX_MODE_SET" },
		{ CMD_OPCODE_RX_FILTER_ADD, "CMD_OPCODE_RX_FILTER_ADD" },
		{ CMD_OPCODE_RX_FILTER_DEL, "CMD_OPCODE_RX_FILTER_DEL" },
		{ CMD_OPCODE_RSS_HASH_SET, "CMD_OPCODE_RSS_HASH_SET" },
		{ CMD_OPCODE_RSS_INDIR_SET, "CMD_OPCODE_RSS_INDIR_SET" },
		{ CMD_OPCODE_STATS_DUMP_START, "CMD_OPCODE_STATS_DUMP_START" },
		{ CMD_OPCODE_STATS_DUMP_STOP, "CMD_OPCODE_STATS_DUMP_STOP" },
		{ 0, 0 }, /* keep last */
	};
	struct cmds *cmd = cmds;
	char *name = "UNKNOWN";

	if (ctx->comp.comp.status) {
		while ((++cmd)->cmd)
			if (cmd->cmd == ctx->cmd.cmd.opcode)
				name = cmd->name;
		netdev_err(netdev, "(%d) %s failed: %d\n", ctx->cmd.cmd.opcode,
			   name, ctx->comp.comp.status);
		return -EIO;
	}

	return 0;
}

int ionic_adminq_post_wait(struct lif *lif, struct ionic_admin_ctx *ctx)
{
	int err;

	err = ionic_api_adminq_post(lif, ctx);
	if (err)
		return err;

	wait_for_completion(&ctx->work);

	return ionic_adminq_check_err(lif, ctx);
}

int ionic_napi(struct napi_struct *napi, int budget, ionic_cq_cb cb,
	       void *cb_arg)
{
	struct cq *cq = napi_to_cq(napi);
	unsigned int work_done;

	work_done = ionic_cq_service(cq, budget, cb, cb_arg);

	if (work_done > 0)
		ionic_intr_return_credits(cq->bound_intr, work_done, 0, true);

	if ((work_done < budget) && napi_complete_done(napi, work_done))
		ionic_intr_mask(cq->bound_intr, false);

	return work_done;
}

static int ionic_dev_cmd_wait(struct ionic_dev *idev, unsigned long max_wait)
{
	unsigned long time;
	signed long wait;
	int done;

	WARN_ON(in_interrupt());

	/* Wait for dev cmd to complete...but no more than max_wait
	 */

	time = jiffies + max_wait;
	do {

		done = ionic_dev_cmd_done(idev);
#ifdef HAPS
		if (done)
			printk(KERN_ERR "DEVCMD done took %ld secs (%ld jiffies)\n",
			       (jiffies + max_wait - time)/HZ, jiffies + max_wait - time);
#endif
		if (done)
			return 0;

		wait = schedule_timeout_interruptible(HZ / 10);
		if (wait > 0)
			return -EINTR;

	} while (time_after(time, jiffies));

#ifdef HAPS
	printk(KERN_ERR "DEVCMD timeout after %ld secs\n", max_wait/HZ);
#endif
	return -ETIMEDOUT;
}

static int ionic_dev_cmd_check_error(struct ionic_dev *idev)
{
	u8 status;

	status = ionic_dev_cmd_status(idev);
	switch (status) {
	case 0:
		return 0;
	}

	return -EIO;
}

int ionic_dev_cmd_wait_check(struct ionic_dev *idev, unsigned long max_wait)
{
	int err;

	err = ionic_dev_cmd_wait(idev, max_wait);
	if (err)
		return err;
	return ionic_dev_cmd_check_error(idev);
}

int ionic_set_dma_mask(struct ionic *ionic)
{
	struct device *dev = ionic->dev;
	int err;

	/* Query system for DMA addressing limitation for the device.
	 */

	err = dma_set_mask(dev, DMA_BIT_MASK(64));
	if (err) {
		dev_err(dev, "No usable 64-bit DMA configuration, aborting\n");
		return err;
	}

	err = dma_set_coherent_mask(dev, DMA_BIT_MASK(64));
	if (err)
		dev_err(dev, "Unable to obtain 64-bit DMA "
			"for consistent allocations, aborting\n");

	return err;
}

#ifndef ADMINQ
#define XXX_DEVCMD_HALF_PAGE 0x800

// XXX temp func to get side-band data from 2nd half page of dev_cmd reg space.
static int SBD_get(struct ionic_dev *idev, void *dst, size_t len)
{
	u32 __iomem *page32 = (void __iomem *)idev->dev_cmd;
	u32 *dst32 = dst;
	unsigned int i, count;

	// check pointer and size alignment
	if ((unsigned long)dst & 0x3 || len & 0x3)
		return -EINVAL;

	// check length fits in 2nd half of page
	if (len > XXX_DEVCMD_HALF_PAGE)
		return -EINVAL;

	page32 += XXX_DEVCMD_HALF_PAGE / sizeof(*page32);
	count = len / sizeof(*page32);

	for (i = 0; i < count; ++i)
		dst32[i] = ioread32(&page32[i]);

	return 0;
}

// XXX temp func to put side-band data into 2nd half page of dev_cmd reg space.
static int SBD_put(struct ionic_dev *idev, void *src, size_t len)
{
	u32 __iomem *page32 = (void __iomem *)idev->dev_cmd;
	u32 *src32 = src;
	unsigned int i, count;

	// check pointer and size alignment
	if ((unsigned long)src & 0x3 || len & 0x3)
		return -EINVAL;

	// check length fits in 2nd half of page
	if (len > XXX_DEVCMD_HALF_PAGE)
		return -EINVAL;

	page32 += XXX_DEVCMD_HALF_PAGE / sizeof(*page32);
	count = len / sizeof(*page32);

	for (i = 0; i < count; ++i)
		iowrite32(src32[i], &page32[i]);

	return 0;
}

void ionic_dev_cmd_work(struct work_struct *work)
{
	struct ionic *ionic = container_of(work, struct ionic, cmd_work);
	struct ionic_admin_ctx *ctx;
	unsigned long irqflags;
	int err = 0;

	spin_lock_irqsave(&ionic->cmd_lock, irqflags);
	if (list_empty(&ionic->cmd_list)) {
		spin_unlock_irqrestore(&ionic->cmd_lock, irqflags);
		return;
	}

	ctx = list_first_entry(&ionic->cmd_list,
			       struct ionic_admin_ctx, list);
	list_del(&ctx->list);
	spin_unlock_irqrestore(&ionic->cmd_lock, irqflags);

	dev_dbg(ionic->dev, "post admin dev command:\n");
	print_hex_dump_debug("cmd ", DUMP_PREFIX_OFFSET, 16, 1,
			     &ctx->cmd, sizeof(ctx->cmd), true);

	if (ctx->side_data) {
		print_hex_dump_debug("data ", DUMP_PREFIX_OFFSET, 16, 1,
				 ctx->side_data, ctx->side_data_len, true);

		err = SBD_put(&ionic->idev, ctx->side_data, ctx->side_data_len);
		if (err)
			goto err_out;
	}

	ionic_dev_cmd_go(&ionic->idev, (void *)&ctx->cmd);

	err = ionic_dev_cmd_wait_check(&ionic->idev, HZ * devcmd_timeout);
	if (err)
		goto err_out;
	
	ionic_dev_cmd_comp(&ionic->idev, &ctx->comp);

	if (ctx->side_data) {
		err = SBD_get(&ionic->idev, ctx->side_data, ctx->side_data_len);
		if (err)
			goto err_out;
	}

	dev_dbg(ionic->dev, "comp admin dev command:\n");
	print_hex_dump_debug("comp ", DUMP_PREFIX_OFFSET, 16, 1,
			     &ctx->comp, sizeof(ctx->comp), true);

err_out:
	if (WARN_ON(err))
		memset(&ctx->comp, 0xAB, sizeof(ctx->comp));

	complete_all(&ctx->work);

	schedule_work(&ionic->cmd_work);
}
#endif

int ionic_setup(struct ionic *ionic)
{
	int err;

	err = ionic_dev_setup(&ionic->idev, ionic->bars, ionic->num_bars);
	if (err)
		return err;

#ifndef ADMINQ
	spin_lock_init(&ionic->cmd_lock);
	INIT_LIST_HEAD(&ionic->cmd_list);
	INIT_WORK(&ionic->cmd_work, ionic_dev_cmd_work);
#endif

	return ionic_debugfs_add_dev_cmd(ionic);
}

int ionic_identify(struct ionic *ionic)
{
	struct device *dev = ionic->dev;
	struct ionic_dev *idev = &ionic->idev;
	union identity *ident;
	dma_addr_t ident_pa;
	int err;
#ifdef HAPS
	unsigned int i;
#endif

	ident = devm_kzalloc(dev, sizeof(*ident), GFP_KERNEL | GFP_DMA);
	if (!ident)
		return -ENOMEM;
	ident_pa = dma_map_single(dev, ident, sizeof(*ident),
				  DMA_BIDIRECTIONAL);
	if (dma_mapping_error(dev, ident_pa))
		return -EIO;

	ident->drv.os_type = OS_TYPE_LINUX;
	ident->drv.os_dist = 0;
	strncpy(ident->drv.os_dist_str, utsname()->release,
		sizeof(ident->drv.os_dist_str) - 1);
	ident->drv.kernel_ver = LINUX_VERSION_CODE;
	strncpy(ident->drv.kernel_ver_str, utsname()->version,
		sizeof(ident->drv.kernel_ver_str) - 1);
	strncpy(ident->drv.driver_ver_str, DRV_VERSION,
		sizeof(ident->drv.driver_ver_str) - 1);

#ifdef HAPS
	for (i = 0; i < 512; i++)
		iowrite32(idev->ident->words[i], &ident->words[i]);
#endif

	ionic_dev_cmd_identify(idev, IDENTITY_VERSION_1, ident_pa);

	err = ionic_dev_cmd_wait_check(idev, HZ * devcmd_timeout);
	if (err)
		goto err_out_unmap;

#ifdef HAPS
	for (i = 0; i < 512; i++)
		ident->words[i] = ioread32(&idev->ident->words[i]);
#endif

	ionic->ident = ident;
	ionic->ident_pa = ident_pa;

	err = ionic_debugfs_add_ident(ionic);
	if (err)
		goto err_out_unmap;

	return 0;

err_out_unmap:
	dma_unmap_single(dev, ident_pa, sizeof(*ident), DMA_BIDIRECTIONAL);
	return err;
}

void ionic_forget_identity(struct ionic *ionic)
{
	dma_unmap_single(ionic->dev, ionic->ident_pa,
			 sizeof(*ionic->ident), DMA_BIDIRECTIONAL);
}

int ionic_reset(struct ionic *ionic)
{
	struct ionic_dev *idev = &ionic->idev;

	ionic_dev_cmd_reset(idev);
	return ionic_dev_cmd_wait_check(idev, HZ * devcmd_timeout);
}

static int __init ionic_init_module(void)
{
	ionic_struct_size_checks();
	ionic_debugfs_create();
	pr_info("%s, ver %s\n", DRV_DESCRIPTION, DRV_VERSION);
	return ionic_bus_register_driver();
}

static void __exit ionic_cleanup_module(void)
{
	ionic_bus_unregister_driver();
	ionic_debugfs_destroy();
}

module_init(ionic_init_module);
module_exit(ionic_cleanup_module);
