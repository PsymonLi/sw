
#include "common.h"

void
mask_all_interrupts( struct ionic *ionic)
{

	struct ionic_dev *idev = &ionic->idev;
	int int_index = 0;

	for( int_index = 0; int_index < ionic->assigned_int_cnt; int_index++) {

		ionic_intr_mask(idev->intr_ctrl, int_index, IONIC_INTR_MASK_SET);
	}

	return;
}

NDIS_STATUS
ionic_map_bars(struct ionic *Adapter,
               PNDIS_MINIPORT_INIT_PARAMETERS InitParameters)
{

    NDIS_STATUS ntStatus = NDIS_STATUS_SUCCESS;
    NDIS_RESOURCE_LIST *pResourceList = NULL;
    ULONG ulIndex = 0;
    CM_PARTIAL_RESOURCE_DESCRIPTOR *pDescriptor = NULL;

	Adapter->assigned_int_cnt = 0;

	if (InitParameters->AllocatedResources == NULL) {
		return NDIS_STATUS_RESOURCES;
	}

    pResourceList = InitParameters->AllocatedResources;

    for (ulIndex = 0; ulIndex < pResourceList->Count; ulIndex++) {

        pDescriptor = &pResourceList->PartialDescriptors[ulIndex];

        switch (pDescriptor->Type) {

        case CmResourceTypeInterrupt: {

            if ((pDescriptor->Flags & CM_RESOURCE_INTERRUPT_MESSAGE) ==
                CM_RESOURCE_INTERRUPT_MESSAGE) {

                DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE,
                          "%s MSI Group %d Int Vec %d Msg cnt %d Aff %I64X\n",
                          __FUNCTION__,
                          pDescriptor->u.MessageInterrupt.Raw.Group,
                          pDescriptor->u.MessageInterrupt.Raw.Vector,
						  pDescriptor->u.MessageInterrupt.Raw.MessageCount,
                          pDescriptor->u.MessageInterrupt.Raw.Affinity));
            } else {
                Adapter->intr_lvl = pDescriptor->u.Interrupt.Level;
                DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE,
                          "%s LEGACY Interrupt level: %d\n", __FUNCTION__,
                          Adapter->intr_lvl));
            }

            Adapter->assigned_int_cnt++;
            break;
        }

        case CmResourceTypePort:
            break;

        case CmResourceTypeMemory: {
            if ((Adapter->num_bars < IONIC_BARS_MAX) &&
                (Adapter->bars[Adapter->num_bars].len == 0)) {

                Adapter->bars[Adapter->num_bars].Flags = pDescriptor->Flags;
                Adapter->bars[Adapter->num_bars].ShareDisposition =
                    pDescriptor->ShareDisposition;
                Adapter->bars[Adapter->num_bars].Type = pDescriptor->Type;

                Adapter->bars[Adapter->num_bars].bus_addr =
                    (ULONG64)pDescriptor->u.Memory.Start.QuadPart;
                Adapter->bars[Adapter->num_bars].len =
                    pDescriptor->u.Memory.Length;

                ntStatus = ionic_map_bar(Adapter, Adapter->num_bars);

                DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE,
                          "%s Bar %d PA %I64X Len %08lX VA %p Status %08lX\n",
                          __FUNCTION__, Adapter->num_bars,
                          Adapter->bars[Adapter->num_bars].bus_addr,
                          Adapter->bars[Adapter->num_bars].len,
                          Adapter->bars[Adapter->num_bars].vaddr, ntStatus));

                if (ntStatus != NDIS_STATUS_SUCCESS) {
                    break;
                }

                Adapter->num_bars++;
            }

            break;
        }

        default:
            DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE,
                      "%s Unsupported resource type %d\n", __FUNCTION__,
                      pDescriptor->Type));
            break;
        }
    }

    return ntStatus;
}

NDIS_STATUS
ionic_map_bar(struct ionic *Adapter, ULONG BarIndex)
{

    NDIS_STATUS ntStatus = NDIS_STATUS_SUCCESS;
    NDIS_PHYSICAL_ADDRESS physAddr = {0};

    physAddr.QuadPart = (ULONGLONG)Adapter->bars[BarIndex].bus_addr;

    ntStatus =
        NdisMMapIoSpace(&Adapter->bars[BarIndex].vaddr, Adapter->adapterhandle,
                        physAddr, Adapter->bars[BarIndex].len);

    if (ntStatus != NDIS_STATUS_SUCCESS) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR,
                  "%s Failed to call NdisMMapIoSpace Status %08lX\n",
                  __FUNCTION__, ntStatus));
    }

    return ntStatus;
}

void
ionic_unmap_bars(struct ionic *Adapter)
{

    ULONG ulIndex = 0;

    for (ulIndex = 0; ulIndex < Adapter->num_bars; ulIndex++) {
        if (Adapter->bars[ulIndex].vaddr != NULL) {
            NdisMUnmapIoSpace(Adapter->adapterhandle,
                              Adapter->bars[ulIndex].vaddr,
                              Adapter->bars[ulIndex].len);

            Adapter->bars[ulIndex].vaddr = NULL;
            Adapter->bars[ulIndex].len = 0;
        }
    }

    Adapter->num_bars = 0;

    return;
}

UCHAR
ioread8(const volatile void *addr)
{
    UCHAR data;

    NdisReadRegisterUchar((PUCHAR)addr, &data);

    return data;
}

USHORT
ioread16(const volatile void *addr)
{
    USHORT data;

    NdisReadRegisterUshort((PUCHAR)addr, &data);

    return data;
}

ULONG
ioread32(const volatile void *addr)
{
    ULONG data;

    NdisReadRegisterUlong((PULONG)addr, &data);

    return data;
}

ULONG64
readq(const volatile void *addr)
{
    ULONG64 data;

    /* read two ulong worth of data */
    READ_REGISTER_BUFFER_ULONG((PULONG)addr, (PULONG)&data, 2);

    return data;
}

void
iowrite32(ULONG data, const volatile void *addr)
{
    NdisWriteRegisterUlong((PULONG)addr, data);
}

void
writeq(u64 data, const volatile void *addr)
{
    NDIS_PHYSICAL_ADDRESS tmp;
    tmp.QuadPart = data;
    WRITE_REGISTER_ULONG64((PULONG64)addr, data);
}

USHORT
get_numa_node(struct ionic *ionic,
			  PROCESSOR_NUMBER *proc_num)
{

	USHORT numa_node = 0;

    for (ULONG i = 0; i < ionic->sys_proc_info->NumberOfProcessors; ++i) {
        PNDIS_PROCESSOR_INFO_EX proc = &ionic->sys_proc[i];

		if (proc_num->Group == proc->ProcNum.Group &&
			proc_num->Number == proc->ProcNum.Number) {
			numa_node = proc->NodeId;
			break;
		}
    }

	return numa_node;
}

