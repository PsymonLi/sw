/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2017 - 2019 Pensando Systems, Inc */

#ifndef _IONIC_DEV_H_
#define _IONIC_DEV_H_

#include <linux/mutex.h>
#include <linux/workqueue.h>

#include <ionic_if.h>

#define IONIC_MIN_MTU		ETH_MIN_MTU
#define IONIC_MAX_MTU		9194
#define IONIC_MAX_TXRX_DESC	65536
#define IONIC_MIN_TXRX_DESC	16
#define IONIC_DEF_TXRX_DESC	4096
#define IONIC_LIFS_MAX		1024

#define BAR0_SIZE			0x8000

#pragma pack(push, 1)

union dev_cmd {
	u32 words[16];
	struct admin_cmd cmd;
	struct nop_cmd nop;
	struct reset_cmd reset;
	struct hang_notify_cmd hang_notify;
	struct identify_cmd identify;
	struct lif_init_cmd lif_init;
	struct adminq_init_cmd adminq_init;
	struct port_config_cmd port_config;
};

union dev_cmd_comp {
	u32 words[4];
	u8 status;
	struct admin_comp comp;
	struct nop_comp nop;
	struct reset_comp reset;
	struct hang_notify_comp hang_notify;
	struct identify_comp identify;
	struct lif_init_comp lif_init;
	struct adminq_init_comp adminq_init;
	struct port_config_comp port_config;
};

struct dev_cmd_regs {
	u32 signature;
	u32 done;
	union dev_cmd cmd;
	union dev_cmd_comp comp;
};

struct dev_cmd_db {
	u32 v;
};

#define IONIC_BARS_MAX		6
#define IONIC_PCI_BAR_DBELL	1

struct ionic_dev_bar {
	void __iomem *vaddr;
	phys_addr_t bus_addr;
	unsigned long len;
	int res_index;
};

/**
 * struct doorbell - Doorbell register layout
 * @p_index: Producer index
 * @ring:    Selects the specific ring of the queue to update.
 *           Type-specific meaning:
 *              ring=0: Default producer/consumer queue.
 *              ring=1: (CQ, EQ) Re-Arm queue.  RDMA CQs
 *              send events to EQs when armed.  EQs send
 *              interrupts when armed.
 * @qid:     The queue id selects the queue destination for the
 *           producer index and flags.
 */
struct doorbell {
	u16 p_index;
	u8 ring:3;
	u8 rsvd:5;
	u8 qid_lo;
	u16 qid_hi;
	u16 rsvd2;
};

#define INTR_CTRL_REGS_MAX	2048
#define INTR_CTRL_COAL_MAX	0x3F

/**
 * struct intr_ctrl - Interrupt control register
 * @coalescing_init:  Coalescing timer initial value, in
 *                    device units.  Use @identity->intr_coal_mult
 *                    and @identity->intr_coal_div to convert from
 *                    usecs to device units:
 *
 *                      coal_init = coal_usecs * coal_mutl / coal_div
 *
 *                    When an interrupt is sent the interrupt
 *                    coalescing timer current value
 *                    (@coalescing_curr) is initialized with this
 *                    value and begins counting down.  No more
 *                    interrupts are sent until the coalescing
 *                    timer reaches 0.  When @coalescing_init=0
 *                    interrupt coalescing is effectively disabled
 *                    and every interrupt assert results in an
 *                    interrupt.  Reset value: 0.
 * @mask:             Interrupt mask.  When @mask=1 the interrupt
 *                    resource will not send an interrupt.  When
 *                    @mask=0 the interrupt resource will send an
 *                    interrupt if an interrupt event is pending
 *                    or on the next interrupt assertion event.
 *                    Reset value: 1.
 * @int_credits:      Interrupt credits.  This register indicates
 *                    how many interrupt events the hardware has
 *                    sent.  When written by software this
 *                    register atomically decrements @int_credits
 *                    by the value written.  When @int_credits
 *                    becomes 0 then the "pending interrupt" bit
 *                    in the Interrupt Status register is cleared
 *                    by the hardware and any pending but unsent
 *                    interrupts are cleared.
 *                    The upper 2 bits are special flags:
 *                       Bits 0-15: Interrupt Events -- Interrupt
 *                       event count.
 *                       Bit 16: @unmask -- When this bit is
 *                       written with a 1 the interrupt resource
 *                       will set mask=0.
 *                       Bit 17: @coal_timer_reset -- When this
 *                       bit is written with a 1 the
 *                       @coalescing_curr will be reloaded with
 *                       @coalescing_init to reset the coalescing
 *                       timer.
 * @mask_on_assert:   Automatically mask on assertion.  When
 *                    @mask_on_assert=1 the interrupt resource
 *                    will set @mask=1 whenever an interrupt is
 *                    sent.  When using interrupts in Legacy
 *                    Interrupt mode the driver must select
 *                    @mask_on_assert=0 for proper interrupt
 *                    operation.
 * @coalescing_curr:  Coalescing timer current value, in
 *                    microseconds.  When this value reaches 0
 *                    the interrupt resource is again eligible to
 *                    send an interrupt.  If an interrupt event
 *                    is already pending when @coalescing_curr
 *                    reaches 0 the pending interrupt will be
 *                    sent, otherwise an interrupt will be sent
 *                    on the next interrupt assertion event.
 */
