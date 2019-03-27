/*
 * Copyright (c) 2019 Pensando Systems. All rights reserved.
 *
 * This software is provided to you under the terms of the GNU
 * General Public License (GPL) Version 2.0, available from the file
 * [ionic_txrx.c] in the main directory of this source tree.
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

#include "ionic.h"
#include "ionic_lif.h"

static void ionic_rx_clean(struct queue *q, struct desc_info *desc_info,
                           struct cq_info *cq_info, void *cb_arg);

static void ionic_rx_recycle(struct queue *q, struct desc_info *desc_info,
                             vmk_PktHandle *pkt)
{
        struct rxq_desc *old = desc_info->desc;
        struct rxq_desc *new = q->head->desc;

        new->addr = old->addr;
        new->len = old->len;

        ionic_q_post(q, VMK_TRUE, ionic_rx_clean, pkt);
}

static void ionic_rx_clean(struct queue *q, struct desc_info *desc_info,
                           struct cq_info *cq_info, void *cb_arg)
{
        VMK_ReturnStatus status;
        struct ionic_en_priv_data *priv_data;
        struct ionic_en_uplink_handle *uplink_handle;
        struct rxq_desc *desc = desc_info->desc;
        struct rxq_comp *comp = cq_info->cq_desc;
        vmk_PktHandle *pkt = cb_arg;
        struct qcq *qcq = q_to_qcq(q);
        struct rx_stats *stats = q_to_rx_stats(q);
        dma_addr_t dma_addr;
        vmk_PktRssType hash_type = VMK_PKT_RSS_TYPE_NONE;
#ifdef HAPS
        vmk_uint32 mtu = 0;
//        __sum16 csum;
#endif

        if (comp->status) {
                ionic_rx_recycle(q, desc_info, pkt);
                return;
        }

        uplink_handle = q->lif->uplink_handle;

#ifdef HAPS
        IONIC_EN_SHARED_AREA_BEGIN_READ(uplink_handle);
        mtu = uplink_handle->uplink_shared_data.mtu;
        IONIC_EN_SHARED_AREA_END_READ(uplink_handle);

        if (comp->len > mtu + IONIC_EN_ETH_HLEN + IONIC_EN_VLAN_HLEN) {
                ionic_err("RX PKT TOO LARGE!  comp->len %d\n", comp->len);
                ionic_rx_recycle(q, desc_info, pkt);
                return;
        }
#endif
        
        // TODO add copybreak to avoid allocating a new skb for small receive
        dma_addr = (dma_addr_t)desc->addr;

        priv_data = IONIC_CONTAINER_OF(q->lif->ionic,
                                       struct ionic_en_priv_data,
                                       ionic);

        status = ionic_dma_unmap(priv_data->dma_engine_streaming,
                                 VMK_DMA_DIRECTION_TO_MEMORY,
                                 desc->len,
                                 dma_addr);

        if (status != VMK_OK) {
                ionic_err("rx clean, ionic_dma_unmap() failed");
                VMK_ASSERT(0);
                return;
        }

        stats->pkts++;
        stats->bytes += comp->len;

        vmk_PktFrameLenSet(pkt, comp->len);

#ifdef HAPS
        //csum = ip_compute_csum(skb->data, skb->len);
#endif
//        skb_record_rx_queue(skb, q->index);

        if (priv_data->uplink_handle.hw_features & ETH_HW_RX_HASH) {
                switch (comp->pkt_type) {
                case PKT_TYPE_IPV4:
                        hash_type = VMK_PKT_RSS_TYPE_IPV4;
                        break;
                case PKT_TYPE_IPV6:
                        hash_type = VMK_PKT_RSS_TYPE_IPV6;
                        break;
                case PKT_TYPE_IPV4_TCP:
                        hash_type = VMK_PKT_RSS_TYPE_IPV4_TCP;
                        break;
                case PKT_TYPE_IPV6_TCP:
                        hash_type = VMK_PKT_RSS_TYPE_IPV6_TCP;
                        break;
                case PKT_TYPE_IPV4_UDP:
                        hash_type = VMK_PKT_RSS_TYPE_IPV4_UDP;
                        break;
                case PKT_TYPE_IPV6_UDP:
                        hash_type = VMK_PKT_RSS_TYPE_IPV6_UDP;
                        break;
                default:
                        break;
                }

                if (VMK_LIKELY(hash_type)) {
                        vmk_PktRssHashSet(pkt, comp->rss_hash, hash_type);
                }
        }

        if ((uplink_handle->hw_features & ETH_HW_RX_CSUM) &&
            comp->csum_calc) {
                if (comp->csum_tcp_bad ||
                    comp->csum_udp_bad ||
                    comp->csum_ip_bad) {
                       stats->csum_err++;
                } else {
                        vmk_PktSetCsumVfd(pkt);
                        stats->csum_complete++;
                }
        } else {
                stats->no_csum++;
        }

        if (uplink_handle->hw_features & ETH_HW_VLAN_RX_STRIP) {
                if (comp->V) {
                        vmk_PktVlanIDSet(pkt, comp->vlan_tci & IONIC_VLAN_MASK);
                        vmk_PktPrioritySet(pkt,
                                           (comp->vlan_tci >> IONIC_VLAN_PRIO_SHIFT) &
                                           IONIC_VLAN_PRIO_MASK);
                }
        }

        status = vmk_NetPollRxPktQueue(qcq->netpoll,
                                       pkt);
        if (status != VMK_OK) {
                ionic_err("vmk_NetPollRxPktQueue() failed, status: %s",
                          vmk_StatusToString(status));
                ionic_en_pkt_release(pkt, NULL);
                VMK_ASSERT(0);
        }
}

static bool
ionic_rx_service(struct cq *cq,
                 struct cq_info *cq_info,
                 void *cb_arg)
{
        struct rxq_comp *comp = cq_info->cq_desc;

        if (comp->color != cq->done_color)
                return VMK_FALSE;

        ionic_q_service(cq->bound_q, cq_info, comp->comp_index);

        return VMK_TRUE;
}

static vmk_PktHandle * 
ionic_rx_pkt_alloc(struct queue *q,
                   unsigned int len,
                   dma_addr_t *dma_addr)
{
        VMK_ReturnStatus status;
        struct ionic_en_priv_data *priv_data;
        struct lif *lif = q->lif;
        struct rx_stats *stats = q_to_rx_stats(q);
        const vmk_SgElem *sge;
        vmk_PktHandle *new_pkt;

        priv_data = IONIC_CONTAINER_OF(lif->ionic,
                                       struct ionic_en_priv_data,
                                       ionic);

        status = vmk_PktAllocForDMAEngine(len,
                                          priv_data->dma_engine_streaming,
                                          &new_pkt);
        if (VMK_UNLIKELY(status != VMK_OK)) {
                stats->alloc_err++;
                return NULL;
        }

        sge = vmk_PktSgElemGet(new_pkt, 0);
        VMK_ASSERT(sge);

        *dma_addr = ionic_dma_map_ma(priv_data->dma_engine_streaming,
                                     VMK_DMA_DIRECTION_TO_MEMORY,
                                     sge->addr,
                                     sge->length);
        if (VMK_UNLIKELY(!(*dma_addr))) {
                ionic_err("ionic_dma_map_ma() failed");
                stats->dma_map_err++;
                ionic_en_pkt_release(new_pkt, NULL);
                new_pkt = NULL;
        }

        return new_pkt;
}

static void
ionic_rx_pkt_free(struct queue *q,
                  vmk_PktHandle*pkt,
                  unsigned int len,
                  dma_addr_t dma_addr)
{
        VMK_ReturnStatus status;
        struct ionic_en_priv_data *priv_data;
        struct lif *lif = q->lif;

        priv_data = IONIC_CONTAINER_OF(lif->ionic,
                                       struct ionic_en_priv_data,
                                       ionic);        

        status = ionic_dma_unmap(priv_data->dma_engine_streaming,
                                 VMK_DMA_DIRECTION_TO_MEMORY,
                                 len,
                                 dma_addr);
        if (VMK_LIKELY(status == VMK_OK)) {
                ionic_en_pkt_release(pkt, NULL);
        } else {
                ionic_err("ionic_dma_unmap() failed, status: %s",
                          vmk_StatusToString(status));
        }
}

#define RX_RING_DOORBELL_STRIDE         ((1 << 2) - 1)

void
ionic_rx_fill(struct queue *q)
{
        struct ionic_en_uplink_handle *uplink_handle;
        struct qcq *qcq = q_to_qcq(q);
        struct rxq_desc *desc;
        vmk_PktHandle *pkt;
        unsigned int i, len;
        dma_addr_t dma_addr = 0;
        bool ring_doorbell;
        struct ionic_en_rx_ring *rx_ring;
        vmk_uint32 num_q_avail;

        uplink_handle = q->lif->uplink_handle;

        IONIC_EN_SHARED_AREA_BEGIN_READ(uplink_handle);
        len = uplink_handle->uplink_shared_data.mtu +
                             IONIC_EN_ETH_HLEN +
                             IONIC_EN_VLAN_HLEN;
        IONIC_EN_SHARED_AREA_END_READ(uplink_handle);

        rx_ring = &uplink_handle->rx_rings[qcq->ring_idx];

        num_q_avail = ionic_q_space_avail(q);

        for (i = num_q_avail; i; i--) {

                pkt = ionic_rx_pkt_alloc(q, len, &dma_addr);
                if (VMK_UNLIKELY(!pkt)) {
                        break;
                }

                desc = q->head->desc;
                desc->addr = dma_addr;
                desc->len = len;
                desc->opcode = RXQ_DESC_OPCODE_SIMPLE;

                ring_doorbell = ((q->head->index + 1) &
                                RX_RING_DOORBELL_STRIDE) == 0;

                ionic_q_post(q, ring_doorbell, ionic_rx_clean, pkt);
        }
}

void ionic_rx_refill(struct queue *q)
{
        struct ionic_en_uplink_handle *uplink_handle;
        struct desc_info *cur = q->tail;
        struct rxq_desc *desc;
        vmk_PktHandle *pkt;
        unsigned int len;
        dma_addr_t dma_addr = 0;

        uplink_handle = q->lif->uplink_handle;

        IONIC_EN_SHARED_AREA_BEGIN_READ(uplink_handle);
        len = uplink_handle->uplink_shared_data.mtu +
                             IONIC_EN_VLAN_HLEN +
                             IONIC_EN_ETH_HLEN;
        IONIC_EN_SHARED_AREA_END_READ(uplink_handle);

        while (cur != q->head) {

                desc = cur->desc;

                pkt = ionic_rx_pkt_alloc(q, len, &dma_addr);
                if (VMK_UNLIKELY(!pkt)) {
                        ionic_warn("Queue index: %d, ionic_rx_pkt_alloc()"
                                   "failed", q->index);
                        break;
                }

                ionic_rx_pkt_free(q, cur->cb_arg, desc->len, desc->addr);

                cur->cb_arg = pkt;
                desc->addr = dma_addr;
                desc->len = len;

                cur = cur->next;
        }
}

void ionic_rx_empty(struct queue *q)
{
        struct desc_info *cur = q->tail;
        struct rxq_desc *desc;

        while (cur != q->head) {
                desc = cur->desc;
                ionic_rx_pkt_free(q, cur->cb_arg, desc->len, desc->addr);
                cur = cur->next;
        }
}

void ionic_rx_flush(struct cq *cq)
{
        unsigned int work_done;

        work_done = ionic_cq_service(cq, -1, ionic_rx_service, NULL);

        if (work_done > 0)
                ionic_intr_return_credits(cq->bound_intr, work_done, 0, VMK_TRUE);
}

static dma_addr_t ionic_tx_map_single(struct queue *q, vmk_VA data, size_t len)
{
        struct ionic_en_priv_data *priv_data;
//        struct device *dev = q->lif->ionic->dev;
        struct tx_stats *stats = q_to_tx_stats(q);
        dma_addr_t dma_addr;

        priv_data = IONIC_CONTAINER_OF(q->lif->ionic,
                                       struct ionic_en_priv_data,
                                       ionic);


        dma_addr = ionic_dma_map_va(priv_data->dma_engine_streaming,
                                    VMK_DMA_DIRECTION_FROM_MEMORY,
                                    data,
                                    len);
        if (VMK_UNLIKELY(!dma_addr)) {
                ionic_err("ionic_dma_map_va() failed,  DMA single map "
                          "failed on %s!\n",
                          q->name);
                stats->dma_map_err++;
                return 0;
        }
        return dma_addr;
}

static dma_addr_t ionic_tx_map_frag(struct queue *q,
                                    const vmk_SgElem *sg_elem,
                                    size_t offset,
                                    size_t len)
{
        struct ionic_en_priv_data *priv_data;
        struct tx_stats *stats = q_to_tx_stats(q);
        dma_addr_t dma_addr;

        priv_data = IONIC_CONTAINER_OF(q->lif->ionic,
                                       struct ionic_en_priv_data,
                                       ionic);

        dma_addr = ionic_dma_map_ma(priv_data->dma_engine_streaming,
                                    VMK_DMA_DIRECTION_FROM_MEMORY,
                                    (vmk_MA) sg_elem->addr + offset,
                                    len);
        if (VMK_UNLIKELY(!dma_addr)) {
                ionic_err("ionic_dma_map_ma() failed,  DMA frag map "
                          "failed on %s!\n",
                          q->name);
                stats->dma_map_err++;
                return 0;
        }

        return dma_addr;
}

static VMK_ReturnStatus
ionic_tx_unmap_frag(struct queue *q,
                    vmk_uint32 len,
                    vmk_IOA ioa)
{
        VMK_ReturnStatus status;
        struct ionic_en_priv_data *priv_data;

        priv_data = IONIC_CONTAINER_OF(q->lif->ionic,
                                       struct ionic_en_priv_data,
                                       ionic);

        status = ionic_dma_unmap(priv_data->dma_engine_streaming,
                                 VMK_DMA_DIRECTION_FROM_MEMORY,
                                 len,
                                 ioa);
        if (VMK_UNLIKELY(status != VMK_OK)) {
                ionic_err("ionic_dma_unmap() failed, status: %s",
                          vmk_StatusToString(status));
        }

        return status;
}


static void ionic_tx_clean(struct queue *q, struct desc_info *desc_info,
                           struct cq_info *cq_info, void *cb_arg)
{
        struct qcq *qcq = q_to_qcq(q);
        struct ionic_en_tx_ring *tx_ring = qcq->ring;
        struct txq_desc *desc = desc_info->desc;
        struct txq_sg_desc *sg_desc = desc_info->sg_desc;
        struct txq_sg_elem *elem = sg_desc->elems;
        struct tx_stats *stats = q_to_tx_stats(q);
        vmk_PktHandle *pkt = (vmk_PktHandle *) cb_arg;
        unsigned int i;

//        dma_unmap_page(dev, (dma_addr_t)desc->addr,
//                       desc->len, DMA_TO_DEVICE);

//num_sg_elems is not valid
        for (i = 0; i < desc->num_sg_elems; i++, elem++)
                ionic_tx_unmap_frag(q,
                                    elem->len,
                                    (vmk_IOA) elem->addr);
        if (VMK_LIKELY(pkt)) {
                if (ionic_q_has_space(q, IONIC_EN_WAKE_QUEUE_THRESHOLD)) {
                        vmk_CPUMemFenceReadWrite();

                        if (VMK_UNLIKELY(ionic_en_is_queue_stop(tx_ring->uplink_handle,
                                                                tx_ring->shared_q_data_idx))) {
                                ionic_en_txq_start(tx_ring->uplink_handle,
                                                   tx_ring->shared_q_data_idx,
                                                   VMK_TRUE);
                                stats->wake++;
                        }
                        ionic_en_pkt_release(pkt, NULL);
                        stats->clean++;
                }
        }
}


static bool
ionic_tx_service(struct cq *cq,
                 struct cq_info *cq_info,
                 void *cb_arg)
{
        struct txq_comp *comp = cq_info->cq_desc;

        if (comp->color != cq->done_color)
                return VMK_FALSE;

        ionic_q_service(cq->bound_q, cq_info, comp->comp_index);

        return VMK_TRUE;
}


vmk_Bool
ionic_tx_netpoll(vmk_AddrCookie priv,
                 vmk_uint32 budget)
{
        vmk_uint32 polled;
        vmk_Bool poll_again = VMK_TRUE;
        void *qcq = priv.ptr;

//        ionic_err("ionic_tx_netpoll(), ring_idx: %d", ((struct qcq*)qcq)->ring_idx);

	polled = ionic_netpoll(budget, ionic_tx_service, qcq);

        if (polled != budget) {
                poll_again = VMK_FALSE;
        }

        return poll_again;
}

vmk_Bool
ionic_rx_netpoll(vmk_AddrCookie priv,
                 vmk_uint32 budget)
{
        vmk_uint32 polled;
        vmk_Bool poll_again = VMK_TRUE;
        void *qcq = priv.ptr;
        struct cq *cq = &((struct qcq *)qcq)->cq;

//        ionic_err("ionic_rx_netpoll(), ring_idx: %d", ((struct qcq*)qcq)->ring_idx);

        polled = ionic_netpoll(budget, ionic_rx_service, qcq);

        if (polled != budget) {
                ionic_rx_fill(cq->bound_q);
                poll_again = VMK_FALSE;
        }


        return poll_again;
}

static void ionic_tx_tso_post(struct queue *q, struct txq_desc *desc,
                              vmk_PktHandle *pkt, unsigned int hdrlen,
                              unsigned int mss, u16 vlan_tci, bool has_vlan,
                              bool outer_csum, bool start, bool done)
{
        desc->opcode = TXQ_DESC_OPCODE_TSO;
        desc->vlan_tci = vlan_tci;

        if (start) {
                desc->hdr_len = hdrlen;
        }

        desc->V = has_vlan;
        desc->C = 1;
        desc->O = outer_csum;
        desc->S = start;
        desc->E = done;
        if (!start) {
                desc->mss = mss;
        }

        if (done) {
                ionic_q_post(q, VMK_TRUE, /* !skb->xmit_more*/ ionic_tx_clean, pkt);
        } else {
                ionic_q_post(q, VMK_FALSE, ionic_tx_clean, NULL);
        }
}

