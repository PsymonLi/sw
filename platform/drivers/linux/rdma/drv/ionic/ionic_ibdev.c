/*
 * Copyright (c) 2018 Pensando Systems, Inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <net/addrconf.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_cache.h>
#include <rdma/ib_mad.h>
#include <rdma/ib_user_verbs.h>

#include "ionic_fw.h"
#include "ionic_ibdev.h"
#include "ionic_ibdebug.h"

MODULE_AUTHOR("Allen Hubbe <allenbh@pensando.io>");
MODULE_DESCRIPTION("Pensando RoCE HCA driver");
MODULE_LICENSE("Dual BSD/GPL");

#define DRIVER_NAME "ionic_rdma"
#define DRIVER_VERSION "0.2.0"
#define DRIVER_DESCRIPTION "Pensando RoCE HCA driver"
#define DEVICE_DESCRIPTION "Pensando RoCE HCA"

/* not a valid queue position or negative error status */
#define IONIC_ADMIN_POSTED 0x10000

/* cpu can be held with irq disabled for COUNT * MS  (for create/destoy_ah) */
#define IONIC_ADMIN_BUSY_RETRY_COUNT 2000
#define IONIC_ADMIN_BUSY_RETRY_MS 1

/* admin queue will be considered failed if a command takes longer */
#define IONIC_ADMIN_TIMEOUT (HZ * 2)

/* will poll for admin cq to tolerate and report from missed event */
#define IONIC_ADMIN_DELAY (HZ / 4)

/* resource is not reserved on the device, indicated in tbl_order */
#define IONIC_RES_INVALID -1

/* port state values for query_port */
#define PHYS_STATE_UP 5
#define PHYS_STATE_DOWN 3

#define ionic_set_ecn(tos) (((tos) | 2u) & ~1u)

/* XXX remove this section for release */
static bool ionic_xxx_pgtbl = true;
module_param_named(xxx_pgtbl, ionic_xxx_pgtbl, bool, 0644);
MODULE_PARM_DESC(xxx_pgtbl, "XXX Allocate pgtbl even for contiguous buffers that don't need it.");
static bool ionic_xxx_pgidx = true;
module_param_named(xxx_pgidx, ionic_xxx_pgidx, bool, 0444);
MODULE_PARM_DESC(xxx_pgidx, "XXX Tell device idx in eight byte blocks instead of cache line size blocks.");
static bool ionic_xxx_limits = false;
module_param_named(xxx_limits, ionic_xxx_limits, bool, 0444);
MODULE_PARM_DESC(xxx_limits, "XXX Hardcode resource limits.");
static bool ionic_xxx_kdbid = true;
module_param_named(xxx_kdbid, ionic_xxx_kdbid, bool, 0644);
MODULE_PARM_DESC(xxx_kdbid, "XXX Kernel doorbell id in user space.");
static bool ionic_xxx_udp = true;
module_param_named(xxx_udp, ionic_xxx_udp, bool, 0644);
MODULE_PARM_DESC(xxx_udp, "XXX Makeshift udp header in template.");
static bool ionic_xxx_noop = false;
module_param_named(xxx_noop, ionic_xxx_noop, bool, 0444);
MODULE_PARM_DESC(xxx_noop, "XXX Adminq noop after probing device.");
static bool ionic_xxx_aq_dbell = true;
module_param_named(xxx_aq_dbell, ionic_xxx_aq_dbell, bool, 0644);
MODULE_PARM_DESC(xxx_aq_dbell, "XXX Enable ringing aq doorbell (to test handling of aq failure).");
static int ionic_xxx_qid_skip = 512;
module_param_named(xxx_qid_skip, ionic_xxx_qid_skip, int, 0444);
MODULE_PARM_DESC(xxx_qid_skip, "XXX Skip every N'th qid");
static bool ionic_xxx_fake_stats = false;
module_param_named(xxx_fake_stats, ionic_xxx_fake_stats, bool, 0444);
MODULE_PARM_DESC(xxx_fake_stats, "XXX Present fake hardware stats");
/* XXX remove above section for release */

static bool ionic_dbgfs_enable = true; /* XXX false for release */
module_param_named(dbgfs, ionic_dbgfs_enable, bool, 0444);
MODULE_PARM_DESC(dbgfs, "Enable debugfs for this driver.");

static u16 ionic_aq_depth = 0x3f;
module_param_named(aq_depth, ionic_aq_depth, ushort, 0444);
MODULE_PARM_DESC(aq_depth, "Min depth for admin queues.");

static u16 ionic_eq_depth = 0x1ff;
module_param_named(eq_depth, ionic_eq_depth, ushort, 0444);
MODULE_PARM_DESC(eq_depth, "Min depth for event queues.");

static u16 ionic_eq_isr_budget = 10;
module_param_named(isr_budget, ionic_eq_isr_budget, ushort, 0644);
MODULE_PARM_DESC(isr_budget, "Max events to poll per round in isr context.");

static u16 ionic_eq_work_budget = 1000;
module_param_named(work_budget, ionic_eq_work_budget, ushort, 0644);
MODULE_PARM_DESC(work_budget, "Max events to poll per round in work context.");

static bool ionic_sqcmb_inline = true;
module_param_named(sqcmb_inline, ionic_sqcmb_inline, bool, 0644);
MODULE_PARM_DESC(sqcmb_inline, "Only alloc sq cmb for inline data capability.");

static int ionic_sqcmb_order = 0; /* XXX needs tuning */
module_param_named(sqcmb_order, ionic_sqcmb_order, int, 0644);
MODULE_PARM_DESC(sqcmb_order, "Only alloc sq cmb less than order.");

static bool ionic_rqcmb_sqcmb = true;
module_param_named(rqcmb_sqcmb, ionic_rqcmb_sqcmb, bool, 0644);
MODULE_PARM_DESC(rqcmb_sqcmb, "Only alloc rq cmb if sq is cmb.");

static int ionic_rqcmb_order = 0; /* XXX needs tuning */
module_param_named(rqcmb_order, ionic_rqcmb_order, int, 0644);
MODULE_PARM_DESC(rqcmb_order, "Only alloc rq cmb less than order.");

static int ionic_max_pd = 1024;
module_param_named(max_pd, ionic_max_pd, int, 0444);
MODULE_PARM_DESC(max_pd, "Max number of PDs.");

static int ionic_max_gid = 1024;
module_param_named(max_gid, ionic_max_gid, int, 0444);
MODULE_PARM_DESC(max_gid, "Max number of GIDs.");

/* work queue for handling network events, managing ib devices */
static struct workqueue_struct *ionic_dev_workq;

/* work queue for polling the event queue and admin cq */
static struct workqueue_struct *ionic_evt_workq;

/* access single-threaded thru ionic_dev_workq */
static LIST_HEAD(ionic_ibdev_list);

static void ionic_xxx_resid_skip(struct resid_bits *bits)
{
	int i = ionic_xxx_qid_skip - 1;

	while (i < bits->inuse_size) {
		set_bit(i, bits->inuse);
		i += ionic_xxx_qid_skip;
	}
}

static int ionic_validate_udata(struct ib_udata *udata,
				size_t inlen, size_t outlen)
{
	if (udata) {
		if (udata->inlen < inlen || udata->outlen < outlen) {
			pr_debug("have udata in %lu out %lu want %lu %lu\n",
				 udata->inlen, udata->outlen,
				 inlen, outlen);
			return -EINVAL;
		}
	} else {
		if (inlen != 0 || outlen != 0) {
			pr_debug("no udata want %lu %lu\n",
				 inlen, outlen);
			return -EINVAL;
		}
	}

	return 0;
}

static int ionic_validate_qdesc(struct ionic_qdesc *q)
{
	if (!q->addr || !q->size || !q->mask ||
	    !q->depth_log2 || !q->stride_log2)
		return -EINVAL;

	if (q->addr & (PAGE_SIZE - 1))
		return -EINVAL;

	if (q->mask != BIT(q->depth_log2) - 1)
		return -EINVAL;

	if (q->size < BIT_ULL(q->depth_log2 + q->stride_log2))
		return -EINVAL;

	return 0;
}

static int ionic_validate_qdesc_zero(struct ionic_qdesc *q)
{
	if (q->addr || q->size || q->mask || q->depth_log2 || q->stride_log2)
		return -EINVAL;

	return 0;
}

static int ionic_verbs_status_to_rc(u32 status)
{
	/* TODO */
	if (status)
		return -EIO;

	return 0;
}

static int ionic_get_pdid(struct ionic_ibdev *dev, u32 *pdid)
{
	int rc;

	mutex_lock(&dev->inuse_lock);
	rc = resid_get(&dev->inuse_pdid);
	mutex_unlock(&dev->inuse_lock);

	if (rc >= 0) {
		*pdid = rc;
		rc = 0;
	}

	return rc;
}

static int ionic_get_ahid(struct ionic_ibdev *dev, u32 *ahid)
{
	unsigned long irqflags;
	int rc;

	spin_lock_irqsave(&dev->inuse_splock, irqflags);
	rc = resid_get(&dev->inuse_ahid);
	spin_unlock_irqrestore(&dev->inuse_splock, irqflags);

	if (rc >= 0) {
		*ahid = rc;
		rc = 0;
	}

	return rc;
}

static int ionic_get_mrid(struct ionic_ibdev *dev, u32 *mrid)
{
	int rc;

	mutex_lock(&dev->inuse_lock);
	/* wrap to 1, skip reserved lkey */
	rc = resid_get_wrap(&dev->inuse_mrid, 1);
	if (rc >= 0) {
		*mrid = ionic_mrid(rc, dev->next_mrkey++);
		rc = 0;
	}
	mutex_unlock(&dev->inuse_lock);

	return rc;
}

static int ionic_get_cqid(struct ionic_ibdev *dev, u32 *cqid)
{
	int rc;

	mutex_lock(&dev->inuse_lock);
	rc = resid_get(&dev->inuse_cqid);
	mutex_unlock(&dev->inuse_lock);

	if (rc >= 0) {
		*cqid = rc + dev->cq_base;
		rc = 0;
	}

	return rc;
}

static int ionic_get_gsi_qpid(struct ionic_ibdev *dev, u32 *qpid)
{
	int rc = 0;

	mutex_lock(&dev->inuse_lock);
	if (test_bit(IB_QPT_GSI, dev->inuse_qpid.inuse)) {
		rc = -EINVAL;
	} else {
		set_bit(IB_QPT_GSI, dev->inuse_qpid.inuse);
		*qpid = IB_QPT_GSI;
	}
	mutex_unlock(&dev->inuse_lock);

	return rc;
}

static int ionic_get_qpid(struct ionic_ibdev *dev, u32 *qpid)
{
	int rc = 0;

	mutex_lock(&dev->inuse_lock);
	rc = resid_get_shared(&dev->inuse_qpid, 2,
			      dev->inuse_qpid.next_id,
			      dev->size_qpid);
	if (rc >= 0) {
		dev->inuse_qpid.next_id = rc + 1;
		*qpid = rc;
		rc = 0;
	}
	mutex_unlock(&dev->inuse_lock);

	return rc;
}

static int ionic_get_srqid(struct ionic_ibdev *dev, u32 *qpid)
{
	int rc = 0;

	mutex_lock(&dev->inuse_lock);
	rc = resid_get_shared(&dev->inuse_qpid,
			      dev->size_qpid,
			      dev->next_srqid,
			      dev->size_srqid);
	if (rc >= 0) {
		dev->next_srqid = rc + 1;
		*qpid = rc;
		rc = 0;
	} else {
		rc = resid_get_shared(&dev->inuse_qpid, 2,
				      dev->inuse_qpid.next_id,
				      dev->size_qpid);
		if (rc >= 0) {
			dev->inuse_qpid.next_id = rc + 1;
			*qpid = rc;
			rc = 0;
		}
	}
	mutex_unlock(&dev->inuse_lock);

	return rc;
}

static void ionic_put_pdid(struct ionic_ibdev *dev, u32 pdid)
{
	resid_put(&dev->inuse_pdid, pdid);
}

static void ionic_put_ahid(struct ionic_ibdev *dev, u32 ahid)
{
	resid_put(&dev->inuse_ahid, ahid);
}

static void ionic_put_mrid(struct ionic_ibdev *dev, u32 mrid)
{
	resid_put(&dev->inuse_mrid, ionic_mrid_index(mrid));
}

static void ionic_put_cqid(struct ionic_ibdev *dev, u32 cqid)
{
	resid_put(&dev->inuse_cqid, cqid - dev->cq_base);
}

static void ionic_put_qpid(struct ionic_ibdev *dev, u32 qpid)
{
	resid_put(&dev->inuse_qpid, qpid);
}

static void ionic_put_srqid(struct ionic_ibdev *dev, u32 qpid)
{
	resid_put(&dev->inuse_qpid, qpid);
}

static int ionic_res_order(int count, int stride, int cl_stride)
{
	/* count becomes log2 of size in bytes */
	count = order_base_2(count) + stride;

	/* zero if less than one cache line */
	if (count < cl_stride)
		return 0;

	return count - cl_stride;
}

static int ionic_get_res(struct ionic_ibdev *dev, struct ionic_tbl_res *res)
{
	int rc = 0;

	mutex_lock(&dev->inuse_lock);
	rc = buddy_get(&dev->inuse_restbl, res->tbl_order);
	mutex_unlock(&dev->inuse_lock);

	if (rc < 0) {
		res->tbl_order = IONIC_RES_INVALID;
		res->tbl_pos = 0;
		return rc;
	}

	res->tbl_pos = rc;

	if (ionic_xxx_pgidx)
		res->tbl_pos <<= dev->cl_stride - dev->pte_stride;

	return 0;
}

static bool ionic_put_res(struct ionic_ibdev *dev, struct ionic_tbl_res *res)
{
	if (res->tbl_order == IONIC_RES_INVALID)
		return false;

	if (ionic_xxx_pgidx)
		res->tbl_pos >>= dev->cl_stride - dev->pte_stride;

	mutex_lock(&dev->inuse_lock);
	buddy_put(&dev->inuse_restbl, res->tbl_pos, res->tbl_order);
	mutex_unlock(&dev->inuse_lock);

	res->tbl_order = IONIC_RES_INVALID;
	res->tbl_pos = 0;

	return true;
}

static int ionic_pgtbl_page(struct ionic_tbl_buf *buf, u64 dma)
{
	if (unlikely(buf->tbl_pages == buf->tbl_limit))
		return -ENOMEM;

	if (buf->tbl_buf)
		buf->tbl_buf[buf->tbl_pages] = cpu_to_le64(dma);
	else
		buf->tbl_dma = dma;

	++buf->tbl_pages;

	return 0;
}

static int ionic_pgtbl_umem(struct ionic_tbl_buf *buf, struct ib_umem *umem)
{
	struct scatterlist *sg;
	u64 page_size, page_dma;
	int rc = 0, sg_i, page_i, page_count, page_shift;

#ifdef HAVE_UMEM_PAGE_SHIFT
	page_shift = umem->page_shift;
	page_size = BIT_ULL(page_shift);
#else
	page_size = umem->page_size;
	page_shift = order_base_2(page_size);
#endif

	for_each_sg(umem->sg_head.sgl, sg, umem->nmap, sg_i) {
		page_dma = sg_dma_address(sg);
		page_count = sg_dma_len(sg) >> page_shift;

		for (page_i = 0; page_i < page_count; ++page_i) {
			rc = ionic_pgtbl_page(buf, page_dma);
			if (rc)
				goto out;

			page_dma += page_size;
		}
	}

	buf->page_size_log2 = page_shift;

out:
	return rc;
}

static int ionic_pgtbl_init(struct ionic_ibdev *dev, struct ionic_tbl_res *res,
			    struct ionic_tbl_buf *buf, struct ib_umem *umem,
			    dma_addr_t dma, int limit)
{
	int rc;

	if (umem)
		limit = ib_umem_num_pages(umem);

	if (limit < 1)
		return -EINVAL;

	res->tbl_order = ionic_res_order(limit, dev->pte_stride,
					 dev->cl_stride);
	res->tbl_pos = 0;

	buf->tbl_limit = limit;
	buf->tbl_pages = 0;
	buf->tbl_size = 0;
	buf->tbl_buf = NULL;
	buf->page_size_log2 = 0;

	/* skip pgtbl if contiguous / direct translation */
	if (ionic_xxx_pgtbl || limit > 1) {
		/* A reservation will be made for page table resources.
		 *
		 * The page table reservation must be large enough to account
		 * for alignment of the first page within the page table.  The
		 * requirement is therefore greater-than-page-size alignment.
		 * If not accounted for, the placement of pages in the page
		 * table could extend beyond the bounds of the reservation.
		 *
		 * To account for the alignment of the page in the reservation,
		 * extend the reservation based on the worst case first page
		 * alignment.  The worst case is required for alloc_mr, because
		 * only the number of pages is given, but the alignment is not
		 * known until registration.
		 *
		 * XXX: is pte alignment a temporary workaround, or permanent?
		 */
		if (likely(dev->cl_stride > dev->pte_stride)) {
			/* increase the limit to account for worst case */
			buf->tbl_limit =
				1 + ALIGN(limit - 1, BIT(dev->cl_stride -
							 dev->pte_stride));

			/* and recalculate the reservation dimensions */
			res->tbl_order = ionic_res_order(buf->tbl_limit,
							 dev->pte_stride,
							 dev->cl_stride);
		}

		rc = ionic_get_res(dev, res);
		if (rc)
			goto err_res;

		/* limit for pte reservation should not affect anything else */
		buf->tbl_limit = limit;
	} else {
		res->tbl_order = IONIC_RES_INVALID;
	}

	if (limit > 1) {
		buf->tbl_size = buf->tbl_limit * sizeof(*buf->tbl_buf);
		buf->tbl_buf = kmalloc(buf->tbl_size, GFP_KERNEL);
		if (!buf->tbl_buf) {
			rc = -ENOMEM;
			goto err_buf;
		}

		buf->tbl_dma = dma_map_single(dev->hwdev, buf->tbl_buf,
					      buf->tbl_size, DMA_FROM_DEVICE);
		rc = dma_mapping_error(dev->hwdev, buf->tbl_dma);
		if (rc)
			goto err_dma;
	}

	if (umem) {
		rc = ionic_pgtbl_umem(buf, umem);
		if (rc)
			goto err_umem;
	} else {
		rc = ionic_pgtbl_page(buf, dma);
		if (rc)
			goto err_umem;

		/* XXX want to use page_size=zero for phys-contiguous,
		 * until hal supports it, only up to 8MB contiguous */
		if (ionic_xxx_pgtbl)
			buf->page_size_log2 = 23;
	}

	return 0;

err_umem:
	if (buf->tbl_buf) {
		dma_unmap_single(dev->hwdev, buf->tbl_dma, buf->tbl_size,
				 DMA_FROM_DEVICE);
err_dma:
		kfree(buf->tbl_buf);
	}
err_buf:
	ionic_put_res(dev, res);
err_res:
	buf->tbl_buf = NULL;
	buf->tbl_limit = 0;
	buf->tbl_pages = 0;
	return rc;
}

static void ionic_pgtbl_unbuf(struct ionic_ibdev *dev,
			      struct ionic_tbl_buf *buf)
{
	if (buf->tbl_buf) {
		dma_unmap_single(dev->hwdev, buf->tbl_dma, buf->tbl_size,
				 DMA_FROM_DEVICE);
		kfree(buf->tbl_buf);
	}

	buf->tbl_buf = NULL;
	buf->tbl_limit = 0;
	buf->tbl_pages = 0;
}

static bool ionic_next_cqe(struct ionic_cq *cq, struct ionic_v1_cqe **cqe)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(cq->ibcq.device);
	struct ionic_v1_cqe *qcqe = ionic_queue_at_prod(&cq->q);

	if (unlikely(cq->color != ionic_v1_cqe_color(qcqe)))
		return false;

	rmb();

	dev_dbg(&dev->ibdev.dev, "poll cq %u prod %u\n", cq->cqid, cq->q.prod);
	print_hex_dump_debug("cqe ", DUMP_PREFIX_OFFSET, 16, 1,
			     qcqe, BIT(cq->q.stride_log2), true);

	*cqe = qcqe;

	return true;
}

static void ionic_clean_cq(struct ionic_cq *cq, u32 qpid)
{
	struct ionic_v1_cqe *qcqe;
	int prod, qtf, qid, type;
	bool color;

	if (!cq->q.ptr)
		return;

	color = cq->color;
	prod = cq->q.prod;
	qcqe = ionic_queue_at(&cq->q, prod);

	while (color == ionic_v1_cqe_color(qcqe)) {
		qtf = ionic_v1_cqe_qtf(qcqe);
		qid = ionic_v1_cqe_qtf_qid(qtf);
		type = ionic_v1_cqe_qtf_type(qtf);

		if (qid == qpid && type != IONIC_V1_CQE_TYPE_ADMIN)
			ionic_v1_cqe_clean(qcqe);

		prod = ionic_queue_next(&cq->q, prod);
		qcqe = ionic_queue_at(&cq->q, prod);
		color = ionic_color_wrap(prod, color);
	}
}

static void ionic_admin_timedout(struct ionic_ibdev *dev)
{
	struct ionic_aq *aq = dev->adminq;
	struct ionic_cq *cq = dev->admincq;
	unsigned long irqflags;
	u16 pos;

	spin_lock_irqsave(&dev->admin_lock, irqflags);
	if (ionic_queue_empty(&aq->q))
		goto out;

	queue_work(ionic_evt_workq, &dev->reset_work);

	dev_err(&dev->ibdev.dev, "admin command timed out\n");

	dev_warn(&dev->ibdev.dev, "admin timeout was set for %ums\n",
		 jiffies_to_msecs(IONIC_ADMIN_TIMEOUT));
	dev_warn(&dev->ibdev.dev, "admin inactivity for %ums\n",
		 jiffies_to_msecs(jiffies - dev->admin_stamp));

	dev_warn(&dev->ibdev.dev, "admin commands outstanding %u\n",
		 ionic_queue_length(&aq->q));
	dev_warn(&dev->ibdev.dev, "more commands pending? %s\n",
		 list_empty(&aq->wr_post) ? "no" : "yes");

	pos = cq->q.prod;

	dev_warn(&dev->ibdev.dev, "admin cq pos %u (next to complete)\n", pos);
	print_hex_dump(KERN_WARNING, "cqe ", DUMP_PREFIX_OFFSET, 16, 1,
		       ionic_queue_at(&cq->q, pos),
		       BIT(cq->q.stride_log2), true);

	pos = (pos - 1) & cq->q.mask;

	dev_warn(&dev->ibdev.dev, "admin cq pos %u (last completed)\n", pos);
	print_hex_dump(KERN_WARNING, "cqe ", DUMP_PREFIX_OFFSET, 16, 1,
		       ionic_queue_at(&cq->q, pos),
		       BIT(cq->q.stride_log2), true);

	pos = aq->q.cons;

	dev_warn(&dev->ibdev.dev, "admin pos %u (next to complete)\n", pos);
	print_hex_dump(KERN_WARNING, "cmd ", DUMP_PREFIX_OFFSET, 16, 1,
		       ionic_queue_at(&aq->q, pos),
		       BIT(aq->q.stride_log2), true);

	pos = (aq->q.prod - 1) & aq->q.mask;
	if (pos == aq->q.cons)
		goto out;

	dev_warn(&dev->ibdev.dev, "admin pos %u (last posted)\n", pos);
	print_hex_dump(KERN_WARNING, "cmd ", DUMP_PREFIX_OFFSET, 16, 1,
		       ionic_queue_at(&aq->q, pos),
		       BIT(aq->q.stride_log2), true);

out:
	spin_unlock_irqrestore(&dev->admin_lock, irqflags);
}

static void ionic_admin_reset_dwork(struct ionic_ibdev *dev)
{
	if (dev->admin_state < IONIC_ADMIN_KILLED)
		mod_delayed_work(ionic_evt_workq, &dev->admin_dwork,
				 IONIC_ADMIN_DELAY);
}

static void ionic_admin_reset_wdog(struct ionic_ibdev *dev)
{
	dev->admin_stamp = jiffies;
	ionic_admin_reset_dwork(dev);
}

static void ionic_admin_poll_locked(struct ionic_ibdev *dev)
{
	struct ionic_aq *aq = dev->adminq;
	struct ionic_cq *cq = dev->admincq;
	struct ionic_admin_wr *wr, *wr_next;
	struct ionic_v1_admin_wqe *wqe;
	struct ionic_v1_cqe *cqe;
	u32 qtf, qid;
	u8 type;
	u16 old_prod;

	if (dev->admin_state >= IONIC_ADMIN_KILLED) {
		list_for_each_entry_safe(wr, wr_next, &aq->wr_prod, aq_ent) {
			INIT_LIST_HEAD(&wr->aq_ent);
			aq->q_wr[wr->status] = NULL;
			wr->status = dev->admin_state;
			complete_all(&wr->work);
		}
		INIT_LIST_HEAD(&aq->wr_prod);

		list_for_each_entry_safe(wr, wr_next, &aq->wr_post, aq_ent) {
			INIT_LIST_HEAD(&wr->aq_ent);
			wr->status = dev->admin_state;
			complete_all(&wr->work);
		}
		INIT_LIST_HEAD(&aq->wr_post);

		return;
	}

	old_prod = cq->q.prod;

	while (ionic_next_cqe(cq, &cqe)) {
		qtf = ionic_v1_cqe_qtf(cqe);
		qid = ionic_v1_cqe_qtf_qid(qtf);
		type = ionic_v1_cqe_qtf_type(qtf);

		if (unlikely(type != IONIC_V1_CQE_TYPE_ADMIN)) {
			dev_warn_ratelimited(&dev->ibdev.dev,
					     "unexpected cqe type %u\n", type);
			goto cq_next;
		}

		if (unlikely(qid != aq->aqid)) {
			dev_warn_ratelimited(&dev->ibdev.dev,
					     "unexpected cqe qid %u\n", qid);
			goto cq_next;
		}

		if (unlikely(be16_to_cpu(cqe->admin.cmd_idx) != aq->q.cons)) {
			dev_warn_ratelimited(&dev->ibdev.dev,
					     "unexpected idx %u cons %u qid %u\n",
					     le16_to_cpu(cqe->admin.cmd_idx),
					     aq->q.cons, qid);
			goto cq_next;
		}

		if (unlikely(ionic_queue_empty(&aq->q))) {
			dev_warn_ratelimited(&dev->ibdev.dev,
					     "unexpected cqe for empty adminq\n");
			goto cq_next;
		}

		wr = aq->q_wr[aq->q.cons];
		if (wr) {
			aq->q_wr[aq->q.cons] = NULL;
			list_del_init(&wr->aq_ent);

			wr->cqe = *cqe;
			wr->status = dev->admin_state;
			complete_all(&wr->work);
		}

		ionic_queue_consume(&aq->q);

cq_next:
		ionic_queue_produce(&cq->q);
		cq->color = ionic_color_wrap(cq->q.prod, cq->color);
	}

	if (old_prod != cq->q.prod) {
		ionic_admin_reset_wdog(dev);
		ionic_dbell_ring(&dev->dbpage[dev->cq_qtype],
				 ionic_queue_dbell_val(&cq->q));
		queue_work(ionic_evt_workq, &dev->admin_work);
	} else if (!dev->admin_armed) {
		dev->admin_armed = true;
		cq->arm_any_prod = ionic_queue_next(&cq->q, cq->arm_any_prod);
		ionic_dbell_ring(&dev->dbpage[dev->cq_qtype],
				 cq->q.dbell | IONIC_DBELL_RING_ARM |
				 cq->arm_any_prod);
		queue_work(ionic_evt_workq, &dev->admin_work);
	}

	if (dev->admin_state != IONIC_ADMIN_ACTIVE)
		return;

	old_prod = aq->q.prod;

	if (ionic_queue_empty(&aq->q) && !list_empty(&aq->wr_post))
		ionic_admin_reset_wdog(dev);

	while (!ionic_queue_full(&aq->q) && !list_empty(&aq->wr_post)) {
		wqe = ionic_queue_at_prod(&aq->q);

		wr = list_first_entry(&aq->wr_post,
				      struct ionic_admin_wr, aq_ent);

		list_move(&wr->aq_ent, &aq->wr_prod);

		wr->status = aq->q.prod;
		aq->q_wr[aq->q.prod] = wr;

		*wqe = wr->wqe;

		dev_dbg(&dev->ibdev.dev, "post admin prod %u\n", aq->q.prod);
		print_hex_dump_debug("wqe ", DUMP_PREFIX_OFFSET, 16, 1,
				     ionic_queue_at_prod(&aq->q),
				     BIT(aq->q.stride_log2), true);

		ionic_queue_produce(&aq->q);
	}

	if (old_prod != aq->q.prod && ionic_xxx_aq_dbell)
		ionic_dbell_ring(&dev->dbpage[dev->aq_qtype],
				 ionic_queue_dbell_val(&aq->q));
}

