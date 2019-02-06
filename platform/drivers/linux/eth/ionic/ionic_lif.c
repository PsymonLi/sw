// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2017 - 2019 Pensando Systems, Inc */

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/pci.h>

#include "ionic.h"
#include "ionic_bus.h"
#include "ionic_api.h"
#include "ionic_lif.h"
#include "ionic_txrx.h"
#include "ionic_ethtool.h"
#include "ionic_debugfs.h"

static void ionic_lif_rx_mode(struct lif *lif, unsigned int rx_mode);
static int ionic_lif_addr_add(struct lif *lif, const u8 *addr);
static int ionic_lif_addr_del(struct lif *lif, const u8 *addr);
static void ionic_link_status_check(struct lif *lif);

static int ionic_lif_stop(struct lif *lif);
static struct lif *ionic_lif_alloc(struct ionic *ionic, unsigned int index);
static int ionic_lif_init(struct lif *lif);
static int ionic_txrx_alloc(struct lif *lif);
static int ionic_txrx_init(struct lif *lif);
static void ionic_lif_free(struct lif *lif);
static void ionic_lif_qcq_deinit(struct lif *lif, struct qcq *qcq);
static void ionic_lif_deinit(struct lif *lif);
static void ionic_lif_rss_teardown(struct lif *lif);
static void ionic_qcq_free(struct lif *lif, struct qcq *qcq);
static int ionic_lif_txqs_init(struct lif *lif);
static int ionic_lif_rxqs_init(struct lif *lif);
static int ionic_station_set(struct lif *lif);
static int ionic_lif_rss_setup(struct lif *lif);
static void ionic_lif_set_netdev_info(struct lif *lif);
static int ionic_intr_remaining(struct ionic *ionic);


static void ionic_lif_deferred_work(struct work_struct *work)
{
	struct lif *lif = container_of(work, struct lif, deferred.work);
	struct deferred *def = &lif->deferred;
	struct deferred_work *w = NULL;

	spin_lock_bh(&def->lock);
	if (!list_empty(&def->list)) {
		w = list_first_entry(&def->list, struct deferred_work, list);
		list_del(&w->list);
	}
	spin_unlock_bh(&def->lock);

	if (w) {
		switch (w->type) {
		case DW_TYPE_RX_MODE:
			ionic_lif_rx_mode(lif, w->rx_mode);
			break;
		case DW_TYPE_RX_ADDR_ADD:
			ionic_lif_addr_add(lif, w->addr);
			break;
		case DW_TYPE_RX_ADDR_DEL:
			ionic_lif_addr_del(lif, w->addr);
			break;
		case DW_TYPE_LINK_STATUS:
			ionic_link_status_check(lif);
			break;
		};
		kfree(w);
		schedule_work(&def->work);
	}
}

static void ionic_lif_deferred_enqueue(struct deferred *def,
				       struct deferred_work *work)
{
	spin_lock_bh(&def->lock);
	list_add_tail(&work->list, &def->list);
	spin_unlock_bh(&def->lock);
	schedule_work(&def->work);
}

static int ionic_qcq_enable(struct qcq *qcq)
{
	struct queue *q = &qcq->q;
	struct lif *lif = q->lif;
	struct device *dev = lif->ionic->dev;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.q_enable = {
			.opcode = CMD_OPCODE_Q_ENABLE,
			.qid = q->qid,
			.qtype = q->qtype,
		},
	};
	int err;

	dev_dbg(dev, "q_enable.qid %d q_enable.qtype %d\n",
		ctx.cmd.q_enable.qid, ctx.cmd.q_enable.qtype);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	if (qcq->intr.index != INTR_INDEX_NOT_ASSIGNED) {
		napi_enable(&qcq->napi);
		ionic_intr_mask(&qcq->intr, false);
	}

	return 0;
}

static int ionic_lif_open(struct lif *lif)
{
	unsigned int i;
	int err;

	dev_dbg(lif->ionic->dev, "%s: %s\n", __func__, lif->name);
	err = ionic_txrx_alloc(lif);
	if (err)
		return err;

	err = ionic_txrx_init(lif);
	if (err)
		goto err_out;

	for (i = 0; i < lif->nxqs; i++) {
		err = ionic_qcq_enable(lif->txqcqs[i]);
		if (err)
			goto err_out;

		ionic_rx_fill(&lif->rxqcqs[i]->q);
		err = ionic_qcq_enable(lif->rxqcqs[i]);
		if (err)
			goto err_out;
	}

	if (!is_master_lif(lif) && lif->upper_dev &&
	    netif_running(lif->ionic->master_lif->netdev)) {
		netif_carrier_on(lif->upper_dev);
		netif_tx_wake_all_queues(lif->upper_dev);

	}
	set_bit(LIF_UP, lif->state);

	return 0;

err_out:
	ionic_lif_stop(lif);
	return err;
}

int ionic_open(struct net_device *netdev)
{
	struct lif *lif = netdev_priv(netdev);
	struct list_head *cur, *tmp;
	struct lif *slif;
	int err;

	netif_carrier_off(netdev);

	err = ionic_lif_open(lif);
	if (err)
		return err;

	ionic_link_status_check(lif);
	if (netif_carrier_ok(netdev))
		netif_tx_wake_all_queues(netdev);

	list_for_each_safe(cur, tmp, &lif->ionic->lifs) {
		slif = list_entry(cur, struct lif, list);
		if (!is_master_lif(slif))
			ionic_lif_open(slif);
	}

	return 0;
}

static int ionic_qcq_disable(struct qcq *qcq)
{
	struct queue *q = &qcq->q;
	struct lif *lif = q->lif;
	struct device *dev = lif->ionic->dev;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.q_disable = {
			.opcode = CMD_OPCODE_Q_DISABLE,
			.qid = q->qid,
			.qtype = q->qtype,
		},
	};

	dev_dbg(dev, "q_disable.qid %d q_disable.qtype %d\n",
		ctx.cmd.q_disable.qid, ctx.cmd.q_disable.qtype);

	if (qcq->intr.index != INTR_INDEX_NOT_ASSIGNED) {
		ionic_intr_mask(&qcq->intr, true);
		synchronize_irq(qcq->intr.vector);
		napi_disable(&qcq->napi);
	}

	return ionic_adminq_post_wait(lif, &ctx);
}

static int ionic_lif_stop(struct lif *lif)
{
	struct net_device *ndev;
	unsigned int i;
	int err = 0;

	if (!test_bit(LIF_UP, lif->state)) {
		dev_dbg(lif->ionic->dev, "%s: %s state=DOWN\n",
			__func__, lif->name);
		return 0;
	}
	dev_dbg(lif->ionic->dev, "%s: %s state=UP\n", __func__, lif->name);
	clear_bit(LIF_UP, lif->state);

	if (!is_master_lif(lif) && lif->upper_dev)
		ndev = lif->upper_dev;
	else
		ndev = lif->netdev;
	netif_tx_stop_all_queues(ndev);
	netif_tx_disable(ndev);
	netif_carrier_off(ndev);

	for (i = 0; i < lif->nxqs; i++) {
		if (lif->txqcqs[i]) {
			err = ionic_qcq_disable(lif->txqcqs[i]);
			if (err)
				break;
			ionic_lif_qcq_deinit(lif, lif->txqcqs[i]);
		}

		if (lif->rxqcqs[i]) {
			err = ionic_qcq_disable(lif->rxqcqs[i]);
			if (err)
				break;
			ionic_rx_flush(&lif->rxqcqs[i]->cq);
			ionic_lif_qcq_deinit(lif, lif->rxqcqs[i]);
		}
	}

	ionic_lif_rss_teardown(lif);

	for (i = 0; i < lif->nxqs; i++) {
		if (lif->rxqcqs[i])
			ionic_rx_empty(&lif->rxqcqs[i]->q);
		ionic_qcq_free(lif, lif->rxqcqs[i]);
	}
	devm_kfree(lif->ionic->dev, lif->rxqcqs);
	lif->rxqcqs = NULL;

	for (i = 0; i < lif->nxqs; i++)
		ionic_qcq_free(lif, lif->txqcqs[i]);
	devm_kfree(lif->ionic->dev, lif->txqcqs);
	lif->txqcqs = NULL;

	return err;
}

static void ionic_slaves_stop(struct ionic *ionic)
{
	struct list_head *cur, *tmp;
	struct lif *lif;

	list_for_each_safe(cur, tmp, &ionic->lifs) {
		lif = list_entry(cur, struct lif, list);
		if (!is_master_lif(lif))
			ionic_lif_stop(lif);
	}
}

int ionic_stop(struct net_device *netdev)
{
	struct lif *lif = netdev_priv(netdev);

	ionic_slaves_stop(lif->ionic);

	return ionic_lif_stop(lif);
}

static bool ionic_adminq_service(struct cq *cq, struct cq_info *cq_info)
{
	struct admin_comp *comp = cq_info->cq_desc;

	if (comp->color != cq->done_color)
		return false;

	ionic_q_service(cq->bound_q, cq_info, comp->comp_index);

	return true;
}

static int ionic_adminq_napi(struct napi_struct *napi, int budget)
{
	return ionic_napi(napi, budget, ionic_adminq_service, NULL, NULL);
}

static void ionic_link_status_check(struct lif *lif)
{
	struct net_device *netdev = lif->netdev;
	bool link_up;

	clear_bit(LIF_LINK_CHECK_NEEDED, lif->state);

	if (!lif->notifyblock)
		return;

	link_up = lif->notifyblock->link_status;

	/* filter out the no-change cases */
	if ((link_up && netif_carrier_ok(netdev)) ||
	    (!link_up && !netif_carrier_ok(netdev)))
		return;

	if (link_up) {
		netdev_info(netdev, "Link up - %d Gbps\n",
			    lif->notifyblock->link_speed);
		netif_carrier_on(netdev);

		if (test_bit(LIF_UP, lif->state))
			netif_tx_wake_all_queues(lif->netdev);
	} else {
		if (test_bit(LIF_UP, lif->state))
			netif_tx_stop_all_queues(netdev);

		netdev_info(netdev, "Link down\n");
		netif_carrier_off(netdev);
	}

	/* TODO: Propagate link status to other lifs on
	 *       this ionic device.
	 */
}

