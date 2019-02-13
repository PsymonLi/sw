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

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>

#include "ionic.h"
#include "ionic_bus.h"
#include "ionic_api.h"
#include "ionic_lif.h"
#include "ionic_txrx.h"

#include "opt_rss.h"

#ifdef	RSS
#include <net/rss_config.h>
#include <netinet/in_rss.h>
#endif

static void ionic_rx_fill(struct rxque *rxq);
static void ionic_rx_empty(struct rxque *rxq);
static void ionic_rx_refill(struct rxque *rxq);

static int ionic_addr_add(struct net_device *netdev, const u8 *addr);
static int ionic_addr_del(struct net_device *netdev, const u8 *addr);

struct lif_addr_work {
	struct work_struct work;
	struct lif *lif;
	u8 addr[ETH_ALEN];
	bool add;
};

struct rx_mode_work {
	struct work_struct work;
	struct lif *lif;
	unsigned int rx_mode;
};

struct lif_vlan_work {
	struct work_struct work;
	struct lif *lif;
	uint16_t vid;
	bool add;
};

static int ionic_q_enable_disable(struct lif* lif, unsigned int qid, unsigned int qtype,
	bool enable)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.q_enable = {
			.opcode = enable ? CMD_OPCODE_Q_ENABLE : CMD_OPCODE_Q_DISABLE,
			.qid = qid,
			.qtype = qtype,
		},
	};
	int err;

	IONIC_NETDEV_INFO(lif->netdev, "%s qid %d qtype:%d\n", enable ? "Enable" : "Disable",
		ctx.cmd.q_enable.qid, ctx.cmd.q_enable.qtype);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err) {
		IONIC_NETDEV_ERROR(lif->netdev, "Q enable failed for qid %d qtype:%d\n",
			ctx.cmd.q_enable.qid, ctx.cmd.q_enable.qtype);
		return err;
	}

#ifndef __FreeBSD__
	synchronize_irq(qcq->intr.vector);
#endif
	return err;
}

static void ionic_rxq_enable(struct rxque *rxq)
{
	int err;

	err = ionic_q_enable_disable(rxq->lif, rxq->qid, rxq->qtype, true /* enable */);
	if (err)
		IONIC_QUE_WARN(rxq, "failed to enabled, err: %d\n", err);
}

static void ionic_rxq_disable(struct rxque *rxq)
{
	int err;

	err = ionic_q_enable_disable(rxq->lif, rxq->qid, rxq->qtype, false /* disable */);
	if (err)
		IONIC_QUE_WARN(rxq, "failed to dsable, err: %d\n", err);
}

static void ionic_txq_enable(struct txque *txq)
{
	int err;

	err = ionic_q_enable_disable(txq->lif, txq->qid, txq->qtype, true /* enable */);
	if (err)
		IONIC_QUE_WARN(txq, "failed to enabled, err: %d\n", err);
}

static void ionic_txq_disable(struct txque *txq)
{
	int err;

	err = ionic_q_enable_disable(txq->lif, txq->qid, txq->qtype, false /* disable */);
	if (err)
		IONIC_QUE_WARN(txq, "failed to dsable, err: %d\n", err);
}

/* Calculate mbuf pool size based on MTU. */
static void ionic_calc_rx_size(struct lif *lif)
{

	if (lif->max_frame_size > MJUM9BYTES)
		lif->rx_mbuf_size  = MJUM16BYTES;
	else if (lif->max_frame_size > MJUMPAGESIZE)
		lif->rx_mbuf_size  = MJUM9BYTES;
	else if (lif->max_frame_size > MCLBYTES)
		lif->rx_mbuf_size  = MJUMPAGESIZE;
	else
		lif->rx_mbuf_size  = MCLBYTES;
}

/*
 * Read the notifyQ to get the latest link event.
 */
static void ionic_read_notify_block(struct lif* lif)
{
	struct notify_block *nb = lif->notifyblock;

	if (nb == NULL || lif == NULL)
		return;

	if (nb->eid < lif->last_eid)
		return;

	lif->last_eid = nb->eid;
	lif->link_speed = nb->link_speed;
	lif->link_up = nb->link_status ? true : false;
	lif->link_error_bits = nb->link_error_bits;
	lif->phy_type = nb->phy_type;
	lif->autoneg_status = nb->autoneg_status;

}

static void ionic_open(struct lif *lif)
{
	struct ifnet  *ifp = lif->netdev;
	struct rxque *rxq;
	struct txque *txq;
	unsigned int i;

	KASSERT(lif, ("lif is NULL"));

	/* already running? */
	if (ifp->if_drv_flags & IFF_DRV_RUNNING)
		return;

	IONIC_NETDEV_INFO(lif->netdev, "starting interface\n");

	ionic_calc_rx_size(lif);

	ionic_read_notify_block(lif);
	for (i = 0; i < lif->nrxqs; i++) {
		rxq = lif->rxqs[i];
		IONIC_RX_LOCK(rxq);
		ionic_rx_fill(rxq);
		IONIC_RX_UNLOCK(rxq);
		ionic_rxq_enable(rxq);
		ionic_intr_mask(&rxq->intr, false);
	}

	for (i = 0; i < lif->ntxqs; i++) {
		txq = lif->txqs[i];
		ionic_txq_enable(txq);
#ifdef IONIC_SEPERATE_TX_INTR
		ionic_intr_mask(&txq->intr, false);
#endif
	}

	ifp->if_drv_flags |= IFF_DRV_RUNNING;
	if_link_state_change(ifp, LINK_STATE_UP);
}

int ionic_stop(struct net_device *netdev)
{
	struct lif *lif = netdev_priv(netdev);
	struct rxque* rxq;
	struct txque* txq;
	unsigned int i;

	KASSERT(lif, ("lif is NULL"));
	KASSERT(IONIC_CORE_LOCK_OWNED(lif), ("%s is not locked", lif->name));

	/* already stopped? */
	if (!(netdev->if_drv_flags & IFF_DRV_RUNNING))
		return 0;

	IONIC_NETDEV_INFO(netdev, "stopping interface\n");

	lif->netdev->if_drv_flags &= ~IFF_DRV_RUNNING;
	if_link_state_change(lif->netdev, LINK_STATE_DOWN);

	for (i = 0; i < lif->nrxqs; i++) {
		rxq = lif->rxqs[i];
		ionic_rxq_disable(rxq);
		ionic_intr_mask(&rxq->intr, true);
	}

	for (i = 0; i < lif->ntxqs; i++) {
		txq = lif->txqs[i];
		ionic_txq_disable(txq);
#ifdef IONIC_SEPERATE_TX_INTR
		ionic_intr_mask(&txq->intr, true);
#endif
	}

	for (i = 0; i < lif->nrxqs; i++) {
		IONIC_RX_LOCK(rxq);
		ionic_rx_clean(rxq, rxq->num_descs);
		IONIC_RX_UNLOCK(rxq);
	}

	for (i = 0; i < lif->ntxqs; i++) {
		IONIC_TX_LOCK(txq);
		ionic_tx_clean(txq, txq->num_descs);
		IONIC_TX_UNLOCK(txq);
	}

	return (0);
}

void ionic_open_or_stop(struct lif *lif)
{
	if (lif->netdev->if_flags & IFF_UP && lif->link_up)
		ionic_open(lif);
	else
		ionic_stop(lif->netdev);
}

/******************* AdminQ ******************************/
int ionic_adminq_clean(struct adminq* adminq, int limit)
{
	struct admin_comp *comp;
	struct admin_cmd *cmd;
	struct ionic_admin_ctx *ctx;
	struct adminq_stats *stat = &adminq->stats;
	int comp_index, cmd_index, processed;

	KASSERT(IONIC_ADMIN_LOCK_OWNED(adminq), ("adminq is not locked"));

	/* Sync every time descriptors. */
	bus_dmamap_sync(adminq->cmd_dma.dma_tag, adminq->cmd_dma.dma_map,
		BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	for (processed = 0 ; processed < limit ; processed++) {
		comp_index = adminq->comp_index;
		comp = &adminq->comp_ring[comp_index];
		cmd_index = adminq->tail_index;
		cmd = &adminq->cmd_ring[cmd_index];
		ctx = adminq->ctx_ring[cmd_index];

		if (comp->color != adminq->done_color)
			break;

		if (ctx) {
			memcpy(&ctx->comp, comp, sizeof(*comp));
			complete_all(&ctx->work);
			IONIC_QUE_INFO(adminq, "completion done %p\n", &ctx->work);
			adminq->ctx_ring[cmd_index] = NULL;
		}

		IONIC_QUE_INFO(adminq, "comp :%d cmd start: %d cmd stop: %d status %d\n",
			comp_index, cmd_index, comp->comp_index, comp->status);

		if (comp->status) {
			IONIC_QUE_ERROR(adminq, "failed for opcode: %d status: %d\n",
				cmd->opcode, comp->status);
			stat->comp_err++;
		}

		IONIC_NETDEV_INFO(adminq->lif->netdev, "admin comp:\n");
#ifdef IONIC_DEBUG
		print_hex_dump_debug("admin comp ", DUMP_PREFIX_OFFSET, 16, 1,
			     comp, sizeof(struct admin_comp), true);
#endif

		adminq->comp_index = (adminq->comp_index + 1) % adminq->num_descs;
		adminq->tail_index = (adminq->tail_index + 1) % adminq->num_descs;
		/* Roll over condition, flip color. */
		if (adminq->comp_index == 0) {
			adminq->done_color = !adminq->done_color;
		}
	}

	IONIC_QUE_INFO(adminq, "head :%d tail: %d comp index: %d\n",
		adminq->head_index, adminq->tail_index, adminq->comp_index);
	return (processed);
}

static irqreturn_t ionic_adminq_isr(int irq, void *data)
{
	struct adminq* adminq = data;
	int processed;

	KASSERT(adminq, ("adminq == NULL"));

	IONIC_ADMIN_LOCK(adminq);
	ionic_intr_mask(&adminq->intr, true);

	processed = ionic_adminq_clean(adminq, adminq->num_descs);

	IONIC_QUE_INFO(adminq, "processed %d/%d\n", processed, adminq->intr.ctrl->int_credits);

	ionic_intr_return_credits(&adminq->intr, processed, true, true);
	IONIC_ADMIN_UNLOCK(adminq);

	KASSERT(processed, ("noting processed in adminq"));

	return IRQ_HANDLED;
}

static int _ionic_lif_addr_add(struct lif *lif, const u8 *addr)
{
	struct rx_filter *f;
	int err;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rx_filter_add = {
			.opcode = CMD_OPCODE_RX_FILTER_ADD,
			.match = RX_FILTER_MATCH_MAC,
		},
	};

	f = ionic_rx_filter_by_addr(lif, addr);
	if (f != NULL) {
		IONIC_NETDEV_ADDR_DEBUG(lif->netdev, addr, "duplicate address");
		return EINVAL;
	}

	memcpy(ctx.cmd.rx_filter_add.mac.addr, addr, ETH_ALEN);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		IONIC_NETDEV_ADDR_DEBUG(lif->netdev, addr, "failed to add, err: %d", err);
	else		
		IONIC_NETDEV_ADDR_INFO(lif->netdev, addr, "added (filter id %d)",
			ctx.comp.rx_filter_add.filter_id);

	err = ionic_rx_filter_save(lif, 0, RXQ_INDEX_ANY, 0, &ctx);
	if (err)
		IONIC_NETDEV_ADDR_DEBUG(lif->netdev, addr, "failed to save filter (filter id %d)",
			ctx.comp.rx_filter_add.filter_id);

	return (err);
}

static int _ionic_lif_addr_del(struct lif *lif, const u8 *addr)
{
	struct rx_filter *f;
	int err;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rx_filter_del = {
			.opcode = CMD_OPCODE_RX_FILTER_DEL,
		},
	};


	f = ionic_rx_filter_by_addr(lif, addr);
	if (!f) {
		IONIC_NETDEV_ADDR_INFO(lif->netdev, addr, "no entry");
		return ENOENT;
	}

	KASSERT(f->filter_id, ("addr del, filter id(%d) == 0", f->filter_id));
	ctx.cmd.rx_filter_del.filter_id = f->filter_id;
	ionic_rx_filter_free(lif, f);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		IONIC_NETDEV_ADDR_DEBUG(lif->netdev, addr, "failed to delete), err: %d", err);
	else
		IONIC_NETDEV_ADDR_INFO(lif->netdev, addr, "deleted (filter id: %d)",
			ctx.cmd.rx_filter_del.filter_id);
	return err;
}

static void ionic_lif_addr_work(struct work_struct *work)
{
	struct lif_addr_work *w  = container_of(work, struct lif_addr_work,
						work);

	IONIC_NETDEV_ADDR_INFO(w->lif->netdev, w->addr, "Work start: rx_filter %s ADDR Work: %p",
						   w->add ? "add" : "del", w);
	IONIC_CORE_LOCK(w->lif);
	if (w->add)
		_ionic_lif_addr_add(w->lif, w->addr);
	else
		_ionic_lif_addr_del(w->lif, w->addr);
	IONIC_CORE_UNLOCK(w->lif);

	free(w, M_IONIC);
}