static struct txq_desc *ionic_tx_tso_next(struct queue *q,
                                          struct txq_sg_elem **elem)
{
        struct txq_desc *desc = q->head->desc;
        struct txq_sg_desc *sg_desc = q->head->sg_desc;

        *elem = sg_desc->elems;
        return desc;
}

static VMK_ReturnStatus
ionic_tx_tso(struct queue *q,
             vmk_PktHandle *pkt,
             ionic_tx_ctx *ctx)
{
        struct tx_stats *stats = q_to_tx_stats(q);
        struct desc_info *abort = q->head;
        struct desc_info *rewind = abort;
        struct txq_desc *desc;
        struct txq_sg_elem *elem;
        const vmk_SgElem *frag;
        unsigned int hdrlen;
        unsigned int mss = ctx->mss;
        unsigned int nfrags = ctx->nr_frags; 
        unsigned int len_left;
        unsigned int frag_left = 0;
        unsigned int left;
        unsigned int seglen;
        unsigned int len;
        unsigned int offset = 0;
        bool outer_csum = vmk_PktIsMustOuterCsum(pkt);
        bool has_vlan;
        bool start, done;
        u64 total_pkts = 0;
        u64 total_bytes = 0;
        vmk_uint32 i;
        u16 vlan_tci;

        has_vlan = !!(ctx->offload_flags & IONIC_TX_VLAN);
        if (has_vlan) {
                vlan_tci = (ctx->vlan_id & IONIC_VLAN_MASK) |
                           (ctx->priority << IONIC_VLAN_PRIO_SHIFT);
        } else {
                vlan_tci = 0;
        }

        hdrlen = ctx->l4_hdr_entry->nextHdrOffset;
        len_left = vmk_PktFrameLenGet(pkt) - hdrlen;
        seglen = hdrlen + mss;
        left = vmk_PktFrameMappedLenGet(pkt);

        desc = ionic_tx_tso_next(q, &elem);
        start = VMK_TRUE;

        while (left > 0) {
                len = IONIC_MIN(seglen, left);

                frag_left = seglen - len;
                
                desc->addr = ionic_tx_map_single(q,
                                                 vmk_PktFrameMappedPointerGet(pkt) + offset,
                                                 len);
                if (!desc->addr)
                        goto err_out_abort;
                desc->len = len;
                desc->num_sg_elems = 0;
                left -= len;
                offset += len;

                if (offset == vmk_PktFrameLenGet(pkt)) {
                        nfrags--;
                        stats->frags++;
                        done = VMK_TRUE;

                        goto done_pkt;
                }

                if (nfrags > 0 && frag_left > 0) {
                        continue;
                }

                done = (nfrags == 0 && left == 0);

done_pkt:
                ionic_tx_tso_post(q, desc, pkt, hdrlen, mss, vlan_tci,
                                  has_vlan, outer_csum, start, done);
                total_pkts++;
                total_bytes += start ? len : len + hdrlen;
                desc = ionic_tx_tso_next(q, &elem);
                start = VMK_FALSE;
                seglen = mss;
        }


        if (nfrags > 1) {
                nfrags--;
        }


        for (i = 1; len_left && nfrags; i++) { 
                frag = vmk_PktSgElemGet(pkt, i);
                offset = 0;
                left = frag->length - offset;
                len_left -= left;
                nfrags--;
                stats->frags++;

                while (left > 0) {
                        if (frag_left > 0) {
                                len = IONIC_MIN(frag_left, left);
                                frag_left -= len;
                                elem->addr = ionic_tx_map_frag(q, frag,
                                                               offset, len);
                                if (!elem->addr)
                                        goto err_out_abort;
                                elem->len = len;
                                elem++;
                                desc->num_sg_elems++;
                                left -= len;
                                offset += len;

                                if (nfrags > 0 && frag_left > 0)
                                        continue;
                                done = (nfrags == 0 && left == 0);
                                ionic_tx_tso_post(q, desc, pkt, hdrlen, mss,
                                                  vlan_tci, has_vlan,
                                                  outer_csum, start, done);
                                total_pkts++;
                                total_bytes += start ? len : len + hdrlen;
                                desc = ionic_tx_tso_next(q, &elem);
                                start = VMK_FALSE;
                        } else {
                                len = IONIC_MIN(mss, left);
                                frag_left = mss - len;
                                desc->addr = ionic_tx_map_frag(q, frag,
                                                               offset, len);
                                if (!desc->addr)
                                        goto err_out_abort;
                                desc->len = len;
                                desc->num_sg_elems = 0;
                                left -= len;
                                offset += len;

                                if (nfrags > 0 && frag_left > 0)
                                        continue;
                                done = (nfrags == 0 && left == 0);
                                ionic_tx_tso_post(q, desc, pkt, hdrlen, mss,
                                                  vlan_tci, has_vlan,
                                                  outer_csum, start, done);
                                total_pkts++;
                                total_bytes += start ? len : len + hdrlen;
                                desc = ionic_tx_tso_next(q, &elem);
                                start = VMK_FALSE;
                        }
                }
        }

        stats->pkts += total_pkts;
        stats->bytes += total_bytes;
        stats->tso++;

        return VMK_OK;

err_out_abort:
        while (rewind->desc != q->head->desc) {
                ionic_tx_clean(q, rewind, NULL, NULL);
                rewind = rewind->next;
        }
        q->head = abort;

        return VMK_NO_MEMORY;
}