struct intr_ctrl {
	u32 coalescing_init:6;
	u32 rsvd:26;
	u32 mask:1;
	u32 rsvd2:31;
	u32 int_credits:16;
	u32 unmask:1;
	u32 coal_timer_reset:1;
	u32 rsvd3:14;
	u32 mask_on_assert:1;
	u32 rsvd4:31;
	u32 coalescing_curr:6;
	u32 rsvd5:26;
	u32 rsvd6[3];
};

#define intr_to_coal(intr_ctrl)			(void *)((u8 *)(intr_ctrl) + 0)
#define intr_to_mask(intr_ctrl)			(void *)((u8 *)(intr_ctrl) + 4)
#define intr_to_credits(intr_ctrl)		(void *)((u8 *)(intr_ctrl) + 8)
#define intr_to_mask_on_assert(intr_ctrl)	(void *)((u8 *)(intr_ctrl) + 12)

struct intr_status {
	u32 status[2];
};

typedef struct intr_msixcfg_s {
	u_int64_t msgaddr;
	u_int32_t msgdata;
	u_int32_t vector_ctrl;
} __attribute__((packed)) intr_msixcfg_t;

#pragma pack(pop)

static inline void ionic_struct_size_checks(void)
{
	BUILD_BUG_ON(sizeof(struct doorbell) != 8);
	BUILD_BUG_ON(sizeof(struct intr_ctrl) != 32);
	BUILD_BUG_ON(sizeof(struct intr_status) != 8);
	BUILD_BUG_ON(sizeof(struct admin_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct admin_comp) != 16);
	BUILD_BUG_ON(sizeof(struct nop_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct nop_comp) != 16);
	BUILD_BUG_ON(sizeof(struct reset_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct reset_comp) != 16);
	BUILD_BUG_ON(sizeof(struct hang_notify_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct hang_notify_comp) != 16);
	BUILD_BUG_ON(sizeof(struct identify_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct identify_comp) != 16);
	BUILD_BUG_ON(sizeof(union identity) != 4096);
	BUILD_BUG_ON(sizeof(struct lif_init_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct lif_init_comp) != 16);
	BUILD_BUG_ON(sizeof(struct adminq_init_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct adminq_init_comp) != 16);
	BUILD_BUG_ON(sizeof(struct txq_init_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct txq_init_comp) != 16);
	BUILD_BUG_ON(sizeof(struct rxq_init_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct rxq_init_comp) != 16);
	BUILD_BUG_ON(sizeof(struct txq_desc) != 16);
	BUILD_BUG_ON(sizeof(struct txq_sg_desc) != 256);
	BUILD_BUG_ON(sizeof(struct txq_comp) != 16);
	BUILD_BUG_ON(sizeof(struct rxq_desc) != 16);
	BUILD_BUG_ON(sizeof(struct rxq_comp) != 16);
	BUILD_BUG_ON(sizeof(struct features_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct features_comp) != 16);
	BUILD_BUG_ON(sizeof(struct q_enable_cmd) != 64);
	BUILD_BUG_ON(sizeof(q_enable_comp) != 16);
	BUILD_BUG_ON(sizeof(struct q_disable_cmd) != 64);
	BUILD_BUG_ON(sizeof(q_disable_comp) != 16);
	BUILD_BUG_ON(sizeof(struct notifyq_init_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct notifyq_init_comp) != 16);
	BUILD_BUG_ON(sizeof(struct notifyq_event) != 64);
	BUILD_BUG_ON(sizeof(struct link_change_event) != 64);
	BUILD_BUG_ON(sizeof(struct reset_event) != 64);
	BUILD_BUG_ON(sizeof(struct heartbeat_event) != 64);
	BUILD_BUG_ON(sizeof(struct log_event) != 64);
	BUILD_BUG_ON(sizeof(struct notifyq_cmd) != 4);
	BUILD_BUG_ON(sizeof(union notifyq_comp) != 64);
	BUILD_BUG_ON(sizeof(struct station_mac_addr_get_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct station_mac_addr_get_comp) != 16);
	BUILD_BUG_ON(sizeof(struct rx_mode_set_cmd) != 64);
	BUILD_BUG_ON(sizeof(rx_mode_set_comp) != 16);
	BUILD_BUG_ON(sizeof(struct mtu_set_cmd) != 64);
	BUILD_BUG_ON(sizeof(mtu_set_comp) != 16);
	BUILD_BUG_ON(sizeof(struct rx_filter_add_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct rx_filter_add_comp) != 16);
	BUILD_BUG_ON(sizeof(struct rx_filter_del_cmd) != 64);
	BUILD_BUG_ON(sizeof(rx_filter_del_comp) != 16);
	BUILD_BUG_ON(sizeof(struct stats_dump_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct stats_dump_comp) != 16);
	BUILD_BUG_ON(sizeof(struct ionic_lif_stats) != 1024);
	BUILD_BUG_ON(sizeof(struct rss_hash_set_cmd) != 64);
	BUILD_BUG_ON(sizeof(rss_hash_set_comp) != 16);
	BUILD_BUG_ON(sizeof(struct rss_indir_set_cmd) != 64);
	BUILD_BUG_ON(sizeof(rss_indir_set_comp) != 16);
	BUILD_BUG_ON(sizeof(struct debug_q_dump_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct debug_q_dump_comp) != 16);
	BUILD_BUG_ON(sizeof(struct rdma_reset_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct rdma_queue_cmd) != 64);
	BUILD_BUG_ON(sizeof(union adminq_cmd) != 64);
	BUILD_BUG_ON(sizeof(union adminq_comp) != 16);
	BUILD_BUG_ON(sizeof(struct set_netdev_info_cmd) != 64);
	BUILD_BUG_ON(sizeof(set_netdev_info_comp) != 16);
	BUILD_BUG_ON(sizeof(struct port_config_cmd) != 64);
	BUILD_BUG_ON(sizeof(struct port_config_comp) != 16);
}