static int ionic_lif_addr(struct lif *lif, const u8 *addr, bool add)
{
	struct lif_addr_work *work;

	work = malloc(sizeof(*work), M_IONIC, M_NOWAIT | M_ZERO);
	if (!work) {
		IONIC_NETDEV_ERROR(lif->netdev, "failed to allocate memory for address work.\n");
		return ENOMEM;
	}

	INIT_WORK(&work->work, ionic_lif_addr_work);
	work->lif = lif;
	memcpy(work->addr, addr, ETH_ALEN);
	work->add = add;
	IONIC_NETDEV_ADDR_INFO(lif->netdev, addr, "deferred: rx_filter %s ADDR Work: %p",
                             add ? "add" : "del", work);
	queue_work(lif->adminq_wq, &work->work);

	return 0;
}

static int ionic_addr_add(struct net_device *netdev, const u8 *addr)
{
	struct lif *lif = netdev_priv(netdev);

	return ionic_lif_addr(lif, addr, true);
}

static int ionic_addr_del(struct net_device *netdev, const u8 *addr)
{
	struct lif *lif = netdev_priv(netdev);

	return ionic_lif_addr(lif, addr, false);
}

static void _ionic_lif_rx_mode(struct lif *lif, unsigned int rx_mode)
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

        i = snprintf(buf, sizeof(buf), "Rx mode:");
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
        if (rx_mode == 0)
                i += snprintf(&buf[i], REMAIN(i), " cleared");
        IONIC_NETDEV_DEBUG(lif->netdev, "%s\n", buf);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err) {
		IONIC_NETDEV_ERROR(lif->netdev, "fail to configure rx mode, error: %d\n", err);
	}
}

static void ionic_lif_rx_mode_work(struct work_struct *work)
{
	struct rx_mode_work *w  = container_of(work, struct rx_mode_work, work);

	IONIC_CORE_LOCK(w->lif);
	_ionic_lif_rx_mode(w->lif, w->rx_mode);
	IONIC_CORE_UNLOCK(w->lif);
	
	free(w, M_IONIC);
}

static void ionic_lif_rx_mode(struct lif *lif, unsigned int rx_mode)
{
	struct rx_mode_work *work;

	IONIC_NETDEV_INFO(lif->netdev, "rx_mode %#x\n", rx_mode);

	work = malloc(sizeof(*work), M_IONIC, M_NOWAIT | M_ZERO);
	if (!work) {
		IONIC_NETDEV_ERROR(lif->netdev, "failed to allocate for rx_mode\n");
		return;
	}
	INIT_WORK(&work->work, ionic_lif_rx_mode_work);
	work->lif = lif;
	work->rx_mode = rx_mode;
	IONIC_NETDEV_INFO(lif->netdev, "deferred: rx_mode\n");
	queue_work(lif->adminq_wq, &work->work);
}

void ionic_set_rx_mode(struct net_device *netdev)
{
	struct lif *lif = netdev_priv(netdev);
	unsigned int rx_mode;

	rx_mode = RX_MODE_F_UNICAST;
	rx_mode |= (netdev->if_flags & IFF_MULTICAST) ? RX_MODE_F_MULTICAST : 0;
	rx_mode |= (netdev->if_flags & IFF_BROADCAST) ? RX_MODE_F_BROADCAST : 0;
	rx_mode |= (netdev->if_flags & IFF_PROMISC) ? RX_MODE_F_PROMISC : 0;
	rx_mode |= (netdev->if_flags & IFF_ALLMULTI) ? RX_MODE_F_ALLMULTI : 0;

	IONIC_NETDEV_INFO(netdev, "Setting rx mode %d\n", rx_mode);

	if (lif->rx_mode != rx_mode) {
		lif->rx_mode = rx_mode;
		ionic_lif_rx_mode(lif, rx_mode);
	}
}

void ionic_clear_rx_mode(struct net_device *netdev)
{
	struct lif *lif = netdev_priv(netdev);

	IONIC_NETDEV_INFO(netdev, "Clearing rx mode %d\n", lif->rx_mode);

	if (lif->rx_mode != 0) {
		lif->rx_mode = 0;
		ionic_lif_rx_mode(lif, 0);
	}
}

/*
 * Program the multicast addresses.
 */
int
ionic_set_multi(struct lif* lif)
{
	struct ifmultiaddr *ifma;
	struct ifnet *ifp = lif->netdev;
	int i, j, mcnt = 0, max_maddrs;
	struct ionic_mc_addr *new_mc_addrs, *tmp;
	uint8_t mac_addr[ETHER_ADDR_LEN];
	int num_new_mc_addrs;
	int err = 0;
	bool found;

	max_maddrs = lif->ionic->ident->dev.nmcasts_per_lif;

	if (lif->mc_addrs == NULL)
		return EIO;

	TAILQ_FOREACH(ifma, &ifp->if_multiaddrs, ifma_link) {
		mcnt++;
	}

	new_mc_addrs = malloc(sizeof(struct ionic_mc_addr) * mcnt, M_IONIC, M_NOWAIT | M_ZERO);
	num_new_mc_addrs = 0;

	/* Find the new address we need to add. */
	TAILQ_FOREACH(ifma, &ifp->if_multiaddrs, ifma_link) {
		if (ifma->ifma_addr->sa_family != AF_LINK)
			continue;
		bcopy(LLADDR((struct sockaddr_dl *) ifma->ifma_addr),
			mac_addr, ETHER_ADDR_LEN);

		found = false;
		for (i = 0 ; i < lif->num_mc_addrs; i++) {
			if (bcmp(mac_addr, lif->mc_addrs[i].addr, ETHER_ADDR_LEN) == 0) {
				found = true;
				lif->mc_addrs[i].present = true;
			}
		}

		if (!found)
			bcopy(mac_addr, new_mc_addrs[num_new_mc_addrs++].addr, ETHER_ADDR_LEN);

	}

	/* Set all multi if we are out of filters. */
	if (mcnt >= max_maddrs) {
		if ((ifp->if_flags & IFF_ALLMULTI) == 0) {
			ifp->if_flags |= IFF_ALLMULTI;
			ionic_set_rx_mode(ifp);
		}
	} else {
		if (ifp->if_flags & IFF_ALLMULTI) {
			ifp->if_flags ^= IFF_ALLMULTI;
			ionic_set_rx_mode(ifp);
		}
	}

	/* Delete the addresses which are not in new list. */
	j = 0;
	for (i = 0; i < lif->num_mc_addrs; i++) {
		IONIC_NETDEV_ADDR_INFO(ifp, lif->mc_addrs[i].addr, "current list[%d]", i);
		tmp = &lif->mc_addrs[i];
		if (!tmp->present) {
			IONIC_NETDEV_ADDR_DEBUG(ifp, tmp->addr, "deleting");
			err = ionic_addr_del(ifp, tmp->addr);
			if (err)
				goto err_out;
		} else {
			IONIC_NETDEV_ADDR_INFO(ifp, lif->mc_addrs[j].addr, "add ");
			bcopy(lif->mc_addrs[i].addr, lif->mc_addrs[j++].addr,
				  ETHER_ADDR_LEN);
		}
		tmp->present = false;
	}
	lif->num_mc_addrs = j;

	/* Now add the addresses which are not present. */
	for (i = 0; i < num_new_mc_addrs && lif->num_mc_addrs < max_maddrs; i++) {
		IONIC_NETDEV_ADDR_DEBUG(ifp, new_mc_addrs[i].addr, "adding");
		bcopy(new_mc_addrs[i].addr, lif->mc_addrs[lif->num_mc_addrs++].addr,
			ETHER_ADDR_LEN);
		err = ionic_addr_add(ifp, new_mc_addrs[i].addr);
		if (err)
			goto err_out;
	}

	IONIC_NETDEV_INFO(ifp, "Number of MCAST: %d\n", lif->num_mc_addrs);

err_out:
	free(new_mc_addrs, M_IONIC);
	return (err);
}

int ionic_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct lif *lif = netdev_priv(netdev);
	struct rxque *rxq;
	unsigned int i;
	int err;
	bool mbuf_size_changed;
	uint32_t old_mbuf_size;

	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.mtu_set = {
			.opcode = CMD_OPCODE_MTU_SET,
			.mtu = new_mtu,
		},
	};

	if (if_getmtu(netdev) == new_mtu) {
		IONIC_NETDEV_INFO(netdev, "unchanged MTU %d\n", new_mtu);
		return (0);
	}

	if (new_mtu < IONIC_MIN_MTU || new_mtu > IONIC_MAX_MTU) {
		IONIC_NETDEV_ERROR(netdev, "invalid MTU %d\n", new_mtu);
		return (EINVAL);
	}

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	/* check rx mbuf size needs to be changed */
	lif->max_frame_size = new_mtu + ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN + ETHER_CRC_LEN;
	old_mbuf_size = lif->rx_mbuf_size;
	ionic_calc_rx_size(lif);
	mbuf_size_changed = (old_mbuf_size != lif->rx_mbuf_size);

	/* if mbuf size has not changed then there is no need to reprogram queues.
	*/
	if (mbuf_size_changed) {
		ionic_stop(netdev);

		for (i = 0; i < lif->nrxqs; i++) {
			rxq = lif->rxqs[i];
			IONIC_RX_LOCK(rxq);
			ionic_rx_refill(rxq);
			IONIC_RX_UNLOCK(rxq);
		}

		ionic_open_or_stop(lif);
	}

	IONIC_NETDEV_INFO(netdev, "Changed MTU %d > %d MBUF %d > %d\n",
		if_getmtu(netdev), new_mtu, old_mbuf_size, lif->rx_mbuf_size);
	if_setmtu(netdev, new_mtu);

	return 0;
}

static int
_ionic_vlan_add(struct net_device *netdev, u16 vid)
{
	struct lif *lif = netdev_priv(netdev);
	struct rx_filter *f;
	int err;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rx_filter_add = {
			.opcode = CMD_OPCODE_RX_FILTER_ADD,
			.match = RX_FILTER_MATCH_VLAN,
			.vlan.vlan = vid,
		},
	};

	f = ionic_rx_filter_by_vlan(lif, vid);
	if (f != NULL) {
		IONIC_NETDEV_ERROR(netdev, "duplicate VLAN%d entry\n", vid);
		return ENOENT;
	}


	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		IONIC_NETDEV_ERROR(netdev, "failed to add VLAN%d (filter id: %d), error: %d\n",
			vid, ctx.comp.rx_filter_add.filter_id, err);
	else
		IONIC_NETDEV_INFO(netdev, "added VLAN%d (filter id: %d)\n",
			vid, ctx.comp.rx_filter_add.filter_id);

	err = ionic_rx_filter_save(lif, 0, RXQ_INDEX_ANY, 0, &ctx);
	if (err)
		IONIC_NETDEV_ERROR(netdev, "failed to save filter for VLAN%d (filter id: %d), error: %d\n",
			vid, ctx.comp.rx_filter_add.filter_id, err);

	return (err);
}

static int
_ionic_vlan_del(struct net_device *netdev, u16 vid)
{
	struct lif *lif = netdev_priv(netdev);
	struct rx_filter *f;
	int err;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rx_filter_del = {
			.opcode = CMD_OPCODE_RX_FILTER_DEL,
		},
	};


	f = ionic_rx_filter_by_vlan(lif, vid);
	if (!f) {
		IONIC_NETDEV_ERROR(netdev, "No VLAN%d filter found\n", vid);
		return ENOENT;
	}

	KASSERT(f->filter_id, ("vlan del, filter id(%d) == 0", f->filter_id));
	ctx.cmd.rx_filter_del.filter_id = f->filter_id;
	ionic_rx_filter_free(lif, f);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err) 
		IONIC_NETDEV_ERROR(netdev, "failed to del VLAN%d (id %d), error: %d\n",
			vid, ctx.cmd.rx_filter_del.filter_id, err);
	else
		IONIC_NETDEV_DEBUG(netdev, "deleted VLAN%d (id %d)\n", vid,
			ctx.cmd.rx_filter_del.filter_id);

	return (err);
}

static void
ionic_lif_vlan_work(struct work_struct *work)
{
	struct lif_vlan_work *w = container_of(work, struct lif_vlan_work, work);
	struct ifnet *netdev = w->lif->netdev;

	IONIC_NETDEV_INFO(netdev, "Work start: %s VLAN%d", w->add ? "add" : "del", w->vid);
	IONIC_CORE_LOCK(w->lif);
	if (w->add)
		_ionic_vlan_add(netdev, w->vid);
	else
		_ionic_vlan_del(netdev, w->vid);
	
	IONIC_CORE_UNLOCK(w->lif);

	free(w, M_IONIC);
}

static int
ionic_lif_vlan(struct lif *lif, const u16 vid, bool add)
{
	struct lif_vlan_work *work;

	work = malloc(sizeof(*work), M_IONIC, M_NOWAIT | M_ZERO);
	if (!work) {
		IONIC_NETDEV_ERROR(lif->netdev, "failed to allocate memory for address work.\n");
		return (ENOMEM);
	}

	INIT_WORK(&work->work, ionic_lif_vlan_work);
	work->lif = lif;
	work->vid = vid;
	work->add = add;
	IONIC_NETDEV_INFO(lif->netdev, "deferred: %s VLAN%d", add ? "add" : "del", vid);
	queue_work(lif->adminq_wq, &work->work);

	return (0);
}