static void ionic_admin_dwork(struct work_struct *ws)
{
	struct ionic_ibdev *dev =
		container_of(ws, struct ionic_ibdev, admin_dwork.work);
	struct ionic_aq *aq = dev->adminq;
	unsigned long irqflags;
	bool progress = false;
	u16 pos;

	/* see if polling the admin cq now makes some progress */
	spin_lock_irqsave(&dev->admin_lock, irqflags);
	if (ionic_queue_empty(&aq->q)) {
		progress = true;
	} else {
		pos = aq->q.cons;
		ionic_admin_poll_locked(dev);
		if (pos != aq->q.cons) {
			dev_dbg(&dev->ibdev.dev,
				"missed event for admin cq\n");
			progress = true;
		}
	}
	spin_unlock_irqrestore(&dev->admin_lock, irqflags);

	if (progress)
		return;

	dev_dbg(&dev->ibdev.dev, "no progress after %ums\n",
		jiffies_to_msecs(jiffies - dev->admin_stamp));

	/* if no progress was made, try to poll again later, until timeout */
	if (time_is_after_eq_jiffies(dev->admin_stamp + IONIC_ADMIN_TIMEOUT))
		ionic_admin_reset_dwork(dev);
	else
		ionic_admin_timedout(dev);
}

static void ionic_admin_work(struct work_struct *ws)
{
	struct ionic_ibdev *dev =
		container_of(ws, struct ionic_ibdev, admin_work);
	unsigned long irqflags;

	spin_lock_irqsave(&dev->admin_lock, irqflags);
	ionic_admin_poll_locked(dev);
	spin_unlock_irqrestore(&dev->admin_lock, irqflags);
}

void ionic_admin_post(struct ionic_ibdev *dev, struct ionic_admin_wr *wr)
{
	struct ionic_aq *aq = dev->adminq;
	unsigned long irqflags;
	bool poll;

	wr->status = IONIC_ADMIN_POSTED;

	spin_lock_irqsave(&dev->admin_lock, irqflags);
	poll = list_empty(&aq->wr_post);
	list_add(&wr->aq_ent, &aq->wr_post);
	if (poll)
		ionic_admin_poll_locked(dev);
	spin_unlock_irqrestore(&dev->admin_lock, irqflags);
}

void ionic_admin_cancel(struct ionic_ibdev *dev, struct ionic_admin_wr *wr)
{
	struct ionic_aq *aq = dev->adminq;
	unsigned long irqflags;

	spin_lock_irqsave(&dev->admin_lock, irqflags);

	if (!list_empty(&wr->aq_ent)) {
		list_del(&wr->aq_ent);
		if (wr->status != IONIC_ADMIN_POSTED)
			aq->q_wr[wr->status] = NULL;
	}

	spin_unlock_irqrestore(&dev->admin_lock, irqflags);
}

static void ionic_admin_wait(struct ionic_ibdev *dev,
			     struct ionic_admin_wr *wr)
{
	wait_for_completion(&wr->work);
}

static int ionic_admin_busy_wait(struct ionic_ibdev *dev,
				 struct ionic_admin_wr *wr)
{
	unsigned long irqflags;
	int try_i;

	for (try_i = 0; ; ++try_i) {
		if (completion_done(&wr->work))
			return 0;

		/* did not complete before timeout, do not continue waiting,
		 * but initiate rdma lif reset and indicate error to caller.
		 */
		if (try_i >= IONIC_ADMIN_BUSY_RETRY_COUNT) {
			ionic_admin_timedout(dev);
			return -ETIMEDOUT;
		}

		mdelay(IONIC_ADMIN_BUSY_RETRY_MS);

		spin_lock_irqsave(&dev->admin_lock, irqflags);
		ionic_admin_poll_locked(dev);
		spin_unlock_irqrestore(&dev->admin_lock, irqflags);
	}

	/* unreachable */
	return -EINTR;
}

static int ionic_v1_noop_cmd(struct ionic_ibdev *dev)
{
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe.op = IONIC_V1_ADMIN_NOOP,
	};
	long timeout;
	int rc;

	ionic_admin_post(dev, &wr);

	timeout = wait_for_completion_interruptible_timeout(&wr.work, HZ);
	if (timeout > 0)
		rc = 0;
	else if (timeout == 0)
		rc = -ETIMEDOUT;
	else
		rc = timeout;

	if (rc) {
		dev_warn(&dev->ibdev.dev, "noop wait status %d\n", rc);
		ionic_admin_cancel(dev, &wr);
	} else if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "noop killed\n");
		rc = -ENODEV;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "noop error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		dev_dbg(&dev->ibdev.dev, "noop success\n");
		rc = 0;
	}

	return rc;
}

static int ionic_noop_cmd(struct ionic_ibdev *dev)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_NOOP)
			return ionic_v1_noop_cmd(dev);
		return -ENOSYS;
	default:
		return -ENOSYS;
	}
}

static int ionic_v1_stats_cmd(struct ionic_ibdev *dev,
			      dma_addr_t dma, size_t len, int op)
{
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = op,
			.stats = {
				.dma_addr = cpu_to_le64(dma),
				.length = cpu_to_le64(len),
			}
		}
	};
	int rc;

	ionic_admin_post(dev, &wr);

	rc = wait_for_completion_interruptible(&wr.work);
	if (rc) {
		dev_warn(&dev->ibdev.dev, "wait status %d\n", rc);
		ionic_admin_cancel(dev, &wr);
	} else if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = -ENODEV;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	return rc;
}

/* XXX makeshift will be removed */
static const char xxx_fake_hdrs[] =
	"xxx_a\0xxx_bc\0xxx_def\0xxx_g\0xxx_h\0xxx_ijkl\0xxx_m\0xxx_n";

static int ionic_stats_hdrs_cmd(struct ionic_ibdev *dev,
				dma_addr_t dma, size_t len,
				char *xxx_buf)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_STATS_HDRS)
			return ionic_v1_stats_cmd(dev, dma, len,
						  IONIC_V1_ADMIN_STATS_HDRS);
		if (!ionic_xxx_fake_stats)
			return -ENOSYS;
		/* XXX makeshift will be removed */
		dma_sync_single_for_cpu(dev->hwdev, dma, len, DMA_FROM_DEVICE);
		memcpy(xxx_buf, xxx_fake_hdrs, min(len, sizeof(xxx_fake_hdrs)));
		dma_sync_single_for_device(dev->hwdev, dma, len, DMA_FROM_DEVICE);
		return 0;
	default:
		return -ENOSYS;
	}
}

static int ionic_stats_vals_cmd(struct ionic_ibdev *dev,
				dma_addr_t dma, size_t len,
				__be64 *xxx_buf)
{
	int stat_i, stat_count = min_t(int, dev->stats_count, len / sizeof(*xxx_buf));

	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_STATS_VALS)
			return ionic_v1_stats_cmd(dev, dma, len,
						  IONIC_V1_ADMIN_STATS_VALS);
		if (!ionic_xxx_fake_stats)
			return -ENOSYS;
		/* XXX makeshift will be removed */
		dma_sync_single_for_cpu(dev->hwdev, dma, len, DMA_FROM_DEVICE);
		for (stat_i = 0; stat_i < stat_count; ++stat_i)
			xxx_buf[stat_i] = cpu_to_be64(stat_i + 1234);
		dma_sync_single_for_device(dev->hwdev, dma, len, DMA_FROM_DEVICE);
		return 0;
	default:
		return -ENOSYS;
	}
}

static int ionic_init_hw_stats(struct ionic_ibdev *dev)
{
	dma_addr_t stats_dma;
	int rc, hdr_i, buf_i = 0;

	if (dev->stats_hdrs)
		return 0;

	/* XXX get count and buf size from device */
	dev->stats_count = 8;
	dev->stats_size = sizeof(xxx_fake_hdrs);

	dev->stats_hdrs = kmalloc_array(dev->stats_count,
					sizeof(*dev->stats_hdrs), GFP_KERNEL);
	if (!dev->stats_hdrs) {
		rc = -ENOMEM;
		goto err_hdrs;
	}

	dev->stats_buf = kmalloc(dev->stats_size, GFP_KERNEL);
	if (!dev->stats_buf) {
		rc = -ENOMEM;
		goto err_buf;
	}

	stats_dma = dma_map_single(dev->hwdev, dev->stats_buf,
				   dev->stats_size, DMA_FROM_DEVICE);
	rc = dma_mapping_error(dev->hwdev, stats_dma);
	if (rc)
		goto err_dma;

	rc = ionic_stats_hdrs_cmd(dev, stats_dma, dev->stats_size,
				  dev->stats_buf);
	if (rc)
		goto err_cmd;

	dma_unmap_single(dev->hwdev, stats_dma, dev->stats_size,
			 DMA_FROM_DEVICE);

	rc = -EINVAL;

	for (hdr_i = 0; hdr_i < dev->stats_count; ++hdr_i, ++buf_i) {
		dev->stats_hdrs[hdr_i] = &dev->stats_buf[buf_i];

		buf_i += strnlen(&dev->stats_buf[buf_i],
				 dev->stats_size - buf_i);

		if (buf_i == dev->stats_size)
			goto err_dma;
	}

	if (hdr_i != dev->stats_count)
		goto err_dma;

	if (buf_i != dev->stats_size)
		goto err_dma;

	return 0;

err_cmd:
	dma_unmap_single(dev->hwdev, stats_dma, dev->stats_size,
			 DMA_FROM_DEVICE);
err_dma:
	kfree(dev->stats_buf);
	dev->stats_buf = NULL;
err_buf:
	kfree(dev->stats_hdrs);
	dev->stats_hdrs = NULL;
err_hdrs:
	dev->stats_count = 0;
	dev->stats_size = 0;
	return rc;
}

static struct rdma_hw_stats *ionic_alloc_hw_stats(struct ib_device *ibdev,
						  u8 port)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);
	int rc;

	if (port != 1)
		return NULL;

	rc = ionic_init_hw_stats(dev);
	if (rc)
		return NULL;

	return rdma_alloc_hw_stats_struct(dev->stats_hdrs, dev->stats_count,
					  RDMA_HW_STATS_DEFAULT_LIFESPAN);
}

static int ionic_get_hw_stats(struct ib_device *ibdev,
			      struct rdma_hw_stats *stats,
			      u8 port, int index)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);
	size_t stats_vec_size;
	__be64 *stats_vec;
	dma_addr_t stats_dma;
	int rc, stat_i;

	if (port != 1)
		return -EINVAL;

	stats_vec_size = dev->stats_count * sizeof(*stats_vec);
	stats_vec = kmalloc(stats_vec_size, GFP_KERNEL);
	if (!stats_vec) {
		rc = -ENOMEM;
		goto err_vec;
	}

	stats_dma = dma_map_single(dev->hwdev, stats_vec, stats_vec_size,
				   DMA_FROM_DEVICE);
	rc = dma_mapping_error(dev->hwdev, stats_dma);
	if (rc)
		goto err_dma;

	rc = ionic_stats_vals_cmd(dev, stats_dma, stats_vec_size, stats_vec);
	if (rc)
		goto err_cmd;

	dma_unmap_single(dev->hwdev, stats_dma, dev->stats_size,
			 DMA_FROM_DEVICE);

	for (stat_i = 0; stat_i < dev->stats_count; ++stat_i)
		stats->value[stat_i] = be64_to_cpu(stats_vec[stat_i]);

	kfree(stats_vec);

	return stat_i;

err_cmd:
	dma_unmap_single(dev->hwdev, stats_dma, dev->stats_size,
			 DMA_FROM_DEVICE);
err_dma:
	kfree(stats_vec);
err_vec:
	return rc;
}

static int ionic_query_device(struct ib_device *ibdev,
			      struct ib_device_attr *attr,
			      struct ib_udata *udata)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);

	*attr = dev->dev_attr;

	return 0;
}

static int ionic_query_port(struct ib_device *ibdev, u8 port,
			    struct ib_port_attr *attr)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);

	if (port != 1)
		return -EINVAL;

	*attr = dev->port_attr;

	return ib_get_eth_speed(ibdev, port,
				&attr->active_speed,
				&attr->active_width);
}

static enum rdma_link_layer ionic_get_link_layer(struct ib_device *ibdev,
						 u8 port)
{
	return IB_LINK_LAYER_ETHERNET;
}

static struct net_device *ionic_get_netdev(struct ib_device *ibdev, u8 port)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);

	if (port != 1)
		return ERR_PTR(-EINVAL);

	dev_hold(dev->ndev);

	return dev->ndev;
}

#ifdef HAVE_REQUIRED_IB_GID
static int ionic_query_gid(struct ib_device *ibdev, u8 port, int index,
			   union ib_gid *gid)
{
	int rc;

	rc = ib_get_cached_gid(ibdev, port, index, gid, NULL);
	if (rc == -EAGAIN) {
		memcpy(gid, &zgid, sizeof(*gid));
		return 0;
	}

	return rc;
}

#ifdef HAVE_IB_GID_DEV_PORT_INDEX
static int ionic_add_gid(struct ib_device *ibdev, u8 port,
			 unsigned int index, const union ib_gid *gid,
			 const struct ib_gid_attr *attr, void **context)
#else
static int ionic_add_gid(const union ib_gid *gid,
			 const struct ib_gid_attr *attr, void **context)
#endif
{
	enum rdma_network_type net;

	net = ib_gid_to_network_type(attr->gid_type, gid);
	if (net != RDMA_NETWORK_IPV4 && net != RDMA_NETWORK_IPV6)
		return -EINVAL;

	return 0;
}

#ifdef HAVE_IB_GID_DEV_PORT_INDEX
static int ionic_del_gid(struct ib_device *ibdev, u8 port,
			 unsigned int index, void **context)
#else
static int ionic_del_gid(const struct ib_gid_attr *attr, void **context)
#endif
{
	return 0;
}
#endif

static int ionic_query_pkey(struct ib_device *ibdev, u8 port, u16 index,
			    u16 *pkey)
{
	if (port != 1)
		return -EINVAL;

	if (index != 0)
		return -EINVAL;

	*pkey = 0xffff;

	return 0;
}

static int ionic_modify_device(struct ib_device *ibdev, int mask,
			       struct ib_device_modify *attr)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);

	if (mask & ~IB_DEVICE_MODIFY_NODE_DESC)
		return -EOPNOTSUPP;

	if (mask & IB_DEVICE_MODIFY_NODE_DESC)
		memcpy(dev->ibdev.node_desc, attr->node_desc,
		       IB_DEVICE_NODE_DESC_MAX);

	return 0;
}

static int ionic_modify_port(struct ib_device *ibdev, u8 port, int mask,
			     struct ib_port_modify *attr)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);

	if (port != 1)
		return -EINVAL;

	dev->port_attr.port_cap_flags |= attr->set_port_cap_mask;
	dev->port_attr.port_cap_flags &= ~attr->clr_port_cap_mask;

	if (mask & IB_PORT_RESET_QKEY_CNTR)
		dev->port_attr.qkey_viol_cntr = 0;

	return 0;
}

static struct ib_ucontext *ionic_alloc_ucontext(struct ib_device *ibdev,
						struct ib_udata *udata)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);
	struct ionic_ctx *ctx;
	struct ionic_ctx_req req;
	struct ionic_ctx_resp resp = {};
	phys_addr_t db_phys = 0;
	int rc;

	rc = ionic_validate_udata(udata, sizeof(req), sizeof(resp));
	if (!rc)
		rc = ib_copy_from_udata(&req, udata, sizeof(req));

	if (rc)
		goto err_ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		rc = -ENOMEM;
		goto err_ctx;
	}

	/* maybe force fallback to kernel space */
	ctx->fallback = req.fallback > 1;
	if (!ctx->fallback) {
		/* try to allocate dbid for user ctx */
		if (ionic_xxx_kdbid) {
			/* XXX kernel dbid in user space */
			ctx->dbid = dev->dbid;
			db_phys = dev->xxx_dbpage_phys;
			rc = 0;
		} else {
			rc = ionic_api_get_dbid(dev->lif, &ctx->dbid, &db_phys);
		}
		if (rc < 0) {
			/* maybe allow fallback to kernel space */
			ctx->fallback = req.fallback > 0;
			if (!ctx->fallback)
				goto err_dbid;
			rc = -1;
		}
	}
	if (ctx->fallback)
		dev_dbg(&dev->ibdev.dev, "fallback kernel space\n");
	else
		dev_dbg(&dev->ibdev.dev, "user space dbid %u\n", ctx->dbid);

	mutex_init(&ctx->mmap_mut);
	ctx->mmap_off = PAGE_SIZE;
	INIT_LIST_HEAD(&ctx->mmap_list);

	ctx->mmap_dbell.offset = 0;
	ctx->mmap_dbell.size = PAGE_SIZE;
	ctx->mmap_dbell.pfn = PHYS_PFN(db_phys);
	ctx->mmap_dbell.writecombine = false;
	list_add(&ctx->mmap_dbell.ctx_ent, &ctx->mmap_list);

	resp.fallback = ctx->fallback;
	resp.page_shift = PAGE_SHIFT;

	resp.dbell_offset = 0;

	resp.version = dev->rdma_version;
	resp.qp_opcodes = dev->qp_opcodes;
	resp.admin_opcodes = dev->admin_opcodes;

	resp.sq_qtype = dev->sq_qtype;
	resp.rq_qtype = dev->rq_qtype;
	resp.cq_qtype = dev->cq_qtype;
	resp.admin_qtype = dev->aq_qtype;
	resp.max_stride = dev->max_stride;

	rc = ib_copy_to_udata(udata, &resp, sizeof(resp));
	if (rc)
		goto err_resp;

	return &ctx->ibctx;

err_resp:
	if (!ionic_xxx_kdbid)
		ionic_api_put_dbid(dev->lif, ctx->dbid);
err_dbid:
	kfree(ctx);
err_ctx:
	return ERR_PTR(rc);
}

static int ionic_dealloc_ucontext(struct ib_ucontext *ibctx)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibctx->device);
	struct ionic_ctx *ctx = to_ionic_ctx(ibctx);

	list_del(&ctx->mmap_dbell.ctx_ent);

	if (WARN_ON(!list_empty(&ctx->mmap_list)))
		list_del(&ctx->mmap_list);

	if (!ionic_xxx_kdbid)
		ionic_api_put_dbid(dev->lif, ctx->dbid);
	kfree(ctx);

	return 0;
}

static int ionic_mmap(struct ib_ucontext *ibctx, struct vm_area_struct *vma)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibctx->device);
	struct ionic_ctx *ctx = to_ionic_ctx(ibctx);
	struct ionic_mmap_info *info;
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	int rc = 0;

	mutex_lock(&ctx->mmap_mut);

	list_for_each_entry(info, &ctx->mmap_list, ctx_ent)
		if (info->offset == offset)
			goto found;

	mutex_unlock(&ctx->mmap_mut);

	/* not found */
	dev_dbg(&dev->ibdev.dev, "not found %#lx\n", offset);
	rc = -EINVAL;
	goto out;

found:
	list_del_init(&info->ctx_ent);
	mutex_unlock(&ctx->mmap_mut);

	if (info->size != size) {
		dev_dbg(&dev->ibdev.dev, "invalid size %#lx (%#lx)\n",
			size, info->size);
		rc = -EINVAL;
		goto out;
	}

	dev_dbg(&dev->ibdev.dev, "writecombine? %d\n", info->writecombine);
	if (info->writecombine)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	else
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	dev_dbg(&dev->ibdev.dev, "remap st %#lx pf %#lx sz %#lx\n",
		vma->vm_start, info->pfn, size);
	rc = io_remap_pfn_range(vma, vma->vm_start, info->pfn, size,
				vma->vm_page_prot);
	if (rc)
		dev_dbg(&dev->ibdev.dev, "remap failed %d\n", rc);

out:
	return rc;
}

static struct ib_pd *ionic_alloc_pd(struct ib_device *ibdev,
				    struct ib_ucontext *ibctx,
				    struct ib_udata *udata)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);
	struct ionic_pd *pd;
	int rc;

	pd = kzalloc(sizeof(*pd), GFP_KERNEL);
	if (!pd) {
		rc = -ENOMEM;
		goto err_pd;
	}

	rc = ionic_get_pdid(dev, &pd->pdid);
	if (rc)
		goto err_pdid;

	return &pd->ibpd;

err_pdid:
	kfree(pd);
err_pd:
	return ERR_PTR(rc);
}

static int ionic_dealloc_pd(struct ib_pd *ibpd)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibpd->device);
	struct ionic_pd *pd = to_ionic_pd(ibpd);

	ionic_put_pdid(dev, pd->pdid);
	kfree(pd);

	return 0;
}

static int ionic_build_hdr(struct ionic_ibdev *dev,
			   struct ib_ud_header *hdr,
			   const struct rdma_ah_attr *attr)
{
	const struct ib_global_route *grh;
#ifndef HAVE_AH_ATTR_CACHED_GID
	struct ib_gid_attr sgid_attr;
	union ib_gid sgid;
	u8 smac[ETH_ALEN];
#endif
	enum rdma_network_type net;
	u16 vlan;
	int rc;

	if (attr->ah_flags != IB_AH_GRH)
		return -EINVAL;

#ifdef HAVE_RDMA_AH_ATTR_TYPE_ROCE
	if (attr->type != RDMA_AH_ATTR_TYPE_ROCE)
		return -EINVAL;
#endif

	grh = rdma_ah_read_grh(attr);

#ifdef HAVE_AH_ATTR_CACHED_GID
	vlan = rdma_vlan_dev_vlan_id(grh->sgid_attr->ndev);
	net = rdma_gid_attr_network_type(grh->sgid_attr);
#else
	rc = ib_get_cached_gid(&dev->ibdev, 1, grh->sgid_index,
			       &sgid, &sgid_attr);
	if (rc)
		return rc;

	if (!sgid_attr.ndev)
		return -ENXIO;

	ether_addr_copy(smac, sgid_attr.ndev->dev_addr);
	vlan = rdma_vlan_dev_vlan_id(sgid_attr.ndev);
	net = ib_gid_to_network_type(sgid_attr.gid_type, &sgid);

	dev_put(sgid_attr.ndev); /* hold from ib_get_cached_gid */
	sgid_attr.ndev = NULL;

	if (net != ib_gid_to_network_type(sgid_attr.gid_type, &grh->dgid))
		return -EINVAL;
#endif

	rc = ib_ud_header_init(0,	/* no payload */
			       0,	/* no lrh */
			       1,	/* yes eth */
			       vlan != 0xffff,
			       0,	/* no grh */
			       net == RDMA_NETWORK_IPV4 ? 4 : 6,
			       1,	/* yes udp */
			       0,	/* no imm */
			       hdr);
	if (rc)
		return rc;

#ifdef HAVE_AH_ATTR_CACHED_GID
	ether_addr_copy(hdr->eth.smac_h, grh->sgid_attr->ndev->dev_addr);
#else
	ether_addr_copy(hdr->eth.smac_h, smac);
#endif

#ifdef HAVE_RDMA_AH_ATTR_TYPE_ROCE
	ether_addr_copy(hdr->eth.dmac_h, attr->roce.dmac);
#else
	ether_addr_copy(hdr->eth.dmac_h, attr->dmac);
#endif

	if (net == RDMA_NETWORK_IPV4) {
		hdr->eth.type = cpu_to_be16(ETH_P_IP);
		hdr->ip4.tos = ionic_set_ecn(grh->traffic_class);
		hdr->ip4.frag_off = cpu_to_be16(0x4000); /* don't fragment */
		hdr->ip4.ttl = grh->hop_limit;
		hdr->ip4.tot_len = 65535;
#ifdef HAVE_AH_ATTR_CACHED_GID
		hdr->ip4.saddr = *(const __be32 *)(grh->sgid_attr->gid.raw + 12);
#else
		hdr->ip4.saddr = *(const __be32 *)(sgid.raw + 12);
#endif
		hdr->ip4.daddr = *(const __be32 *)(grh->dgid.raw + 12);
	} else {
		hdr->eth.type = cpu_to_be16(ETH_P_IPV6);
		hdr->grh.traffic_class = ionic_set_ecn(grh->traffic_class);
		hdr->grh.flow_label = cpu_to_be32(grh->flow_label);
		hdr->grh.hop_limit = grh->hop_limit;
#ifdef HAVE_AH_ATTR_CACHED_GID
		hdr->grh.source_gid = grh->sgid_attr->gid;
#else
		hdr->grh.source_gid = sgid;
#endif
		hdr->grh.destination_gid = grh->dgid;
	}

	if (vlan != 0xffff) {
		vlan |= attr->sl << 13; /* 802.1q PCP */
		hdr->vlan.tag = cpu_to_be16(vlan);
		hdr->vlan.type = hdr->eth.type;
		hdr->eth.type = cpu_to_be16(ETH_P_8021Q);
	}

	hdr->udp.sport = cpu_to_be16(49152); /* XXX hardcode val */
	hdr->udp.dport = cpu_to_be16(ROCE_V2_UDP_DPORT);

	return 0;
}

static int ionic_v1_create_ah_cmd(struct ionic_ibdev *dev,
				  struct ionic_ah *ah,
				  struct ionic_pd *pd,
				  struct rdma_ah_attr *attr,
				  u32 flags)
{
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = IONIC_V1_ADMIN_CREATE_AH,
			.dbid_flags = cpu_to_le16(dev->dbid),
			.id_ver = cpu_to_le32(ah->ahid),
			.ah = {
				.pd_id = cpu_to_le32(pd->pdid),
			}
		}
	};
	struct ib_ud_header *hdr;
	dma_addr_t hdr_dma = 0;
	void *hdr_buf;
	int rc, hdr_len = 0, gfp = GFP_ATOMIC;

	if (flags & RDMA_CREATE_AH_SLEEPABLE)
		gfp = GFP_KERNEL;

	hdr = kmalloc(sizeof(*hdr), gfp);
	if (!hdr) {
		rc = -ENOMEM;
		goto err_hdr;
	}

	rc = ionic_build_hdr(dev, hdr, attr);
	if (rc)
		goto err_buf;

	hdr_buf = kmalloc(PAGE_SIZE, gfp);
	if (!hdr_buf) {
		rc = -ENOMEM;
		goto err_buf;
	}

	hdr_len = ib_ud_header_pack(hdr, hdr_buf);
	hdr_len -= IB_BTH_BYTES;
	hdr_len -= IB_DETH_BYTES;
	hdr_len -= IB_UDP_BYTES;

	if (ionic_xxx_udp)
		hdr_len += IB_UDP_BYTES;

	dev_dbg(&dev->ibdev.dev, "roce packet header template\n");
	print_hex_dump_debug("hdr ", DUMP_PREFIX_OFFSET, 16, 1,
			     hdr_buf, hdr_len, true);

	hdr_dma = dma_map_single(dev->hwdev, hdr_buf, hdr_len,
				 DMA_TO_DEVICE);

	rc = dma_mapping_error(dev->hwdev, hdr_dma);
	if (rc)
		goto err_dma;

	wr.wqe.ah.dma_addr = cpu_to_le64(hdr_dma);
	wr.wqe.ah.length = cpu_to_le64(hdr_len);

	ionic_admin_post(dev, &wr);

	if (flags & RDMA_CREATE_AH_SLEEPABLE) {
		ionic_admin_wait(dev, &wr);
		rc = 0;
	} else {
		rc = ionic_admin_busy_wait(dev, &wr);
	}

	if (rc) {
		dev_warn(&dev->ibdev.dev, "wait status %d\n", rc);
		ionic_admin_cancel(dev, &wr);
	} else if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = -ENODEV;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	dma_unmap_single(dev->hwdev, hdr_dma, hdr_len,
			 DMA_TO_DEVICE);
err_dma:
	kfree(hdr_buf);
err_buf:
	kfree(hdr);
err_hdr:
	return rc;
}

