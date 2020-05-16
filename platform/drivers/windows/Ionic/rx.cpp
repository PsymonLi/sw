
#include "common.h"

static bool ionic_rx_clean(struct queue *q,
                           struct desc_info *desc_info,
                           struct cq_info *cq_info,
                           void *cb_arg,
                           PNET_BUFFER_LIST *packets_to_indicate,
                           PNET_BUFFER_LIST *last_packet);

static ULONG_PTR ndis_hash_type[16] = {
    0,
    NDIS_HASH_IPV4,     // 1
    NDIS_HASH_TCP_IPV4, // 2
#if (NDIS_SUPPORT_NDIS680)
    NDIS_HASH_UDP_IPV4, // 3
#endif
    NDIS_HASH_IPV6,     // 4
    NDIS_HASH_TCP_IPV6, // 5
#if (NDIS_SUPPORT_NDIS680)
    NDIS_HASH_UDP_IPV6, // 6
#endif
    NDIS_HASH_IPV6_EX,     // 7 Not supported
    NDIS_HASH_TCP_IPV6_EX, // 8 Not supported
#if (NDIS_SUPPORT_NDIS680)
    NDIS_HASH_UDP_IPV6_EX, // 9
#endif
    0, // 10
    0, // 11
    0, // 12
    0, // 13
    0, // 14
    0  // 15
};

static const char *ndis_hash_type_str[16] = {
    "NONE",
    "NDIS_HASH_IPV4",        // 1
    "NDIS_HASH_TCP_IPV4",    // 2
    "NDIS_HASH_UDP_IPV4",    // 3 UDP_IPv4 only in 6.8 NDIS
    "NDIS_HASH_IPV6",        // 4
    "NDIS_HASH_TCP_IPV6",    // 5
    "NDIS_HASH_UDP_IPV6",    // 6  UDP_IPv6 only in 6.8 NDIS
    "NDIS_HASH_IPV6_EX",     // 7 Not supported
    "NDIS_HASH_TCP_IPV6_EX", // 8 Not supported
    "NDIS_HASH_UDP_IPV6_EX", // 9 UDP IPv6 only in 6.8 NDIS
    "UNKNOWN",               // 10
    "UNKNOWN",               // 11
    "UNKNOWN",               // 12
    "UNKNOWN",               // 13
    "UNKNOWN",               // 14
    "UNKNOWN"                // 15
};

static u8
getmappedtype(u8 color_type)
{

    u8 type = 0;
    u8 pkt_type = (color_type & IONIC_RXQ_COMP_PKT_TYPE_MASK);

    switch (pkt_type) {
    case PKT_TYPE_NON_IP:
        type = 0;
        break;
    case PKT_TYPE_IPV4:
        type = 1;
        break;

    case PKT_TYPE_IPV4_TCP:
        type = 2;
        break;

    case PKT_TYPE_IPV4_UDP:
        type = 3;
        break;

    case PKT_TYPE_IPV6:
        type = 4;
        break;

    case PKT_TYPE_IPV6_TCP:
        type = 5;
        break;

    case PKT_TYPE_IPV6_UDP:
        type = 6;
        break;
    }

    return type;
}

static inline void
ionic_rxq_post(struct queue *q, bool ring_dbell, desc_cb cb_func, void *cb_arg)
{
    ionic_q_post(q, ring_dbell, cb_func, cb_arg);
}

void
ionic_reset_rxq_pkts(struct lif *lif)
{
    struct rxq_pkt *rxq_pkt = NULL;
    ULONG sg_slots = 0;
    ULONG rxq_pkt_len = 0;

    sg_slots = (lif->rx_pkt_buffer_elementsize / PAGE_SIZE);
    ASSERT(sg_slots != 0);
    sg_slots--; // The one entry built into the rxq_pkt

	rxq_pkt_len = ALIGN_SZ( (sizeof(struct rxq_pkt) + (sg_slots * sizeof(u64))), MEMORY_ALLOCATION_ALIGNMENT);

    rxq_pkt = (struct rxq_pkt *)lif->rxq_pkt_base;

    /* First, reset the entries in the list */
	InitializeSListHead(&lif->rx_pkts_list);
#ifdef DBG
    lif->rx_pkts_free_count = 0;
#endif
    for (unsigned int i = 0; i < lif->rx_pkt_cnt; i++) {

        rxq_pkt->next.Next = NULL;

		InterlockedPushEntrySList( &lif->rx_pkts_list,
								   &rxq_pkt->next);

        rxq_pkt = (struct rxq_pkt *)((char *)rxq_pkt + rxq_pkt_len);
#ifdef DBG
        lif->rx_pkts_free_count++;
#endif
    }
}

void
ionic_release_rxq_pkts(struct lif *lif)
{
    struct rxq_pkt *rxq_pkt = NULL;

    rxq_pkt = (struct rxq_pkt *)FirstEntrySList(&lif->rx_pkts_list);

    for (unsigned int i = 0; i < lif->rx_pkt_cnt; i++) {

        if (rxq_pkt == NULL) {
            break;
        }

        if (rxq_pkt->parent_nbl != NULL) {
            NdisFreeNetBufferList(rxq_pkt->parent_nbl);
            rxq_pkt->parent_nbl = NULL;
            rxq_pkt->packet = NULL;
        }

        rxq_pkt = (struct rxq_pkt *)rxq_pkt->next.Next;
    }
}

NDIS_STATUS
ionic_alloc_rxq_pkts(struct ionic *ionic, struct lif *lif)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    NET_BUFFER_LIST_POOL_PARAMETERS pool_params;
    ULONG size;
    ULONG sg_slots = 0;
    struct rxq_pkt *rxq_pkt = NULL;
    ULONG rxq_len = 0;

    ASSERT(ionic != NULL);

    NdisZeroMemory(&pool_params, sizeof(NET_BUFFER_LIST_POOL_PARAMETERS));
    pool_params.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    pool_params.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
    pool_params.Header.Size = sizeof(pool_params);
    pool_params.ProtocolId = 0;
    pool_params.ContextSize = 0;
    pool_params.fAllocateNetBuffer = TRUE;
    pool_params.PoolTag = IONIC_RX_MEM_TAG;

    lif->rx_pkts_nbl_pool =
        NdisAllocateNetBufferListPool(ionic->adapterhandle, &pool_params);

    if (lif->rx_pkts_nbl_pool == NULL) {
        // Error out
        status = NDIS_STATUS_RESOURCES;
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR,
                  "%s Failed to alloc rq packet nbl pool adapter %p.\n",
                  __FUNCTION__, ionic));
        goto err_alloc_failed;
    }

    //
    // How many pages per descriptor
    //

    ASSERT((lif->rx_pkt_buffer_elementsize % PAGE_SIZE) == 0);
    sg_slots = (lif->rx_pkt_buffer_elementsize / PAGE_SIZE);
    ASSERT(sg_slots != 0);
    sg_slots--; // The one entry built into the descriptor opcode

    rxq_len = ALIGN_SZ( (sizeof(struct rxq_pkt) + (sg_slots * sizeof(u64))), MEMORY_ALLOCATION_ALIGNMENT);

    size = lif->rx_pkt_cnt * rxq_len;
    lif->rxq_pkt_base =
        (struct rxq_pkt *)NdisAllocateMemoryWithTagPriority_internal(
            ionic->adapterhandle, size, IONIC_RX_MEM_TAG, NormalPoolPriority);

    if (lif->rxq_pkt_base == NULL) {
        status = NDIS_STATUS_RESOURCES;
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR,
                  "%s Failed to alloc rq pkt adapter %p array size %d\n",
                  __FUNCTION__, ionic, size));
        goto err_alloc_failed;
    }

    NdisZeroMemory(lif->rxq_pkt_base, size);

    DbgTrace((TRACE_COMPONENT_MEMORY, TRACE_LEVEL_VERBOSE,
                  "%s Allocated 0x%p len %08lX\n",
                  __FUNCTION__,
                  lif->rxq_pkt_base, size));

    InitializeSListHead(&lif->rx_pkts_list);
#ifdef DBG
    lif->rx_pkts_free_count = 0;
#endif

    rxq_pkt = (struct rxq_pkt *)lif->rxq_pkt_base;

    for (unsigned int i = 0; i < lif->rx_pkt_cnt; i++) {

        rxq_pkt->next.Next = NULL;

		InterlockedPushEntrySList( &lif->rx_pkts_list,
								   &rxq_pkt->next);

        rxq_pkt = (struct rxq_pkt *)((char *)rxq_pkt + rxq_len);
#ifdef DBG
        lif->rx_pkts_free_count++;
#endif
    }

    return NDIS_STATUS_SUCCESS;

err_alloc_failed:

    return status;
}

struct rxq_pkt *
ionic_get_next_rxq_pkt(struct lif *lif)
{
    struct rxq_pkt *rxq_pkt = NULL;
#ifdef DBG
	LARGE_INTEGER start = {0,0};
	start = KeQueryPerformanceCounter( NULL);
#endif

	rxq_pkt = (struct rxq_pkt *)InterlockedPopEntrySList( &lif->rx_pkts_list);

	if( rxq_pkt != NULL) {

#ifdef DBG
		InterlockedAdd64( (LONG64 *)&lif->lif_stats->rx_pool_alloc_time,
						  (KeQueryPerformanceCounter( NULL).QuadPart - start.QuadPart));
		InterlockedIncrement( (LONG *)&lif->lif_stats->rx_pool_alloc_cnt);

		ASSERT( lif->rx_pkts_free_count > 0);
		InterlockedDecrement(&lif->rx_pkts_free_count);
#endif
	}

    return rxq_pkt;
}

