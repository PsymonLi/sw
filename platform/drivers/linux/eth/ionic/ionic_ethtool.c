// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2017 - 2019 Pensando Systems, Inc */

#include <linux/module.h>
#include <linux/netdevice.h>

#include "ionic.h"
#include "ionic_api.h"
#include "ionic_bus.h"
#include "ionic_lif.h"
#include "ionic_ethtool.h"
#include "ionic_stats.h"

static const char ionic_priv_flags_strings[][ETH_GSTRING_LEN] = {
#define PRIV_F_SW_DBG_STATS		BIT(0)
	"sw-dbg-stats",
};
#define PRIV_FLAGS_COUNT ARRAY_SIZE(ionic_priv_flags_strings)

static void ionic_get_stats_strings(struct lif *lif, u8 *buf)
{
	u32 i;

	for (i = 0; i < ionic_num_stats_grps; i++)
		ionic_stats_groups[i].get_strings(lif, &buf);
}

static void ionic_get_stats(struct net_device *netdev,
			    struct ethtool_stats *stats, u64 *buf)
{
	struct lif *lif;
	u32 i;

	lif = netdev_priv(netdev);

	for (i = 0; i < ionic_num_stats_grps; i++)
		ionic_stats_groups[i].get_values(lif, &buf);
}

static int ionic_get_stats_count(struct lif *lif)
{
	int i, num_stats = 0;

	for (i = 0; i < ionic_num_stats_grps; i++)
		num_stats += ionic_stats_groups[i].get_count(lif);

	return num_stats;
}

static int ionic_get_sset_count(struct net_device *netdev, int sset)
{
	struct lif *lif = netdev_priv(netdev);
	int count = 0;

	switch (sset) {
	case ETH_SS_STATS:
		count = ionic_get_stats_count(lif);
		break;
	case ETH_SS_TEST:
		break;
	case ETH_SS_PRIV_FLAGS:
		count = PRIV_FLAGS_COUNT;
		break;
	default:
		return -EOPNOTSUPP;
	}
	return count;
}

static void ionic_get_strings(struct net_device *netdev,
			      u32 sset, u8 *buf)
{
	struct lif *lif = netdev_priv(netdev);

	switch (sset) {
	case ETH_SS_STATS:
		ionic_get_stats_strings(lif, buf);
		break;
	case ETH_SS_PRIV_FLAGS:
		memcpy(buf, ionic_priv_flags_strings,
		       PRIV_FLAGS_COUNT * ETH_GSTRING_LEN);
		break;
	case ETH_SS_TEST:
		// IONIC_TODO
	default:
		netdev_err(netdev, "Invalid sset %d\n", sset);
	}
}

static void ionic_get_drvinfo(struct net_device *netdev,
			      struct ethtool_drvinfo *drvinfo)
{
	struct lif *lif = netdev_priv(netdev);
	struct ionic *ionic = lif->ionic;

	strlcpy(drvinfo->driver, DRV_NAME, sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, DRV_VERSION, sizeof(drvinfo->version));
	strlcpy(drvinfo->fw_version, ionic->ident->dev.fw_version,
		sizeof(drvinfo->fw_version));
	strlcpy(drvinfo->bus_info, ionic_bus_info(ionic),
		sizeof(drvinfo->bus_info));
}

static int ionic_get_link_ksettings(struct net_device *netdev,
				    struct ethtool_link_ksettings *ks)
{
	struct lif *lif = netdev_priv(netdev);

	ethtool_link_ksettings_zero_link_mode(ks, supported);
	ethtool_link_ksettings_zero_link_mode(ks, advertising);

	/* supported and advertised speeds */
	ethtool_link_ksettings_add_link_mode(ks, supported, 10000baseT_Full);
	ethtool_link_ksettings_add_link_mode(ks, advertising, 10000baseT_Full);

	/* supported and advertised ports */
	ethtool_link_ksettings_add_link_mode(ks, supported, FIBRE);
	ethtool_link_ksettings_add_link_mode(ks, advertising, FIBRE);

	ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);
	ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);


	ks->base.speed = lif->notifyblock->link_speed;
	ks->base.port = PORT_FIBRE;
	ks->base.autoneg = AUTONEG_ENABLE;

	if (lif->notifyblock->link_status)
		ks->base.duplex = DUPLEX_FULL;
	else
		ks->base.duplex = DUPLEX_UNKNOWN;

	return 0;
}