static VMK_ReturnStatus
ionic_tx_calc_csum(struct queue *q,
                   vmk_PktHandle *pkt,
                   ionic_tx_ctx *ctx)
{
        struct txq_desc *desc = q->head->desc;
        struct tx_stats *stats = q_to_tx_stats(q);
        vmk_Bool is_insert_vlan;
        dma_addr_t addr;

        addr = ionic_tx_map_single(q,
                                   vmk_PktFrameMappedPointerGet(pkt),
                                   ctx->mapped_len);
        if (VMK_UNLIKELY(!addr)) {
                ionic_err("ionic_tx_map_single() failed, status:"
                          " VMK_DMA_MAPPING_FAILED");
                return VMK_DMA_MAPPING_FAILED;
        }

        is_insert_vlan = !!(ctx->offload_flags & IONIC_TX_VLAN);

        desc->opcode = TXQ_DESC_OPCODE_CALC_CSUM_TCPUDP;
        desc->num_sg_elems = ctx->nr_frags - 1;
        desc->len = ctx->mapped_len;
        desc->addr = addr;

        if (is_insert_vlan) {
                desc->vlan_tci = (ctx->vlan_id & IONIC_VLAN_MASK) |
                                 (ctx->priority << IONIC_VLAN_PRIO_SHIFT);
        } else {
                desc->vlan_tci = 0;
        }

        desc->hdr_len = 0;
//        desc->csum_offset = 0; 
        desc->V = is_insert_vlan;
        desc->C = 1;
//        desc->O = ctx->is_encap;
        /* Since ESXi doesn't has API to check if we need l3 csum,
         * we make it follow the l4 csum value */
        desc->l3_csum = VMK_TRUE; 
        desc->l4_csum = VMK_TRUE;
//        if (skb->csum_not_inet)
//                stats->crc32_csum++;
//        else
        stats->csum++;

        return VMK_OK;
}