static bool ionic_notifyq_cb(struct cq *cq, struct cq_info *cq_info)
{
	union notifyq_comp *comp = cq_info->cq_desc;
	struct net_device *netdev;
	struct queue *q;
	struct lif *lif;

	q = cq->bound_q;
	lif = q->info[0].cb_arg;
	netdev = lif->netdev;

	/* Have we run out of new completions to process? */
	if (!(comp->event.eid > lif->last_eid))
		return false;

	lif->last_eid = comp->event.eid;

	dev_dbg(&netdev->dev, "notifyq event:\n");
	dynamic_hex_dump("event ", DUMP_PREFIX_OFFSET, 16, 1,
			 comp, sizeof(*comp), true);

	switch (comp->event.ecode) {
	case EVENT_OPCODE_LINK_CHANGE:
		netdev_info(netdev, "Notifyq EVENT_OPCODE_LINK_CHANGE eid=%lld\n",
			    comp->event.eid);
		netdev_info(netdev,
			    "  link_status=%d link_errors=0x%02x phy_type=%d link_speed=%d autoneg=%d\n",
			    comp->link_change.link_status,
			    comp->link_change.link_error_bits,
			    comp->link_change.phy_type,
			    comp->link_change.link_speed,
			    comp->link_change.autoneg_status);

		set_bit(LIF_LINK_CHECK_NEEDED, lif->state);

		break;
	case EVENT_OPCODE_RESET:
		netdev_info(netdev, "Notifyq EVENT_OPCODE_RESET eid=%lld\n",
			    comp->event.eid);
		netdev_info(netdev, "  reset_code=%d state=%d\n",
			    comp->reset.reset_code,
			    comp->reset.state);
		break;
	case EVENT_OPCODE_HEARTBEAT:
		netdev_info(netdev, "Notifyq EVENT_OPCODE_HEARTBEAT eid=%lld\n",
			    comp->event.eid);
		break;
	case EVENT_OPCODE_LOG:
		netdev_info(netdev, "Notifyq EVENT_OPCODE_LOG eid=%lld\n",
			    comp->event.eid);
		print_hex_dump(KERN_INFO, "notifyq ", DUMP_PREFIX_OFFSET, 16, 1,
			       comp->log.data, sizeof(comp->log.data), true);
		break;
	default:
		netdev_warn(netdev, "Notifyq bad event ecode=%d eid=%lld\n",
			    comp->event.ecode, comp->event.eid);
		break;
	}

	return true;
}

static int ionic_notifyq_napi(struct napi_struct *napi, int budget)
{
	struct qcq *qcq = container_of(napi, struct qcq, napi);
	struct lif *lif = qcq->q.lif;
	struct cq *cq = &qcq->cq;
	int napi_return;
	void *reg;
	u32 val;

	napi_return = ionic_napi(napi, budget, ionic_notifyq_cb, NULL, NULL);

	/* if we ran out of budget, there are more events
	 * to process and napi will reschedule us soon
	 */
	if (napi_return == budget)
		goto return_to_napi;

	/* After outstanding events are processed we can check on
	 * the link status and any outstanding interrupt credits.
	 *
	 * We wait until here to check on the link status in case
	 * there was a long list of link events from a flap episode.
	 */
	if (test_bit(LIF_LINK_CHECK_NEEDED, lif->state)) {
		struct deferred_work *work;

		work = kzalloc(sizeof(*work), GFP_ATOMIC);
		if (!work) {
			netdev_err(lif->netdev, "%s OOM\n", __func__);
		} else {
			work->type = DW_TYPE_LINK_STATUS;
			ionic_lif_deferred_enqueue(&lif->deferred, work);
		}
	}

	/* We check the interrupt credits to be sure we've returned
	 * enough, in case we missed a few events in an overflow.
	 */
	reg = intr_to_credits(cq->bound_intr->ctrl);
	val = ioread32(reg) & 0xFFFF;
	if (val) {
		netdev_info(lif->netdev, "returning %d nq credits\n", val);
		ionic_intr_return_credits(cq->bound_intr, val, true, true);
	}

return_to_napi:
	return napi_return;
}

#ifdef HAVE_VOID_NDO_GET_STATS64
static void ionic_get_stats64(struct net_device *netdev,
			      struct rtnl_link_stats64 *ns)
#else
static struct rtnl_link_stats64 *ionic_get_stats64(struct net_device *netdev,
						   struct rtnl_link_stats64 *ns)
#endif
{
	struct lif *lif = netdev_priv(netdev);
	struct ionic_lif_stats *ls = lif->lif_stats;

	if (!test_bit(LIF_UP, lif->state))
#ifdef HAVE_VOID_NDO_GET_STATS64
		return;
#else
		return ns;
#endif

	ns->rx_packets = ls->rx_ucast_packets +
			 ls->rx_mcast_packets +
			 ls->rx_bcast_packets;

	ns->tx_packets = ls->tx_ucast_packets +
			 ls->tx_mcast_packets +
			 ls->tx_bcast_packets;

	ns->rx_bytes = ls->rx_ucast_bytes +
		       ls->rx_mcast_bytes +
		       ls->rx_bcast_bytes;

	ns->tx_bytes = ls->tx_ucast_bytes +
		       ls->tx_mcast_bytes +
		       ls->tx_bcast_bytes;

	ns->rx_dropped = ls->rx_ucast_drop_packets +
			 ls->rx_mcast_drop_packets +
			 ls->rx_bcast_drop_packets;

	ns->tx_dropped = ls->tx_ucast_drop_packets +
			 ls->tx_mcast_drop_packets +
			 ls->tx_bcast_drop_packets;

	ns->multicast = ls->rx_mcast_packets;

	ns->rx_over_errors = ls->rx_queue_empty;

	ns->rx_missed_errors = ls->rx_dma_error +
			       ls->rx_queue_disabled +
			       ls->rx_desc_fetch_error +
			       ls->rx_desc_data_error;

	ns->tx_aborted_errors = ls->tx_dma_error +
				ls->tx_queue_disabled +
				ls->tx_desc_fetch_error +
				ls->tx_desc_data_error;

	ns->rx_errors = ns->rx_over_errors +
			ns->rx_missed_errors;

	ns->tx_errors = ns->tx_aborted_errors;

#ifndef HAVE_VOID_NDO_GET_STATS64
	return ns;
#endif
}

static int ionic_lif_addr_add(struct lif *lif, const u8 *addr)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rx_filter_add = {
			.opcode = CMD_OPCODE_RX_FILTER_ADD,
			.match = RX_FILTER_MATCH_MAC,
		},
	};
	struct rx_filter *f;
	int err;

	/* don't bother if we already have it */
	spin_lock_bh(&lif->rx_filters.lock);
	f = ionic_rx_filter_by_addr(lif, addr);
	spin_unlock_bh(&lif->rx_filters.lock);
	if (f)
		return 0;

	/* make sure we're not getting a slave's filter */
	/* TODO: use a global hash rather than search every slave */
	if (is_master_lif(lif)) {
		struct list_head *cur, *tmp;
		struct lif *slave_lif;

		list_for_each_safe(cur, tmp, &lif->ionic->lifs) {
			slave_lif = list_entry(cur, struct lif, list);
			spin_lock_bh(&slave_lif->rx_filters.lock);
			f = ionic_rx_filter_by_addr(slave_lif, addr);
			spin_unlock_bh(&slave_lif->rx_filters.lock);
			if (f)
				return 0;
		}
	}

	memcpy(ctx.cmd.rx_filter_add.mac.addr, addr, ETH_ALEN);
	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	netdev_info(lif->netdev, "rx_filter add ADDR %pM (id %d)\n", addr,
		    ctx.comp.rx_filter_add.filter_id);

	return ionic_rx_filter_save(lif, 0, RXQ_INDEX_ANY, 0, &ctx);
}

static int ionic_lif_addr_del(struct lif *lif, const u8 *addr)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rx_filter_del = {
			.opcode = CMD_OPCODE_RX_FILTER_DEL,
		},
	};
	struct rx_filter *f;
	int err;

	spin_lock_bh(&lif->rx_filters.lock);
	f = ionic_rx_filter_by_addr(lif, addr);
	if (!f) {
		spin_unlock_bh(&lif->rx_filters.lock);
		return -ENOENT;
	}

	ctx.cmd.rx_filter_del.filter_id = f->filter_id;
	ionic_rx_filter_free(lif, f);
	spin_unlock_bh(&lif->rx_filters.lock);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	netdev_info(lif->netdev, "rx_filter del ADDR %pM (id %d)\n", addr,
		    ctx.cmd.rx_filter_del.filter_id);

	return 0;
}

static int ionic_lif_addr(struct lif *lif, const u8 *addr, bool add)
{
	struct deferred_work *work;

	if (in_interrupt()) {
		work = kzalloc(sizeof(*work), GFP_ATOMIC);
		if (!work) {
			netdev_err(lif->netdev, "%s OOM\n", __func__);
			return -ENOMEM;
		}
		work->type = add ? DW_TYPE_RX_ADDR_ADD : DW_TYPE_RX_ADDR_DEL;
		memcpy(work->addr, addr, ETH_ALEN);
		netdev_info(lif->netdev, "deferred: rx_filter %s %pM\n",
			   add ? "add" : "del", addr);
		ionic_lif_deferred_enqueue(&lif->deferred, work);
	} else {
		if (add)
			return ionic_lif_addr_add(lif, addr);
		else
			return ionic_lif_addr_del(lif, addr);
	}

	return 0;
}

static int ionic_addr_add(struct net_device *netdev, const u8 *addr)
{
	return ionic_lif_addr(netdev_priv(netdev), addr, true);
}

static int ionic_addr_del(struct net_device *netdev, const u8 *addr)
{
	return ionic_lif_addr(netdev_priv(netdev), addr, false);
}

static void ionic_lif_rx_mode(struct lif *lif, unsigned int rx_mode)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rx_mode_set = {
			.opcode = CMD_OPCODE_RX_MODE_SET,
			.rx_mode = rx_mode,
		},
	};
	char buf[128];
	int err;
	int i;
#define REMAIN(__x) (sizeof(buf) - (__x))

	i = snprintf(buf, sizeof(buf), "rx_mode 0x%04x -> 0x%04x:",
		     lif->rx_mode, rx_mode);
	if (rx_mode & RX_MODE_F_UNICAST)
		i += snprintf(&buf[i], REMAIN(i), " RX_MODE_F_UNICAST");
	if (rx_mode & RX_MODE_F_MULTICAST)
		i += snprintf(&buf[i], REMAIN(i), " RX_MODE_F_MULTICAST");
	if (rx_mode & RX_MODE_F_BROADCAST)
		i += snprintf(&buf[i], REMAIN(i), " RX_MODE_F_BROADCAST");
	if (rx_mode & RX_MODE_F_PROMISC)
		i += snprintf(&buf[i], REMAIN(i), " RX_MODE_F_PROMISC");
	if (rx_mode & RX_MODE_F_ALLMULTI)
		i += snprintf(&buf[i], REMAIN(i), " RX_MODE_F_ALLMULTI");
	netdev_info(lif->netdev, "lif%d %s\n", lif->index, buf);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		netdev_warn(lif->netdev, "set rx_mode 0x%04x failed: %d\n",
			    rx_mode, err);
	else
		lif->rx_mode = rx_mode;
}