static int ionic_get_coalesce(struct net_device *netdev,
			      struct ethtool_coalesce *coalesce)
{
	struct lif *lif = netdev_priv(netdev);

	coalesce->tx_coalesce_usecs = lif->tx_coalesce_usecs;
	coalesce->rx_coalesce_usecs = lif->rx_coalesce_usecs;

	return 0;
}

static int ionic_set_coalesce(struct net_device *netdev,
			      struct ethtool_coalesce *coalesce)
{
	struct lif *lif = netdev_priv(netdev);
	union identity *ident = lif->ionic->ident;
	u32 tx_coal, rx_coal;
	unsigned int i;

	if (coalesce->rx_max_coalesced_frames ||
	    coalesce->rx_coalesce_usecs_irq ||
	    coalesce->rx_max_coalesced_frames_irq ||
	    coalesce->tx_max_coalesced_frames ||
	    coalesce->tx_coalesce_usecs_irq ||
	    coalesce->tx_max_coalesced_frames_irq ||
	    coalesce->stats_block_coalesce_usecs ||
	    coalesce->use_adaptive_rx_coalesce ||
	    coalesce->use_adaptive_tx_coalesce ||
	    coalesce->pkt_rate_low ||
	    coalesce->rx_coalesce_usecs_low ||
	    coalesce->rx_max_coalesced_frames_low ||
	    coalesce->tx_coalesce_usecs_low ||
	    coalesce->tx_max_coalesced_frames_low ||
	    coalesce->pkt_rate_high ||
	    coalesce->rx_coalesce_usecs_high ||
	    coalesce->rx_max_coalesced_frames_high ||
	    coalesce->tx_coalesce_usecs_high ||
	    coalesce->tx_max_coalesced_frames_high ||
	    coalesce->rate_sample_interval)
		return -EINVAL;

	if (ident->dev.intr_coal_div == 0)
		return -EIO;

	/* Convert from usecs to device units */

	tx_coal = coalesce->tx_coalesce_usecs * ident->dev.intr_coal_mult /
		  ident->dev.intr_coal_div;
	rx_coal = coalesce->rx_coalesce_usecs * ident->dev.intr_coal_mult /
		  ident->dev.intr_coal_div;

	if (tx_coal > INTR_CTRL_COAL_MAX || rx_coal > INTR_CTRL_COAL_MAX)
		return -ERANGE;

	if (coalesce->tx_coalesce_usecs != lif->tx_coalesce_usecs) {
		for (i = 0; i < lif->nxqs; i++)
			ionic_intr_coal_set(&lif->txqcqs[i]->intr, tx_coal);
		lif->tx_coalesce_usecs = coalesce->tx_coalesce_usecs;
	}

	if (coalesce->rx_coalesce_usecs != lif->rx_coalesce_usecs) {
		for (i = 0; i < lif->nxqs; i++)
			ionic_intr_coal_set(&lif->rxqcqs[i]->intr, rx_coal);
		lif->rx_coalesce_usecs = coalesce->rx_coalesce_usecs;
	}

	return 0;
}

static void ionic_get_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring)
{
	struct lif *lif = netdev_priv(netdev);

	ring->tx_max_pending = IONIC_MAX_TXRX_DESC;
	ring->tx_pending = lif->ntxq_descs;
	ring->rx_max_pending = IONIC_MAX_TXRX_DESC;
	ring->rx_pending = lif->nrxq_descs;
}

static int ionic_set_ringparam(struct net_device *netdev,
			       struct ethtool_ringparam *ring)
{
	struct lif *lif = netdev_priv(netdev);
	bool running;
	int i, j;

	if ((ring->rx_mini_pending) || (ring->rx_jumbo_pending)) {
		netdev_info(netdev, "Changing jumbo or mini descriptors not supported\n");
		return -EINVAL;
	}