static VMK_ReturnStatus
ionic_tx_calc_no_csum(struct queue *q,
                      vmk_PktHandle *pkt,
                      ionic_tx_ctx *ctx)
{
        struct txq_desc *desc = q->head->desc;
        struct tx_stats *stats = q_to_tx_stats(q);
        vmk_Bool is_insert_vlan;
        dma_addr_t addr;

        addr = ionic_tx_map_single(q,
                                   vmk_PktFrameMappedPointerGet(pkt),
                                   ctx->mapped_len);
        if (VMK_UNLIKELY(!addr)) {
                ionic_err("ionic_tx_map_single() failed, status:"
                          " VMK_DMA_MAPPING_FAILED");
                return VMK_DMA_MAPPING_FAILED;
        }

        is_insert_vlan = !!(ctx->offload_flags & IONIC_TX_VLAN);

        desc->opcode = TXQ_DESC_OPCODE_CALC_NO_CSUM;
        desc->num_sg_elems = ctx->nr_frags - 1;
        desc->len = ctx->mapped_len;
        desc->addr = addr;

        if (is_insert_vlan) {
                desc->vlan_tci = (ctx->vlan_id & IONIC_VLAN_MASK) |
                                 (ctx->priority << IONIC_VLAN_PRIO_SHIFT);
        } else {
                desc->vlan_tci = 0;
        }

        desc->hdr_len = 0;
        desc->csum_offset = 0;
        desc->V = is_insert_vlan;
        desc->C = 1;
//        desc->O = ctx->is_encap;

        stats->no_csum++;

        return VMK_OK;
}

