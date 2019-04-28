/*
 * Copyright (c) 2019 Pensando Systems. All rights reserved.
 */

/*
 * ionic.h --
 *
 * Definitions shared by ionic_en
 */

#ifndef _IONIC_H_
#define _IONIC_H_

#include <vmkapi.h>
#include "ionic_memory.h"
#include "ionic_locks.h"
#include "ionic_log.h"
#include "ionic_types.h"
#include "ionic_dev.h"
#include "ionic_pci.h"
#include "ionic_rx_filter.h"
#include "ionic_work_queue.h"
#include "ionic_utilities.h"
#include "ionic_time.h"
#include "ionic_dma.h"
#include "ionic_interrupt.h"
#include "ionic_logical_dev_register.h"
#include "ionic_hash.h"
#include "ionic_completion.h"
#include "ionic_lif.h"
#include "ionic_en_uplink.h"
#include "ionic_device_list.h"
#include "ionic_txrx.h"
#include "ionic_en_mq.h"
#include "ionic_api.h"

//#define ADMINQ
#define DRV_DESCRIPTION       "Pensando Ethernet NIC Driver"
#define DRV_VERSION           "0.8"
#define DRV_REL_DATE          "Mar-31-2019"
#define IONIC_MAX_DEVCMD_TIMEOUT 60
#define IONIC_DEFAULT_DEVCMD_TIMEOUT 50
/* In bytes */
#define MEMPOOL_INIT_SIZE     (16 * 1024)
#define MEMPOOL_MAX_SIZE      (1024 * 1024 * 1024)
#define HEAP_INIT_SIZE        (16 * 1024)
#define HEAP_MAX_SIZE         (1024 * 1024 * 1024)
#define MIN_NUM_TX_DESC       128
#define MAX_NUM_TX_DESC       1024 * 16
#define MIN_NUM_RX_DESC       128
#define MAX_NUM_RX_DESC       1024 * 8


extern unsigned int ntxq_descs;
extern unsigned int nrxq_descs;
extern unsigned int DRSS;
extern unsigned int devcmd_timeout;
extern struct ionic_driver ionic_driver;
extern unsigned int vlan_tx_insert;
extern unsigned int vlan_rx_strip;
extern vmk_uint32 log_level;

struct ionic_admin_ctx;

typedef enum ionic_bar {
        IONIC_BAR0              = 0x0,
        IONIC_BAR1              = 0x2,
} ionic_bar;

typedef struct ionic_bitmap {
        vmk_Lock                      lock;
        vmk_BitVector                 *bit_vector;
} ionic_bitmap;

struct ionic_en_device {
        struct ionic_dev              idev;
        vmk_Device                    vmk_device;
        vmk_Device                    uplink_vmk_dev;
        vmk_PCIDevice                 pci_device;
        vmk_PCIDeviceID               pci_device_id;
        vmk_PCIDeviceAddr             sbdf;
        vmk_PCIResource               pci_resources[IONIC_BARS_MAX];
        vmk_VA                        bars[IONIC_BARS_MAX];
        struct ionic_pci_device_entry dev_entry;
};

struct ionic {
        struct ionic_en_device en_dev;
        struct dentry *dentry;
        vmk_uint32 bar0_size;
        struct ionic_dev_bar bars[IONIC_BARS_MAX];
        unsigned int num_bars;
        struct identity ident;
        vmk_Lock lifs_lock;
        vmk_ListLinks lifs;
        vmk_Bool is_mgmt_nic;
        unsigned int nnqs_per_lif;
        unsigned int neqs_per_lif;
        unsigned int ntxqs_per_lif;
        unsigned int nrxqs_per_lif;
        unsigned int nintrs;
        vmk_Mutex dev_cmd_lock;
        ionic_bitmap intrs;
};

struct ionic_driver {
        vmk_Name                      name;
        vmk_ModuleID                  module_id;
        vmk_HeapID                    heap_id;
        vmk_MemPool                   mem_pool;
        vmk_LockDomainID              lock_domain;
        vmk_Driver                    drv_handle;
        vmk_LogComponent              log_component;
        struct ionic_device_list      uplink_dev_list;        
};

struct ionic_en_priv_data {
        vmk_ModuleID                  module_id;
        vmk_HeapID                    heap_id;
        struct ionic                  ionic;
        struct ionic_en_uplink_handle uplink_handle;
        vmk_DMAEngine                 dma_engine_streaming;
        vmk_DMAEngine                 dma_engine_coherent;
        vmk_LockDomainID              lock_domain;
        vmk_MemPool                   mem_pool;
        vmk_IntrCookie                *intr_cookie_array;
        vmk_WorldID                   dev_recover_world;
        vmk_Bool                      is_lifs_size_compl;
};


VMK_ReturnStatus
ionic_pci_query(struct ionic_en_priv_data *priv_data);

VMK_ReturnStatus
ionic_pci_start(struct ionic_en_priv_data *priv_data);

void
ionic_pci_stop(struct ionic_en_priv_data *priv_data);

VMK_ReturnStatus
ionic_adminq_post_wait(struct lif *lif, struct ionic_admin_ctx *ctx);

int ionic_netpoll(int budget, ionic_cq_cb cb, void *cb_arg);

VMK_ReturnStatus
ionic_dev_cmd_wait_check(struct ionic_dev *idev, unsigned long max_wait);

VMK_ReturnStatus
ionic_identify(struct ionic *ionic);

VMK_ReturnStatus
ionic_init(struct ionic *ionic);

VMK_ReturnStatus
ionic_reset(struct ionic *ionic);

VMK_ReturnStatus
ionic_port_identify(struct ionic *ionic);

VMK_ReturnStatus
ionic_port_init(struct ionic *ionic);

VMK_ReturnStatus
ionic_port_reset(struct ionic *ionic);



#endif /* End of _IONIC_H_ */