static void _ionic_lif_rx_mode(struct lif *lif, unsigned int rx_mode)
{
	struct deferred_work *work;

	if (in_interrupt()) {
		work = kzalloc(sizeof(*work), GFP_ATOMIC);
		if (!work) {
			netdev_err(lif->netdev, "%s OOM\n", __func__);
			return;
		}
		work->type = DW_TYPE_RX_MODE;
		work->rx_mode = rx_mode;
		netdev_info(lif->netdev, "deferred: rx_mode\n");
		ionic_lif_deferred_enqueue(&lif->deferred, work);
	} else {
		ionic_lif_rx_mode(lif, rx_mode);
	}
}

static void ionic_set_rx_mode(struct net_device *netdev)
{
	struct lif *master_lif = netdev_priv(netdev);
	union identity *ident = master_lif->ionic->ident;
	unsigned int rx_mode;

	rx_mode = RX_MODE_F_UNICAST;
	rx_mode |= (netdev->flags & IFF_MULTICAST) ? RX_MODE_F_MULTICAST : 0;
	rx_mode |= (netdev->flags & IFF_BROADCAST) ? RX_MODE_F_BROADCAST : 0;
	rx_mode |= (netdev->flags & IFF_PROMISC) ? RX_MODE_F_PROMISC : 0;
	rx_mode |= (netdev->flags & IFF_ALLMULTI) ? RX_MODE_F_ALLMULTI : 0;

	if (netdev_uc_count(netdev) + 1 > ident->dev.nucasts_per_lif)
		rx_mode |= RX_MODE_F_PROMISC;
	if (netdev_mc_count(netdev) > ident->dev.nmcasts_per_lif)
		rx_mode |= RX_MODE_F_ALLMULTI;

	if (master_lif->rx_mode != rx_mode)
		_ionic_lif_rx_mode(master_lif, rx_mode);

	__dev_uc_sync(netdev, ionic_addr_add, ionic_addr_del);
	__dev_mc_sync(netdev, ionic_addr_add, ionic_addr_del);
}

static int ionic_set_mac_address(struct net_device *netdev, void *sa)
{
	struct sockaddr *addr = sa;
	u8 *mac = (u8*)addr->sa_data;

	if (ether_addr_equal(netdev->dev_addr, mac))
		return 0;

	if (!is_valid_ether_addr(mac))
		return -EADDRNOTAVAIL;

	if (!is_zero_ether_addr(netdev->dev_addr)) {
		netdev_info(netdev, "deleting mac addr %pM\n",
			   netdev->dev_addr);
		ionic_addr_del(netdev, netdev->dev_addr);
	}

	memcpy(netdev->dev_addr, mac, netdev->addr_len);
	netdev_info(netdev, "updating mac addr %pM\n", mac);

	return ionic_addr_add(netdev, mac);
}

static int ionic_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct lif *lif = netdev_priv(netdev);
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.mtu_set = {
			.opcode = CMD_OPCODE_MTU_SET,
			.mtu = new_mtu,
		},
	};
	bool running;
	int err;

	running = netif_running(netdev);

	if (running)
		ionic_stop(netdev);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (!err)
		netdev->mtu = new_mtu;

	if (running)
		ionic_open(netdev);

	return err;
}

static void ionic_tx_timeout_work(struct work_struct *ws)
{
	struct lif *lif = container_of(ws, struct lif, tx_timeout_work);
	struct ionic_dev *idev = &lif->ionic->idev;

	ionic_dev_cmd_hang_notify(idev);
	ionic_dev_cmd_wait_check(idev, HZ * devcmd_timeout);

	// TODO implement reset and re-init queues and so on
	// TODO to get interface back on its feet
}

static void ionic_tx_timeout(struct net_device *netdev)
{
	struct lif *lif = netdev_priv(netdev);

	schedule_work(&lif->tx_timeout_work);
}

static int ionic_vlan_rx_add_vid(struct net_device *netdev, __be16 proto,
				 u16 vid)
{
	struct lif *lif = netdev_priv(netdev);
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rx_filter_add = {
			.opcode = CMD_OPCODE_RX_FILTER_ADD,
			.match = RX_FILTER_MATCH_VLAN,
			.vlan.vlan = vid,
		},
	};
	int err;

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	netdev_info(netdev, "rx_filter add VLAN %d (id %d)\n", vid,
		    ctx.comp.rx_filter_add.filter_id);

	return ionic_rx_filter_save(lif, 0, RXQ_INDEX_ANY, 0, &ctx);
}

static int ionic_vlan_rx_kill_vid(struct net_device *netdev, __be16 proto,
				  u16 vid)
{
	struct lif *lif = netdev_priv(netdev);
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rx_filter_del = {
			.opcode = CMD_OPCODE_RX_FILTER_DEL,
		},
	};
	struct rx_filter *f;
	int err;

	spin_lock_bh(&lif->rx_filters.lock);

	f = ionic_rx_filter_by_vlan(lif, vid);
	if (!f) {
		spin_unlock_bh(&lif->rx_filters.lock);
		return -ENOENT;
	}

	ctx.cmd.rx_filter_del.filter_id = f->filter_id;
	ionic_rx_filter_free(lif, f);
	spin_unlock_bh(&lif->rx_filters.lock);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	netdev_info(netdev, "rx_filter del VLAN %d (id %d)\n", vid,
		    ctx.cmd.rx_filter_del.filter_id);

	return 0;
}

static int ionic_slave_alloc(struct ionic *ionic)
{
	int index;

	/* slave index starts at 1, master_lif is 0 */
	index = find_first_zero_bit(ionic->lifbits, ionic->nslaves);
	if (index > ionic->nslaves)
		return -ENOSPC;

	set_bit(index, ionic->lifbits);

	return index;
}

void ionic_slave_free(struct ionic *ionic, int index)
{
	if (index >= ionic->nslaves)
		return;
	clear_bit(index, ionic->lifbits);
}

static void *ionic_dfwd_add_station(struct net_device *lower_dev,
				    struct net_device *upper_dev)
{
	struct lif *master_lif = netdev_priv(lower_dev);
	struct ionic *ionic = master_lif->ionic;
	struct lif *lif;
	int index;
	int err;

	if (!macvlan_supports_dest_filter(upper_dev))
		return NULL;

	/* slaves need 2 interrupts - adminq and txrx queue pair */
	if (ionic_intr_remaining(ionic) < 2) {
		netdev_info(lower_dev, "insufficient device interupts left for macvlan offload\n");
		return NULL;
	}

	/* master_lif index is 0, slave index starts at 1 */
	index = ionic_slave_alloc(ionic);
	if (index < 0) {
		netdev_info(lower_dev, "no space left for macvlan offload: %d, max=%d\n",
			    index, ionic->nslaves);
		return NULL;
	}
	netdev_info(lower_dev, "slave index %d for macvlan dev %s\n",
		    index, upper_dev->name);

	lif = ionic_lif_alloc(ionic, index);
	if (IS_ERR(lif))
		return NULL;

	lif->upper_dev = upper_dev;
	err = ionic_lif_init(lif);
	if (err) {
		netdev_err(lower_dev, "Cannot init slave lif %d for %s: %d\n",
			   index, upper_dev->name, err);
		goto err_out_free_slave;
	}

	ionic_lif_set_netdev_info(lif);
	_ionic_lif_rx_mode(lif, master_lif->rx_mode);
	ionic_lif_addr(lif, upper_dev->dev_addr, true);

	if (test_bit(LIF_UP, master_lif->state)) {
		err = ionic_lif_open(lif);
		if (err)
			goto err_out_deinit_slave;
	}

	/* bump up the netdev's in-use queue count if needed */
	if ((master_lif->nxqs + index) > lower_dev->real_num_tx_queues) {
		int max = lower_dev->real_num_tx_queues + 1;

		netif_set_real_num_tx_queues(lower_dev, max);
		netif_set_real_num_rx_queues(lower_dev, max);
	}

	/* WARNING - UGLY HACK */
	/* This is to work around a bug in versions of the macvlan
	 * driver prior to v4.18, where macvlan_open() doesn't call
	 * macvlan_hash_add() in the case of an offload macvlan.  This
	 * results in vlan->hlist not being initialized and eventually
	 * causing NULL pointer violations.
	 * The code below is hijacked from the macvlan driver, since
	 * it is defined static and unaccessible.
	 */
	{
#define MACVLAN_HASH_SIZE	(1<<MACVLAN_HASH_BITS)
#define MACVLAN_HASH_BITS	8
		struct macvlan_port {
			struct net_device	*dev;
			struct hlist_head	vlan_hash[MACVLAN_HASH_SIZE];
			struct list_head	vlans;
			struct sk_buff_head	bc_queue;
			struct work_struct	bc_work;
			u32			flags;
			int			count;
			struct hlist_head	vlan_source_hash[MACVLAN_HASH_SIZE];
			DECLARE_BITMAP(mc_filter, MACVLAN_MC_FILTER_SZ);
			unsigned char           perm_addr[ETH_ALEN];
		};

		struct macvlan_dev *vlan = netdev_priv(upper_dev);
		struct macvlan_port *port = (void *)vlan->port;
		const unsigned char *addr = vlan->dev->dev_addr;
		u32 idx;
		u64 value = get_unaligned((u64 *)addr);

		/* only want 6 bytes */
#ifdef __BIG_ENDIAN
		value >>= 16;
#else
		value <<= 16;
#endif
		idx = hash_64(value, MACVLAN_HASH_BITS);
		hlist_add_head_rcu(&vlan->hlist, &port->vlan_hash[idx]);
	}

	return lif;

err_out_deinit_slave:
	ionic_lif_deinit(lif);
err_out_free_slave:
	ionic_lif_free(lif);

	return NULL;
}

static void ionic_dfwd_del_station(struct net_device *lower_dev, void *priv)
{
	struct lif *master_lif = netdev_priv(lower_dev);
	struct lif *lif = priv;
	unsigned long index = lif->index;
	struct macvlan_dev *vlan = netdev_priv(lif->upper_dev);

	netdev_info(lower_dev, "%s: %s\n", __func__, lif->name);
	ionic_lif_stop(lif);
	ionic_lif_deinit(lif);

	list_del(&lif->list);
	lif->upper_dev = NULL;
	ionic_lif_free(lif);

	/* if this was the highest slot, we can decrement
	 * the number of queues in use
	 */
	if ((master_lif->nxqs + index) == lower_dev->real_num_tx_queues) {
		int max = lower_dev->real_num_tx_queues - 1;

		netif_set_real_num_tx_queues(lower_dev, max);
		netif_set_real_num_rx_queues(lower_dev, max);
	}

	/* WARNING - UGLY HACK part deux */
	/* This is to work around a bug in versions of the macvlan
	 * driver prior to v4.18, where macvlan_stop() doesn't call
	 * macvlan_hash_del() in the case of an offload macvlan.  This
	 * results in vlan->hlist not being cleaned up and eventually
	 * causing havoc.
	 * The code below is hijacked from the macvlan driver, since
	 * it is defined static and unaccessible.
	 */
//static void macvlan_hash_del(struct macvlan_dev *vlan, bool sync)
	{
		hlist_del_rcu(&vlan->hlist);
		synchronize_rcu();
	}
}