void
_ionic_return_rxq_pkt(struct lif *lif, struct rxq_pkt *rxq_pkt)
{
    PMDL mdl = NET_BUFFER_FIRST_MDL(rxq_pkt->packet);

    // reset oob data per mbl in case it was used for rsc
    NET_BUFFER_LIST_COALESCED_SEG_COUNT(rxq_pkt->parent_nbl) = 0;
    NET_BUFFER_LIST_DUP_ACK_COUNT(rxq_pkt->parent_nbl) = 0;

    // readjust mdl in case it was used for rsc
    mdl->StartVa = (void *)((char *)mdl->StartVa - rxq_pkt->rsc_off);
    mdl->ByteOffset -= rxq_pkt->rsc_off;
    mdl->ByteCount -= rxq_pkt->rsc_off;
    mdl->Next = NULL;

    // reset the rsc info
    rxq_pkt->rsc_off = 0;
    rxq_pkt->rsc_last_mdl = NULL;
    rxq_pkt->rsc_next = NULL;

#ifdef DBG
	LARGE_INTEGER start = {0,0};
	
	InterlockedIncrement(&lif->rx_pkts_free_count);	
	start = KeQueryPerformanceCounter( NULL);
#endif

	rxq_pkt->q = NULL;
    rxq_pkt->next.Next = NULL;

	InterlockedPushEntrySList( &lif->rx_pkts_list,
							   &rxq_pkt->next);

#ifdef DBG
	InterlockedAdd64( (LONG64 *)&lif->lif_stats->rx_pool_free_time,
						(KeQueryPerformanceCounter( NULL).QuadPart - start.QuadPart));
	InterlockedIncrement( (LONG *)&lif->lif_stats->rx_pool_free_cnt);
#endif

    return;
}

void
ionic_return_rxq_pkt(struct lif *lif, struct rxq_pkt *rxq_pkt)
{

    while (rxq_pkt->rsc_next != NULL) {
        struct rxq_pkt *rsc_pkt = rxq_pkt->rsc_next;

        rxq_pkt->rsc_next = rsc_pkt->rsc_next;
        rsc_pkt->rsc_next = NULL;

        _ionic_return_rxq_pkt(lif, rsc_pkt);
    }

    _ionic_return_rxq_pkt(lif, rxq_pkt);
}

NDIS_STATUS
ionic_alloc_rxq_netbuffers(struct ionic *ionic,
                           struct lif *lif,
                           unsigned int size,
                           struct rxq_pkt *rxq_pkt)
{
	NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    PNET_BUFFER packet = NULL;
    PNET_BUFFER_LIST parent_nbl = NULL;
    PMDL mdl = NULL;

    ASSERT(ionic != NULL);
    ASSERT(rxq_pkt != NULL);

    mdl = NdisAllocateMdl(ionic->adapterhandle, rxq_pkt->addr, size);

    if (mdl == NULL) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR,
                  "%s Failed to Rxq alloc MDL adapter %p\n", __FUNCTION__,
                  ionic));
		status = NDIS_STATUS_RESOURCES;
        goto err_mdl_alloc_failed;
    }

    // allocate a packet descriptor
    parent_nbl =
        NdisAllocateNetBufferAndNetBufferList(lif->rx_pkts_nbl_pool, 0, 0, mdl, 0, 0);

    if (parent_nbl == NULL) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR,
                  "%s Failed to alloc rq net buffer list adapter %p\n",
                  __FUNCTION__, ionic));
		status = NDIS_STATUS_RESOURCES;
        goto err_nbl_alloc_failed;
    }

    packet = NET_BUFFER_LIST_FIRST_NB(parent_nbl);
    NET_BUFFER_FIRST_MDL(packet) = mdl;

    /* setup the rq_pkt */
    rxq_pkt->parent_nbl = parent_nbl;

    rxq_pkt->packet = packet;

    rxq_pkt->filter_info.Value = 0;
    rxq_pkt->filter_info.FilteringInfo.FilterId = 0;
    rxq_pkt->filter_info.FilteringInfo.QueueVPortInfo.QueueId = 0;
    NET_BUFFER_LIST_INFO(rxq_pkt->parent_nbl, NetBufferListFilteringInfo) =
        rxq_pkt->filter_info.Value;

    rxq_pkt->nb_shared_memory_info.NextSharedMemorySegment = NULL;
    rxq_pkt->nb_shared_memory_info.SharedMemoryFlags = 0;
    rxq_pkt->nb_shared_memory_info.SharedMemoryHandle = lif->rx_pkt_buffer_handle;
    rxq_pkt->nb_shared_memory_info.SharedMemoryLength = size;
    rxq_pkt->nb_shared_memory_info.SharedMemoryOffset = rxq_pkt->offset;

    packet->SharedMemoryInfo = &rxq_pkt->nb_shared_memory_info;

    *(struct rxq_pkt **)NET_BUFFER_MINIPORT_RESERVED(packet) = rxq_pkt;

    return NDIS_STATUS_SUCCESS;

err_nbl_alloc_failed:
    NdisFreeMdl(mdl);

err_mdl_alloc_failed:

    return status;
}

enum ionic_rsc_result {
    IONIC_RSC_MERGE,    // same flow, merged into prior packet
    IONIC_RSC_REPLACE,  // same flow, not merged
    IONIC_RSC_COLLIDE,  // same hash but not same flow
    IONIC_RSC_IGNORE,   // not eligible for rsc
};