struct ionic_dev {
	struct dev_cmd_regs __iomem *dev_cmd;
	struct dev_cmd_db __iomem *dev_cmd_db;
	struct doorbell __iomem *db_pages;
	dma_addr_t phy_db_pages;
	struct intr_ctrl __iomem *intr_ctrl;
	struct intr_status __iomem *intr_status;
	uint8_t* msix_cfg_base;
	struct mutex cmb_inuse_lock; /* for cmb_inuse */
	unsigned long *cmb_inuse;
	dma_addr_t phy_cmb_pages;
	uint32_t cmb_npages;
#ifdef HAPS
	union identity __iomem *ident;
#endif
};

struct cq_info {
	void *cq_desc;
	struct cq_info *next;
	unsigned int index;
	bool last;
};

struct queue;
struct desc_info;

typedef void (*desc_cb)(struct queue *q, struct desc_info *desc_info,
			struct cq_info *cq_info, void *cb_arg);

struct desc_info {
	void *desc;
	void *sg_desc;
	struct desc_info *next;
	unsigned int index;
	unsigned int left;
	desc_cb cb;
	void *cb_arg;
};

#define QUEUE_NAME_MAX_SZ		(32)

struct queue {
	char name[QUEUE_NAME_MAX_SZ];
	struct ionic_dev *idev;
	struct lif *lif;
	unsigned int index;
	void *base;
	void *sg_base;
	dma_addr_t base_pa;
	dma_addr_t sg_base_pa;
	struct desc_info *info;
	struct desc_info *tail;
	struct desc_info *head;
	unsigned int num_descs;
	unsigned int desc_size;
	unsigned int sg_desc_size;
	struct doorbell __iomem *db;
	void *nop_desc;
	unsigned int pid;
	unsigned int qid;
	unsigned int qtype;
	u64 dbell_count;
	u64 drop;
	u64 stop;
	u64 wake;
};

