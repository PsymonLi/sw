#include "nic/include/base.hpp"
#include <arpa/inet.h>
#include "nic/include/hal_lock.hpp"
#include "nic/include/pd_api.hpp"
#include "nic/hal/pd/iris/internal/cpucb_pd.hpp"
#include "nic/hal/pd/iris/internal/gccb_pd.hpp"
#include "nic/hal/pd/capri/capri_loader.h"
#include "nic/hal/pd/capri/capri_hbm.hpp"
#include "nic/hal/pd/libs/wring/wring_pd.hpp"
#include "nic/hal/src/internal/proxy.hpp"
#include "nic/hal/hal.hpp"
#include "nic/hal/plugins/cfg/lif/lif_manager.hpp"
#include "gen/p4gen/cpu_rxdma/include/cpu_rxdma_p4plus_ingress.h"
#include "gen/p4gen/cpu_txdma/include/cpu_txdma_p4plus_ingress.h"
#include "nic/hal/pd/iris/internal/p4plus_pd_api.h"

namespace hal {
namespace pd {

void *
cpucb_pd_get_hw_key_func (void *entry)
{
    HAL_ASSERT(entry != NULL);
    return (void *)&(((pd_cpucb_t *)entry)->hw_id);
}

uint32_t
cpucb_pd_compute_hw_hash_func (void *key, uint32_t ht_size)
{
    return sdk::lib::hash_algo::fnv_hash(key, sizeof(cpucb_hw_id_t)) % ht_size;
}

bool
cpucb_pd_compare_hw_key_func (void *key1, void *key2)
{
    HAL_ASSERT((key1 != NULL) && (key2 != NULL));
    if (*(cpucb_hw_id_t *)key1 == *(cpucb_hw_id_t *)key2) {
        return true;
    }
    return false;
}

/********************************************
 * RxDMA
 * ******************************************/

hal_ret_t
p4pd_get_cpu_rx_stage0_prog_addr(uint64_t* offset)
{
    char progname[] = "rxdma_stage0.bin";
    char labelname[]= "cpu_rx_stage0";

    int ret = capri_program_label_to_offset("p4plus",
                                            progname,
                                            labelname,
                                            offset);
    if(ret < 0) {
        return HAL_RET_HW_FAIL;
    }
    HAL_TRACE_DEBUG("Received offset for stage0 program: {:#x}", *offset);
    return HAL_RET_OK;
}

static hal_ret_t
p4pd_add_or_del_cpu_rx_stage0_entry(pd_cpucb_t* cpucb_pd, bool del)
{
    common_p4plus_stage0_app_header_table_d     data = {0};
    hal_ret_t                                   ret = HAL_RET_OK;
    uint64_t                                    pc_offset = 0;

    // hardware index for this entry
    cpucb_hw_id_t hwid = cpucb_pd->hw_id +
        (P4PD_CPUCB_STAGE_ENTRY_OFFSET * P4PD_HWID_CPU_RX_STAGE0);

    if(!del) {
        // get pc address
        if(p4pd_get_cpu_rx_stage0_prog_addr(&pc_offset) != HAL_RET_OK) {
            HAL_TRACE_ERR("Failed to get pc address");
            ret = HAL_RET_HW_FAIL;
        }
        pc_offset = (pc_offset >> 6);
        HAL_TRACE_DEBUG("programming action-id: {:#x}", pc_offset);
        data.action_id = pc_offset;

        data.u.cpu_rxdma_initial_action_d.debug_dol = (uint8_t)cpucb_pd->cpucb->debug_dol;
        HAL_TRACE_DEBUG("CPUCB: debug_dol: {:#x}", data.u.cpu_rxdma_initial_action_d.debug_dol);

        data.u.cpu_rxdma_initial_action_d.flags = cpucb_pd->cpucb->cfg_flags;
        HAL_TRACE_DEBUG("CPUCB: flags: {:#x}", data.u.cpu_rxdma_initial_action_d.flags);
    }
    HAL_TRACE_DEBUG("Programming stage0 at hw-id: {:#x}", hwid);
    if(!p4plus_hbm_write(hwid,  (uint8_t *)&data, sizeof(data),
                P4PLUS_CACHE_INVALIDATE_BOTH)){
        HAL_TRACE_ERR("Failed to create rx: stage0 entry for CPUCB");
        ret = HAL_RET_HW_FAIL;
    }
    return ret;
}

hal_ret_t
p4pd_add_or_del_cpucb_rxdma_entry(pd_cpucb_t* cpucb_pd, bool del)
{
    hal_ret_t   ret = HAL_RET_OK;

    ret = p4pd_add_or_del_cpu_rx_stage0_entry(cpucb_pd, del);
    if(ret != HAL_RET_OK) {
        goto cleanup;
    }

    return HAL_RET_OK;
cleanup:
    /* TODO: CLEANUP */
    return ret;
}

hal_ret_t
p4pd_get_cpu_rx_stage0_entry(pd_cpucb_t* cpucb_pd)
{
    common_p4plus_stage0_app_header_table_d data = {0};

    // hardware index for this entry
    cpucb_hw_id_t hwid = cpucb_pd->hw_id +
        (P4PD_CPUCB_STAGE_ENTRY_OFFSET * P4PD_HWID_CPU_RX_STAGE0);

    if(!p4plus_hbm_read(hwid,  (uint8_t *)&data, sizeof(data))){
        HAL_TRACE_ERR("Failed to get rx: stage0 entry for CPU CB");
        return HAL_RET_HW_FAIL;
    }
    return HAL_RET_OK;
}

hal_ret_t
p4pd_get_cpucb_rxdma_entry(pd_cpucb_t* cpucb_pd)
{
    hal_ret_t   ret = HAL_RET_OK;

    ret = p4pd_get_cpu_rx_stage0_entry(cpucb_pd);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to get cpu_rx entry");
        goto cleanup;
    }
cleanup:
    return ret;
}

/********************************************
 * TxDMA
 * ******************************************/

hal_ret_t
p4pd_get_cpu_tx_stage0_prog_addr(uint64_t* offset)
{
    char progname[] = "txdma_stage0.bin";
    char labelname[]= "cpu_tx_stage0";

    int ret = capri_program_label_to_offset("p4plus",
                                            progname,
                                            labelname,
                                            offset);
    if(ret < 0) {
        return HAL_RET_HW_FAIL;
    }
    HAL_TRACE_DEBUG("Received offset for stage0 program: {:#x}", *offset);
    return HAL_RET_OK;
}

static hal_ret_t
p4pd_add_or_del_cpu_tx_stage0_entry(pd_cpucb_t* cpucb_pd, bool del)
{
    cpu_tx_initial_action_d                     data = {0};
    hal_ret_t                                   ret = HAL_RET_OK;
    uint64_t                                    pc_offset = 0;

    // hardware index for this entry
    cpucb_hw_id_t hwid = cpucb_pd->hw_id +
        (P4PD_CPUCB_STAGE_ENTRY_OFFSET * P4PD_HWID_CPU_TX_STAGE0);

    if(!del) {
        // get pc address
        if(p4pd_get_cpu_tx_stage0_prog_addr(&pc_offset) != HAL_RET_OK) {
            HAL_TRACE_ERR("Failed to get pc address");
            ret = HAL_RET_HW_FAIL;
        }
        pc_offset = (pc_offset >> 6);
        HAL_TRACE_DEBUG("programming action-id: {:#x}", pc_offset);
        data.action_id = pc_offset;
        data.u.cpu_tx_initial_action_d.total = 1;

        // get asq address
        wring_hw_id_t   asq_base;
        ret = wring_pd_get_base_addr(types::WRING_TYPE_ASQ, 0, &asq_base);
        if(ret != HAL_RET_OK) {
            HAL_TRACE_ERR("Failed to receive asq base, ret: {}", ret);
        } else {
            HAL_TRACE_DEBUG("asq base: {:#x}", asq_base);
            data.u.cpu_tx_initial_action_d.asq_base = asq_base;
        }
        
        // get ascq address 
        wring_hw_id_t   ascq_base;
        ret = wring_pd_get_base_addr(types::WRING_TYPE_ASCQ, 0, &ascq_base);
        if(ret != HAL_RET_OK) {
            HAL_TRACE_ERR("Failed to receive ascq base, ret: {}", ret);
        } else {
            HAL_TRACE_DEBUG("ascq base: {:#x}", ascq_base);
            data.u.cpu_tx_initial_action_d.ascq_base = ascq_base;
        }
        
        // get ascq sem address
        HAL_TRACE_DEBUG("ascq sem: {:#x}", CAPRI_SEM_ASCQ_INF_ADDR(0));
        data.u.cpu_tx_initial_action_d.ascq_sem_inf_addr = CAPRI_SEM_ASCQ_INF_ADDR(0);
    }

    HAL_TRACE_DEBUG("Programming tx stage0 at hw-id: {:#x}", hwid);
    if(!p4plus_hbm_write(hwid,  (uint8_t *)&data, sizeof(data),
                P4PLUS_CACHE_INVALIDATE_BOTH)){
        HAL_TRACE_ERR("Failed to create tx: stage0 entry for CPUCB");
        ret = HAL_RET_HW_FAIL;
    }
    return ret;
}

hal_ret_t
p4pd_get_cpu_tx_stage0_entry(pd_cpucb_t* cpucb_pd)
{
    cpu_tx_initial_action_d data = {0};

    // hardware index for this entry
    cpucb_hw_id_t hwid = cpucb_pd->hw_id +
        (P4PD_CPUCB_STAGE_ENTRY_OFFSET * P4PD_HWID_CPU_TX_STAGE0);

    if(!p4plus_hbm_read(hwid,  (uint8_t *)&data, sizeof(data))){
        HAL_TRACE_ERR("Failed to get rx: stage0 entry for CPU CB");
        return HAL_RET_HW_FAIL;
    }
    cpucb_pd->cpucb->total_tx_pkts = ntohll(data.u.cpu_tx_initial_action_d.asq_total_pkts);
    return HAL_RET_OK;
}


hal_ret_t
p4pd_add_or_del_cpucb_txdma_entry(pd_cpucb_t* cpucb_pd, bool del)
{
    hal_ret_t   ret = HAL_RET_OK;

    ret = p4pd_add_or_del_cpu_tx_stage0_entry(cpucb_pd, del);
    if(ret != HAL_RET_OK) {
        goto cleanup;
    }

    // Initialize CPU Descriptor and page rings
    ret = wring_pd_table_init(types::WRING_TYPE_CPUDR, 
                              cpucb_pd->cpucb->cb_id);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to init CPUDR ring: {}", ret);
        goto cleanup;
    }