static enum ionic_rsc_result
_ionic_do_rsc(struct qcq *qcq,
             PNET_BUFFER_LIST nbl,
             PNET_BUFFER_LIST last_nbl)
{
    struct dev_rx_ring_stats *rx_stats = qcq->rx_stats;
    PNET_BUFFER nb = NET_BUFFER_LIST_FIRST_NB(nbl);
    PNET_BUFFER last_nb = NET_BUFFER_LIST_FIRST_NB(last_nbl);
    struct rxq_pkt *pkt = NULL, *last_pkt = NULL;
    struct tcphdr *tcp = NULL, *last_tcp = NULL;
    struct ipv4hdr *ipv4 = NULL, *last_ipv4 = NULL;
    struct ipv6hdr *ipv6 = NULL, *last_ipv6 = NULL;
    struct ethhdr_vlan *eth = NULL, *last_eth = NULL;
    char *buf = NULL, *last_buf = NULL;
    ULONG off = 0, eth_hlen = 0, ip_hlen = 0, tcp_hlen = 0;
    ULONG shorten = 0, lengthen = 0;
    PMDL mdl = NULL, last_mdl = NULL;

    // likely same flow, hint to prefetch headers

    mdl = NET_BUFFER_CURRENT_MDL(nb);
    buf = (char*)MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);

    PreFetchCacheLine(PF_TEMPORAL_LEVEL_1, buf);
    PreFetchCacheLine(PF_TEMPORAL_LEVEL_1, buf + 64);

    last_mdl = NET_BUFFER_CURRENT_MDL(last_nb);
    last_buf = (char*)MmGetSystemAddressForMdlSafe(last_mdl, NormalPagePriority);

    PreFetchCacheLine(PF_TEMPORAL_LEVEL_1, last_buf);
    PreFetchCacheLine(PF_TEMPORAL_LEVEL_1, last_buf + 64);

    // must be same vlan
    if (NDIS_GET_NET_BUFFER_LIST_VLAN_ID(nbl) !=
        NDIS_GET_NET_BUFFER_LIST_VLAN_ID(last_nbl)) {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s rsc not same vlan\n", __FUNCTION__));
        return IONIC_RSC_COLLIDE;
    }

    // resolve and compare packet headers

    eth = (struct ethhdr_vlan *)buf;
    last_eth = (struct ethhdr_vlan*)last_buf;

    // compare eth src and dest
    if (memcmp(buf, last_buf, 12)) {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s rsc not same ether addr\n", __FUNCTION__));
        return IONIC_RSC_COLLIDE;
    }

    // resolve ip headers
    if (eth->h_proto == ETH_P_IPv4_LE && last_eth->h_proto == ETH_P_IPv4_LE) {
        off += ETH_HLEN;
        ipv4 = (struct ipv4hdr *)(buf + off);
        last_ipv4 = (struct ipv4hdr *)(last_buf + off);
    } else if (eth->h_proto == ETH_P_IPv6_LE && last_eth->h_proto == ETH_P_IPv6_LE) {
        off += ETH_HLEN;
        ipv6 = (struct ipv6hdr *)(buf + off);
        last_ipv6 = (struct ipv6hdr*)(last_buf + off);
    } else if (eth->h_proto == ETH_P_8021Q_LE && last_eth->h_proto == ETH_P_8021Q_LE &&
               eth->vh_proto == ETH_P_IPv4_LE && eth->vh_proto == ETH_P_IPv4_LE) {
        off += VLAN_ETH_HLEN;
        ipv4 = (struct ipv4hdr*)(buf + off);
        last_ipv4 = (struct ipv4hdr*)(last_buf + off);
    } else if (eth->h_proto == ETH_P_8021Q_LE && last_eth->h_proto == ETH_P_8021Q_LE &&
               eth->vh_proto == ETH_P_IPv6_LE && eth->vh_proto == ETH_P_IPv6_LE) {
        off += VLAN_ETH_HLEN;
        ipv6 = (struct ipv6hdr*)(buf + off);
        last_ipv6 = (struct ipv6hdr*)(last_buf + off);
    } else {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s rsc not iphdr %#lx %#lx\n", __FUNCTION__, eth->h_proto, last_eth->h_proto));
        return IONIC_RSC_IGNORE;
    }

    eth_hlen = off;

    // compare ip headers, resolve tcp headers
    if (ipv4) {
        ULONG ipv4_hlen = 4 * ipv4->ihl;
        if (ipv4_hlen < 20) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s rsc ipv4 invalid ihl %ul\n", __FUNCTION__, ipv4->ihl));
            return IONIC_RSC_IGNORE;
        }
        // next is tcp
        if (ipv4->protocol != IPPROTO_TCP || last_ipv4->protocol != IPPROTO_TCP) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s rsc ipv4 not proto tcp\n", __FUNCTION__));
            return IONIC_RSC_IGNORE;
        }
        // ver, ihl, dscp, ecn
        if (memcmp(buf + off, last_buf + off, 2)) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s rsc ipv4 not same ver, ihl, dscp, ecn\n", __FUNCTION__));
            return IONIC_RSC_COLLIDE;
        }
        // src dest addr, options
        if (memcmp(buf + off + 12, last_buf + off + 12, ipv4_hlen - 12)) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s rsc ipv4 not same addr, options\n", __FUNCTION__));
            return IONIC_RSC_COLLIDE;
        }
        // resolve tcp header
        off += ipv4_hlen;
        tcp = (struct tcphdr *)(buf + off);
        last_tcp = (struct tcphdr*)(last_buf + off);
    } else if (ipv6) {
        // next header is tcp (not supporting ipv6 extension headers)
        if (ipv6->nexthdr != IPPROTO_TCP || last_ipv6->nexthdr != IPPROTO_TCP) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s rsc ipv6 not nexthdr tcp\n", __FUNCTION__));
            return IONIC_RSC_IGNORE;
        }
        // ver, tc, flow
        if (memcmp(buf + off, last_buf + off, 4)) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s rsc ipv6 not same ver, tc, flow\n", __FUNCTION__));
            return IONIC_RSC_COLLIDE;
        }
        // src dest addr, extension headers
        if (memcmp(buf + off + 8, last_buf + off + 8, 32)) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s rsc ipv6 not same addr\n", __FUNCTION__));
            return IONIC_RSC_COLLIDE;
        }
        // resolve tcp header
        off += 40;
        tcp = (struct tcphdr*)(buf + off);
        last_tcp = (struct tcphdr*)(last_buf + off);
    } else {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s rsc not ipv4 or ipv6 BUG\n", __FUNCTION__));
        return IONIC_RSC_IGNORE;
    }

    ip_hlen = off - eth_hlen;

    // no options, nothing urgent
    if (tcp->doff != 5 || last_tcp->doff != 5 ||
        tcp->urg || tcp->urg_ptr || last_tcp->urg || last_tcp->urg_ptr) {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s rsc tcp options or urgent\n", __FUNCTION__));
        return IONIC_RSC_IGNORE;
    }

    // tcp src dst ports
    if (memcmp(buf + off, last_buf + off, 4)) {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s rsc tcp not same ports\n", __FUNCTION__));
        return IONIC_RSC_COLLIDE;
    }

    off += 20;
    tcp_hlen = 20;

    // next in seq
    if (ntohl(tcp->seq) != ntohl(last_tcp->seq) + NET_BUFFER_DATA_LENGTH(last_nb) - off) {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s rsc tcp not same seq\n", __FUNCTION__));
        return IONIC_RSC_REPLACE;
    }

    // non-decreasing ack
    if (ntohl(tcp->ack_seq) - ntohl(last_tcp->ack_seq) >= 0x80000000ul) {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s rsc tcp invalid ack\n", __FUNCTION__));
        return IONIC_RSC_REPLACE;
    }

    // avoid overflowing ip length
    if (NET_BUFFER_DATA_LENGTH(last_nb) + NET_BUFFER_DATA_LENGTH(nb) >= 0xffff) {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s rsc max packets already coalesced\n", __FUNCTION__));
        return IONIC_RSC_REPLACE;
    }

    lengthen = NET_BUFFER_DATA_LENGTH(nb) - off;

    // validate and update the IP length field
    if (last_ipv4) {
        ULONG ipv4_len = 0, last_ipv4_len = 0;

        ipv4_len = ntohs(ipv4->tot_len);
        if (NET_BUFFER_DATA_LENGTH(nb) - ipv4_len - eth_hlen > 64) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s bogus nb ipv4 len %lu packet len %lu\n",
                __FUNCTION__, ipv4_len, NET_BUFFER_DATA_LENGTH(nb)));
            return IONIC_RSC_REPLACE;
        }

        last_ipv4_len = ntohs(last_ipv4->tot_len);
        if (NET_BUFFER_DATA_LENGTH(last_nb) - last_ipv4_len - eth_hlen > 64) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s bogus nb ipv4 len %lu packet len %lu\n",
                __FUNCTION__, last_ipv4_len, NET_BUFFER_DATA_LENGTH(last_nb)));
            return IONIC_RSC_REPLACE;
        }

        // truncate any padding in the first packet
        shorten = NET_BUFFER_DATA_LENGTH(last_nb) - last_ipv4_len - eth_hlen;
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s first packet pad length %lu ipv4 len %lu packet len %lu\n",
            __FUNCTION__, shorten, last_ipv4_len, NET_BUFFER_DATA_LENGTH(last_nb)));

        // update ip len with payload to be added
        last_ipv4->tot_len = htons(last_ipv4_len + ipv4_len - ip_hlen - tcp_hlen);
    }
    else if (last_ipv6) {
        ULONG ipv6_len = 0, last_ipv6_len = 0;

        ipv6_len = ntohs(ipv6->payload_len);
        if (NET_BUFFER_DATA_LENGTH(nb) - ipv6_len - ip_hlen - eth_hlen > 64) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s bogus nb ipv6 len %lu packet len %lu\n",
                __FUNCTION__, ipv6_len, NET_BUFFER_DATA_LENGTH(nb)));
            return IONIC_RSC_REPLACE;
        }

        last_ipv6_len = ntohs(last_ipv6->payload_len);
        if (NET_BUFFER_DATA_LENGTH(last_nb) - last_ipv6_len - ip_hlen - eth_hlen > 64) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s bogus nb ipv6 len %lu packet len %lu\n",
                __FUNCTION__, last_ipv6_len, NET_BUFFER_DATA_LENGTH(last_nb)));
            return IONIC_RSC_REPLACE;
        }

        // truncate any padding in the first packet
        shorten = NET_BUFFER_DATA_LENGTH(last_nb) - last_ipv6_len - ip_hlen - eth_hlen;
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s first packet pad length %lu\n", __FUNCTION__, shorten));

        // update ip len with payload to be added
        last_ipv6->payload_len = htons(last_ipv6_len + ipv6_len - tcp_hlen);
    }

    // YES we will RSC these packets...

    DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
        "%s rsc yes we will rsc\n", __FUNCTION__));

    if (NET_BUFFER_LIST_COALESCED_SEG_COUNT(last_nbl) == 0) {
        rx_stats->rsc_bytes += NET_BUFFER_DATA_LENGTH(last_nb) - off;
        ++rx_stats->rsc_events;
        ++rx_stats->rsc_packets;
        ++NET_BUFFER_LIST_COALESCED_SEG_COUNT(last_nbl);
    }

    rx_stats->rsc_bytes += NET_BUFFER_DATA_LENGTH(nb) - off;
    ++rx_stats->rsc_packets;
    ++NET_BUFFER_LIST_COALESCED_SEG_COUNT(last_nbl);

    if (tcp->ack == last_tcp->ack) {
        ++NET_BUFFER_LIST_DUP_ACK_COUNT(last_nbl);
    } else {
        last_tcp->ack = tcp->ack;
    }

    pkt = *(struct rxq_pkt**)NET_BUFFER_MINIPORT_RESERVED(nb);
    last_pkt = *(struct rxq_pkt**)NET_BUFFER_MINIPORT_RESERVED(last_nb);

    // remember to return rsc'd pkts to the pool
    pkt->rsc_next = last_pkt->rsc_next;
    last_pkt->rsc_next = pkt;

    // append to mdl chain
    if (last_pkt->rsc_last_mdl) {
        last_mdl = last_pkt->rsc_last_mdl;
    }
    last_pkt->rsc_last_mdl = mdl;
    last_mdl->Next = mdl;

    // Truncate first packet, append payload length
    last_mdl->ByteCount -= shorten;
    NET_BUFFER_DATA_LENGTH(last_nb) += lengthen - shorten;

#ifdef IONIC_MDL_STRIP
    // strip header from second packet by advancing start of mdl

    DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
        "%s strip headers %u bytes from nb %u bytes\n",
        __FUNCTION__, off, NET_BUFFER_DATA_LENGTH(nb)));

    DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
        "%s mdl va %p offset %lu count %lu (before)\n",
        __FUNCTION__, mdl->StartVa, mdl->ByteOffset, mdl->ByteCount));

    // strip the header (not using MmAdvanceMdl)
    mdl->StartVa = (void *)((char *)mdl->StartVa + off);
    mdl->ByteOffset += off;
    mdl->ByteCount -= off;
    pkt->rsc_off = off;

    DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
        "%s mdl va %p offset %lu count %lu (after)\n",
        __FUNCTION__, mdl->StartVa, mdl->ByteOffset, mdl->ByteCount));
#else
    // copy end of first packet overwriting header of second packet

    last_mdl->ByteCount -= off;
    memcpy(mdl->StartVa, (char*)last_mdl->StartVa + last_mdl->ByteCount, off);

    // scrub that part of the first packet, for pcap debugging
    memset((char*)last_mdl->StartVa + last_mdl->ByteCount, 'A', off);
#endif

    return IONIC_RSC_MERGE;
}

static bool
ionic_do_rsc(struct qcq *qcq, PNET_BUFFER_LIST nbl, u8 pkt_type_color)
{
    struct ionic* ionic = qcq->q.lif->ionic;
    struct rsc_scratch *rsc = qcq->rsc;
    NDIS_TCP_IP_CHECKSUM_NET_BUFFER_LIST_INFO csum_info = {};
    ULONG rss_hash = NET_BUFFER_LIST_GET_HASH_VALUE(nbl);
    ULONG flow_i = 0;
    enum ionic_rsc_result result;

    // to be eligible, must be ipv4 or ipv6 tcp
    if ((pkt_type_color & PKT_TYPE_IPV4_TCP) == PKT_TYPE_IPV4_TCP) {
        if (!ionic->rscv4_enabled) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s not enabled rsc ipv4\n", __FUNCTION__));
            return false;
        }
    }
    else if ((pkt_type_color & PKT_TYPE_IPV6_TCP) == PKT_TYPE_IPV6_TCP) {
        if (!ionic->rscv6_enabled) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s not enabled rsc ipv6\n", __FUNCTION__));
            return false;
        }
    }
    else {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s not tcp\n", __FUNCTION__));
        return false;
    }

    // to be eligible, must have csum offload valid
    csum_info.Value = NET_BUFFER_LIST_INFO(nbl, TcpIpChecksumNetBufferListInfo);
    if (!csum_info.Receive.TcpChecksumSucceeded) {
        DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
            "%s not valid tcp csum offload\n", __FUNCTION__));
        return false;
    }

    // find and merge a matching flow
    for (flow_i = 0; flow_i < IONIC_MAX_RSC_FLOWS; ++flow_i) {
        if (rss_hash != rsc->hash[flow_i] ||
            rsc->packet[flow_i] == NULL) {
            continue;
        }

        // same flow hash, likely same flow, try to merge
        result = _ionic_do_rsc(qcq, nbl, rsc->packet[flow_i]);

        if (result == IONIC_RSC_MERGE) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                      "%s packet merged into flow %lu\n",
                      __FUNCTION__, flow_i));
            return true;
        }

        if (result == IONIC_RSC_REPLACE) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                      "%s packet replaces flow %lu\n",
                      __FUNCTION__, flow_i));
            rsc->packet[flow_i] = nbl;
            rsc->hash[flow_i] = rss_hash;
            return false;
        }

        if (result == IONIC_RSC_IGNORE) {
            DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
                "%s packet ignored for rsc\n", __FUNCTION__));
            return false;
        }

        // if flow hash collides, keep searching
    }

    // if not found, evict/replace other entry (simple fifo policy)

    flow_i = rsc->fifo_i;

    DbgTrace((TRACE_COMPONENT_RSC, TRACE_LEVEL_VERBOSE,
              "%s evict and replace flow %lu\n",
              __FUNCTION__, flow_i));

    rsc->packet[flow_i] = nbl;
    rsc->hash[flow_i] = rss_hash;

    rsc->fifo_i = (flow_i + 1) & (IONIC_MAX_RSC_FLOWS - 1);

    return false;
}