#define INTR_INDEX_NOT_ASSIGNED		(-1)
#define INTR_NAME_MAX_SZ		(32)

struct intr {
	char name[INTR_NAME_MAX_SZ];
	unsigned int index;
	unsigned int vector;
	struct intr_ctrl __iomem *ctrl;
	u64 rearm_count;
	unsigned int cpu;
	cpumask_t affinity_mask;
};

struct cq {
	void *base;
	dma_addr_t base_pa;
	struct lif *lif;
	struct cq_info *info;
	struct cq_info *tail;
	struct queue *bound_q;
	struct intr *bound_intr;
	u64 compl_count;
	unsigned int num_descs;
	unsigned int desc_size;
	bool done_color;
};

struct ionic;
int ionic_dev_setup(struct ionic *ionic);

union dev_cmd; //Need to remove it
void ionic_dev_cmd_go(struct ionic_dev *idev, union dev_cmd *cmd);
u8 ionic_dev_cmd_status(struct ionic_dev *idev);
bool ionic_dev_cmd_done(struct ionic_dev *idev);
void ionic_dev_cmd_comp(struct ionic_dev *idev, void *mem);
void ionic_dev_cmd_reset(struct ionic_dev *idev);
void ionic_dev_cmd_port_config(struct ionic_dev *idev, struct port_config *pc);
void ionic_dev_cmd_hang_notify(struct ionic_dev *idev);
void ionic_dev_cmd_identify(struct ionic_dev *idev, u16 ver, dma_addr_t addr);
void ionic_dev_cmd_lif_init(struct ionic_dev *idev, u32 index);
void ionic_dev_cmd_lif_reset(struct ionic_dev *idev, u32 index);
void ionic_dev_cmd_adminq_init(struct ionic_dev *idev, struct queue *adminq,
			       unsigned int index, unsigned int lif_index,
			       unsigned int intr_index);

char *ionic_dev_asic_name(u8 asic_type);
struct doorbell __iomem *ionic_db_map(struct lif *lif, struct queue *q);
int ionic_db_page_num(struct lif *lif, int pid);

void ionic_intr_init(struct ionic_dev *idev, struct intr *intr,
		     unsigned long index);
void ionic_intr_mask_on_assertion(struct intr *intr);
void ionic_intr_return_credits(struct intr *intr, unsigned int credits,
			       bool unmask, bool reset_timer);
void ionic_intr_mask(struct intr *intr, bool mask);
void ionic_intr_coal_set(struct intr *intr, u32 coal_usecs);
int ionic_cq_init(struct lif *lif, struct cq *cq, struct intr *intr,
		  unsigned int num_descs, size_t desc_size);
void ionic_cq_map(struct cq *cq, void *base, dma_addr_t base_pa);
void ionic_cq_bind(struct cq *cq, struct queue *q);
typedef bool (*ionic_cq_cb)(struct cq *cq, struct cq_info *cq_info);
typedef void (*ionic_cq_done_cb)(void *done_arg);
unsigned int ionic_cq_service(struct cq *cq, unsigned int work_to_do,
			      ionic_cq_cb cb, ionic_cq_done_cb done_cb,
			      void *done_arg);

int ionic_q_init(struct lif *lif, struct ionic_dev *idev, struct queue *q,
		 unsigned int index, const char *name, unsigned int num_descs,
		 size_t desc_size, size_t sg_desc_size, unsigned int pid);
void ionic_q_map(struct queue *q, void *base, dma_addr_t base_pa);
void ionic_q_sg_map(struct queue *q, void *base, dma_addr_t base_pa);
void ionic_q_post(struct queue *q, bool ring_doorbell, desc_cb cb,
		  void *cb_arg);
void ionic_q_rewind(struct queue *q, struct desc_info *start);
unsigned int ionic_q_space_avail(struct queue *q);
bool ionic_q_has_space(struct queue *q, unsigned int want);
void ionic_q_service(struct queue *q, struct cq_info *cq_info,
		     unsigned int stop_index);

#endif /* _IONIC_DEV_H_ */