NDIS_STATUS
ionic_register_interrupts(struct ionic *ionic,
    PNDIS_MINIPORT_INIT_PARAMETERS init_params)
{
    NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS interrupt;
    NDIS_STATUS status = NDIS_STATUS_NOT_SUPPORTED;
	SIZE_T	rss_tbl_sz = 0;
	NDIS_RSS_PROCESSOR_INFO *rss_tbl = NULL;

    NdisZeroMemory(&interrupt, sizeof(NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS));
    interrupt.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_INTERRUPT;
    interrupt.Header.Revision = NDIS_MINIPORT_INTERRUPT_REVISION_1;
    interrupt.Header.Size = sizeof(NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS);
    interrupt.MsiSupported = TRUE;
    interrupt.MsiSyncWithAllMessages = FALSE;
    interrupt.MessageInterruptHandler = ionic_msi_handler;
    interrupt.MessageInterruptDpcHandler = ionic_msi_dpc_handler;
    interrupt.DisableMessageInterruptHandler = ionic_msi_disable;
    interrupt.EnableMessageInterruptHandler = ionic_msi_enable;
    status = NdisMRegisterInterruptEx(ionic->adapterhandle, ionic,
                                      &interrupt, &ionic->intr_obj);
    if (status != NDIS_STATUS_SUCCESS) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR,
                  "Unable to register interrupt status: %x ionic: 0x%p\n",
                  status, ionic));
        goto exit;
    }

	DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE,
		"Interrupt Type: 0x%x ionic: 0x%p\n", interrupt.InterruptType,
		ionic));
	ionic->intr_type = interrupt.InterruptType;
	ionic->intr_msginfo_tbl = interrupt.MessageInfoTable;
	_dump_message_table(ionic->intr_msginfo_tbl);

    /*
        Setup the interrupt message table
    */

    ionic->intr_msg_tbl =
        (struct intr_msg*)NdisAllocateMemoryWithTagPriority_internal(
            ionic->adapterhandle,
            ionic->intr_msginfo_tbl->MessageCount * sizeof(struct intr_msg),
            IONIC_INT_TABLE_TAG, NormalPoolPriority);

    if (ionic->intr_msg_tbl == NULL) {
        status = NDIS_STATUS_RESOURCES;
        goto exit;
    }

    NdisZeroMemory(ionic->intr_msg_tbl,
        ionic->intr_msginfo_tbl->MessageCount * sizeof(struct intr_msg));

    if (ionic->intr_msg_tbl == NULL) {
        status = NDIS_STATUS_FAILURE;
        goto exit;
    }

	PCM_PARTIAL_RESOURCE_LIST prl = init_params->AllocatedResources;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
	ULONG msg_id;

	msg_id = 0;

    for (ULONG prd_idx = 0; prd_idx < prl->Count; ++prd_idx) {

        prd = &prl->PartialDescriptors[prd_idx];

        if (prd->Type == CmResourceTypeInterrupt && (prd->Flags & CM_RESOURCE_INTERRUPT_MESSAGE)) {

            ionic->intr_msg_tbl[msg_id].id = msg_id;
            ionic->intr_msg_tbl[msg_id].proc.Group = prd->u.MessageInterrupt.Raw.Group;
            if (RtlNumberOfSetBitsUlongPtr((ULONG_PTR)prd->u.MessageInterrupt.Raw.Affinity) == 1) {
                ionic->intr_msg_tbl[msg_id].proc.Number =
                    RtlFindLeastSignificantBit(prd->u.MessageInterrupt.Raw.Affinity);
                ionic->intr_msg_tbl[msg_id].affinity_policy = IrqPolicySpecifiedProcessors;
                ionic->intr_msg_tbl[msg_id].proc_idx = KeGetProcessorIndexFromNumber(&ionic->intr_msg_tbl[msg_id].proc);
            }
            else {
                ionic->intr_msg_tbl[msg_id].affinity_policy = IrqPolicyAllCloseProcessors;
                ionic->intr_msg_tbl[msg_id].proc_idx = INVALID_PROCESSOR_INDEX;
            }
			ionic->intr_msg_tbl[msg_id].affinity = prd->u.MessageInterrupt.Raw.Affinity;

			DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE,
				"%s msg_id %d affinity_policy %d group %u proc %d affinity 0x%llx\n",
				__FUNCTION__,
				ionic->intr_msg_tbl[msg_id].id,
                ionic->intr_msg_tbl[msg_id].affinity_policy,
                ionic->intr_msg_tbl[msg_id].proc.Group,
                ionic->intr_msg_tbl[msg_id].proc.Number,
                ionic->intr_msg_tbl[msg_id].affinity));

            ++msg_id;
        }
    }

	// Grab the RSS processor list and mark the entries below if they are to be used for RSS
	status = NdisGetRssProcessorInformation( ionic->adapterhandle,
											 NULL,
											 &rss_tbl_sz);

	if( status != NDIS_STATUS_BUFFER_TOO_SHORT) {
        status = NDIS_STATUS_RESOURCES;
        goto exit;
	}

	rss_tbl =
        (NDIS_RSS_PROCESSOR_INFO *)NdisAllocateMemoryWithTagPriority_internal(
            ionic->adapterhandle,
            (ULONG)rss_tbl_sz,
            IONIC_GENERIC_TAG, NormalPoolPriority);

	if (rss_tbl == NULL) {
        status = NDIS_STATUS_RESOURCES;
        goto exit;
	}

	NdisZeroMemory( rss_tbl, rss_tbl_sz);

	status = NdisGetRssProcessorInformation( ionic->adapterhandle,
											 rss_tbl,
											 &rss_tbl_sz);

	if( status != NDIS_STATUS_SUCCESS) {
		NdisFreeMemoryWithTagPriority_internal( ionic->adapterhandle,
												rss_tbl,
												IONIC_GENERIC_TAG);
        goto exit;
	}

    for (ULONG i = 0; i < ionic->intr_msginfo_tbl->MessageCount; ++i) {
        ionic->intr_msg_tbl[i].id = i;

		ionic->intr_msg_tbl[i].numa_node = get_numa_node( ionic,
														  &ionic->intr_msg_tbl[i].proc);

        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_VERBOSE,
			"%s msg %lu vec 0x%lx addr 0x%lx data 0x%lx affinity %lx NUMA %d\n",
            __FUNCTION__, i,
            ionic->intr_msginfo_tbl->MessageInfo[i].Vector,
            ionic->intr_msginfo_tbl->MessageInfo[i].MessageAddress,
            ionic->intr_msginfo_tbl->MessageInfo[i].MessageData,
            ionic->intr_msginfo_tbl->MessageInfo[i].TargetProcessorSet,
			ionic->intr_msg_tbl[i].numa_node));
    }

	NdisFreeMemoryWithTagPriority_internal( ionic->adapterhandle,
											rss_tbl,
											IONIC_GENERIC_TAG);
    
	/*
        Setup the interrupt resource table
    */
    ionic->intr_tbl =
        (struct intr*)NdisAllocateMemoryWithTagPriority_internal(
            ionic->adapterhandle,
            ionic->ident.dev.nintrs * sizeof(struct intr),
            IONIC_INT_TABLE_TAG, NormalPoolPriority);

    if (ionic->intr_tbl == NULL) {
        status = NDIS_STATUS_RESOURCES;
        goto exit;
    }

    NdisZeroMemory(ionic->intr_tbl, ionic->ident.dev.nintrs * sizeof(struct intr));

    if (ionic->intr_tbl == NULL) {
        status = NDIS_STATUS_FAILURE;
        goto exit;
    }

    /*
        Setup the default mapping of interrupt resources to messages. Initially, the OS sets up the mapping of
        in the following manner.

        Each message interrupt resource in the list is assigned a message number later that corresponds to the
        order it shows in the list. For example, the first message interrupt resources in the list is assigned
        to message 0, the second one is assigned to message 1, and so on.

        Source: https://docs.microsoft.com/en-us/windows-hardware/drivers/network/msi-x-resource-filtering
    */

    for (ULONG i = 0; i < ionic->ident.dev.nintrs; ++i) {
        ionic->intr_tbl[i].index = i;
    }

    return status;

exit:

	if (ionic->intr_tbl != NULL) {
		NdisFreeMemoryWithTagPriority_internal(ionic->adapterhandle,
			ionic->intr_tbl,
			IONIC_INT_TABLE_TAG);
		ionic->intr_tbl = NULL;
	}

	if (ionic->intr_msg_tbl != NULL) {
		NdisFreeMemoryWithTagPriority_internal(ionic->adapterhandle,
			ionic->intr_msg_tbl,
			IONIC_INT_TABLE_TAG);
		ionic->intr_msg_tbl = NULL;
    }

    return status;
}