static bool
ionic_rx_clean(struct queue *q,
               struct desc_info *desc_info,
               struct cq_info *cq_info,
               void *cb_arg,
               PNET_BUFFER_LIST *packets_to_indicate,
               PNET_BUFFER_LIST *last_packet)
{
    struct lif *lif = q->lif;
    struct ionic *ionic = lif->ionic;
    struct rxq_comp *comp = (struct rxq_comp *)cq_info->cq_desc;
    PNET_BUFFER packet = (PNET_BUFFER)cb_arg;
    PNET_BUFFER_LIST parent_nbl;
    struct rxq_pkt *rxq_pkt;
    NDIS_TCP_IP_CHECKSUM_NET_BUFFER_LIST_INFO csum_info;
    NDIS_NET_BUFFER_LIST_8021Q_INFO vlan_pri_info;
    PMDL mdl;
    u8 rss_type = 0;
    u32 rss_hash = 0;
    struct qcq *qcq = q_to_qcq(q);
    struct dev_rx_ring_stats *rx_stats = q_to_rx_dev_stats(q);
    ULONG comp_len = comp->len;
    ULONG comp_status = comp->status;
    ULONG q_index = 0;
    ULONG frame_type = 0;
    bool indicate_nbl = false;
    void *rx_buffer = NULL;
    ULONG zero_len = 0;

    UNREFERENCED_PARAMETER(desc_info);

    DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
              "%s Processing packet for lif %d\n", __FUNCTION__,
              q->lif->index));

    mdl = NET_BUFFER_FIRST_MDL(packet);

    rxq_pkt = *(struct rxq_pkt **)NET_BUFFER_MINIPORT_RESERVED(packet);

    frame_type = get_frame_type((void *)rxq_pkt->addr);

    parent_nbl = rxq_pkt->parent_nbl;

    NET_BUFFER_LIST_STATUS(parent_nbl) = NDIS_STATUS_SUCCESS;
    parent_nbl->SourceHandle = ionic->adapterhandle;

    NET_BUFFER_LIST_NEXT_NBL(parent_nbl) = NULL;

    DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
              "%s RXQ Process Pkts adapter %p NB: %p NBL %p Len 0x%08lX Pad %s "
              "status 0x%08lX \n",
              __FUNCTION__, ionic, packet, parent_nbl, comp_len,
              comp_len < IONIC_MINIMUM_RX_PACKET_LEN ? "Yes" : "No",
              comp_status));

    if (comp_status) {
        DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_ERROR,
                  "%s RXQ Packet Error adapter %p Queue %d Status 0x%0x\n",
                  __FUNCTION__, ionic, q->index, comp_status));
        NET_BUFFER_DATA_LENGTH(packet) = 0;
        NdisAdjustMdlLength(mdl, 0);

        ++rx_stats->completion_errors;

        goto cleanup;
    }

#ifdef DBG
    for (u32 sg_index = 0; sg_index < rxq_pkt->sg_count; sg_index++) {
        DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
                    "%s Rxq packet %p sg%d PA %I64X\n", 
                    __FUNCTION__,
                    rxq_pkt,
                    sg_index,
                    rxq_pkt->phys_addr[sg_index]));
    }
#endif

    //
    // Check we meet the minimum size for the packet
    //

    if (comp_len < IONIC_MINIMUM_RX_PACKET_LEN) {
        zero_len = IONIC_MINIMUM_RX_PACKET_LEN - comp_len;
        rx_buffer = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);

        DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
                    "%s Padding packet buffer %p offset %08lX len %08lX\n", 
                    __FUNCTION__,
                    rx_buffer,
                    comp_len,
                    zero_len));

        if (rx_buffer != NULL) {
            NdisZeroMemory((void *)((char *)rx_buffer + comp_len), zero_len);
        }
        comp_len = IONIC_MINIMUM_RX_PACKET_LEN;
    }

    rxq_pkt->bytes = comp_len;

    vlan_pri_info.Value = 0;

    if (BooleanFlagOn(comp->csum_flags, IONIC_RXQ_COMP_CSUM_F_VLAN)) {

        vlan_pri_info.TagHeader.UserPriority =
            ETH_GET_VLAN_PRIORITY(comp->vlan_tci);
        vlan_pri_info.TagHeader.VlanId = ETH_GET_VLAN_ID(comp->vlan_tci);

        DbgTrace(
            (TRACE_COMPONENT_VLAN_PRI, TRACE_LEVEL_VERBOSE,
             "%s Receive Packet VLAN adapter %p VLAN %d Priority %d Lif %d\n",
             __FUNCTION__, ionic, vlan_pri_info.TagHeader.VlanId,
             vlan_pri_info.TagHeader.UserPriority, q->lif->index));

        rx_stats->vlan_stripped++;
    }

    NDIS_SET_NET_BUFFER_LIST_VLAN_ID(parent_nbl,
                                     vlan_pri_info.TagHeader.VlanId);
    NDIS_SET_NET_BUFFER_LIST_PRIORITY(parent_nbl,
                                      vlan_pri_info.TagHeader.UserPriority);

    //
    // Check if we should have a vlan tag on this queue
    //

    NET_BUFFER_LIST_INFO(rxq_pkt->parent_nbl, NetBufferListFilteringInfo) =
        rxq_pkt->filter_info.Value;

    if (BooleanFlagOn(ionic->ConfigStatus, IONIC_SRIOV_MODE)) {
        q_index = NET_BUFFER_LIST_RECEIVE_FILTER_VPORT_ID(rxq_pkt->parent_nbl);

        if (BooleanFlagOn(ionic->SriovSwitch.Ports[q_index].Flags,
                          IONIC_VPORT_STATE_VLAN_FLTR_SET) &&
            vlan_pri_info.TagHeader.VlanId == 0) {
            NET_BUFFER_LIST_RECEIVE_QUEUE_ID(parent_nbl) = 0;
        } else if (ionic->SriovSwitch.Ports[q_index].filter_cnt == 0) {
            NET_BUFFER_LIST_RECEIVE_QUEUE_ID(parent_nbl) = 0;
        }
    } else {
        q_index = NET_BUFFER_LIST_RECEIVE_QUEUE_ID(rxq_pkt->parent_nbl);

        if (BooleanFlagOn(ionic->vm_queue[q_index].Flags,
                          IONIC_QUEUE_STATE_VLAN_FLTR_SET) &&
            vlan_pri_info.TagHeader.VlanId == 0) {
            NET_BUFFER_LIST_RECEIVE_QUEUE_ID(parent_nbl) = 0;
        } else if (ionic->vm_queue[q_index].active_filter_cnt == 0) {
            NET_BUFFER_LIST_RECEIVE_QUEUE_ID(parent_nbl) = 0;
        }
    }

    DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
              "%s Processing packet %p queue id %d-%d\n", __FUNCTION__, rxq_pkt,
              rxq_pkt->filter_info.FilteringInfo.QueueVPortInfo.QueueId,
              NET_BUFFER_LIST_RECEIVE_QUEUE_ID(parent_nbl)));

    NET_BUFFER_DATA_LENGTH(packet) = comp_len;

    NdisAdjustMdlLength(mdl, comp_len);

    //NdisFlushBuffer(mdl, FALSE);

    csum_info.Value = 0;

    if ((comp->csum_flags & IONIC_RXQ_COMP_VALID_CSUM_FLAGS) != 0) {

        if (((comp->pkt_type_color & PKT_TYPE_IPV4) != 0 &&
             ionic->tcpv4_rx_state != NDIS_OFFLOAD_SET_OFF) ||
            ((comp->pkt_type_color & PKT_TYPE_IPV6) != 0 &&
             ionic->tcpv6_rx_state != NDIS_OFFLOAD_SET_OFF)) {

            if ((csum_info.Receive.TcpChecksumSucceeded = BooleanFlagOn(
                     comp->csum_flags, IONIC_RXQ_COMP_CSUM_F_TCP_OK)) != 0) {
                rx_stats->csum_tcp++;
            } else if ((csum_info.Receive.TcpChecksumFailed = BooleanFlagOn(
                            comp->csum_flags, IONIC_RXQ_COMP_CSUM_F_TCP_BAD)) !=
                       0) {
                rx_stats->csum_tcp_bad++;
            }
        }

        if (((comp->pkt_type_color & PKT_TYPE_IPV4) != 0 &&
             ionic->udpv4_rx_state != NDIS_OFFLOAD_SET_OFF) ||
            ((comp->pkt_type_color & PKT_TYPE_IPV6) != 0 &&
             ionic->udpv6_rx_state != NDIS_OFFLOAD_SET_OFF)) {
            if ((csum_info.Receive.UdpChecksumSucceeded = BooleanFlagOn(
                     comp->csum_flags, IONIC_RXQ_COMP_CSUM_F_UDP_OK)) != 0) {
                rx_stats->csum_udp++;
            } else if ((csum_info.Receive.UdpChecksumFailed = BooleanFlagOn(
                            comp->csum_flags, IONIC_RXQ_COMP_CSUM_F_UDP_BAD)) !=
                       0) {
                rx_stats->csum_udp_bad++;
            }
        }

        if ((comp->pkt_type_color & PKT_TYPE_IPV4) != 0 &&
            ionic->ipv4_rx_state != NDIS_OFFLOAD_SET_OFF) {
            if ((csum_info.Receive.IpChecksumSucceeded = BooleanFlagOn(
                     comp->csum_flags, IONIC_RXQ_COMP_CSUM_F_IP_OK)) != 0) {
                rx_stats->csum_ip++;
            } else if ((csum_info.Receive.IpChecksumFailed =
                            BooleanFlagOn(comp->csum_flags,
                                          IONIC_RXQ_COMP_CSUM_F_IP_BAD)) != 0) {
                rx_stats->csum_ip_bad++;
            }
        }

        DbgTrace((
            TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
            "%s Receive Packet adapter %p Checksum TcpChecksumFailed %s "
            "TcpChecksumSucceeded %s UdpChecksumFailed %s UdpChecksumSucceeded "
            "%s IpChecksumFailed %s IpChecksumSucceeded %s\n",
            __FUNCTION__, ionic,
            csum_info.Receive.TcpChecksumFailed ? "Yes" : "No",
            csum_info.Receive.TcpChecksumSucceeded ? "Yes" : "No",
            csum_info.Receive.UdpChecksumFailed ? "Yes" : "No",
            csum_info.Receive.UdpChecksumSucceeded ? "Yes" : "No",
            csum_info.Receive.IpChecksumFailed ? "Yes" : "No",
            csum_info.Receive.IpChecksumSucceeded ? "Yes" : "No"));
    } else {
        DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
                  "%s Receive Packet ADAPTER %p NO Checksum\n", __FUNCTION__,
                  ionic));
        rx_stats->csum_none++;
    }

    NET_BUFFER_LIST_INFO(parent_nbl, TcpIpChecksumNetBufferListInfo) =
        csum_info.Value;

    rss_type = getmappedtype((u8)comp->pkt_type_color);
    rss_hash = comp->rss_hash;

    if (rss_type != 0 && BooleanFlagOn(lif->rss_hash_flags,
                                       NDIS_RECEIVE_HASH_FLAG_ENABLE_HASH)) {
        NET_BUFFER_LIST_SET_HASH_VALUE(parent_nbl, rss_hash);
        NET_BUFFER_LIST_INFO(parent_nbl, NetBufferListHashInfo) =
            (PVOID)NDIS_RSS_HASH_INFO_FROM_TYPE_AND_FUNC(
                ndis_hash_type[rss_type], NdisHashFunctionToeplitz);

        DbgTrace((TRACE_COMPONENT_RSS_PROCESSING, TRACE_LEVEL_VERBOSE,
                  "%s Receive Packet adapter %p  rss flags 0x%08lX rss_type: 0x%08lX, rss_type_str "
                  "%s, rss_hash: 0x%08lX\n",
                  __FUNCTION__, ionic, lif->rss_hash_flags,
                  rss_type, ndis_hash_type_str[rss_type],
                  rss_hash));
    } else {
        DbgTrace((TRACE_COMPONENT_RSS_PROCESSING, TRACE_LEVEL_VERBOSE,
                  "%s Receive Packet adapter %p NO RSS rss flags 0x%08lX "
                  "rss_type: 0x%08lX, rss_hash: 0x%08lX\n",
                  __FUNCTION__, ionic, lif->rss_hash_flags,
                  rss_type, rss_hash));

        NET_BUFFER_LIST_INFO(parent_nbl, NetBufferListHashInfo) = 0;
        NET_BUFFER_LIST_SET_HASH_VALUE(parent_nbl, 0);
    }

    rx_stats->buffers_posted++;

    switch (frame_type) {

    case NDIS_PACKET_TYPE_BROADCAST: {
        rx_stats->bcast_packets++;
        rx_stats->bcast_bytes += comp_len;
        break;
    }

    case NDIS_PACKET_TYPE_MULTICAST: {
        rx_stats->mcast_packets++;
        rx_stats->mcast_bytes += comp_len;
        break;
    }

    default: {
        rx_stats->directed_packets++;
        rx_stats->directed_bytes += comp_len;
        break;
    }
    }

    if (!ionic_do_rsc(qcq, rxq_pkt->parent_nbl, comp->pkt_type_color)) {
        if (*packets_to_indicate == NULL) {
            *packets_to_indicate = rxq_pkt->parent_nbl;
            *last_packet = rxq_pkt->parent_nbl;
            ref_request(ionic);
        } else  {
            NET_BUFFER_LIST_NEXT_NBL(*last_packet) = rxq_pkt->parent_nbl;
            *last_packet = rxq_pkt->parent_nbl;
            ref_request(ionic);
        }

        indicate_nbl = true;
    }