static void
ionic_register_vlan(void *arg, struct ifnet *ifp, u16 vtag)
{
	struct lif *lif = ifp->if_softc;
	int index = vtag / 8;
	int bit = vtag % 8;

	if (ifp->if_softc != arg)   /* Not our event */
		return;

	if ((vtag == 0) || (vtag > MAX_VLAN_TAG))  /* Invalid */
		return;


	if (lif->vlan_bitmap[index] & BIT(bit)) {
		IONIC_NETDEV_WARN(lif->netdev, "VLAN: %d is already registered\n", vtag);
		IONIC_CORE_UNLOCK(lif);
		return;
	}

	if (ionic_lif_vlan(lif, vtag, true)) {
		IONIC_NETDEV_ERROR(lif->netdev, "VLAN: %d register failed\n", vtag);
		IONIC_CORE_UNLOCK(lif);
		return;
	}

	lif->vlan_bitmap[index] |= BIT(bit);
	++lif->num_vlans;

	IONIC_NETDEV_INFO(lif->netdev, "VLAN: %d registered\n", vtag);
}

static void
ionic_unregister_vlan(void *arg, struct ifnet *ifp, u16 vtag)
{
	struct lif *lif = ifp->if_softc;
	int index = vtag / 8;
	int bit = vtag % 8;

	if (ifp->if_softc != arg)
		return;

	if ((vtag == 0) || (vtag > MAX_VLAN_TAG))  /* Invalid */
		return;

	if ((lif->vlan_bitmap[index] & BIT(bit)) == 0) {
		IONIC_NETDEV_WARN(lif->netdev, "VLAN: %d is not registered\n", vtag);
		IONIC_CORE_UNLOCK(lif);
		return;
	}

	if (ionic_lif_vlan(lif, vtag, false)) {
		IONIC_NETDEV_ERROR(lif->netdev, "VLAN: %d unregister failed\n", vtag);
		IONIC_CORE_UNLOCK(lif);
		return;
	}

	lif->vlan_bitmap[index] &= ~BIT(bit);
	--lif->num_vlans;

	IONIC_NETDEV_INFO(lif->netdev, "VLAN: %d unregistered\n", vtag);
}


int ionic_dev_intr_reserve(struct lif *lif, struct intr *intr)
{
	struct ionic *ionic = lif->ionic;
	struct ionic_dev *idev = &ionic->idev;
	unsigned long index;

	KASSERT((intr->index == INTR_INDEX_NOT_ASSIGNED),
		("%s already has intr resource(%d)", intr->name, intr->index));

	index = find_first_zero_bit(ionic->intrs, ionic->nintrs);
	if (index == ionic->nintrs)
		return ENOSPC;

	set_bit(index, ionic->intrs);

	return ionic_intr_init(idev, intr, index);
}

void ionic_dev_intr_unreserve(struct lif *lif, struct intr *intr)
{
	if (intr->index != INTR_INDEX_NOT_ASSIGNED)
		clear_bit(intr->index, lif->ionic->intrs);
}

static int ionic_setup_intr(struct lif *lif, struct intr* intr)
{
	int error, irq;

	/* Setup interrupt */
	error = ionic_dev_intr_reserve(lif, intr);
	if (error) {
		IONIC_NETDEV_ERROR(lif->netdev, "no available interrupt, error: %d\n", error);
		return (error);
	}

	/*
	 * Legacy IRQ allocation is done later on.
	 * Since its level trigger, don't mask on assert.
	 */
	if (ionic_enable_msix == 0)
		return (0);

	irq = ionic_get_msix_irq(lif->ionic, intr->index);
	if (irq < 0) {
		ionic_dev_intr_unreserve(lif, intr);
		IONIC_NETDEV_ERROR(lif->netdev, "no available IRQ, error: %d\n", error);
		return (ENOSPC);
	}

	intr->vector = irq;

	ionic_intr_mask_on_assertion(intr);

	return (0);
}

static int ionic_adminq_alloc(struct lif *lif, unsigned int qnum,
			unsigned int num_descs, unsigned int pid,
			struct adminq **padminq)
{
	struct adminq *adminq;
	int error = ENOMEM;

	uint32_t cmd_ring_size, comp_ring_size, total_size;

	*padminq = NULL;

	adminq = malloc(sizeof(*adminq), M_IONIC, M_NOWAIT | M_ZERO);
	if(adminq == NULL) {
		IONIC_NETDEV_ERROR(lif->netdev, "failed to allocate adminq%d\n", qnum);
		return (error);
	}

	snprintf(adminq->name, sizeof(adminq->name) - 1, "aq%d", qnum);
	snprintf(adminq->mtx_name, sizeof(adminq->mtx_name) - 1, "aq%d_mtx", qnum);
	adminq->lif = lif;
	adminq->index = qnum;
	adminq->num_descs = num_descs;
	adminq->pid = lif->kern_pid;
	adminq->done_color = 1;
	adminq->intr.index = INTR_INDEX_NOT_ASSIGNED;

	IONIC_ADMIN_LOCK_INIT(adminq);

	adminq->ctx_ring = malloc(sizeof(struct ionic_admin_ctx *) * num_descs,
		M_IONIC, M_NOWAIT | M_ZERO);
	/* Allocate DMA for command and completion rings. They must be consecutive. */
	cmd_ring_size = sizeof(*adminq->cmd_ring) * num_descs;
	comp_ring_size = sizeof(*adminq->comp_ring) * num_descs;
	total_size = ALIGN(cmd_ring_size, PAGE_SIZE) + ALIGN(comp_ring_size, PAGE_SIZE);

	if ((error = ionic_dma_alloc(adminq->lif->ionic, total_size, &adminq->cmd_dma, BUS_DMA_NOWAIT))) {
		IONIC_QUE_ERROR(adminq, "failed to allocated DMA cmd ring, err: %d\n", error);
		goto failed_alloc;
	}

	adminq->cmd_ring_pa = adminq->cmd_dma.dma_paddr;
	adminq->cmd_ring = (struct admin_cmd *)adminq->cmd_dma.dma_vaddr;
	IONIC_QUE_INFO(adminq, "cmd base pa: 0x%lx size: 0x%x comp size: 0x%x total size: 0x%x\n",
		adminq->cmd_ring_pa, cmd_ring_size, comp_ring_size, total_size);
	/*
	 * Command and completion rings are side by side.
	 */
	adminq->comp_ring = (struct admin_comp *)(adminq->cmd_dma.dma_vaddr + ALIGN(cmd_ring_size, PAGE_SIZE));

	IONIC_QUE_INFO(adminq, "cmd base pa: 0x%lx size: 0x%x comp size: 0x%x total size: 0x%x\n",
		adminq->cmd_ring_pa, cmd_ring_size, comp_ring_size, total_size);
	bzero((void *)adminq->cmd_ring, total_size);

	error = ionic_setup_intr(lif, &adminq->intr);
	if (error) {
		IONIC_QUE_ERROR(adminq, "no available interrupt, error: %d\n", error);
		goto failed_alloc;
	}

	*padminq = adminq;
	return 0;

failed_alloc:
	if (adminq->cmd_ring) {
		ionic_dma_free(adminq->lif->ionic, &adminq->cmd_dma);
		adminq->cmd_ring = NULL;
		adminq->comp_ring = NULL;
	}

	free(adminq, M_IONIC);
	return (error);
}

static int ionic_notifyq_alloc(struct lif *lif, unsigned int qnum,
			unsigned int num_descs, unsigned int pid,
			struct notifyq **pnotifyq)
{
	struct notifyq *notifyq;
	int error = ENOMEM;
	uint32_t cmd_ring_size, comp_ring_size, total_size;

	*pnotifyq = NULL;

	notifyq = malloc(sizeof(*notifyq), M_IONIC, M_NOWAIT | M_ZERO);
	if(notifyq == NULL) {
		IONIC_NETDEV_ERROR(lif->netdev, "failed to allocate notifyq%d\n", qnum);
		return (error);
	}

	snprintf(notifyq->name, sizeof(notifyq->name) - 1, "nq%d", qnum);
	snprintf(notifyq->mtx_name, sizeof(notifyq->mtx_name) - 1, "nq%d_mtx", qnum);
	notifyq->lif = lif;
	notifyq->index = qnum;
	notifyq->num_descs = num_descs;
	notifyq->pid = pid;
	notifyq->intr.index = INTR_INDEX_NOT_ASSIGNED;
	lif->last_eid = 1;	/* Valid events are non zero. */

	IONIC_NOTIFYQ_LOCK_INIT(notifyq);

	/* Allocate DMA for command and completion rings. They must be consecutive. */
	cmd_ring_size = sizeof(*notifyq->cmd_ring) * num_descs;
	comp_ring_size = sizeof(*notifyq->comp_ring) * num_descs;
	total_size = ALIGN(cmd_ring_size, PAGE_SIZE) + ALIGN(cmd_ring_size, PAGE_SIZE);

	if ((error = ionic_dma_alloc(notifyq->lif->ionic, total_size, &notifyq->cmd_dma, BUS_DMA_NOWAIT))) {
		IONIC_QUE_ERROR(notifyq, "failed to allocated DMA cmd ring, err: %d\n", error);
		goto failed_alloc;
	}

	notifyq->cmd_ring_pa = notifyq->cmd_dma.dma_paddr;
	notifyq->cmd_ring = (struct notifyq_cmd *)notifyq->cmd_dma.dma_vaddr;
	IONIC_QUE_INFO(notifyq, "cmd base pa: 0x%lx size: 0x%x comp size: 0x%x total size: 0x%x\n",
		notifyq->cmd_ring_pa, cmd_ring_size, comp_ring_size, total_size);
	/*
	 * Command and completion rings are side by side.
	 */
	notifyq->comp_ring = (union notifyq_comp *)(notifyq->cmd_dma.dma_vaddr + ALIGN(cmd_ring_size, PAGE_SIZE));

	bzero((void *)notifyq->cmd_ring, total_size);

	error = ionic_setup_intr(lif, &notifyq->intr);
	if (error) {
		IONIC_QUE_ERROR(notifyq, "no available interrupt, error: %d\n", error);
		goto failed_alloc;
	}

	*pnotifyq = notifyq;
	return 0;

failed_alloc:
	if (notifyq->cmd_ring) {
		ionic_dma_free(notifyq->lif->ionic, &notifyq->cmd_dma);
		notifyq->cmd_ring = NULL;
		notifyq->comp_ring = NULL;
	}

	free(notifyq, M_IONIC);
	return (error);
}

static int ionic_rxque_alloc(struct lif *lif, unsigned int qnum,
			unsigned int num_descs, unsigned int pid,
			struct rxque **prxq)
{
	struct rxque *rxq;
	struct ionic_rx_buf *rxbuf;
	int i, error = ENOMEM;
	uint32_t cmd_ring_size, comp_ring_size, total_size;

	*prxq = NULL;

	rxq = malloc(sizeof(*rxq), M_IONIC, M_NOWAIT | M_ZERO);
	if(rxq == NULL) {
		IONIC_NETDEV_ERROR(lif->netdev, "failed to allocate rxq%d\n", qnum);
		return (error);
	}

	snprintf(rxq->name, sizeof(rxq->name) - 1, "rxq%d", qnum);
	snprintf(rxq->mtx_name, sizeof(rxq->mtx_name) - 1, "rxq%d_mtx", qnum);
	rxq->lif = lif;
	rxq->index = qnum;
	rxq->num_descs = num_descs;
	rxq->pid = pid;
	rxq->done_color = 1;
	rxq->intr.index = INTR_INDEX_NOT_ASSIGNED;

	IONIC_RX_LOCK_INIT(rxq);

	/* Setup command ring. */
	rxq->rxbuf = malloc(sizeof(*rxq->rxbuf) * num_descs, M_IONIC, M_NOWAIT | M_ZERO);
	if (rxq->rxbuf == NULL) {
		IONIC_QUE_ERROR(rxq, "Couldn't allocate rx buffer descriptors\n");
		goto failed_alloc;
	}

	/* Allocate DMA for command and completion rings. They must be consecutive. */
	cmd_ring_size = sizeof(*rxq->cmd_ring) * num_descs;
	comp_ring_size = sizeof(*rxq->comp_ring) * num_descs;
	total_size = ALIGN(cmd_ring_size, PAGE_SIZE) + ALIGN(cmd_ring_size, PAGE_SIZE);

	if ((error = ionic_dma_alloc(rxq->lif->ionic, total_size, &rxq->cmd_dma, BUS_DMA_NOWAIT))) {
		IONIC_QUE_ERROR(rxq, "failed to allocated DMA cmd ring, err: %d\n", error);
		goto failed_alloc;
	}

	rxq->cmd_ring_pa = rxq->cmd_dma.dma_paddr;
	rxq->cmd_ring = (struct rxq_desc *)rxq->cmd_dma.dma_vaddr;
	IONIC_QUE_INFO(rxq, "cmd base pa: 0x%lx size: 0x%x comp size: 0x%x total size: 0x%x\n",
		rxq->cmd_ring_pa, cmd_ring_size, comp_ring_size, total_size);
	/*
	 * We assume that competion ring is next to command ring.
	 */
	rxq->comp_ring = (struct rxq_comp *)(rxq->cmd_dma.dma_vaddr + ALIGN(cmd_ring_size, PAGE_SIZE));