    ret = wring_pd_table_init(types::WRING_TYPE_CPUPR, 
                              cpucb_pd->cpucb->cb_id);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to init CPUDR ring: {}", ret);
        goto cleanup;
    }

    return HAL_RET_OK;
cleanup:
    /* TODO: CLEANUP */
    return ret;
}

hal_ret_t
p4pd_get_cpucb_txdma_entry(pd_cpucb_t* cpucb_pd)
{
    hal_ret_t   ret = HAL_RET_OK;

    ret = p4pd_get_cpu_tx_stage0_entry(cpucb_pd);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to get cpu_rx entry");
        goto cleanup;
    }
cleanup:
    return ret;
}

/**************************/

cpucb_hw_id_t
pd_cpucb_get_base_hw_index(pd_cpucb_t* cpucb_pd)
{
    HAL_ASSERT(NULL != cpucb_pd);
    HAL_ASSERT(NULL != cpucb_pd->cpucb);

    // Get the base address of CPU CB from LIF Manager.
    // Set qtype and qid as 0 to get the start offset.
    uint64_t offset = lif_manager()->GetLIFQStateAddr(SERVICE_LIF_CPU, 0, 0);
    HAL_TRACE_DEBUG("received offset {:#x}", offset);
    return offset + \
        (cpucb_pd->cpucb->cb_id * P4PD_HBM_CPU_CB_ENTRY_SIZE);
}