BOOLEAN
ionic_msi_handler(PVOID miniport_interrupt_context,
                  ULONG message_id,
                  PBOOLEAN queue_default_interrupt_dpc,
                  PULONG target_processors)
{
    struct ionic *ionic = (struct ionic *)miniport_interrupt_context;
    struct intr_msg* int_tbl = NULL;

    UNREFERENCED_PARAMETER(target_processors);

    /* Only receive interrupts on the adminq/notifyq if we're not ready for them */
	if (ionic->hardware_status != NdisHardwareStatusReady &&
		message_id != 0) {
		IoPrint("%s Have msg id %d before being ready\n", __FUNCTION__, message_id);
		goto exit;
	}

    check_intr_msg_affinity(ionic, message_id);

    int_tbl = get_intr_msg(ionic, message_id);
    if (int_tbl == NULL) {
        goto exit;
    }
#ifdef DBG
    InterlockedIncrement64(&int_tbl->isr_cnt);
#endif

exit:
    *queue_default_interrupt_dpc = TRUE;

    return TRUE;
}

VOID
ionic_msi_dpc_handler(NDIS_HANDLE miniport_interrupt_context,
                      ULONG message_id,
                      PVOID miniport_dpc_context,
                      PVOID receive_throttle_params,
                      PVOID ndis_reserved2)
{
    struct ionic *ionic = (struct ionic *)miniport_interrupt_context;
    struct lif *lif;
    u32 ndis_budget = 0;
    u32 budget = 0;
    struct intr_msg *int_tbl = NULL;
	ULONG credits = 0;
	NDIS_RECEIVE_THROTTLE_PARAMETERS *throttle_params = (NDIS_RECEIVE_THROTTLE_PARAMETERS *)receive_throttle_params;
	ULONG flags = 0;
	struct qcq *tx_qcq = NULL;
	struct qcq *rx_qcq = NULL;

	UNREFERENCED_PARAMETER(miniport_dpc_context);
    UNREFERENCED_PARAMETER(ndis_reserved2);
   
    check_intr_msg_affinity(ionic, message_id);

    int_tbl = get_intr_msg(ionic, message_id);
    if (int_tbl == NULL) {
        IoPrint("%s invalid msg_id %d\n", __FUNCTION__, message_id);
        return;
    }

#ifdef DBG
    InterlockedIncrement64(&int_tbl->dpc_cnt);
#endif

	lif = (struct lif *)InterlockedCompareExchangePointer( (PVOID *)&int_tbl->lif,
														   NULL,
														   (PVOID)MAXULONG_PTR);

	rx_qcq = (struct qcq *)InterlockedCompareExchangePointer( (PVOID *)&int_tbl->rx_qcq,
														   NULL,
														   (PVOID)MAXULONG_PTR);

	tx_qcq = (struct qcq *)InterlockedCompareExchangePointer( (PVOID *)&int_tbl->tx_qcq,
														   NULL,
														   (PVOID)MAXULONG_PTR);
	
	if (lif == NULL) {
		IoPrint("%s unbound msg_id %d\n", __FUNCTION__, message_id);
		return;
	}

    ref_request(lif);

    DbgTrace((TRACE_COMPONENT_INTERRUPT, TRACE_LEVEL_VERBOSE,
         "%s Enter Adapter %p Lif %p ionic_msi_dpc_handler msg_id: 0x%08lX\n",
         __FUNCTION__, ionic, lif, message_id));

    if (int_tbl->admin_qcq != NULL) {

        if (likely(lif->notifyqcq && (lif->notifyqcq->flags & QCQ_F_INITED))) {
            ionic_notifyq_clean(lif, (unsigned int)-1);
        }

        if (likely(lif->adminqcq && (lif->adminqcq->flags & QCQ_F_INITED))) {
            ionic_napi(lif, (unsigned int)-1, ionic_adminq_service, NULL, NULL);
        }

    } else {
		ndis_budget = throttle_params->MaxNblsToIndicate;

		if (rx_qcq != NULL) {
			budget = IONIC_RX_BUDGET_DEFAULT;
			if (ionic->RxBudget != 0) {
				budget = ionic->RxBudget;
			} else if (ndis_budget != 0) {
				budget = ndis_budget;
			}
			if (budget > rx_qcq->cq.num_descs) {
				budget = rx_qcq->cq.num_descs;
			}
			throttle_params->MaxNblsToIndicate = budget;

			ionic_rx_napi( lif, rx_qcq, throttle_params);

			credits = budget - throttle_params->MaxNblsToIndicate;
		}

		if (tx_qcq != NULL) {
			budget = IONIC_TX_BUDGET_DEFAULT;
			if (ionic->TxBudget != 0) {
				budget = ionic->TxBudget;
			} else if (ndis_budget != 0) {
				budget = ndis_budget;
			}
			if (budget > tx_qcq->cq.num_descs) {
				budget = tx_qcq->cq.num_descs;
			}
			throttle_params->MaxNblsToIndicate = budget;

			tx_packet_dpc_callback(tx_qcq, throttle_params);

			credits += budget - throttle_params->MaxNblsToIndicate;
		}

		if( !throttle_params->MoreNblsPending) {
			flags = IONIC_INTR_CRED_UNMASK;
		}

		if( credits != 0 ||
			flags != 0) {
			flags |= IONIC_INTR_CRED_RESET_COALESCE;
			ionic_intr_credits(ionic->idev.intr_ctrl, int_tbl->int_index, credits,
								flags);
		}
	}

	deref_request(lif, 1);
}

VOID
ionic_msi_disable(PVOID miniport_interrupt_context, ULONG message_id)
{
    struct ionic *ionic = (struct ionic *)miniport_interrupt_context;

    UNREFERENCED_PARAMETER(message_id);
    UNREFERENCED_PARAMETER(ionic);

    DbgTrace((TRACE_COMPONENT_INTERRUPT, TRACE_LEVEL_VERBOSE,
              "%s Entry %p MsgId 0x%08lX\n", __FUNCTION__, ionic, message_id));

    return;
}

VOID
ionic_msi_enable(PVOID miniport_interrupt_context, ULONG message_id)
{
    struct ionic *ionic = (struct ionic *)miniport_interrupt_context;

    UNREFERENCED_PARAMETER(message_id);
    UNREFERENCED_PARAMETER(ionic);

    DbgTrace((TRACE_COMPONENT_INTERRUPT, TRACE_LEVEL_VERBOSE,
              "%s Entry %p MsgId 0x%08lX\n", __FUNCTION__, ionic, message_id));

    return;
}

void
ionic_process_sg_list(PDEVICE_OBJECT DeviceObject,
                      PVOID Irp,
                      PSCATTER_GATHER_LIST sg_list,
                      PVOID Context)
{

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    struct txq_pkt *txq_pkt = (struct txq_pkt *)Context;

    DbgTrace((TRACE_COMPONENT_IO, TRACE_LEVEL_VERBOSE,
              "%s Enter Process SG List NB %p\n", __FUNCTION__,
              txq_pkt->packet_orig));

    txq_pkt->sg_os_list = sg_list;

	return;
}

NDIS_STATUS
ionic_bus_alloc_irq_vectors(struct ionic *ionic, unsigned int nintrs)
{
    UNREFERENCED_PARAMETER(ionic);
    UNREFERENCED_PARAMETER(nintrs);

    return NDIS_STATUS_SUCCESS;
}

int
ionic_bus_get_irq(struct ionic *ionic, unsigned int num)
{

    UNREFERENCED_PARAMETER(ionic);
    UNREFERENCED_PARAMETER(num);

    return 0;
}

u64 __iomem *
ionic_bus_map_dbpage(struct ionic *ionic, u32 db_page_offset)
{

    u64 __iomem *db_page = NULL;

    db_page = (u64 __iomem *)((char *)ionic->idev.db_pages +
                              (db_page_offset * PAGE_SIZE));

    return db_page;
}

void
ionic_bus_unmap_dbpage(struct ionic *ionic, u64 *db_page)
{

    UNREFERENCED_PARAMETER(ionic);
    UNREFERENCED_PARAMETER(db_page);

    return;
}

