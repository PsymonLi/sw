/*
 * Copyright (c) 2019 Pensando Systems. All rights reserved.
 */

#include "ionic.h"
#include "ionic_utilities.h"

/* BAR0 resources
 */

#define BAR0_SIZE                       0x8000

#define BAR0_DEV_CMD_REGS_OFFSET        0x0000
#define BAR0_DEV_CMD_DB_OFFSET          0x1000
#define BAR0_INTR_STATUS_OFFSET         0x2000
#define BAR0_INTR_CTRL_OFFSET           0x3000

#define DEV_CMD_DONE                    0x00000001

#define ASIC_TYPE_CAPRI                 0

extern struct ionic_driver ionic_driver;

VMK_ReturnStatus
ionic_dev_setup(struct ionic_dev *idev, struct ionic_dev_bar bars[],
                unsigned int num_bars)
{
        VMK_ReturnStatus status;
        struct ionic_dev_bar *bar = &bars[0];
        u32 sig;

        /* BAR0 resources
         */

        if (num_bars < 1 || bar->len != BAR0_SIZE)
                return VMK_BAD_ADDR_RANGE;

        idev->dev_cmd = bar->vaddr + BAR0_DEV_CMD_REGS_OFFSET;
        idev->dev_cmd_db = bar->vaddr + BAR0_DEV_CMD_DB_OFFSET;
        idev->intr_status = bar->vaddr + BAR0_INTR_STATUS_OFFSET;
        idev->intr_ctrl = bar->vaddr + BAR0_INTR_CTRL_OFFSET;
#ifdef HAPS
        idev->ident = bar->vaddr + 0x800;
#endif

//        sig = ioread32(&idev->dev_cmd->signature);
        sig = ionic_readl_raw((vmk_VA)&idev->dev_cmd->signature);
        if (sig != DEV_CMD_SIGNATURE)
                return VMK_BAD_ADDR_RANGE;

        /* BAR1 resources
         */

        bar++;
        if (num_bars < 2)
                return VMK_BAD_ADDR_RANGE;

        idev->db_pages = bar->vaddr;
        idev->phy_db_pages = bar->bus_addr;

        /* BAR2 resources
        */

        status = ionic_mutex_create("ionic_cmb_mutex",
                                    ionic_driver.module_id,
                                    ionic_driver.heap_id,
                                    VMK_LOCKDOMAIN_INVALID,
                                    VMK_MUTEX,
                                    VMK_MUTEX_UNRANKED,
                                    &idev->cmb_inuse_lock);
        if (status != VMK_OK) {
                ionic_err("ionic_mutex_create() faild, status: %s", 
                          vmk_StatusToString(status));
                return status;
        }

        bar++;
        if (num_bars < 3) {
                idev->phy_cmb_pages = 0;
                idev->cmb_npages = 0;
                idev->cmb_inuse = NULL;
                return status;
        }

        idev->phy_cmb_pages = bar->bus_addr;
        idev->cmb_npages = bar->len / VMK_PAGE_SIZE;
        idev->cmb_inuse = ionic_heap_zalloc(ionic_driver.heap_id,
                                            BITS_TO_LONGS(idev->cmb_npages) * sizeof(long));
        if (!idev->cmb_inuse) {
                idev->phy_cmb_pages = 0;
                idev->cmb_npages = 0;
        }

        return status;
}

inline void
ionic_dev_clean(struct ionic *ionic)
{
        if (ionic->num_bars >= 3) { 
                ionic_heap_free(ionic_driver.heap_id,
                                ionic->en_dev.idev.cmb_inuse);
        }

        ionic_mutex_destroy(ionic->en_dev.idev.cmb_inuse_lock);
}

u8 ionic_dev_cmd_status(struct ionic_dev *idev)
{
//        return ioread8(&idev->dev_cmd->comp.status);
        return ionic_readb_raw((vmk_VA)&idev->dev_cmd->comp.status);
}