hal_ret_t
p4pd_add_or_del_cpucb_entry(pd_cpucb_t* cpucb_pd, bool del)
{
    hal_ret_t                   ret = HAL_RET_OK;

    ret = p4pd_add_or_del_cpucb_rxdma_entry(cpucb_pd, del);
    if(ret != HAL_RET_OK) {
        goto err;
    }

    ret = p4pd_add_or_del_cpucb_txdma_entry(cpucb_pd, del);
    if(ret != HAL_RET_OK) {
        goto err;
    }

err:
    return ret;
}

static
hal_ret_t
p4pd_get_cpucb_entry(pd_cpucb_t* cpucb_pd)
{
    hal_ret_t                   ret = HAL_RET_OK;

    ret = p4pd_get_cpucb_rxdma_entry(cpucb_pd);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to get rxdma entry for cpucb");
        goto err;
    }

    ret = p4pd_get_cpucb_txdma_entry(cpucb_pd);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to get txdma entry for cpucb");
        goto err;
    }

err:
    /*TODO: cleanup */
    return ret;
}

/********************************************
 * APIs
 *******************************************/

hal_ret_t
pd_cpucb_create (pd_func_args_t *pd_func_args)
{
    hal_ret_t               ret;
    pd_cpucb_create_args_t *args = pd_func_args->pd_cpucb_create;
    pd_cpucb_s              *cpucb_pd;

    HAL_TRACE_DEBUG("Creating pd state for CPU CB.");

    // allocate PD cpucb state
    cpucb_pd = cpucb_pd_alloc_init();
    if (cpucb_pd == NULL) {
        return HAL_RET_OOM;
    }
    HAL_TRACE_DEBUG("Alloc done");
    cpucb_pd->cpucb = args->cpucb;
    // get hw-id for this CPUCB
    cpucb_pd->hw_id = pd_cpucb_get_base_hw_index(cpucb_pd);

    // program cpucb
    ret = p4pd_add_or_del_cpucb_entry(cpucb_pd, false);
    if(ret != HAL_RET_OK) {
        goto cleanup;
    }

    // program garbage collector cb
    ret = p4pd_init_gc_cbs();
    if(ret != HAL_RET_OK) {
        goto cleanup;
    }


    // add to db
    ret = add_cpucb_pd_to_db(cpucb_pd);
    if (ret != HAL_RET_OK) {
       goto cleanup;
    }
    args->cpucb->pd = cpucb_pd;

    return HAL_RET_OK;

cleanup:

    if (cpucb_pd) {
        cpucb_pd_free(cpucb_pd);
    }
    return ret;
}