static int ionic_tx_pkt_frags(struct queue *q,
                              vmk_PktHandle *pkt,
                              ionic_tx_ctx *ctx)
{
        VMK_ReturnStatus status = VMK_OK;
        struct tx_stats *stats = q_to_tx_stats(q);
        const vmk_SgElem *sg_elem;
        unsigned int len_left = ctx->frame_len - ctx->mapped_len;
        struct txq_sg_desc *sg_desc = q->head->sg_desc;
        struct txq_sg_elem *elem = sg_desc->elems;
        dma_addr_t dma_addr;
        vmk_uint32 i;

        VMK_ASSERT(len_left >= 0);

        for (i = 1; len_left; i++, elem++) {
                sg_elem = vmk_PktSgElemGet(pkt, i);
                if (VMK_UNLIKELY(!sg_elem)) {
                        ionic_err("vmk_PktSgElemGet() for index: %d failed", i);
                        status = VMK_FAILURE;
                        stats->dma_map_err++;
                        goto map_err;
                }

                dma_addr = ionic_tx_map_frag(q, sg_elem, 0, sg_elem->length);
                if (VMK_UNLIKELY(!dma_addr)) {
                        ionic_err("ionic_tx_map_frag() failed, status: "
                                  "VMK_NO_MEMORY");
                        status = VMK_DMA_MAPPING_FAILED;
                        stats->dma_map_err++;
                        goto map_err;
                }
                          
                elem->len = sg_elem->length;
                elem->addr = dma_addr;
                len_left -= elem->len;
                stats->frags++;
        }

        return status;

map_err:
        for(i -= 1; i >= 1; i--) {
                elem--;
                ionic_tx_unmap_frag(q, elem->len, elem->addr);
                elem->len = 0;
                elem->addr = 0;
        }

        return status;
}

static VMK_ReturnStatus
ionic_tx(struct queue *q,
         vmk_PktHandle *pkt,
         ionic_tx_ctx *ctx,
         vmk_Bool is_last_pkt)
{
        VMK_ReturnStatus status;

        struct tx_stats *stats = q_to_tx_stats(q);

        if (ctx->offload_flags & IONIC_TX_CSO) {
                status  = ionic_tx_calc_csum(q, pkt, ctx);
                if (status != VMK_OK) {
                        ionic_err("ionic_tx_calc_csum() failed, status: %s",
                                  vmk_StatusToString(status));
                        return status;
                }
        } else {
                status = ionic_tx_calc_no_csum(q, pkt, ctx);
                if (status != VMK_OK) {
                        ionic_err("ionic_tx_calc_no_csum() failed, "
                                  "status: %s", vmk_StatusToString(status));
                        return status;
                }
        }               

        status = ionic_tx_pkt_frags(q, pkt, ctx);
        if (status != VMK_OK) {
                ionic_err("ionic_tx_pkt_frags() failed, status: %s",
                          vmk_StatusToString(status));
                return status;
        }

        ionic_q_post(q, is_last_pkt, ionic_tx_clean, pkt);

        stats->pkts++;
        stats->bytes += ctx->frame_len;

        return status;
}