	bzero((void *)rxq->cmd_ring, total_size);

	/* Setup interrupt */
	error = ionic_setup_intr(lif, &rxq->intr);
	if (error) {
		IONIC_QUE_ERROR(rxq, "no available interrupt, error: %d\n", error);
		goto failed_alloc;
	}

	/*
	 * Create just one tag for Rx buffers.
	 */
	error = bus_dma_tag_create(
	         /*      parent */ bus_get_dma_tag(rxq->lif->ionic->dev->bsddev),
	         /*   alignment */ 1,
	         /*      bounds */ 0,
	         /*     lowaddr */ BUS_SPACE_MAXADDR,
	         /*    highaddr */ BUS_SPACE_MAXADDR,
	         /*      filter */ NULL,
	         /*   filterarg */ NULL,
	         /*     maxsize */ MJUM16BYTES,
	         /*   nsegments */ 1,
	         /*  maxsegsize */ MJUM16BYTES,
	         /*       flags */ 0,
	         /*    lockfunc */ NULL,
	         /* lockfuncarg */ NULL,
	                           &rxq->buf_tag);

	if (error) {
		IONIC_QUE_ERROR(rxq, "failed to create DMA tag, err: %d\n", error);
		goto free_intr;
	}

	for ( rxbuf = rxq->rxbuf, i = 0 ; rxbuf != NULL && i < num_descs; i++, rxbuf++ ) {
		error = bus_dmamap_create(rxq->buf_tag, 0, &rxbuf->dma_map);
		if (error) {
			IONIC_QUE_ERROR(rxq, "failed to create map for entry%d, err: %d\n", i, error);
			bus_dma_tag_destroy(rxq->buf_tag);
			goto free_intr;
		}
	}

	*prxq = rxq;
	return 0;

free_intr:
	ionic_dev_intr_unreserve(lif, &rxq->intr);

failed_alloc:
	if (rxq->cmd_ring) {
		ionic_dma_free(rxq->lif->ionic, &rxq->cmd_dma);
		rxq->cmd_ring = NULL;
		rxq->comp_ring = NULL;
	}

	if (rxq->rxbuf) {
		free(rxq->rxbuf, M_IONIC);
		rxq->rxbuf = NULL;
	}

	free(rxq, M_IONIC);

	return (error);
}

static int ionic_txque_alloc(struct lif *lif, unsigned int qnum,
			   unsigned int num_descs, unsigned int pid,
			   struct txque **ptxq)
{
	struct txque *txq;
	struct ionic_tx_buf *txbuf;
	int i, error = ENOMEM;
	uint32_t cmd_ring_size, comp_ring_size, sg_ring_size, total_size;

	*ptxq = NULL;

	txq = malloc(sizeof(*txq), M_IONIC, M_NOWAIT | M_ZERO);
	if(txq == NULL) {
		IONIC_NETDEV_ERROR(lif->netdev, "failed to allocate txq%d\n", qnum);
		return (error);
	}

	snprintf(txq->name, sizeof(txq->name) - 1, "txq%d", qnum);
	snprintf(txq->mtx_name, sizeof(txq->mtx_name) - 1, "tx%d_mtx", qnum);
	txq->lif = lif;
	txq->index = qnum;
	txq->num_descs = num_descs;
	txq->pid = pid;
	txq->done_color = 1;
	txq->intr.index = INTR_INDEX_NOT_ASSIGNED;

	IONIC_TX_LOCK_INIT(txq);

	/* Setup command ring. */
	txq->txbuf = malloc(sizeof(*txq->txbuf) * num_descs, M_IONIC, M_NOWAIT | M_ZERO);
	if (txq->txbuf == NULL) {
		IONIC_QUE_ERROR(txq, "Couldn't allocate tx buffer descriptors\n");
		goto failed_alloc;
	}

	/* Allocate DMA for command and completion rings. They must be consecutive. */
	cmd_ring_size = sizeof(*txq->cmd_ring) * num_descs;
	comp_ring_size = sizeof(*txq->comp_ring) * num_descs;
	sg_ring_size = sizeof(*txq->sg_ring) * num_descs;
	total_size = ALIGN(cmd_ring_size, PAGE_SIZE) + ALIGN(cmd_ring_size, PAGE_SIZE) + ALIGN(sg_ring_size, PAGE_SIZE);

	if ((error = ionic_dma_alloc(txq->lif->ionic, total_size, &txq->cmd_dma, BUS_DMA_NOWAIT))) {
		IONIC_QUE_ERROR(txq, "failed to allocated DMA cmd ring, err: %d\n", error);
		goto failed_alloc;
	}

	txq->cmd_ring_pa = txq->cmd_dma.dma_paddr;
	txq->cmd_ring = (struct txq_desc *)txq->cmd_dma.dma_vaddr;
	IONIC_QUE_INFO(txq, "cmd base pa: 0x%lx size: 0x%x comp size: 0x%x total size: 0x%x\n",
		txq->cmd_ring_pa, cmd_ring_size, comp_ring_size, total_size);
	/*
	 * Command and completion rings are side by side.
	 */
	txq->comp_ring = (struct txq_comp *)(txq->cmd_dma.dma_vaddr + ALIGN(cmd_ring_size, PAGE_SIZE));
	txq->sg_ring = (struct txq_sg_desc *)(txq->cmd_dma.dma_vaddr + ALIGN(cmd_ring_size, PAGE_SIZE) + ALIGN(comp_ring_size, PAGE_SIZE));

	bzero((void *)txq->cmd_ring, total_size);

	/* Allocate buffere ring. */
	txq->br = buf_ring_alloc(4096, M_IONIC, M_WAITOK, &txq->tx_mtx);
	if (txq->br == NULL) {
		IONIC_QUE_ERROR(txq, "failed to allocated buffer ring\n");
		goto failed_alloc;
	}

#ifdef IONIC_SEPERATE_TX_INTR
	/* Setup interrupt for Tx. */
	error = ionic_setup_intr(lif, &txq->intr);
	if (error) {
		IONIC_QUE_ERROR(txq, "no available interrupt, error: %d\n", error);
		goto failed_alloc;
	}
#endif
	/*
	 * Create just one tag for Rx bufferrs.
	 */
	error = bus_dma_tag_create(
	         /*      parent */ bus_get_dma_tag(txq->lif->ionic->dev->bsddev),
	         /*   alignment */ 1,
	         /*      bounds */ 0,
	         /*     lowaddr */ BUS_SPACE_MAXADDR,
	         /*    highaddr */ BUS_SPACE_MAXADDR,
	         /*      filter */ NULL,
	         /*   filterarg */ NULL,
	         /*     maxsize */ IONIC_MAX_TSO_SIZE,
	         /*   nsegments */ IONIC_MAX_TSO_SG_ENTRIES,
	         /*  maxsegsize */ IONIC_MAX_TSO_SG_SIZE,
	         /*       flags */ 0,
	         /*    lockfunc */ NULL,
	         /* lockfuncarg */ NULL,
	                           &txq->buf_tag);

	if (error) {
		IONIC_QUE_ERROR(txq, "failed to create DMA tag, err: %d\n", error);
		goto free_intr;
	}

	for ( txbuf = txq->txbuf, i = 0 ; txbuf != NULL && i < num_descs; i++, txbuf++ ) {
		error = bus_dmamap_create(txq->buf_tag, 0, &txbuf->dma_map);
		if (error) {
			IONIC_QUE_ERROR(txq, "failed to create map for entry%d, err: %d\n", i, error);
			bus_dma_tag_destroy(txq->buf_tag);
			goto free_intr;
		}
	}

	*ptxq = txq;
	return 0;

free_intr:
#ifdef IONIC_SEPERATE_TX_INTR
	ionic_dev_intr_unreserve(lif, &txq->intr);
#endif

failed_alloc:
	if (txq->br) {
		buf_ring_free(txq->br, M_IONIC);
		txq->br = NULL;
	}

	if (txq->cmd_ring) {
		/* Completion ring is part of command ring allocation. */
		ionic_dma_free(txq->lif->ionic, &txq->cmd_dma);
		txq->cmd_ring = NULL;
		txq->comp_ring = NULL;
		txq->sg_ring = NULL;
	}

	if (txq->txbuf) {
		free(txq->txbuf, M_IONIC);
		txq->txbuf = NULL;
	}

	free(txq, M_IONIC);
	return (error);
}

static void ionic_rxq_free(struct lif *lif, struct rxque *rxq)
{

	IONIC_RX_LOCK(rxq);

	tcp_lro_free(&rxq->lro);
	free(rxq->rxbuf, M_IONIC);

	if (rxq->cmd_ring) {
		/* Completion ring is part of command ring allocation. */
		ionic_dma_free(rxq->lif->ionic, &rxq->cmd_dma);
		rxq->cmd_ring = NULL;
		rxq->comp_ring = NULL;
	}

	ionic_dev_intr_unreserve(lif, &rxq->intr);

	IONIC_RX_UNLOCK(rxq);
	IONIC_RX_LOCK_DESTROY(rxq);

	free(rxq, M_IONIC);
}

static void ionic_txq_free(struct lif *lif, struct txque *txq)
{
	IONIC_TX_LOCK(txq);

	if (txq->br) {
		buf_ring_free(txq->br, M_IONIC);
		txq->br = NULL;
	}

	free(txq->txbuf, M_IONIC);

	if (txq->cmd_ring) {
		/* Completion ring is part of command ring allocation. */
		ionic_dma_free(txq->lif->ionic, &txq->cmd_dma);
		txq->cmd_ring = NULL;
		txq->comp_ring = NULL;
	}

#ifdef IONIC_SEPERATE_TX_INTR
	ionic_dev_intr_unreserve(lif, &txq->intr);
#endif

	IONIC_TX_UNLOCK(txq);
	IONIC_TX_LOCK_DESTROY(txq);

	free(txq, M_IONIC);
}

static void ionic_adminq_free(struct lif *lif, struct adminq *adminq)
{

	IONIC_ADMIN_LOCK(adminq);
	if (adminq->cmd_ring) {
		/* completion ring is part of command ring allocation. */
		ionic_dma_free(adminq->lif->ionic, &adminq->cmd_dma);
		adminq->cmd_ring = NULL;
		adminq->comp_ring = NULL;
	}

	ionic_dev_intr_unreserve(lif, &adminq->intr);

	IONIC_ADMIN_UNLOCK(adminq);
	IONIC_ADMIN_LOCK_DESTROY(adminq);

	if (adminq->ctx_ring)
		free(adminq->ctx_ring, M_IONIC);
	free(adminq, M_IONIC);
}

static void ionic_notifyq_free(struct lif *lif, struct notifyq *notifyq)
{

	IONIC_NOTIFYQ_LOCK(notifyq);
	if (notifyq->cmd_ring) {
		/* completion ring is part of command ring allocation. */
		ionic_dma_free(notifyq->lif->ionic, &notifyq->cmd_dma);
		notifyq->cmd_ring = NULL;
		notifyq->comp_ring = NULL;
	}

	ionic_dev_intr_unreserve(lif, &notifyq->intr);

	IONIC_NOTIFYQ_UNLOCK(notifyq);
	IONIC_NOTIFYQ_LOCK_DESTROY(notifyq);

	free(notifyq, M_IONIC);
}

static int ionic_qcqs_alloc(struct lif *lif)
{
	unsigned int i;
	int err = ENOMEM;

	lif->txqs = malloc(sizeof(*lif->txqs) * lif->ntxqs, M_IONIC, M_ZERO | M_NOWAIT);
	if (!lif->txqs)
		return ENOMEM;

	lif->rxqs = malloc(sizeof(*lif->rxqs) * lif->nrxqs, M_IONIC, M_ZERO | M_NOWAIT);
	if (!lif->rxqs) {
		free(lif->txqs, M_IONIC);
		lif->txqs = NULL;
		return ENOMEM;
	}

	err = ionic_adminq_alloc(lif, 0, adminq_descs, lif->kern_pid, &lif->adminq);
	if (err)
		return err;

	if (lif->ionic->nnqs_per_lif) {
		err = ionic_notifyq_alloc(lif, 0, ionic_notifyq_descs, lif->kern_pid, &lif->notifyq);
		if (err)
			goto err_out_free_adminq;
	}

	for (i = 0; i < lif->ntxqs; i++) {
		err = ionic_txque_alloc(lif, i, ntxq_descs, lif->kern_pid,
					&lif->txqs[i]);
		if (err)
			goto err_out_free_notifyq;
	}

	for (i = 0; i < lif->nrxqs; i++) {
		err = ionic_rxque_alloc(lif, i, nrxq_descs, lif->kern_pid,
					&lif->rxqs[i]);
		if (err)
			goto err_out_free_txqs;
	}

	return 0;

err_out_free_txqs:
	for (i = 0; i < lif->ntxqs; i++)
		ionic_txq_free(lif, lif->txqs[i]);
err_out_free_notifyq:
	if(lif->notifyq)
		ionic_notifyq_free(lif, lif->notifyq);
err_out_free_adminq:
	ionic_adminq_free(lif, lif->adminq);

	free(lif->rxqs, M_IONIC);
	free(lif->txqs, M_IONIC);

	return err;
}