hal_ret_t
pd_cpucb_update (pd_func_args_t *pd_func_args)
{
    hal_ret_t               ret;
    pd_cpucb_update_args_t *args = pd_func_args->pd_cpucb_update;

    if(!args) {
       return HAL_RET_INVALID_ARG;
    }

    cpucb_t*                cpucb = args->cpucb;
    pd_cpucb_t*             cpucb_pd = (pd_cpucb_t*)cpucb->pd;

    HAL_TRACE_DEBUG("CPUCB pd update");

    // program cpucb
    ret = p4pd_add_or_del_cpucb_entry(cpucb_pd, false);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to update cpucb");
    }
    return ret;
}

hal_ret_t
pd_cpucb_delete (pd_func_args_t *pd_func_args)
{
    hal_ret_t               ret;
    pd_cpucb_delete_args_t *args = pd_func_args->pd_cpucb_delete;

    if(!args) {
       return HAL_RET_INVALID_ARG;
    }

    cpucb_t*                cpucb = args->cpucb;
    pd_cpucb_t*             cpucb_pd = (pd_cpucb_t*)cpucb->pd;

    HAL_TRACE_DEBUG("CPUCB pd delete");

    // program cpucb
    ret = p4pd_add_or_del_cpucb_entry(cpucb_pd, true);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Failed to delete cpucb entry");
    }

    del_cpucb_pd_from_db(cpucb_pd);

    cpucb_pd_free(cpucb_pd);

    return ret;
}

hal_ret_t
pd_cpucb_get (pd_func_args_t *pd_func_args)
{
    hal_ret_t               ret;
    pd_cpucb_get_args_t *args = pd_func_args->pd_cpucb_get;
    pd_cpucb_t              cpucb_pd;

    HAL_TRACE_DEBUG("CPUCB pd get for id: {}", args->cpucb->cb_id);

    // allocate PD cpucb state
    cpucb_pd_init(&cpucb_pd);
    cpucb_pd.cpucb = args->cpucb;

    // get hw-id for this CPUCB
    cpucb_pd.hw_id = pd_cpucb_get_base_hw_index(&cpucb_pd);
    HAL_TRACE_DEBUG("Received hw-id {:#x}", cpucb_pd.hw_id);

    // get hw cpucb entry
    ret = p4pd_get_cpucb_entry(&cpucb_pd);
    if(ret != HAL_RET_OK) {
        HAL_TRACE_ERR("Get request failed for id {}", cpucb_pd.cpucb->cb_id);
    }
    return ret;
}

}    // namespace pd
}    // namespace hal