static VMK_ReturnStatus
ionic_pkt_header_parse(vmk_PktHandle *pkt,
                       ionic_tx_ctx *ctx)
{
        VMK_ReturnStatus status = VMK_OK;
        vmk_PktHeaderEntry *l3_hdr_entry = NULL, *l4_hdr_entry = NULL;

        ctx->is_encap = vmk_PktIsInnerOffload(pkt);

        if (ctx->offload_flags & (IONIC_TX_TSO | IONIC_TX_CSO)) {
                /*
                 * Find L4 header first, so that L2/L3 headers parsing results
                 * can be cached.
                 */
                if (ctx->is_encap) {
                        status = vmk_PktHeaderEncapL4Find(pkt,
                                                          &l4_hdr_entry,
                                                          NULL);
                        if (status != VMK_OK) {
                                ionic_err("vmk_PktHeaderEncapL4Find() failed, "
                                          "status: %s",
                                          vmk_StatusToString(status));
                                return status;
                        }
                        
                        status = vmk_PktHeaderEncapL3Find(pkt, 
                                                          &l3_hdr_entry,
                                                          NULL);
                        if (status != VMK_OK) {
                                ionic_err("vmk_PktHeaderEncapL3Find() failed, "
                                          "status: %s",
                                          vmk_StatusToString(status));
                                return status;
                        }

                } else {
                        status = vmk_PktHeaderL4Find(pkt,
                                                     &l4_hdr_entry,
                                                     NULL);
                        if (status != VMK_OK) {
                                ionic_err("vmk_PktHeaderL4Find() failed, "
                                          "status: %s",
                                          vmk_StatusToString(status));
                                return status;
                        }

                        status = vmk_PktHeaderL3Find(pkt, 
                                                     &l3_hdr_entry,
                                                     NULL);
                        if (status != VMK_OK) {
                                ionic_err("vmk_PktHeaderL3Find() failed, "
                                          "status: %s",
                                          vmk_StatusToString(status));
                                return status;
                        }
                }
        }

        if (ctx->offload_flags & IONIC_TX_TSO) {
               /*
                * The length of Ethernet, IP, and TCP headers, including
                * header options and extensions.
                */ 
               if (VMK_UNLIKELY(l4_hdr_entry->nextHdrOffset >
                                IONIC_MAX_TX_BUF_SIZE)) {
                        ionic_err("Drop over-sized TSO header, size: %d",
                                  l4_hdr_entry->nextHdrOffset);
                        return status;
                }
        } else if (ctx->offload_flags & IONIC_TX_CSO) {
                VMK_ASSERT(l4_hdr_entry->type == VMK_PKT_HEADER_L4_TCP ||
                           l4_hdr_entry->type == VMK_PKT_HEADER_L4_UDP);
                ctx->csum_offset = (l4_hdr_entry->type == VMK_PKT_HEADER_L4_TCP)?
                                   vmk_offsetof(vmk_TCPHdr, checksum) :
                                   vmk_offsetof(vmk_UDPHdr, checksum);
                if (VMK_UNLIKELY(l4_hdr_entry->offset + ctx->csum_offset >
                                 IONIC_MAX_CSUM_OFFSET)) {
                        ionic_err("Drop over-sized CSO header, size: %d",
                                  l4_hdr_entry->offset + ctx->csum_offset);
                        return status;
                }
        }

        ctx->l3_hdr_entry = l3_hdr_entry;
        ctx->l4_hdr_entry = l4_hdr_entry;

        return status;
}


static VMK_ReturnStatus
ionic_tx_descs_needed(struct queue *q,
                      vmk_PktHandle **pkt,
                      vmk_uint32 *num_tx_descs,
                      ionic_tx_ctx *ctx)
{
        VMK_ReturnStatus status = VMK_OK;
        struct tx_stats *stats = q_to_tx_stats(q);
        vmk_PktHandle *flat_pkt = NULL;
        vmk_PktHandle *orig_pkt = *pkt;

        ctx->is_tso_needed = vmk_PktIsLargeTcpPacket(orig_pkt);
        ctx->nr_frags = vmk_PktSgArrayGet(orig_pkt)->numElems;

        ctx->frame_len = vmk_PktFrameLenGet(orig_pkt);

        /* If TSO, need roundup(len/mss) descs */
        if (ctx->is_tso_needed) {
                ctx->mss = vmk_PktGetLargeTcpPacketMss(orig_pkt);
                *num_tx_descs = (ctx->frame_len / ctx->mss) + 1;

                if (ctx->nr_frags > IONIC_TX_MAX_SG_ELEMS) {
                        goto linearize;
                } else {
                        goto out;
                }
        }

        *num_tx_descs = 1;

        /* If non-TSO, just need 1 desc and nr_frags sg elems */
        if (ctx->nr_frags <= IONIC_TX_MAX_SG_ELEMS) {
                goto out;
        }

linearize:

        status = vmk_PktCopy(orig_pkt, &flat_pkt);
        if (VMK_UNLIKELY(status != VMK_OK)) {
                ionic_err("Failed to linearize non-TSO pkt");
                *num_tx_descs = 0;
                goto out;
        }

        ctx->nr_frags = 1;

        ionic_en_pkt_release(orig_pkt, NULL);
        *pkt = flat_pkt;
        stats->linearize++;

out:
        return status;
}