static void ionic_qcqs_free(struct lif *lif)
{
	unsigned int i;
	struct rxque *rxq;
	struct txque *txq;

	for (i = 0; i < lif->nrxqs; i++) {
		rxq = lif->rxqs[i];
		ionic_rx_empty(rxq);
		ionic_rxq_free(lif, rxq);
	}
	for (i = 0; i < lif->ntxqs; i++) {
		txq = lif->txqs[i];
		ionic_txq_free(lif, txq);
	}

	if (lif->notifyq)
		ionic_notifyq_free(lif, lif->notifyq);
	if (lif->adminq)
		ionic_adminq_free(lif, lif->adminq);

	free(lif->rxqs, M_IONIC);
	free(lif->txqs, M_IONIC);
}

static void
ionic_setup_intr_coal(struct lif* lif)
{
	union identity *ident = lif->ionic->ident;
	u32 rx_coal;
	u32 tx_coal;
	unsigned int i;

	if (ident->dev.intr_coal_div == 0)
		return;

	tx_coal = lif->tx_coalesce_usecs * ident->dev.intr_coal_mult /
		  ident->dev.intr_coal_div;
	rx_coal = lif->rx_coalesce_usecs * ident->dev.intr_coal_mult /
		  ident->dev.intr_coal_div;

	if (tx_coal > INTR_CTRL_COAL_MAX || rx_coal > INTR_CTRL_COAL_MAX) {
		IONIC_NETDEV_ERROR(lif->netdev, "Coalescing value out of range\n");
		return;
	}

#ifdef IONIC_SEPERATE_TX_INTR
	for (i = 0; i < lif->ntxqs; i++)
		ionic_intr_coal_set(&lif->txqs[i]->intr, tx_coal);
#endif

	for (i = 0; i < lif->nrxqs; i++)
		ionic_intr_coal_set(&lif->rxqs[i]->intr, rx_coal);
}

static int ionic_lif_alloc(struct ionic *ionic, unsigned int index)
{
	struct device *dev = ionic->dev;
	struct lif *lif;
	int err, dbpage_num;
	char name[16];

	lif = malloc(sizeof(*lif), M_IONIC, M_NOWAIT | M_ZERO);
	if (!lif) {
		dev_err(dev, "Cannot allocate lif, aborting\n");
		return ENOMEM;
	}

	snprintf(lif->name, sizeof(lif->name), "ionic%u", index);
	lif->ionic = ionic;
	lif->index = index;
	lif->neqs = ionic->neqs_per_lif;
	lif->ntxqs = ionic->ntxqs_per_lif;
	lif->nrxqs = ionic->nrxqs_per_lif;
	lif->nnqs = ionic->nnqs_per_lif;

	lif->mc_addrs = malloc(sizeof(struct ionic_mc_addr) * ionic->ident->dev.nmcasts_per_lif, M_IONIC, M_NOWAIT | M_ZERO);

	if (ntxq_descs > IONIX_TX_MAX_DESC) {
		dev_err(dev, "num of tx descriptors > %d\n", IONIX_TX_MAX_DESC);
		ntxq_descs = IONIX_TX_MAX_DESC;
	}

	if (nrxq_descs > IONIX_RX_MAX_DESC) {
		dev_err(dev, "num of rx descriptors > %d\n", IONIX_RX_MAX_DESC);
		nrxq_descs = IONIX_RX_MAX_DESC;
	}

	err = ionic_lif_netdev_alloc(lif, ntxq_descs);
	if (err) {
		dev_err(dev, "Cannot allocate netdev, aborting\n");
		return (err);
	}

	snprintf(name, sizeof(name), "adwq%d", index);
	lif->adminq_wq = create_workqueue(name);

	mutex_init(&lif->dbid_inuse_lock);
	lif->dbid_count = lif->ionic->ident->dev.ndbpgs_per_lif;
	if (!lif->dbid_count) {
		dev_err(dev, "No doorbell pages, aborting\n");
		err = -EINVAL;
		goto err_out_free_netdev;
	}

	lif->dbid_inuse = malloc(BITS_TO_LONGS(lif->dbid_count) * sizeof(long),
				  M_IONIC, M_NOWAIT | M_ZERO);
	if (!lif->dbid_inuse) {
		dev_err(dev, "Failed alloc doorbell id bitmap, aborting\n");
		err = -ENOMEM;
		goto err_out_free_netdev;
	}

	/* first doorbell id reserved for kernel (dbid aka pid == zero) */
	set_bit(0, lif->dbid_inuse);
	lif->kern_pid = 0;

	dbpage_num = ionic_db_page_num(&ionic->idev, index, 0);
	lif->kern_dbpage = ionic_bus_map_dbpage(ionic, dbpage_num);
	if (!lif->kern_dbpage) {
		dev_err(dev, "Cannot map dbpage, aborting\n");
		err = -ENOMEM;
		goto err_out_free_dbid;
	}

	err = ionic_qcqs_alloc(lif);
	if (err)
		goto err_out_unmap_dbell;

	/* Setup tunables. */
	lif->tx_coalesce_usecs = ionic_tx_coalesce_usecs;
	lif->rx_coalesce_usecs = ionic_rx_coalesce_usecs;

	ionic_setup_intr_coal(lif);

	list_add_tail(&lif->list, &ionic->lifs);

	return 0;

err_out_unmap_dbell:
	ionic_bus_unmap_dbpage(ionic, lif->kern_dbpage);
err_out_free_dbid:
	free(lif->dbid_inuse, M_IONIC);
err_out_free_netdev:
	ionic_lif_netdev_free(lif);
	free(lif, M_IONIC);

	return err;
}

int ionic_lifs_alloc(struct ionic *ionic)
{
	unsigned int i;
	int err;

	INIT_LIST_HEAD(&ionic->lifs);

	for (i = 0; i < ionic->ident->dev.nlifs; i++) {
		err = ionic_lif_alloc(ionic, i);
		if (err)
			return err;
	}

	return 0;
}

void ionic_lifs_free(struct ionic *ionic)
{
	struct list_head *cur, *tmp;
	struct lif *lif;

	list_for_each_safe(cur, tmp, &ionic->lifs) {
		lif = list_entry(cur, struct lif, list);
		list_del(&lif->list);
		flush_workqueue(lif->adminq_wq);
		destroy_workqueue(lif->adminq_wq);
		ionic_qcqs_free(lif);
		ionic_lif_netdev_free(lif);

		if (lif->notifyblock) {
			/* completion ring is part of command ring allocation. */
			ionic_dma_free(ionic, &lif->notify_dma);
			lif->notifyblock = NULL;
			lif->notifyblock_pa = 0;
		}

		ionic_bus_unmap_dbpage(lif->ionic, lif->kern_dbpage);
		free(lif->dbid_inuse, M_IONIC);
		free(lif, M_IONIC);
	}
}

static int
ionic_lif_stats_start(struct lif *lif, unsigned int ver)
{
	struct net_device *netdev = lif->netdev;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.lif_stats = {
			.opcode = CMD_OPCODE_LIF_STATS_START,
			.ver = ver,
		},
	};
	int error;

	if ((error = ionic_dma_alloc(lif->ionic, sizeof(struct ionic_lif_stats),
				&lif->stats_dma, BUS_DMA_NOWAIT))) {
		IONIC_NETDEV_ERROR(netdev, "no buffer for stats, err: %d\n", error);
		return error;
	}

	lif->lif_stats_pa = lif->stats_dma.dma_paddr;
	lif->lif_stats = (struct ionic_lif_stats *)lif->stats_dma.dma_vaddr;

	if (lif->lif_stats == NULL) {
		IONIC_NETDEV_ERROR(netdev, "failed to allocate stats buffer\n");
		goto err_out_free;
	}
	bzero(lif->lif_stats, sizeof(struct ionic_lif_stats));
	ctx.cmd.lif_stats.addr = lif->lif_stats_pa;

	IONIC_NETDEV_INFO(netdev, "lif_stats START ver %d addr %p(0x%lx)\n", ver,
		    lif->lif_stats, lif->lif_stats_pa);

	error = ionic_adminq_post_wait(lif, &ctx);
	if (error)
		goto err_out_free;

	return 0;

err_out_free:
	if (lif->lif_stats) {
		ionic_dma_free(lif->ionic, &lif->stats_dma);
		lif->lif_stats = NULL;
		lif->lif_stats_pa = 0;
	}

	return error;
}

static void
ionic_lif_stats_stop(struct lif *lif)
{
	struct net_device *netdev = lif->netdev;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.lif_stats = {
			.opcode = CMD_OPCODE_LIF_STATS_STOP,
		},
	};
	int err;

	IONIC_NETDEV_INFO(netdev, "lif_stats STOP\n");

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err) {
		IONIC_NETDEV_ERROR(netdev, "lif_stats cmd failed %d\n", err);
		return;
	}

	if (lif->lif_stats) {
		ionic_dma_free(lif->ionic, &lif->stats_dma);
		lif->lif_stats = NULL;
		lif->lif_stats_pa = 0;
	}
}

int
ionic_rss_ind_tbl_set(struct lif *lif, const u32 *indir)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rss_indir_set = {
			.opcode = CMD_OPCODE_RSS_INDIR_SET,
			.addr = lif->rss_ind_tbl_pa,
		},
#ifdef HAPS
		.side_data = lif->rss_ind_tbl,
		.side_data_len = RSS_IND_TBL_SIZE,
#endif
	};
	unsigned int i;

	if (indir)
		for (i = 0; i < RSS_IND_TBL_SIZE; i++)
			lif->rss_ind_tbl[i] = indir[i];

	IONIC_NETDEV_INFO(lif->netdev, "rss_ind_tbl_set\n");

	return ionic_adminq_post_wait(lif, &ctx);
}

int ionic_rss_hash_key_set(struct lif *lif, const u8 *key, uint16_t rss_types)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rss_hash_set = {
			.opcode = CMD_OPCODE_RSS_HASH_SET,
			.types = rss_types,
		},
	};

	memcpy(lif->rss_hash_key, key, RSS_HASH_KEY_SIZE);

	memcpy(ctx.cmd.rss_hash_set.key, lif->rss_hash_key,
	       RSS_HASH_KEY_SIZE);

	IONIC_NETDEV_INFO(lif->netdev, "rss_hash_key_set\n");

	return ionic_adminq_post_wait(lif, &ctx);
}

static void ionic_lif_adminq_deinit(struct lif *lif)
{
	struct adminq *adminq = lif->adminq;

	if (adminq->intr.index == INTR_INDEX_NOT_ASSIGNED)
		return;

	ionic_intr_mask(&adminq->intr, true);
	if (adminq->intr.vector)
		free_irq(adminq->intr.vector, adminq);

}

static void ionic_lif_notifyq_deinit(struct lif *lif)
{

	struct notifyq *notifyq = lif->notifyq;

	if (notifyq == NULL)
		return;

	ionic_q_enable_disable(lif, notifyq->qid, notifyq->qtype, false /* disable */);
	ionic_intr_mask(&notifyq->intr, true);

	if (notifyq->intr.vector)
		free_irq(notifyq->intr.vector, notifyq);

}

static void ionic_lif_txqs_deinit(struct lif *lif)
{
	unsigned int i;
	struct txque* txq;

	for (i = 0; i < lif->nrxqs; i++) {
		txq = lif->txqs[i];

		ionic_txq_disable(txq);
#ifdef IONIC_SEPERATE_TX_INTR
		ionic_intr_mask(&txq->intr, true);
		if (txq->intr.vector) {
			free_irq(txq->intr.vector, txq);
		}
#endif
		if (txq->taskq) {
			taskqueue_drain(txq->taskq, &txq->task);
			taskqueue_free(txq->taskq);
		}
	}
}

static void ionic_lif_rxqs_deinit(struct lif *lif)
{
	unsigned int i;
	struct rxque* rxq;

	for (i = 0; i < lif->nrxqs; i++) {
		rxq = lif->rxqs[i];

		ionic_intr_mask(&rxq->intr, true);
		ionic_rxq_disable(rxq);

		if (rxq->intr.vector) {
			free_irq(rxq->intr.vector, rxq);
		}

		if (rxq->taskq) {
			taskqueue_drain(rxq->taskq, &rxq->task);
			taskqueue_free(rxq->taskq);
		}
	}
}

static void ionic_lif_reset(struct lif *lif)
{
	struct ionic_dev *idev = &lif->ionic->idev;

	ionic_dev_cmd_lif_reset(idev, lif->index);
	ionic_dev_cmd_wait_check(idev, ionic_devcmd_timeout * HZ);
}