static const struct net_device_ops ionic_netdev_ops = {
	.ndo_open               = ionic_open,
	.ndo_stop               = ionic_stop,
	.ndo_start_xmit		= ionic_start_xmit,
	.ndo_select_queue	= ionic_select_queue,
	.ndo_get_stats64	= ionic_get_stats64,
	.ndo_set_rx_mode	= ionic_set_rx_mode,
	.ndo_set_mac_address	= ionic_set_mac_address,
	.ndo_validate_addr	= eth_validate_addr,
#ifdef HAVE_RHEL7_EXTENDED_MIN_MAX_MTU
        .extended.ndo_change_mtu = ionic_change_mtu,
#else
        .ndo_change_mtu         = ionic_change_mtu,
#endif
        .ndo_tx_timeout         = ionic_tx_timeout,
        .ndo_vlan_rx_add_vid    = ionic_vlan_rx_add_vid,
        .ndo_vlan_rx_kill_vid   = ionic_vlan_rx_kill_vid,

#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
	.extended.ndo_dfwd_add_station = ionic_dfwd_add_station,
	.extended.ndo_dfwd_del_station = ionic_dfwd_del_station,
#else
	.ndo_dfwd_add_station	= ionic_dfwd_add_station,
	.ndo_dfwd_del_station	= ionic_dfwd_del_station,
#endif

#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
/* RHEL7 requires this to be defined to enable extended ops.  RHEL7 uses the
 * function get_ndo_ext to retrieve offsets for extended fields from with the
 * net_device_ops struct and ndo_size is checked to determine whether or not
 * the offset is valid.
 */
	.ndo_size		= sizeof(const struct net_device_ops),
#endif
};

static irqreturn_t ionic_isr(int irq, void *data)
{
	struct napi_struct *napi = data;

	napi_schedule_irqoff(napi);

	return IRQ_HANDLED;
}

int ionic_intr_alloc(struct lif *lif, struct intr *intr)
{
	struct ionic *ionic = lif->ionic;
	struct ionic_dev *idev = &ionic->idev;
	int index;

	index = find_first_zero_bit(ionic->intrs, ionic->nintrs);
	if (index == ionic->nintrs) {
		netdev_warn(lif->netdev, "%s: no intr, index=%d nintrs=%d\n",
			    __func__, index, ionic->nintrs);
		return -ENOSPC;
	}

	set_bit(index, ionic->intrs);
	ionic_intr_init(idev, intr, index);

	return 0;
}

void ionic_intr_free(struct lif *lif, struct intr *intr)
{
	if (intr->index != INTR_INDEX_NOT_ASSIGNED)
		clear_bit(intr->index, lif->ionic->intrs);
}

static int ionic_intr_remaining(struct ionic *ionic)
{
	int intrs_remaining;
	unsigned long bit;

	intrs_remaining = ionic->nintrs;
	for_each_set_bit(bit, ionic->intrs, ionic->nintrs)
		intrs_remaining--;

	return intrs_remaining;
}

static void ionic_link_qcq_interrupts(struct qcq *src_qcq, struct qcq *n_qcq)
{
	n_qcq->intr.vector = src_qcq->intr.vector;
	n_qcq->intr.ctrl = src_qcq->intr.ctrl;
}

static int ionic_qcq_alloc(struct lif *lif, unsigned int index,
			   const char *name, unsigned int flags,
			   unsigned int num_descs, unsigned int desc_size,
			   unsigned int cq_desc_size,
			   unsigned int sg_desc_size,
			   unsigned int pid, struct qcq **qcq)
{
	struct ionic_dev *idev = &lif->ionic->idev;
	struct device *dev = lif->ionic->dev;
	struct qcq *new;
	unsigned int q_size = num_descs * desc_size;
	unsigned int cq_size = num_descs * cq_desc_size;
	unsigned int sg_size = num_descs * sg_desc_size;
	unsigned int total_size = ALIGN(q_size, PAGE_SIZE) +
				  ALIGN(cq_size, PAGE_SIZE) +
				  ALIGN(sg_size, PAGE_SIZE);
	void *q_base, *cq_base, *sg_base;
	dma_addr_t q_base_pa, cq_base_pa, sg_base_pa;
	int err;

	*qcq = NULL;

	total_size = ALIGN(q_size, PAGE_SIZE) + ALIGN(cq_size, PAGE_SIZE);
	if (flags & QCQ_F_SG)
		total_size += ALIGN(sg_size, PAGE_SIZE);

	new = devm_kzalloc(dev, sizeof(*new), GFP_KERNEL);
	if (!new) {
		err = -ENOMEM;
		goto err_out;
	}

	new->flags = flags;

	new->q.info = devm_kzalloc(dev, sizeof(*new->q.info) * num_descs,
				   GFP_KERNEL);
	if (!new->q.info) {
		err = -ENOMEM;
		goto err_out;
	}

	err = ionic_q_init(lif, idev, &new->q, index, name, num_descs,
			   desc_size, sg_desc_size, pid);
	if (err)
		goto err_out;

	if (flags & QCQ_F_INTR) {
		err = ionic_intr_alloc(lif, &new->intr);
		if (err) {
			netdev_warn(lif->netdev, "no intr for %s: %d\n",
				    name, err);
			goto err_out;
		}

		err = ionic_bus_get_irq(lif->ionic, new->intr.index);
		if (err < 0) {
			netdev_warn(lif->netdev, "no vector for %s: %d\n",
				    name, err);
			goto err_out_free_intr;
		}
		new->intr.vector = err;
		ionic_intr_mask_on_assertion(&new->intr);
	} else {
		new->intr.index = INTR_INDEX_NOT_ASSIGNED;
	}

	new->cq.info = devm_kzalloc(dev, sizeof(*new->cq.info) * num_descs,
				    GFP_KERNEL);
	if (!new->cq.info) {
		err = -ENOMEM;
		goto err_out_free_intr;
	}

	err = ionic_cq_init(lif, &new->cq, &new->intr, num_descs, cq_desc_size);
	if (err)
		goto err_out_free_intr;

	new->base = dma_alloc_coherent(dev, total_size, &new->base_pa,
				       GFP_KERNEL);
	if (!new->base) {
		err = -ENOMEM;
		goto err_out_free_intr;
	}

	new->total_size = total_size;

	q_base = new->base;
	q_base_pa = new->base_pa;

	cq_base = (void *)ALIGN((uintptr_t)q_base + q_size, PAGE_SIZE);
	cq_base_pa = ALIGN(q_base_pa + q_size, PAGE_SIZE);

	if (flags & QCQ_F_SG) {
		sg_base = (void *)ALIGN((uintptr_t)cq_base + cq_size,
					PAGE_SIZE);
		sg_base_pa = ALIGN(cq_base_pa + cq_size, PAGE_SIZE);
		ionic_q_sg_map(&new->q, sg_base, sg_base_pa);
	}

	ionic_q_map(&new->q, q_base, q_base_pa);
	ionic_cq_map(&new->cq, cq_base, cq_base_pa);
	ionic_cq_bind(&new->cq, &new->q);

	*qcq = new;

	return 0;

err_out_free_intr:
	ionic_intr_free(lif, &new->intr);
err_out:
	dev_err(dev, "qcq alloc of %s%d failed %d\n", name, index, err);
	return err;
}

static void ionic_qcq_free(struct lif *lif, struct qcq *qcq)
{
	struct device *dev = lif->ionic->dev;

	if (!qcq)
		return;

	dma_free_coherent(dev, qcq->total_size, qcq->base, qcq->base_pa);
	qcq->base = NULL;
	qcq->base_pa = 0;

	/* only the slave Tx and Rx qcqs will have master_slot set */
	if (qcq->master_slot) {
		int max = lif->ionic->master_lif->nxqs + lif->ionic->nslaves;

		if (qcq->master_slot >= max)
			dev_err(dev, "bad slot number %d\n", qcq->master_slot);
		else if (qcq->flags & QCQ_F_TX_STATS)
			lif->ionic->master_lif->txqcqs[qcq->master_slot] = NULL;
		else
			lif->ionic->master_lif->rxqcqs[qcq->master_slot] = NULL;
	}

	ionic_intr_free(lif, &qcq->intr);
	dma_free_coherent(dev, qcq->total_size, qcq->base, qcq->base_pa);
	devm_kfree(dev, qcq->cq.info);
	devm_kfree(dev, qcq->q.info);
	devm_kfree(dev, qcq);
}

static int ionic_link_to_master_qcq(struct qcq *qcq, struct qcq *master_qs[])
{
	struct lif *master_lif = qcq->q.lif->ionic->master_lif;
	unsigned int slot;

	slot = master_lif->nxqs + qcq->q.lif->index - 1;

	/* TODO: should never be true */
	if (master_qs[slot]) {
		netdev_err(master_lif->netdev,
			   "bad slot number %d\n", qcq->master_slot);
		return -ENOSPC;
	}

	master_qs[slot] = qcq;
	qcq->master_slot = slot;

	return 0;
}

