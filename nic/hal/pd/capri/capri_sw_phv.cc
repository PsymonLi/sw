//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//
// sw PHV injection
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <cmath>

#include "nic/include/base.hpp"
#include "nic/sdk/lib/p4/p4_api.hpp"
#include "nic/hal/pd/capri/capri_sw_phv.hpp"
#include "nic/hal/pd/capri/capri_hbm.hpp"

#include "third-party/asic/capri/model/utils/cap_blk_reg_model.h"
#include "third-party/asic/capri/model/cap_top/cap_top_csr.h"
#include "third-party/asic/capri/model/cap_ptd/cap_pt_csr.h"
#include "third-party/asic/capri/model/cap_prd/cap_pr_csr.h"
#include "third-party/asic/capri/model/cap_ppa/cap_ppa_csr.h"
#include "third-party/asic/capri/model/cap_dpa/cap_dpr_csr.h"
#include "third-party/asic/capri/model/cap_dpa/cap_dpp_csr.h"

namespace hal {
namespace pd {

//16 64B PHV entries(Flits)
#define CAPRI_SW_PHV_NUM_MEM_ENTRIES 16

//8 Profiles (config and control)
#define CAPRI_SW_PHV_NUM_PROFILES  8

// Number of parser instances
#define CAPRI_NUM_PPA 2

// Capri Flit size in bytes
#define CAPRI_FLIT_SIZE (512/8)

// capri_pd_flit_t is used to access PHV data one flit at a time
typedef struct capri_pd_flit_ {
    uint8_t flit_data[CAPRI_FLIT_SIZE];
} capri_pd_flit_t;


// capri_psp_swphv_init Initializes TxDMA(pt)/RxDMA(pr) SW PHV generator profiles
static hal_ret_t
capri_psp_swphv_init (bool rx)
{
    cap_top_csr_t &cap0 = CAP_BLK_REG_MODEL_ACCESS(cap_top_csr_t, 0, 0);
    cap_psp_csr_t *psp_csr = &cap0.pt.pt.psp;

    if (rx) {
        psp_csr = &cap0.pr.pr.psp;
    }

    int index = 0;

    HAL_TRACE_DEBUG("CAPRI-PSP::{}: Initializing PSP Global Config", 
                    __func__);

    psp_csr->cfg_sw_phv_global.start_enable(1);
    psp_csr->cfg_sw_phv_global.err_enable(0);
    psp_csr->cfg_sw_phv_global.write();

    for (index = 0; index < CAPRI_SW_PHV_NUM_MEM_ENTRIES; index++) {
        cap_psp_csr_dhs_sw_phv_mem_entry_t &phv_mem_entry = psp_csr->dhs_sw_phv_mem.entry[index];
        phv_mem_entry.data(0);
	phv_mem_entry.write();
    }

    for (index = 0; index < CAPRI_SW_PHV_NUM_PROFILES; index++) {

        cap_psp_csr_cfg_sw_phv_control_t &sw_phv_ctrl = psp_csr->cfg_sw_phv_control[index];

        sw_phv_ctrl.start_enable(0);
        sw_phv_ctrl.counter_repeat_enable(0);
        sw_phv_ctrl.qid_repeat_enable(0);
        sw_phv_ctrl.localtime_enable(0);
        sw_phv_ctrl.frame_size_enable(0);
        sw_phv_ctrl.packet_len_enable(0);
        sw_phv_ctrl.qid_enable(0);
	sw_phv_ctrl.write();

        cap_psp_csr_cfg_sw_phv_config_t &sw_phv_cfg = psp_csr->cfg_sw_phv_config[index];

        sw_phv_cfg.start_addr(index);
        sw_phv_cfg.num_flits(1);
        sw_phv_cfg.insertion_period_clocks(0);
        sw_phv_cfg.counter_max(0);
        sw_phv_cfg.qid_min(0);
        sw_phv_cfg.qid_max(0);
	sw_phv_cfg.write();
    }

    return HAL_RET_OK;
}

// capri_ppa_swphv_init initializes P4 Ingress/Egress sw phv units
static hal_ret_t
capri_ppa_swphv_init ()
{
    cap_top_csr_t &cap0 = CAP_BLK_REG_MODEL_ACCESS(cap_top_csr_t, 0, 0);
    int pidx = 0;

    for (pidx = 0; pidx < CAPRI_NUM_PPA; pidx++) {
        cap_ppa_csr_t &ppa_csr = cap0.ppa.ppa[pidx];
        cap_dpr_csr_t &dpr_csr = cap0.dpr.dpr[pidx];
        cap_dpp_csr_t &dpp_csr = cap0.dpp.dpp[pidx];
        int index = 0;

        HAL_TRACE_DEBUG("CAPRI-PPA::{}: Initializing PPA Global Config", 
                        __func__);

	// enable sw phv
        ppa_csr.cfg_sw_phv_global.start_enable(1);
        ppa_csr.cfg_sw_phv_global.err_enable(0);
	ppa_csr.cfg_sw_phv_global.write();

	// enable drop phvs
	dpr_csr.cfg_global_2.dump_drop_no_data_phv(1);
	dpr_csr.cfg_global_2.write();
	dpp_csr.cfg_global_2.dump_drop_no_data_phv_0(1);
	dpp_csr.cfg_global_2.dump_drop_no_data_phv_1(1);
	dpp_csr.cfg_global_2.write();

	// init phv entries
        for (index = 0; index < CAPRI_SW_PHV_NUM_MEM_ENTRIES; index++) {
            cap_ppa_csr_dhs_sw_phv_mem_entry_t &phv_mem_entry = ppa_csr.dhs_sw_phv_mem.entry[index];
            phv_mem_entry.data(0);
        }

        for (index = 0; index < CAPRI_SW_PHV_NUM_PROFILES; index++) {

            cap_ppa_csr_cfg_sw_phv_control_t &sw_phv_ctrl = ppa_csr.cfg_sw_phv_control[index];

            sw_phv_ctrl.start_enable(0);
            sw_phv_ctrl.counter_repeat_enable(0);
            sw_phv_ctrl.qid_repeat_enable(0);
            sw_phv_ctrl.localtime_enable(0);
            sw_phv_ctrl.frame_size_enable(0);
            sw_phv_ctrl.packet_len_enable(0);
            sw_phv_ctrl.qid_enable(0);
	    sw_phv_ctrl.write();

            cap_ppa_csr_cfg_sw_phv_config_t &sw_phv_cfg = ppa_csr.cfg_sw_phv_config[index];

            sw_phv_cfg.start_addr(index);
            sw_phv_cfg.num_flits(1);
            sw_phv_cfg.insertion_period_clocks(0);
            sw_phv_cfg.counter_max(0);
            sw_phv_cfg.qid_min(0);
            sw_phv_cfg.qid_max(0);
	    sw_phv_cfg.write();
        }
    }

    return HAL_RET_OK;
}


// capri_sw_phv_init initializes Software PHV modules
hal_ret_t
capri_sw_phv_init ()
{
    hal_ret_t ret = HAL_RET_OK;

    HAL_TRACE_DEBUG("Capri Sw PHV Init ");

    ret = capri_ppa_swphv_init();

    if (ret == HAL_RET_OK) {
        ret = capri_psp_swphv_init(false);
    }
    if (ret == HAL_RET_OK) {
        ret = capri_psp_swphv_init(true);
    }

    return ret;
}

// capri_psp_sw_phv_inject injects a PHV into Rx/Tx DMA pipeline
static hal_ret_t
capri_pr_psp_sw_phv_inject (uint8_t prof_num, uint8_t start_idx, uint8_t num_flits, void *data)
{
    cap_top_csr_t &cap0 = CAP_BLK_REG_MODEL_ACCESS(cap_top_csr_t, 0, 0);
    cap_psp_csr_t &pr_psp_csr = cap0.pr.pr.psp;
    pu_cpp_int< 512 > flit_data;

    HAL_TRACE_DEBUG("CAPRI-PSP-PHV-INJECT::{}: Injecting Software PHV.", 
                    __func__);

    int index = 0;
    capri_pd_flit_t *curr_flit_ptr = (capri_pd_flit_t *)data;

    // write flit data
    for (index = 0; index < num_flits; index++) {
	capri_pd_flit_t rdata;
	// swap the bytes in data
	int i;
        for (i = 0; i < CAPRI_FLIT_SIZE; i++) {
	    printf("%02x ", curr_flit_ptr->flit_data[i]);
	    rdata.flit_data[CAPRI_FLIT_SIZE-1-i] = curr_flit_ptr->flit_data[i];
        }
	printf("\n");
        for (i = 0; i < CAPRI_FLIT_SIZE; i++) {
	    printf("%02x ", rdata.flit_data[i]);
	}
	printf("\n");

        cap_psp_csr_dhs_sw_phv_mem_entry_t &phv_mem_entry = pr_psp_csr.dhs_sw_phv_mem.entry[index];
	cpp_int_helper::s_cpp_int_from_array(flit_data, 0, (CAPRI_FLIT_SIZE-1), (uint8_t *)curr_flit_ptr);
        phv_mem_entry.data(flit_data);
	phv_mem_entry.write();
	curr_flit_ptr++;
    }

    cap_psp_csr_cfg_sw_phv_config_t &sw_phv_cfg = pr_psp_csr.cfg_sw_phv_config[prof_num];
    sw_phv_cfg.start_addr(start_idx);
    sw_phv_cfg.num_flits(num_flits-1);
    sw_phv_cfg.insertion_period_clocks(0);
    sw_phv_cfg.counter_max(0);
    sw_phv_cfg.qid_min(0);
    sw_phv_cfg.qid_max(0);
    sw_phv_cfg.write();

    cap_psp_csr_cfg_sw_phv_control_t &sw_phv_ctrl = pr_psp_csr.cfg_sw_phv_control[prof_num];
    sw_phv_ctrl.start_enable(1);
    sw_phv_ctrl.counter_repeat_enable(0);
    sw_phv_ctrl.qid_repeat_enable(0);
    sw_phv_ctrl.localtime_enable(0);
    sw_phv_ctrl.frame_size_enable(0);
    sw_phv_ctrl.packet_len_enable(0);
    sw_phv_ctrl.qid_enable(0);
    sw_phv_ctrl.write();

    HAL_TRACE_DEBUG("CAPRI-PHV-INJECT::{}: Software PHV injected. done", 
                    __func__);
    return HAL_RET_OK;
}

// capri_psp_sw_phv_inject injects a PHV into Rx/Tx DMA pipeline
static hal_ret_t
capri_pt_psp_sw_phv_inject (uint8_t prof_num, uint8_t start_idx, uint8_t num_flits, void *data)
{
    cap_top_csr_t &cap0 = CAP_BLK_REG_MODEL_ACCESS(cap_top_csr_t, 0, 0);
    cap_psp_csr_t &pt_psp_csr = cap0.pt.pt.psp;
    pu_cpp_int< 512 > flit_data;

    HAL_TRACE_DEBUG("CAPRI-PSP-PHV-INJECT::{}: Injecting Software PHV.", 
                    __func__);

    int index = 0;
    capri_pd_flit_t *curr_flit_ptr = (capri_pd_flit_t *)data;
    printf("PHV contents:\n");

    // write flit data
    for (index = 0; index < num_flits; index++) {
	capri_pd_flit_t rdata;
	// swap the bytes in data
	int i;
        for (i = 0; i < CAPRI_FLIT_SIZE; i++) {
	    printf("%02x ", curr_flit_ptr->flit_data[i]);
	    rdata.flit_data[CAPRI_FLIT_SIZE-1-i] = curr_flit_ptr->flit_data[i];
        }
	printf("\n");
        for (i = 0; i < CAPRI_FLIT_SIZE; i++) {
	    printf("%02x ", rdata.flit_data[i]);
	}
	printf("\n");

        cap_psp_csr_dhs_sw_phv_mem_entry_t &phv_mem_entry = pt_psp_csr.dhs_sw_phv_mem.entry[index];
	cpp_int_helper::s_cpp_int_from_array(flit_data, 0, (CAPRI_FLIT_SIZE-1), curr_flit_ptr->flit_data);
        phv_mem_entry.data(flit_data);
	phv_mem_entry.write();
	curr_flit_ptr++;
    }

    cap_psp_csr_cfg_sw_phv_config_t &sw_phv_cfg = pt_psp_csr.cfg_sw_phv_config[prof_num];
    sw_phv_cfg.start_addr(start_idx);
    sw_phv_cfg.num_flits(num_flits-1);
    sw_phv_cfg.insertion_period_clocks(0);
    sw_phv_cfg.counter_max(0);
    sw_phv_cfg.qid_min(0);
    sw_phv_cfg.qid_max(0);
    sw_phv_cfg.write();

    cap_psp_csr_cfg_sw_phv_control_t &sw_phv_ctrl = pt_psp_csr.cfg_sw_phv_control[prof_num];
    sw_phv_ctrl.start_enable(1);
    sw_phv_ctrl.counter_repeat_enable(0);
    sw_phv_ctrl.qid_repeat_enable(0);
    sw_phv_ctrl.localtime_enable(0);
    sw_phv_ctrl.frame_size_enable(0);
    sw_phv_ctrl.packet_len_enable(0);
    sw_phv_ctrl.qid_enable(0);
    sw_phv_ctrl.write();

    HAL_TRACE_DEBUG("CAPRI-PHV-INJECT::{}: Software PHV injected. done", 
                    __func__);
    return HAL_RET_OK;
}

// capri_ppa_sw_phv_inject injects a PHV into P4 Ingress/Egress pipeline
static hal_ret_t
capri_ppa_sw_phv_inject (uint8_t pidx, uint8_t prof_num, uint8_t start_idx, uint8_t num_flits, void *data)
{
    cap_top_csr_t &cap0 = CAP_BLK_REG_MODEL_ACCESS(cap_top_csr_t, 0, 0);
    cap_ppa_csr_t &ppa_csr = cap0.ppa.ppa[pidx];
    pu_cpp_int< 512 > flit_data;

    HAL_TRACE_DEBUG("CAPRI-PHV-INJECT::{}: Injecting PHV into PPA {}", __func__, pidx);

    int index = 0;
    capri_pd_flit_t *curr_flit_ptr = (capri_pd_flit_t *)data;
    printf("PHV contents:\n");

    // write flit data
    for (index = 0; index < num_flits; index++) {
	capri_pd_flit_t rdata;
	// swap the bytes in data
	int i;
        for (i = 0; i < CAPRI_FLIT_SIZE; i++) {
	    printf("%02x ", curr_flit_ptr->flit_data[i]);
	    rdata.flit_data[CAPRI_FLIT_SIZE-1-i] = curr_flit_ptr->flit_data[i];
        }
	printf("\n");
        for (i = 0; i < CAPRI_FLIT_SIZE; i++) {
	    printf("%02x ", rdata.flit_data[i]);
	}
	printf("\n");

        cap_ppa_csr_dhs_sw_phv_mem_entry_t &phv_mem_entry = ppa_csr.dhs_sw_phv_mem.entry[index];
	cpp_int_helper::s_cpp_int_from_array(flit_data, 0, (CAPRI_FLIT_SIZE-1), (uint8_t *)curr_flit_ptr);
        phv_mem_entry.data(flit_data);
	phv_mem_entry.write();
	curr_flit_ptr++;
    }

    cap_ppa_csr_cfg_sw_phv_config_t &sw_phv_cfg = ppa_csr.cfg_sw_phv_config[prof_num];
    sw_phv_cfg.start_addr(start_idx);
    sw_phv_cfg.num_flits(num_flits);
    sw_phv_cfg.insertion_period_clocks(0);
    sw_phv_cfg.counter_max(0);
    sw_phv_cfg.qid_min(0);
    sw_phv_cfg.qid_max(0);
    sw_phv_cfg.write();

    cap_ppa_csr_cfg_sw_phv_control_t &sw_phv_ctrl = ppa_csr.cfg_sw_phv_control[prof_num];
    sw_phv_ctrl.start_enable(true);
    sw_phv_ctrl.counter_repeat_enable(0);
    sw_phv_ctrl.qid_repeat_enable(0);
    sw_phv_ctrl.localtime_enable(0);
    sw_phv_ctrl.frame_size_enable(0);
    sw_phv_ctrl.packet_len_enable(0);
    sw_phv_ctrl.qid_enable(0);
    sw_phv_ctrl.write();

    return HAL_RET_OK;
}

// capri_sw_phv_inject injects a software PHV into a pipeline
hal_ret_t 
capri_sw_phv_inject (asicpd_swphv_type_t type, uint8_t prof_num, uint8_t start_idx, uint8_t num_flits, void *data)
{
    hal_ret_t   ret = HAL_RET_OK;

    HAL_TRACE_DEBUG("CAPRI-PHV-INJECT::{}: Injecting Software PHV type {}", 
                    __func__, type);

    // switch based on pipeline type
    switch(type) {
    case ASICPD_SWPHV_TYPE_RXDMA:
        ret = capri_pr_psp_sw_phv_inject(prof_num, start_idx, num_flits, data);
	break;
    case ASICPD_SWPHV_TYPE_TXDMA:
        ret = capri_pt_psp_sw_phv_inject(prof_num, start_idx, num_flits, data);
	break;
    case ASICPD_SWPHV_TYPE_INGRESS:
        ret = capri_ppa_sw_phv_inject(1, prof_num, start_idx, num_flits, data);
	break;
    case ASICPD_SWPHV_TYPE_EGRESS:
        ret = capri_ppa_sw_phv_inject(0, prof_num, start_idx, num_flits, data);
	break;
    }

    return ret;
}

// capri_psp_sw_phv_state gets Rx/Tx DMA pipeline PHV state
static hal_ret_t
capri_pr_psp_sw_phv_state (uint8_t prof_num, asicpd_sw_phv_state_t *state)
{
    cap_top_csr_t &cap0 = CAP_BLK_REG_MODEL_ACCESS(cap_top_csr_t, 0, 0);
    cap_psp_csr_t &pr_psp_csr = cap0.pr.pr.psp;

    HAL_TRACE_DEBUG("CAPRI-PSP-PHV-STATE::{}: Getting Software PHV.", __func__);


    cap_psp_csr_sta_sw_phv_state_t &sw_phv_state = pr_psp_csr.sta_sw_phv_state[prof_num];
    sw_phv_state.read();
	
    cap_psp_csr_cfg_sw_phv_global_t &phv_global = pr_psp_csr.cfg_sw_phv_global;
    phv_global.read();

    cap_prd_csr_t &prd_csr = cap0.pr.pr.prd;
    cap_prd_csr_CNT_phv_t &prd_cnt = prd_csr.CNT_phv;
    prd_cnt.read();

    // set the status
    state->enabled = (bool)phv_global.start_enable();
    state->done = (bool)sw_phv_state.done();
    state->current_cntr = (uint32_t)sw_phv_state.current_counter();
    state->no_data_cntr = (uint32_t)prd_cnt.no_data();
    state->drop_no_data_cntr = (uint32_t)prd_cnt.drop();

    return HAL_RET_OK;
}

// capri_psp_sw_phv_state gets Rx/Tx DMA pipeline PHV state
static hal_ret_t
capri_pt_psp_sw_phv_state (uint8_t prof_num, asicpd_sw_phv_state_t *state)
{
    cap_top_csr_t &cap0 = CAP_BLK_REG_MODEL_ACCESS(cap_top_csr_t, 0, 0);
    cap_psp_csr_t &pt_psp_csr = cap0.pt.pt.psp;

    HAL_TRACE_DEBUG("CAPRI-PSP-PHV-STATE::{}: Getting Software PHV state.", __func__);

    // read the status registers
    cap_psp_csr_sta_sw_phv_state_t &sw_phv_state = pt_psp_csr.sta_sw_phv_state[prof_num];
    sw_phv_state.read();
	
    cap_psp_csr_cfg_sw_phv_global_t &phv_global = pt_psp_csr.cfg_sw_phv_global;
    phv_global.read();

    cap_ptd_csr_t &ptd_csr = cap0.pt.pt.ptd;
    cap_ptd_csr_CNT_phv_t &ptd_cnt = ptd_csr.CNT_phv;
    ptd_cnt.read();

    // set the status
    state->done = (bool)sw_phv_state.done();
    state->current_cntr = (uint32_t)sw_phv_state.current_counter();
    state->enabled = (bool)phv_global.start_enable();
    state->no_data_cntr = (uint32_t)ptd_cnt.no_data();
    state->drop_no_data_cntr = (uint32_t)ptd_cnt.drop();

    return HAL_RET_OK;
}

// capri_ppa_sw_phv_state gets P4 Ingress/Egress pipeline PHV state
static hal_ret_t
capri_ppa_sw_phv_state (uint8_t pidx, uint8_t prof_num, asicpd_sw_phv_state_t *state)
{
    cap_top_csr_t &cap0 = CAP_BLK_REG_MODEL_ACCESS(cap_top_csr_t, 0, 0);
    cap_ppa_csr_t &ppa_csr = cap0.ppa.ppa[pidx];

    HAL_TRACE_DEBUG("CAPRI-PHV-STATE::{}: Getting Software PHV state", __func__);

    // read the status registers
    cap_ppa_csr_sta_sw_phv_state_t &sw_phv_state = ppa_csr.sta_sw_phv_state[prof_num];
    sw_phv_state.read();
	
    cap_ppa_csr_cfg_sw_phv_global_t &phv_global = ppa_csr.cfg_sw_phv_global;
    phv_global.read();

    cap_dpr_csr_t &dpr_csr = cap0.dpr.dpr[pidx];
    cap_dprstats_csr_t &dpr_stats = dpr_csr.stats;
    dpr_stats.CNT_dpr_phv_no_data.read();
    dpr_stats.CNT_dpr_phv_drop_no_data.read();
    dpr_stats.CNT_dpr_phv_drop_no_data_drop.read();

    // set the state
    state->enabled = (bool)phv_global.start_enable();
    state->done = (bool)sw_phv_state.done();
    state->current_cntr = (uint32_t)sw_phv_state.current_counter();
    state->no_data_cntr = (uint32_t)dpr_stats.CNT_dpr_phv_no_data.all();
    state->drop_no_data_cntr = (uint32_t)dpr_stats.CNT_dpr_phv_drop_no_data.all();

    return HAL_RET_OK;
}

// capri_sw_phv_get gets the current state of the PHV
hal_ret_t 
capri_sw_phv_get (asicpd_swphv_type_t type, uint8_t prof_num, asicpd_sw_phv_state_t *state)
{
    hal_ret_t   ret = HAL_RET_OK;

    HAL_TRACE_DEBUG("CAPRI-PHV-STATE::{}: Getting Software PHV state for type {}", 
                    __func__, type);

    // switch based on pipeline type
    switch(type) {
    case ASICPD_SWPHV_TYPE_RXDMA:
        ret = capri_pr_psp_sw_phv_state(prof_num, state);
	break;
    case ASICPD_SWPHV_TYPE_TXDMA:
        ret = capri_pt_psp_sw_phv_state(prof_num, state);
	break;
    case ASICPD_SWPHV_TYPE_INGRESS:
        ret = capri_ppa_sw_phv_state(1, prof_num, state);
	break;
    case ASICPD_SWPHV_TYPE_EGRESS:
        ret = capri_ppa_sw_phv_state(0, prof_num, state);
	break;
    }

    return ret;
}

}    // namespace pd
}    // namespace hal