bool ionic_dev_cmd_done(struct ionic_dev *idev)
{
//        return ioread32(&idev->dev_cmd->done) & DEV_CMD_DONE;
        return ionic_readl_raw((vmk_VA)&idev->dev_cmd->done) & DEV_CMD_DONE;
}

void ionic_dev_cmd_comp(struct ionic_dev *idev, void *mem)
{
        union dev_cmd_comp *comp = mem;
        unsigned int i;

        for (i = 0; i < ARRAY_SIZE(comp->words); i++)
//                comp->words[i] = ioread32(&idev->dev_cmd->comp.words[i]);
                comp->words[i] = ionic_readl_raw((vmk_VA)&idev->dev_cmd->comp.words[i]);
}

void ionic_dev_cmd_go(struct ionic_dev *idev, union dev_cmd *cmd)
{
        unsigned int i;

        for (i = 0; i < ARRAY_SIZE(cmd->words); i++)
//                iowrite32(cmd->words[i], &idev->dev_cmd->cmd.words[i]);
                ionic_writel_raw(cmd->words[i],
                                 (vmk_VA)&idev->dev_cmd->cmd.words[i]);

//        iowrite32(0, &idev->dev_cmd->done);
//        iowrite32(1, &idev->dev_cmd_db->v);
        ionic_writel_raw(0, (vmk_VA)&idev->dev_cmd->done);
        ionic_writel_raw(1, (vmk_VA)&idev->dev_cmd_db->v);

}

void ionic_dev_cmd_reset(struct ionic_dev *idev)
{
        union dev_cmd cmd = {
                .reset.opcode = CMD_OPCODE_RESET,
        };

        ionic_dev_cmd_go(idev, &cmd);
}


void ionic_dev_cmd_port_config(struct ionic_dev *idev, struct port_config *pc)
{
        union dev_cmd cmd = {
                .port_config.opcode = CMD_OPCODE_PORT_CONFIG_SET,
                .port_config.config = *pc,
        };

         ionic_dev_cmd_go(idev, &cmd);
}


