#include "nic/hal/test/utils/hal_test_utils.hpp"
#include "nic/include/hal_state.hpp"
#include "nic/hal/pd/pd_api.hpp"
#include <gtest/gtest.h>

using hal::g_hal_state;
using hal::utils::g_hal_mem_mgr;

void
hal_initialize (void)
{
    char              cfg_file[] = "hal.json";
    char              *cfg_path;
    std::string       full_path;
    hal::hal_cfg_t    hal_cfg;

    bzero(&hal_cfg, sizeof(hal_cfg));
    cfg_path = std::getenv("HAL_CONFIG_PATH");
    if (cfg_path) {
        full_path =  std::string(cfg_path) + "/" + std::string(cfg_file);
        // std::cerr << "full path " << full_path << std::endl;
    } else {
        full_path = std::string(cfg_file);
    }

    // make sure cfg file exists
    if (access(full_path.c_str(), R_OK) < 0) {
        fprintf(stderr, "config file %s has no read permissions\n",
                full_path.c_str());
        exit(1);
    }
    printf("Json file: %s\n", full_path.c_str());

    if (hal::hal_parse_cfg(full_path.c_str(), &hal_cfg) != HAL_RET_OK) {
        fprintf(stderr, "HAL config file parsing failed, quitting ...\n");
        // ASSERT_TRUE(0);
    }
    printf("Parsed cfg json file \n");

    // set the full path of the catalog file
    hal_cfg.catalog_file = hal_cfg.cfg_path + "/catalog.json";

    // make sure catalog file exists
    if (access(hal_cfg.catalog_file.c_str(), R_OK) < 0) {
        fprintf(stderr, "Catalog file %s has no read permissions\n",
                hal_cfg.catalog_file.c_str());
        exit(1);
    }

    // initialize HAL
    if (hal::hal_init(&hal_cfg) != HAL_RET_OK) {
        fprintf(stderr, "HAL initialization failed, quitting ...\n");
        exit(1);
    }
    printf("HAL Initialized \n");
}

// slab test utility
void 
hal_test_utils_slab_disable_delete (void)
{
    hal::g_delay_delete = false;
}

slab_stats_t *
hal_test_utils_collect_slab_stats (void)
{
    slab_stats_t    *stats = NULL, *t_stats = NULL;
    slab            *slab = NULL;

    stats = (slab_stats_t *)malloc(sizeof(slab_stats_t) * hal::HAL_SLAB_MAX);
    memset(stats, 0, sizeof(slab_stats_t) * hal::HAL_SLAB_MAX);

    t_stats = stats;
    for (uint32_t i = hal::HAL_SLAB_NONE; i < hal::HAL_SLAB_MAX; i++) {
        slab = hal_test_utils_get_slab((hal_slab_t)i);
        if (slab) {
            hal_test_utils_populate(t_stats, slab);
        }
        t_stats++;
    }

    return stats;
}

slab *
hal_test_utils_get_slab (hal_slab_t slab_id)
{
    hal_ret_t ret = HAL_RET_OK;
    slab *pi_slab = NULL;
    pi_slab = g_hal_state->get_slab(slab_id);
    hal::pd::pd_get_slab_args_t args;

    if (pi_slab) {
        return pi_slab;
    }

    args.slab_id = slab_id;
    ret = hal::pd::hal_pd_call(hal::pd::PD_FUNC_ID_GET_SLAB, (void *)&args);
    HAL_ASSERT(ret == HAL_RET_OK);
    return args.slab;
    // return g_hal_state_pd->get_slab(slab_id);
    // return NULL;
}

void
hal_test_utils_populate (slab_stats_t *stats, slab *slab)
{
    if (stats == NULL || slab == NULL) {
        return;
    }

    stats->slab_id = (hal_slab_t)slab->slab_id();
    stats->num_in_use = slab->num_in_use();
    stats->num_allocs = slab->num_allocs();
    stats->num_frees = slab->num_frees();

    return;
}

void 
hal_test_utils_slab_stats_free (slab_stats_t *stats)
{
    free(stats);
}

void
hal_test_utils_check_slab_leak (slab_stats_t *pre,
                                slab_stats_t *post, bool *is_leak)
{
    *is_leak = false;
    if (pre == NULL || post == NULL) {
        return;
    }
    for (uint32_t i = hal::HAL_SLAB_NONE; i < hal::HAL_SLAB_MAX; i++) {
        // if (memcmp(pre, post, sizeof(slab_stats_t))) {
        if (pre->num_in_use != post->num_in_use) {
            *is_leak = true;
            HAL_TRACE_DEBUG("-----  Leaked  ------");
            hal_test_utils_trace(pre, post);
        } else {
            // HAL_TRACE_DEBUG("-----  Passed  ------");
            // hal_test_utils_trace(pre, post);
        }
        pre++;
        post++;
    }

    return;
}