cleanup:

    return indicate_nbl;
}

static bool
ionic_rx_service(struct cq *cq,
                 struct cq_info *cq_info,
                 PNET_BUFFER_LIST *packets_to_indicate,
                 PNET_BUFFER_LIST *last_packet,
                 bool *indicate_nbl)
{
    struct rxq_comp *comp = (struct rxq_comp *)cq_info->cq_desc;
    struct queue *q = cq->bound_q;
    struct desc_info *desc_info;

    *indicate_nbl = false;

    if (!color_match(comp->pkt_type_color, cq->done_color))
        return false;

    /* check for empty queue */
    if (q->tail->index == q->head->index)
        return false;

    desc_info = q->tail;
    if (desc_info->index != le16_to_cpu(comp->comp_index))
        return false;

    q->tail = desc_info->next;

    *indicate_nbl =
        ionic_rx_clean(q, desc_info, cq_info, desc_info->cb_arg,
                       packets_to_indicate, last_packet);

    desc_info->cb = NULL;
    desc_info->cb_arg = NULL;

    return true;
}

static u32
ionic_rx_walk_cq(struct cq *rxcq,
                 u32 indicate_limit,
                 PNET_BUFFER_LIST *packets_to_indicate)
{
    struct qcq *qcq = cq_to_qcq(rxcq);
    u32 indicate_count = 0;
    u32 work_limit = indicate_limit * IONIC_MAX_RSC_FLOWS;
    u32 work_done = 0;
    PNET_BUFFER_LIST last_packet = NULL;
    bool indicate_nbl = false;

    // reset rsc_hash to nonzero (to avoid collisions with zero)
    // and ensure there are no dangling packets in the table
    memset(qcq->rsc->packet, 0, sizeof(qcq->rsc->packet));
    memset(qcq->rsc->hash, 'A', sizeof(qcq->rsc->hash));
    qcq->rsc->fifo_i = 0;

    while (ionic_rx_service(rxcq, rxcq->tail, packets_to_indicate, &last_packet,
                            &indicate_nbl)) {
        if (rxcq->tail->last)
            rxcq->done_color = !rxcq->done_color;
        rxcq->tail = rxcq->tail->next;

        if (++work_done >= work_limit) {
            break;
        }

        if (indicate_nbl && ++indicate_count >= indicate_limit) {
            break;
        }
    }

    // leave no dangling packets in the table
    memset(qcq->rsc->packet, 0, sizeof(qcq->rsc->packet));

    return work_done;
}

void
ionic_rx_flush(struct cq *cq)
{

    struct ionic_dev *idev = &cq->lif->ionic->idev;
    u32 work_done;
    PNET_BUFFER_LIST packets_to_indicate = NULL;

    DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE,
              "%s Entry for lif %s budget %d\n", __FUNCTION__, cq->lif->name,
              cq->num_descs));

    work_done = ionic_rx_walk_cq(cq, cq->num_descs, &packets_to_indicate);

    if (work_done) {
        ionic_intr_credits(idev->intr_ctrl, cq->bound_intr->index, work_done,
                           IONIC_INTR_CRED_RESET_COALESCE);

        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE,
                  "%s Returning %d packets\n", __FUNCTION__, work_done));

        ionic_return_packet((NDIS_HANDLE)cq->lif->ionic, packets_to_indicate,
                            0);
    }

    return;
}

#define RX_RING_DOORBELL_STRIDE ((1 << 2) - 1)

NDIS_STATUS
ionic_rx_init(struct lif *lif, struct qcqst *qcqst)
{
    NDIS_STATUS ntStatus = NDIS_STATUS_SUCCESS;
    struct rxq_desc *desc;
    struct desc_info *desc_info;
    struct rxq_sg_desc *sg_desc;
    bool ring_doorbell;
    unsigned int len;
    unsigned int i;
    struct qcq *qcq = qcqst->qcq;
    struct queue *q = &qcq->q;
    struct rxq_pkt *rxq_pkt = NULL;
    ULONG sg_index = 0;
    struct rxq_sg_elem *sg_elem;
	unsigned int remaining_len = 0;

    len = lif->ionic->frame_size;

    for (i = 0; i < ionic_q_space_avail(q); i++) {

        rxq_pkt = ionic_get_next_rxq_pkt(lif);
        ASSERT(rxq_pkt != NULL);
		ASSERT(rxq_pkt->packet != NULL);

		rxq_pkt->q = q;

        desc_info = q->head;
        desc = (struct rxq_desc *)desc_info->desc;
        sg_desc = (struct rxq_sg_desc *)desc_info->sg_desc;

		remaining_len = len;

        desc->opcode = (rxq_pkt->sg_count > 1) ? (u8)RXQ_DESC_OPCODE_SG
                                                : (u8)RXQ_DESC_OPCODE_SIMPLE;

        desc->addr = cpu_to_le64(rxq_pkt->phys_addr[0]);
        desc->len = (__le16)cpu_to_le16(min( remaining_len, PAGE_SIZE));
		remaining_len -= desc->len;
           
        for (sg_index = 0; sg_index < (rxq_pkt->sg_count - 1); sg_index++) {
            sg_elem = &sg_desc->elems[sg_index];
            sg_elem->addr = cpu_to_le64(rxq_pkt->phys_addr[sg_index + 1]);
            sg_elem->len = (__le16)cpu_to_le16(min( remaining_len, PAGE_SIZE));
			remaining_len -= sg_elem->len;
        }

		ASSERT( remaining_len == 0);

        // zero filled sentinel
        sg_elem = &sg_desc->elems[rxq_pkt->sg_count];
        sg_elem->addr = 0;
        sg_elem->len = 0;

        ring_doorbell =
            ((q->head->index + 1) & RX_RING_DOORBELL_STRIDE) == 0;

        ionic_rxq_post(q, ring_doorbell, NULL, rxq_pkt->packet);
    }

    return ntStatus;
}