void ionic_dev_cmd_hang_notify(struct ionic_dev *idev)
{
        union dev_cmd cmd = {
                .hang_notify.opcode = CMD_OPCODE_HANG_NOTIFY,
        };

        ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_identify(struct ionic_dev *idev, u16 ver, dma_addr_t addr)
{
        union dev_cmd cmd = {
                .identify.opcode = CMD_OPCODE_IDENTIFY,
                .identify.ver = ver,
                .identify.addr = addr,
        };

        ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_lif_init(struct ionic_dev *idev, u32 index)
{
        union dev_cmd cmd = {
                .lif_init.opcode = CMD_OPCODE_LIF_INIT,
                .lif_init.index = index,
        };

        ionic_dev_cmd_go(idev, &cmd);
}

void ionic_dev_cmd_adminq_init(struct ionic_dev *idev, struct queue *adminq,
                               unsigned int index, unsigned int lif_index,
                               unsigned int intr_index)
{
        union dev_cmd cmd = {
                .adminq_init.opcode = CMD_OPCODE_ADMINQ_INIT,
                .adminq_init.index = adminq->index,
                .adminq_init.pid = adminq->pid,
                .adminq_init.intr_index = intr_index,
                .adminq_init.lif_index = lif_index,
                .adminq_init.ring_size = ionic_ilog2(adminq->num_descs),
                .adminq_init.ring_base = adminq->base_pa,
        };

        //printk(KERN_ERR "adminq_init.pid %d\n", cmd.adminq_init.pid);
        //printk(KERN_ERR "adminq_init.index %d\n", cmd.adminq_init.index);
        //printk(KERN_ERR "adminq_init.ring_base %llx\n",
        //       cmd.adminq_init.ring_base);
        //printk(KERN_ERR "adminq_init.ring_size %d\n",
        //       cmd.adminq_init.ring_size);
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

struct doorbell __iomem *ionic_db_map(struct ionic_dev *idev, struct queue *q)
{
        struct doorbell __iomem *db;

        db = (void *)idev->db_pages + (q->pid * VMK_PAGE_SIZE);
        db += q->qtype;

        return db;
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

//        iowrite32(*(u32 *)intr_to_mask_on_assert(&ctrl),
//                  intr_to_mask_on_assert(intr->ctrl));
        ionic_writel_raw(*(u32 *)intr_to_mask_on_assert(&ctrl),
                         (vmk_VA)intr_to_mask_on_assert(intr->ctrl));
}

void ionic_intr_return_credits(struct intr *intr, unsigned int credits,
                               bool unmask, bool reset_timer)
{
        struct intr_ctrl ctrl = {
                .int_credits = credits,
                .unmask = unmask,
                .coal_timer_reset = reset_timer,
        };

//        iowrite32(*(u32 *)intr_to_credits(&ctrl),
//                  intr_to_credits(intr->ctrl));
        ionic_writel_raw(*(u32 *)intr_to_credits(&ctrl),
                         (vmk_VA)intr_to_credits(intr->ctrl));
}

void ionic_intr_mask(struct intr *intr, bool mask)
{
        struct intr_ctrl ctrl = {
                .mask = mask ? 1 : 0,
        };

//        iowrite32(*(u32 *)intr_to_mask(&ctrl),
//                  intr_to_mask(intr->ctrl));
//        (void)ioread32(intr_to_mask(intr->ctrl)); /* flush write */

        ionic_writel_raw(*(u32 *)intr_to_mask(&ctrl),
                         (vmk_VA)intr_to_mask(intr->ctrl));
        ionic_readl_raw((vmk_VA)intr_to_mask(intr->ctrl)); /* flush write */

}

void ionic_intr_coal_set(struct intr *intr, u32 intr_coal)
{
        struct intr_ctrl ctrl = {
                .coalescing_init = intr_coal > INTR_CTRL_COAL_MAX ?
                        INTR_CTRL_COAL_MAX : intr_coal,
        };

        //iowrite32(*(u32 *)intr_to_coal(&ctrl), intr_to_coal(intr->ctrl));
        //(void)ioread32(intr_to_coal(intr->ctrl)); /* flush write */

        ionic_writel_raw(*(u32 *)intr_to_coal(&ctrl),
                         (vmk_VA)intr_to_coal(intr->ctrl));
        ionic_readl_raw((vmk_VA)intr_to_coal(intr->ctrl)); /* flush write */
}

VMK_ReturnStatus
ionic_cq_init(struct lif *lif, struct cq *cq, struct intr *intr,
              unsigned int num_descs, size_t desc_size)
{
        struct cq_info *cur;
        unsigned int ring_size;
        unsigned int i;

        if (desc_size == 0 || !ionic_is_power_of_2(num_descs))
                return VMK_BAD_PARAM;

        ring_size = ionic_ilog2(num_descs);
        if (ring_size < 2 || ring_size > 16)
                return VMK_FAILURE;

        cq->lif = lif;
        cq->bound_intr = intr;
        cq->num_descs = num_descs;
        cq->desc_size = desc_size;
        cq->tail = cq->info;
        cq->done_color = 1;

        cur = cq->info;

        for (i = 0; i < num_descs; i++) {
                if (i + 1 == num_descs) {
                        cur->next = cq->info;
                        cur->last = true;
                } else {
                        cur->next = cur + 1;
                }
                cur->index = i;
                cur++;
        }

        return VMK_OK;
}

void ionic_cq_map(struct cq *cq, void *base, dma_addr_t base_pa)
{
        struct cq_info *cur;
        unsigned int i;

        cq->base = base;
        cq->base_pa = base_pa;

        for (i = 0, cur = cq->info; i < cq->num_descs; i++, cur++)
                cur->cq_desc = base + (i * cq->desc_size);
}

void ionic_cq_bind(struct cq *cq, struct queue *q)
{
        // TODO support many:1 bindings using qid as index into bound_q array
        cq->bound_q = q;
}

unsigned int ionic_cq_service(struct cq *cq, unsigned int work_to_do,
                              ionic_cq_cb cb, void *cb_arg)
{
        unsigned int work_done = 0;

        if (work_to_do == 0)
                return 0;

        while (cb(cq, cq->tail, cb_arg)) {
                if (cq->tail->last)
                        cq->done_color = !cq->done_color;
                cq->tail = cq->tail->next;
                if (++work_done >= work_to_do)
                        break;
        }

        return work_done;
}

VMK_ReturnStatus
ionic_q_init(struct lif *lif, struct ionic_dev *idev, struct queue *q,
             unsigned int index, const char *base, unsigned int num_descs,
             size_t desc_size, size_t sg_desc_size, unsigned int pid)
{
        struct desc_info *cur;
        unsigned int ring_size;
        unsigned int i;

        if (desc_size == 0 || !ionic_is_power_of_2(num_descs))
                return VMK_BAD_PARAM;

        ring_size = ionic_ilog2(num_descs);
        if (ring_size < 2 || ring_size > 16)
                return VMK_FAILURE;

        q->lif = lif;
        q->idev = idev;
        q->index = index;
        q->num_descs = num_descs;
        q->desc_size = desc_size;
        q->sg_desc_size = sg_desc_size;
        q->head = q->tail = q->info;
        q->pid = pid;

        //snprintf(q->name, sizeof(q->name), "%s%u", base, index);
        vmk_Snprintf(q->name, sizeof(q->name), "%s%u", base, index);

        cur = q->info;

        for (i = 0; i < num_descs; i++) {
                if (i + 1 == num_descs)
                        cur->next = q->info;
                else
                        cur->next = cur + 1;
                cur->index = i;
                cur->left = num_descs - i;
                cur++;
        }

        return VMK_OK;
}

void ionic_q_map(struct queue *q, void *base, dma_addr_t base_pa)
{
        struct desc_info *cur;
        unsigned int i;

        q->base = base;
        q->base_pa = base_pa;

        for (i = 0, cur = q->info; i < q->num_descs; i++, cur++)
                cur->desc = base + (i * q->desc_size);
}

void ionic_q_sg_map(struct queue *q, void *base, dma_addr_t base_pa)
{
        struct desc_info *cur;
        unsigned int i;

        q->sg_base = base;
        q->sg_base_pa = base_pa;

        for (i = 0, cur = q->info; i < q->num_descs; i++, cur++)
                cur->sg_desc = base + (i * q->sg_desc_size);
}

void ionic_q_post(struct queue *q, bool ring_doorbell, desc_cb cb,
                  void *cb_arg)
{
        q->head->cb = cb;
        q->head->cb_arg = cb_arg;
        q->head = q->head->next;

        if (ring_doorbell) {
                struct doorbell db = {
                        .qid_lo = q->qid,
                        .qid_hi = q->qid >> 8,
                        .ring = 0,
                        .p_index = q->head->index,
                };

                ionic_writeq_raw(*(u64 *)&db, (vmk_VA)q->db);                        
        }
}

void ionic_q_rewind(struct queue *q, struct desc_info *start)
{
        struct desc_info *cur = start;

        while (cur != q->head) {
                if (cur->cb)
                        cur->cb(q, cur, NULL, cur->cb_arg);
                cur = cur->next;
        }

        q->head = start;
}

unsigned int ionic_q_space_avail(struct queue *q)
{
        unsigned int avail = q->tail->index;

        if (q->head->index >= avail)
                avail += q->head->left - 1;
        else
                avail -= q->head->index + 1;

        return avail;
}

bool ionic_q_has_space(struct queue *q, unsigned int want)
{
        return ionic_q_space_avail(q) >= want;
}

void ionic_q_service(struct queue *q, struct cq_info *cq_info,
                     unsigned int stop_index)
{
        struct desc_info *desc_info;

        do {
                desc_info = q->tail;
                if (desc_info->cb) {
                        desc_info->cb(q, desc_info, cq_info,
                                      desc_info->cb_arg);
                }
                q->tail = q->tail->next;
        } while (desc_info->index != stop_index);
}