unsigned int
ionic_intr_alloc(struct ionic* ionic)
{
    unsigned int index = 0;

    index = RtlFindClearBits(&ionic->intrs, 1, 0);
    if (index != (ULONG)-1) {
        RtlSetBit(&ionic->intrs, (ULONG)index);
            }

    return (unsigned int)index;
}

void
ionic_intr_free(struct ionic *ionic, unsigned int index)
{
    if (index != INTR_INDEX_NOT_ASSIGNED && index < ionic->nintrs)
        RtlClearBit(&ionic->intrs, (ULONG)index);
}

NDIS_STATUS
ionic_intr_affinitize(struct ionic* ionic, unsigned int index, ULONG message_id)
{
    struct intr_msg* intr_msg = &ionic->intr_msg_tbl[message_id];
    struct intr* intr = &ionic->intr_tbl[index];
    NDIS_MSIX_CONFIG_PARAMETERS int_cfg;
    NDIS_STATUS status;

    int_cfg.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    int_cfg.Header.Revision = NDIS_MSIX_CONFIG_PARAMETERS_REVISION_1;
    int_cfg.Header.Size = NDIS_SIZEOF_MSIX_CONFIG_PARAMETERS_REVISION_1;
    int_cfg.ConfigOperation = NdisMSIXTableConfigSetTableEntry;
    int_cfg.TableEntry = index;
    int_cfg.MessageNumber = message_id;

    intr_msg->lif = intr->lif;
	if( intr->qcq->q.type == IONIC_QTYPE_ADMINQ) {
		intr_msg->admin_qcq = intr->qcq;
	}
	else if( intr->qcq->q.type == IONIC_QTYPE_RXQ) {
		intr_msg->rx_qcq = intr->qcq;
	}
	else if (intr->qcq->q.type == IONIC_QTYPE_TXQ) {
		intr_msg->tx_qcq = intr->qcq;
	}

	intr_msg->int_index = index;

    status = NdisMConfigMSIXTableEntry(ionic->adapterhandle, &int_cfg);
    if (status != NDIS_STATUS_SUCCESS) {
        IoPrint("%s tbl_ent %d msg_id %d failed %08lX\n",
            __FUNCTION__,
            int_cfg.TableEntry, int_cfg.MessageNumber,
            status);
    }

    return status;
}

void
ionic_intr_init(struct ionic *ionic, unsigned int index)
{
    ionic_intr_clean(ionic->idev.intr_ctrl, index);
}

int
ionic_db_page_num(struct lif *lif, int pid)
{
    return (lif->hw_index * lif->dbid_count) + pid;
}

struct intr_msg *
get_intr_msg(struct ionic *ionic, ULONG message_id)
{
    if (message_id < ionic->intr_msginfo_tbl->MessageCount) {
        return &ionic->intr_msg_tbl[message_id];
    }

    return NULL;
}

void
invoke_intr_msgs_rss(struct ionic *ionic, struct lif *lif)
{
    struct intr_msg* intr;
	GROUP_AFFINITY affinity;

    /* don't touch the control interrupt */
    for (ULONG i = IONIC_CTL_INTR_CNT; i < ionic->intr_msginfo_tbl->MessageCount; ++i) {
        intr = &ionic->intr_msg_tbl[i];

        if (intr->lif == lif && intr->inuse) {

			NdisZeroMemory( &affinity,
							sizeof(GROUP_AFFINITY));

			affinity.Mask = (ULONG_PTR)1 << intr->proc.Number;
			affinity.Group = intr->proc.Group;

			NdisMQueueDpcEx( ionic->intr_obj,
							 intr->id,
							 &affinity,
							 NULL);
        }
    }
}

void
dump_intr_tbl(struct ionic *ionic)
{
    struct intr_msg* intr;

    /* don't touch the control interrupt */
    for (ULONG i = IONIC_CTL_INTR_CNT; i < ionic->intr_msginfo_tbl->MessageCount; ++i) {
        intr = &ionic->intr_msg_tbl[i];
		if( intr->inuse) {
			DbgTrace((TRACE_COMPONENT_QUEUE_INIT, TRACE_LEVEL_VERBOSE,
								"%s Msg %d Core %d\n",
								__FUNCTION__,
								i,
								intr->proc_idx));
		}
    }

	return;
}

void
unuse_intr_msgs_rss(struct ionic *ionic, struct lif *lif)
{
    struct intr_msg* intr;

    /* don't touch the control interrupt */
    for (ULONG i = IONIC_CTL_INTR_CNT; i < ionic->intr_msginfo_tbl->MessageCount; ++i) {
        intr = &ionic->intr_msg_tbl[i];

        if (intr->lif == lif) {
            intr->inuse = false;
			intr->lif = NULL;
			intr->rx_qcq = NULL;
			intr->tx_qcq = NULL;
        }
    }
}

struct intr_msg *
find_intr_msg(struct ionic *ionic, ULONG proc_idx, ULONG proximity)
{
    struct intr_msg* intr;

    for (ULONG i = 0; i < ionic->intr_msginfo_tbl->MessageCount; ++i) {
        intr = &ionic->intr_msg_tbl[i];
        if (!intr->inuse) {
            if (proximity == ALL_CLOSE_PROCESSORS &&
                intr->affinity_policy == IrqPolicyAllCloseProcessors) {
                return intr;
            }
            if (proximity == ANY_PROCESSOR_INDEX) {
				return intr;
            }
            if (proximity == NOT_PROCESSOR_INDEX &&
				intr->proc_idx != proc_idx) {
				return intr;
            }
			if( proximity == ANY_PROCESSOR_CLOSE_INDEX &&
				intr->numa_node == ionic->numa_node) {
				return intr;
			}
            if (proximity == MATCH_PROCESSOR_INDEX &&
                intr->affinity_policy == IrqPolicySpecifiedProcessors &&
				intr->proc_idx == proc_idx) {
                return intr;
            }
        }
    }

    return NULL;
}

BOOLEAN
sync_intr_msg(NDIS_HANDLE SynchronizeContext)
{
    struct intr_sync_ctx* ctx = (struct intr_sync_ctx *)SynchronizeContext;
    NDIS_STATUS status;

    status = ionic_intr_affinitize(ctx->lif->ionic, ctx->index, ctx->id);

    return (status == NDIS_STATUS_SUCCESS);
}

void
check_intr_msg_affinity(struct ionic* ionic, ULONG message_id)
{
    struct intr_msg* int_tbl = NULL;

    int_tbl = get_intr_msg(ionic, message_id);
    if (int_tbl == NULL) {
        IoPrint("%s invalid msg %d\n", message_id);
        return;
        }

	PROCESSOR_NUMBER proc_numb;
	KeGetCurrentProcessorNumberEx(&proc_numb);

    if (int_tbl->lif == NULL) {
        IoPrint("%s not bound - msg %d group %d proc %d affinity %d\n",
            __FUNCTION__,
            message_id,
            proc_numb.Group, proc_numb.Number);
        return;
    }

    if (int_tbl->affinity_policy == IrqPolicySpecifiedProcessors &&
        !(int_tbl->affinity & (1ull << proc_numb.Number))) {
        IoPrint("%s wrong affinity - msg %d group %d proc %d affinity %d\n",
            __FUNCTION__,
            message_id,
            proc_numb.Group, proc_numb.Number,
            int_tbl->affinity);
    }
}