VMK_ReturnStatus
ionic_start_xmit(vmk_PktHandle *pkt,
                 struct ionic_en_uplink_handle *uplink_handle,
                 struct ionic_en_tx_ring *tx_ring,
                 vmk_Bool is_last_pkt)
{
        VMK_ReturnStatus status;
        struct queue *q = &(tx_ring->txqcq->q);
        struct tx_stats *stats = q_to_tx_stats(q);
        ionic_tx_ctx ctx;
        int ndescs = 0;

        VMK_ASSERT(pkt);
        VMK_ASSERT(uplink_handle);
        VMK_ASSERT(tx_ring);

        if (ionic_en_is_queue_stop(uplink_handle,
                                   tx_ring->shared_q_data_idx)) {
                ionic_info("Queue: %d is not available",
                           tx_ring->shared_q_data_idx);
                return VMK_BUSY;
        }

        vmk_Memset(&ctx, 0, sizeof(ionic_tx_ctx));
        status = ionic_tx_descs_needed(q,
                                       &pkt,
                                       &ndescs,
                                       &ctx);
        if (status != VMK_OK) {
                ionic_err("ionic_tx_descs_needed() failed, status: %s",
                          vmk_StatusToString(status));
                goto err_out_drop;
        }

        if (VMK_UNLIKELY(!ionic_q_has_space(q, ndescs))) {
                /* Might race with ionic_tx_clean, check again */
                vmk_CPUMemFenceReadWrite();

                if (VMK_LIKELY(ionic_q_has_space(q, ndescs))) {
                        stats->wake++;
                        ionic_en_txq_start(uplink_handle,
                                           tx_ring->shared_q_data_idx,
                                           VMK_TRUE);
                } else {
                        ionic_en_txq_quiesce(uplink_handle,
                                             tx_ring->shared_q_data_idx,
                                             VMK_TRUE);
                        stats->stop++;

                        return VMK_BUSY;
                }
        }

        ctx.offload_flags |= vmk_PktIsLargeTcpPacket(pkt) ? IONIC_TX_TSO : 0;
        ctx.offload_flags |= vmk_PktIsMustCsum(pkt) ? IONIC_TX_CSO : 0;

        if (uplink_handle->hw_features & ETH_HW_VLAN_TX_TAG) {
                if (vmk_PktMustVlanTag(pkt)) {
                        ctx.offload_flags |= IONIC_TX_VLAN;
                        ctx.vlan_id    = vmk_PktVlanIDGet(pkt);
                        ctx.priority   = vmk_PktPriorityGet(pkt);
                }
        }

        status = ionic_pkt_header_parse(pkt,
                                        &ctx);
        if (status != VMK_OK) {
                ionic_err("ionic_pkt_header_parse() failed, status: %s",
                          vmk_StatusToString(status));
                goto err_out_drop;
        }

        ctx.mapped_len = vmk_PktFrameMappedLenGet(pkt);

        if (ctx.is_tso_needed) {
                status = ionic_tx_tso(q, pkt, &ctx);
        } else {
                status = ionic_tx(q, pkt, &ctx, is_last_pkt);
        
        }
        //TODO: improve logging
        if (status != VMK_OK) {
                ionic_err("ionic_tx() failed, status: %s",
                          vmk_StatusToString(status));
                goto err_out_drop;
        }

        return VMK_OK;

err_out_drop:
        stats->drop++;
        if (VMK_LIKELY(pkt)) {
                ionic_en_pkt_release(pkt, NULL);
        }
        return VMK_OK;
}


/*
 ******************************************************************************
 *
 *   ionic_en_tx_ring_init
 *
 *   Initialize the given tx ring      
 *
 *  Parameters:
 *    ring_idx           - IN (tx ring index)
 *    shared_q_data_idx  - IN (index of uplink_q_data of uplink_handle)
 *    priv_data          - IN (driver private data)
 *    lif                - IN (pointer to lif struct)
 *
 *  Results:
 *     None
 *
 *  Side effects:
 *     None
 *
 ******************************************************************************
 */


inline void
ionic_en_tx_ring_init(vmk_uint32 ring_idx,
                      vmk_uint32 shared_q_data_idx,
                      struct ionic_en_priv_data *priv_data,
                      struct lif *lif)
{
        struct ionic_en_tx_ring *tx_ring;

        tx_ring = &priv_data->uplink_handle.tx_rings[ring_idx];

        tx_ring->ring_idx          = ring_idx;
        tx_ring->shared_q_data_idx = shared_q_data_idx;
        tx_ring->uplink_handle     = &priv_data->uplink_handle;
        tx_ring->txqcq             = lif->txqcqs[ring_idx]; 
        tx_ring->netpoll           = lif->txqcqs[ring_idx]->netpoll;

        tx_ring->is_init           = VMK_TRUE;
        tx_ring->is_actived        = VMK_TRUE;

        lif->txqcqs[ring_idx]->ring = tx_ring;

        ionic_en_txq_start(&priv_data->uplink_handle,
                           shared_q_data_idx,
                           VMK_TRUE);
}


/*
 ******************************************************************************
 *
 *   ionic_en_tx_ring_deinit
 *
 *   De-initialize the given tx ring      
 *
 *  Parameters:
 *    ring_idx           - IN (tx ring index)
 *    priv_data          - IN (driver private data)
 *
 *  Results:
 *     None
 *
 *  Side effects:
 *     None
 *
 ******************************************************************************
 */


inline void
ionic_en_tx_ring_deinit(vmk_uint32 ring_idx,
                        struct ionic_en_priv_data *priv_data)
{
        struct ionic_en_tx_ring *tx_ring;

        tx_ring = &priv_data->uplink_handle.tx_rings[ring_idx];

        VMK_ASSERT(tx_ring->is_init == VMK_TRUE);

        tx_ring->is_init           = VMK_FALSE;
        tx_ring->is_actived        = VMK_FALSE;

        ionic_en_txq_quiesce(&priv_data->uplink_handle,
                             tx_ring->shared_q_data_idx,
                             VMK_TRUE);
}


/*
 ******************************************************************************
 *
 *   ionic_en_rx_ring_init
 *
 *   Initialize the given rx ring      
 *
 *  Parameters:
 *    ring_idx           - IN (rx ring index)
 *    shared_q_data_idx  - IN (index of uplink_q_data of uplink_handle)
 *    priv_data          - IN (driver private data)
 *    lif                - IN (pointer to lif struct)
 *
 *  Results:
 *     None
 *
 *  Side effects:
 *     None
 *
 ******************************************************************************
 */

VMK_ReturnStatus
ionic_en_rx_ring_init(vmk_uint32 ring_idx,
                      vmk_uint32 shared_q_data_idx,
                      struct ionic_en_priv_data *priv_data,
                      struct lif *lif)
{
        VMK_ReturnStatus status;
        struct ionic_en_rx_ring *rx_ring;
        struct ionic_en_uplink_handle *uplink_handle;
        vmk_Name netpoll_name;

        uplink_handle = &priv_data->uplink_handle;

        rx_ring = &uplink_handle->rx_rings[ring_idx];

        rx_ring->ring_idx          = ring_idx;
        rx_ring->shared_q_data_idx = shared_q_data_idx;
        rx_ring->uplink_handle     = uplink_handle;
        rx_ring->rxqcq             = lif->rxqcqs[ring_idx]; 
        rx_ring->netpoll           = lif->rxqcqs[ring_idx]->netpoll;