static int ionic_create_ah_cmd(struct ionic_ibdev *dev,
			       struct ionic_ah *ah,
			       struct ionic_pd *pd,
			       struct rdma_ah_attr *attr,
			       u32 flags)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_CREATE_AH)
			return ionic_v1_create_ah_cmd(dev, ah, pd, attr, flags);
		return -ENOSYS;
	default:
		return -ENOSYS;
	}
}

static int ionic_v1_destroy_ah_cmd(struct ionic_ibdev *dev, u32 ahid, u32 flags)
{
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = IONIC_V1_ADMIN_DESTROY_AH,
			.id_ver = cpu_to_le32(ahid),
		}
	};
	int rc;

	ionic_admin_post(dev, &wr);

	if (flags & RDMA_CREATE_AH_SLEEPABLE) {
		ionic_admin_wait(dev, &wr);
		rc = 0;
	} else {
		rc = ionic_admin_busy_wait(dev, &wr);
	}

	if (rc) {
		dev_warn(&dev->ibdev.dev, "wait status %d\n", rc);
		ionic_admin_cancel(dev, &wr);

		/* No host-memory resource is associated with ah, so it is ok
		 * to "succeed" and complete this destroy ah on the host.
		 */
		rc = 0;
	} else if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = 0;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	return rc;
}

static int ionic_destroy_ah_cmd(struct ionic_ibdev *dev, u32 ahid, u32 flags)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_DESTROY_AH)
			return ionic_v1_destroy_ah_cmd(dev, ahid, flags);
		/* XXX require opcode destroy ah */
		//return -ENOSYS;
		return 0;
	default:
		return -ENOSYS;
	}
}

#ifdef HAVE_CREATE_AH_UDATA
#ifdef HAVE_CREATE_AH_FLAGS
static struct ib_ah *ionic_create_ah(struct ib_pd *ibpd,
				     struct rdma_ah_attr *attr,
				     u32 flags,
				     struct ib_udata *udata)
#else
static struct ib_ah *ionic_create_ah(struct ib_pd *ibpd,
				     struct rdma_ah_attr *attr,
				     struct ib_udata *udata)
#endif
#else
static struct ib_ah *ionic_create_ah(struct ib_pd *ibpd,
				     struct rdma_ah_attr *attr)
#endif
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibpd->device);
	struct ionic_ctx *ctx = to_ionic_ctx_uobj(ibpd->uobject);
	struct ionic_pd *pd = to_ionic_pd(ibpd);
	struct ionic_ah *ah;
	struct ionic_ah_resp resp = {};
	int rc, gfp = GFP_ATOMIC;
#ifndef HAVE_CREATE_AH_FLAGS
	u32 flags = 0;
#endif

#ifdef HAVE_CREATE_AH_UDATA
	if (ctx)
		rc = ionic_validate_udata(udata, 0, sizeof(resp));
	else
		rc = ionic_validate_udata(udata, 0, 0);
	if (rc)
		goto err_ah;
#endif

	if (flags & RDMA_CREATE_AH_SLEEPABLE)
		gfp = GFP_KERNEL;

	ah = kzalloc(sizeof(*ah), gfp);
	if (!ah) {
		rc = -ENOMEM;
		goto err_ah;
	}

	rc = ionic_get_ahid(dev, &ah->ahid);
	if (rc)
		goto err_ahid;

	rc = ionic_create_ah_cmd(dev, ah, pd, attr, flags);
	if (rc)
		goto err_cmd;

	if (ctx) {
		resp.ahid = ah->ahid;

#ifdef HAVE_CREATE_AH_UDATA
		rc = ib_copy_to_udata(udata, &resp, sizeof(resp));
		if (rc)
			goto err_resp;
#endif
	}

	return &ah->ibah;

#ifdef HAVE_CREATE_AH_UDATA
err_resp:
	ionic_destroy_ah_cmd(dev, ah->ahid, flags);
#endif
err_cmd:
	ionic_put_ahid(dev, ah->ahid);
err_ahid:
	kfree(ah);
err_ah:
	return ERR_PTR(rc);
}

#ifdef HAVE_CREATE_AH_FLAGS
static int ionic_destroy_ah(struct ib_ah *ibah, u32 flags)
#else
static int ionic_destroy_ah(struct ib_ah *ibah)
#endif
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibah->device);
	struct ionic_ah *ah = to_ionic_ah(ibah);
	int rc;
#ifndef HAVE_CREATE_AH_FLAGS
	u32 flags = 0;
#endif

	rc = ionic_destroy_ah_cmd(dev, ah->ahid, flags);
	if (rc)
		return rc;

	ionic_put_ahid(dev, ah->ahid);
	kfree(ah);

	return 0;
}

static int ionic_v1_create_mr_cmd(struct ionic_ibdev *dev, struct ionic_pd *pd,
				  struct ionic_mr *mr, u64 addr, u64 length)
{
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = IONIC_V1_ADMIN_CREATE_MR,
			.dbid_flags = cpu_to_le16(mr->flags),
			.id_ver = cpu_to_le32(mr->mrid),
			.mr = {
				.va = cpu_to_le64(addr),
				.length = cpu_to_le64(length),
				.pd_id = cpu_to_le32(pd->pdid),
				.page_size_log2 = mr->buf.page_size_log2,
				.tbl_index = cpu_to_le32(mr->res.tbl_pos),
				.map_count = cpu_to_le32(mr->buf.tbl_pages),
				.dma_addr = cpu_to_le64(mr->buf.tbl_dma),
			}
		}
	};
	int rc;

	ionic_admin_post(dev, &wr);
	ionic_admin_wait(dev, &wr);

	if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = -ENODEV;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "cqe error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	return rc;
}

static int ionic_create_mr_cmd(struct ionic_ibdev *dev, struct ionic_pd *pd,
			       struct ionic_mr *mr, u64 addr, u64 length)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_DESTROY_MR)
			return ionic_v1_create_mr_cmd(dev, pd, mr,
						      addr, length);
		return -ENOSYS;
	default:
		return -ENOSYS;
	}
}

static int ionic_v1_destroy_mr_cmd(struct ionic_ibdev *dev, u32 mrid)
{
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = IONIC_V1_ADMIN_DESTROY_MR,
			.id_ver = cpu_to_le32(mrid),
		}
	};
	int rc;

	ionic_admin_post(dev, &wr);
	ionic_admin_wait(dev, &wr);

	if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = 0;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "cqe error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	return rc;
}

static int ionic_destroy_mr_cmd(struct ionic_ibdev *dev, u32 mrid)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_DESTROY_MR)
			return ionic_v1_destroy_mr_cmd(dev, mrid);
		return -ENOSYS;
	default:
		return -ENOSYS;
	}
}

static struct ib_mr *ionic_get_dma_mr(struct ib_pd *ibpd, int access)
{
	struct ionic_mr *mr;

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr)
		return ERR_PTR(-ENOMEM);

	return &mr->ibmr;
}

static struct ib_mr *ionic_reg_user_mr(struct ib_pd *ibpd, u64 start,
				       u64 length, u64 addr, int access,
				       struct ib_udata *udata)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibpd->device);
	struct ionic_pd *pd = to_ionic_pd(ibpd);
	struct ionic_mr *mr;
	int rc;

	rc = ionic_validate_udata(udata, 0, 0);
	if (rc)
		goto err_mr;

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr) {
		rc = -ENOMEM;
		goto err_mr;
	}

	rc = ionic_get_mrid(dev, &mr->mrid);
	if (rc)
		goto err_mrid;

	mr->ibmr.lkey = mr->mrid;
	mr->ibmr.rkey = mr->mrid;
	mr->ibmr.iova = addr;
	mr->ibmr.length = length;

	mr->flags = IONIC_MRF_USER_MR | to_ionic_mr_flags(access);

	mr->umem = ib_umem_get(ibpd->uobject->context, start, length, access, 0);
	if (IS_ERR(mr->umem)) {
		rc = PTR_ERR(mr->umem);
		goto err_umem;
	}

	rc = ionic_pgtbl_init(dev, &mr->res, &mr->buf, mr->umem, 0, 1);
	if (rc)
		goto err_pgtbl;

	rc = ionic_create_mr_cmd(dev, pd, mr, addr, length);
	if (rc)
		goto err_cmd;

	ionic_dbgfs_add_mr(dev, mr);

	ionic_pgtbl_unbuf(dev, &mr->buf);

	return &mr->ibmr;

err_cmd:
	ionic_pgtbl_unbuf(dev, &mr->buf);
	ionic_put_res(dev, &mr->res);
err_pgtbl:
	ib_umem_release(mr->umem);
err_umem:
	ionic_put_mrid(dev, mr->mrid);
err_mrid:
	kfree(mr);
err_mr:
	return ERR_PTR(rc);
}

static int ionic_rereg_user_mr(struct ib_mr *ibmr, int flags, u64 start,
			       u64 length, u64 addr, int access,
			       struct ib_pd *ibpd, struct ib_udata *udata)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibmr->device);
	struct ionic_mr *mr = to_ionic_mr(ibmr);
	struct ionic_pd *pd;
	int rc;

	if (!mr->ibmr.lkey)
		return -EINVAL;

	if (mr->res.tbl_order == IONIC_RES_INVALID) {
		/* must set translation if not already on device */
		if (~flags & IB_MR_REREG_TRANS)
			return -EINVAL;
	} else {
		/* destroy on device first if not already on device */
		rc = ionic_destroy_mr_cmd(dev, mr->mrid);
		if (rc)
			return rc;
	}

	if (~flags & IB_MR_REREG_PD)
		ibpd = mr->ibmr.pd;
	pd = to_ionic_pd(ibpd);

	mr->mrid = ib_inc_rkey(mr->mrid);
	mr->ibmr.lkey = mr->mrid;
	mr->ibmr.rkey = mr->mrid;

	if (flags & IB_MR_REREG_ACCESS)
		mr->flags = IONIC_MRF_USER_MR | to_ionic_mr_flags(access);

	if (flags & IB_MR_REREG_TRANS) {
		ionic_pgtbl_unbuf(dev, &mr->buf);
		ionic_put_res(dev, &mr->res);

		if (mr->umem)
			ib_umem_release(mr->umem);

		mr->ibmr.iova = addr;
		mr->ibmr.length = length;

		mr->umem = ib_umem_get(ibpd->uobject->context, start,
				       length, access, 0);
		if (IS_ERR(mr->umem)) {
			rc = PTR_ERR(mr->umem);
			goto err_umem;
		}

		rc = ionic_pgtbl_init(dev, &mr->res, &mr->buf, mr->umem, 0, 1);
		if (rc)
			goto err_pgtbl;
	}

	rc = ionic_create_mr_cmd(dev, pd, mr, addr, length);
	if (rc)
		goto err_cmd;

	ionic_pgtbl_unbuf(dev, &mr->buf);

	return 0;

err_cmd:
	ionic_pgtbl_unbuf(dev, &mr->buf);
	ionic_put_res(dev, &mr->res);
err_pgtbl:
	ib_umem_release(mr->umem);
	mr->umem = NULL;
err_umem:
	mr->res.tbl_order = IONIC_RES_INVALID;
	return rc;
}

static int ionic_dereg_mr(struct ib_mr *ibmr)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibmr->device);
	struct ionic_mr *mr = to_ionic_mr(ibmr);
	int rc;

	if (!mr->ibmr.lkey)
		goto out;

	/* no reservation, and the mr does not exist on device */
	if (mr->res.tbl_order == IONIC_RES_INVALID)
		goto out_mrid;

	rc = ionic_destroy_mr_cmd(dev, mr->mrid);
	if (rc)
		return rc;

	ionic_dbgfs_rm_mr(mr);

	ionic_pgtbl_unbuf(dev, &mr->buf);
	ionic_put_res(dev, &mr->res);

	if (mr->umem)
		ib_umem_release(mr->umem);

out_mrid:
	ionic_put_mrid(dev, mr->mrid);

out:
	kfree(mr);

	return 0;
}

static struct ib_mr *ionic_alloc_mr(struct ib_pd *ibpd,
				    enum ib_mr_type type,
				    u32 max_sg)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibpd->device);
	struct ionic_pd *pd = to_ionic_pd(ibpd);
	struct ionic_mr *mr;
	int rc;

	if (type != IB_MR_TYPE_MEM_REG) {
		rc = -EINVAL;
		goto err_mr;
	}

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr) {
		rc = -ENOMEM;
		goto err_mr;
	}

	rc = ionic_get_mrid(dev, &mr->mrid);
	if (rc)
		goto err_mrid;

	mr->ibmr.lkey = mr->mrid;
	mr->ibmr.rkey = mr->mrid;

	mr->flags = IONIC_MRF_PHYS_MR;

	rc = ionic_pgtbl_init(dev, &mr->res, &mr->buf, mr->umem, 0, max_sg);
	if (rc)
		goto err_pgtbl;

	mr->buf.tbl_pages = 0;

	rc = ionic_create_mr_cmd(dev, pd, mr, 0, 0);
	if (rc)
		goto err_cmd;

	ionic_dbgfs_add_mr(dev, mr);

	return &mr->ibmr;

err_cmd:
	ionic_pgtbl_unbuf(dev, &mr->buf);
	ionic_put_res(dev, &mr->res);
err_pgtbl:
	ionic_put_mrid(dev, mr->mrid);
err_mrid:
	kfree(mr);
err_mr:
	return ERR_PTR(rc);
}

static int ionic_map_mr_page(struct ib_mr *ibmr, u64 dma)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibmr->device);
	struct ionic_mr *mr = to_ionic_mr(ibmr);

	dev_dbg(&dev->ibdev.dev, "dma %#llx\n", dma);
	return ionic_pgtbl_page(&mr->buf, dma);
}

static int ionic_map_mr_sg(struct ib_mr *ibmr, struct scatterlist *sg,
			   int sg_nents, unsigned int *sg_offset)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibmr->device);
	struct ionic_mr *mr = to_ionic_mr(ibmr);
	int rc;

	/* mr must be allocated using ib_alloc_mr() */
	if (unlikely(!mr->buf.tbl_limit))
		return -EINVAL;

	mr->buf.tbl_pages = 0;

	if (mr->buf.tbl_buf)
		dma_sync_single_for_cpu(dev->hwdev, mr->buf.tbl_dma,
					mr->buf.tbl_size, DMA_TO_DEVICE);

	dev_dbg(&dev->ibdev.dev, "sg %p nent %d\n", sg, sg_nents);
	rc = ib_sg_to_pages(ibmr, sg, sg_nents, sg_offset, ionic_map_mr_page);

	if (mr->buf.tbl_buf)
		dma_sync_single_for_device(dev->hwdev, mr->buf.tbl_dma,
					   mr->buf.tbl_size, DMA_TO_DEVICE);

	return rc;
}

static struct ib_mw *ionic_alloc_mw(struct ib_pd *ibpd, enum ib_mw_type type,
				    struct ib_udata *udata)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibpd->device);
	struct ionic_pd *pd = to_ionic_pd(ibpd);
	struct ionic_mr *mr;
	int rc;

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr) {
		rc = -ENOMEM;
		goto err_mr;
	}

	rc = ionic_get_mrid(dev, &mr->mrid);
	if (rc)
		goto err_mrid;

	mr->ibmw.rkey = mr->mrid;
	mr->ibmw.type = type;

	if (type == IB_MW_TYPE_1)
		mr->flags = IONIC_MRF_MW_1;
	else
		mr->flags = IONIC_MRF_MW_2;

	rc = ionic_create_mr_cmd(dev, pd, mr, 0, 0);
	if (rc)
		goto err_cmd;

	ionic_dbgfs_add_mr(dev, mr);

	return &mr->ibmw;

err_cmd:
	ionic_put_mrid(dev, mr->mrid);
err_mrid:
	kfree(mr);
err_mr:
	return ERR_PTR(rc);
}

static int ionic_dealloc_mw(struct ib_mw *ibmw)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibmw->device);
	struct ionic_mr *mr = to_ionic_mw(ibmw);
	int rc;

	rc = ionic_destroy_mr_cmd(dev, mr->mrid);
	if (rc)
		return rc;

	ionic_dbgfs_rm_mr(mr);

	ionic_put_mrid(dev, mr->mrid);

	return 0;
}

static int ionic_v1_create_cq_cmd(struct ionic_ibdev *dev, struct ionic_cq *cq,
				  struct ionic_tbl_buf *buf)
{
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = IONIC_V1_ADMIN_CREATE_CQ,
			.dbid_flags = cpu_to_le16(ionic_dbid(dev, cq->ibcq.uobject)),
			.id_ver = cpu_to_le32(cq->cqid),
			.cq = {
				.eq_id = cpu_to_le32(cq->eqid),
				.depth_log2 = cq->q.depth_log2,
				.stride_log2 = cq->q.stride_log2,
				.page_size_log2 = buf->page_size_log2,
				.tbl_index = cpu_to_le32(cq->res.tbl_pos),
				.map_count = cpu_to_le32(buf->tbl_pages),
				.dma_addr = cpu_to_le64(buf->tbl_dma),
			}
		}
	};
	int rc;

	ionic_admin_post(dev, &wr);
	ionic_admin_wait(dev, &wr);

	if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = -ENODEV;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "cqe error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	return rc;
}

static int ionic_v1_destroy_cq_cmd(struct ionic_ibdev *dev, u32 cqid)
{
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = IONIC_V1_ADMIN_DESTROY_CQ,
			.id_ver = cpu_to_le32(cqid),
		}
	};
	int rc;

	ionic_admin_post(dev, &wr);
	ionic_admin_wait(dev, &wr);

	if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = 0;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "cqe error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	return rc;
}

static int ionic_create_cq_cmd(struct ionic_ibdev *dev, struct ionic_cq *cq,
			       struct ionic_tbl_buf *buf)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_CREATE_CQ)
			return ionic_v1_create_cq_cmd(dev, cq, buf);
		return -ENOSYS;
	default:
		return -ENOSYS;
	}
}

static int ionic_destroy_cq_cmd(struct ionic_ibdev *dev, u32 cqid)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_DESTROY_CQ)
			return ionic_v1_destroy_cq_cmd(dev, cqid);
		/* XXX require opcode destroy cq */
		//return -ENOSYS;
		return 0;
	default:
		return -ENOSYS;
	}
}

static struct ionic_cq *__ionic_create_cq(struct ionic_ibdev *dev,
					  struct ionic_tbl_buf *buf,
					  const struct ib_cq_init_attr *attr,
					  struct ib_ucontext *ibctx,
					  struct ib_udata *udata)
{
	struct ionic_ctx *ctx = to_ionic_ctx_fb(ibctx);
	struct ionic_cq *cq;
	struct ionic_cq_req req;
	struct ionic_cq_resp resp;
	int rc, eq_idx;

	if (!ctx) {
		rc = ionic_validate_udata(udata, 0, 0);
	} else {
		rc = ionic_validate_udata(udata, sizeof(req), sizeof(resp));
		if (!rc)
			rc = ib_copy_from_udata(&req, udata, sizeof(req));
	}

	if (rc)
		goto err_cq;

	if (attr->cqe < 1 || attr->cqe + IONIC_CQ_GRACE > 0xffff) {
		rc = -EINVAL;
		goto err_cq;
	}

	cq = kzalloc(sizeof(*cq), GFP_KERNEL);
	if (!cq) {
		rc = -ENOMEM;
		goto err_cq;
	}

	rc = ionic_get_cqid(dev, &cq->cqid);
	if (rc)
		goto err_cqid;

	eq_idx = attr->comp_vector;
	if (eq_idx >= dev->eq_count)
		eq_idx = 0;

	cq->eqid = dev->eq_vec[eq_idx]->eqid;

	spin_lock_init(&cq->lock);
	INIT_LIST_HEAD(&cq->poll_sq);
	INIT_LIST_HEAD(&cq->flush_sq);
	INIT_LIST_HEAD(&cq->flush_rq);

	if (!ctx) {
		rc = ionic_queue_init(&cq->q, dev->hwdev,
				      attr->cqe + IONIC_CQ_GRACE,
				      sizeof(struct ionic_v1_cqe));
		if (rc)
			goto err_q;

		ionic_queue_dbell_init(&cq->q, cq->cqid);
		cq->color = true;
		cq->reserve = cq->q.mask;
	} else {
		rc = ionic_validate_qdesc(&req.cq);
		if (rc)
			goto err_q;

		cq->umem = ib_umem_get(&ctx->ibctx, req.cq.addr, req.cq.size,
				       IB_ACCESS_LOCAL_WRITE, 0);
		if (IS_ERR(cq->umem)) {
			rc = PTR_ERR(cq->umem);
			goto err_q;
		}

		cq->q.ptr = NULL;
		cq->q.size = req.cq.size;
		cq->q.mask = req.cq.mask;
		cq->q.depth_log2 = req.cq.depth_log2;
		cq->q.stride_log2 = req.cq.stride_log2;

		resp.cqid = cq->cqid;

		rc = ib_copy_to_udata(udata, &resp, sizeof(resp));
		if (rc)
			goto err_resp;
	}

	rc = ionic_pgtbl_init(dev, &cq->res, buf, cq->umem, cq->q.dma, 1);
	if (rc)
		goto err_resp;

	ionic_dbgfs_add_cq(dev, cq);

	mutex_lock(&dev->tbl_lock);
	tbl_alloc_node(&dev->cq_tbl);
	tbl_insert(&dev->cq_tbl, cq, cq->cqid);
	mutex_unlock(&dev->tbl_lock);

	return cq;

err_resp:
	if (cq->umem)
		ib_umem_release(cq->umem);
	else
		ionic_queue_destroy(&cq->q, dev->hwdev);
err_q:
	ionic_put_cqid(dev, cq->cqid);
err_cqid:
	kfree(cq);
err_cq:
	return ERR_PTR(rc);
}

static void __ionic_destroy_cq(struct ionic_ibdev *dev, struct ionic_cq *cq)
{
	mutex_lock(&dev->tbl_lock);
	tbl_free_node(&dev->cq_tbl);
	tbl_delete(&dev->cq_tbl, cq->cqid);
	mutex_unlock(&dev->tbl_lock);

	synchronize_rcu();

	ionic_dbgfs_rm_cq(cq);

	ionic_put_res(dev, &cq->res);

	if (cq->umem)
		ib_umem_release(cq->umem);
	else
		ionic_queue_destroy(&cq->q, dev->hwdev);

	ionic_put_cqid(dev, cq->cqid);

	kfree(cq);
}

static struct ib_cq *ionic_create_cq(struct ib_device *ibdev,
				     const struct ib_cq_init_attr *attr,
				     struct ib_ucontext *ibctx,
				     struct ib_udata *udata)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);
	struct ionic_cq *cq;
	struct ionic_tbl_buf buf;
	int rc;

	cq = __ionic_create_cq(dev, &buf, attr, ibctx, udata);
	if (IS_ERR(cq)) {
		rc = PTR_ERR(cq);
		goto err_cq;
	}

	rc = ionic_create_cq_cmd(dev, cq, &buf);
	if (rc)
		goto err_cmd;

	ionic_pgtbl_unbuf(dev, &buf);

	return &cq->ibcq;

err_cmd:
	ionic_pgtbl_unbuf(dev, &buf);
	__ionic_destroy_cq(dev, cq);
err_cq:
	return ERR_PTR(rc);
}

static int ionic_destroy_cq(struct ib_cq *ibcq)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibcq->device);
	struct ionic_cq *cq = to_ionic_cq(ibcq);
	int rc;

	rc = ionic_destroy_cq_cmd(dev, cq->cqid);
	if (rc)
		return rc;

	__ionic_destroy_cq(dev, cq);

	return 0;
}

static int ionic_resize_cq(struct ib_cq *ibcq, int cqe,
			   struct ib_udata *udata)
{
	return -ENOSYS;
}

static int ionic_flush_recv(struct ionic_qp *qp, struct ib_wc *wc)
{
	struct ionic_v1_wqe *wqe;
	struct ionic_rq_meta *meta;

	if (!qp->rq_flush)
		return 0;

	if (ionic_queue_empty(&qp->rq))
		return 0;

	/* This depends on the RQ polled in-order.  It does not work for SRQ,
	 * which can be polled out-of-order.  Driver does not flush SRQ.
	 */
	wqe = ionic_queue_at_cons(&qp->rq);

	/* wqe_id must be a valid queue index */
	if (unlikely(wqe->base.wqe_id >> qp->rq.depth_log2))
		return -EIO;

	/* wqe_id must indicate a request that is outstanding */
	meta = &qp->rq_meta[wqe->base.wqe_id];
	if (unlikely(meta->next != IONIC_META_POSTED))
		return -EIO;

	ionic_queue_consume(&qp->rq);

	memset(wc, 0, sizeof(*wc));

	wc->status = IB_WC_WR_FLUSH_ERR;
	wc->wr_id = meta->wrid;

	/* XXX possible use-after-free after rcu read unlock */
	wc->qp = &qp->ibqp;

	meta->next = qp->rq_meta_head;
	qp->rq_meta_head = meta;

	return 1;
}

static int ionic_flush_recv_many(struct ionic_qp *qp, struct ib_wc *wc, int nwc)
{
	int rc = 0, npolled = 0;

	while(npolled < nwc) {
		rc = ionic_flush_recv(qp, wc + npolled);
		if (rc <= 0)
			break;

		npolled += rc;
	}

	return npolled ?: rc;
}

static int ionic_flush_send(struct ionic_qp *qp, struct ib_wc *wc)
{
	struct ionic_sq_meta *meta;

	if (!qp->sq_flush)
		return 0;

	if (ionic_queue_empty(&qp->sq))
		return 0;

	meta = &qp->sq_meta[qp->sq.cons];

	ionic_queue_consume(&qp->sq);

	memset(wc, 0, sizeof(*wc));

	wc->status = IB_WC_WR_FLUSH_ERR;
	wc->wr_id = meta->wrid;
	wc->qp = &qp->ibqp;

	return 1;
}

static int ionic_flush_send_many(struct ionic_qp *qp, struct ib_wc *wc, int nwc)
{
	int rc = 0, npolled = 0;

	while(npolled < nwc) {
		rc = ionic_flush_send(qp, wc + npolled);
		if (rc <= 0)
			break;

		npolled += rc;
	}

	return npolled ?: rc;
}

static int ionic_poll_recv(struct ionic_ibdev *dev, struct ionic_cq *cq,
			   struct ionic_qp *cqe_qp, struct ionic_v1_cqe *cqe,
			   struct ib_wc *wc)
{
	struct ionic_qp *qp = NULL;
	struct ionic_rq_meta *meta;
	u32 src_qpn, st_len;
	u16 vlan_tag;
	u8 op;

	if (cqe_qp->rq_flush)
		return 0;

	if (cqe_qp->has_rq) {
		qp = cqe_qp;
	} else {
		if (unlikely(cqe_qp->is_srq))
			return -EIO;

		if (unlikely(!cqe_qp->ibqp.srq))
			return -EIO;

		qp = to_ionic_srq(cqe_qp->ibqp.srq);
	}

	st_len = be32_to_cpu(cqe->status_length);

	/* ignore wqe_id in case of flush error */
	if (ionic_v1_cqe_error(cqe) && st_len == IONIC_STS_WQE_FLUSHED_ERR) {
		/* should only see flushed for rq, never srq, check anyway */
		cqe_qp->rq_flush = !qp->is_srq;
		if (cqe_qp->rq_flush) {
			cq->flush = true;
			list_move_tail(&qp->cq_flush_rq, &cq->flush_rq);
		}

		/* posted recvs (if any) flushed by ionic_flush_recv */
		return 0;
	}

	/* there had better be something in the recv queue to complete */
	if (ionic_queue_empty(&qp->rq))
		return -EIO;

	/* wqe_id must be a valid queue index */
	if (unlikely(cqe->recv.wqe_id >> qp->rq.depth_log2))
		return -EIO;

	/* wqe_id must indicate a request that is outstanding */
	meta = &qp->rq_meta[cqe->recv.wqe_id];
	if (unlikely(meta->next != IONIC_META_POSTED))
		return -EIO;

	meta->next = qp->rq_meta_head;
	qp->rq_meta_head = meta;

	memset(wc, 0, sizeof(*wc));

	wc->wr_id = meta->wrid;

	if (!cqe_qp->is_srq)
		/* XXX possible use-after-free after rcu read unlock */
		wc->qp = &cqe_qp->ibqp;