static int ionic_txrx_alloc(struct lif *lif)
{
	struct device *dev = lif->ionic->dev;
	unsigned int flags;
	unsigned int i;
	unsigned int q_list_size;
	int err = 0;

	q_list_size = sizeof(*lif->txqcqs) * lif->nxqs;
	if (is_master_lif(lif))
		q_list_size += sizeof(*lif->txqcqs) * lif->ionic->nslaves;

	lif->txqcqs = devm_kzalloc(dev, q_list_size, GFP_KERNEL);
	if (!lif->txqcqs)
		return -ENOMEM;

	lif->rxqcqs = devm_kzalloc(dev, q_list_size, GFP_KERNEL);
	if (!lif->rxqcqs)
		goto err_out_free_txq_block;

	flags = QCQ_F_TX_STATS | QCQ_F_SG;
	for (i = 0; i < lif->nxqs; i++) {
		err = ionic_qcq_alloc(lif, i, "tx", flags, lif->ntxq_descs,
				      sizeof(struct txq_desc),
				      sizeof(struct txq_comp),
				      sizeof(struct txq_sg_desc),
				      lif->kern_pid, &lif->txqcqs[i]);
		if (err)
			goto err_out_free_txqcqs;

		if (!is_master_lif(lif)) {
			struct qcq **txqs = lif->ionic->master_lif->txqcqs;

			err = ionic_link_to_master_qcq(lif->txqcqs[i], txqs);
			if (err)
				goto err_out_free_txqcqs;
		}
	}

	flags = QCQ_F_RX_STATS | QCQ_F_INTR;
	for (i = 0; i < lif->nxqs; i++) {
		err = ionic_qcq_alloc(lif, i, "rx", flags, lif->nrxq_descs,
				      sizeof(struct rxq_desc),
				      sizeof(struct rxq_comp),
				      0, lif->kern_pid, &lif->rxqcqs[i]);
		if (err)
			goto err_out_free_rxqcqs;

		ionic_link_qcq_interrupts(lif->rxqcqs[i], lif->txqcqs[i]);

		if (!is_master_lif(lif)) {
			struct qcq **rxqs = lif->ionic->master_lif->rxqcqs;

			err = ionic_link_to_master_qcq(lif->rxqcqs[i], rxqs);
			if (err)
				goto err_out_free_rxqcqs;
		}
	}

	return 0;

err_out_free_rxqcqs:
	for (i = 0; i < lif->nxqs; i++)
		ionic_qcq_free(lif, lif->rxqcqs[i]);
err_out_free_txqcqs:
	for (i = 0; i < lif->nxqs; i++)
		ionic_qcq_free(lif, lif->txqcqs[i]);
	devm_kfree(dev, lif->rxqcqs);
	lif->rxqcqs = NULL;
err_out_free_txq_block:
	devm_kfree(dev, lif->txqcqs);
	lif->txqcqs = NULL;

	return err;
}

static int ionic_txrx_init(struct lif *lif)
{
	int err;

	err = ionic_lif_txqs_init(lif);
	if (err)
		return err;

	err = ionic_lif_rxqs_init(lif);
	if (err)
		goto err_out;

	if (is_master_lif(lif) && lif->netdev->features & NETIF_F_RXHASH)
		ionic_lif_rss_setup(lif);

	ionic_set_rx_mode(lif->netdev);

	return 0;

err_out:
	ionic_stop(lif->netdev);

	return err;
}

static int ionic_qcqs_alloc(struct lif *lif)
{
	unsigned int flags;
	int err;

	flags = QCQ_F_INTR;
	err = ionic_qcq_alloc(lif, 0, "admin", flags, 1 << 4,
			      sizeof(struct admin_cmd),
			      sizeof(struct admin_comp),
			      0, lif->kern_pid, &lif->adminqcq);
	if (err)
		return err;

	if (is_master_lif(lif) && lif->ionic->nnqs_per_lif) {
		flags = QCQ_F_INTR | QCQ_F_NOTIFYQ;
		err = ionic_qcq_alloc(lif, 0, "notifyq", flags, NOTIFYQ_LENGTH,
				      sizeof(struct notifyq_cmd),
				      sizeof(union notifyq_comp),
				      0, lif->kern_pid, &lif->notifyqcq);
		if (err)
			goto err_out_free_adminqcq;
	}

	return 0;

err_out_free_adminqcq:
	ionic_qcq_free(lif, lif->adminqcq);
	lif->adminqcq = NULL;

	return err;
}

static struct lif *ionic_lif_alloc(struct ionic *ionic, unsigned int index)
{
	struct device *dev = ionic->dev;
	struct lif *lif;
	int err, dbpage_num;

	if (index == 0) {
		struct net_device *netdev;
		int nqueues;
		/* master lif is built into the netdev */

		/* We create a netdev big enough to handle all the queues
		 * needed for our macvlan slave lifs, then set the real
		 * number of Tx and Rx queues used to just the queue set
		 * for lif0.  When we start building macvlans, we'll
		 * update the real_num_tx/rx_queues as needed.
		 */
		nqueues = ionic->ntxqs_per_lif + ionic->nslaves;
		dev_info(ionic->dev, "nxqs=%d nslaves=%d nqueues=%d\n",
			 ionic->ntxqs_per_lif, ionic->nslaves, nqueues);
		netdev = alloc_etherdev_mqs(sizeof(*lif), nqueues, nqueues);
		if (!netdev) {
			dev_err(dev, "Cannot allocate netdev, aborting\n");
			return ERR_PTR(-ENOMEM);
		}
		netif_set_real_num_tx_queues(netdev, ionic->ntxqs_per_lif);
		netif_set_real_num_rx_queues(netdev, ionic->nrxqs_per_lif);

		SET_NETDEV_DEV(netdev, dev);

		lif = netdev_priv(netdev);
		lif->netdev = netdev;
		ionic->master_lif = lif;

		netdev->netdev_ops = &ionic_netdev_ops;
		ionic_ethtool_set_ops(netdev);
		netdev->watchdog_timeo = 2 * HZ;

#ifdef HAVE_NETDEVICE_MIN_MAX_MTU
#ifdef HAVE_RHEL7_EXTENDED_MIN_MAX_MTU
		netdev->extended->min_mtu = IONIC_MIN_MTU;
		netdev->extended->max_mtu = IONIC_MAX_MTU;
#else
		netdev->min_mtu = IONIC_MIN_MTU;
		netdev->max_mtu = IONIC_MAX_MTU;
#endif /* HAVE_RHEL7_EXTENDED_MIN_MAX_MTU */
#endif /* HAVE_NETDEVICE_MIN_MAX_MTU */

		lif->neqs = ionic->neqs_per_lif;
		lif->nxqs = ionic->ntxqs_per_lif;
	} else {
		/* slave lifs */

		lif = kzalloc(sizeof(*lif), GFP_KERNEL);
		if (!lif) {
			dev_err(dev, "Cannot allocate slave lif %d\n", index);
			return ERR_PTR(-ENOMEM);
		}
		lif->netdev = ionic->master_lif->netdev;

		lif->neqs = 0;
		lif->nxqs = 1;
	}

	lif->ionic = ionic;
	lif->index = index;
	lif->ntxq_descs = IONIC_DEF_TXRX_DESC;
	lif->nrxq_descs = IONIC_DEF_TXRX_DESC;

	snprintf(lif->name, sizeof(lif->name), "lif%u", index);

	spin_lock_init(&lif->adminq_lock);

	spin_lock_init(&lif->deferred.lock);
	INIT_LIST_HEAD(&lif->deferred.list);
	INIT_WORK(&lif->deferred.work, ionic_lif_deferred_work);

	mutex_init(&lif->dbid_inuse_lock);
	lif->dbid_count = lif->ionic->ident->dev.ndbpgs_per_lif;
	if (!lif->dbid_count) {
		dev_err(dev, "No doorbell pages, aborting\n");
		err = -EINVAL;
		goto err_out_free_netdev;
	}

	lif->dbid_inuse = kzalloc(BITS_TO_LONGS(lif->dbid_count) * sizeof(long),
				  GFP_KERNEL);
	if (!lif->dbid_inuse) {
		dev_err(dev, "Failed alloc doorbell id bitmap, aborting\n");
		err = -ENOMEM;
		goto err_out_free_netdev;
	}

	/* first doorbell id reserved for kernel (dbid aka pid == zero) */
	set_bit(0, lif->dbid_inuse);
	lif->kern_pid = 0;

	dbpage_num = ionic_db_page_num(lif, 0);
	lif->kern_dbpage = ionic_bus_map_dbpage(ionic, dbpage_num);
	if (!lif->kern_dbpage) {
		dev_err(dev, "Cannot map dbpage, aborting\n");
		err = -ENOMEM;
		goto err_out_free_dbid;
	}

	if (is_master_lif(lif) && lif->ionic->nnqs_per_lif) {
		lif->notifyblock_sz = ALIGN(sizeof(*lif->notifyblock),
					    PAGE_SIZE);
		lif->notifyblock = devm_kzalloc(dev, lif->notifyblock_sz,
						GFP_KERNEL | GFP_DMA);
		if (!lif->notifyblock) {
			err = -ENOMEM;
			goto err_out_unmap_dbell;
		}
		lif->notifyblock_pa = dma_map_single(dev, lif->notifyblock,
						     lif->notifyblock_sz,
						     DMA_BIDIRECTIONAL);
		if (dma_mapping_error(dev, lif->notifyblock_pa)) {
			err = -EIO;
			goto err_out_free_notify;
		}
	}

	err = ionic_qcqs_alloc(lif);
	if (err)
		goto err_out_unmap_notify;

	list_add_tail(&lif->list, &ionic->lifs);

	return lif;

err_out_unmap_notify:
	if (lif->notifyblock_pa) {
		dma_unmap_single(dev, lif->notifyblock_pa, lif->notifyblock_sz,
				 DMA_BIDIRECTIONAL);
		lif->notifyblock_pa = 0;
	}
err_out_free_notify:
	if (lif->notifyblock) {
		devm_kfree(dev, lif->notifyblock);
		lif->notifyblock = NULL;
	}
err_out_unmap_dbell:
	ionic_bus_unmap_dbpage(ionic, lif->kern_dbpage);
	lif->kern_dbpage = NULL;
err_out_free_dbid:
	kfree(lif->dbid_inuse);
	lif->dbid_inuse = NULL;
err_out_free_netdev:
	if (is_master_lif(lif))
		free_netdev(lif->netdev);
	else
		kfree(lif);
	lif = NULL;

	return ERR_PTR(err);
}

int ionic_lifs_alloc(struct ionic *ionic)
{
	struct lif *lif;

	INIT_LIST_HEAD(&ionic->lifs);

	/* only build the first lif, others are for dynamic macvlan offload */
	set_bit(0, ionic->lifbits);
	lif = ionic_lif_alloc(ionic, 0);

	return PTR_ERR_OR_ZERO(lif);
}

static void ionic_lif_reset(struct lif *lif)
{
	struct ionic_dev *idev = &lif->ionic->idev;

	ionic_dev_cmd_lif_reset(idev, lif->index);
	ionic_dev_cmd_wait_check(idev, HZ * devcmd_timeout);
}