static void ionic_lif_deinit(struct lif *lif)
{

	ionic_lif_stats_stop(lif);
	ionic_rx_filters_deinit(lif);
	ionic_lif_rss_teardown(lif);

	/* Unregister VLAN events */
	if (lif->vlan_attach != NULL)
		EVENTHANDLER_DEREGISTER(vlan_config, lif->vlan_attach);
	if (lif->vlan_detach != NULL)
		EVENTHANDLER_DEREGISTER(vlan_unconfig, lif->vlan_detach);

	/* Free legacy interrupt resources. */
	if(ionic_enable_msix == 0) {
		free_irq(lif->ionic->pdev->irq, lif);
	}

	if (lif->mc_addrs) {
		free(lif->mc_addrs, M_IONIC);
		lif->mc_addrs = NULL;
	}

	ifmedia_removeall(&lif->media);

	if (lif->netdev && if_getifaddr(lif->netdev))
		ether_ifdetach(lif->netdev);

	ionic_lif_txqs_deinit(lif);
	ionic_lif_rxqs_deinit(lif);
	ionic_lif_notifyq_deinit(lif);
	ionic_lif_adminq_deinit(lif);

	ionic_lif_reset(lif);
}

void ionic_lifs_deinit(struct ionic *ionic)
{
	struct list_head *cur;
	struct lif *lif;

	list_for_each(cur, &ionic->lifs) {
		lif = list_entry(cur, struct lif, list);
		ionic_lif_deinit(lif);
	}
}

static int ionic_lif_adminq_init(struct lif *lif)
{
	struct adminq *adminq = lif->adminq;
	struct ionic_dev *idev = &lif->ionic->idev;
	struct adminq_init_comp comp;
	int err = 0;

	union dev_cmd cmd = {
		.adminq_init.opcode = CMD_OPCODE_ADMINQ_INIT,
		.adminq_init.index = adminq->index,
		.adminq_init.pid = adminq->pid,
		.adminq_init.intr_index = adminq->intr.index,
		.adminq_init.lif_index = lif->index,
		.adminq_init.ring_size = ilog2(adminq->num_descs),
		.adminq_init.ring_base = adminq->cmd_ring_pa,
	};

	//printk(KERN_ERR "adminq_init.pid %d\n", cmd.adminq_init.pid);
	//printk(KERN_ERR "adminq_init.index %d\n", cmd.adminq_init.index);
	//printk(KERN_ERR "adminq_init.ring_base %llx\n",
	//       cmd.adminq_init.ring_base);
	//printk(KERN_ERR "adminq_init.ring_size %d\n",
	//       cmd.adminq_init.ring_size);
	ionic_dev_cmd_go(idev, &cmd);

	err = ionic_dev_cmd_wait_check(idev, ionic_devcmd_timeout * HZ);
	if (err)
		return err;

	ionic_dev_cmd_comp(idev, &comp);

	IONIC_QUE_INFO(adminq, "qid %d pid %d lif %d ring_base 0x%lx ring_size %d\n",
		comp.qid, cmd.adminq_init.pid, cmd.adminq_init.index, cmd.adminq_init.ring_base,
		cmd.adminq_init.ring_size);

	adminq->qid = comp.qid;
	adminq->qtype = comp.qtype;

	snprintf(adminq->intr.name, sizeof(adminq->intr.name), "%s", adminq->name);

	if (ionic_enable_msix == 0)
		return (0);

	err = request_irq(adminq->intr.vector, ionic_adminq_isr, 0, adminq->intr.name, adminq);

	return (err);
}

static void ionic_process_event(struct notifyq* notifyq, union notifyq_comp *comp)
{
	struct lif *lif = notifyq->lif;
	struct ifnet *ifp = lif->netdev;

	switch (comp->event.ecode) {
	case EVENT_OPCODE_LINK_CHANGE:
		ionic_read_notify_block(lif);

		if (lif->last_eid < comp->event.eid) {
			if_printf(ifp, "comp eid: %ld > last_eid: %ld\n",
				comp->event.eid, lif->last_eid);
			lif->link_speed = comp->link_change.link_speed;
			lif->link_up = comp->link_change.link_status ? true : false;
			lif->link_error_bits = comp->link_change.link_error_bits;
			lif->phy_type = comp->link_change.phy_type;
			lif->autoneg_status = comp->link_change.autoneg_status;
			lif->last_eid = comp->event.eid;
		}

		IONIC_CORE_LOCK(lif);

		ionic_open_or_stop(lif);

		IONIC_CORE_UNLOCK(lif);
		if_printf(ifp, "[eid:%ld]link status: %s speed: %d\n",
			lif->last_eid, (lif->link_up ? "up" : "down"), lif->link_speed);
		break;

	case EVENT_OPCODE_RESET:
		if_printf(ifp, "[eid:%ld]reset_code: %d state: %d\n",
				comp->event.eid, comp->reset.reset_code,
			    comp->reset.state);
		break;

	case EVENT_OPCODE_HEARTBEAT:
		if_printf(ifp, "[eid:%ld]heartbeat\n", comp->event.eid);
		break;

	case EVENT_OPCODE_LOG:
		if_printf(ifp, "[eid:%ld]log \n",
			    comp->event.eid);
		print_hex_dump(KERN_INFO, "nq log", DUMP_PREFIX_OFFSET, 16, 1,
			       comp->log.data, sizeof(comp->log.data), true);
		break;

	default:
		if_printf(ifp, "[eid:%ld]unknown event: %d\n",
			    comp->event.eid, comp->event.ecode);
		break;
	}
}

static int ionic_notifyq_clean(struct notifyq* notifyq)
{
	struct lif *lif = notifyq->lif;
	int comp_index = notifyq->comp_index;
	union notifyq_comp *comp = &notifyq->comp_ring[comp_index];

	/* Sync every time descriptors. */
	bus_dmamap_sync(notifyq->cmd_dma.dma_tag, notifyq->cmd_dma.dma_map,
		BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	IONIC_QUE_INFO(notifyq, "enter comp index: %d\n", notifyq->comp_index);

	if (comp->event.eid < lif->last_eid) {
		IONIC_QUE_WARN(notifyq, "out of order comp: %d, event id: %ld, expected: %ld\n",
					comp_index, comp->event.eid, lif->last_eid);
		return (lif->last_eid - comp->event.eid);
	}

	IONIC_QUE_INFO(notifyq, "comp index: %d event id: %ld expected: %ld\n",
					comp_index, comp->event.eid, lif->last_eid);
	lif->last_eid = comp->event.eid;

	IONIC_NETDEV_INFO(lif->netdev, "noytifyq event:\n");
#ifdef IONIC_DEBUG
	print_hex_dump_debug("event ", DUMP_PREFIX_OFFSET, 16, 1,
						comp, sizeof(union notifyq_comp), true);
#endif

	ionic_process_event(notifyq, comp);

	notifyq->comp_index = (notifyq->comp_index + 1) % notifyq->num_descs;

	return 1;
}

static irqreturn_t ionic_notifyq_isr(int irq, void *data)
{
	struct notifyq* notifyq = data;
	int processed;

	IONIC_QUE_INFO(notifyq, "Enter\n");
	KASSERT(notifyq, ("notifyq == NULL"));

	IONIC_NOTIFYQ_LOCK(notifyq);
	ionic_intr_mask(&notifyq->intr, true);

	processed = ionic_notifyq_clean(notifyq);

	IONIC_QUE_INFO(notifyq, "processed %d\n", processed);

	ionic_intr_return_credits(&notifyq->intr, processed, 0, true);

	ionic_intr_mask(&notifyq->intr, false);
	IONIC_NOTIFYQ_UNLOCK(notifyq);

	return IRQ_HANDLED;
}


static int ionic_lif_notifyq_block_init(struct lif *lif)
{
	struct net_device *netdev = lif->netdev;
	int error;

	lif->notifyblock_sz = ALIGN(sizeof(*lif->notifyblock), PAGE_SIZE);

	if ((error = ionic_dma_alloc(lif->ionic, lif->notifyblock_sz, &lif->notify_dma, BUS_DMA_NOWAIT))) {
		IONIC_NETDEV_ERROR(netdev, "failed to allocated notifyq block, err: %d\n", error);
		return error;
	}

	lif->notifyblock = (struct notify_block *)lif->notify_dma.dma_vaddr;

	if (!lif->notifyblock) {
		IONIC_NETDEV_ERROR(netdev, "failed to allocate notify block\n");
		return ENOMEM;
	}

	bzero(lif->notifyblock, sizeof(struct notify_block ));
	lif->notifyblock_pa = lif->notify_dma.dma_paddr;

	IONIC_NETDEV_INFO(netdev, "notify addr %p(0x%lx)\n",
		    lif->notifyblock, lif->notifyblock_pa);


	return 0;
}

static int ionic_lif_notifyq_init(struct lif *lif, struct notifyq *notifyq)
{
	int err;

	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.notifyq_init = {
			.opcode = CMD_OPCODE_NOTIFYQ_INIT,
			.index = notifyq->index,
			.pid = notifyq->pid,
			.intr_index = notifyq->intr.index,
			.lif_index = lif->index,
			.ring_base = notifyq->cmd_ring_pa,
			.ring_size = ilog2(notifyq->num_descs),
		},
	};

	err = ionic_lif_notifyq_block_init(lif);
	if (err)
		return err;

	ctx.cmd.notifyq_init.notify_size = ilog2(lif->notifyblock_sz);
	ctx.cmd.notifyq_init.notify_base = lif->notifyblock_pa;

	IONIC_QUE_INFO(notifyq, "pid %d index %d ring_base 0x%lx ring_size %d"
				" notiyfy_base 0x%lx notify_size %d\n",
				ctx.cmd.notifyq_init.pid, ctx.cmd.notifyq_init.index,
				ctx.cmd.notifyq_init.ring_base, ctx.cmd.notifyq_init.ring_size,
				ctx.cmd.notifyq_init.notify_base, ctx.cmd.notifyq_init.notify_size);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err) {
		goto free_notify_block;
	}

	notifyq->qid = ctx.comp.rxq_init.qid;
	notifyq->qtype = ctx.comp.rxq_init.qtype;

	snprintf(notifyq->intr.name, sizeof(notifyq->intr.name), "%s", notifyq->name);

	/* Legacy interrupt allocation is done once. */
	if (ionic_enable_msix == 0)
		return (0);

	err = request_irq(notifyq->intr.vector, ionic_notifyq_isr, 0,
					notifyq->intr.name, notifyq);
	if (err) {
		IONIC_QUE_ERROR(notifyq, "request_irq failed, error: %d\n", err);
		return (err);
	}

	ionic_intr_mask(&notifyq->intr, false);

	ionic_read_notify_block(lif);
	return 0;

free_notify_block:
	if (lif->notifyblock) {
		ionic_dma_free(lif->ionic, &lif->notify_dma);
		lif->notifyblock = NULL;
		lif->notifyblock_pa = 0;
	}

	return err;
}