	if (ionic_v1_cqe_error(cqe)) {
		wc->vendor_err = st_len;
		wc->status = ionic_to_ib_status(st_len);

		cqe_qp->rq_flush = !qp->is_srq;
		if (cqe_qp->rq_flush) {
			cq->flush = true;
			list_move_tail(&qp->cq_flush_rq, &cq->flush_rq);
		}
		goto out;
	}

	wc->vendor_err = 0;
	wc->status = IB_WC_SUCCESS;

	src_qpn = be32_to_cpu(cqe->recv.src_qpn_op);
	op = src_qpn >> IONIC_V1_CQE_RECV_OP_SHIFT;

	src_qpn &= IONIC_V1_CQE_RECV_QPN_MASK;
	op &= IONIC_V1_CQE_RECV_OP_MASK;

	wc->opcode = IB_WC_RECV;
	switch (op) {
	case IONIC_V1_CQE_RECV_OP_RDMA_IMM:
		wc->opcode = IB_WC_RECV_RDMA_WITH_IMM;
		/* fallthrough */
	case IONIC_V1_CQE_RECV_OP_SEND_IMM:
		wc->wc_flags |= IB_WC_WITH_IMM;
		wc->ex.imm_data = cqe->recv.imm_data_rkey; /* be32 in wc */
		break;
	case IONIC_V1_CQE_RECV_OP_SEND_INV:
		wc->wc_flags |= IB_WC_WITH_INVALIDATE;
		wc->ex.invalidate_rkey = be32_to_cpu(cqe->recv.imm_data_rkey);
	}

	wc->byte_len = st_len;
	wc->src_qp = src_qpn;

	if (qp->ibqp.qp_type == IB_QPT_UD ||
	    qp->ibqp.qp_type == IB_QPT_GSI) {
		wc->wc_flags |= IB_WC_GRH | IB_WC_WITH_SMAC;
		ether_addr_copy(wc->smac, cqe->recv.src_mac);

		wc->wc_flags |= IB_WC_WITH_NETWORK_HDR_TYPE;
		if (ionic_v1_cqe_recv_is_ipv4(cqe))
			wc->network_hdr_type = RDMA_NETWORK_IPV4;
		else
			wc->network_hdr_type = RDMA_NETWORK_IPV6;

		if (ionic_v1_cqe_recv_is_vlan(cqe))
			wc->wc_flags |= IB_WC_WITH_VLAN;

		/* vlan_tag in cqe will be valid from dpath even if no vlan */
		vlan_tag = be16_to_cpu(cqe->recv.vlan_tag);
		wc->vlan_id = vlan_tag & 0xfff; /* 802.1q VID */
		wc->sl = vlan_tag >> 13; /* 802.1q PCP */
	}

	wc->pkey_index = 0;
	wc->port_num = 1;

out:
	ionic_queue_consume(&qp->rq);

	return 1;
}

static bool ionic_peek_send(struct ionic_qp *qp)
{
	struct ionic_sq_meta *meta;

	if (qp->sq_flush)
		return false;

	/* completed all send queue requests? */
	if (ionic_queue_empty(&qp->sq))
		return false;

	/* waiting for local completion? */
	if (qp->sq.cons == qp->sq_npg_cons)
		return false;

	meta = &qp->sq_meta[qp->sq.cons];

	/* waiting for remote completion? */
	if (meta->remote && meta->seq == qp->sq_msn_cons)
		return false;

	return true;
}

static int ionic_poll_send(struct ionic_cq *cq, struct ionic_qp *qp,
			   struct ib_wc *wc)
{
	struct ionic_sq_meta *meta;

	if (qp->sq_flush)
		return 0;

	do {
		/* completed all send queue requests? */
		if (ionic_queue_empty(&qp->sq))
			goto out_empty;

		/* waiting for local completion? */
		if (qp->sq.cons == qp->sq_npg_cons)
			goto out_empty;

		meta = &qp->sq_meta[qp->sq.cons];

		/* waiting for remote completion? */
		if (meta->remote && meta->seq == qp->sq_msn_cons)
			goto out_empty;

		ionic_queue_consume(&qp->sq);

		/* produce wc only if signaled or error status */
	} while (!meta->signal && meta->ibsts == IB_WC_SUCCESS);

	memset(wc, 0, sizeof(*wc));

	wc->status = meta->ibsts;
	wc->wr_id = meta->wrid;

	/* XXX possible use-after-free after rcu read unlock */
	wc->qp = &qp->ibqp;

	if (meta->ibsts == IB_WC_SUCCESS) {
		wc->byte_len = meta->len;
		wc->opcode = meta->ibop;
	} else {
		wc->vendor_err = meta->len;

		qp->sq_flush = true;
		cq->flush = true;
		list_move_tail(&qp->cq_flush_sq, &cq->flush_sq);
	}

	return 1;

out_empty:
	if (qp->sq_flush_rcvd) {
		qp->sq_flush = true;
		cq->flush = true;
		list_move_tail(&qp->cq_flush_sq, &cq->flush_sq);
	}
	return 0;
}

static int ionic_poll_send_many(struct ionic_cq *cq, struct ionic_qp *qp,
				struct ib_wc *wc, int nwc)
{
	int rc = 0, npolled = 0;

	while(npolled < nwc) {
		rc = ionic_poll_send(cq, qp, wc + npolled);
		if (rc <= 0)
			break;

		npolled += rc;
	}

	return npolled ?: rc;
}

static int ionic_validate_cons(u16 prod, u16 cons,
			       u16 comp, u16 mask)
{
	if (((prod - cons) & mask) < ((comp - cons) & mask))
		return -EIO;

	return 0;
}

static int ionic_comp_msn(struct ionic_qp *qp, struct ionic_v1_cqe *cqe)
{
	struct ionic_sq_meta *meta;
	u16 cqe_seq, cqe_idx;
	int rc;

	if (qp->sq_flush)
		return 0;

	cqe_seq = be32_to_cpu(cqe->send.msg_msn) & qp->sq.mask;

	if (ionic_v1_cqe_error(cqe))
		cqe_idx = qp->sq_msn_idx[cqe_seq];
	else
		cqe_idx = qp->sq_msn_idx[(cqe_seq - 1) & qp->sq.mask];

	rc = ionic_validate_cons(qp->sq_msn_prod,
				 qp->sq_msn_cons,
				 cqe_seq, qp->sq.mask);
	if (rc)
		return rc;

	qp->sq_msn_cons = cqe_seq;

	if (ionic_v1_cqe_error(cqe)) {
		meta = &qp->sq_meta[cqe_idx];
		meta->len = be32_to_cpu(cqe->status_length);
		meta->ibsts = ionic_to_ib_status(meta->len);
		meta->remote = false;
	}

	/* remote completion coalesces local requests, too */
	cqe_seq = ionic_queue_next(&qp->sq, cqe_idx);
	if (!ionic_validate_cons(qp->sq.prod,
				 qp->sq_npg_cons,
				 cqe_seq, qp->sq.mask))
		qp->sq_npg_cons = cqe_seq;

	return 0;
}

static int ionic_comp_npg(struct ionic_qp *qp, struct ionic_v1_cqe *cqe)
{
	struct ionic_sq_meta *meta;
	u16 cqe_seq, cqe_idx;
	uint32_t st_len;
	int rc;

	if (qp->sq_flush)
		return 0;

	st_len = be32_to_cpu(cqe->status_length);

	if (ionic_v1_cqe_error(cqe) && st_len == IONIC_STS_WQE_FLUSHED_ERR) {
		/* Flush cqe does not consume a wqe on the device, and maybe
		 * no such work request is posted.
		 *
		 * The driver should begin flushing after the last indicated
		 * normal or error completion.  Here, only set a hint that the
		 * flush request was indicated.  In poll_send, if nothing more
		 * can be polled normally, then begin flushing.
		 */
		qp->sq_flush_rcvd = true;
		return 0;
	}

	cqe_idx = cqe->send.npg_wqe_id & qp->sq.mask;
	cqe_seq = ionic_queue_next(&qp->sq, cqe_idx);

	rc = ionic_validate_cons(qp->sq.prod,
				 qp->sq_npg_cons,
				 cqe_seq, qp->sq.mask);
	if (rc)
		return rc;

	qp->sq_npg_cons = cqe_seq;

	if (ionic_v1_cqe_error(cqe)) {
		meta = &qp->sq_meta[cqe_idx];
		meta->len = st_len;
		meta->ibsts = ionic_to_ib_status(st_len);
		meta->remote = false;
	}

	return 0;
}

static void ionic_reserve_sync_cq(struct ionic_ibdev *dev, struct ionic_cq *cq)
{
	if (!ionic_queue_empty(&cq->q)) {
		cq->reserve += ionic_queue_length(&cq->q);
		cq->q.cons = cq->q.prod;

		ionic_dbell_ring(&dev->dbpage[dev->cq_qtype],
				 ionic_queue_dbell_val(&cq->q));
	}
}

static void ionic_reserve_cq(struct ionic_ibdev *dev, struct ionic_cq *cq,
			     int spend)
{
	cq->reserve -= spend;

	if (cq->reserve <= 0)
		ionic_reserve_sync_cq(dev, cq);
}

static int ionic_poll_cq(struct ib_cq *ibcq, int nwc, struct ib_wc *wc)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibcq->device);
	struct ionic_cq *cq = to_ionic_cq(ibcq);
	struct ionic_qp *qp, *qp_next;
	struct ionic_v1_cqe *cqe;
	u32 qtf, qid;
	u8 type;
	u16 old_prod;
	bool peek;
	int rc = 0, npolled = 0;
	unsigned long irqflags;

	/* Note about rc: (noted here because poll is different)
	 *
	 * Functions without "poll" in the name, if they return an integer,
	 * return zero on success, or a positive error number.  Functions
	 * returning a pointer return NULL on error and set errno to a positve
	 * error number.
	 *
	 * Functions with "poll" in the name return negative error numbers, or
	 * greater or equal to zero number of completions on success.
	 */

	if (nwc < 1)
		return 0;

	spin_lock_irqsave(&cq->lock, irqflags);

	old_prod = cq->q.prod;

	/* poll already indicated work completions for send queue */

	list_for_each_entry_safe(qp, qp_next, &cq->poll_sq, cq_poll_sq) {
		if (npolled == nwc)
			goto out;

		spin_lock(&qp->sq_lock);
		rc = ionic_poll_send_many(cq, qp, wc + npolled, nwc - npolled);
		spin_unlock(&qp->sq_lock);

		if (rc > 0)
			npolled += rc;

		if (npolled < nwc)
			list_del_init(&qp->cq_poll_sq);
	}

	/* poll for more work completions */

	while (likely(ionic_next_cqe(cq, &cqe))) {
		if (npolled == nwc)
			goto out;

		qtf = ionic_v1_cqe_qtf(cqe);
		qid = ionic_v1_cqe_qtf_qid(qtf);
		type = ionic_v1_cqe_qtf_type(qtf);

		qp = tbl_lookup(&dev->qp_tbl, qid);

		if (unlikely(!qp)) {
			dev_dbg(&dev->ibdev.dev, "missing qp for qid %u\n", qid);
			goto cq_next;
		}

		switch(type) {
		case IONIC_V1_CQE_TYPE_RECV:
			spin_lock(&qp->rq_lock);
			rc = ionic_poll_recv(dev, cq, qp, cqe, wc + npolled);
			spin_unlock(&qp->rq_lock);

			if (rc < 0)
				goto out;

			npolled += rc;

			break;

		case IONIC_V1_CQE_TYPE_SEND_MSN:
			spin_lock(&qp->sq_lock);
			rc = ionic_comp_msn(qp, cqe);
			if (!rc) {
				rc = ionic_poll_send_many(cq, qp,
							  wc + npolled,
							  nwc - npolled);
				peek = ionic_peek_send(qp);
			}
			spin_unlock(&qp->sq_lock);

			if (rc < 0)
				goto out;

			npolled += rc;

			if (peek)
				list_move_tail(&qp->cq_poll_sq, &cq->poll_sq);
			break;

		case IONIC_V1_CQE_TYPE_SEND_NPG:
			spin_lock(&qp->sq_lock);
			rc = ionic_comp_npg(qp, cqe);
			if (!rc) {
				rc = ionic_poll_send_many(cq, qp,
							  wc + npolled,
							  nwc - npolled);
				peek = ionic_peek_send(qp);
			}
			spin_unlock(&qp->sq_lock);

			if (rc < 0)
				goto out;

			npolled += rc;

			if (peek)
				list_move_tail(&qp->cq_poll_sq, &cq->poll_sq);
			break;

		default:
			dev_dbg(&dev->ibdev.dev,
				"unexpected cqe type %u\n", type);

			rc = -EIO;
			goto out;
		}

cq_next:
		ionic_queue_produce(&cq->q);
		cq->color = ionic_color_wrap(cq->q.prod, cq->color);
	}

	/* lastly, flush send and recv queues */

	if (likely(!cq->flush))
		goto out;

	cq->flush = false;

	list_for_each_entry_safe(qp, qp_next, &cq->flush_sq, cq_flush_sq) {
		if (npolled == nwc)
			goto out;

		spin_lock(&qp->sq_lock);
		rc = ionic_flush_send_many(qp, wc + npolled, nwc - npolled);
		spin_unlock(&qp->sq_lock);

		if (rc > 0)
			npolled += rc;

		if (npolled < nwc)
			list_del_init(&qp->cq_flush_sq);
		else
			cq->flush = true;
	}

	list_for_each_entry_safe(qp, qp_next, &cq->flush_rq, cq_flush_rq) {
		if (npolled == nwc)
			goto out;

		spin_lock(&qp->rq_lock);
		rc = ionic_flush_recv_many(qp, wc + npolled, nwc - npolled);
		spin_unlock(&qp->rq_lock);

		if (rc > 0)
			npolled += rc;

		if (npolled < nwc)
			list_del_init(&qp->cq_flush_rq);
		else
			cq->flush = true;
	}

out:
	/* in case reserve was depleted (more work posted than cq depth) */
	if (cq->reserve <= 0)
		ionic_reserve_sync_cq(dev, cq);

	spin_unlock_irqrestore(&cq->lock, irqflags);

	return npolled ?: rc;
}

static int ionic_req_notify_cq(struct ib_cq *ibcq,
			       enum ib_cq_notify_flags flags)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibcq->device);
	struct ionic_cq *cq = to_ionic_cq(ibcq);
	u64 dbell_val = cq->q.dbell;

	if (flags & IB_CQ_SOLICITED) {
		cq->arm_sol_prod = ionic_queue_next(&cq->q, cq->arm_sol_prod);
		dbell_val |= cq->arm_sol_prod | IONIC_DBELL_RING_ARM_SOLICITED;
	} else {
		cq->arm_any_prod = ionic_queue_next(&cq->q, cq->arm_any_prod);
		dbell_val |= cq->arm_any_prod | IONIC_DBELL_RING_ARM;
	}

	ionic_reserve_sync_cq(dev, cq);

	ionic_dbell_ring(&dev->dbpage[dev->cq_qtype], dbell_val);

	/* IB_CQ_REPORT_MISSED_EVENTS:
	 *
	 * The queue index in ring zero guarantees no missed events.
	 *
	 * Here, we check if the color bit in the next cqe is flipped.  If it
	 * is flipped, then progress can be made by immediately polling the cq.
	 * Sill, the cq will be armed, and an event will be generated.  The cq
	 * may be empty when polled after the event, because the next poll
	 * after arming the cq can empty it.
	 */
	return (flags & IB_CQ_REPORT_MISSED_EVENTS) &&
		cq->color == ionic_v1_cqe_color(ionic_queue_at_prod(&cq->q));
}

static int ionic_v1_create_qp_cmd(struct ionic_ibdev *dev,
				  struct ionic_pd *pd,
				  struct ionic_cq *send_cq,
				  struct ionic_cq *recv_cq,
				  struct ionic_qp *qp,
				  struct ionic_tbl_buf *sq_buf,
				  struct ionic_tbl_buf *rq_buf,
				  struct ib_qp_init_attr *attr)
{
	const u32 flags = to_ionic_qp_flags(0, 0, qp->sq_is_cmb, qp->rq_is_cmb,
					    !pd->ibpd.uobject);
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = IONIC_V1_ADMIN_CREATE_QP,
			.type_state = to_ionic_qp_type(attr->qp_type),
			.dbid_flags = cpu_to_le16(ionic_dbid(dev, pd->ibpd.uobject)),
			.id_ver = cpu_to_le32(qp->qpid),
			.qp = {
				.pd_id = cpu_to_le32(pd->pdid),
				.priv_flags = cpu_to_be32(flags),
			}
		}
	};
	int rc;

	if (qp->has_sq) {
		wr.wqe.qp.sq_cq_id = cpu_to_le32(send_cq->cqid);
		wr.wqe.qp.sq_depth_log2 = qp->sq.depth_log2;
		wr.wqe.qp.sq_stride_log2 = qp->sq.stride_log2;
		wr.wqe.qp.sq_page_size_log2 = sq_buf->page_size_log2;
		wr.wqe.qp.sq_tbl_index_xrcd_id = cpu_to_le32(qp->sq_res.tbl_pos);
		wr.wqe.qp.sq_map_count = cpu_to_le32(sq_buf->tbl_pages);
		wr.wqe.qp.sq_dma_addr = cpu_to_le64(sq_buf->tbl_dma);
	} else if (attr->xrcd) {
		wr.wqe.qp.sq_tbl_index_xrcd_id = 0; /* TODO */
	}

	if (qp->has_rq) {
		wr.wqe.qp.rq_cq_id = cpu_to_le32(recv_cq->cqid);
		wr.wqe.qp.rq_depth_log2 = qp->rq.depth_log2;
		wr.wqe.qp.rq_stride_log2 = qp->rq.stride_log2;
		wr.wqe.qp.rq_page_size_log2 = rq_buf->page_size_log2;
		wr.wqe.qp.rq_tbl_index_srq_id = cpu_to_le32(qp->rq_res.tbl_pos);
		wr.wqe.qp.rq_map_count = cpu_to_le32(rq_buf->tbl_pages);
		wr.wqe.qp.rq_dma_addr = cpu_to_le64(rq_buf->tbl_dma);
	} else if (attr->srq) {
		wr.wqe.qp.rq_tbl_index_srq_id =
			cpu_to_le32(to_ionic_srq(attr->srq)->qpid);
	}

	ionic_admin_post(dev, &wr);
	ionic_admin_wait(dev, &wr);

	if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = -ENODEV;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "cqe error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	return rc;
}

static int ionic_v1_modify_qp_cmd(struct ionic_ibdev *dev,
				  struct ionic_qp *qp,
				  struct ib_qp_attr *attr,
				  int mask)
{
	const u32 flags = to_ionic_qp_flags(attr->qp_access_flags,
					    attr->en_sqd_async_notify,
					    qp->sq_is_cmb, qp->rq_is_cmb,
					    !qp->ibqp.uobject);
	const u8 state = to_ionic_qp_modify_state(attr->qp_state,
						  attr->cur_qp_state);
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = IONIC_V1_ADMIN_MODIFY_QP,
			.type_state = state,
			.id_ver = cpu_to_le32(qp->qpid),
			.mod_qp = {
				.attr_mask = cpu_to_be32(mask),
				.access_flags = cpu_to_be32(flags),
				.rq_psn = attr->rq_psn,
				.sq_psn = attr->sq_psn,
#ifdef HAVE_QP_RATE_LIMIT
				.rate_limit_kbps = cpu_to_le32(attr->rate_limit),
#endif
				.pmtu = (attr->path_mtu + 7), /* XXX add 7 on device */
				.retry = attr->retry_cnt | (attr->rnr_retry << 4),
				.rnr_timer = attr->min_rnr_timer,
				.retry_timeout = attr->timeout,
			}
		}
	};
	struct ib_ud_header *hdr = NULL;
	void *hdr_buf = NULL;
	dma_addr_t hdr_dma = 0;
	int rc, hdr_len = 0;

	if ((mask & IB_QP_MAX_DEST_RD_ATOMIC) && attr->max_dest_rd_atomic) {
		/* Note, round up/down was already done for allocating
		 * resources on the device. The allocation order is in cache
		 * line size.  We can't use the order of the resource
		 * allocation to determine the order wqes here, because for
		 * queue length <= one cache line it is not distinct.
		 *
		 * Therefore, order wqes is computed again here.
		 *
		 * Account for hole and round up to the next order.
		 */
		wr.wqe.mod_qp.rsq_depth =
			order_base_2(attr->max_dest_rd_atomic + 1);
		wr.wqe.mod_qp.rsq_index = cpu_to_le32(qp->rsq_res.tbl_pos);
	}

	if ((mask & IB_QP_MAX_QP_RD_ATOMIC) && attr->max_rd_atomic) {
		/* Account for hole and round down to the next order */
		wr.wqe.mod_qp.rrq_depth =
			order_base_2(attr->max_rd_atomic + 2) - 1;
		wr.wqe.mod_qp.rrq_index = cpu_to_le32(qp->rrq_res.tbl_pos);
	}

	if (qp->ibqp.qp_type == IB_QPT_RC || qp->ibqp.qp_type == IB_QPT_UC)
		wr.wqe.mod_qp.qkey_dest_qpn = cpu_to_le32(attr->dest_qp_num);
	else
		wr.wqe.mod_qp.qkey_dest_qpn = cpu_to_le32(attr->qkey);

	if (mask & IB_QP_AV) {
		hdr = kmalloc(sizeof(*hdr), GFP_KERNEL);
		if (!hdr) {
			rc = -ENOMEM;
			goto err_hdr;
		}

		rc = ionic_build_hdr(dev, hdr, &attr->ah_attr);
		if (rc)
			goto err_buf;

		hdr_buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
		if (!hdr_buf) {
			rc = -ENOMEM;
			goto err_buf;
		}

		hdr_len = ib_ud_header_pack(hdr, hdr_buf);
		hdr_len -= IB_BTH_BYTES;
		hdr_len -= IB_DETH_BYTES;
		hdr_len -= IB_UDP_BYTES;

		if (ionic_xxx_udp)
			hdr_len += IB_UDP_BYTES;

		dev_dbg(&dev->ibdev.dev, "roce packet header template\n");
		print_hex_dump_debug("hdr ", DUMP_PREFIX_OFFSET, 16, 1,
				     hdr_buf, hdr_len, true);

		hdr_dma = dma_map_single(dev->hwdev, hdr_buf, hdr_len,
					 DMA_TO_DEVICE);

		rc = dma_mapping_error(dev->hwdev, hdr_dma);
		if (rc)
			goto err_dma;

		wr.wqe.mod_qp.ah_id_len = cpu_to_le32(qp->ahid | (hdr_len << 24));
		wr.wqe.mod_qp.dma_addr = hdr_dma;

		wr.wqe.mod_qp.en_pcp = attr->ah_attr.sl;
		wr.wqe.mod_qp.ip_dscp =
			rdma_ah_read_grh(&attr->ah_attr)->traffic_class >> 2;
	}

	ionic_admin_post(dev, &wr);
	ionic_admin_wait(dev, &wr);

	if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = -ENODEV;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "cqe error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	if (mask & IB_QP_AV)
		dma_unmap_single(dev->hwdev, hdr_dma, hdr_len,
				 DMA_TO_DEVICE);
err_dma:
	if (mask & IB_QP_AV)
		kfree(hdr_buf);
err_buf:
	if (mask & IB_QP_AV)
		kfree(hdr);
err_hdr:
	return rc;
}

static int ionic_v1_destroy_qp_cmd(struct ionic_ibdev *dev, u32 qpid)
{
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = IONIC_V1_ADMIN_DESTROY_QP,
			.id_ver = cpu_to_le32(qpid),
		}
	};
	int rc;

	ionic_admin_post(dev, &wr);
	ionic_admin_wait(dev, &wr);

	if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = 0;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "cqe error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	return rc;
}

static int ionic_create_qp_cmd(struct ionic_ibdev *dev,
			       struct ionic_pd *pd,
			       struct ionic_cq *send_cq,
			       struct ionic_cq *recv_cq,
			       struct ionic_qp *qp,
			       struct ionic_tbl_buf *sq_buf,
			       struct ionic_tbl_buf *rq_buf,
			       struct ib_qp_init_attr *attr)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_CREATE_QP)
			return ionic_v1_create_qp_cmd(dev, pd, send_cq, recv_cq,
						      qp, sq_buf, rq_buf, attr);
		return -ENOSYS;
	default:
		return -ENOSYS;
	}
}

static int ionic_modify_qp_cmd(struct ionic_ibdev *dev,
			       struct ionic_qp *qp,
			       struct ib_qp_attr *attr,
			       int mask)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_MODIFY_QP)
			return ionic_v1_modify_qp_cmd(dev, qp, attr, mask);
		return -ENOSYS;
	default:
		return -ENOSYS;
	}
}

static int ionic_destroy_qp_cmd(struct ionic_ibdev *dev, u32 qpid)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_DESTROY_QP)
			return ionic_v1_destroy_qp_cmd(dev, qpid);
		return -ENOSYS;
	default:
		return -ENOSYS;
	}
}

static void ionic_qp_sq_init_cmb(struct ionic_ibdev *dev,
				 struct ionic_ctx *ctx,
				 struct ionic_qp *qp,
				 bool cap_inline)
{
	int rc;

	if (!qp->has_sq)
		goto err_cmb;

	if (ionic_sqcmb_inline && !cap_inline)
		goto err_cmb;

	qp->sq_cmb_order = order_base_2(qp->sq.size / PAGE_SIZE);

	if (qp->sq_cmb_order >= ionic_sqcmb_order)
		goto err_cmb;

	rc = ionic_api_get_cmb(dev->lif, &qp->sq_cmb_pgid,
			       &qp->sq_cmb_addr, qp->sq_cmb_order);
	if (rc)
		goto err_cmb;

	qp->sq_cmb_ptr = ioremap_wc(qp->sq_cmb_addr, qp->sq.size);
	if (!qp->sq_cmb_ptr)
		goto err_map;

	memset_io(qp->sq_cmb_ptr, 0, qp->sq.size);

	if (ctx) {
		iounmap(qp->sq_cmb_ptr);
		qp->sq_cmb_ptr = NULL;
	}

	qp->sq_is_cmb = true;

	return;

err_map:
	ionic_api_put_cmb(dev->lif, qp->sq_cmb_pgid, qp->sq_cmb_order);
err_cmb:
	qp->sq_is_cmb = false;
	qp->sq_cmb_ptr = NULL;
	qp->sq_cmb_order = IONIC_RES_INVALID;
	qp->sq_cmb_pgid = 0;
	qp->sq_cmb_addr = 0;

	qp->sq_cmb_mmap.offset = 0;
	qp->sq_cmb_mmap.size = 0;
	qp->sq_cmb_mmap.pfn = 0;
}

static void ionic_qp_sq_destroy_cmb(struct ionic_ibdev *dev,
				    struct ionic_ctx *ctx,
				    struct ionic_qp *qp)
{
	if (!qp->sq_is_cmb)
		return;

	if (ctx) {
		mutex_lock(&ctx->mmap_mut);
		list_del(&qp->sq_cmb_mmap.ctx_ent);
		mutex_unlock(&ctx->mmap_mut);
	} else {
		iounmap(qp->sq_cmb_ptr);
	}

	ionic_api_put_cmb(dev->lif, qp->sq_cmb_pgid, qp->sq_cmb_order);
}