void
ionic_rx_fill(struct qcq *qcq)
{
    struct cq *rxcq = &qcq->cq;
    struct queue *q = rxcq->bound_q;
    struct rxq_desc *desc;
    bool ring_doorbell;
    unsigned int i;
    struct rxq_pkt *rxq_pkt = NULL;
    struct desc_info *desc_info;
    struct rxq_sg_desc *sg_desc;
    ULONG sg_index = 0;
    struct rxq_sg_elem *sg_elem;
	unsigned int remaining_len = 0;

    for (i = ionic_q_space_avail(q); i; i--) {

        rxq_pkt = ionic_get_next_rxq_pkt(q->lif);

        if (rxq_pkt == NULL) {
            DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_ERROR,
                      "%s No rxq packets available on fill\n", __FUNCTION__));
            break;
        }

		rxq_pkt->q = q;
		
		remaining_len = q->lif->ionic->frame_size;

        desc_info = q->head;
        desc = (struct rxq_desc *)desc_info->desc;
        sg_desc = (struct rxq_sg_desc *)desc_info->sg_desc;

        desc->opcode = (rxq_pkt->sg_count > 1) ? (u8)RXQ_DESC_OPCODE_SG
                                               : (u8)RXQ_DESC_OPCODE_SIMPLE;

        desc->addr = cpu_to_le64(rxq_pkt->phys_addr[0]);
		desc->len = (__le16)cpu_to_le16(min( remaining_len, PAGE_SIZE));
		remaining_len -= desc->len;

        for (sg_index = 0; sg_index < (rxq_pkt->sg_count - 1); sg_index++) {
            sg_elem = &sg_desc->elems[sg_index];
            sg_elem->addr = cpu_to_le64(rxq_pkt->phys_addr[sg_index + 1]);
			sg_elem->len = (__le16)cpu_to_le16(min( remaining_len, PAGE_SIZE));
			remaining_len -= sg_elem->len;
        }

		ASSERT( remaining_len == 0);

        // zero filled sentinel
        sg_elem = &sg_desc->elems[rxq_pkt->sg_count];
        sg_elem->addr = 0;
        sg_elem->len = 0;

        ring_doorbell = ((q->head->index + 1) & RX_RING_DOORBELL_STRIDE) == 0;

        ASSERT(rxq_pkt->packet != NULL);
        ionic_rxq_post(q, ring_doorbell, NULL, rxq_pkt->packet);
    }
#ifdef DBG
    InterlockedAdd( (LONG *)&qcq->rx_stats->pool_packet_count,
                      q->lif->rx_pkts_free_count);
    InterlockedIncrement( (LONG *)&qcq->rx_stats->pool_sample_count);
#endif
    return;
}

void
ionic_rx_filters_deinit(struct lif *lif)
{
    LIST_ENTRY *head;
    LIST_ENTRY *cur;
    LIST_ENTRY *next = NULL;
    struct rx_filter *f;
    unsigned int i;

    for (i = 0; i < RX_FILTER_HLISTS; i++) {

        head = &lif->rx_filters.by_id[i];

        if (!IsListEmpty(head)) {

            cur = head->Flink;
            do {

                f = CONTAINING_RECORD(cur, struct rx_filter, by_id);

                next = cur->Flink;

                ionic_rx_filter_free(lif, f);

                cur = next;
            } while (cur != head);
        }
    }
}

void
ionic_rx_napi(struct intr_msg *int_info,
              unsigned int budget,
              NDIS_RECEIVE_THROTTLE_PARAMETERS *receive_throttle_params)
{
    struct lif *lif = int_info->lif;
    struct qcq* qcq = int_info->qcq;
    struct cq *rxcq = &qcq->cq;
    unsigned int qi = rxcq->bound_q->index;
    struct ionic_dev *idev = &lif->ionic->idev;
    u32 rx_work_done = 0;
    u32 flags = 0;
    PNET_BUFFER_LIST packets_to_indicate = NULL;
#ifdef DBG
	LARGE_INTEGER start_time;
	LARGE_INTEGER end_time;
#endif

	if( !BooleanFlagOn( StateFlags, IONIC_STATE_TX_INTERRUPT)) {
		KeInsertQueueDpc(&lif->txqcqs[qi].qcq->tx_packet_dpc,
			NULL,
			NULL);
	}

    NdisDprAcquireSpinLock(&qcq->rx_ring_lock);

    if (budget == 0) {
        if (receive_throttle_params->MaxNblsToIndicate == 0) {
            budget = IONIC_RX_BUDGET_DEFAULT;
        }
        else {
            budget = receive_throttle_params->MaxNblsToIndicate;
        }
    }

    rx_work_done = ionic_rx_walk_cq(rxcq, budget, &packets_to_indicate);

#ifdef DBG
	start_time = KeQueryPerformanceCounter( NULL);
	InterlockedAdd64( &qcq->dpc_walk_time,
						(LONG64)(start_time.QuadPart - qcq->dpc_last_time.QuadPart));
#endif

    if (rx_work_done) {
#ifdef DBG
		InterlockedAdd( (LONG *)&qcq->rx_stats->queue_len,
						  rx_work_done);
		InterlockedIncrement( (LONG *)&qcq->rx_stats->dpc_count);
		if (rx_work_done > qcq->rx_stats->max_queue_len) {
			qcq->rx_stats->max_queue_len = rx_work_done;
		}
#endif

        ionic_rx_fill(qcq);
#ifdef DBG
		end_time = KeQueryPerformanceCounter( NULL);
		InterlockedAdd64( &qcq->dpc_fill_time,
						(LONG64)(end_time.QuadPart - start_time.QuadPart));
#endif
    }

    NdisDprReleaseSpinLock(&qcq->rx_ring_lock);

    if (rx_work_done == 0) {
#ifdef DBG
        InterlockedIncrement64(&int_info->spurious_cnt);
#endif
        DbgTrace((
            TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
            "%s Found nothing to process on lif %d ******************\n",
            __FUNCTION__, lif->index));
    }
	else {
#ifdef DBG
		start_time = KeQueryPerformanceCounter( NULL);
#endif
		ionic_rq_indicate_bufs(lif, &lif->rxqcqs[qcq->q.index], qcq, rx_work_done,
			packets_to_indicate);
#ifdef DBG
		end_time = KeQueryPerformanceCounter( NULL);
		InterlockedAdd64( &qcq->dpc_indicate_time,
						(LONG64)(end_time.QuadPart - start_time.QuadPart));
#endif
	}

	if (rx_work_done < (u32)budget) {
		flags |= IONIC_INTR_CRED_UNMASK;
		receive_throttle_params->MoreNblsPending = FALSE;
	}
	else {
		receive_throttle_params->MoreNblsPending = TRUE;
	}

    if (rx_work_done || flags) {
        flags |= IONIC_INTR_CRED_RESET_COALESCE;
        ionic_intr_credits(idev->intr_ctrl, rxcq->bound_intr->index, rx_work_done,
                           flags);
    }
#ifdef DBG
	end_time = KeQueryPerformanceCounter( NULL);
	InterlockedAdd64( &qcq->dpc_total_time,
						(LONG64)(end_time.QuadPart - qcq->dpc_end_time.QuadPart));
#endif

    return;
}

void
ionic_rq_indicate_bufs(struct lif *lif,
                       struct qcqst *qcqst,
                       struct qcq *qcq,
                       unsigned int count,
                       PNET_BUFFER_LIST packets_to_indicate)
{
    struct ionic *ionic = lif->ionic;
    ULONG flags = NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL;

    UNREFERENCED_PARAMETER(qcqst);
	UNREFERENCED_PARAMETER(qcq);

    DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
              "%s Enter Indicate RXQ Pkts adapter %p Count %d\n", __FUNCTION__,
              ionic, count));

    flags |= (NDIS_RECEIVE_FLAGS_SINGLE_QUEUE |
              NDIS_RECEIVE_FLAGS_SHARED_MEMORY_INFO_VALID);

    if (packets_to_indicate != NULL) {

        DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
                  "%s Indicating %d packets nbl %p\n", __FUNCTION__, count,
                  packets_to_indicate));

        /* inform ndis about received packets */
        NdisMIndicateReceiveNetBufferLists(
            ionic->adapterhandle, packets_to_indicate, 0, count, flags);
#ifdef DBG
        InterlockedAdd(&qcq->outstanding_rx_count, (LONG)count);
#endif
    }

    DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
              "%s Exit Indicate RXQ Pkts adapter %p\n", __FUNCTION__, ionic));
}