static void ionic_lif_free(struct lif *lif)
{
	struct device *dev = lif->ionic->dev;

	ionic_lif_reset(lif);
	cancel_work_sync(&lif->deferred.work);

	ionic_qcq_free(lif, lif->notifyqcq);
	lif->notifyqcq = NULL;
	ionic_qcq_free(lif, lif->adminqcq);
	lif->adminqcq = NULL;
	ionic_bus_unmap_dbpage(lif->ionic, lif->kern_dbpage);
	lif->kern_dbpage = NULL;
	kfree(lif->dbid_inuse);
	lif->dbid_inuse = NULL;

	if (lif->notifyblock) {
		dma_unmap_single(dev, lif->notifyblock_pa,
				 lif->notifyblock_sz,
				 DMA_BIDIRECTIONAL);
		devm_kfree(dev, lif->notifyblock);
		lif->notifyblock = NULL;
		lif->notifyblock_pa = 0;
	}

	ionic_debugfs_del_lif(lif);

	if (is_master_lif(lif)) {
		free_netdev(lif->netdev);
	} else {
		ionic_slave_free(lif->ionic, lif->index);
		kfree(lif);
	}
}

void ionic_lifs_free(struct ionic *ionic)
{
	struct list_head *cur, *tmp;
	struct lif *lif;

	list_for_each_safe(cur, tmp, &ionic->lifs) {
		lif = list_entry(cur, struct lif, list);
		list_del(&lif->list);

		ionic_lif_free(lif);
	}
}

static int ionic_lif_stats_start(struct lif *lif, unsigned int ver)
{
	struct net_device *netdev = lif->netdev;
	struct device *dev = lif->ionic->dev;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.lif_stats = {
			.opcode = CMD_OPCODE_LIF_STATS_START,
			.ver = ver,
		},
	};
	int err;

	lif->lif_stats = dma_alloc_coherent(dev, sizeof(*lif->lif_stats),
					    &lif->lif_stats_pa, GFP_KERNEL);

	if (!lif->lif_stats) {
		netdev_err(netdev, "%s OOM\n", __func__);
		return -ENOMEM;
	}

	ctx.cmd.lif_stats.addr = lif->lif_stats_pa;

	dev_dbg(dev, "lif_stats START ver %d addr 0x%llx\n", ver,
		lif->lif_stats_pa);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		goto err_out_free;

	return 0;

err_out_free:
	dma_free_coherent(dev, sizeof(*lif->lif_stats), lif->lif_stats,
			  lif->lif_stats_pa);
	lif->lif_stats = NULL;
	lif->lif_stats_pa = 0;

	return err;
}

static void ionic_lif_stats_stop(struct lif *lif)
{
	struct net_device *netdev = lif->netdev;
	struct device *dev = lif->ionic->dev;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.lif_stats = {
			.opcode = CMD_OPCODE_LIF_STATS_STOP,
		},
	};
	int err;

	dev_dbg(dev, "lif_stats STOP\n");

	if (!lif->lif_stats_pa)
		return;

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err) {
		netdev_err(netdev, "lif_stats cmd failed %d\n", err);
		return;
	}

	dma_free_coherent(dev, sizeof(*lif->lif_stats), lif->lif_stats,
			  lif->lif_stats_pa);
	lif->lif_stats = NULL;
	lif->lif_stats_pa = 0;
}

static int ionic_lif_rss_setup(struct lif *lif)
{
	struct net_device *netdev = lif->netdev;
	struct device *dev = lif->ionic->dev;
	size_t tbl_size = sizeof(*lif->rss_ind_tbl) * RSS_IND_TBL_SIZE;
	static const u8 toeplitz_symmetric_key[] = {
		0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
		0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
		0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
		0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
		0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
	};
	unsigned int i;
	int err;

	lif->rss_ind_tbl = dma_alloc_coherent(dev, tbl_size,
					      &lif->rss_ind_tbl_pa,
					      GFP_KERNEL);

	if (!lif->rss_ind_tbl) {
		netdev_err(netdev, "%s OOM\n", __func__);
		return -ENOMEM;
	}

	/* Fill indirection table with 'default' values */

	for (i = 0; i < RSS_IND_TBL_SIZE; i++)
		lif->rss_ind_tbl[i] = i % lif->nxqs;

	err = ionic_rss_ind_tbl_set(lif, NULL);
	if (err)
		goto err_out_free;

	err = ionic_rss_hash_key_set(lif, toeplitz_symmetric_key);
	if (err)
		goto err_out_free;

	return 0;

err_out_free:
	ionic_lif_rss_teardown(lif);
	return err;
}

static void ionic_lif_rss_teardown(struct lif *lif)
{
	struct device *dev = lif->ionic->dev;
	size_t tbl_size = sizeof(*lif->rss_ind_tbl) * RSS_IND_TBL_SIZE;
	dma_addr_t tbl_pa;

	if (!lif->rss_ind_tbl)
		return;

	/* clear the table from the NIC, then release the DMA space */
	tbl_pa = lif->rss_ind_tbl_pa;
	lif->rss_ind_tbl_pa = 0;
	ionic_rss_ind_tbl_set(lif, NULL);

	dma_free_coherent(dev, tbl_size, lif->rss_ind_tbl, tbl_pa);
	lif->rss_ind_tbl = NULL;
	lif->rss_ind_tbl_pa = 0;
}

static void ionic_lif_qcq_deinit(struct lif *lif, struct qcq *qcq)
{
	struct device *dev = lif->ionic->dev;

	if (!qcq)
		return;

	ionic_debugfs_del_qcq(qcq);

	if (!(qcq->flags & QCQ_F_INITED))
		return;

	if (qcq->intr.index != INTR_INDEX_NOT_ASSIGNED) {
		ionic_intr_mask(&qcq->intr, true);
		synchronize_irq(qcq->intr.vector);
		devm_free_irq(dev, qcq->intr.vector, &qcq->napi);
		netif_napi_del(&qcq->napi);
	}

	qcq->flags &= ~QCQ_F_INITED;
}

static void ionic_lif_deinit(struct lif *lif)
{
	if (!test_bit(LIF_INITED, lif->state))
		return;
	clear_bit(LIF_INITED, lif->state);

	ionic_lif_stats_stop(lif);
	ionic_rx_filters_deinit(lif);

	if (lif->notifyqcq) {
		napi_disable(&lif->notifyqcq->napi);
		ionic_lif_qcq_deinit(lif, lif->notifyqcq);
	}

	napi_disable(&lif->adminqcq->napi);
	ionic_lif_qcq_deinit(lif, lif->adminqcq);
}

void ionic_lifs_deinit(struct ionic *ionic)
{
	struct list_head *cur, *tmp;
	struct lif *lif;

	list_for_each_safe(cur, tmp, &ionic->lifs) {
		lif = list_entry(cur, struct lif, list);
		ionic_lif_deinit(lif);
	}
}

static int ionic_request_irq(struct lif *lif, struct qcq *qcq)
{
	struct device *dev = lif->ionic->dev;
	struct intr *intr = &qcq->intr;
	struct queue *q = &qcq->q;
	struct napi_struct *napi = &qcq->napi;
	const char *name;

	if (lif->registered)
		name = lif->netdev->name;
	else if (!is_master_lif(lif) && lif->upper_dev)
		name = lif->upper_dev->name;
	else
		name = dev_name(dev);

	snprintf(intr->name, sizeof(intr->name),
		 "%s-%s-%s", DRV_NAME, name, q->name);

	return devm_request_irq(dev, intr->vector, ionic_isr,
				0, intr->name, napi);
}

static int ionic_lif_adminq_init(struct lif *lif)
{
	struct ionic_dev *idev = &lif->ionic->idev;
	struct qcq *qcq = lif->adminqcq;
	struct queue *q = &qcq->q;
	struct napi_struct *napi = &qcq->napi;
	struct adminq_init_comp comp;
	int err;

	ionic_dev_cmd_adminq_init(idev, q, 0, lif->index, qcq->intr.index);
	err = ionic_dev_cmd_wait_check(idev, HZ * devcmd_timeout);
	if (err)
		return err;

	ionic_dev_cmd_comp(idev, &comp);
	q->qid = comp.qid;
	q->qtype = comp.qtype;
	q->db = ionic_db_map(lif, q);
	dev_dbg(&lif->netdev->dev, "lif=%d qtype=%d qid=%d db=0x%08llx",
		lif->index, q->qtype, q->qid, virt_to_phys(q->db));

	netif_napi_add(lif->netdev, napi, ionic_adminq_napi,
		       NAPI_POLL_WEIGHT);

	err = ionic_request_irq(lif, qcq);
	if (err) {
		netif_napi_del(napi);
		return err;
	}

	qcq->flags |= QCQ_F_INITED;

	/* Enabling interrupts on adminq from here on... */
	napi_enable(napi);
	ionic_intr_mask(&lif->adminqcq->intr, false);

	err = ionic_debugfs_add_qcq(lif, qcq);
	if (err)
		netdev_warn(lif->netdev, "debugfs add for adminq failed %d\n",
			    err);

	return 0;
}

int ionic_lif_notifyq_init(struct lif *lif)
{
	struct device *dev = lif->ionic->dev;
	struct qcq *qcq = lif->notifyqcq;
	struct napi_struct *napi = &qcq->napi;
	struct queue *q = &qcq->q;
	int err;

	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.notifyq_init.opcode = CMD_OPCODE_NOTIFYQ_INIT,
		.cmd.notifyq_init.index = q->index,
		.cmd.notifyq_init.pid = q->pid,
		.cmd.notifyq_init.intr_index = qcq->intr.index,
		.cmd.notifyq_init.lif_index = lif->index,
		.cmd.notifyq_init.ring_size = ilog2(q->num_descs),
		.cmd.notifyq_init.ring_base = q->base_pa,
		.cmd.notifyq_init.notify_size = ilog2(lif->notifyblock_sz),
		.cmd.notifyq_init.notify_base = lif->notifyblock_pa,
	};

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	q->qid = ctx.comp.notifyq_init.qid;
	q->qtype = ctx.comp.notifyq_init.qtype;
	q->db = NULL;

	/* preset the callback info */
	q->info[0].cb_arg = lif;

	netif_napi_add(lif->netdev, napi, ionic_notifyq_napi,
		       NAPI_POLL_WEIGHT);

	err = ionic_request_irq(lif, qcq);
	if (err) {
		netif_napi_del(napi);
		return err;
	}

	qcq->flags |= QCQ_F_INITED;

	/* Enabling interrupts on notifyq from here on... */
	napi_enable(napi);
	ionic_intr_mask(&lif->notifyqcq->intr, false);

	err = ionic_debugfs_add_qcq(lif, qcq);
	if (err)
		dev_warn(dev, "debugfs add for notifyq failed %d\n", err);

	return 0;
}