static int ionic_qp_sq_init(struct ionic_ibdev *dev, struct ionic_ctx *ctx,
			    struct ionic_qp *qp, struct ionic_qdesc *sq,
			    struct ionic_tbl_buf *buf, int max_wr, int max_sge,
			    int max_data)
{
	int rc = 0;
	u32 wqe_size;

	qp->sq_msn_prod = 0;
	qp->sq_msn_cons = 0;
	qp->sq_npg_cons = 0;
	qp->sq_cmb_prod = 0;

	INIT_LIST_HEAD(&qp->sq_cmb_mmap.ctx_ent);

	if (!qp->has_sq) {
		if (ctx)
			rc = ionic_validate_qdesc_zero(sq);

		return rc;
	}

	rc = -EINVAL;
	if (max_wr < 1 || max_wr > 0xffff)
		goto err_sq;
	if (max_sge < 1 || max_sge > ionic_v1_send_wqe_max_sge(dev->max_stride))
		goto err_sq;
	if (max_data < 0 || max_data > ionic_v1_send_wqe_max_data(dev->max_stride))
		goto err_sq;

	if (ctx) {
		rc = ionic_validate_qdesc(sq);
		if (rc)
			goto err_sq;

		qp->sq.ptr = NULL;
		qp->sq.size = sq->size;
		qp->sq.mask = sq->mask;
		qp->sq.depth_log2 = sq->depth_log2;
		qp->sq.stride_log2 = sq->stride_log2;

		qp->sq_meta = NULL;
		qp->sq_msn_idx = NULL;

		qp->sq_umem = ib_umem_get(&ctx->ibctx, sq->addr,
					  sq->size, 0, 0);
		if (IS_ERR(qp->sq_umem)) {
			rc = PTR_ERR(qp->sq_umem);
			goto err_sq;
		}
	} else {
		qp->sq_umem = NULL;

		wqe_size = ionic_v1_send_wqe_min_size(max_sge, max_data);
		rc = ionic_queue_init(&qp->sq, dev->hwdev,
				      max_wr, wqe_size);
		if (rc)
			goto err_sq;

		ionic_queue_dbell_init(&qp->sq, qp->qpid);

		qp->sq_meta = kmalloc_array((u32)qp->sq.mask + 1,
					    sizeof(*qp->sq_meta),
					    GFP_KERNEL);
		if (!qp->sq_meta) {
			rc = -ENOMEM;
			goto err_sq_meta;
		}

		qp->sq_msn_idx = kmalloc_array((u32)qp->sq.mask + 1,
					       sizeof(*qp->sq_msn_idx),
					       GFP_KERNEL);
		if (!qp->sq_msn_idx) {
			rc = -ENOMEM;
			goto err_sq_msn;
		}
	}

	ionic_qp_sq_init_cmb(dev, ctx, qp, max_data > 0);
	if (qp->sq_is_cmb)
		rc = ionic_pgtbl_init(dev, &qp->sq_res, buf, NULL,
				      (u64)qp->sq_cmb_pgid << PAGE_SHIFT, 1);
	else
		rc = ionic_pgtbl_init(dev, &qp->sq_res, buf,
				      qp->sq_umem, qp->sq.dma, 1);
	if (rc)
		goto err_sq_tbl;

	return 0;

err_sq_tbl:
	ionic_qp_sq_destroy_cmb(dev, ctx, qp);

	if (qp->sq_msn_idx)
		kfree(qp->sq_msn_idx);
err_sq_msn:
	if (qp->sq_meta)
		kfree(qp->sq_meta);
err_sq_meta:
	if (qp->sq_umem)
		ib_umem_release(qp->sq_umem);
	else
		ionic_queue_destroy(&qp->sq, dev->hwdev);
err_sq:
	return rc;
}

static void ionic_qp_sq_destroy(struct ionic_ibdev *dev,
				struct ionic_ctx *ctx,
				struct ionic_qp *qp)
{
	if (!qp->has_sq)
		return;

	ionic_put_res(dev, &qp->sq_res);

	ionic_qp_sq_destroy_cmb(dev, ctx, qp);

	if (qp->sq_msn_idx)
		kfree(qp->sq_msn_idx);

	if (qp->sq_meta)
		kfree(qp->sq_meta);

	if (qp->sq_umem)
		ib_umem_release(qp->sq_umem);
	else
		ionic_queue_destroy(&qp->sq, dev->hwdev);
}

static void ionic_qp_rq_init_cmb(struct ionic_ibdev *dev,
				 struct ionic_ctx *ctx,
				 struct ionic_qp *qp)
{
	int rc;

	if (!qp->has_rq)
		goto err_cmb;

	if (ionic_rqcmb_sqcmb && !qp->sq_is_cmb)
		goto err_cmb;

	qp->rq_cmb_order = order_base_2(qp->rq.size / PAGE_SIZE);

	if (qp->rq_cmb_order >= ionic_rqcmb_order)
		goto err_cmb;

	rc = ionic_api_get_cmb(dev->lif, &qp->rq_cmb_pgid,
			       &qp->rq_cmb_addr, qp->rq_cmb_order);
	if (rc)
		goto err_cmb;

	qp->rq_cmb_ptr = ioremap_wc(qp->rq_cmb_addr, qp->rq.size);
	if (!qp->rq_cmb_ptr)
		goto err_map;

	memset_io(qp->rq_cmb_ptr, 0, qp->rq.size);

	if (ctx) {
		iounmap(qp->rq_cmb_ptr);
		qp->rq_cmb_ptr = NULL;
	}

	qp->rq_is_cmb = true;

	return;

err_map:
	ionic_api_put_cmb(dev->lif, qp->rq_cmb_pgid, qp->rq_cmb_order);
err_cmb:
	qp->rq_is_cmb = false;
	qp->rq_cmb_ptr = NULL;
	qp->rq_cmb_order = IONIC_RES_INVALID;
	qp->rq_cmb_pgid = 0;
	qp->rq_cmb_addr = 0;

	qp->rq_cmb_mmap.offset = 0;
	qp->rq_cmb_mmap.size = 0;
	qp->rq_cmb_mmap.pfn = 0;
}

static void ionic_qp_rq_destroy_cmb(struct ionic_ibdev *dev,
				    struct ionic_ctx *ctx,
				    struct ionic_qp *qp)
{
	if (!qp->rq_is_cmb)
		return;

	if (ctx) {
		mutex_lock(&ctx->mmap_mut);
		list_del(&qp->rq_cmb_mmap.ctx_ent);
		mutex_unlock(&ctx->mmap_mut);
	} else {
		iounmap(qp->rq_cmb_ptr);
	}

	ionic_api_put_cmb(dev->lif, qp->rq_cmb_pgid, qp->rq_cmb_order);
}

static int ionic_qp_rq_init(struct ionic_ibdev *dev, struct ionic_ctx *ctx,
			    struct ionic_qp *qp, struct ionic_qdesc *rq,
			    struct ionic_tbl_buf *buf, int max_wr, int max_sge)
{
	u32 wqe_size;
	int rc = 0, i;

	qp->rq_cmb_prod = 0;

	INIT_LIST_HEAD(&qp->rq_cmb_mmap.ctx_ent);

	if (!qp->has_rq) {
		if (ctx)
			rc = ionic_validate_qdesc_zero(rq);

		return rc;
	}

	rc = -EINVAL;
	if (max_wr < 1 || max_wr > 0xffff)
		goto err_rq;
	if (max_sge < 1 || max_sge > ionic_v1_recv_wqe_max_sge(dev->max_stride))
		goto err_rq;

	if (ctx) {
		rc = ionic_validate_qdesc(rq);
		if (rc)
			goto err_rq;

		qp->rq.ptr = NULL;
		qp->rq.size = rq->size;
		qp->rq.mask = rq->mask;
		qp->rq.depth_log2 = rq->depth_log2;
		qp->rq.stride_log2 = rq->stride_log2;

		qp->rq_meta = NULL;

		qp->rq_umem = ib_umem_get(&ctx->ibctx, rq->addr,
					  rq->size, 0, 0);
		if (IS_ERR(qp->rq_umem)) {
			rc = PTR_ERR(qp->rq_umem);
			goto err_rq;
		}
	} else {
		qp->rq_umem = NULL;
		qp->rq_res.tbl_order = IONIC_RES_INVALID;
		qp->rq_res.tbl_pos = 0;

		wqe_size = ionic_v1_recv_wqe_min_size(max_sge);
		rc = ionic_queue_init(&qp->rq, dev->hwdev,
				      max_wr, wqe_size);
		if (rc)
			goto err_rq;

		ionic_queue_dbell_init(&qp->rq, qp->qpid);

		qp->rq_meta = kmalloc_array((u32)qp->rq.mask + 1,
					    sizeof(*qp->rq_meta),
					    GFP_KERNEL);
		if (!qp->rq_meta) {
			rc = -ENOMEM;
			goto err_rq_meta;
		}

		for (i = 0; i < qp->rq.mask; ++i)
			qp->rq_meta[i].next = &qp->rq_meta[i + 1];
		qp->rq_meta[i].next = IONIC_META_LAST;
		qp->rq_meta_head = &qp->rq_meta[0];
	}

	ionic_qp_rq_init_cmb(dev, ctx, qp);
	if (qp->rq_is_cmb)
		rc = ionic_pgtbl_init(dev, &qp->rq_res, buf, NULL,
				      (u64)qp->rq_cmb_pgid << PAGE_SHIFT, 1);
	else
		rc = ionic_pgtbl_init(dev, &qp->rq_res, buf,
				      qp->rq_umem, qp->rq.dma, 1);
	if (rc)
		goto err_rq_tbl;

	return 0;

err_rq_tbl:
	ionic_qp_rq_destroy_cmb(dev, ctx, qp);

	if (qp->rq_meta)
		kfree(qp->rq_meta);
err_rq_meta:
	if (qp->rq_umem)
		ib_umem_release(qp->rq_umem);
	else
		ionic_queue_destroy(&qp->rq, dev->hwdev);
err_rq:

	return rc;
}

static void ionic_qp_rq_destroy(struct ionic_ibdev *dev,
				struct ionic_ctx *ctx,
				struct ionic_qp *qp)
{
	if (!qp->has_rq)
		return;

	ionic_put_res(dev, &qp->rq_res);

	ionic_qp_rq_destroy_cmb(dev, ctx, qp);

	if (qp->rq_meta)
		kfree(qp->rq_meta);

	if (qp->rq_umem)
		ib_umem_release(qp->rq_umem);
	else
		ionic_queue_destroy(&qp->rq, dev->hwdev);
}

static struct ib_qp *ionic_create_qp(struct ib_pd *ibpd,
				     struct ib_qp_init_attr *attr,
				     struct ib_udata *udata)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibpd->device);
	struct ionic_ctx *ctx = to_ionic_ctx_uobj(ibpd->uobject);
	struct ionic_pd *pd = to_ionic_pd(ibpd);
	struct ionic_qp *qp;
	struct ionic_cq *cq;
	struct ionic_qp_req req;
	struct ionic_qp_resp resp = {0};
	struct ionic_tbl_buf sq_buf, rq_buf;
	unsigned long irqflags;
	int rc;

	if (!ctx) {
		rc = ionic_validate_udata(udata, 0, 0);
	} else {
		rc = ionic_validate_udata(udata, sizeof(req), sizeof(resp));
		if (!rc)
			rc = ib_copy_from_udata(&req, udata, sizeof(req));
	}

	if (rc)
		goto err_qp;

	qp = kzalloc(sizeof(*qp), GFP_KERNEL);
	if (!qp) {
		rc = -ENOMEM;
		goto err_qp;
	}

	qp->state = IB_QPS_RESET;

	INIT_LIST_HEAD(&qp->cq_poll_sq);
	INIT_LIST_HEAD(&qp->cq_flush_sq);
	INIT_LIST_HEAD(&qp->cq_flush_rq);

	spin_lock_init(&qp->sq_lock);
	spin_lock_init(&qp->rq_lock);

	if (attr->qp_type == IB_QPT_SMI) {
		rc = -EINVAL;
		goto err_qp;
	}

	if (attr->qp_type > IB_QPT_UD) {
		rc = -EINVAL;
		goto err_qp;
	}

	if (attr->qp_type == IB_QPT_GSI)
		rc = ionic_get_gsi_qpid(dev, &qp->qpid);
	else
		rc = ionic_get_qpid(dev, &qp->qpid);
	if (rc)
		goto err_qpid;

	qp->has_ah = attr->qp_type == IB_QPT_RC;

	qp->has_sq = attr->qp_type != IB_QPT_XRC_TGT;

	qp->has_rq = !attr->srq &&
		attr->qp_type != IB_QPT_XRC_INI &&
		attr->qp_type != IB_QPT_XRC_TGT;

	qp->is_srq = false;

	if (qp->has_ah) {
		rc = ionic_get_ahid(dev, &qp->ahid);
		if (rc)
			goto err_ahid;
	}

	qp->rrq_res.tbl_order = IONIC_RES_INVALID;
	qp->rrq_res.tbl_pos = 0;
	qp->rsq_res.tbl_order = IONIC_RES_INVALID;
	qp->rsq_res.tbl_pos = 0;

	rc = ionic_qp_sq_init(dev, ctx, qp, &req.sq, &sq_buf,
			      attr->cap.max_send_wr, attr->cap.max_send_sge,
			      attr->cap.max_inline_data);
	if (rc)
		goto err_sq;

	rc = ionic_qp_rq_init(dev, ctx, qp, &req.rq, &rq_buf,
			      attr->cap.max_recv_wr, attr->cap.max_recv_sge);
	if (rc)
		goto err_rq;

	rc = ionic_create_qp_cmd(dev, pd,
				 to_ionic_cq(attr->send_cq),
				 to_ionic_cq(attr->recv_cq),
				 qp, &sq_buf, &rq_buf, attr);
	if (rc)
		goto err_cmd;

	if (ctx) {
		resp.qpid = qp->qpid;

		if (qp->sq_is_cmb) {
			qp->sq_cmb_mmap.size = qp->sq.size;
			qp->sq_cmb_mmap.pfn = PHYS_PFN(qp->sq_cmb_addr);
			qp->sq_cmb_mmap.writecombine = true;

			mutex_lock(&ctx->mmap_mut);
			qp->sq_cmb_mmap.offset = ctx->mmap_off;
			ctx->mmap_off += qp->sq.size;
			list_add(&qp->sq_cmb_mmap.ctx_ent, &ctx->mmap_list);
			mutex_unlock(&ctx->mmap_mut);

			resp.sq_cmb_offset = qp->sq_cmb_mmap.offset;
		}

		if (qp->rq_is_cmb) {
			qp->rq_cmb_mmap.size = qp->rq.size;
			qp->rq_cmb_mmap.pfn = PHYS_PFN(qp->rq_cmb_addr);
			qp->rq_cmb_mmap.writecombine = true;

			mutex_lock(&ctx->mmap_mut);
			qp->rq_cmb_mmap.offset = ctx->mmap_off;
			ctx->mmap_off += qp->rq.size;
			list_add(&qp->rq_cmb_mmap.ctx_ent, &ctx->mmap_list);
			mutex_unlock(&ctx->mmap_mut);

			resp.rq_cmb_offset = qp->rq_cmb_mmap.offset;
		}

		rc = ib_copy_to_udata(udata, &resp, sizeof(resp));
		if (rc)
			goto err_resp;
	}

	ionic_pgtbl_unbuf(dev, &rq_buf);
	ionic_pgtbl_unbuf(dev, &sq_buf);

	qp->ibqp.qp_num = qp->qpid;

	ionic_dbgfs_add_qp(dev, qp);

	mutex_lock(&dev->tbl_lock);
	tbl_alloc_node(&dev->qp_tbl);
	tbl_insert(&dev->qp_tbl, qp, qp->qpid);
	mutex_unlock(&dev->tbl_lock);

	if (attr->send_cq) {
		cq = to_ionic_cq(attr->send_cq);
		spin_lock_irqsave(&cq->lock, irqflags);
		spin_unlock_irqrestore(&cq->lock, irqflags);
	}

	if (attr->recv_cq) {
		cq = to_ionic_cq(attr->recv_cq);
		spin_lock_irqsave(&cq->lock, irqflags);
		spin_unlock_irqrestore(&cq->lock, irqflags);
	}

	return &qp->ibqp;

err_resp:
	ionic_destroy_qp_cmd(dev, qp->qpid);
err_cmd:
	ionic_pgtbl_unbuf(dev, &rq_buf);
	ionic_qp_rq_destroy(dev, ctx, qp);
err_rq:
	ionic_pgtbl_unbuf(dev, &sq_buf);
	ionic_qp_sq_destroy(dev, ctx, qp);
err_sq:
	if (qp->has_ah)
		ionic_put_ahid(dev, qp->ahid);
err_ahid:
	ionic_put_qpid(dev, qp->qpid);
err_qpid:
	kfree(qp);
err_qp:
	return ERR_PTR(rc);
}

static void ionic_reset_qp(struct ionic_qp *qp)
{
	struct ionic_cq *cq;
	unsigned long irqflags;

	if (qp->ibqp.send_cq) {
		cq = to_ionic_cq(qp->ibqp.send_cq);
		spin_lock_irqsave(&cq->lock, irqflags);
		ionic_clean_cq(cq, qp->qpid);
		spin_unlock_irqrestore(&cq->lock, irqflags);
	}

	if (qp->ibqp.recv_cq) {
		cq = to_ionic_cq(qp->ibqp.recv_cq);
		spin_lock_irqsave(&cq->lock, irqflags);
		ionic_clean_cq(cq, qp->qpid);
		spin_unlock_irqrestore(&cq->lock, irqflags);
	}

	if (qp->has_sq) {
		spin_lock_irqsave(&qp->sq_lock, irqflags);
		qp->sq_flush = false;
		qp->sq_flush_rcvd = false;
		qp->sq_msn_prod = 0;
		qp->sq_msn_cons = 0;
		qp->sq_npg_cons = 0;
		qp->sq_cmb_prod = 0;
		qp->sq.prod = 0;
		qp->sq.cons = 0;
		spin_unlock_irqrestore(&qp->sq_lock, irqflags);
	}

	if (qp->has_rq) {
		spin_lock_irqsave(&qp->rq_lock, irqflags);
		qp->rq_flush = false;
		qp->rq_cmb_prod = 0;
		qp->rq.prod = 0;
		qp->rq.cons = 0;
		spin_unlock_irqrestore(&qp->rq_lock, irqflags);
	}
}

static int ionic_check_modify_qp(struct ionic_qp *qp, struct ib_qp_attr *attr,
				 int mask)
{
	enum ib_qp_state cur_state = (mask & IB_QP_CUR_STATE) ?
		attr->cur_qp_state : qp->state;
	enum ib_qp_state next_state = (mask & IB_QP_STATE) ?
		attr->qp_state : cur_state;

	if (!ib_modify_qp_is_ok(cur_state, next_state, qp->ibqp.qp_type, mask))
		return -EINVAL;

	/* unprivileged qp not allowed priveleged qkey */
	if ((mask & IB_QP_QKEY) && (attr->qkey & 0x80000000) &&
	    qp->ibqp.uobject)
		return -EPERM;

	return 0;
}

static int ionic_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
			   int mask, struct ib_udata *udata)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibqp->device);
	struct ionic_qp *qp = to_ionic_qp(ibqp);
	unsigned long max_rsrq;
	int rc;

	rc = ionic_validate_udata(udata, 0, 0);
	if (rc)
		goto err_qp;

	rc = ionic_check_modify_qp(qp, attr, mask);
	if (rc)
		goto err_qp;

	if (mask & IB_QP_CAP) {
		rc = -EINVAL;
		goto err_qp;
	}

	if ((mask & IB_QP_MAX_QP_RD_ATOMIC) && attr->max_rd_atomic) {
		WARN_ON(ionic_put_res(dev, &qp->rrq_res));

		/* Account for hole and round down to the next order */
		max_rsrq = attr->max_rd_atomic;
		max_rsrq = rounddown_pow_of_two(max_rsrq + 1);

		qp->rrq_res.tbl_order = ionic_res_order(max_rsrq,
							dev->rrq_stride,
							dev->cl_stride);

		rc = ionic_get_res(dev, &qp->rrq_res);
		if (rc)
			goto err_qp;
	}

	if ((mask & IB_QP_MAX_DEST_RD_ATOMIC) && attr->max_dest_rd_atomic) {
		WARN_ON(ionic_put_res(dev, &qp->rsq_res));

		/* Account for hole and round up to the next order */
		max_rsrq = attr->max_dest_rd_atomic;
		max_rsrq = roundup_pow_of_two(max_rsrq + 1);

		qp->rsq_res.tbl_order = ionic_res_order(max_rsrq,
							dev->rsq_stride,
							dev->cl_stride);

		rc = ionic_get_res(dev, &qp->rsq_res);
		if (rc)
			goto err_qp;
	}

	rc = ionic_modify_qp_cmd(dev, qp, attr, mask);
	if (rc)
		goto err_qp;

	if (mask & IB_QP_STATE) {
		qp->state = attr->qp_state;

		if (attr->qp_state == IB_QPS_RESET) {
			ionic_reset_qp(qp);

			ionic_put_res(dev, &qp->rrq_res);
			ionic_put_res(dev, &qp->rsq_res);
		}
	}

	return 0;

err_qp:
	if (mask & IB_QP_MAX_QP_RD_ATOMIC)
		ionic_put_res(dev, &qp->rrq_res);

	if (mask & IB_QP_MAX_DEST_RD_ATOMIC)
		ionic_put_res(dev, &qp->rsq_res);

	return rc;
}

static int ionic_v1_query_qp_cmd(struct ionic_ibdev *dev,
				 struct ionic_qp *qp,
				 struct ib_qp_attr *attr)
{
	struct ionic_admin_wr wr = {
		.work = COMPLETION_INITIALIZER_ONSTACK(wr.work),
		.wqe = {
			.op = IONIC_V1_ADMIN_QUERY_QP,
			.id_ver = cpu_to_le32(qp->qpid),
		}
	};
	struct ionic_v1_admin_query_qp *query_buf;
	dma_addr_t query_dma;
	int flags, rc;

	attr->cap.max_send_sge =
		ionic_v1_send_wqe_max_sge(qp->sq.stride_log2);
	attr->cap.max_recv_sge =
		ionic_v1_recv_wqe_max_sge(qp->rq.stride_log2);
	attr->cap.max_inline_data =
		ionic_v1_send_wqe_max_data(qp->sq.stride_log2);

	query_buf = kmalloc(sizeof(*query_buf), GFP_KERNEL);
	if (!query_buf) {
		rc = -ENOMEM;
		goto err_buf;
	}

	query_dma = dma_map_single(dev->hwdev, query_buf, sizeof(*query_buf),
				   DMA_FROM_DEVICE);
	rc = dma_mapping_error(dev->hwdev, query_dma);
	if (rc)
		goto err_dma;

	wr.wqe.query.dma_addr = cpu_to_le64(query_dma);

	ionic_admin_post(dev, &wr);
	ionic_admin_wait(dev, &wr);

	if (wr.status == IONIC_ADMIN_KILLED) {
		dev_dbg(&dev->ibdev.dev, "killed\n");
		rc = -ENODEV;
	} else if (ionic_v1_cqe_error(&wr.cqe)) {
		dev_warn(&dev->ibdev.dev, "cqe error %u\n",
			 be32_to_cpu(wr.cqe.status_length));
		rc = -EINVAL;
	} else {
		rc = 0;
	}

	dma_unmap_single(dev->hwdev, query_dma, sizeof(*query_buf),
			 DMA_FROM_DEVICE);

	if (rc)
		goto err_dma;

	flags = le32_to_cpu(query_buf->access_perms_flags);

	attr->qp_state = from_ionic_qp_state(query_buf->state_pmtu & 0xf);
	attr->cur_qp_state = attr->qp_state;
	attr->path_mtu = query_buf->state_pmtu >> 4;
	attr->path_mig_state = IB_MIG_MIGRATED;
	attr->qkey = le32_to_cpu(query_buf->qkey_dest_qpn);
	attr->rq_psn = le32_to_cpu(query_buf->rq_psn);
	attr->sq_psn = le32_to_cpu(query_buf->sq_psn);
	attr->dest_qp_num = attr->qkey;
	attr->qp_access_flags = from_ionic_qp_flags(flags);
	attr->pkey_index = 0;
	attr->alt_pkey_index = 0;
	attr->en_sqd_async_notify = !!(flags & IONIC_QPF_SQD_NOTIFY);
	attr->sq_draining = !!(flags & IONIC_QPF_SQ_DRAINING);
	attr->max_rd_atomic = BIT(query_buf->rrq_depth) - 1;
	attr->max_dest_rd_atomic = BIT(query_buf->rsq_depth) - 1;
	attr->min_rnr_timer = query_buf->rnr_timer;
	attr->port_num = 0;
	attr->timeout = query_buf->retry_timeout;
	attr->retry_cnt = query_buf->retry_rnrtry & 0xf;
	attr->rnr_retry = query_buf->retry_rnrtry >> 4;
	attr->alt_port_num = 0;
	attr->alt_timeout = 0;
#ifdef HAVE_QP_RATE_LIMIT
	attr->rate_limit = le32_to_cpu(query_buf->rate_limit_kbps);
#endif

err_dma:
	kfree(query_buf);
err_buf:
	return rc;
}

static int ionic_query_qp_cmd(struct ionic_ibdev *dev,
			      struct ionic_qp *qp,
			      struct ib_qp_attr *attr)
{
	switch (dev->rdma_version) {
	case 1:
		if (dev->admin_opcodes > IONIC_V1_ADMIN_QUERY_QP)
			return ionic_v1_query_qp_cmd(dev, qp, attr);
		return -ENOSYS;
	default:
		return -ENOSYS;
	}
}

static int ionic_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
			  int mask, struct ib_qp_init_attr *init_attr)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibqp->device);
	struct ionic_qp *qp = to_ionic_qp(ibqp);
	int rc;

	memset(attr, 0, sizeof(*attr));
	memset(init_attr, 0, sizeof(*init_attr));

	rc = ionic_query_qp_cmd(dev, qp, attr);
	if (rc)
		goto err_cmd;

	attr->cap.max_send_wr = qp->sq.mask;
	attr->cap.max_recv_wr = qp->rq.mask;

	init_attr->event_handler = ibqp->event_handler;
	init_attr->qp_context = ibqp->qp_context;
	init_attr->send_cq = ibqp->send_cq;
	init_attr->recv_cq = ibqp->recv_cq;
	init_attr->srq = ibqp->srq;
	init_attr->xrcd = ibqp->xrcd;
	init_attr->cap = attr->cap;
	init_attr->sq_sig_type = qp->sig_all ?
		IB_SIGNAL_ALL_WR : IB_SIGNAL_REQ_WR;
	init_attr->qp_type = ibqp->qp_type;
	init_attr->create_flags = 0;
	init_attr->port_num = 0;
	init_attr->rwq_ind_tbl = ibqp->rwq_ind_tbl;
#ifdef HAVE_QP_INIT_SRC_QPN
	init_attr->source_qpn = 0;
#endif

err_cmd:
	return rc;
}

static int ionic_destroy_qp(struct ib_qp *ibqp)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibqp->device);
	struct ionic_ctx *ctx = to_ionic_ctx_uobj(ibqp->uobject);
	struct ionic_qp *qp = to_ionic_qp(ibqp);
	struct ionic_cq *cq;
	unsigned long irqflags;
	int rc;

	rc = ionic_destroy_qp_cmd(dev, qp->qpid);
	if (rc)
		return rc;

	mutex_lock(&dev->tbl_lock);
	tbl_free_node(&dev->qp_tbl);
	tbl_delete(&dev->qp_tbl, qp->qpid);
	mutex_unlock(&dev->tbl_lock);

	if (qp->ibqp.send_cq) {
		cq = to_ionic_cq(qp->ibqp.send_cq);
		spin_lock_irqsave(&cq->lock, irqflags);
		ionic_clean_cq(cq, qp->qpid);
		list_del(&qp->cq_poll_sq);
		list_del(&qp->cq_flush_sq);
		spin_unlock_irqrestore(&cq->lock, irqflags);
	}

	if (qp->ibqp.recv_cq) {
		cq = to_ionic_cq(qp->ibqp.recv_cq);
		spin_lock_irqsave(&cq->lock, irqflags);
		ionic_clean_cq(cq, qp->qpid);
		list_del(&qp->cq_flush_rq);
		spin_unlock_irqrestore(&cq->lock, irqflags);
	}

	ionic_dbgfs_rm_qp(qp);

	ionic_qp_rq_destroy(dev, ctx, qp);
	ionic_qp_sq_destroy(dev, ctx, qp);
	if (qp->has_ah)
		ionic_put_ahid(dev, qp->ahid);
	ionic_put_qpid(dev, qp->qpid);

	ionic_put_res(dev, &qp->rrq_res);
	ionic_put_res(dev, &qp->rsq_res);

	kfree(qp);

	return 0;
}

static s64 ionic_prep_inline(void *data, u32 max_data,
			     const struct ib_sge *ib_sgl, int num_sge)
{
	static const s64 bit_31 = 1u << 31;
	s64 len = 0, sg_len;
	int sg_i;

	for (sg_i = 0; sg_i < num_sge; ++sg_i) {
		sg_len = ib_sgl[sg_i].length;

		/* sge length zero means 2GB */
		if (unlikely(sg_len == 0))
			sg_len = bit_31;

		/* greater than max inline data is invalid */
		if (unlikely(len + sg_len > max_data))
			return -EINVAL;

		memcpy(data + len, (void *)ib_sgl[sg_i].addr, sg_len);

		len += sg_len;
	}

	return len;
}