void
ionic_return_packet(NDIS_HANDLE adapter_context,
                    PNET_BUFFER_LIST pnetlist,
                    ULONG return_flags)
{
    struct ionic *ionic = (struct ionic *)adapter_context;
    ULONG num_nbls = 0;
    PNET_BUFFER packet;
    struct rxq_pkt *rxq_pkt;
    unsigned int len = ionic->frame_size;
    PMDL mdl;
    PNET_BUFFER_LIST nbl, nbl_next;
    struct dev_rx_ring_stats *rx_stats = NULL;

    UNREFERENCED_PARAMETER(return_flags);

    DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
              "%s Enter ionic_return_packets adapter %p nbl %p\n", __FUNCTION__,
              ionic, pnetlist));

    nbl = pnetlist;

    while (nbl) {
        // Ideally we need to check the NB and return the buffer to
        // the pool, but we are not there yet so for now just ignore

        packet = NET_BUFFER_LIST_FIRST_NB(nbl);
        rxq_pkt = *(struct rxq_pkt **)NET_BUFFER_MINIPORT_RESERVED(packet);

        nbl_next = NET_BUFFER_LIST_NEXT_NBL(nbl);
        NET_BUFFER_LIST_NEXT_NBL(nbl) = NULL;

        ASSERT(rxq_pkt != NULL);

        if (rxq_pkt != NULL) {
            struct qcq *qcq;

            qcq = q_to_qcq(rxq_pkt->q);
            rx_stats = qcq->rx_stats;
#ifdef DBG
            InterlockedDecrement(&qcq->outstanding_rx_count);
#endif
            // Dev stats
            rx_stats->completion_count++;

            // Reinitialize the packet so that it can be reused
            mdl = NET_BUFFER_FIRST_MDL(packet);
            NdisAdjustMdlLength(mdl, len);

            DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
                      "%s Return  RXQ NBL adapter %p NB %p NBL %p\n",
                      __FUNCTION__, ionic, packet, rxq_pkt->parent_nbl));
			
            ionic_return_rxq_pkt(qcq->q.lif, rxq_pkt);
        }
        // deref the adapter
        deref_request( ionic, 1);

        nbl = nbl_next;
        num_nbls++;
    }

    DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
              "%s Exit ionic_return_packets adapter %p num_nbls [%d]\n",
              __FUNCTION__, ionic, num_nbls));
}

struct rx_filter *
ionic_rx_filter_by_macvlan(struct lif *lif, u16 vid)
{
    unsigned int key = hash_32(vid, RX_FILTER_HASH_BITS);
    LIST_ENTRY *head = &lif->rx_filters.by_hash[key];
    LIST_ENTRY *current = NULL;
    struct rx_filter *f;

    if (!IsListEmpty(head)) {

        current = head->Flink;
        do {

            f = CONTAINING_RECORD(current, struct rx_filter, by_hash);

            if (le16_to_cpu(f->cmd.match) == RX_FILTER_MATCH_MAC_VLAN) {
                if (le16_to_cpu(f->cmd.mac_vlan.vlan) == vid)
                    return f;
            }

            current = current->Flink;
        } while (current != head);
    }

    return NULL;
}

struct rx_filter *
ionic_rx_filter_by_vlan(struct lif *lif, u16 vid)
{
    unsigned int key = hash_32(vid, RX_FILTER_HASH_BITS);
    LIST_ENTRY *head = &lif->rx_filters.by_hash[key];
    LIST_ENTRY *current = NULL;
    struct rx_filter *f;

    if (!IsListEmpty(head)) {

        current = head->Flink;
        do {

            f = CONTAINING_RECORD(current, struct rx_filter, by_hash);

            if (le16_to_cpu(f->cmd.match) == RX_FILTER_MATCH_VLAN) {
                if (le16_to_cpu(f->cmd.vlan.vlan) == vid)
                    return f;
            }

            current = current->Flink;
        } while (current != head);
    }

    return NULL;
}

struct rx_filter *
ionic_rx_filter_by_addr(struct lif *lif, const u8 *addr)
{

    unsigned int key = hash_32(*(u32 *)addr, RX_FILTER_HASH_BITS);
    LIST_ENTRY *head = &lif->rx_filters.by_hash[key];
    LIST_ENTRY *current = NULL;
    struct rx_filter *f;

    if (!IsListEmpty(head)) {

        current = head->Flink;
        do {

            f = CONTAINING_RECORD(current, struct rx_filter, by_hash);

            if (le16_to_cpu(f->cmd.match) == RX_FILTER_MATCH_MAC) {
                if (memcmp(addr, f->cmd.mac.addr, ETH_ALEN) == 0)
                    return f;
            }

            current = current->Flink;
        } while (current != head);
    }

    return NULL;
}

int
ionic_rx_filter_save(struct lif *lif,
                     u32 flow_id,
                     u16 rxq_index,
                     u32 hash,
                     struct ionic_admin_ctx *ctx)
{

    struct rx_filter_add_cmd *ac;
    LIST_ENTRY *head;
    struct rx_filter *f;
    unsigned int key;

    UNREFERENCED_PARAMETER(hash);

    ac = &ctx->cmd.rx_filter_add;

    switch (le16_to_cpu(ac->match)) {
    case RX_FILTER_MATCH_VLAN:
        key = le16_to_cpu(ac->vlan.vlan);
        break;
    case RX_FILTER_MATCH_MAC:
        key = *(u32 *)ac->mac.addr;
        break;
    case RX_FILTER_MATCH_MAC_VLAN:
        key = le16_to_cpu(ac->mac_vlan.vlan);
        break;
    default:
        return NDIS_STATUS_INVALID_DATA;
    }

    f = (struct rx_filter *)NdisAllocateMemoryWithTagPriority_internal(
        lif->ionic->adapterhandle, sizeof(struct rx_filter),
        IONIC_RX_FILTER_TAG, NormalPoolPriority);
    if (!f)
        return NDIS_STATUS_RESOURCES;

    NdisZeroMemory(f, sizeof(struct rx_filter));

    DbgTrace((TRACE_COMPONENT_MEMORY, TRACE_LEVEL_VERBOSE,
                  "%s Allocated 0x%p len %08lX\n",
                  __FUNCTION__,
                  f, sizeof(struct rx_filter)));
    
    f->flow_id = flow_id;
    f->filter_id = le32_to_cpu(ctx->comp.rx_filter_add.filter_id);
    f->rxq_index = rxq_index;
    memcpy(&f->cmd, ac, sizeof(f->cmd));

    // INIT_HLIST_NODE(&f->by_hash);
    // INIT_HLIST_NODE(&f->by_id);

    NdisAcquireSpinLock(&lif->rx_filters.lock);

    key = hash_32(key, RX_FILTER_HASH_BITS);
    head = &lif->rx_filters.by_hash[key];
    InsertHeadList(head, &f->by_hash);

    key = f->filter_id & RX_FILTER_HLISTS_MASK;
    head = &lif->rx_filters.by_id[key];
    InsertHeadList(head, &f->by_id);

    NdisReleaseSpinLock(&lif->rx_filters.lock);

    return NDIS_STATUS_SUCCESS;
}

void
ionic_rx_filter_free(struct lif *lif, struct rx_filter *f)
{

    RemoveEntryList(&f->by_id);
    RemoveEntryList(&f->by_hash);
    NdisFreeMemoryWithTagPriority_internal(lif->ionic->adapterhandle, f,
                                  IONIC_RX_FILTER_TAG);
}

int
ionic_rx_filters_init(struct lif *lif)
{
    unsigned int i;

    NdisAllocateSpinLock(&lif->rx_filters.lock);

    for (i = 0; i < RX_FILTER_HLISTS; i++) {
        InitializeListHead(&lif->rx_filters.by_hash[i]);
        InitializeListHead(&lif->rx_filters.by_id[i]);
    }

    return 0;
}

NDIS_STATUS
_ionic_lif_rx_mode(struct lif *lif, unsigned int rx_mode)
{
    return ionic_lif_rx_mode(lif, rx_mode);
}

NDIS_STATUS
ionic_set_rx_mode(struct lif *lif, int rx_mode)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;

    if (lif->rx_mode != (unsigned int)rx_mode)
        status = _ionic_lif_rx_mode(lif, rx_mode);

    return status;
}

void
IndicateRxQueueState(struct ionic *ionic,
                     NDIS_RECEIVE_QUEUE_ID QueueId,
                     NDIS_RECEIVE_QUEUE_OPERATIONAL_STATE State)
{
    NDIS_STATUS_INDICATION Status;
    NDIS_RECEIVE_QUEUE_STATE QueueState;

    NdisZeroMemory(&Status, sizeof(Status));
    NdisZeroMemory(&QueueState, sizeof(QueueState));

    QueueState.Header.Revision = NDIS_RECEIVE_QUEUE_STATE_REVISION_1;
    QueueState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    QueueState.Header.Size = NDIS_SIZEOF_NDIS_RECEIVE_QUEUE_STATE_REVISION_1;
    QueueState.QueueId = QueueId;
    QueueState.QueueState = State;

    Status.Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
    Status.Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
    Status.Header.Size = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
    Status.StatusCode = NDIS_STATUS_RECEIVE_QUEUE_STATE;
    Status.StatusBuffer = &QueueState;
    Status.StatusBufferSize = QueueState.Header.Size;

    NdisMIndicateStatusEx(ionic->adapterhandle, &Status);

    return;
}