	i = ring->tx_pending & (ring->tx_pending - 1);
	j = ring->rx_pending & (ring->rx_pending - 1);
	if (i || j) {
		netdev_info(netdev, "Descriptor count must be a power of 2\n");
		return -EINVAL;
	}

	if (ring->tx_pending > IONIC_MAX_TXRX_DESC ||
	    ring->tx_pending < IONIC_MIN_TXRX_DESC ||
	    ring->rx_pending > IONIC_MAX_TXRX_DESC ||
	    ring->rx_pending < IONIC_MIN_TXRX_DESC) {
		netdev_info(netdev, "Descriptors count must be in the range [%d-%d]\n",
			    IONIC_MIN_TXRX_DESC, IONIC_MAX_TXRX_DESC);
		return -EINVAL;
	}

	/* if nothing to do return success */
	if ((ring->tx_pending == lif->ntxq_descs) &&
	    (ring->rx_pending == lif->nrxq_descs))
		return 0;

	running = test_bit(LIF_UP, lif->state);
	if (running)
		ionic_stop(netdev);

	lif->ntxq_descs = ring->tx_pending;
	lif->nrxq_descs = ring->rx_pending;

	if (running)
		ionic_open(netdev);

	return 0;
}

static void ionic_get_channels(struct net_device *netdev,
			       struct ethtool_channels *ch)
{
	struct lif *lif = netdev_priv(netdev);

	/* report maximum channels */
	ch->max_combined = lif->ionic->ntxqs_per_lif;

	/* report current channels */
	ch->combined_count = lif->nxqs;
}

static int ionic_get_rxnfc(struct net_device *netdev,
			   struct ethtool_rxnfc *info, u32 *rules)
{
	struct lif *lif = netdev_priv(netdev);
	int err = 0;

	switch (info->cmd) {
	case ETHTOOL_GRXRINGS:
		info->data = lif->nxqs;
		break;
	default:
		netdev_err(netdev, "Command parameter %d is not supported\n",
			   info->cmd);
		err = -EOPNOTSUPP;
	}

	return err;
}

static u32 ionic_get_rxfh_indir_size(struct net_device *netdev)
{
	return RSS_IND_TBL_SIZE;
}

static u32 ionic_get_rxfh_key_size(struct net_device *netdev)
{
	return RSS_HASH_KEY_SIZE;
}

#ifdef HAVE_RXFH_HASHFUNC
static int ionic_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key,
			  u8 *hfunc)
#else
static int ionic_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
#endif
{
	struct lif *lif = netdev_priv(netdev);
	unsigned int i;

	if (indir)
		for (i = 0; i < RSS_IND_TBL_SIZE; i++)
			indir[i] = lif->rss_ind_tbl[i];

	if (key)
		memcpy(key, lif->rss_hash_key, RSS_HASH_KEY_SIZE);

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;
#endif

	return 0;
}

int ionic_rss_ind_tbl_set(struct lif *lif, const u32 *indir)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rss_indir_set = {
			.opcode = CMD_OPCODE_RSS_INDIR_SET,
			.addr = lif->rss_ind_tbl_pa,
		},
	};
	unsigned int i;

	if (indir)
		for (i = 0; i < RSS_IND_TBL_SIZE; i++)
			lif->rss_ind_tbl[i] = indir[i];

	netdev_info(lif->netdev, "rss_ind_tbl_set\n");

	return ionic_adminq_post_wait(lif, &ctx);
}

int ionic_rss_hash_key_set(struct lif *lif, const u8 *key)
{
	struct ionic_admin_ctx ctx = {
		.work = COMPLETION_INITIALIZER_ONSTACK(ctx.work),
		.cmd.rss_hash_set = {
			.opcode = CMD_OPCODE_RSS_HASH_SET,
			.types = RSS_TYPE_IPV4
			       | RSS_TYPE_IPV4_TCP
			       | RSS_TYPE_IPV4_UDP
			       | RSS_TYPE_IPV6
			       | RSS_TYPE_IPV6_TCP
			       | RSS_TYPE_IPV6_UDP
			       | RSS_TYPE_IPV6_EX
			       | RSS_TYPE_IPV6_TCP_EX
			       | RSS_TYPE_IPV6_UDP_EX,
		},
	};

	memcpy(lif->rss_hash_key, key, RSS_HASH_KEY_SIZE);

	memcpy(ctx.cmd.rss_hash_set.key, lif->rss_hash_key,
	       RSS_HASH_KEY_SIZE);

	return ionic_adminq_post_wait(lif, &ctx);
}