static s64 ionic_prep_sgl(struct ionic_sge *sgl, u32 max_sge,
			  const struct ib_sge *ib_sgl, int num_sge)
{
	static const s64 bit_31 = 1l << 31;
	s64 len = 0, sg_len;
	int sg_i;

	if (unlikely(num_sge < 0 || (u32)num_sge > max_sge))
		return -EINVAL;

	for (sg_i = 0; sg_i < num_sge; ++sg_i) {
		sg_len = ib_sgl[sg_i].length;

		/* sge length zero means 2GB */
		if (unlikely(sg_len == 0))
			sg_len = bit_31;

		/* greater than 2GB data is invalid */
		if (unlikely(len + sg_len > bit_31))
			return -EINVAL;

		sgl[sg_i].va = cpu_to_be64(ib_sgl[sg_i].addr);
		sgl[sg_i].len = cpu_to_be32(sg_len);
		sgl[sg_i].lkey = cpu_to_be32(ib_sgl[sg_i].lkey);

		len += sg_len;
	}

	return len;
}

static void ionic_v1_prep_base(struct ionic_qp *qp,
			       const struct ib_send_wr *wr,
			       struct ionic_sq_meta *meta,
			       struct ionic_v1_wqe *wqe)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(qp->ibqp.device);

	meta->wrid = wr->wr_id;
	meta->ibsts = IB_WC_SUCCESS;
	meta->signal = false;

	wqe->base.wqe_id = qp->sq.prod;

	if (wr->send_flags & IB_SEND_FENCE)
		wqe->base.flags |= cpu_to_be16(IONIC_V1_FLAG_FENCE);

	if (wr->send_flags & IB_SEND_SOLICITED)
		wqe->base.flags |= cpu_to_be16(IONIC_V1_FLAG_SOL);

	if (qp->sig_all || wr->send_flags & IB_SEND_SIGNALED) {
		wqe->base.flags |= cpu_to_be16(IONIC_V1_FLAG_SIG);
		meta->signal = true;
	}

	meta->seq = qp->sq_msn_prod;
	meta->remote =
		qp->ibqp.qp_type != IB_QPT_UD &&
		qp->ibqp.qp_type != IB_QPT_GSI &&
		!ionic_ibop_is_local(wr->opcode);

	if (meta->remote) {
		qp->sq_msn_idx[meta->seq] = qp->sq.prod;
		qp->sq_msn_prod = ionic_queue_next(&qp->sq, qp->sq_msn_prod);
	}

	dev_dbg(&dev->ibdev.dev, "post send %u prod %u\n", qp->qpid, qp->sq.prod);
	print_hex_dump_debug("wqe ", DUMP_PREFIX_OFFSET, 16, 1,
			     wqe, BIT(qp->sq.stride_log2), true);

	ionic_queue_produce(&qp->sq);
}

static int ionic_v1_prep_common(struct ionic_qp *qp,
				const struct ib_send_wr *wr,
				struct ionic_sq_meta *meta,
				struct ionic_v1_wqe *wqe)
{
	int64_t signed_len;
	uint32_t mval;

	if (wr->send_flags & IB_SEND_INLINE) {
		wqe->base.num_sge_key = 0;
		wqe->base.flags |= cpu_to_be16(IONIC_V1_FLAG_INL);
		mval = ionic_v1_send_wqe_max_data(qp->sq.stride_log2);
		signed_len = ionic_prep_inline(wqe->common.data, mval,
					       wr->sg_list, wr->num_sge);
	} else {
		wqe->base.num_sge_key = wr->num_sge;
		mval = ionic_v1_send_wqe_max_sge(qp->sq.stride_log2);
		signed_len = ionic_prep_sgl(wqe->common.sgl, mval,
					    wr->sg_list, wr->num_sge);
	}

	if (unlikely(signed_len < 0))
		return signed_len;

	meta->len = signed_len;
	wqe->common.length = cpu_to_be32(signed_len);

	ionic_v1_prep_base(qp, wr, meta, wqe);

	return 0;
}

static int ionic_v1_prep_send(struct ionic_qp *qp,
			      const struct ib_send_wr *wr)
{
	struct ionic_sq_meta *meta;
	struct ionic_v1_wqe *wqe;

	meta = &qp->sq_meta[qp->sq.prod];
	wqe = ionic_queue_at_prod(&qp->sq);

	memset(wqe, 0, 1u << qp->sq.stride_log2);

	meta->ibop = IB_WC_SEND;

	switch (wr->opcode) {
	case IB_WR_SEND:
		wqe->base.op = IONIC_V1_OP_SEND;
		break;
	case IB_WR_SEND_WITH_IMM:
		wqe->base.op = IONIC_V1_OP_SEND_IMM;
		wqe->base.imm_data_key = wr->ex.imm_data;
		break;
	case IB_WR_SEND_WITH_INV:
		wqe->base.op = IONIC_V1_OP_SEND_INV;
		wqe->base.imm_data_key =
			cpu_to_be32(wr->ex.invalidate_rkey);
		break;
	default:
		return -EINVAL;
	}

	return ionic_v1_prep_common(qp, wr, meta, wqe);
}

static int ionic_v1_prep_send_ud(struct ionic_qp *qp,
				 const struct ib_ud_wr *wr)
{
	struct ionic_sq_meta *meta;
	struct ionic_v1_wqe *wqe;
	struct ionic_ah *ah;

	if (unlikely(!wr->ah))
		return -EINVAL;

	ah = to_ionic_ah(wr->ah);

	meta = &qp->sq_meta[qp->sq.prod];
	wqe = ionic_queue_at_prod(&qp->sq);

	memset(wqe, 0, 1u << qp->sq.stride_log2);

	wqe->common.send.ah_id = cpu_to_be32(ah->ahid);
	wqe->common.send.dest_qpn = cpu_to_be32(wr->remote_qpn);
	wqe->common.send.dest_qkey = cpu_to_be32(wr->remote_qkey);

	meta->ibop = IB_WC_SEND;

	switch (wr->wr.opcode) {
	case IB_WR_SEND:
		wqe->base.op = IONIC_V1_OP_SEND;
		break;
	case IB_WR_SEND_WITH_IMM:
		wqe->base.op = IONIC_V1_OP_SEND_IMM;
		wqe->base.imm_data_key = wr->wr.ex.imm_data;
		break;
	default:
		return -EINVAL;
	}

	return ionic_v1_prep_common(qp, &wr->wr, meta, wqe);
}

static int ionic_v1_prep_rdma(struct ionic_qp *qp,
			      const struct ib_rdma_wr *wr)
{
	struct ionic_sq_meta *meta;
	struct ionic_v1_wqe *wqe;

	meta = &qp->sq_meta[qp->sq.prod];
	wqe = ionic_queue_at_prod(&qp->sq);

	memset(wqe, 0, 1u << qp->sq.stride_log2);

	meta->ibop = IB_WC_RDMA_WRITE;

	switch (wr->wr.opcode) {
	case IB_WR_RDMA_READ:
		if (wr->wr.send_flags & (IB_SEND_SOLICITED | IB_SEND_INLINE))
			return -EINVAL;
		meta->ibop = IB_WC_RDMA_READ;
		wqe->base.op = IONIC_V1_OP_RDMA_READ;
		break;
	case IB_WR_RDMA_WRITE:
		if (wr->wr.send_flags & IB_SEND_SOLICITED)
			return -EINVAL;
		wqe->base.op = IONIC_V1_OP_RDMA_WRITE;
		break;
	case IB_WR_RDMA_WRITE_WITH_IMM:
		wqe->base.op = IONIC_V1_OP_RDMA_WRITE_IMM;
		wqe->base.imm_data_key = wr->wr.ex.imm_data;
		break;
	default:
		return -EINVAL;
	}

	wqe->common.rdma.remote_va_high = cpu_to_be32(wr->remote_addr >> 32);
	wqe->common.rdma.remote_va_low = cpu_to_be32(wr->remote_addr);
	wqe->common.rdma.remote_rkey = cpu_to_be32(wr->rkey);

	return ionic_v1_prep_common(qp, &wr->wr, meta, wqe);
}

static int ionic_v1_prep_atomic(struct ionic_qp *qp,
				const struct ib_atomic_wr *wr)
{
	struct ionic_sq_meta *meta;
	struct ionic_v1_wqe *wqe;

	if (wr->wr.num_sge != 1 || wr->wr.sg_list[0].length != 8)
		return -EINVAL;

	if (wr->wr.send_flags & (IB_SEND_SOLICITED | IB_SEND_INLINE))
		return -EINVAL;

	meta = &qp->sq_meta[qp->sq.prod];
	wqe = ionic_queue_at_prod(&qp->sq);

	memset(wqe, 0, 1u << qp->sq.stride_log2);

	meta->ibop = IB_WC_RDMA_WRITE;

	switch (wr->wr.opcode) {
	case IB_WR_ATOMIC_CMP_AND_SWP:
		meta->ibop = IB_WC_COMP_SWAP;
		wqe->base.op = IONIC_V1_OP_ATOMIC_CS;
		wqe->atomic.swap_add_high = cpu_to_be32(wr->swap >> 32);
		wqe->atomic.swap_add_low = cpu_to_be32(wr->swap);
		wqe->atomic.compare_high = cpu_to_be32(wr->compare_add >> 32);
		wqe->atomic.compare_low = cpu_to_be32(wr->compare_add);
		break;
	case IB_WR_ATOMIC_FETCH_AND_ADD:
		meta->ibop = IB_WC_FETCH_ADD;
		wqe->base.op = IONIC_V1_OP_ATOMIC_FA;
		wqe->atomic.swap_add_high = cpu_to_be32(wr->compare_add >> 32);
		wqe->atomic.swap_add_low = cpu_to_be32(wr->compare_add);
		break;
	default:
		return -EINVAL;
	}

	wqe->atomic.remote_va_high = cpu_to_be32(wr->remote_addr >> 32);
	wqe->atomic.remote_va_low = cpu_to_be32(wr->remote_addr);
	wqe->atomic.remote_rkey = cpu_to_be32(wr->rkey);

	wqe->base.num_sge_key = 1;
	wqe->atomic.sge.va = cpu_to_be64(wr->wr.sg_list[0].addr);
	wqe->atomic.sge.len = cpu_to_be32(8);
	wqe->atomic.sge.lkey = cpu_to_be32(wr->wr.sg_list[0].lkey);

	return ionic_v1_prep_common(qp, &wr->wr, meta, wqe);
}

static int ionic_v1_prep_inv(struct ionic_qp *qp,
			     const struct ib_send_wr *wr)
{
	struct ionic_sq_meta *meta;
	struct ionic_v1_wqe *wqe;

	if (wr->send_flags & (IB_SEND_SOLICITED | IB_SEND_INLINE))
		return -EINVAL;

	meta = &qp->sq_meta[qp->sq.prod];
	wqe = ionic_queue_at_prod(&qp->sq);

	memset(wqe, 0, 1u << qp->sq.stride_log2);

	wqe->base.op = IONIC_V1_OP_LOCAL_INV;
	wqe->base.imm_data_key = cpu_to_be32(wr->ex.invalidate_rkey);

	ionic_v1_prep_base(qp, wr, meta, wqe);

	return 0;
}

static int ionic_v1_prep_reg(struct ionic_qp *qp,
			     const struct ib_reg_wr *wr)
{
	struct ionic_mr *mr = to_ionic_mr(wr->mr);
	struct ionic_sq_meta *meta;
	struct ionic_v1_wqe *wqe;
	int flags;

	if (wr->wr.send_flags & (IB_SEND_SOLICITED | IB_SEND_INLINE))
		return EINVAL;

	/* must call ib_map_mr_sg before posting reg wr */
	if (!mr->buf.tbl_pages)
		return EINVAL;

	meta = &qp->sq_meta[qp->sq.prod];
	wqe = ionic_queue_at_prod(&qp->sq);

	memset(wqe, 0, 1u << qp->sq.stride_log2);

	flags = to_ionic_mr_flags(wr->access);

	wqe->base.op = IONIC_V1_OP_REG_MR;
	wqe->base.num_sge_key = wr->key;
	wqe->base.imm_data_key = cpu_to_be32(mr->ibmr.lkey);
	wqe->reg_mr.va = cpu_to_be64(mr->ibmr.iova);
	wqe->reg_mr.length = cpu_to_be64(mr->ibmr.length);
	wqe->reg_mr.offset =
		cpu_to_be64(mr->ibmr.iova & (mr->ibmr.page_size - 1));

	if (mr->buf.tbl_pages == 1 && mr->buf.tbl_buf)
		wqe->reg_mr.dma_addr = mr->buf.tbl_buf[0];
	else
		wqe->reg_mr.dma_addr = cpu_to_le64(mr->buf.tbl_dma);

	wqe->reg_mr.map_count = cpu_to_be32(mr->buf.tbl_pages);
	wqe->reg_mr.flags = cpu_to_be16(flags);
	wqe->reg_mr.dir_size_log2 = 0;
	wqe->reg_mr.page_size_log2 = order_base_2(mr->ibmr.page_size);

	ionic_v1_prep_base(qp, &wr->wr, meta, wqe);

	return 0;
}

static int ionic_prep_one_rc(struct ionic_qp *qp,
			     const struct ib_send_wr *wr)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(qp->ibqp.device);
	int rc = 0;

	switch (wr->opcode) {
	case IB_WR_SEND:
	case IB_WR_SEND_WITH_IMM:
	case IB_WR_SEND_WITH_INV:
		rc = ionic_v1_prep_send(qp, wr);
		break;
	case IB_WR_RDMA_READ:
	case IB_WR_RDMA_WRITE:
	case IB_WR_RDMA_WRITE_WITH_IMM:
		rc = ionic_v1_prep_rdma(qp, rdma_wr(wr));
		break;
	case IB_WR_ATOMIC_CMP_AND_SWP:
	case IB_WR_ATOMIC_FETCH_AND_ADD:
		rc = ionic_v1_prep_atomic(qp, atomic_wr(wr));
		break;
	case IB_WR_LOCAL_INV:
		rc = ionic_v1_prep_inv(qp, wr);
		break;
	case IB_WR_REG_MR:
		rc = ionic_v1_prep_reg(qp, reg_wr(wr));
		break;
	default:
		dev_dbg(&dev->ibdev.dev, "invalid opcode %d\n", wr->opcode);
		rc = -EINVAL;
	}

	return rc;
}

static int ionic_prep_one_ud(struct ionic_qp *qp,
			     const struct ib_send_wr *wr)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(qp->ibqp.device);
	int rc = 0;

	switch (wr->opcode) {
	case IB_WR_SEND:
	case IB_WR_SEND_WITH_IMM:
		rc = ionic_v1_prep_send_ud(qp, ud_wr(wr));
		break;
	default:
		dev_dbg(&dev->ibdev.dev, "invalid opcode %d\n", wr->opcode);
		rc = -EINVAL;
	}

	return rc;
}

static void ionic_post_send_cmb(struct ionic_ibdev *dev, struct ionic_qp *qp)
{
	void __iomem *cmb_ptr;
	void *wqe_ptr;
	u32 stride;
	u16 pos, end;
	u8 stride_log2;

	stride_log2 = qp->sq.stride_log2;
	stride = BIT(stride_log2);

	pos = qp->sq_cmb_prod;
	end = qp->sq.prod;

	while (pos != end) {
		cmb_ptr = qp->sq_cmb_ptr + ((size_t)pos << stride_log2);
		wqe_ptr = ionic_queue_at(&qp->sq, pos);

		memcpy_toio(cmb_ptr, wqe_ptr, stride);

		pos = ionic_queue_next(&qp->sq, pos);

		ionic_dbell_ring(&dev->dbpage[dev->sq_qtype],
				 qp->sq.dbell | pos);
	}

	qp->sq_cmb_prod = end;
}

static void ionic_post_recv_cmb(struct ionic_ibdev *dev, struct ionic_qp *qp)
{
	void __iomem *cmb_ptr;
	void *wqe_ptr;
	u32 stride;
	u16 pos, end;
	u8 stride_log2;

	stride_log2 = qp->rq.stride_log2;

	pos = qp->rq_cmb_prod;
	end = qp->rq.prod;


	if (pos > end) {
		cmb_ptr = qp->rq_cmb_ptr + ((size_t)pos << stride_log2);
		wqe_ptr = ionic_queue_at(&qp->rq, pos);

		stride = (u32)(qp->rq.mask - pos + 1) << stride_log2;
		memcpy_toio(cmb_ptr, wqe_ptr, stride);

		pos = 0;

		ionic_dbell_ring(&dev->dbpage[dev->rq_qtype],
				 qp->rq.dbell | pos);
	}

	if (pos < end) {
		cmb_ptr = qp->rq_cmb_ptr + ((size_t)pos << stride_log2);
		wqe_ptr = ionic_queue_at(&qp->rq, pos);

		stride = (u32)(end - pos) << stride_log2;
		memcpy_toio(cmb_ptr, wqe_ptr, stride);

		pos = end;

		ionic_dbell_ring(&dev->dbpage[dev->rq_qtype],
				 qp->rq.dbell | pos);
	}

	qp->rq_cmb_prod = end;
}

static int ionic_v1_prep_recv(struct ionic_qp *qp,
			      const struct ib_recv_wr *wr)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(qp->ibqp.device);
	struct ionic_rq_meta *meta;
	struct ionic_v1_wqe *wqe;
	s64 signed_len;
	u32 mval;

	wqe = ionic_queue_at_prod(&qp->rq);

	/* if wqe is owned by device, caller can try posting again soon */
	if (wqe->base.flags & cpu_to_be16(IONIC_V1_FLAG_FENCE))
		return -EAGAIN;

	meta = qp->rq_meta_head;
	if (unlikely(meta == IONIC_META_LAST) ||
	    unlikely(meta == IONIC_META_POSTED))
		return -EIO;

	memset(wqe, 0, 1u << qp->rq.stride_log2);

	mval = ionic_v1_recv_wqe_max_sge(qp->rq.stride_log2);
	signed_len = ionic_prep_sgl(wqe->recv.sgl, mval,
				    wr->sg_list, wr->num_sge);
	if (signed_len < 0)
		return signed_len;

	meta->wrid = wr->wr_id;

	wqe->base.wqe_id = meta - qp->rq_meta;
	wqe->base.num_sge_key = wr->num_sge;

	/* total length for recv goes in base imm_data_key */
	wqe->base.imm_data_key = cpu_to_be32(signed_len);

	/* if this is a srq, set fence bit to indicate device ownership */
	if (qp->is_srq)
		wqe->base.flags |= cpu_to_be16(IONIC_V1_FLAG_FENCE);

	dev_dbg(&dev->ibdev.dev, "post recv %u prod %u\n", qp->qpid, qp->rq.prod);
	print_hex_dump_debug("wqe ", DUMP_PREFIX_OFFSET, 16, 1,
			     wqe, BIT(qp->rq.stride_log2), true);

	ionic_queue_produce(&qp->rq);

	qp->rq_meta_head = meta->next;
	meta->next = IONIC_META_POSTED;

	return 0;
}

static int ionic_post_send_common(struct ionic_ibdev *dev,
				  struct ionic_ctx *ctx,
				  struct ionic_cq *cq,
				  struct ionic_qp *qp,
				  const struct ib_send_wr *wr,
				  const struct ib_send_wr **bad)
{
	unsigned long irqflags;
	int spend, rc = 0;

	if (!bad)
		return -EINVAL;

	if (ctx || !qp->has_sq) {
		*bad = wr;
		return -EINVAL;
	}

	if (qp->state < IB_QPS_RTS) {
		*bad = wr;
		return -EINVAL;
	}

	spin_lock_irqsave(&qp->sq_lock, irqflags);

	if (qp->ibqp.qp_type == IB_QPT_UD || qp->ibqp.qp_type == IB_QPT_GSI) {
		while (wr) {
			if (ionic_queue_full(&qp->sq)) {
				dev_dbg(&dev->ibdev.dev, "queue full");
				rc = -ENOMEM;
				goto out;
			}

			rc = ionic_prep_one_ud(qp, wr);
			if (rc)
				goto out;

			wr = wr->next;
		}
	} else {
		while (wr) {
			if (ionic_queue_full(&qp->sq)) {
				dev_dbg(&dev->ibdev.dev, "queue full");
				rc = -ENOMEM;
				goto out;
			}

			rc = ionic_prep_one_rc(qp, wr);
			if (rc)
				goto out;

			wr = wr->next;
		}
	}

out:
	/* irq remains saved here, not restored/saved again */
	if (!spin_trylock(&cq->lock)) {
		spin_unlock(&qp->sq_lock);
		spin_lock(&cq->lock);
		spin_lock(&qp->sq_lock);
	}

	if (likely(qp->sq.prod != qp->sq_old_prod)) {
		/* ring cq doorbell just in time */
		spend = (qp->sq.prod - qp->sq_old_prod) * qp->sq.mask;
		ionic_reserve_cq(dev, cq, spend);

		qp->sq_old_prod = qp->sq.prod;

		if (qp->sq_cmb_ptr)
			ionic_post_send_cmb(dev, qp);
		else
			ionic_dbell_ring(&dev->dbpage[dev->sq_qtype],
					 ionic_queue_dbell_val(&qp->sq));
	}

	if (qp->sq_flush) {
		cq->flush = true;
		list_move_tail(&qp->cq_flush_sq, &cq->flush_sq);
	}

	spin_unlock(&qp->sq_lock);
	spin_unlock_irqrestore(&cq->lock, irqflags);

	*bad = wr;
	return rc;
}

static int ionic_post_recv_common(struct ionic_ibdev *dev,
				  struct ionic_ctx *ctx,
				  struct ionic_cq *cq,
				  struct ionic_qp *qp,
				  const struct ib_recv_wr *wr,
				  const struct ib_recv_wr **bad)
{
	unsigned long irqflags;
	int spend, rc = 0;

	if (!bad)
		return -EINVAL;

	if (ctx || !qp->has_rq) {
		*bad = wr;
		return -EINVAL;
	}

	if (qp->state < IB_QPS_INIT) {
		*bad = wr;
		return -EINVAL;
	}

	spin_lock_irqsave(&qp->rq_lock, irqflags);

	while (wr) {
		if (ionic_queue_full(&qp->rq)) {
			dev_dbg(&dev->ibdev.dev, "queue full");
			rc = -ENOMEM;
			goto out;
		}

		rc = ionic_v1_prep_recv(qp, wr);
		if (rc)
			goto out;

		wr = wr->next;
	}

out:
	/* irq remains saved here, not restored/saved again */
	if (!spin_trylock(&cq->lock)) {
		spin_unlock(&qp->rq_lock);
		spin_lock(&cq->lock);
		spin_lock(&qp->rq_lock);
	}

	if (likely(qp->rq.prod != qp->rq_old_prod)) {
		/* ring cq doorbell just in time */
		spend = (qp->rq.prod - qp->rq_old_prod) * qp->rq.mask;
		ionic_reserve_cq(dev, cq, spend);

		qp->rq_old_prod = qp->rq.prod;

		if (qp->rq_cmb_ptr)
			ionic_post_recv_cmb(dev, qp);
		else
			ionic_dbell_ring(&dev->dbpage[dev->rq_qtype],
					 ionic_queue_dbell_val(&qp->rq));
	}

	if (qp->rq_flush) {
		cq->flush = true;
		list_move_tail(&qp->cq_flush_rq, &cq->flush_rq);
	}

	spin_unlock(&qp->rq_lock);
	spin_unlock_irqrestore(&cq->lock, irqflags);

	*bad = wr;
	return rc;
}


#ifdef HAVE_CONST_IB_WR
static int ionic_post_send(struct ib_qp *ibqp,
			   const struct ib_send_wr *wr,
			   const struct ib_send_wr **bad)
#else
static int ionic_post_send(struct ib_qp *ibqp,
			   struct ib_send_wr *wr,
			   struct ib_send_wr **bad)
#endif
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibqp->device);
	struct ionic_ctx *ctx = to_ionic_ctx_uobj(ibqp->uobject);
	struct ionic_cq *cq = to_ionic_cq(ibqp->send_cq);
	struct ionic_qp *qp = to_ionic_qp(ibqp);

#ifdef HAVE_CONST_IB_WR
	return ionic_post_send_common(dev, ctx, cq, qp, wr, bad);
#else
	return ionic_post_send_common(dev, ctx, cq, qp, wr,
				      (const struct ib_send_wr **)bad);
#endif
}

#ifdef HAVE_CONST_IB_WR
static int ionic_post_recv(struct ib_qp *ibqp,
			   const struct ib_recv_wr *wr,
			   const struct ib_recv_wr **bad)
#else
static int ionic_post_recv(struct ib_qp *ibqp,
			   struct ib_recv_wr *wr,
			   struct ib_recv_wr **bad)
#endif
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibqp->device);
	struct ionic_ctx *ctx = to_ionic_ctx_uobj(ibqp->uobject);
	struct ionic_cq *cq = to_ionic_cq(ibqp->recv_cq);
	struct ionic_qp *qp = to_ionic_qp(ibqp);

#ifdef HAVE_CONST_IB_WR
	return ionic_post_recv_common(dev, ctx, cq, qp, wr, bad);
#else
	return ionic_post_recv_common(dev, ctx, cq, qp, wr,
				      (const struct ib_recv_wr **)bad);
#endif
}

static struct ib_srq *ionic_create_srq(struct ib_pd *ibpd,
				       struct ib_srq_init_attr *attr,
				       struct ib_udata *udata)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibpd->device);
	struct ionic_ctx *ctx = to_ionic_ctx_uobj(ibpd->uobject);
	struct ionic_qp *qp;
	struct ionic_cq *cq;
	struct ionic_srq_req req;
	struct ionic_srq_resp resp = {0};
	struct ionic_tbl_buf rq_buf;
	unsigned long irqflags;
	int rc;

	if (!ctx) {
		rc = ionic_validate_udata(udata, 0, 0);
	} else {
		rc = ionic_validate_udata(udata, sizeof(req), sizeof(resp));
		if (!rc)
			rc = ib_copy_from_udata(&req, udata, sizeof(req));
	}

	if (rc)
		goto err_srq;

	qp = kzalloc(sizeof(*qp), GFP_KERNEL);
	if (!qp) {
		rc = -ENOSYS;
		goto err_srq;
	}

	qp->state = IB_QPS_INIT;

	rc = ionic_get_srqid(dev, &qp->qpid);
	if (rc)
		goto err_srqid;

	qp->has_ah = false;
	qp->has_sq = false;
	qp->has_rq = true;
	qp->is_srq = true;

	spin_lock_init(&qp->rq_lock);

	rc = ionic_qp_rq_init(dev, ctx, qp, &req.rq, &rq_buf,
			      attr->attr.max_wr, attr->attr.max_sge);
	if (rc)
		goto err_rq;

	/* TODO need admin command */
	rc = -ENOSYS;
	goto err_cmd;

	if (ctx) {
		resp.qpid = qp->qpid;

		rc = ib_copy_to_udata(udata, &resp, sizeof(resp));
		if (rc)
			goto err_resp;
	}

	ionic_pgtbl_unbuf(dev, &rq_buf);

	if (ib_srq_has_cq(qp->ibsrq.srq_type)) {
		qp->ibsrq.ext.xrc.srq_num = qp->qpid;

		mutex_lock(&dev->tbl_lock);
		tbl_alloc_node(&dev->qp_tbl);
		tbl_insert(&dev->qp_tbl, qp, qp->qpid);
		mutex_unlock(&dev->tbl_lock);

#ifdef HAVE_SRQ_EXT_CQ
		cq = to_ionic_cq(attr->ext.cq);
#else
		cq = to_ionic_cq(attr->ext.xrc.cq);
#endif
		spin_lock_irqsave(&cq->lock, irqflags);
		spin_unlock_irqrestore(&cq->lock, irqflags);
	}

	return &qp->ibsrq;

err_resp:
	ionic_destroy_qp_cmd(dev, qp->qpid);
err_cmd:
	ionic_pgtbl_unbuf(dev, &rq_buf);
	ionic_qp_rq_destroy(dev, ctx, qp);
err_rq:
	ionic_put_srqid(dev, qp->qpid);
err_srqid:
	kfree(qp);
err_srq:
	return ERR_PTR(rc);
}