/*************************** Transmit ************************/
int ionic_tx_clean(struct txque* txq , int tx_limit)
{
	struct txq_comp *comp;
	struct ionic_tx_buf *txbuf;
	int comp_index, processed, cmd_stop_index, batch = 0;
	struct tx_stats * stats = &txq->stats;

	KASSERT(IONIC_TX_LOCK_OWNED(txq), ("%s is not locked", txq->name));
	stats->clean++;

	bus_dmamap_sync(txq->cmd_dma.dma_tag, txq->cmd_dma.dma_map,
		BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	for ( processed = 0 ; processed < tx_limit ; processed++) {
		comp_index = txq->comp_index;

		comp = &txq->comp_ring[comp_index];
		cmd_stop_index = comp->comp_index;

		if (comp->color != txq->done_color)
			break;

		batch++;

		//IONIC_TX_TRACE(txq, "[%ld] comp @ %d for desc @ %d comp->color %d done_color %d\n",
		//	stats->clean, comp_index, cmd_stop_index, comp->color, txq->done_color);

		txbuf = &txq->txbuf[cmd_stop_index];
		if (comp->status) {
			stats->comp_err++;
		}
		/* TSO last buffer points to head mbuf. */
		if (txbuf->m != NULL) {
			IONIC_TX_TRACE(txq, "took %lu tsc to free %d descs txbuf index @:%d\n",
				rdtsc() - txbuf->timestamp, batch, cmd_stop_index);
			bus_dmamap_sync(txq->buf_tag, txbuf->dma_map, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(txq->buf_tag, txbuf->dma_map);
			m_freem(txbuf->m);
			batch = 0;
		}

		txq->comp_index = (txq->comp_index + 1) % txq->num_descs;
		txq->tail_index = (cmd_stop_index + 1) % txq->num_descs;
		/* Roll over condition, flip color. */
		if (txq->comp_index == 0) {
			txq->done_color = !txq->done_color;
		}
	}

	//KASSERT(processed, ("%s tx_clean didn't processed any packet", txq->name));

	return (processed);
}

static int ionic_lif_txq_init(struct lif *lif, struct txque *txq)
{
	int err;
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.txq_init = {
			.opcode = CMD_OPCODE_TXQ_INIT,
			.I = false,
			.E = false,
			.pid = txq->pid,
#ifdef IONIC_SEPERATE_TX_INTR
			/* XXX: we can use Rx interrupt here. */
			.intr_index = txq->intr.index,
#endif
			.type = TXQ_TYPE_ETHERNET,
			.index = txq->index,
			.cos = 0,
			.ring_base = txq->cmd_ring_pa,
			.ring_size = ilog2(txq->num_descs),
		},
	};

	IONIC_QUE_INFO(txq, "qid %d pid %d index %d ring_base 0x%lx ring_size %d\n",
		ctx.comp.txq_init.qid, ctx.cmd.txq_init.pid, ctx.cmd.txq_init.index,
		ctx.cmd.txq_init.ring_base, ctx.cmd.txq_init.ring_size);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	txq->qid = ctx.comp.txq_init.qid;
	txq->qtype = ctx.comp.txq_init.qtype;

	snprintf(txq->intr.name, sizeof(txq->intr.name), "%s", txq->name);

#ifdef IONIC_SEPERATE_TX_INTR
	err = ionic_setup_tx_intr(txq);
	if (err)
		return (err);
#endif

	return (0);
}

static int ionic_lif_txqs_init(struct lif *lif)
{
	unsigned int i;
	int err;

	for (i = 0; i < lif->ntxqs; i++) {
		err = ionic_lif_txq_init(lif, lif->txqs[i]);
		if (err)
			return err;
	}

	return 0;
}

void ionic_tx_ring_doorbell(struct txque *txq, int index)
{
	struct doorbell *db;

	db = txq->lif->kern_dbpage + txq->qtype;

	ionic_ring_doorbell(db, txq->qid, index);
}

 /*******************************  RX side. ******************************/
static void ionic_rx_ring_doorbell(struct rxque *rxq, int index)
{
	struct doorbell *db;

	db = rxq->lif->kern_dbpage + rxq->qtype;

	ionic_ring_doorbell(db, rxq->qid, index);
}

static void ionic_rx_fill(struct rxque *rxq)
{
	struct rxq_desc *desc;
	struct ionic_rx_buf *rxbuf;
	int error, index;
	bool db_ring = false, posted = false;

	KASSERT(IONIC_RX_LOCK_OWNED(rxq), ("%s is not locked", rxq->name));

	/* Fill till there is only one slot left empty which is Q full. */
	for (; rxq->descs < rxq->num_descs - 1; rxq->descs++) {
		posted = true;
		index = rxq->head_index;
		rxbuf = &rxq->rxbuf[index];
		desc = &rxq->cmd_ring[index];

		bzero(desc, sizeof(*desc));
		KASSERT((rxbuf->m == NULL), ("%s: rxbuf not empty for %d", rxq->name, index));
		if ((error = ionic_rx_mbuf_alloc(rxq, index, rxq->lif->rx_mbuf_size))) {
			IONIC_QUE_ERROR(rxq, "rx_fill mbuf alloc failed for p_index :%d, error: %d\n",
				index, error);
			break;
		}

		desc->addr = rxbuf->pa_addr;
		desc->len = rxq->lif->rx_mbuf_size;
		desc->opcode = RXQ_DESC_OPCODE_SIMPLE;

		rxq->head_index = (rxq->head_index + 1) % rxq->num_descs;

		if (index % ionic_rx_stride == 0) {
			ionic_rx_ring_doorbell(rxq, index);
			db_ring = true;
		}
	 }

	/* If we haven't rung the doorbell, do it now. */
	if (!db_ring && posted)
		ionic_rx_ring_doorbell(rxq, index);

	IONIC_RX_TRACE(rxq, "head: %d tail :%d desc_posted: %d\n",
		rxq->head_index, rxq->tail_index, rxq->descs);
}

/*
 * Refill the rx queue, called during reinit.
 */
static void ionic_rx_refill(struct rxque *rxq)
{
	struct ionic_rx_buf *rxbuf;
	struct rxq_desc *desc;
	int i, count, error;

	KASSERT(IONIC_RX_LOCK_OWNED(rxq), ("%s is not locked", rxq->name));
	for (i = rxq->tail_index, count = 0; count < rxq->descs; i = (i + 1) % rxq->num_descs, count++) {
		rxbuf = &rxq->rxbuf[i];
		desc = &rxq->cmd_ring[i];

		KASSERT(rxbuf->m, ("%s: ionic_rx_refill rxbuf empty for %d", rxq->name, i));

		ionic_rx_mbuf_free(rxq, rxbuf);
		if ((error = ionic_rx_mbuf_alloc(rxq, i, rxq->lif->rx_mbuf_size))) {
			IONIC_QUE_ERROR(rxq, "mbuf alloc failed for p_index :%d, error: %d\n",
				i, error);
			break;
		}

		/* Update the descriptors with new mbuf address and length. */
		desc->addr = rxbuf->pa_addr;
		desc->len = rxq->lif->rx_mbuf_size;
	};

	IONIC_RX_TRACE(rxq, "head: %d tail :%d refilled: %d\n",
		rxq->head_index, rxq->tail_index, count);
}

/*
 * Empty Rx queue buffer, called from queue teardown.
 */
static void ionic_rx_empty(struct rxque *rxq)
{
	struct ionic_rx_buf *rxbuf;

	IONIC_RX_TRACE(rxq, "head: %d tail :%d desc_posted: %d\n",
		rxq->head_index, rxq->tail_index, rxq->descs);
	IONIC_RX_LOCK(rxq);
	for (; rxq->descs; rxq->descs--) {
		rxbuf = &rxq->rxbuf[rxq->tail_index];

		KASSERT(rxbuf->m, ("%s: ionic_rx_empty rxbuf empty for %d",
			rxq->name, rxq->tail_index));

		ionic_rx_mbuf_free(rxq, rxbuf);
		ionic_rx_destroy_map(rxq, rxbuf);

		rxq->tail_index = (rxq->tail_index + 1) % rxq->num_descs;
	};

	IONIC_RX_TRACE(rxq, "head: %d tail :%d desc_posted: %d\n",
		rxq->head_index, rxq->tail_index, rxq->descs);
	IONIC_RX_UNLOCK(rxq);
}

/*
 * Called from Rx completion paths.
 */
int ionic_rx_clean(struct rxque* rxq , int rx_limit)
{
	struct rxq_comp *comp;
	struct rxq_desc *cmd;
	struct ionic_rx_buf *rxbuf;
	int i, comp_index, cmd_index;

	KASSERT(IONIC_RX_LOCK_OWNED(rxq), ("%s is not locked", rxq->name));
	IONIC_RX_TRACE(rxq, "comp index: %d head: %d tail: %d desc_posted: %d\n",
		rxq->comp_index, rxq->head_index, rxq->tail_index, rxq->descs);

	/* Sync descriptors. */
	bus_dmamap_sync(rxq->cmd_dma.dma_tag, rxq->cmd_dma.dma_map,
			BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	/* Process Rx descriptors for the given limit or till queue is empty. */
	for (i = 0; i < rx_limit && rxq->descs; i++, rxq->descs--) {
		comp_index = rxq->comp_index;
		comp = &rxq->comp_ring[comp_index];

		if (comp->color != rxq->done_color)
			break;

		IONIC_RX_TRACE(rxq, "comp index: %d color: %d done_color: %d desc_posted: %d\n",
			comp_index, comp->color, rxq->done_color, rxq->descs);

		cmd_index = rxq->tail_index;
		rxbuf = &rxq->rxbuf[cmd_index];
		cmd = &rxq->cmd_ring[cmd_index];

		ionic_rx_input(rxq, rxbuf, comp, cmd);

		rxq->comp_index = (rxq->comp_index + 1) % rxq->num_descs;
		/* Roll over condition, flip color. */
		if (rxq->comp_index == 0) {
			rxq->done_color = !rxq->done_color;
		}

		rxq->tail_index = (rxq->tail_index + 1) % rxq->num_descs;
	}

	IONIC_RX_TRACE(rxq, "comp index: %d head: %d tail :%d desc_posted: %d processed: %d\n",
		rxq->comp_index, rxq->head_index, rxq->tail_index, rxq->descs, i);

	/* XXX: flush at the end of ISR or taskqueue handler? */
	tcp_lro_flush_all(&rxq->lro);

	if ((rxq->num_descs - rxq->descs) >= ionic_rx_fill_threshold)
		ionic_rx_fill(rxq);

	return (i);
}

static int ionic_lif_rxq_init(struct lif *lif, struct rxque *rxq)
{
	int err;
	struct lro_ctrl *lro = &rxq->lro;

	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rxq_init = {
			.opcode = CMD_OPCODE_RXQ_INIT,
			.I = false,
			.E = false,
			.pid = rxq->pid,
			.intr_index = rxq->intr.index,
			.type = RXQ_TYPE_ETHERNET,
			.index = rxq->index,
			.ring_base = rxq->cmd_ring_pa,
			.ring_size = ilog2(rxq->num_descs),
		},
	};

	IONIC_QUE_INFO(rxq, "pid %d index %d ring_base 0x%lx ring_size %d\n",
		 ctx.cmd.rxq_init.pid, ctx.cmd.rxq_init.index, ctx.cmd.rxq_init.ring_base, ctx.cmd.rxq_init.ring_size);

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	rxq->qid = ctx.comp.rxq_init.qid;
	rxq->qtype = ctx.comp.rxq_init.qtype;

#ifndef IONIC_SEPERATE_TX_INTR
	snprintf(rxq->intr.name, sizeof(rxq->intr.name), "rxtx%d", rxq->index);
#else
	snprintf(rxq->intr.name, sizeof(rxq->intr.name), "%s", rxq->name);
#endif

	err = tcp_lro_init(lro);
	if (err) {
		IONIC_QUE_WARN(rxq, "LRO setup failed, error: %d\n", err);
	}

	lro->ifp = lif->netdev;
	err = ionic_setup_rx_intr(rxq);

	return err;
}

static int ionic_lif_rxqs_init(struct lif *lif)
{
	unsigned int i;
	int err;

	for (i = 0; i < lif->nrxqs; i++) {
		err = ionic_lif_rxq_init(lif, lif->rxqs[i]);
		if (err)
			return err;
	}

	return 0;
}

static void
ionic_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct lif* lif = ifp->if_softc;

	IONIC_CORE_LOCK(lif);
	ionic_read_notify_block(lif);

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	/* link_up may have changed due to reading notify block */
	ionic_open_or_stop(lif);

	if (!lif->link_up) {
		IONIC_CORE_UNLOCK(lif);
		return;
	}

	ifp->if_baudrate = IF_Mbps(lif->link_speed);

	ifmr->ifm_status |= IFM_ACTIVE;
	ifmr->ifm_active |= IFM_FDX;

	switch(lif->link_speed) {
	case IONIC_LINK_SPEED_100G:
		/* XXX: more media type. */
		ifmr->ifm_active |= IFM_100G_CR4;
		break;

	case IONIC_LINK_SPEED_1G:
		/* Only for mgmt interface. */
		if (lif->ionic->is_mgmt_nic)
			ifmr->ifm_active |= IFM_1000_KX;
		else
			ifmr->ifm_active |= IFM_UNKNOWN;
		break;

	default:
		ifmr->ifm_active |= IFM_UNKNOWN;
		break;
	}

	IONIC_CORE_UNLOCK(lif);
}

static int
ionic_media_change(struct ifnet *ifp)
{
	struct lif* lif = ifp->if_softc;
	struct ifmedia *ifm = &lif->media;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return (EINVAL);

	if_printf(ifp, "Changing speed is not supported\n");

	return (ENODEV);
}

static void ionic_lif_set_netdev_info(struct lif *lif)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.netdev_info = {
			.opcode = CMD_OPCODE_SET_NETDEV_INFO,
		},
	};

	strlcpy(ctx.cmd.netdev_info.nd_name, lif->netdev->if_xname,
		sizeof(ctx.cmd.netdev_info.nd_name));
	strlcpy(ctx.cmd.netdev_info.dev_name, ionic_bus_info(lif->ionic),
		sizeof(ctx.cmd.netdev_info.dev_name));

	IONIC_NETDEV_INFO(lif->netdev, "SET_NETDEV_INFO %s %s\n",
		ctx.cmd.netdev_info.nd_name, ctx.cmd.netdev_info.dev_name);

	(void)ionic_adminq_post_wait(lif, &ctx);
}
/*
 * Do the rest of netdev initialisation.
 */
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

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	if (!is_zero_ether_addr(lif->dev_addr)) {
		IONIC_NETDEV_ADDR_INFO(netdev, lif->dev_addr, "deleting station MAC addr");
		ether_ifdetach(netdev);
		ionic_addr_del(lif->netdev, lif->dev_addr);
	}
	memcpy(lif->dev_addr, ctx.comp.station_mac_addr_get.addr,
	       ETHER_ADDR_LEN);

	IONIC_NETDEV_ADDR_INFO(netdev, lif->dev_addr, "adding station MAC addr");

	ionic_addr_add(lif->netdev, lif->dev_addr);

	ether_ifattach(netdev, lif->dev_addr);

	ionic_lif_set_netdev_info(lif);

	lif->max_frame_size = netdev->if_mtu + ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN + ETHER_CRC_LEN;
        netdev->if_hw_tsomax = IONIC_MAX_TSO_SIZE - (ETHER_HDR_LEN + ETHER_CRC_LEN);
        netdev->if_hw_tsomaxsegcount = IONIC_MAX_TSO_SG_ENTRIES;
        netdev->if_hw_tsomaxsegsize = IONIC_MAX_TSO_SG_SIZE;

	ifmedia_init(&lif->media, IFM_IMASK, ionic_media_change,
	    ionic_media_status);

	if (lif->ionic->is_mgmt_nic) {
		ifmedia_add(&lif->media, IFM_ETHER | IFM_1000_KX, 0, NULL);
	} else {
		/* XXX: add more media type */
		ifmedia_add(&lif->media, IFM_ETHER | IFM_100G_CR4, 0, NULL);
	}

	ifmedia_add(&lif->media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&lif->media, IFM_ETHER | IFM_AUTO);

	lif->registered = true;

	return 0;
}