        rx_ring->is_init           = VMK_TRUE;
        rx_ring->is_actived        = VMK_TRUE;

        lif->rxqcqs[ring_idx]->ring = rx_ring;

        if (shared_q_data_idx == uplink_handle->max_rx_normal_queues) {
                vmk_NameFormat(&netpoll_name,
                               "rx-rss-%d",
                               ring_idx - uplink_handle->max_rx_normal_queues);
                status = vmk_NetPollRegisterUplink(rx_ring->netpoll,
                                                   uplink_handle->uplink_dev,
                                                   netpoll_name,
                                                   VMK_TRUE);
                VMK_ASSERT(status == VMK_OK);
        } else {
                ionic_en_rxq_start(uplink_handle, shared_q_data_idx);
        }

        status = ionic_qcq_enable(rx_ring->rxqcq);
        if (status != VMK_OK) {
                ionic_err("ionic_qcq_enable() failed, status: %s",
                          vmk_StatusToString(status));
        }

        return status;
}


/*
 ******************************************************************************
 *
 *   ionic_en_rx_ring_deinit
 *
 *   De-initialize the given rx ring      
 *
 *  Parameters:
 *    ring_idx           - IN (rx ring index)
 *    priv_data          - IN (driver private data)
 *
 *  Results:
 *     None
 *
 *  Side effects:
 *     None
 *
 ******************************************************************************
 */


inline void
ionic_en_rx_ring_deinit(vmk_uint32 ring_idx,
                        struct ionic_en_priv_data *priv_data)
{
        struct ionic_en_rx_ring *rx_ring;
        struct ionic_en_uplink_handle *uplink_handle;

        uplink_handle = &priv_data->uplink_handle;

        rx_ring = &uplink_handle->rx_rings[ring_idx];

        VMK_ASSERT(rx_ring->is_init == VMK_TRUE);

        rx_ring->is_init           = VMK_FALSE;
        rx_ring->is_actived        = VMK_FALSE;

        if (rx_ring->shared_q_data_idx == uplink_handle->max_rx_normal_queues) {
                vmk_NetPollUnregisterUplink(rx_ring->netpoll);
        } else {
                ionic_en_rxq_quiesce(&priv_data->uplink_handle,
                                     rx_ring->shared_q_data_idx);
        }
}


/*
 ******************************************************************************
 *
 *   ionic_en_rx_rss_init
 *
 *   Initialize the rss queue and associated rx rss rings
 *
 *  Parameters:
 *    priv_data          - IN (driver private data)
 *    lif                - IN (pointer to lif struct)
 *
 *  Results:
 *     None
 *
 *  Side effects:
 *     None
 *
 ******************************************************************************
 */

void
ionic_en_rx_rss_init(struct ionic_en_priv_data *priv_data,
                     struct lif *lif)
{
        vmk_uint32 i, attached_rx_ring_idx, num_attached_rings;
        struct ionic_en_rx_rss_ring *rx_rss_ring;
        struct ionic_en_uplink_handle *uplink_handle;

        uplink_handle = &priv_data->uplink_handle;

        rx_rss_ring = &uplink_handle->rx_rss_ring;
        num_attached_rings = uplink_handle->rx_rings_per_rss_queue;

        ionic_dbg("ionic_en_rx_rss_ring_init() called");

        /* We only have one rx rss ring */
        rx_rss_ring->ring_idx              = 0;
        rx_rss_ring->shared_q_data_idx     = uplink_handle->max_rx_normal_queues;
        rx_rss_ring->priv_data             = priv_data;
        rx_rss_ring->num_attached_rx_rings = num_attached_rings; 

        for (i = 0; i < num_attached_rings; i++) {
                attached_rx_ring_idx = uplink_handle->max_rx_normal_queues + i;
 
                ionic_dbg("INIT rx rss, ring_idx: %d, shared_q_idx: %d, "
                          "attached rx ring idx: %d",
                          i, rx_rss_ring->shared_q_data_idx,
                          attached_rx_ring_idx);

                ionic_en_rx_ring_init(attached_rx_ring_idx,
                                      rx_rss_ring->shared_q_data_idx,
                                      priv_data,
                                      lif);
                rx_rss_ring->p_attached_rx_rings[i] =
                        &uplink_handle->rx_rings[attached_rx_ring_idx];

        }

        rx_rss_ring->is_init               = VMK_TRUE;
        rx_rss_ring->is_actived            = VMK_FALSE;
}


/*
 ******************************************************************************
 *
 *   ionic_en_rx_rss_deinit
 *
 *   De-initialize the rx rss ring
 *
 *  Parameters:
 *    priv_data          - IN (driver private data)
 *    lif                - IN (pointer to lif struct)
 *
 *  Results:
 *     None
 *
 *  Side effects:
 *     None
 *
 ******************************************************************************
 */


void
ionic_en_rx_rss_deinit(struct ionic_en_priv_data *priv_data,
                       struct lif *lif)
{
        vmk_uint32 i, attached_rx_ring_idx, num_attached_rings;
        struct ionic_en_rx_rss_ring *rx_rss_ring;
        struct ionic_en_uplink_handle *uplink_handle;

        uplink_handle = &priv_data->uplink_handle;

        rx_rss_ring = &uplink_handle->rx_rss_ring;

        VMK_ASSERT(rx_rss_ring->is_init == VMK_TRUE);

        num_attached_rings = uplink_handle->rx_rings_per_rss_queue;

        for (i = 0; i < num_attached_rings; i++) {
                attached_rx_ring_idx = uplink_handle->max_rx_normal_queues + i;
 
                ionic_dbg("DEINIT rx rss, ring_idx: %d, shared_q_idx: %d"
                          "attached rx ring idx: %d",
                          i, rx_rss_ring->shared_q_data_idx,
                          attached_rx_ring_idx);
                       
                ionic_en_rx_ring_deinit(attached_rx_ring_idx,
                                        priv_data);

                rx_rss_ring->p_attached_rx_rings[i] = NULL;
        }


        rx_rss_ring->is_init           = VMK_FALSE;
        rx_rss_ring->is_actived        = VMK_FALSE;
}