static int ionic_modify_srq(struct ib_srq *ibsrq, struct ib_srq_attr *attr,
			    enum ib_srq_attr_mask mask, struct ib_udata *udata)
{
	return -ENOSYS;
}

static int ionic_query_srq(struct ib_srq *ibsrq,
			   struct ib_srq_attr *attr)
{
	return -ENOSYS;
}

static int ionic_destroy_srq(struct ib_srq *ibsrq)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibsrq->device);
	struct ionic_ctx *ctx = to_ionic_ctx_uobj(ibsrq->uobject);
	struct ionic_qp *qp = to_ionic_srq(ibsrq);
	struct ionic_cq *cq;
	unsigned long irqflags;
	int rc;

	rc = ionic_destroy_qp_cmd(dev, qp->qpid);
	if (rc)
		return rc;

	if (ib_srq_has_cq(qp->ibsrq.srq_type)) {
		mutex_lock(&dev->tbl_lock);
		tbl_free_node(&dev->qp_tbl);
		tbl_delete(&dev->qp_tbl, qp->qpid);
		mutex_unlock(&dev->tbl_lock);

#ifdef HAVE_SRQ_EXT_CQ
		cq = to_ionic_cq(qp->ibsrq.ext.cq);
#else
		cq = to_ionic_cq(qp->ibsrq.ext.xrc.cq);
#endif
		spin_lock_irqsave(&cq->lock, irqflags);
		ionic_clean_cq(cq, qp->qpid);
		list_del(&qp->cq_flush_rq);
		spin_unlock_irqrestore(&cq->lock, irqflags);
	}

	ionic_qp_rq_destroy(dev, ctx, qp);
	ionic_put_srqid(dev, qp->qpid);

	kfree(qp);

	return 0;
}

#ifdef HAVE_CONST_IB_WR
static int ionic_post_srq_recv(struct ib_srq *ibsrq,
			       const struct ib_recv_wr *wr,
			       const struct ib_recv_wr **bad)
#else
static int ionic_post_srq_recv(struct ib_srq *ibsrq,
			       struct ib_recv_wr *wr,
			       struct ib_recv_wr **bad)
#endif
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibsrq->device);
	struct ionic_ctx *ctx = to_ionic_ctx_uobj(ibsrq->uobject);
	struct ionic_cq *cq = NULL;
	struct ionic_qp *qp = to_ionic_srq(ibsrq);

#ifdef HAVE_SRQ_EXT_CQ
	if (ibsrq->ext.cq)
		cq = to_ionic_cq(ibsrq->ext.cq);
#else
	if (ibsrq->ext.xrc.cq)
		cq = to_ionic_cq(ibsrq->ext.xrc.cq);
#endif

#ifdef HAVE_CONST_IB_WR
	return ionic_post_recv_common(dev, ctx, cq, qp, wr, bad);
#else
	return ionic_post_recv_common(dev, ctx, cq, qp, wr,
				      (const struct ib_recv_wr **)bad);
#endif
}

static int ionic_get_port_immutable(struct ib_device *ibdev, u8 port,
				    struct ib_port_immutable *attr)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);

	if (port != 1)
		return -EINVAL;

	attr->core_cap_flags = RDMA_CORE_PORT_IBA_ROCE_UDP_ENCAP;

	attr->pkey_tbl_len = dev->port_attr.pkey_tbl_len;
	attr->gid_tbl_len = dev->port_attr.gid_tbl_len;
	attr->max_mad_size = IB_MGMT_MAD_SIZE;

	return 0;
}

#ifdef HAVE_GET_DEV_FW_STR
#ifdef HAVE_GET_DEV_FW_STR_LEN
static void ionic_get_dev_fw_str(struct ib_device *ibdev, char *str,
				 size_t str_len)
#else
static void ionic_get_dev_fw_str(struct ib_device *ibdev, char *str)
#endif
{
	str[0] = 0;
}
#endif

#ifdef HAVE_GET_VECTOR_AFFINITY
static const struct cpumask *ionic_get_vector_affinity(struct ib_device *ibdev,
						       int comp_vector)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibdev);

	if (comp_vector < 0 || comp_vector >= dev->eq_count)
		return NULL;

	return irq_get_affinity_mask(dev->eq_vec[comp_vector]->irq);
}
#endif

static void ionic_port_event(struct ionic_ibdev *dev, enum ib_event_type event)
{
	struct ib_event ev;

	ev.device = &dev->ibdev;
	ev.element.port_num = 1;
	ev.event = event;

	ib_dispatch_event(&ev);
}

static void ionic_cq_event(struct ionic_ibdev *dev, u32 cqid, u8 code)
{
	struct ib_event ibev;
	struct ionic_cq *cq;

	rcu_read_lock();

	cq = tbl_lookup(&dev->cq_tbl, cqid);
	if (!cq) {
		dev_dbg(&dev->ibdev.dev, "missing cqid %#x code %u\n",
			cqid, code);
		goto out;
	}

	ibev.device = &dev->ibdev;
	ibev.element.cq = &cq->ibcq;

	switch(code) {
	case IONIC_V1_EQE_CQ_NOTIFY:
		cq->ibcq.comp_handler(&cq->ibcq, cq->ibcq.cq_context);
		goto out;

	case IONIC_V1_EQE_CQ_ERR:
		ibev.event = IB_EVENT_CQ_ERR;
		break;

	default:
		dev_dbg(&dev->ibdev.dev, "unrecognized cqid %#x code %u\n",
			cqid, code);
		goto out;
	}

	cq->ibcq.event_handler(&ibev, cq->ibcq.cq_context);

out:
	rcu_read_unlock();
}

static void ionic_qp_event(struct ionic_ibdev *dev, u32 qpid, u8 code)
{
	struct ib_event ibev;
	struct ionic_qp *qp;

	rcu_read_lock();

	qp = tbl_lookup(&dev->qp_tbl, qpid);
	if (!qp) {
		dev_dbg(&dev->ibdev.dev, "missing qpid %#x code %u\n",
			qpid, code);
		goto out;
	}

	ibev.device = &dev->ibdev;

	if (qp->is_srq) {
		ibev.element.srq = &qp->ibsrq;

		switch(code) {
		case IONIC_V1_EQE_SRQ_LEVEL:
			ibev.event = IB_EVENT_SRQ_LIMIT_REACHED;
			break;

		case IONIC_V1_EQE_QP_ERR:
			ibev.event = IB_EVENT_SRQ_ERR;
			break;

		default:
			dev_dbg(&dev->ibdev.dev,
				"unrecognized srqid %#x code %u\n",
				qpid, code);
			goto out;
		}

		qp->ibsrq.event_handler(&ibev, qp->ibsrq.srq_context);
	} else {
		ibev.element.qp = &qp->ibqp;

		switch(code) {
		case IONIC_V1_EQE_SQ_DRAIN:
			ibev.event = IB_EVENT_SQ_DRAINED;
			break;

		case IONIC_V1_EQE_QP_COMM_EST:
			ibev.event = IB_EVENT_COMM_EST;
			break;

		case IONIC_V1_EQE_QP_LAST_WQE:
			ibev.event = IB_EVENT_QP_LAST_WQE_REACHED;
			break;

		case IONIC_V1_EQE_QP_ERR:
			ibev.event = IB_EVENT_QP_FATAL;
			break;

		case IONIC_V1_EQE_QP_ERR_REQUEST:
			ibev.event = IB_EVENT_QP_REQ_ERR;
			break;

		case IONIC_V1_EQE_QP_ERR_ACCESS:
			ibev.event = IB_EVENT_QP_ACCESS_ERR;
			break;

		default:
			dev_dbg(&dev->ibdev.dev,
				"unrecognized qpid %#x code %u\n",
				qpid, code);
			goto out;
		}

		qp->ibqp.event_handler(&ibev, qp->ibqp.qp_context);
	}


out:
	rcu_read_unlock();
}

static bool ionic_next_eqe(struct ionic_eq *eq, struct ionic_v1_eqe *eqe)
{
	struct ionic_v1_eqe *qeqe;
	bool color;

	qeqe = ionic_queue_at_prod(&eq->q);
	color = ionic_v1_eqe_color(qeqe);

	/* cons is color for eq */
	if (eq->q.cons != color)
		return false;

	rmb();

	*eqe = *qeqe;

	dev_dbg(&eq->dev->ibdev.dev, "poll eq prod %u\n", eq->q.prod);
	print_hex_dump_debug("eqe ", DUMP_PREFIX_OFFSET, 16, 1,
			     eqe, BIT(eq->q.stride_log2), true);

	return true;
}

static u16 ionic_poll_eq(struct ionic_eq *eq, u16 budget)
{
	struct ionic_ibdev *dev = eq->dev;
	struct ionic_v1_eqe eqe;
	u32 evt, qid;
	u8 type, code;
	u16 old_prod;
	u16 npolled = 0;

	old_prod = eq->q.prod;

	while (npolled < budget) {
		if (!ionic_next_eqe(eq, &eqe))
			break;

		ionic_queue_produce(&eq->q);

		/* cons is color for eq */
		eq->q.cons = ionic_color_wrap(eq->q.prod, eq->q.cons);

		++npolled;

		evt = ionic_v1_eqe_evt(&eqe);
		type = ionic_v1_eqe_evt_type(evt);
		code = ionic_v1_eqe_evt_code(evt);
		qid = ionic_v1_eqe_evt_qid(evt);

		switch (type) {
		case IONIC_V1_EQE_TYPE_CQ:
			ionic_cq_event(dev, qid, code);
			break;

		case IONIC_V1_EQE_TYPE_QP:
			ionic_qp_event(dev, qid, code);
			break;

		default:
			dev_dbg(&dev->ibdev.dev,
				"unrecognized event %#x type %u\n",
				evt, type);
		}
	}

	if (old_prod != eq->q.prod)
		ionic_dbell_ring(&dev->dbpage[dev->eq_qtype],
				 ionic_queue_dbell_val(&eq->q));

	return npolled;
}

static void ionic_poll_eq_work(struct work_struct *work)
{
	struct ionic_eq *eq = container_of(work, struct ionic_eq, work);
	int npolled;

	if (unlikely(!eq->enable) || WARN_ON(eq->armed))
		return;

	npolled = ionic_poll_eq(eq, ionic_eq_work_budget);

	if (npolled) {
		ionic_intr_credits(eq->dev, eq->intr, npolled);
		queue_work(ionic_evt_workq, &eq->work);
	} else {
		xchg(&eq->armed, true);
		ionic_intr_credits(eq->dev, eq->intr, IONIC_INTR_CRED_UNMASK);
	}
}

static irqreturn_t ionic_poll_eq_isr(int irq, void *eqptr)
{
	struct ionic_eq *eq = eqptr;
	u16 npolled;
	bool was_armed;

	was_armed = xchg(&eq->armed, false);

	if (unlikely(!eq->enable) || !was_armed)
		return IRQ_HANDLED;

	npolled = ionic_poll_eq(eq, ionic_eq_isr_budget);

	ionic_intr_credits(eq->dev, eq->intr, npolled);
	queue_work(ionic_evt_workq, &eq->work);

	return IRQ_HANDLED;
}

static int ionic_rdma_devcmd(struct ionic_ibdev *dev,
			     struct ionic_admin_ctx *admin)
{
	int rc;

	rc = ionic_api_adminq_post(dev->lif, admin);
	if (rc)
		goto err_cmd;

	wait_for_completion(&admin->work);

	if (0 /* XXX did the queue fail? */) {
		rc = -EIO;
		goto err_cmd;
	}

	rc = ionic_verbs_status_to_rc(admin->comp.comp.status);

err_cmd:
	return rc;
}

static int ionic_rdma_reset_devcmd(struct ionic_ibdev *dev)
{
	struct ionic_admin_ctx admin = {
		.work = COMPLETION_INITIALIZER_ONSTACK(admin.work),
		.cmd.rdma_reset = {
			.opcode = (__force u16)cpu_to_le16(CMD_OPCODE_RDMA_RESET_LIF),
			.lif_index = (__force u16)cpu_to_le16(dev->lif_id),
		},
	};

	return ionic_rdma_devcmd(dev, &admin);
}

static int ionic_rdma_queue_devcmd(struct ionic_ibdev *dev,
				   struct ionic_queue *q,
				   u32 qid, u32 cid, u16 opcode,
				   int xxx_table_index)
{
	struct ionic_admin_ctx admin = {
		.work = COMPLETION_INITIALIZER_ONSTACK(admin.work),
		.cmd.rdma_queue = {
			.opcode = cpu_to_le16(opcode),
			.lif_index = cpu_to_le16(dev->lif_id),
			.qid_ver = cpu_to_le32(qid),
			.cid = cpu_to_le32(cid),
			.dbid = cpu_to_le16(dev->dbid),
			.depth_log2 = q->depth_log2,
			.stride_log2 = q->stride_log2,
			.dma_addr = cpu_to_le64(q->dma),
			.xxx_table_index = cpu_to_le32(xxx_table_index),
		},
	};

	return ionic_rdma_devcmd(dev, &admin);
}

static struct ionic_eq *ionic_create_eq(struct ionic_ibdev *dev, int eqid)
{
	struct ionic_eq *eq;
	int rc;

	eq = kmalloc(sizeof(*eq), GFP_KERNEL);
	if (!eq) {
		rc = -ENOMEM;
		goto err_eq;
	}

	eq->dev = dev;

	rc = ionic_queue_init(&eq->q, dev->hwdev, ionic_eq_depth,
			      sizeof(struct ionic_v1_eqe));
	if (rc)
		goto err_q;

	eq->eqid = eqid;

	eq->armed = true;
	eq->enable = false;
	INIT_WORK(&eq->work, ionic_poll_eq_work);

	rc = ionic_api_get_intr(dev->lif, &eq->irq);
	if (rc < 0)
		goto err_intr;

	eq->intr = rc;

	ionic_queue_dbell_init(&eq->q, eq->eqid);

	/* cons is color for eq */
	eq->q.cons = true;

	snprintf(eq->name, sizeof(eq->name), "%s-%d-%d-eq",
		 DRIVER_NAME, dev->lif_id, eq->eqid);

	ionic_intr_mask(dev, eq->intr, IONIC_INTR_MASK_SET);
	ionic_intr_mask_assert(dev, eq->intr, IONIC_INTR_MASK_SET);
	ionic_intr_coalesce_init(dev, eq->intr, 0);
	ionic_intr_credits(dev, eq->intr, 0);

	eq->enable = true;

	rc = request_irq(eq->irq, ionic_poll_eq_isr, 0, eq->name, eq);
	if (rc)
		goto err_irq;

	rc = ionic_rdma_queue_devcmd(dev, &eq->q, eq->eqid, eq->intr,
				     CMD_OPCODE_RDMA_CREATE_EQ, 0);
	if (rc)
		goto err_cmd;

	ionic_intr_mask(dev, eq->intr, IONIC_INTR_MASK_CLEAR);

	ionic_dbgfs_add_eq(dev, eq);

	return eq;

err_cmd:
	eq->enable = false;
	flush_work(&eq->work);
	free_irq(eq->irq, eq);
err_irq:
	ionic_api_put_intr(dev->lif, eq->intr);
err_intr:
	ionic_queue_destroy(&eq->q, dev->hwdev);
err_q:
	kfree(eq);
err_eq:
	return ERR_PTR(rc);
}

static void ionic_destroy_eq(struct ionic_eq *eq)
{
	struct ionic_ibdev *dev = eq->dev;

	ionic_dbgfs_rm_eq(eq);

	eq->enable = false;
	flush_work(&eq->work);
	free_irq(eq->irq, eq);

	ionic_api_put_intr(dev->lif, eq->intr);
	ionic_queue_destroy(&eq->q, dev->hwdev);
	kfree(eq);
}

static void ionic_rdma_admincq_comp(struct ib_cq *ibcq, void *cq_context)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(ibcq->device);
	unsigned long irqflags;

	spin_lock_irqsave(&dev->admin_lock, irqflags);
	dev->admin_armed = false;
	if (dev->admin_state < IONIC_ADMIN_KILLED)
		queue_work(ionic_evt_workq, &dev->admin_work);
	spin_unlock_irqrestore(&dev->admin_lock, irqflags);
}

static void ionic_rdma_admincq_event(struct ib_event *event, void *cq_context)
{
	struct ionic_ibdev *dev = to_ionic_ibdev(event->device);

	dev_err(&dev->ibdev.dev, "admincq event %d\n", event->event);
}

static struct ionic_cq *ionic_create_rdma_admincq(struct ionic_ibdev *dev,
						  int comp_vector)
{
	struct ionic_cq *cq;
	struct ionic_tbl_buf buf;
	struct ib_cq_init_attr attr = {
		.cqe = ionic_aq_depth,
		.comp_vector = comp_vector,
	};
	int rc;

	cq = __ionic_create_cq(dev, &buf, &attr, NULL, NULL);
	if (IS_ERR(cq)) {
		rc = PTR_ERR(cq);
		goto err_cq;
	}

	rc = ionic_rdma_queue_devcmd(dev, &cq->q, cq->cqid, cq->eqid,
				     CMD_OPCODE_RDMA_CREATE_CQ,
				     cq->res.tbl_pos);
	if (rc)
		goto err_cmd;

	cq->ibcq.device = &dev->ibdev;
	cq->ibcq.uobject = NULL;
	cq->ibcq.comp_handler = ionic_rdma_admincq_comp;
	cq->ibcq.event_handler = ionic_rdma_admincq_event;
	cq->ibcq.cq_context = NULL;
	atomic_set(&cq->ibcq.usecnt, 0);

	return cq;

err_cmd:
	__ionic_destroy_cq(dev, cq);
err_cq:
	return ERR_PTR(rc);
}

static struct ionic_aq *__ionic_create_rdma_adminq(struct ionic_ibdev *dev,
						   u32 cqid)
{
	struct ionic_aq *aq;
	int rc;

	aq = kmalloc(sizeof(*aq), GFP_KERNEL);
	if (!aq) {
		rc = -ENOMEM;
		goto err_aq;
	}

	aq->dev = dev;

	aq->aqid = dev->aq_base;

	aq->cqid = cqid;

	spin_lock_init(&aq->lock);

	rc = ionic_queue_init(&aq->q, dev->hwdev, ionic_aq_depth,
			      sizeof(struct ionic_v1_admin_wqe));
	if (rc)
		goto err_q;

	ionic_queue_dbell_init(&aq->q, aq->aqid);

	aq->q_wr = kcalloc((u32)aq->q.mask + 1, sizeof(*aq->q_wr), GFP_KERNEL);
	if (!aq->q_wr) {
		rc = -ENOMEM;
		goto err_wr;
	}

	INIT_LIST_HEAD(&aq->wr_prod);
	INIT_LIST_HEAD(&aq->wr_post);

	ionic_dbgfs_add_aq(dev, aq);

	return aq;

err_wr:
	ionic_queue_destroy(&aq->q, dev->hwdev);
err_q:
	kfree(aq);
err_aq:
	return ERR_PTR(rc);
}

static void __ionic_destroy_rdma_adminq(struct ionic_ibdev *dev,
					struct ionic_aq *aq)
{

	ionic_dbgfs_rm_aq(aq);

	ionic_queue_destroy(&aq->q, dev->hwdev);
	kfree(aq);
}

static struct ionic_aq *ionic_create_rdma_adminq(struct ionic_ibdev *dev,
						 u32 cqid)
{
	struct ionic_aq *aq;
	int rc;

	aq = __ionic_create_rdma_adminq(dev, cqid);
	if (IS_ERR(aq)) {
		rc = PTR_ERR(aq);
		goto err_aq;
	}

	rc = ionic_rdma_queue_devcmd(dev, &aq->q, aq->aqid, aq->cqid,
				     CMD_OPCODE_RDMA_CREATE_ADMINQ, 0);
	if (rc)
		goto err_cmd;

	return aq;

err_cmd:
	__ionic_destroy_rdma_adminq(dev, aq);
err_aq:
	return ERR_PTR(rc);
}

static void ionic_kill_ibdev(void *dev_ptr)
{
	struct ionic_ibdev *dev = dev_ptr;
	unsigned long irqflags;

	dev_warn(&dev->ibdev.dev, "reset has been indicated\n");

	spin_lock_irqsave(&dev->admin_lock, irqflags);
	dev->admin_state = IONIC_ADMIN_KILLED;
	ionic_admin_poll_locked(dev);
	spin_unlock_irqrestore(&dev->admin_lock, irqflags);
}

static void ionic_kill_rdma_admin(struct ionic_ibdev *dev)
{
	unsigned long irqflags;
	int rc;

	if (!dev->adminq)
		return;

	/* pause rdma admin queue to reset device (only once: if active) */
	spin_lock_irqsave(&dev->admin_lock, irqflags);
	if (dev->admin_state != IONIC_ADMIN_ACTIVE) {
		spin_unlock_irqrestore(&dev->admin_lock, irqflags);
		return;
	}
	dev->admin_state = IONIC_ADMIN_PAUSED;
	spin_unlock_irqrestore(&dev->admin_lock, irqflags);

	/* After resetting the device, it will be safe to resume the rdma admin
	 * queue in the killed state.  Commands will not be issued to the
	 * device, but will complete locally with status IONIC_ADMIN_KILLED.
	 * Handling completion will ensure that creating or modifying resources
	 * fails, but destroying resources succeds.
	 *
	 * If there was a failure resetting the device using this strategy,
	 * then the state of the device is unknown.  The rdma admin queue is
	 * left here in the paused state.  No new commands are issued to the
	 * device, nor are completed locally.  The eth driver will use a
	 * different strategy to reset the device.  A callback from the eth
	 * driver will indicate that the reset is done and it is safe to
	 * continue.  Then, the rdma admin queue will be transitioned to the
	 * killed state and new and outstanding commands will complete locally.
	 */

	rc = ionic_rdma_reset_devcmd(dev);
	if (unlikely(rc)) {
		dev_err(&dev->ibdev.dev, "failed to reset rdma %d\n", rc);
		ionic_api_request_reset(dev->lif);
	} else {
		spin_lock_irqsave(&dev->admin_lock, irqflags);
		dev->admin_state = IONIC_ADMIN_KILLED;
		ionic_admin_poll_locked(dev);
		spin_unlock_irqrestore(&dev->admin_lock, irqflags);
	}
}

static void ionic_reset_work(struct work_struct *ws)
{
	struct ionic_ibdev *dev =
		container_of(ws, struct ionic_ibdev, reset_work);

	ionic_kill_rdma_admin(dev);
}

static int ionic_create_rdma_admin(struct ionic_ibdev *dev)
{
	struct ionic_eq *eq;
	int eq_i = 0;
	int rc = 0;

	dev->eq_vec = NULL;

	INIT_WORK(&dev->reset_work, ionic_reset_work);
	INIT_WORK(&dev->admin_work, ionic_admin_work);
	INIT_DELAYED_WORK(&dev->admin_dwork, ionic_admin_dwork);
	spin_lock_init(&dev->admin_lock);
	dev->admincq = NULL;
	dev->adminq = NULL;
	dev->admin_armed = false;
	dev->admin_state = IONIC_ADMIN_ACTIVE;

	/* need at least one eq */
	if (!dev->eq_count) {
		rc = -EINVAL;
		goto out;
	}

	dev->eq_vec = kmalloc_array(dev->eq_count, sizeof(*dev->eq_vec), GFP_KERNEL);
	if (!dev->eq_vec) {
		rc = -ENOMEM;
		goto out;
	}

	for (eq_i = 0; eq_i < dev->eq_count; ++eq_i) {
		eq = ionic_create_eq(dev, eq_i + dev->eq_base);
		if (IS_ERR(eq)) {
			rc = PTR_ERR(eq);

			/* need at least one eq */
			if (!eq_i) {
				dev_err(dev->hwdev, "fail create eq %d\n", rc);
				goto out;
			}

			/* ok, just fewer eq than device supports */
			dev_dbg(dev->hwdev, "eq count %d want %d rc %d\n",
				eq_i, dev->eq_count, rc);

			rc = 0;
			break;
		}

		dev->eq_vec[eq_i] = eq;
	}

	dev->admincq = ionic_create_rdma_admincq(dev, 0);
	if (IS_ERR(dev->admincq)) {
		rc = PTR_ERR(dev->admincq);
		goto out;
	}

	dev->adminq = ionic_create_rdma_adminq(dev, dev->admincq->cqid);
	if (IS_ERR(dev->adminq)) {
		rc = PTR_ERR(dev->adminq);
		goto out;
	}

out:
	dev->eq_count = eq_i;

	return rc;
}

static void ionic_destroy_rdma_admin(struct ionic_ibdev *dev)
{
	struct ionic_eq *eq;

	cancel_work_sync(&dev->admin_work);
	cancel_delayed_work_sync(&dev->admin_dwork);
	cancel_work_sync(&dev->reset_work);

	if (dev->adminq)
		__ionic_destroy_rdma_adminq(dev, dev->adminq);

	if (dev->admincq)
		__ionic_destroy_cq(dev, dev->admincq);

	if (dev->eq_vec) {
		while (dev->eq_count > 0) {
			eq = dev->eq_vec[--dev->eq_count];
			ionic_destroy_eq(eq);
		}

		kfree(dev->eq_vec);
	}
}

static void ionic_destroy_ibdev(struct ionic_ibdev *dev)
{
	struct net_device *ndev = dev->ndev;

	list_del(&dev->driver_ent);

	ionic_kill_rdma_admin(dev);

	ib_unregister_device(&dev->ibdev);

	ionic_destroy_rdma_admin(dev);

	kfree(dev->stats_buf);
	kfree(dev->stats_hdrs);

	ionic_dbgfs_rm_dev(dev);

	resid_destroy(&dev->inuse_qpid);
	resid_destroy(&dev->inuse_cqid);
	resid_destroy(&dev->inuse_mrid);
	resid_destroy(&dev->inuse_ahid);
	resid_destroy(&dev->inuse_pdid);
	buddy_destroy(&dev->inuse_restbl);
	tbl_destroy(&dev->qp_tbl);
	tbl_destroy(&dev->cq_tbl);

	ib_dealloc_device(&dev->ibdev);

	dev_put(ndev);
}

#ifdef HAVE_IB_DEVICE_OPS
static const struct ib_device_ops ionic_dev_ops = {
	.alloc_hw_stats		= ionic_alloc_hw_stats,
	.get_hw_stats		= ionic_get_hw_stats,

	.query_device		= ionic_query_device,
	.query_port		= ionic_query_port,
	.get_link_layer		= ionic_get_link_layer,
	.get_netdev		= ionic_get_netdev,
#ifdef HAVE_REQUIRED_IB_GID
	.query_gid		= ionic_query_gid,
	.add_gid		= ionic_add_gid,
	.del_gid		= ionic_del_gid,
#endif
	.query_pkey		= ionic_query_pkey,
	.modify_device		= ionic_modify_device,
	.modify_port		= ionic_modify_port,

	.alloc_ucontext		= ionic_alloc_ucontext,
	.dealloc_ucontext	= ionic_dealloc_ucontext,
	.mmap			= ionic_mmap,

	.alloc_pd		= ionic_alloc_pd,
	.dealloc_pd		= ionic_dealloc_pd,

	.create_ah		= ionic_create_ah,
	.destroy_ah		= ionic_destroy_ah,

	.get_dma_mr		= ionic_get_dma_mr,
	.reg_user_mr		= ionic_reg_user_mr,
	.rereg_user_mr		= ionic_rereg_user_mr,
	.dereg_mr		= ionic_dereg_mr,
	.alloc_mr		= ionic_alloc_mr,
	.map_mr_sg		= ionic_map_mr_sg,

	.alloc_mw		= ionic_alloc_mw,
	.dealloc_mw		= ionic_dealloc_mw,

	.create_cq		= ionic_create_cq,
	.destroy_cq		= ionic_destroy_cq,
	.resize_cq		= ionic_resize_cq,
	.poll_cq		= ionic_poll_cq,
	.req_notify_cq		= ionic_req_notify_cq,

	.create_qp		= ionic_create_qp,
	.modify_qp		= ionic_modify_qp,
	.query_qp		= ionic_query_qp,
	.destroy_qp		= ionic_destroy_qp,
	.post_send		= ionic_post_send,
	.post_recv		= ionic_post_recv,

	.create_srq		= ionic_create_srq,
	.modify_srq		= ionic_modify_srq,
	.query_srq		= ionic_query_srq,
	.destroy_srq		= ionic_destroy_srq,
	.post_srq_recv		= ionic_post_srq_recv,

	.get_port_immutable	= ionic_get_port_immutable,
#ifdef HAVE_GET_DEV_FW_STR
	.get_dev_fw_str		= ionic_get_dev_fw_str,
#endif
#ifdef HAVE_GET_VECTOR_AFFINITY
	.get_vector_affinity	= ionic_get_vector_affinity,
#endif
};
#endif