int ionic_set_mac(struct net_device *netdev)
{
	struct lif *lif = netdev_priv(netdev);
	int err = 0;

	KASSERT(lif, ("lif is NULL"));
	KASSERT(IONIC_CORE_LOCK_OWNED(lif), ("%s is not locked", lif->name));

	/* if mac addr has not changed then nop */
	if (bcmp(IF_LLADDR(netdev), lif->dev_addr, ETHER_ADDR_LEN) == 0) {
		return (0);
	}

	/* add filter for new mac addr */
	err = ionic_addr_add(lif->netdev, IF_LLADDR(netdev));
	if (err) {
		IONIC_NETDEV_ERROR(lif->netdev, "Failed to add new MAC %6D, err: %d\n",
			lif->dev_addr, ":", err);
		return (err);
	}

	/* remove the filter for old mac addr */
	err = ionic_addr_del(lif->netdev, lif->dev_addr);
	if (err) {
		IONIC_NETDEV_ERROR(lif->netdev, "Failed to delete old MAC %6D, err: %d\n",
			lif->dev_addr, ":", err);
		return (err);
	}

	IONIC_NETDEV_INFO(lif->netdev, "Changed MAC from %6D to %6D\n",
		lif->dev_addr, ":", IF_LLADDR(netdev), ":");

	bcopy(IF_LLADDR(netdev), lif->dev_addr, ETHER_ADDR_LEN);

	return (err);
}

static int ionic_lif_init(struct lif *lif)
{
	struct ionic_dev *idev = &lif->ionic->idev;
	struct intr* intr = &lif->adminq->intr;
	int err;

	ionic_dev_cmd_lif_init(idev, lif->index);
	err = ionic_dev_cmd_wait_check(idev, ionic_devcmd_timeout * HZ);
	if (err)
		return err;

	err = ionic_rx_filters_init(lif);
	if (err) {
		IONIC_NETDEV_ERROR(lif->netdev, "filter init failed, error = %d\n", err);
		return err;
	}

	err = ionic_lif_adminq_init(lif);
	if (err) {
		IONIC_NETDEV_ERROR(lif->netdev, "adminq init failed, error = %d\n", err);
		goto err_out_rx_filters_deinit;
	}

	/* Enabling interrupts on adminq from here on. */
	ionic_intr_mask(intr, false);

	/* Enable notifyQ and arm it. */
	if (lif->notifyq) {
		err = ionic_lif_notifyq_init(lif, lif->notifyq);
		if (err) {
			IONIC_NETDEV_ERROR(lif->netdev, "notifyq init failed, error = %d\n", err);
			goto err_out_adminq_deinit;
		}
	}

	/* Register for VLAN events */
	lif->vlan_attach = EVENTHANDLER_REGISTER(vlan_config,
	    ionic_register_vlan, lif, EVENTHANDLER_PRI_FIRST);
	lif->vlan_detach = EVENTHANDLER_REGISTER(vlan_unconfig,
	    ionic_unregister_vlan, lif, EVENTHANDLER_PRI_FIRST);

	err = ionic_lif_txqs_init(lif);
	if (err)
		goto err_out_notifyq_deinit;

	err = ionic_lif_rxqs_init(lif);
	if (err)
		goto err_out_txqs_deinit;

	/* All queues are initilaised, setup legacy interrupts now. */
	if (ionic_enable_msix == 0) {
		err = ionic_setup_legacy_intr(lif);

		IONIC_NETDEV_ERROR(lif->netdev, "Legacy interrupt setup failed, error = %d\n", err);
		if (err)
			return err;
	}

	err = ionic_station_set(lif);
	if (err) {
		IONIC_NETDEV_ERROR(lif->netdev, "ionic_station_set failed, error = %d\n", err);
		goto err_out_rxqs_deinit;
	}

	err = ionic_lif_rss_setup(lif);
	if (err) {
		IONIC_NETDEV_ERROR(lif->netdev, "ionic_lif_rss_setup failed, error = %d\n", err);
		goto err_out_rxqs_deinit;
	}

	err = ionic_lif_stats_start(lif, STATS_DUMP_VERSION_1);
	if (err)
		goto err_out_rss_teardown;

	ionic_setup_sysctls(lif);

	lif->api_private = NULL;

	return 0;

err_out_rss_teardown:
	ionic_lif_rss_teardown(lif);
err_out_rxqs_deinit:
	ionic_lif_rxqs_deinit(lif);
err_out_txqs_deinit:
	ionic_lif_txqs_deinit(lif);
err_out_notifyq_deinit:
	ionic_lif_notifyq_deinit(lif);
err_out_adminq_deinit:
	ionic_lif_adminq_deinit(lif);
err_out_rx_filters_deinit:

	if (lif->vlan_attach != NULL)
		EVENTHANDLER_DEREGISTER(vlan_config, lif->vlan_attach);
	if (lif->vlan_detach != NULL)
		EVENTHANDLER_DEREGISTER(vlan_unconfig, lif->vlan_detach);

	return err;
}

int ionic_lifs_init(struct ionic *ionic)
{
	struct list_head *cur;
	struct lif *lif;
	int err;

	list_for_each(cur, &ionic->lifs) {
		lif = list_entry(cur, struct lif, list);
		err = ionic_lif_init(lif);
		if (err)
			return err;
	}

	return 0;
}

/*
 * Configure the NIC for required capabilities.
 */
int ionic_set_hw_features(struct lif *lif, uint32_t features)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.features = {
			.opcode = CMD_OPCODE_FEATURES,
			.set = FEATURE_SET_ETH_HW_FEATURES,
			.wanted = features,
		},
	};
	int err;

	if (features == lif->hw_features) {
		IONIC_NETDEV_INFO(lif->netdev, "features unchanged\n");
		return (0);
	}

	err = ionic_adminq_post_wait(lif, &ctx);
	if (err)
		return err;

	lif->hw_features = ctx.cmd.features.wanted &
			   ctx.comp.features.supported;

	if (lif->hw_features & ETH_HW_VLAN_TX_TAG)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_VLAN_TX_TAG\n");
	if (lif->hw_features & ETH_HW_VLAN_RX_STRIP)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_VLAN_RX_STRIP\n");
	if (lif->hw_features & ETH_HW_VLAN_RX_FILTER)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_VLAN_RX_FILTER\n");
	if (lif->hw_features & ETH_HW_RX_HASH)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_RX_HASH\n");
	if (lif->hw_features & ETH_HW_TX_SG)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TX_SG\n");
	if (lif->hw_features & ETH_HW_TX_CSUM)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TX_CSUM\n");
	if (lif->hw_features & ETH_HW_RX_CSUM)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_RX_CSUM\n");
	if (lif->hw_features & ETH_HW_TSO)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TSO\n");
	if (lif->hw_features & ETH_HW_TSO_IPV6)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TSO_IPV6\n");
	if (lif->hw_features & ETH_HW_TSO_ECN)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TSO_ECN\n");
	if (lif->hw_features & ETH_HW_TSO_GRE)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TSO_GRE\n");
	if (lif->hw_features & ETH_HW_TSO_GRE_CSUM)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TSO_GRE_CSUM\n");
	if (lif->hw_features & ETH_HW_TSO_IPXIP4)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TSO_IPXIP4\n");
	if (lif->hw_features & ETH_HW_TSO_IPXIP6)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TSO_IPXIP6\n");
	if (lif->hw_features & ETH_HW_TSO_UDP)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TSO_UDP\n");
	if (lif->hw_features & ETH_HW_TSO_UDP_CSUM)
		IONIC_NETDEV_INFO(lif->netdev, "feature ETH_HW_TSO_UDP_CSUM\n");

	return 0;
}

/*
 * Set HW & SW capabilities
 */
static int ionic_set_features(struct lif *lif, uint32_t features)
{
	int err;

	err = ionic_set_hw_features(lif, features);
	if (err)
		return err;

	err = ionic_set_os_features(lif->netdev, lif->hw_features);
	if (err)
		return err;

	return 0;
}

static int ionic_lif_register(struct lif *lif)
{
	int err;

	err = ionic_set_features(lif,
				 ETH_HW_VLAN_TX_TAG
				| ETH_HW_VLAN_RX_STRIP
				| ETH_HW_VLAN_RX_FILTER
				| ETH_HW_RX_HASH
				| ETH_HW_TX_SG
				| ETH_HW_TX_CSUM
				| ETH_HW_RX_CSUM
				| ETH_HW_TSO
				| ETH_HW_TSO_IPV6);

	if (err)
		return err;

	return 0;
}

struct lif *ionic_netdev_lif(struct net_device *netdev)
{
	if (!netdev || netdev->if_transmit != ionic_start_xmit)
		return NULL;

	return if_getsoftc(netdev);
}

int ionic_lifs_register(struct ionic *ionic)
{
	struct list_head *cur;
	struct lif *lif;
	int err;

	list_for_each(cur, &ionic->lifs) {
		lif = list_entry(cur, struct lif, list);
		err = ionic_lif_register(lif);
		if (err)
			return err;
	}

	return 0;
}

void ionic_lifs_unregister(struct ionic *ionic)
{
	struct list_head *cur;
	struct lif *lif;

	list_for_each(cur, &ionic->lifs) {
		lif = list_entry(cur, struct lif, list);
		if (lif->registered) {
			lif->registered = false;
		}
	}
}

int ionic_lifs_size(struct ionic *ionic)
{
	union identity *ident = ionic->ident;
	unsigned int nlifs = ident->dev.nlifs;
	unsigned int neqs = ident->dev.rdma_eq_qtype.qid_count;
	unsigned int nnqs = ident->dev.notify_qtype.qid_count;
	/* Tx and Rx Qs are in pair. */
	int nqs = min(ident->dev.tx_qtype.qid_count,
		      ident->dev.rx_qtype.qid_count);
	unsigned int nintrs, dev_nintrs = ident->dev.nintrs;
	int err;


	/* Use only one notifyQ. */
	if (nnqs > 1) {
		IONIC_DEV_WARN(ionic->dev, "too many notifyQs(%d)\n", nnqs);
		nnqs = 1;
	}

	/* Limit the number of queues as specified by user. */
	if (ionic_max_queues && (nqs > ionic_max_queues))
		nqs = ionic_max_queues;

	/* Don't create number of queues more than number of cores. */
	nqs = min(nqs, mp_ncpus);

try_again:
#ifdef RSS
	/*
	 * Max number of Qs can't be more than number of RSS buckets.
	 */
	if (nqs > rss_getnumbuckets()) {
		nqs = rss_getnumbuckets();
		IONIC_DEV_INFO(ionic->dev,
			"reduced number of Qs to %u based on RSS buckets\n", nqs);
	}
#endif

#ifdef IONIC_SEPERATE_TX_INTR
	/* Seperate interrupts for transmit and receive. */
	nintrs = nlifs * (nnqs + neqs + 2 * nqs + 1 /* adminq */);
#else
	/* Interrupt is shared by transmit and receive. */
	nintrs = nlifs * (nnqs + neqs + nqs + 1 /* adminq */);
#endif
	if (nintrs > dev_nintrs) {
		goto try_fewer;
	}

	if (ionic_enable_msix) {
		err = ionic_alloc_msix_vectors(ionic, nintrs);
		if (err < 0 && err != -ENOSPC) {
			return err;
		}

		if (err == -ENOSPC)
			goto try_fewer;

		if (err != nintrs) {
			ionic_free_msix_vector(ionic);
			goto try_fewer;
		}
	}

	ionic->neqs_per_lif = neqs;
	ionic->nnqs_per_lif = nnqs;
	ionic->ntxqs_per_lif = nqs;
	ionic->nrxqs_per_lif = nqs;
	ionic->nintrs = nintrs;

	dev_info(ionic->dev, "Lifs: %u, Intrs: %u/%u, NotifyQs: %u/%u, "
		"TxQs: %u/%u, RxQs: %u/%u, EQs: %u/%u\n",
		ident->dev.nlifs, nintrs, dev_nintrs,
		nnqs, ident->dev.notify_qtype.qid_count,
		nqs, ident->dev.tx_qtype.qid_count,
		nqs, ident->dev.rx_qtype.qid_count,
		neqs, ident->dev.rdma_eq_qtype.qid_count);
	dev_info(ionic->dev, "ucasts: %u, mcasts: %u, intr_coal: %u, div: %u\n",
		ident->dev.nucasts_per_lif, ident->dev.nmcasts_per_lif,
		ident->dev.intr_coal_mult, ident->dev.intr_coal_div);

	return 0;
try_fewer:
	if (neqs > 1) {
		neqs /= 2;
		goto try_again;
	}
	if (nqs > 1) {
		nqs /= 2;
		goto try_again;
	}

	return ENOSPC;
}