#ifdef HAVE_RXFH_HASHFUNC
static int ionic_set_rxfh(struct net_device *netdev, const u32 *indir,
			  const u8 *key, const u8 hfunc)
#else
static int ionic_set_rxfh(struct net_device *netdev, const u32 *indir,
			  const u8 *key)
#endif
{
	struct lif *lif = netdev_priv(netdev);
	int err;

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc != ETH_RSS_HASH_NO_CHANGE && hfunc != ETH_RSS_HASH_TOP)
		return -EOPNOTSUPP;
#endif

	if (indir) {
		err = ionic_rss_ind_tbl_set(lif, indir);
		if (err)
			return err;
	}

	if (key) {
		err = ionic_rss_hash_key_set(lif, key);
		if (err)
			return err;
	}

	return 0;
}

static u32 ionic_get_priv_flags(struct net_device *netdev)
{
	u32 priv_flags = 0;
#ifdef IONIC_DEBUG_STATS
	struct lif *lif = netdev_priv(netdev);

	if (test_bit(LIF_SW_DEBUG_STATS, lif->state))
		priv_flags |= PRIV_F_SW_DBG_STATS;
#endif

	return priv_flags;
}

static int ionic_set_priv_flags(struct net_device *netdev, u32 priv_flags)
{
	struct lif *lif = netdev_priv(netdev);
	u32 flags = lif->flags;

#ifdef IONIC_DEBUG_STATS
	clear_bit(LIF_SW_DEBUG_STATS, lif->state);
	if (priv_flags & PRIV_F_SW_DBG_STATS)
		set_bit(LIF_SW_DEBUG_STATS, lif->state);
#endif

	if (flags != lif->flags)
		lif->flags = flags;

	return 0;
}

static int ionic_set_tunable(struct net_device *dev,
	const struct ethtool_tunable *tuna, const void *data)
{
	struct lif *lif = netdev_priv(dev);
	u32 val;

	switch (tuna->id) {
	case ETHTOOL_RX_COPYBREAK:
		val = *(u32 *)data;
		lif->rx_copybreak = *(u32 *)data;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int ionic_get_tunable(struct net_device *dev,
	const struct ethtool_tunable *tuna, void *data)
{
	struct lif *lif = netdev_priv(dev);

	switch (tuna->id) {
	case ETHTOOL_RX_COPYBREAK:
		*(u32 *)data = lif->rx_copybreak;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static const struct ethtool_ops ionic_ethtool_ops = {
	.get_drvinfo		= ionic_get_drvinfo,
	.get_link		= ethtool_op_get_link,
	.get_link_ksettings	= ionic_get_link_ksettings,
	.get_coalesce		= ionic_get_coalesce,
	.set_coalesce		= ionic_set_coalesce,
	.get_ringparam		= ionic_get_ringparam,
	.set_ringparam		= ionic_set_ringparam,
	.get_channels		= ionic_get_channels,
	.get_strings		= ionic_get_strings,
	.get_ethtool_stats	= ionic_get_stats,
	.get_sset_count		= ionic_get_sset_count,
	.get_rxnfc		= ionic_get_rxnfc,
	.get_rxfh_indir_size	= ionic_get_rxfh_indir_size,
	.get_rxfh_key_size	= ionic_get_rxfh_key_size,
	.get_rxfh		= ionic_get_rxfh,
	.set_rxfh		= ionic_set_rxfh,
	.get_priv_flags		= ionic_get_priv_flags,
	.set_priv_flags		= ionic_set_priv_flags,
	.get_tunable		= ionic_get_tunable,
	.set_tunable		= ionic_set_tunable,
};

void ionic_ethtool_set_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &ionic_ethtool_ops;
}
