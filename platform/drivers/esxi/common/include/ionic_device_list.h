/*
 * Copyright (c) 2019 Pensando Systems. All rights reserved.
 */

/*
 * ionic_device_list.h --
 *
 * Definitions for maintaining device list(can be used by mgmt interface)
 */

#ifndef _IONIC_DEVICE_LIST_H_
#define _IONIC_DEVICE_LIST_H_

#include <vmkapi.h>

#define IONIC_DEVICE_LIST_SIZE_HINT     32

typedef vmk_PCIDeviceAddr (*ionic_get_sbdf) (vmk_AddrCookie data);

struct ionic_device_list {
        vmk_HashTable      list;
        vmk_Semaphore      lock;
        ionic_get_sbdf     get_sbdf_cb;
};

VMK_ReturnStatus
ionic_device_list_init(vmk_ModuleID module_id,
                       vmk_HeapID heap_id,
                       ionic_get_sbdf cb,
                       struct ionic_device_list *device_list);

VMK_ReturnStatus
ionic_device_list_destroy(struct ionic_device_list *device_list);

VMK_ReturnStatus
ionic_device_list_flush(struct ionic_device_list *device_list);

VMK_ReturnStatus
ionic_device_list_add(vmk_Name device_name,
                      vmk_AddrCookie data,
                      struct ionic_device_list *device_list);

VMK_ReturnStatus
ionic_device_list_remove(vmk_Name device_name,
                         struct ionic_device_list *device_list,
                         vmk_AddrCookie data);
VMK_ReturnStatus
ionic_device_list_get(vmk_Name device_name,
                      struct ionic_device_list *device_list,
                      vmk_AddrCookie data);

#endif /* End of _IONIC_DEVICE_LIST_H_ */