static int ionic_get_features(struct lif *lif)
{
	struct device *dev = lif->ionic->dev;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.features = {
			.opcode = CMD_OPCODE_FEATURES,
			.set = FEATURE_SET_ETH_HW_FEATURES,
			.wanted = ETH_HW_VLAN_TX_TAG
				| ETH_HW_VLAN_RX_STRIP
				| ETH_HW_VLAN_RX_FILTER
				| ETH_HW_RX_HASH
				| ETH_HW_TX_SG
				| ETH_HW_TX_CSUM
				| ETH_HW_RX_CSUM
				| ETH_HW_TSO
				| ETH_HW_TSO_IPV6
				| ETH_HW_TSO_ECN,
		},
	};
	int err;

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	lif->hw_features = ctx.cmd.features.wanted &
			   ctx.comp.features.supported;

	if (lif->hw_features & ETH_HW_VLAN_TX_TAG)
		dev_dbg(dev, "feature ETH_HW_VLAN_TX_TAG\n");
	if (lif->hw_features & ETH_HW_VLAN_RX_STRIP)
		dev_dbg(dev, "feature ETH_HW_VLAN_RX_STRIP\n");
	if (lif->hw_features & ETH_HW_VLAN_RX_FILTER)
		dev_dbg(dev, "feature ETH_HW_VLAN_RX_FILTER\n");
	if (lif->hw_features & ETH_HW_RX_HASH)
		dev_dbg(dev, "feature ETH_HW_RX_HASH\n");
	if (lif->hw_features & ETH_HW_TX_SG)
		dev_dbg(dev, "feature ETH_HW_TX_SG\n");
	if (lif->hw_features & ETH_HW_TX_CSUM)
		dev_dbg(dev, "feature ETH_HW_TX_CSUM\n");
	if (lif->hw_features & ETH_HW_RX_CSUM)
		dev_dbg(dev, "feature ETH_HW_RX_CSUM\n");
	if (lif->hw_features & ETH_HW_TSO)
		dev_dbg(dev, "feature ETH_HW_TSO\n");
	if (lif->hw_features & ETH_HW_TSO_IPV6)
		dev_dbg(dev, "feature ETH_HW_TSO_IPV6\n");
	if (lif->hw_features & ETH_HW_TSO_ECN)
		dev_dbg(dev, "feature ETH_HW_TSO_ECN\n");
	if (lif->hw_features & ETH_HW_TSO_GRE)
		dev_dbg(dev, "feature ETH_HW_TSO_GRE\n");
	if (lif->hw_features & ETH_HW_TSO_GRE_CSUM)
		dev_dbg(dev, "feature ETH_HW_TSO_GRE_CSUM\n");
	if (lif->hw_features & ETH_HW_TSO_IPXIP4)
		dev_dbg(dev, "feature ETH_HW_TSO_IPXIP4\n");
	if (lif->hw_features & ETH_HW_TSO_IPXIP6)
		dev_dbg(dev, "feature ETH_HW_TSO_IPXIP6\n");
	if (lif->hw_features & ETH_HW_TSO_UDP)
		dev_dbg(dev, "feature ETH_HW_TSO_UDP\n");
	if (lif->hw_features & ETH_HW_TSO_UDP_CSUM)
		dev_dbg(dev, "feature ETH_HW_TSO_UDP_CSUM\n");

	return 0;
}

static int ionic_set_features(struct lif *lif)
{
	struct net_device *netdev = lif->netdev;
	int err;

	err = ionic_get_features(lif);
	if (err)
		return err;

	if (!is_master_lif(lif))
		return 0;

	netdev->features |= NETIF_F_HIGHDMA;

	if (lif->hw_features & ETH_HW_VLAN_TX_TAG)
		netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX;
	if (lif->hw_features & ETH_HW_VLAN_RX_STRIP)
		netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX;
	if (lif->hw_features & ETH_HW_VLAN_RX_FILTER)
		netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_FILTER;
	if (lif->hw_features & ETH_HW_RX_HASH)
		netdev->hw_features |= NETIF_F_RXHASH;
	if (lif->hw_features & ETH_HW_TX_SG)
		netdev->hw_features |= NETIF_F_SG;

	if (lif->hw_features & ETH_HW_TX_CSUM)
		netdev->hw_enc_features |= NETIF_F_HW_CSUM;
	if (lif->hw_features & ETH_HW_RX_CSUM)
		netdev->hw_enc_features |= NETIF_F_RXCSUM;
	if (lif->hw_features & ETH_HW_TSO)
		netdev->hw_enc_features |= NETIF_F_TSO;
	if (lif->hw_features & ETH_HW_TSO_IPV6)
		netdev->hw_enc_features |= NETIF_F_TSO6;
	if (lif->hw_features & ETH_HW_TSO_ECN)
		netdev->hw_enc_features |= NETIF_F_TSO_ECN;
	if (lif->hw_features & ETH_HW_TSO_GRE)
		netdev->hw_enc_features |= NETIF_F_GSO_GRE;
	if (lif->hw_features & ETH_HW_TSO_GRE_CSUM)
		netdev->hw_enc_features |= NETIF_F_GSO_GRE_CSUM;
#ifdef NETIF_F_GSO_IPXIP4
	if (lif->hw_features & ETH_HW_TSO_IPXIP4)
		netdev->hw_enc_features |= NETIF_F_GSO_IPXIP4;
#endif
#ifdef NETIF_F_GSO_IPXIP6
	if (lif->hw_features & ETH_HW_TSO_IPXIP6)
		netdev->hw_enc_features |= NETIF_F_GSO_IPXIP6;
#endif
	if (lif->hw_features & ETH_HW_TSO_UDP)
		netdev->hw_enc_features |= NETIF_F_GSO_UDP_TUNNEL;
	if (lif->hw_features & ETH_HW_TSO_UDP_CSUM)
		netdev->hw_enc_features |= NETIF_F_GSO_UDP_TUNNEL_CSUM;

	if (lif->ionic->nslaves)
		netdev->hw_features |= NETIF_F_HW_L2FW_DOFFLOAD;

	netdev->hw_features |= netdev->hw_enc_features;
	netdev->features |= netdev->hw_features;
	netdev->vlan_features |= netdev->features;

	netdev->priv_flags |= IFF_UNICAST_FLT;

	return 0;
}

static int ionic_lif_txq_init(struct lif *lif, struct qcq *qcq)
{
	struct device *dev = lif->ionic->dev;
	struct queue *q = &qcq->q;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.txq_init = {
			.opcode = CMD_OPCODE_TXQ_INIT,
			.I = false,
			.E = false,
			.pid = q->pid,
			.intr_index = lif->rxqcqs[q->index]->intr.index,
			.type = TXQ_TYPE_ETHERNET,
			.index = q->index,
			.cos = 0,
			.ring_base = q->base_pa,
			.ring_size = ilog2(q->num_descs),
		},
	};
	int err;


	dev_dbg(dev, "txq_init.pid %d\n", ctx.cmd.txq_init.pid);
	dev_dbg(dev, "txq_init.index %d\n", ctx.cmd.txq_init.index);
	dev_dbg(dev, "txq_init.ring_base 0x%llx\n", ctx.cmd.txq_init.ring_base);
	dev_dbg(dev, "txq_init.ring_size %d\n", ctx.cmd.txq_init.ring_size);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	q->qid = ctx.comp.txq_init.qid;
	q->qtype = ctx.comp.txq_init.qtype;
	q->db = ionic_db_map(lif, q);
	qcq->flags |= QCQ_F_INITED;

	dev_dbg(dev, "txq->qid %d\n", q->qid);
	dev_dbg(dev, "txq->qtype %d\n", q->qtype);
	dev_dbg(dev, "txq->db %p\n", q->db);

	err = ionic_debugfs_add_qcq(lif, qcq);
	if (err)
		netdev_warn(lif->netdev, "debugfs add for txq %d failed %d\n",
			    q->qid, err);

	return 0;
}

static int ionic_lif_txqs_init(struct lif *lif)
{
	unsigned int i;
	int err;

	for (i = 0; i < lif->nxqs; i++) {
		err = ionic_lif_txq_init(lif, lif->txqcqs[i]);
		if (err)
			goto err_out;
	}

	return 0;

err_out:
	for (; i > 0; i--)
		ionic_lif_qcq_deinit(lif, lif->txqcqs[i-1]);

	return err;
}

static int ionic_lif_rxq_init(struct lif *lif, struct qcq *qcq)
{
	struct device *dev = lif->ionic->dev;
	struct queue *q = &qcq->q;
	struct cq *cq = &qcq->cq;
	struct napi_struct *napi = &qcq->napi;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rxq_init = {
			.opcode = CMD_OPCODE_RXQ_INIT,
			.I = false,
			.E = false,
			.pid = q->pid,
			.intr_index = cq->bound_intr->index,
			.type = RXQ_TYPE_ETHERNET,
			.index = q->index,
			.ring_base = q->base_pa,
			.ring_size = ilog2(q->num_descs),
		},
	};
	int err;

	dev_dbg(dev, "rxq_init.pid %d\n", ctx.cmd.rxq_init.pid);
	dev_dbg(dev, "rxq_init.index %d\n", ctx.cmd.rxq_init.index);
	dev_dbg(dev, "rxq_init.ring_base 0x%llx\n", ctx.cmd.rxq_init.ring_base);
	dev_dbg(dev, "rxq_init.ring_size %d\n", ctx.cmd.rxq_init.ring_size);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	q->qid = ctx.comp.rxq_init.qid;
	q->qtype = ctx.comp.rxq_init.qtype;
	q->db = ionic_db_map(lif, q);

	netif_napi_add(lif->netdev, napi, ionic_rx_napi,
		       NAPI_POLL_WEIGHT);

	err = ionic_request_irq(lif, qcq);
	if (err) {
		netif_napi_del(napi);
		return err;
	}

	qcq->flags |= QCQ_F_INITED;

	dev_dbg(dev, "rxq->qid %d\n", q->qid);
	dev_dbg(dev, "rxq->qtype %d\n", q->qtype);
	dev_dbg(dev, "rxq->db %p\n", q->db);

	err = ionic_debugfs_add_qcq(lif, qcq);
	if (err)
		netdev_warn(lif->netdev, "debugfs add for rxq %d failed %d\n",
			    q->qid, err);

	return 0;
}

static int ionic_lif_rxqs_init(struct lif *lif)
{
	unsigned int i;
	int err;

	for (i = 0; i < lif->nxqs; i++) {
		err = ionic_lif_rxq_init(lif, lif->rxqcqs[i]);
		if (err)
			goto err_out;
	}

	return 0;

err_out:
	for (; i > 0; i--)
		ionic_lif_qcq_deinit(lif, lif->rxqcqs[i-1]);

	return err;
}