void
hal_test_utils_trace (slab_stats_t *pre, slab_stats_t *post)
{
    if (pre == NULL || post == NULL) {
        return;
    }
    HAL_ASSERT(pre->slab_id == post->slab_id);

    HAL_TRACE_DEBUG("slab_id: {}", pre->slab_id);
    HAL_TRACE_DEBUG("           Pre             Post");
    HAL_TRACE_DEBUG(" In-Use    {}              {}  ", 
            pre->num_in_use, post->num_in_use);       
    HAL_TRACE_DEBUG(" Allocs    {}              {}  ", 
            pre->num_allocs, post->num_allocs);       
    HAL_TRACE_DEBUG(" Frees     {}              {}  ", 
            pre->num_frees, post->num_frees);       

    return;
}

bool 
mtrack_cb (void *ctxt, uint32_t alloc_id, mtrack_info_t *minfo) 
{
    mtrack_info_t       *mtrack_stats = (mtrack_info_t *)ctxt;

#if 0
    HAL_ASSERT(alloc_id < hal::HAL_MEM_ALLOC_OTHER);
    mtrack_stats[alloc_id] = *minfo;
    HAL_TRACE_DEBUG("{}:alloc_id:{} -> allocs: {}, frees: {}; allocs: {}, frees: {}", __FUNCTION__, alloc_id,
                    minfo->num_allocs, minfo->num_frees, mtrack_stats[alloc_id].num_allocs, mtrack_stats[alloc_id].num_frees);
#endif
    mtrack_stats[alloc_id] = *minfo;
    HAL_TRACE_DEBUG("{}:alloc_id:{} -> allocs: {}, frees: {}; allocs: {}, frees: {}", __FUNCTION__, alloc_id,
                    minfo->num_allocs, minfo->num_frees, mtrack_stats[alloc_id].num_allocs, mtrack_stats[alloc_id].num_frees);
    return true;
}


mtrack_info_t *
hal_test_utils_collect_mtrack_stats (void)
{
    mtrack_info_t       *mtrack_stats;

    mtrack_stats = (mtrack_info_t *)malloc(sizeof(mtrack_info_t) * 
                                           
                                           hal::HAL_MEM_ALLOC_OTHER);
    memset(mtrack_stats, 0, sizeof(mtrack_info_t) * hal::HAL_MEM_ALLOC_OTHER);
    g_hal_mem_mgr.walk(mtrack_stats, mtrack_cb); 

    return mtrack_stats;

}

void 
hal_test_utils_mtrack_info_free (mtrack_info_t *minfo)
{
    free(minfo);
}

void 
hal_test_utils_mtrack_trace (uint32_t i, mtrack_info_t *pre,
                             mtrack_info_t *post)
{
    if (pre == NULL || post == NULL) {
        return;
    }
    HAL_TRACE_DEBUG("mtrack_id: {}", i);

    HAL_TRACE_DEBUG("           Pre             Post");
    HAL_TRACE_DEBUG(" In-Use    {}              {}  ", 
            (pre->num_allocs - pre->num_frees), 
            (post->num_allocs - post->num_frees));
    HAL_TRACE_DEBUG(" Allocs    {}              {}  ", 
            pre->num_allocs, post->num_allocs);       
    HAL_TRACE_DEBUG(" Frees     {}              {}  ", 
            pre->num_frees, post->num_frees);       

    return;
}

void
hal_test_utils_check_mtrack_leak (mtrack_info_t *pre,
                                 mtrack_info_t *post, bool *is_leak)
{
    *is_leak = false;
    if (pre == NULL || post == NULL) {
        return;
    }
    HAL_TRACE_DEBUG("check leak alloc_id : 6 allocs: {}, frees: {}, other:{}", 
                    pre[6].num_allocs, pre[6].num_frees, hal::HAL_MEM_ALLOC_OTHER);
    for (uint32_t i = hal::HAL_MEM_ALLOC_NONE; i < hal::HAL_MEM_ALLOC_OTHER; i++) {
        // if (memcmp(pre, post, sizeof(slab_stats_t))) {
        if ((pre->num_allocs - pre->num_frees) != (post->num_allocs - post->num_frees)) {
            *is_leak = true;
            HAL_TRACE_DEBUG("-----  Leaked  ------");
            hal_test_utils_mtrack_trace(i, pre, post);
        } else {
#if 0
            HAL_TRACE_DEBUG("-----  Passed  ------");
            hal_test_utils_mtrack_trace(i, pre, post);
#endif
        }
        pre++;
        post++;
    }

    return;
}