void
ionic_set_offload_hw_capabilities(struct ionic *ionic,
                                  PNDIS_OFFLOAD HardwareOffload)
{

    struct lif *lif = ionic->master_lif;

    NdisZeroMemory(HardwareOffload, sizeof(NDIS_OFFLOAD));

    HardwareOffload->Header.Type = NDIS_OBJECT_TYPE_OFFLOAD;

#if (NDIS_SUPPORT_NDIS650)
    HardwareOffload->Header.Revision = NDIS_OFFLOAD_REVISION_4;
    HardwareOffload->Header.Size = NDIS_SIZEOF_NDIS_OFFLOAD_REVISION_4;
#endif

#if (NDIS_SUPPORT_NDIS670)
    HardwareOffload->Header.Revision = NDIS_OFFLOAD_REVISION_5;
    HardwareOffload->Header.Size = NDIS_SIZEOF_NDIS_OFFLOAD_REVISION_5;
#endif

    if (lif->hw_features & ETH_HW_TX_CSUM) {

        HardwareOffload->Checksum.IPv4Transmit.Encapsulation =
            NDIS_ENCAPSULATION_IEEE_802_3;
        HardwareOffload->Checksum.IPv4Transmit.IpOptionsSupported =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv4Transmit.TcpOptionsSupported =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv4Transmit.TcpChecksum =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv4Transmit.UdpChecksum =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv4Transmit.IpChecksum =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv6Transmit.Encapsulation =
            NDIS_ENCAPSULATION_IEEE_802_3;
        HardwareOffload->Checksum.IPv6Transmit.IpExtensionHeadersSupported =
            NDIS_OFFLOAD_NOT_SUPPORTED;
        HardwareOffload->Checksum.IPv6Transmit.TcpOptionsSupported =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv6Transmit.TcpChecksum =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv6Transmit.UdpChecksum =
            NDIS_OFFLOAD_SUPPORTED;
    }

    if (lif->hw_features & ETH_HW_RX_CSUM) {

        HardwareOffload->Checksum.IPv4Receive.Encapsulation =
            NDIS_ENCAPSULATION_IEEE_802_3;
        HardwareOffload->Checksum.IPv4Receive.IpOptionsSupported =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv4Receive.TcpOptionsSupported =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv4Receive.TcpChecksum =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv4Receive.UdpChecksum =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv4Receive.IpChecksum =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv6Receive.Encapsulation =
            NDIS_ENCAPSULATION_IEEE_802_3;
        HardwareOffload->Checksum.IPv6Receive.IpExtensionHeadersSupported =
            NDIS_OFFLOAD_NOT_SUPPORTED;
        HardwareOffload->Checksum.IPv6Receive.TcpOptionsSupported =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv6Receive.TcpChecksum =
            NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->Checksum.IPv6Receive.UdpChecksum =
            NDIS_OFFLOAD_SUPPORTED;
    }

    if (lif->hw_features & ETH_HW_TSO) {
        HardwareOffload->LsoV1.IPv4.Encapsulation =
            NDIS_ENCAPSULATION_IEEE_802_3;
        HardwareOffload->LsoV1.IPv4.MaxOffLoadSize = IONIC_NDIS_MAX_LSO_PKT;
        HardwareOffload->LsoV1.IPv4.MinSegmentCount = 2;
        HardwareOffload->LsoV1.IPv4.TcpOptions = NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->LsoV1.IPv4.IpOptions = NDIS_OFFLOAD_SUPPORTED;
        HardwareOffload->LsoV2.IPv4.Encapsulation =
            NDIS_ENCAPSULATION_IEEE_802_3;
        HardwareOffload->LsoV2.IPv4.MaxOffLoadSize = IONIC_NDIS_MAX_LSO_PKT;
        HardwareOffload->LsoV2.IPv4.MinSegmentCount = 2;
        HardwareOffload->LsoV2.IPv6.Encapsulation =
            NDIS_ENCAPSULATION_IEEE_802_3;
        HardwareOffload->LsoV2.IPv6.MaxOffLoadSize = IONIC_NDIS_MAX_LSO_PKT;
        HardwareOffload->LsoV2.IPv6.MinSegmentCount = 2;
        HardwareOffload->LsoV2.IPv6.IpExtensionHeadersSupported = 0;
        HardwareOffload->LsoV2.IPv6.TcpOptionsSupported =
            NDIS_OFFLOAD_SUPPORTED;
    }

    HardwareOffload->Rsc.IPv4.Enabled = TRUE;
    HardwareOffload->Rsc.IPv6.Enabled = TRUE;

    return;
}

void
ionic_set_offload_default_capabilities(struct ionic *ionic,
                                       PNDIS_OFFLOAD DefaultOffload)
{

    NdisZeroMemory(DefaultOffload, sizeof(NDIS_OFFLOAD));

    DefaultOffload->Header.Type = NDIS_OBJECT_TYPE_OFFLOAD;

#if (NDIS_SUPPORT_NDIS650)
    DefaultOffload->Header.Revision = NDIS_OFFLOAD_REVISION_4;
    DefaultOffload->Header.Size = NDIS_SIZEOF_NDIS_OFFLOAD_REVISION_4;
#endif

#if (NDIS_SUPPORT_NDIS670)
    DefaultOffload->Header.Revision = NDIS_OFFLOAD_REVISION_5;
    DefaultOffload->Header.Size = NDIS_SIZEOF_NDIS_OFFLOAD_REVISION_5;
#endif

    DefaultOffload->Checksum.IPv4Transmit.Encapsulation =
        NDIS_ENCAPSULATION_IEEE_802_3;
    DefaultOffload->Checksum.IPv4Transmit.IpOptionsSupported =
        NDIS_OFFLOAD_SUPPORTED;
    DefaultOffload->Checksum.IPv4Transmit.TcpOptionsSupported =
        NDIS_OFFLOAD_SUPPORTED;
    DefaultOffload->Checksum.IPv4Transmit.TcpChecksum = ionic->tcpv4_tx_state;
    DefaultOffload->Checksum.IPv4Transmit.UdpChecksum = ionic->udpv4_tx_state;
    DefaultOffload->Checksum.IPv4Transmit.IpChecksum = ionic->ipv4_tx_state;

    DefaultOffload->Checksum.IPv6Transmit.Encapsulation =
        NDIS_ENCAPSULATION_IEEE_802_3;
    DefaultOffload->Checksum.IPv6Transmit.IpExtensionHeadersSupported =
        NDIS_OFFLOAD_NOT_SUPPORTED;
    DefaultOffload->Checksum.IPv6Transmit.TcpOptionsSupported =
        NDIS_OFFLOAD_SUPPORTED;
    DefaultOffload->Checksum.IPv6Transmit.TcpChecksum = ionic->tcpv6_tx_state;
    DefaultOffload->Checksum.IPv6Transmit.UdpChecksum = ionic->udpv6_tx_state;

    DefaultOffload->Checksum.IPv4Receive.Encapsulation =
        NDIS_ENCAPSULATION_NULL;
    DefaultOffload->Checksum.IPv4Receive.IpOptionsSupported =
        NDIS_OFFLOAD_SUPPORTED;
    DefaultOffload->Checksum.IPv4Receive.TcpOptionsSupported =
        NDIS_OFFLOAD_SUPPORTED;
    DefaultOffload->Checksum.IPv4Receive.TcpChecksum = ionic->tcpv4_rx_state;
    DefaultOffload->Checksum.IPv4Receive.UdpChecksum = ionic->udpv4_rx_state;
    DefaultOffload->Checksum.IPv4Receive.IpChecksum = ionic->ipv4_rx_state;

    DefaultOffload->Checksum.IPv6Receive.Encapsulation =
        NDIS_ENCAPSULATION_NULL;
    DefaultOffload->Checksum.IPv6Receive.IpExtensionHeadersSupported =
        NDIS_OFFLOAD_NOT_SUPPORTED;
    DefaultOffload->Checksum.IPv6Receive.TcpOptionsSupported =
        NDIS_OFFLOAD_SUPPORTED;
    DefaultOffload->Checksum.IPv6Receive.TcpChecksum = ionic->tcpv6_rx_state;
    DefaultOffload->Checksum.IPv6Receive.UdpChecksum = ionic->udpv6_rx_state;

    if (ionic->lsov2ipv4_state == NDIS_OFFLOAD_PARAMETERS_LSOV2_ENABLED) {
        DefaultOffload->LsoV2.IPv4.Encapsulation =
            NDIS_ENCAPSULATION_IEEE_802_3;
        DefaultOffload->LsoV2.IPv4.MaxOffLoadSize = IONIC_NDIS_MAX_LSO_PKT;
        DefaultOffload->LsoV2.IPv4.MinSegmentCount = 2;
    }

    if (ionic->lsov2ipv6_state == NDIS_OFFLOAD_PARAMETERS_LSOV2_ENABLED) {
        DefaultOffload->LsoV2.IPv6.Encapsulation =
            NDIS_ENCAPSULATION_IEEE_802_3;
        DefaultOffload->LsoV2.IPv6.MaxOffLoadSize = IONIC_NDIS_MAX_LSO_PKT;
        DefaultOffload->LsoV2.IPv6.MinSegmentCount = 2;
        DefaultOffload->LsoV2.IPv6.IpExtensionHeadersSupported = 0;
        DefaultOffload->LsoV2.IPv6.TcpOptionsSupported = NDIS_OFFLOAD_SUPPORTED;
    }

    if (ionic->lsov1_state == NDIS_OFFLOAD_PARAMETERS_LSOV1_ENABLED) {
        DefaultOffload->LsoV1.IPv4.Encapsulation =
            NDIS_ENCAPSULATION_IEEE_802_3;
        DefaultOffload->LsoV1.IPv4.IpOptions = NDIS_OFFLOAD_SUPPORTED;
        DefaultOffload->LsoV1.IPv4.MaxOffLoadSize = IONIC_NDIS_MAX_LSO_PKT;
        DefaultOffload->LsoV1.IPv4.MinSegmentCount = 2;
        DefaultOffload->LsoV1.IPv4.TcpOptions = NDIS_OFFLOAD_SUPPORTED;
    }

    DefaultOffload->Rsc.IPv4.Enabled = ionic->rscv4_enabled;
    DefaultOffload->Rsc.IPv6.Enabled = ionic->rscv6_enabled;
}