static int ionic_station_set(struct lif *lif)
{
	struct net_device *netdev = lif->netdev;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.station_mac_addr_get = {
			.opcode = CMD_OPCODE_STATION_MAC_ADDR_GET,
		},
	};
	int err;

	if (!is_master_lif(lif))
		return 0;

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	if (!is_zero_ether_addr(netdev->dev_addr)) {
		netdev_info(lif->netdev, "deleting station MAC addr %pM\n",
			   netdev->dev_addr);
		ionic_lif_addr(lif, netdev->dev_addr, false);
	}
	memcpy(netdev->dev_addr, ctx.comp.station_mac_addr_get.addr,
	       netdev->addr_len);
	netdev_info(lif->netdev, "adding station MAC addr %pM\n",
		   netdev->dev_addr);
	ionic_lif_addr(lif, netdev->dev_addr, true);

	return 0;
}

static int ionic_lif_init(struct lif *lif)
{
	struct ionic_dev *idev = &lif->ionic->idev;
	int err;

	err = ionic_debugfs_add_lif(lif);
	if (err)
		return err;

	ionic_dev_cmd_lif_init(idev, lif->index);
	err = ionic_dev_cmd_wait_check(idev, HZ * devcmd_timeout);
	if (err)
		return err;

	err = ionic_lif_adminq_init(lif);
	if (err)
		goto err_out_adminq_deinit;

	if (is_master_lif(lif) && lif->ionic->nnqs_per_lif) {
		err = ionic_lif_notifyq_init(lif);
		if (err)
			goto err_out_notifyq_deinit;
	}

	err = ionic_set_features(lif);
	if (err)
		goto err_out_notifyq_deinit;

	err = ionic_rx_filters_init(lif);
	if (err)
		goto err_out_notifyq_deinit;

	err = ionic_station_set(lif);
	if (err)
		goto err_out_notifyq_deinit;

	err = ionic_lif_stats_start(lif, STATS_DUMP_VERSION_1);
	if (err)
		goto err_out_notifyq_deinit;

	lif->rx_copybreak = rx_copybreak;

	lif->api_private = NULL;
	set_bit(LIF_INITED, lif->state);

	INIT_WORK(&lif->tx_timeout_work, ionic_tx_timeout_work);
	return 0;

err_out_notifyq_deinit:
	ionic_lif_qcq_deinit(lif, lif->notifyqcq);
err_out_adminq_deinit:
	ionic_lif_qcq_deinit(lif, lif->adminqcq);
	ionic_lif_reset(lif);

	return err;
}

int ionic_lifs_init(struct ionic *ionic)
{
	struct list_head *cur, *tmp;
	struct lif *lif;
	int err;

	list_for_each_safe(cur, tmp, &ionic->lifs) {
		lif = list_entry(cur, struct lif, list);
		err = ionic_lif_init(lif);
		if (err)
			return err;
	}

	return 0;
}

int ionic_lif_register(struct lif *lif)
{
	struct device *dev = lif->ionic->dev;
	int err;

	err = register_netdev(lif->netdev);
	if (err) {
		dev_err(dev, "Cannot register net device, aborting\n");
		return err;
	}

	ionic_link_status_check(lif);
	lif->registered = true;

	return 0;
}

static void ionic_lif_notify_work(struct work_struct *ws)
{
}

#ifdef NETDEV_CHANGEUPPER
static void ionic_lif_changeupper(struct ionic *ionic, struct lif *lif,
				  struct netdev_notifier_changeupper_info *info)
{
	struct netdev_lag_upper_info *upper_info;

	dev_dbg(ionic->dev, "lif %d is lag port %d\n",
		lif->index, netif_is_lag_port(lif->netdev));
	dev_dbg(ionic->dev, "lif %d upper %s is lag master %d\n",
		lif->index, info->upper_dev->name,
		netif_is_lag_master(info->upper_dev));
	dev_dbg(ionic->dev, "lif %d has upper info %d\n",
		lif->index, !!info->upper_info);

	if (!netif_is_lag_port(lif->netdev) ||
	    !netif_is_lag_master(info->upper_dev) ||
	    !info->upper_info)
		return;

	upper_info = info->upper_info;

	dev_dbg(ionic->dev, "upper tx type %d\n",
		upper_info->tx_type);
}
#endif /* NETDEV_CHANGEUPPER */

#ifdef NETDEV_CHANGELOWERSTATE
static void ionic_lif_changelowerstate(struct ionic *ionic, struct lif *lif,
			    struct netdev_notifier_changelowerstate_info *info)
{
	struct netdev_lag_lower_state_info *lower_info;

	dev_dbg(ionic->dev, "lif %d is lag port %d\n",
		lif->index, netif_is_lag_port(lif->netdev));
	dev_dbg(ionic->dev, "lif %d has lower info %d\n",
		lif->index, !!info->lower_state_info);

	if (!netif_is_lag_port(lif->netdev) ||
	    !info->lower_state_info)
		return;

	lower_info = info->lower_state_info;

	dev_dbg(ionic->dev, "link up %d enable %d\n",
		lower_info->link_up, lower_info->tx_enabled);
}


static void ionic_lif_set_netdev_info(struct lif *lif)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.netdev_info = {
			.opcode = CMD_OPCODE_SET_NETDEV_INFO,
		},
	};

	if (is_master_lif(lif))
		strlcpy(ctx.cmd.netdev_info.nd_name, lif->netdev->name,
			sizeof(ctx.cmd.netdev_info.nd_name));
	else
		strlcpy(ctx.cmd.netdev_info.nd_name, lif->upper_dev->name,
			sizeof(ctx.cmd.netdev_info.nd_name));
	strlcpy(ctx.cmd.netdev_info.dev_name, ionic_bus_info(lif->ionic),
		sizeof(ctx.cmd.netdev_info.dev_name));

	dev_info(lif->ionic->dev, "NETDEV_CHANGENAME %s %s %s\n",
		lif->name, ctx.cmd.netdev_info.nd_name,
		ctx.cmd.netdev_info.dev_name);

	ionic_adminq_post_wait(lif, &ctx);
}
#endif /* NETDEV_CHANGELOWERSTATE */

struct lif *ionic_netdev_lif(struct net_device *netdev)
{
	if (!netdev || netdev->netdev_ops->ndo_start_xmit != ionic_start_xmit)
		return NULL;

	return netdev_priv(netdev);
}

static int ionic_lif_notify(struct notifier_block *nb,
			    unsigned long event, void *info)
{
	struct ionic *ionic = container_of(nb, struct ionic, nb);
	struct net_device *ndev = netdev_notifier_info_to_dev(info);
	struct lif *lif = ionic_netdev_lif(ndev);

	if (!lif || lif->ionic != ionic)
		return NOTIFY_DONE;

	switch (event) {
#ifdef NETDEV_CHANGEUPPER
	case NETDEV_CHANGEUPPER:
		ionic_lif_changeupper(ionic, lif, info);
		break;
#endif
#ifdef NETDEV_CHANGELOWERSTATE
	case NETDEV_CHANGELOWERSTATE:
		ionic_lif_changelowerstate(ionic, lif, info);
		break;
	case NETDEV_CHANGENAME:
		ionic_lif_set_netdev_info(lif);
		break;
#endif
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_DONE;
}

int ionic_lifs_register(struct ionic *ionic)
{
	int err;

	INIT_WORK(&ionic->nb_work, ionic_lif_notify_work);

	ionic->nb.notifier_call = ionic_lif_notify;

	err = register_netdevice_notifier(&ionic->nb);
	if (err)
		ionic->nb.notifier_call = NULL;

	return ionic_lif_register(ionic->master_lif);
}

void ionic_lifs_unregister(struct ionic *ionic)
{
	struct list_head *cur, *tmp;
	struct lif *lif;

	if (ionic->nb.notifier_call) {
		unregister_netdevice_notifier(&ionic->nb);
		cancel_work_sync(&ionic->nb_work);
		ionic->nb.notifier_call = NULL;
	}

	list_for_each_safe(cur, tmp, &ionic->lifs) {
		lif = list_entry(cur, struct lif, list);
		if (lif->registered) {
			unregister_netdev(lif->netdev);
			cancel_work_sync(&lif->tx_timeout_work);
			lif->registered = false;
		}
	}
}

int ionic_lifs_size(struct ionic *ionic)
{
	union identity *ident = ionic->ident;
	unsigned int nlifs = ident->dev.nlifs;
	unsigned int neqs_per_lif = ident->dev.rdma_eq_qtype.qid_count;
	unsigned int nnqs_per_lif = ident->dev.notify_qtype.qid_count;
	unsigned int ntxqs_per_lif = ident->dev.tx_qtype.qid_count;
	unsigned int nrxqs_per_lif = ident->dev.rx_qtype.qid_count;
	unsigned int nintrs, dev_nintrs = ident->dev.nintrs;
	unsigned int min_intrs;
	unsigned int nslaves;
	unsigned int nxqs, neqs;
	int err;

	nxqs = min(ntxqs_per_lif, nrxqs_per_lif);
	nxqs = min(nxqs, num_online_cpus());
	neqs = min(neqs_per_lif, num_online_cpus());

	nslaves = nlifs - 1;

try_again:
	/* interrupt usage:
	 *    1 for master lif adminq
	 *    1 for master lif notifyq
	 *    1 for each CPU for master lif TxRx queue pairs
	 *    2 for each slave lif: 1 adminq, 1 TxRx queuepair
	 *    whatever's left is for RDMA queues
	 */
	nintrs = 1 + 1 + nxqs + (nslaves * 2) + neqs;
	min_intrs = 3;  /* adminq + notifyq + 1 TxRx queue pair */

	if (nintrs > dev_nintrs)
		goto try_fewer;

	err = ionic_bus_alloc_irq_vectors(ionic, nintrs);
	if (err < 0 && err != -ENOSPC) {
		dev_err(ionic->dev, "Can't get intrs from OS: %d\n", err);
		return err;
	}
	if (err == -ENOSPC)
		goto try_fewer;
	if (err < min_intrs) {
		dev_err(ionic->dev, "Can't get minimum %d intrs from OS: %d\n",
			min_intrs, err);
		return -ENOSPC;
	}

	if (err != nintrs) {
		ionic_bus_free_irq_vectors(ionic);
		goto try_fewer;
	}

	ionic->nnqs_per_lif = nnqs_per_lif;
	ionic->neqs_per_lif = neqs;
	ionic->ntxqs_per_lif = nxqs;
	ionic->nrxqs_per_lif = nxqs;
	ionic->nintrs = nintrs;
	ionic->nslaves = nslaves;

	return ionic_debugfs_add_sizes(ionic);

try_fewer:
	if (nnqs_per_lif > 1) {
		--nnqs_per_lif;
		goto try_again;
	}
	if (neqs > 1) {
		--neqs;
		goto try_again;
	}
	if (nslaves) {
		--nslaves;
		goto try_again;
	}
	if (nxqs > 1) {
		--nxqs;
		goto try_again;
	}
	return -ENOSPC;
}