NDIS_STATUS
init_rx_queue_info(struct ionic *ionic)
{

    NDIS_STATUS ntStatus = NDIS_STATUS_SUCCESS;
    ULONG ulIndex = 0;

    while (ulIndex < IONIC_MAX_VM_QUEUE_COUNT) {

        ionic->vm_queue[ulIndex].QueueLock =
            NdisAllocateRWLock(ionic->adapterhandle);

        if (ionic->vm_queue[ulIndex].QueueLock == NULL) {
            ntStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        ulIndex++;
    }

    return ntStatus;
}

NDIS_STATUS
delete_rx_queue_info(struct ionic *ionic)
{

    NDIS_STATUS ntStatus = NDIS_STATUS_SUCCESS;
    ULONG ulIndex = 0;

    while (ulIndex < IONIC_MAX_VM_QUEUE_COUNT) {

        if (ionic->vm_queue[ulIndex].QueueLock != NULL) {
            NdisFreeRWLock(ionic->vm_queue[ulIndex].QueueLock);
            ionic->vm_queue[ulIndex].QueueLock = NULL;
        }

        ulIndex++;
    }

    return ntStatus;
}

void
DumpRxPacket(struct rxq_pkt *rxq_pkt)
{

    u8 *pStream = (u8 *)rxq_pkt->addr;
    u8 *pSrcMac = NULL;
    u8 *pDestMac = NULL;
    u8 *pICmp = NULL;
    u8 *pProto = NULL;

    pDestMac = pStream;

    pSrcMac = pStream + ETH_ALEN;

    pICmp = pStream + 14;

    if (*pICmp == 0x45) {

        pProto = pICmp + 9;

        if (*pProto == 1) {

            IoPrint("%s ******* Have ping packet %p Src "
                    "%02lX:%02lX:%02lX:%02lX:%02lX:%02lX Dst "
                    "%02lX:%02lX:%02lX:%02lX:%02lX:%02lX ********\n",
                    __FUNCTION__, rxq_pkt, pSrcMac[0], pSrcMac[1], pSrcMac[2],
                    pSrcMac[3], pSrcMac[4], pSrcMac[5], pDestMac[0],
                    pDestMac[1], pDestMac[2], pDestMac[3], pDestMac[4],
                    pDestMac[5]);
        }
    }

    return;
}

ULONG
GetPktCount(struct lif *lif)
{
    ULONG cnt = 0;

    cnt = (lif->nrxqs * lif->nrxq_descs) + (lif->ionic->rx_pool_factor * lif->nrxq_descs);

    return cnt;
}

void
ionic_rx_empty(struct queue *q)
{
    struct desc_info *cur;
    struct rxq_sg_desc *sg_desc;
    struct rxq_desc *desc;

    for (cur = q->tail; cur != q->head; cur = cur->next) {
        desc = (struct rxq_desc *)cur->desc;
        desc->addr = 0;
        desc->len = 0;

        sg_desc = (struct rxq_sg_desc *)cur->sg_desc;

        cur->cb_arg = NULL;
    }
}

NDIS_STATUS
ionic_lif_rxq_pkt_init(struct lif* lif)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    u32 ring_index = 0;
    void *dma_entry_addr = NULL;
    u32 data_offset = 0;
    struct rxq_pkt *rxq_pkt = NULL;
    SCATTER_GATHER_LIST *pSGList = NULL;
    ULONG sg_count = 0;
    ULONG sg_index = 0;
    ULONG rxq_sg_index = 0;
    SCATTER_GATHER_ELEMENT *pSrcElem = NULL;
    ULONGLONG ullSrcPA = 0;
    ULONG ulSrcLen = 0;

    pSGList = (SCATTER_GATHER_LIST *)lif->rx_pkt_sgl_buffer;

    dma_entry_addr = lif->rx_pkt_buffer_base;
    data_offset = 0;
    ring_index = 0;

    ASSERT((lif->rx_pkt_buffer_elementsize % PAGE_SIZE) == 0);
    sg_count = lif->rx_pkt_buffer_elementsize / PAGE_SIZE;
    sg_index = 0;

    rxq_pkt = (struct rxq_pkt *)FirstEntrySList( &lif->rx_pkts_list);

    pSrcElem = &pSGList->Elements[0];
    ullSrcPA = pSrcElem->Address.QuadPart;
    ulSrcLen = pSrcElem->Length;

    while (ring_index < lif->rx_pkt_cnt) {

        rxq_pkt->sg_count = sg_count;
        rxq_pkt->addr = dma_entry_addr;

        rxq_sg_index = 0;
        while (rxq_sg_index < sg_count) {
            ASSERT(pSrcElem != NULL);
            ASSERT((pSrcElem->Length % PAGE_SIZE) == 0);

            rxq_pkt->phys_addr[rxq_sg_index] = ullSrcPA;

            ullSrcPA += PAGE_SIZE;
            ulSrcLen -= PAGE_SIZE;

            if (ulSrcLen == 0) {

                sg_index++;

                if (sg_index < pSGList->NumberOfElements) {

                    pSrcElem++;
                    ullSrcPA = pSrcElem->Address.QuadPart;
                    ulSrcLen = pSrcElem->Length;
                } else {

                    if (rxq_sg_index == sg_count - 1) {
                        break;
                    }

                    status = NDIS_STATUS_RESOURCES;
                    goto err_out;
                }
            }

            rxq_sg_index++;
        }

        rxq_pkt->offset = data_offset;

        status = ionic_alloc_rxq_netbuffers(lif->ionic, lif, lif->ionic->frame_size, rxq_pkt);
        if (status != NDIS_STATUS_SUCCESS) {
            DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_ERROR,
                      "%s RXQ Buffer Alloc Failed adapter %p Status 0x%0x\n",
                      __FUNCTION__, lif->ionic, status));

            goto err_out;
        }

        dma_entry_addr =
            (void *)((char *)dma_entry_addr +
                        lif->rx_pkt_buffer_elementsize);
        data_offset += lif->rx_pkt_buffer_elementsize;

        rxq_pkt = (struct rxq_pkt *)rxq_pkt->next.Next;

        ring_index++;
    }

    return NDIS_STATUS_SUCCESS;

err_out:

    return status;
}

NDIS_STATUS
ionic_alloc_rxq_pool(struct lif *lif)
{

	NDIS_STATUS		status = NDIS_STATUS_SUCCESS;
    u32 len = 0;
    struct ionic *ionic = lif->ionic;
    NDIS_SHARED_MEMORY_PARAMETERS stParams;
    ULONG ulSGListSize = 0;
    ULONG ulSGListNumElements = 0;
    PSCATTER_GATHER_LIST pSGListBuffer = NULL;

	lif->rx_pkt_cnt = GetPktCount( lif);

    len = lif->ionic->frame_size;
    len = ((len / PAGE_SIZE) + 1) * PAGE_SIZE;

    lif->rx_pkt_buffer_length =
        len * lif->rx_pkt_cnt;

    ASSERT(lif->rx_pkt_buffer_length != 0);

    NdisZeroMemory(&stParams, sizeof(NDIS_SHARED_MEMORY_PARAMETERS));

    stParams.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    stParams.Header.Revision = NDIS_SHARED_MEMORY_PARAMETERS_REVISION_2;
    stParams.Header.Size =
        NDIS_SIZEOF_SHARED_MEMORY_PARAMETERS_REVISION_2;

    stParams.Flags = 0; // NDIS_SHARED_MEM_PARAMETERS_CONTIGOUS;

    stParams.Usage = NdisSharedMemoryUsageReceive;
    stParams.PreferredNode = ionic->numa_node; //MM_ANY_NODE_OK;
    stParams.Length = lif->rx_pkt_buffer_length;

    ulSGListNumElements = BYTES_TO_PAGES(stParams.Length);

    ulSGListSize =
        sizeof(SCATTER_GATHER_LIST) +
        (sizeof(SCATTER_GATHER_ELEMENT) * ulSGListNumElements);
    pSGListBuffer =
        (PSCATTER_GATHER_LIST)NdisAllocateMemoryWithTagPriority_internal(
            ionic->adapterhandle, ulSGListSize, IONIC_SG_LIST_TAG,
            NormalPoolPriority);

    if (pSGListBuffer == NULL) {
        status = NDIS_STATUS_RESOURCES;
        goto out;
    }

    NdisZeroMemory(pSGListBuffer, ulSGListSize);

    DbgTrace((TRACE_COMPONENT_MEMORY, TRACE_LEVEL_VERBOSE,
            "%s Allocated 0x%p len %08lX\n",
            __FUNCTION__,
            pSGListBuffer,
            ulSGListSize));

    lif->rx_pkt_sgl_buffer = (void *)pSGListBuffer;

    pSGListBuffer->NumberOfElements = ulSGListNumElements;

    stParams.SGListBufferLength = ulSGListSize;
    stParams.SGListBuffer = pSGListBuffer;

    status = NdisAllocateSharedMemory(
        ionic->adapterhandle, &stParams,
        &lif->rx_pkt_buffer_alloc_handle);

    if (status != NDIS_STATUS_SUCCESS) {
        ASSERT(FALSE);
        status = NDIS_STATUS_RESOURCES;
        goto out;
    }

    DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE,
                "%s Rx VA %p Handle %p RxPool Length 0x%08lX\n",
                __FUNCTION__, stParams.VirtualAddress,
                stParams.SharedMemoryHandle,
                lif->rx_pkt_buffer_length));

    NdisZeroMemory(stParams.VirtualAddress,
                    lif->rx_pkt_buffer_length);

	ASSERT( pSGListBuffer->NumberOfElements == 1);

    lif->rx_pkt_buffer_handle = stParams.SharedMemoryHandle;
    lif->rx_pkt_buffer_base = stParams.VirtualAddress;
    lif->rx_pkt_buffer_elementsize = len;

    status = ionic_alloc_rxq_pkts(lif->ionic, lif);

    if (status != NDIS_STATUS_SUCCESS) {
        goto out;
    }

    status = ionic_lif_rxq_pkt_init(lif);
    if (status != NDIS_STATUS_SUCCESS) {
        goto out;
    }

	return NDIS_STATUS_SUCCESS;
out:

	ionic_free_rxq_pool(lif);
	
	return status;
}

void
ionic_free_rxq_pool( struct lif *lif)
{

    if (lif->rxq_pkt_base != NULL) {

        ionic_release_rxq_pkts(lif);

        NdisFreeMemoryWithTagPriority_internal(
            lif->ionic->adapterhandle, lif->rxq_pkt_base, IONIC_RX_MEM_TAG);
        lif->rxq_pkt_base = NULL;

		InitializeSListHead( &lif->rx_pkts_list);
#ifdef DBG
        lif->rx_pkts_free_count = 0;
#endif
    }

    if (lif->rx_pkts_nbl_pool != NULL) {
        NdisFreeNetBufferListPool(lif->rx_pkts_nbl_pool);
        lif->rx_pkts_nbl_pool = NULL;
    }

    if (lif->rx_pkt_buffer_base != NULL) {

        NdisFreeSharedMemory(lif->ionic->adapterhandle,
                             lif->rx_pkt_buffer_alloc_handle);

        lif->rx_pkt_buffer_alloc_handle = NULL;
        lif->rx_pkt_buffer_handle = NULL;
        lif->rx_pkt_buffer_base = NULL;
    }

    if (lif->rx_pkt_sgl_buffer != NULL) {

        NdisFreeMemoryWithTagPriority_internal(lif->ionic->adapterhandle, lif->rx_pkt_sgl_buffer,
                                      IONIC_SG_LIST_TAG);
        lif->rx_pkt_sgl_buffer = NULL;
    }

	return;
}