NDIS_STATUS
ionic_set_offload_attributes(struct ionic *ionic)
{
    NDIS_MINIPORT_ADAPTER_OFFLOAD_ATTRIBUTES adapterOffloadAttribs;
    NDIS_OFFLOAD stDefaultOffload;
    NDIS_OFFLOAD stHardwareOffload;

    NdisZeroMemory(&adapterOffloadAttribs,
                   sizeof(NDIS_MINIPORT_ADAPTER_OFFLOAD_ATTRIBUTES));

    adapterOffloadAttribs.Header.Type =
        NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_OFFLOAD_ATTRIBUTES;
    adapterOffloadAttribs.Header.Revision =
        NDIS_MINIPORT_ADAPTER_OFFLOAD_ATTRIBUTES_REVISION_1;
    adapterOffloadAttribs.Header.Size =
        sizeof(NDIS_MINIPORT_ADAPTER_OFFLOAD_ATTRIBUTES);

    ionic_set_offload_hw_capabilities(ionic, &stHardwareOffload);
    ionic_set_offload_default_capabilities(ionic, &stDefaultOffload);

    adapterOffloadAttribs.DefaultOffloadConfiguration = &stDefaultOffload;
    adapterOffloadAttribs.HardwareOffloadCapabilities = &stHardwareOffload;

    return NdisMSetMiniportAttributes(
        ionic->adapterhandle,
        (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&adapterOffloadAttribs);
}

NDIS_STATUS
oid_handle_offload_encapsulation(struct ionic *ionic,
                                 VOID *info_buffer,
                                 ULONG info_buffer_length,
                                 ULONG *bytes_read,
                                 ULONG *bytes_needed)
{
    NDIS_STATUS ntStatus = NDIS_STATUS_SUCCESS;
    PNDIS_OFFLOAD_ENCAPSULATION pOffload =
        (PNDIS_OFFLOAD_ENCAPSULATION)info_buffer;
    NDIS_OFFLOAD_ENCAPSULATION encap_indicated;

    UNREFERENCED_PARAMETER(ionic);

    if (info_buffer_length < sizeof(NDIS_OFFLOAD_ENCAPSULATION)) {
        *bytes_needed = sizeof(NDIS_OFFLOAD_ENCAPSULATION);
        *bytes_read = 0;
        return NDIS_STATUS_INVALID_LENGTH;
    }

    *bytes_read = sizeof(NDIS_OFFLOAD_ENCAPSULATION);
    *bytes_needed = 0;

    if (pOffload->IPv4.Enabled == NDIS_OFFLOAD_SET_ON &&
        pOffload->IPv4.EncapsulationType != NDIS_ENCAPSULATION_IEEE_802_3) {
        ntStatus = NDIS_STATUS_NOT_SUPPORTED;
        goto cleanup;
    }

    if (pOffload->IPv6.Enabled == NDIS_OFFLOAD_SET_ON &&
        pOffload->IPv6.EncapsulationType != NDIS_ENCAPSULATION_IEEE_802_3) {
        ntStatus = NDIS_STATUS_NOT_SUPPORTED;
        goto cleanup;
    }

    NdisZeroMemory(&encap_indicated, sizeof(NDIS_OFFLOAD_ENCAPSULATION));

    encap_indicated.Header.Revision = NDIS_OFFLOAD_ENCAPSULATION_REVISION_1;
    encap_indicated.Header.Size = NDIS_SIZEOF_OFFLOAD_ENCAPSULATION_REVISION_1;
    encap_indicated.Header.Type = NDIS_OBJECT_TYPE_OFFLOAD_ENCAPSULATION;

    encap_indicated.IPv4.Enabled = pOffload->IPv4.Enabled;
    encap_indicated.IPv4.EncapsulationType = pOffload->IPv4.EncapsulationType;
    encap_indicated.IPv4.HeaderSize = pOffload->IPv4.HeaderSize;

    encap_indicated.IPv6.Enabled = pOffload->IPv6.Enabled;
    encap_indicated.IPv6.EncapsulationType = pOffload->IPv6.EncapsulationType;
    encap_indicated.IPv6.HeaderSize = pOffload->IPv6.HeaderSize;

    DbgTrace((TRACE_COMPONENT_OID, TRACE_LEVEL_VERBOSE,
              "%s Parameters IpV4 Enabled %s Type %d IpV6 Enabled %s Type %d\n",
              __FUNCTION__,
              pOffload->IPv4.Enabled == NDIS_OFFLOAD_SET_ON ? "Yes" : "No",
              pOffload->IPv4.EncapsulationType,
              pOffload->IPv6.Enabled == NDIS_OFFLOAD_SET_ON ? "Yes" : "No",
              pOffload->IPv6.EncapsulationType));

    ionic_indicate_status(ionic, NDIS_STATUS_OFFLOAD_ENCASPULATION_CHANGE,
                          &encap_indicated, encap_indicated.Header.Size);

cleanup:

    return ntStatus;
}

NDIS_STATUS
oid_handle_offload_parameters(struct ionic *ionic,
                              VOID *info_buffer,
                              ULONG info_buffer_length,
                              ULONG *bytes_read,
                              ULONG *bytes_needed)
{
    NDIS_STATUS ntStatus = NDIS_STATUS_SUCCESS;
    PNDIS_OFFLOAD_PARAMETERS pOffload = (PNDIS_OFFLOAD_PARAMETERS)info_buffer;
    NDIS_OFFLOAD offload_indication;

    if (info_buffer_length < sizeof(NDIS_OFFLOAD_PARAMETERS)) {
        *bytes_needed = sizeof(NDIS_OFFLOAD_PARAMETERS);
        *bytes_read = 0;
        ntStatus = NDIS_STATUS_INVALID_LENGTH;
        goto cleanup;
    }

    *bytes_read = sizeof(NDIS_OFFLOAD_PARAMETERS);
    *bytes_needed = 0;

    DbgTrace((TRACE_COMPONENT_OID, TRACE_LEVEL_VERBOSE,
              "%s Parameters IPV4CS %02lX TCPV4CS %02lX UDPV4CS %02lX LsoV1 "
              "%02lX IPsecV1 %02lX IPsecV2 %02lX IPsecV2IPv4 %02lX LSOV2IPV4 "
              "%02lX LSOV2IPV6 %02lX RscIPv4 %02lX RscIPv6 %02lX\n",
              __FUNCTION__, pOffload->IPv4Checksum, pOffload->TCPIPv4Checksum,
              pOffload->UDPIPv4Checksum, pOffload->LsoV1, pOffload->IPsecV1,
              pOffload->IPsecV2, pOffload->IPsecV2IPv4, pOffload->LsoV2IPv4,
              pOffload->LsoV2IPv6, pOffload->RscIPv4, pOffload->RscIPv6));

    if (pOffload->IPv4Checksum == NDIS_OFFLOAD_PARAMETERS_TX_RX_ENABLED) {
        ionic->ipv4_rx_state = NDIS_OFFLOAD_SET_ON;
        ionic->ipv4_tx_state = NDIS_OFFLOAD_SET_ON;
    } else if (pOffload->IPv4Checksum ==
               NDIS_OFFLOAD_PARAMETERS_RX_ENABLED_TX_DISABLED) {
        ionic->ipv4_rx_state = NDIS_OFFLOAD_SET_ON;
        ionic->ipv4_tx_state = NDIS_OFFLOAD_SET_OFF;
    } else if (pOffload->IPv4Checksum ==
               NDIS_OFFLOAD_PARAMETERS_TX_ENABLED_RX_DISABLED) {
        ionic->ipv4_rx_state = NDIS_OFFLOAD_SET_OFF;
        ionic->ipv4_tx_state = NDIS_OFFLOAD_SET_ON;
    } else if (pOffload->IPv4Checksum ==
               NDIS_OFFLOAD_PARAMETERS_TX_RX_DISABLED) {
        ionic->ipv4_rx_state = NDIS_OFFLOAD_SET_OFF;
        ionic->ipv4_tx_state = NDIS_OFFLOAD_SET_OFF;
    }

    DbgTrace((TRACE_COMPONENT_OID, TRACE_LEVEL_VERBOSE, "\tIpv4 rx %s tx %s\n",
              ionic->ipv4_rx_state == NDIS_OFFLOAD_SET_OFF ? "No" : "Yes",
              ionic->ipv4_tx_state == NDIS_OFFLOAD_SET_OFF ? "No" : "Yes"));

    if (pOffload->TCPIPv4Checksum == NDIS_OFFLOAD_PARAMETERS_TX_RX_ENABLED) {
        ionic->tcpv4_rx_state = NDIS_OFFLOAD_SET_ON;
        ionic->tcpv4_tx_state = NDIS_OFFLOAD_SET_ON;
    } else if (pOffload->TCPIPv4Checksum ==
               NDIS_OFFLOAD_PARAMETERS_RX_ENABLED_TX_DISABLED) {
        ionic->tcpv4_rx_state = NDIS_OFFLOAD_SET_ON;
        ionic->tcpv4_tx_state = NDIS_OFFLOAD_SET_OFF;
    } else if (pOffload->TCPIPv4Checksum ==
               NDIS_OFFLOAD_PARAMETERS_TX_ENABLED_RX_DISABLED) {
        ionic->tcpv4_rx_state = NDIS_OFFLOAD_SET_OFF;
        ionic->tcpv4_tx_state = NDIS_OFFLOAD_SET_ON;
    } else if (pOffload->TCPIPv4Checksum ==
               NDIS_OFFLOAD_PARAMETERS_TX_RX_DISABLED) {
        ionic->tcpv4_rx_state = NDIS_OFFLOAD_SET_OFF;
        ionic->tcpv4_tx_state = NDIS_OFFLOAD_SET_OFF;
    }

    DbgTrace((TRACE_COMPONENT_OID, TRACE_LEVEL_VERBOSE, "\tTcpv4 rx %s tx %s\n",
              ionic->tcpv4_rx_state == NDIS_OFFLOAD_SET_OFF ? "No" : "Yes",
              ionic->tcpv4_tx_state == NDIS_OFFLOAD_SET_OFF ? "No" : "Yes"));

    if (pOffload->TCPIPv6Checksum == NDIS_OFFLOAD_PARAMETERS_TX_RX_ENABLED) {
        ionic->tcpv6_rx_state = NDIS_OFFLOAD_SET_ON;
        ionic->tcpv6_tx_state = NDIS_OFFLOAD_SET_ON;
    } else if (pOffload->TCPIPv6Checksum ==
               NDIS_OFFLOAD_PARAMETERS_RX_ENABLED_TX_DISABLED) {
        ionic->tcpv6_rx_state = NDIS_OFFLOAD_SET_ON;
        ionic->tcpv6_tx_state = NDIS_OFFLOAD_SET_OFF;
    } else if (pOffload->TCPIPv6Checksum ==
               NDIS_OFFLOAD_PARAMETERS_TX_ENABLED_RX_DISABLED) {
        ionic->tcpv6_rx_state = NDIS_OFFLOAD_SET_OFF;
        ionic->tcpv6_tx_state = NDIS_OFFLOAD_SET_ON;
    } else if (pOffload->TCPIPv6Checksum ==
               NDIS_OFFLOAD_PARAMETERS_TX_RX_DISABLED) {
        ionic->tcpv6_rx_state = NDIS_OFFLOAD_SET_OFF;
        ionic->tcpv6_tx_state = NDIS_OFFLOAD_SET_OFF;
    }

    DbgTrace((TRACE_COMPONENT_OID, TRACE_LEVEL_VERBOSE, "\tTcpv6 rx %s tx %s\n",
              ionic->tcpv6_rx_state == NDIS_OFFLOAD_SET_OFF ? "No" : "Yes",
              ionic->tcpv6_tx_state == NDIS_OFFLOAD_SET_OFF ? "No" : "Yes"));

    if (pOffload->UDPIPv4Checksum == NDIS_OFFLOAD_PARAMETERS_TX_RX_ENABLED) {
        ionic->udpv4_rx_state = NDIS_OFFLOAD_SET_ON;
        ionic->udpv4_tx_state = NDIS_OFFLOAD_SET_ON;
    } else if (pOffload->UDPIPv4Checksum ==
               NDIS_OFFLOAD_PARAMETERS_RX_ENABLED_TX_DISABLED) {
        ionic->udpv4_rx_state = NDIS_OFFLOAD_SET_ON;
        ionic->udpv4_tx_state = NDIS_OFFLOAD_SET_OFF;
    } else if (pOffload->UDPIPv4Checksum ==
               NDIS_OFFLOAD_PARAMETERS_TX_ENABLED_RX_DISABLED) {
        ionic->udpv4_rx_state = NDIS_OFFLOAD_SET_OFF;
        ionic->udpv4_tx_state = NDIS_OFFLOAD_SET_ON;
    } else if (pOffload->UDPIPv4Checksum ==
               NDIS_OFFLOAD_PARAMETERS_TX_RX_DISABLED) {
        ionic->udpv4_rx_state = NDIS_OFFLOAD_SET_OFF;
        ionic->udpv4_tx_state = NDIS_OFFLOAD_SET_OFF;
    }

    DbgTrace((TRACE_COMPONENT_OID, TRACE_LEVEL_VERBOSE, "\tUdpv4 rx %s tx %s\n",
              ionic->udpv4_rx_state == NDIS_OFFLOAD_SET_OFF ? "No" : "Yes",
              ionic->udpv4_tx_state == NDIS_OFFLOAD_SET_OFF ? "No" : "Yes"));

    if (pOffload->UDPIPv6Checksum == NDIS_OFFLOAD_PARAMETERS_TX_RX_ENABLED) {
        ionic->udpv6_rx_state = NDIS_OFFLOAD_SET_ON;
        ionic->udpv6_tx_state = NDIS_OFFLOAD_SET_ON;
    } else if (pOffload->UDPIPv6Checksum ==
               NDIS_OFFLOAD_PARAMETERS_RX_ENABLED_TX_DISABLED) {
        ionic->udpv6_rx_state = NDIS_OFFLOAD_SET_ON;
        ionic->udpv6_tx_state = NDIS_OFFLOAD_SET_OFF;
    } else if (pOffload->UDPIPv6Checksum ==
               NDIS_OFFLOAD_PARAMETERS_TX_ENABLED_RX_DISABLED) {
        ionic->udpv6_rx_state = NDIS_OFFLOAD_SET_OFF;
        ionic->udpv6_tx_state = NDIS_OFFLOAD_SET_ON;
    } else if (pOffload->UDPIPv6Checksum ==
               NDIS_OFFLOAD_PARAMETERS_TX_RX_DISABLED) {
        ionic->udpv6_rx_state = NDIS_OFFLOAD_SET_OFF;
        ionic->udpv6_tx_state = NDIS_OFFLOAD_SET_OFF;
    }

    DbgTrace((TRACE_COMPONENT_OID, TRACE_LEVEL_VERBOSE, "\tUdpv6 rx %s tx %s\n",
              ionic->udpv6_rx_state == NDIS_OFFLOAD_SET_OFF ? "No" : "Yes",
              ionic->udpv6_tx_state == NDIS_OFFLOAD_SET_OFF ? "No" : "Yes"));

    if (pOffload->LsoV2IPv4 != NDIS_OFFLOAD_PARAMETERS_NO_CHANGE) {
        ionic->lsov2ipv4_state = pOffload->LsoV2IPv4;
    }
    if (pOffload->LsoV2IPv6 != NDIS_OFFLOAD_PARAMETERS_NO_CHANGE) {
        ionic->lsov2ipv6_state = pOffload->LsoV2IPv6;
    }
    if (pOffload->LsoV1 != NDIS_OFFLOAD_PARAMETERS_NO_CHANGE) {
        ionic->lsov1_state = pOffload->LsoV1;
    }

    DbgTrace((
        TRACE_COMPONENT_OID, TRACE_LEVEL_VERBOSE,
        "\tLSO Offload V1 %s V2Ipv4 %s V2Ipv6 %s\n",
        ionic->lsov1_state == NDIS_OFFLOAD_PARAMETERS_LSOV2_DISABLED ? "No" : "Yes",
        ionic->lsov2ipv4_state == NDIS_OFFLOAD_PARAMETERS_LSOV2_DISABLED ? "No" : "Yes",
        ionic->lsov2ipv6_state == NDIS_OFFLOAD_PARAMETERS_LSOV2_DISABLED ? "No" : "Yes"));

    if (pOffload->RscIPv4 == NDIS_OFFLOAD_PARAMETERS_RSC_ENABLED) {
        ionic->rscv4_enabled = TRUE;
    }
    else if (pOffload->RscIPv4 == NDIS_OFFLOAD_PARAMETERS_RSC_DISABLED) {
        ionic->rscv4_enabled = FALSE;
    }

    if (pOffload->RscIPv6 == NDIS_OFFLOAD_PARAMETERS_RSC_ENABLED) {
        ionic->rscv6_enabled = TRUE;
    }
    else if (pOffload->RscIPv6 == NDIS_OFFLOAD_PARAMETERS_RSC_DISABLED) {
        ionic->rscv6_enabled = FALSE;
    }

    DbgTrace((
        TRACE_COMPONENT_OID, TRACE_LEVEL_VERBOSE,
        "\tRSC Ipv4 %s Ipv6 %s\n",
        ionic->rscv4_enabled ? "Yes" : "No",
        ionic->rscv6_enabled ? "Yes" : "No"));

    ionic_set_offload_default_capabilities(ionic, &offload_indication);

    ionic_indicate_status(ionic, NDIS_STATUS_TASK_OFFLOAD_CURRENT_CONFIG,
                          &offload_indication, offload_indication.Header.Size);

cleanup:

    return ntStatus;
}

NDIS_STATUS
oid_query_bar_info(struct ionic *ionic,
                   void *info_buffer,
                   ULONG info_buffer_length,
                   ULONG *bytes_written)
{

    NDIS_STATUS ntStatus = NDIS_STATUS_SUCCESS;
    NDIS_SRIOV_PROBED_BARS_INFO *pParams =
        (NDIS_SRIOV_PROBED_BARS_INFO *)info_buffer;
    ULONG *pBAR = NULL;
    ULONG ulIndex = 0;

    pBAR = (ULONG *)((char *)pParams + pParams->BaseRegisterValuesOffset);

    while (ulIndex < PCI_TYPE0_ADDRESSES) {
        *pBAR = ionic->ProbedBARS[ulIndex];
        pBAR++;
        ulIndex++;
    }

    *bytes_written = info_buffer_length;

    return ntStatus;
}

NDIS_STATUS
init_dma( struct ionic *ionic)
{

	NDIS_STATUS		status = NDIS_STATUS_SUCCESS;
	NDIS_SG_DMA_DESCRIPTION stDmaDesc;

    //
    // Initialize system resources for use in subsequent DMA ops
    //

    NdisZeroMemory(&stDmaDesc, sizeof(NDIS_SG_DMA_DESCRIPTION));

    stDmaDesc.Header.Type = NDIS_OBJECT_TYPE_SG_DMA_DESCRIPTION;
    stDmaDesc.Header.Revision = NDIS_SG_DMA_DESCRIPTION_REVISION_1;
    stDmaDesc.Header.Size = sizeof(NDIS_SG_DMA_DESCRIPTION);
    stDmaDesc.Flags = NDIS_SG_DMA_64_BIT_ADDRESS;
    stDmaDesc.MaximumPhysicalMapping = IONIC_MAX_PHYSICAL_MAPPING;
    stDmaDesc.ProcessSGListHandler = ionic_process_sg_list;
    stDmaDesc.SharedMemAllocateCompleteHandler = NULL;

    status = NdisMRegisterScatterGatherDma(ionic->adapterhandle, &stDmaDesc,
                                           &ionic->dma_handle);

    if (status != NDIS_STATUS_SUCCESS) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR,
                  "%s Failed NdisMRegisterScatterGatherDma Status %08lX\n",
                  __FUNCTION__, status));
        goto exit;
    }

    ionic->sgl_size_in_bytes = stDmaDesc.ScatterGatherListSize;

    ionic->max_sgl_elements =
        (ionic->sgl_size_in_bytes - sizeof(SCATTER_GATHER_LIST)) /
        sizeof(SCATTER_GATHER_ELEMENT);

    if (ionic->max_sgl_elements < IONIC_TX_MAX_SG_ELEMS ||
        ionic->max_sgl_elements < IONIC_RX_MAX_SG_ELEMS) {
        DbgTrace((TRACE_COMPONENT_INIT, TRACE_LEVEL_ERROR,
                  "%s Insufficient sg element count\n", __FUNCTION__,
                  ionic->max_sgl_elements));
        status = NDIS_STATUS_RESOURCES;
        goto exit;
    }

    //
    // Minimum dma alignment should be at least 64
    //
    ionic->dma_alignment = NdisMGetDmaAlignment(ionic->adapterhandle);

    if (ionic->dma_alignment < IONIC_DEFAULT_DMA_ALIGN) {
        ionic->dma_alignment = IONIC_DEFAULT_DMA_ALIGN;
    }

exit:

	return status;
}

void
deinit_dma(struct ionic *ionic)
{

	if( ionic->dma_handle != NULL) {
		NdisMDeregisterScatterGatherDma(ionic->dma_handle);
		ionic->dma_handle = NULL;
	}
}