static struct ionic_ibdev *ionic_create_ibdev(struct lif *lif,
					      struct net_device *ndev)
{
	struct ib_device *ibdev;
	struct ionic_ibdev *dev;
	const union identity *ident;
	struct dentry *lif_dbgfs;
	int rc, val, lif_id, version;

	dev_hold(ndev);

	ident = ionic_api_get_identity(lif, &lif_id);

	version = le16_to_cpu(ident->dev.rdma_version);

	if (version < IONIC_MIN_RDMA_VERSION) {
		netdev_err(ndev, FW_INFO "ionic_rdma: Firmware RDMA Version %u\n",
			   version);
		netdev_err(ndev, FW_INFO "ionic_rdma: Driver Min RDMA Version %u\n",
			   IONIC_MIN_RDMA_VERSION);
		rc = -EINVAL;
		goto err_dev;
	}

	if (version > IONIC_MAX_RDMA_VERSION) {
		netdev_err(ndev, FW_INFO "ionic_rdma: Firmware RDMA Version %u\n",
			   version);
		netdev_err(ndev, FW_INFO "ionic_rdma: Driver Max RDMA Version %u\n",
			   IONIC_MAX_RDMA_VERSION);
		rc = -EINVAL;
		goto err_dev;
	}

	ibdev = ib_alloc_device(sizeof(*dev));
	if (!ibdev) {
		rc = -ENOMEM;
		goto err_dev;
	}

	dev = to_ionic_ibdev(ibdev);
	dev->hwdev = ndev->dev.parent;
	dev->ndev = ndev;
	dev->lif = lif;
	dev->lif_id = lif_id;

	ionic_api_kernel_dbpage(lif, &dev->intr_ctrl,
				&dev->dbid, &dev->dbpage,
				&dev->xxx_dbpage_phys);

	dev->rdma_version = version;
	dev->qp_opcodes = ident->dev.rdma_qp_opcodes;
	dev->admin_opcodes = ident->dev.rdma_admin_opcodes;

	/* base opcodes must be supported, extended opcodes are optional*/
	if (dev->qp_opcodes <= IONIC_V1_OP_BIND_MW) {
		netdev_dbg(ndev, "ionic_rdma: qp opcodes %d want min %d\n",
			   dev->qp_opcodes, IONIC_V1_OP_BIND_MW + 1);
		rc = -ENODEV;
		goto err_dev;
	}

	/* need at least one rdma admin queue (driver creates one) */
	val = le32_to_cpu(ident->dev.rdma_aq_qtype.qid_count);
	if (!val) {
		netdev_dbg(ndev, "ionic_rdma: No RDMA Admin Queue\n");
		rc = -ENODEV;
		goto err_dev;
	}

	/* qp ids start at zero, and sq id == qp id */
	val = le32_to_cpu(ident->dev.rdma_sq_qtype.qid_base);
	if (val) {
		netdev_dbg(ndev, "ionic_rdma: Nonzero sq qid base %u\n", val);
		rc = -EINVAL;
		goto err_dev;
	}

	/* qp ids start at zero, and rq id == qp id */
	val = le32_to_cpu(ident->dev.rdma_rq_qtype.qid_base);
	if (val) {
		netdev_dbg(ndev, "ionic_rdma: Nonzero rq qid base %u\n", val);
		rc = -EINVAL;
		goto err_dev;
	}

	/* driver supports these qtypes starting at nonzero base */
	dev->aq_base = le32_to_cpu(ident->dev.rdma_aq_qtype.qid_base);
	dev->cq_base = le32_to_cpu(ident->dev.rdma_cq_qtype.qid_base);
	dev->eq_base = le32_to_cpu(ident->dev.rdma_eq_qtype.qid_base);

	/* eq count may be reduced by ionic_create_rdma_admin */
	dev->eq_count = le32_to_cpu(ident->dev.rdma_eq_qtype.qid_count);

	dev->aq_qtype = ident->dev.rdma_aq_qtype.qtype;
	dev->sq_qtype = ident->dev.rdma_sq_qtype.qtype;
	dev->rq_qtype = ident->dev.rdma_rq_qtype.qtype;
	dev->cq_qtype = ident->dev.rdma_cq_qtype.qtype;
	dev->eq_qtype = ident->dev.rdma_eq_qtype.qtype;

	dev->max_stride = ident->dev.rdma_max_stride;
	dev->cl_stride = ident->dev.rdma_cl_stride;
	dev->pte_stride = ident->dev.rdma_pte_stride;
	dev->rrq_stride = ident->dev.rdma_rrq_stride;
	dev->rsq_stride = ident->dev.rdma_rsq_stride;

	mutex_init(&dev->tbl_lock);

	tbl_init(&dev->qp_tbl);
	tbl_init(&dev->cq_tbl);

	mutex_init(&dev->inuse_lock);
	spin_lock_init(&dev->inuse_splock);

	if (ionic_xxx_pgidx)
		rc = buddy_init(&dev->inuse_restbl,
				le32_to_cpu(ident->dev.nrdma_pts_per_lif) >>
				(dev->cl_stride - dev->pte_stride));
	else
		rc = buddy_init(&dev->inuse_restbl,
				le32_to_cpu(ident->dev.nrdma_pts_per_lif));
	if (rc)
		goto err_restbl;

	rc = resid_init(&dev->inuse_pdid, ionic_max_pd);
	if (rc)
		goto err_pdid;

	rc = resid_init(&dev->inuse_ahid,
			le32_to_cpu(ident->dev.nrdma_ahs_per_lif));
	if (rc)
		goto err_ahid;

	rc = resid_init(&dev->inuse_mrid,
			le32_to_cpu(ident->dev.nrdma_mrs_per_lif));
	if (rc)
		goto err_mrid;

	/* skip reserved lkey */
	dev->inuse_mrid.next_id = 1;
	dev->next_mrkey = 1;

	rc = resid_init(&dev->inuse_cqid,
			le32_to_cpu(ident->dev.rdma_cq_qtype.qid_count));
	if (rc)
		goto err_cqid;

	/* prefer srqids after qpids */
	dev->size_qpid = le32_to_cpu(ident->dev.rdma_sq_qtype.qid_count);
	dev->size_srqid = le32_to_cpu(ident->dev.rdma_rq_qtype.qid_count);
	dev->next_srqid = dev->size_qpid;

	rc = resid_init(&dev->inuse_qpid, max(dev->size_qpid,
					      dev->size_srqid));
	if (rc)
		goto err_qpid;

	if (ionic_xxx_qid_skip > 0) {
		ionic_xxx_resid_skip(&dev->inuse_qpid);
		ionic_xxx_resid_skip(&dev->inuse_cqid);
	}

	/* skip reserved SMI and GSI qpids */
	dev->inuse_qpid.next_id = 2;

	/* initialize dev_attr (TODO: remove from struct to save space) */
	dev->dev_attr.fw_ver = 0;
	dev->dev_attr.sys_image_guid = 0;
	dev->dev_attr.max_mr_size =
		(dev->inuse_restbl.inuse_size * PAGE_SIZE / 2) <<
		(dev->cl_stride - dev->pte_stride);
	dev->dev_attr.page_size_cap = ~0;
	dev->dev_attr.vendor_id = 0;
	dev->dev_attr.vendor_part_id = 0;
	dev->dev_attr.hw_ver = 0;
	dev->dev_attr.max_qp = dev->size_qpid;
	dev->dev_attr.max_qp_wr = IONIC_MAX_DEPTH;
	dev->dev_attr.device_cap_flags =
		IB_DEVICE_LOCAL_DMA_LKEY |
		IB_DEVICE_MEM_WINDOW |
		IB_DEVICE_MEM_MGT_EXTENSIONS |
		IB_DEVICE_MEM_WINDOW_TYPE_2B |
		0;
#ifdef HAVE_IBDEV_MAX_SEND_RECV_SGE
	dev->dev_attr.max_send_sge = ionic_v1_send_wqe_max_sge(dev->max_stride);
	dev->dev_attr.max_recv_sge = ionic_v1_recv_wqe_max_sge(dev->max_stride);
#else
	dev->dev_attr.max_sge =
		min(ionic_v1_send_wqe_max_sge(dev->max_stride),
		    ionic_v1_recv_wqe_max_sge(dev->max_stride));
#endif
	dev->dev_attr.max_sge_rd = 0;
	dev->dev_attr.max_cq = dev->inuse_cqid.inuse_size;
	dev->dev_attr.max_cqe = IONIC_MAX_CQ_DEPTH;
	dev->dev_attr.max_mr = dev->inuse_mrid.inuse_size;
	dev->dev_attr.max_pd = ionic_max_pd;
	dev->dev_attr.max_qp_rd_atom = 16;
	dev->dev_attr.max_ee_rd_atom = 0;
	dev->dev_attr.max_res_rd_atom = 16;
	dev->dev_attr.max_qp_init_rd_atom = 16;
	dev->dev_attr.max_ee_init_rd_atom = 0;
	dev->dev_attr.atomic_cap = IB_ATOMIC_HCA; /* XXX or global? */
	dev->dev_attr.masked_atomic_cap = IB_ATOMIC_HCA; /* XXX or global? */
	dev->dev_attr.max_mw = dev->inuse_mrid.inuse_size;
	dev->dev_attr.max_mcast_grp = 0;
	dev->dev_attr.max_mcast_qp_attach = 0;
	dev->dev_attr.max_ah = dev->inuse_ahid.inuse_size;
	dev->dev_attr.max_srq = dev->size_srqid;
	dev->dev_attr.max_srq_wr = IONIC_MAX_DEPTH;
	dev->dev_attr.max_srq_sge = ionic_v1_recv_wqe_max_sge(dev->max_stride);
	dev->dev_attr.max_fast_reg_page_list_len =
		(dev->inuse_restbl.inuse_size / 2) <<
		(dev->cl_stride - dev->pte_stride);
	dev->dev_attr.max_pkeys = 1;

	/* XXX hardcode values, intentionally low, should come from identify */
	if (netif_running(ndev) && netif_carrier_ok(ndev)) {
		dev->port_attr.state = IB_PORT_ACTIVE;
		dev->port_attr.phys_state = PHYS_STATE_UP;
	} else {
		dev->port_attr.state = IB_PORT_DOWN;
		dev->port_attr.phys_state = PHYS_STATE_DOWN;
	}
#ifdef HAVE_NETDEV_MAX_MTU
	dev->port_attr.max_mtu = ib_mtu_int_to_enum(ndev->max_mtu);
#else
	dev->port_attr.max_mtu = IB_MTU_4096;
#endif
	dev->port_attr.active_mtu = ib_mtu_int_to_enum(ndev->mtu);
	dev->port_attr.gid_tbl_len = ionic_max_gid;
#ifdef HAVE_IBDEV_PORT_CAP_FLAGS
	dev->port_attr.port_cap_flags = IB_PORT_IP_BASED_GIDS;
#endif
#ifdef HAVE_IBDEV_IP_GIDS
	dev->port_attr.ip_gids = true;
#endif
	dev->port_attr.max_msg_sz = 0x80000000;
	dev->port_attr.pkey_tbl_len = 1;
	dev->port_attr.max_vl_num = 1;
	dev->port_attr.subnet_prefix = 0xfe80000000000000ull;

	if (ionic_dbgfs_enable)
		lif_dbgfs = ionic_api_get_debugfs(lif);
	else
		lif_dbgfs = NULL;

	ionic_dbgfs_add_dev(dev, lif_dbgfs);

	rc = ionic_rdma_reset_devcmd(dev);
	if (rc)
		goto err_reset;

	rc = ionic_create_rdma_admin(dev);
	if (rc)
		goto err_register;

	ibdev->owner = THIS_MODULE;
	ibdev->dev.parent = dev->hwdev;

#ifndef HAVE_IB_REGISTER_DEVICE_NAME
	strlcpy(ibdev->name, "ionic_%d", IB_DEVICE_NAME_MAX);
#endif
	strlcpy(ibdev->node_desc, DEVICE_DESCRIPTION, IB_DEVICE_NODE_DESC_MAX);

	ibdev->node_type = RDMA_NODE_IB_CA;
	ibdev->phys_port_cnt = 1;
	ibdev->num_comp_vectors = dev->eq_count;

	addrconf_ifid_eui48((u8 *)&ibdev->node_guid, ndev);

	ibdev->uverbs_abi_ver = IONIC_ABI_VERSION;
	ibdev->uverbs_cmd_mask =
		BIT_ULL(IB_USER_VERBS_CMD_GET_CONTEXT)		|
		BIT_ULL(IB_USER_VERBS_CMD_QUERY_DEVICE)		|
		BIT_ULL(IB_USER_VERBS_CMD_QUERY_PORT)		|
		BIT_ULL(IB_USER_VERBS_CMD_ALLOC_PD)		|
		BIT_ULL(IB_USER_VERBS_CMD_DEALLOC_PD)		|
		BIT_ULL(IB_USER_VERBS_CMD_CREATE_AH)		|
		BIT_ULL(IB_USER_VERBS_CMD_MODIFY_AH)		|
		BIT_ULL(IB_USER_VERBS_CMD_QUERY_AH)		|
		BIT_ULL(IB_USER_VERBS_CMD_DESTROY_AH)		|
		BIT_ULL(IB_USER_VERBS_CMD_REG_MR)		|
		BIT_ULL(IB_USER_VERBS_CMD_REG_SMR)		|
		BIT_ULL(IB_USER_VERBS_CMD_REREG_MR)		|
		BIT_ULL(IB_USER_VERBS_CMD_DEREG_MR)		|
		BIT_ULL(IB_USER_VERBS_CMD_ALLOC_MW)		|
		BIT_ULL(IB_USER_VERBS_CMD_BIND_MW)		|
		BIT_ULL(IB_USER_VERBS_CMD_DEALLOC_MW)		|
		BIT_ULL(IB_USER_VERBS_CMD_CREATE_COMP_CHANNEL)	|
		BIT_ULL(IB_USER_VERBS_CMD_CREATE_CQ)		|
		BIT_ULL(IB_USER_VERBS_CMD_RESIZE_CQ)		|
		BIT_ULL(IB_USER_VERBS_CMD_DESTROY_CQ)		|
		BIT_ULL(IB_USER_VERBS_CMD_POLL_CQ)		|
		BIT_ULL(IB_USER_VERBS_CMD_PEEK_CQ)		|
		BIT_ULL(IB_USER_VERBS_CMD_REQ_NOTIFY_CQ)	|
		BIT_ULL(IB_USER_VERBS_CMD_CREATE_QP)		|
		BIT_ULL(IB_USER_VERBS_CMD_QUERY_QP)		|
		BIT_ULL(IB_USER_VERBS_CMD_MODIFY_QP)		|
		BIT_ULL(IB_USER_VERBS_CMD_DESTROY_QP)		|
		BIT_ULL(IB_USER_VERBS_CMD_POST_SEND)		|
		BIT_ULL(IB_USER_VERBS_CMD_POST_RECV)		|
		BIT_ULL(IB_USER_VERBS_CMD_CREATE_SRQ)		|
		BIT_ULL(IB_USER_VERBS_CMD_MODIFY_SRQ)		|
		BIT_ULL(IB_USER_VERBS_CMD_QUERY_SRQ)		|
		BIT_ULL(IB_USER_VERBS_CMD_DESTROY_SRQ)		|
		BIT_ULL(IB_USER_VERBS_CMD_POST_SRQ_RECV)	|
		BIT_ULL(IB_USER_VERBS_CMD_OPEN_XRCD)		|
		BIT_ULL(IB_USER_VERBS_CMD_CLOSE_XRCD)		|
		BIT_ULL(IB_USER_VERBS_CMD_CREATE_XSRQ)		|
		BIT_ULL(IB_USER_VERBS_CMD_OPEN_QP)		|
		0;
	ibdev->uverbs_ex_cmd_mask =
		BIT_ULL(IB_USER_VERBS_EX_CMD_CREATE_QP)		|
#ifdef HAVE_EX_CMD_MODIFY_QP
		BIT_ULL(IB_USER_VERBS_EX_CMD_MODIFY_QP)		|
#endif
		0;

#ifdef HAVE_IB_DEVICE_OPS
	ib_set_device_ops(&dev->ibdev, &ionic_dev_ops);
#else
	dev->ibdev.alloc_hw_stats	= ionic_alloc_hw_stats;
	dev->ibdev.get_hw_stats		= ionic_get_hw_stats;

	dev->ibdev.query_device		= ionic_query_device;
	dev->ibdev.query_port		= ionic_query_port;
	dev->ibdev.get_link_layer	= ionic_get_link_layer;
	dev->ibdev.get_netdev		= ionic_get_netdev;
#ifdef HAVE_REQUIRED_IB_GID
	dev->ibdev.query_gid		= ionic_query_gid;
	dev->ibdev.add_gid		= ionic_add_gid;
	dev->ibdev.del_gid		= ionic_del_gid;
#endif
	dev->ibdev.query_pkey		= ionic_query_pkey;
	dev->ibdev.modify_device	= ionic_modify_device;
	dev->ibdev.modify_port		= ionic_modify_port;

	dev->ibdev.alloc_ucontext	= ionic_alloc_ucontext;
	dev->ibdev.dealloc_ucontext	= ionic_dealloc_ucontext;
	dev->ibdev.mmap			= ionic_mmap;

	dev->ibdev.alloc_pd		= ionic_alloc_pd;
	dev->ibdev.dealloc_pd		= ionic_dealloc_pd;

	dev->ibdev.create_ah		= ionic_create_ah;
	dev->ibdev.destroy_ah		= ionic_destroy_ah;

	dev->ibdev.get_dma_mr		= ionic_get_dma_mr;
	dev->ibdev.reg_user_mr		= ionic_reg_user_mr;
	dev->ibdev.rereg_user_mr	= ionic_rereg_user_mr;
	dev->ibdev.dereg_mr		= ionic_dereg_mr;
	dev->ibdev.alloc_mr		= ionic_alloc_mr;
	dev->ibdev.map_mr_sg		= ionic_map_mr_sg;

	dev->ibdev.alloc_mw		= ionic_alloc_mw;
	dev->ibdev.dealloc_mw		= ionic_dealloc_mw;

	dev->ibdev.create_cq		= ionic_create_cq;
	dev->ibdev.destroy_cq		= ionic_destroy_cq;
	dev->ibdev.resize_cq		= ionic_resize_cq;
	dev->ibdev.poll_cq		= ionic_poll_cq;
	dev->ibdev.req_notify_cq	= ionic_req_notify_cq;

	dev->ibdev.create_qp		= ionic_create_qp;
	dev->ibdev.modify_qp		= ionic_modify_qp;
	dev->ibdev.query_qp		= ionic_query_qp;
	dev->ibdev.destroy_qp		= ionic_destroy_qp;
	dev->ibdev.post_send		= ionic_post_send;
	dev->ibdev.post_recv		= ionic_post_recv;

	dev->ibdev.create_srq		= ionic_create_srq;
	dev->ibdev.modify_srq		= ionic_modify_srq;
	dev->ibdev.query_srq		= ionic_query_srq;
	dev->ibdev.destroy_srq		= ionic_destroy_srq;
	dev->ibdev.post_srq_recv	= ionic_post_srq_recv;

	dev->ibdev.get_port_immutable	= ionic_get_port_immutable;
#ifdef HAVE_GET_DEV_FW_STR
	dev->ibdev.get_dev_fw_str	= ionic_get_dev_fw_str;
#endif
#ifdef HAVE_GET_VECTOR_AFFINITY
	dev->ibdev.get_vector_affinity	= ionic_get_vector_affinity;
#endif
#endif

	if (ionic_xxx_noop) {
		rc = ionic_noop_cmd(dev);
		if (rc)
			dev_err(&dev->ibdev.dev, "admin queue may be inoperable\n");
	}

#ifdef HAVE_REQUIRED_DMA_DEVICE
	ibdev->dma_device = ibdev->dev.parent;
#endif

#ifdef HAVE_IB_REGISTER_DEVICE_NAME
	rc = ib_register_device(ibdev, "ionic_%d", NULL);
#else
	rc = ib_register_device(ibdev, NULL);
#endif
	if (rc)
		goto err_register;

	list_add(&dev->driver_ent, &ionic_ibdev_list);

	return dev;

err_register:
	ionic_kill_rdma_admin(dev);
	ionic_destroy_rdma_admin(dev);
	kfree(dev->stats_buf);
	kfree(dev->stats_hdrs);
err_reset:
	ionic_dbgfs_rm_dev(dev);
	resid_destroy(&dev->inuse_qpid);
err_qpid:
	resid_destroy(&dev->inuse_cqid);
err_cqid:
	resid_destroy(&dev->inuse_mrid);
err_mrid:
	resid_destroy(&dev->inuse_ahid);
err_ahid:
	resid_destroy(&dev->inuse_pdid);
err_pdid:
	buddy_destroy(&dev->inuse_restbl);
err_restbl:
	tbl_destroy(&dev->qp_tbl);
	tbl_destroy(&dev->cq_tbl);
	ib_dealloc_device(ibdev);
err_dev:
	dev_put(ndev);
	return ERR_PTR(rc);
}

struct ionic_netdev_work {
	struct work_struct ws;
	unsigned long event;
	struct net_device *ndev;
	struct lif *lif;
};

static void ionic_netdev_work(struct work_struct *ws)
{
	struct ionic_netdev_work *work =
		container_of(ws, struct ionic_netdev_work, ws);
	struct net_device *ndev = work->ndev;
	struct ionic_ibdev *dev;
	int rc;

	dev = ionic_api_get_private(work->lif, IONIC_PRSN_RDMA);

	switch (work->event) {
	case NETDEV_REGISTER:
		if (dev) {
			netdev_dbg(ndev, "already registered\n");
			break;
		}

		netdev_dbg(ndev, "register ibdev\n");

		dev = ionic_create_ibdev(work->lif, ndev);
		if (IS_ERR(dev)) {
			netdev_dbg(ndev, "error register ibdev %d\n",
				   (int)PTR_ERR(dev));
			break;
		}

		rc = ionic_api_set_private(work->lif, dev, ionic_kill_ibdev,
					   IONIC_PRSN_RDMA);
		if (rc) {
			netdev_dbg(ndev, "error set private %d\n", rc);
			ionic_destroy_ibdev(dev);
		}

		break;

	case NETDEV_UNREGISTER:
		if (!dev) {
			netdev_dbg(ndev, "not registered\n");
			break;
		}

		netdev_dbg(ndev, "unregister ibdev\n");

		ionic_api_set_private(work->lif, NULL, NULL,
				      IONIC_PRSN_RDMA);
		ionic_destroy_ibdev(dev);

		break;

	case NETDEV_UP:
		if (!dev)
			break;

		dev->port_attr.state = IB_PORT_ACTIVE;
		dev->port_attr.phys_state = PHYS_STATE_UP;
		ionic_port_event(dev, IB_EVENT_PORT_ACTIVE);

		break;

	case NETDEV_DOWN:
		if (!dev)
			break;

		dev->port_attr.state = IB_PORT_DOWN;
		dev->port_attr.phys_state = PHYS_STATE_DOWN;
		ionic_port_event(dev, IB_EVENT_PORT_ERR);

		break;

	case NETDEV_CHANGE:
		if (!dev)
			break;

		if (netif_running(ndev) && netif_carrier_ok(ndev)) {
			dev->port_attr.state = IB_PORT_ACTIVE;
			dev->port_attr.phys_state = PHYS_STATE_UP;
			ionic_port_event(dev, IB_EVENT_PORT_ACTIVE);
		} else {
			dev->port_attr.state = IB_PORT_DOWN;
			dev->port_attr.phys_state = PHYS_STATE_DOWN;
			ionic_port_event(dev, IB_EVENT_PORT_ERR);
		}

		break;

	case NETDEV_CHANGEMTU:
		if (!dev)
			break;

		dev->port_attr.active_mtu = ib_mtu_int_to_enum(ndev->mtu);

		break;

	default:
		if (!dev)
			break;

		netdev_dbg(ndev, "unhandled event %lu\n", work->event);
	}

	dev_put(ndev);
	kfree(work);
}

static int ionic_netdev_event(struct notifier_block *notifier,
			      unsigned long event, void *ptr)
{
	struct ionic_netdev_work *work;
	struct net_device *ndev;
	struct lif *lif;
	int rc;

	ndev = netdev_notifier_info_to_dev(ptr);

	lif = get_netdev_ionic_lif(ndev, IONIC_API_VERSION, IONIC_PRSN_RDMA);
	if (!lif) {
		pr_devel("unrecognized netdev: %s\n", netdev_name(ndev));
		goto out;
	}

	pr_devel("ionic netdev: %s\n", netdev_name(ndev));
	netdev_dbg(ndev, "event %lu\n", event);

	work = kmalloc(sizeof(*work), GFP_ATOMIC);
	if (WARN_ON_ONCE(!work)) {
		rc = -ENOMEM;
		goto err_work;
	}

	dev_hold(ndev);

	INIT_WORK(&work->ws, ionic_netdev_work);
	work->event = event;
	work->ndev = ndev;
	work->lif = lif;

	queue_work(ionic_dev_workq, &work->ws);

out:
	return NOTIFY_DONE;

err_work:
	return notifier_from_errno(rc);
}

static struct notifier_block ionic_netdev_notifier = {
	.notifier_call = ionic_netdev_event,
};

static int __init ionic_mod_init(void)
{
	int rc;

	pr_info("%s v%s : %s\n", DRIVER_NAME, DRIVER_VERSION, DRIVER_DESCRIPTION);

	ionic_dev_workq = create_singlethread_workqueue(DRIVER_NAME "-dev");
	if (!ionic_dev_workq) {
		rc = -ENOMEM;
		goto err_dev_workq;
	}

	ionic_evt_workq = create_workqueue(DRIVER_NAME "-evt");
	if (!ionic_evt_workq) {
		rc = -ENOMEM;
		goto err_evt_workq;
	}

	rc = register_netdevice_notifier(&ionic_netdev_notifier);
	if (rc)
		goto err_notifier;

	return 0;

err_notifier:
	destroy_workqueue(ionic_evt_workq);
err_evt_workq:
	destroy_workqueue(ionic_dev_workq);
err_dev_workq:
	return rc;
}

static void __exit ionic_exit_work(struct work_struct *ws)
{
	struct ionic_ibdev *dev, *dev_next;

	list_for_each_entry_safe_reverse(dev, dev_next, &ionic_ibdev_list,
					 driver_ent) {
		ionic_api_set_private(dev->lif, NULL, NULL,
				      IONIC_PRSN_RDMA);
		ionic_destroy_ibdev(dev);
	}
}

static void __exit ionic_mod_exit(void)
{
	struct work_struct ws;

	unregister_netdevice_notifier(&ionic_netdev_notifier);

	INIT_WORK_ONSTACK(&ws, ionic_exit_work);
	queue_work(ionic_dev_workq, &ws);
	flush_work(&ws);
	destroy_work_on_stack(&ws);

	destroy_workqueue(ionic_evt_workq);
	destroy_workqueue(ionic_dev_workq);

	BUILD_BUG_ON(sizeof(struct ionic_v1_cqe) != 32);
	BUILD_BUG_ON(sizeof(struct ionic_v1_base_hdr) != 16);
	BUILD_BUG_ON(sizeof(struct ionic_v1_recv_bdy) != 48);
	BUILD_BUG_ON(sizeof(struct ionic_v1_common_bdy) != 48);
	BUILD_BUG_ON(sizeof(struct ionic_v1_atomic_bdy) != 48);
	BUILD_BUG_ON(sizeof(struct ionic_v1_reg_mr_bdy) != 48);
	BUILD_BUG_ON(sizeof(struct ionic_v1_bind_mw_bdy) != 48);
	BUILD_BUG_ON(sizeof(struct ionic_v1_wqe) != 64);
	BUILD_BUG_ON(sizeof(struct ionic_v1_admin_wqe) != 64);
	BUILD_BUG_ON(sizeof(struct ionic_v1_eqe) != 4);
}

module_init(ionic_mod_init);
module_exit(ionic_mod_exit);
