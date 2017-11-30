#include <sys/queue.h>

#include "nic/fte/fte.hpp"
#include "nic/fte/fte_flow.hpp"
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "nic/p4/nw/include/defines.h"
#include "nic/include/cpupkt_api.hpp"

namespace fte {

/*-----------------------------------------------------
    End FTE DB Constructor/Destructor APIs
------------------------------------------------------*/

// FTE features
typedef struct feature_s feature_t;
struct feature_s {
    std::string            name;
    bool                   registered;
    feature_state_init_t   state_init_fn;  // handler to init feature specific state
    size_t                 state_size;     // Size of the feature state
    uint32_t               state_offset;   // Offset of the feature state in fte::ctx_t
    exec_handler_t         exec_handler;   // Feature exec handler
};

static std::map<std::string, feature_t*> g_feature_list_;
static size_t g_feature_state_size_; // Total size of all feature specific states

static inline feature_t *feature_alloc_()
{
    return (feature_t *)HAL_CALLOC(hal::HAL_MEM_ALLOC_FTE, sizeof(feature_t));
}

static inline feature_t *feature_lookup_(const std::string &name)
{
    auto entry = g_feature_list_.find(name);
    if (entry == g_feature_list_.end()) {
        return nullptr;
    }

    return entry->second;
}

//------------------------------------------------------------------------------
// Returns total size of all feature specific states together
//------------------------------------------------------------------------------
size_t feature_state_size()
{
    return g_feature_state_size_;
}

//------------------------------------------------------------------------------
// Returns pointer to feature specific state
//------------------------------------------------------------------------------
void *feature_state_pointer(const std::string &name, uint8_t *state, size_t sz)
{
    feature_t *feature = feature_lookup_(name);

    if (!feature || !feature->registered || feature->state_size == 0) {
        return NULL;
    }

    if ((feature->state_offset +  feature->state_size) <= sz) {
        return (void *)(state + feature->state_offset);
    }

    return NULL;
}

//------------------------------------------------------------------------------
// Init feature state
//------------------------------------------------------------------------------
void feature_state_init(uint8_t *state, size_t sz)
{
    void *feature_state;

    for (auto node : g_feature_list_) {
        feature_t *feature = node.second;

        if ((feature->state_size == 0) ||
            (feature->state_offset +  feature->state_size) > sz) {
            continue;
        }

        feature_state = (void*)(state + feature->state_offset);
        if (feature->state_init_fn) {
            feature->state_init_fn(feature_state);
        } else {
            std::memset(feature_state, 0, feature->state_size);
        }
    }
}

//------------------------------------------------------------------------------
// Dummy FTE feature handler for plugins not yet loaded
//------------------------------------------------------------------------------
fte::pipeline_action_t dummy_handler(fte::ctx_t&)
{
    return fte::PIPELINE_CONTINUE;
}


//------------------------------------------------------------------------------
// Add an fte feature
//------------------------------------------------------------------------------
hal_ret_t add_feature(const std::string& name)
{
    feature_t *feature;

    HAL_TRACE_DEBUG("fte::{}: name={}", __FUNCTION__, name);

    if (feature_lookup_(name) != nullptr) {
        HAL_TRACE_ERR("fte::{}: name={} - duplicate feature", __FUNCTION__, name);
        return HAL_RET_INVALID_ARG;
    }

    feature = feature_alloc_();
    feature->name = name;
    feature->exec_handler = dummy_handler;
    g_feature_list_[name] = feature;

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// Register a feature handler
//------------------------------------------------------------------------------
hal_ret_t register_feature(const std::string& name,
                           const exec_handler_t &exec_handler,
                           const feature_info_t &feature_info)
{
    feature_t *feature;

    HAL_TRACE_DEBUG("fte::{}: name={}", __FUNCTION__, name);

    if (!exec_handler) {
        HAL_TRACE_ERR("fte: skipping invalid feature name={} - null exec_handler",
                      name);
        return HAL_RET_INVALID_ARG;
    }

    if ((feature = feature_lookup_(name)) == nullptr) {
        HAL_TRACE_ERR("fte::{}: name={} - no such feature", __FUNCTION__, name);
        return HAL_RET_INVALID_ARG;
    }

    feature->exec_handler = exec_handler;

    if (feature->state_size != 0) {
        // Previously registered
        if (feature->state_size != feature_info.state_size) {
            HAL_TRACE_ERR("fte::{}: invalid state size update feature={} old={} new={}",
                          __FUNCTION__, name, feature->state_size, feature_info.state_size);
            return HAL_RET_INVALID_ARG;
        }
    } else {
        feature->state_offset = g_feature_state_size_;
        feature->state_size = feature_info.state_size;
        g_feature_state_size_ += feature_info.state_size;
    }

    feature->state_init_fn = feature_info.state_init_fn;
    feature->registered = true;

    return HAL_RET_OK;
}

//------------------------------------------------------------------------------
// Unregister a featurte handler
//------------------------------------------------------------------------------
hal_ret_t unregister_feature(const std::string& name)

{
    feature_t *feature;

    HAL_TRACE_DEBUG("fte::{}: name={}", __FUNCTION__, name);

    if ((feature = feature_lookup_(name)) == nullptr) {
        HAL_TRACE_ERR("fte::{}: name={} - no such feature", __FUNCTION__, name);
        return HAL_RET_INVALID_ARG;
    }

    feature->registered = false;
    feature->exec_handler = dummy_handler;

    return HAL_RET_OK;
}


// FTE pipelines
// A list is used instead of a map for pipelines. It is most likely faster
// to scan a list as the no.o pipelines will be very small (single digits).
// Also we keep the most frequently used pipelines in the front (flow miss
// pipeline will be the first, with minimal lookup cost)
typedef struct pipeline_s pipeline_t;
struct pipeline_s
{
    STAILQ_ENTRY(pipeline_s) entries;
    lifqid_t lifq;
    lifqid_t lifq_mask;
    std::string name;
    uint16_t num_features_outbound;
    uint16_t num_features_inbound;
    feature_t *features[0];
};

static STAILQ_HEAD(pipeline_list_s_, pipeline_s) g_pipeline_list_ =
    STAILQ_HEAD_INITIALIZER(g_pipeline_list_);

static inline pipeline_t *
pipeline_alloc_(uint16_t num_features)
{
    return (pipeline_t *)HAL_CALLOC(hal::HAL_MEM_ALLOC_FTE, sizeof(pipeline_t) +
                                    num_features * sizeof(feature_t *));
}

static inline void
pipeline_add_(pipeline_t *pipeline)
{
    STAILQ_INSERT_TAIL(&g_pipeline_list_, pipeline, entries);
}

static inline pipeline_t *
pipeline_lookup_(const lifqid_t& lifq)
{
    pipeline_t *pipeline;

    STAILQ_FOREACH(pipeline, &g_pipeline_list_, entries) {
        if (pipeline->lifq.lif == (lifq.lif & pipeline->lifq_mask.lif) &&
            pipeline->lifq.qtype == (lifq.qtype & pipeline->lifq_mask.qtype) &&
            pipeline->lifq.qid == (lifq.qid & pipeline->lifq_mask.qid)) {
            return pipeline;
        }
    }
    return nullptr;
}

static inline pipeline_action_t
pipeline_invoke_exec_(pipeline_t *pipeline, ctx_t &ctx, uint8_t start, uint8_t end)
{
    pipeline_action_t rc;

    HAL_TRACE_DEBUG("Invoking pipeline with start: {} end: {}", start, end);

    for (int i = start; i < end; i++) {
        feature_t *feature = pipeline->features[i];

        ctx.set_feature_name(feature->name.c_str());
        ctx.set_feature_status(HAL_RET_OK);
        rc = feature->exec_handler(ctx);
        HAL_TRACE_DEBUG("fte:exec_handler feature={} pipeline={} action={}", feature->name,
                        pipeline->name, rc);

        if (rc != PIPELINE_CONTINUE) {
            break;
        }
    }

    ctx.set_feature_name("");
    return rc;
}

hal_ret_t
register_pipeline(const std::string& name, const lifqid_t& lifq,
                  const std::vector<std::string> &features_outbound,
                  const std::vector<std::string> &features_inbound,
                  const lifqid_t& lifq_mask)
{
    pipeline_t *pipeline;

    HAL_TRACE_DEBUG("fte::{}: name={} lifq={}", __FUNCTION__, name, lifq);
    if ((pipeline = pipeline_lookup_(lifq)) != nullptr) {
        HAL_TRACE_ERR("fte: skipping duplicate pipline {} lifq={} old-name={}",
                      name, lifq, pipeline->name);
        return HAL_RET_DUP_INS_FAIL;
    }

    pipeline = pipeline_alloc_(features_outbound.size()+features_inbound.size());
    pipeline->lifq = lifq;
    pipeline->lifq_mask = lifq_mask;
    pipeline->name = name;

    // Initialize feature blocks
    pipeline->num_features_outbound = features_outbound.size();
    feature_t **features = pipeline->features;
    for (const std::string& fname: features_outbound) {
        feature_t *feature = feature_lookup_(fname);
        if (!feature) {
            HAL_TRACE_ERR("fte: unknown feature {} in outbound pipeline {} - skipping",
                          fname, name);
            HAL_FREE(hal::HAL_MEM_ALLOC_FTE, pipeline);
            return HAL_RET_INVALID_ARG;
        }
        HAL_TRACE_DEBUG("fte: outbound pipeline feature {}/{}", name, feature->name);
        *features++ = feature;
    }

    pipeline->num_features_inbound = features_inbound.size();
    for (const std::string& fname: features_inbound) {
        feature_t *feature = feature_lookup_(fname);
        if (!feature) {
            HAL_TRACE_ERR("fte: unknown feature {} in inbound pipeline {} - skipping",
                          fname, name);
        }
        HAL_TRACE_DEBUG("fte: inbound pipeline feature {}/{}", name, feature->name);
        *features++ = feature;
    }

    // Add to global pipline list
    pipeline_add_(pipeline);

    return HAL_RET_OK;
}

hal_ret_t
execute_pipeline(ctx_t &ctx)
{
    uint8_t iflow_start, iflow_end, rflow_start, rflow_end;
    pipeline_action_t rc;

    do {
        pipeline_t *pipeline = pipeline_lookup_(ctx.arm_lifq());
        if (!pipeline) {
            HAL_TRACE_ERR("fte: pipeline not registered for lifq {} - ignoring packet",
                          ctx.arm_lifq());
            return HAL_RET_INVALID_ARG;
        }
        
        HAL_TRACE_DEBUG("fte: executing pipeline {} lifq={} dir={}",
                        pipeline->name, pipeline->lifq, ctx.direction());

        iflow_start = 0;
        iflow_end = rflow_start = pipeline->num_features_outbound;
        rflow_end = pipeline->num_features_outbound + pipeline->num_features_inbound;
        // Flip the feature list for network initiated
        if (pipeline->num_features_inbound > 0 && ctx.direction() == FLOW_DIR_FROM_UPLINK) {
            rflow_start = 0;
            rflow_end = iflow_start = pipeline->num_features_outbound;
            iflow_end = pipeline->num_features_outbound + pipeline->num_features_inbound;
        }

        if (ctx.role() == hal::FLOW_ROLE_RESPONDER && rflow_end > rflow_start) {
            // Invoke all responder feature handlers if we are processing Rflow
            rc = pipeline_invoke_exec_(pipeline, ctx, rflow_start, rflow_end); 
        } else {
            // Invoke all initiator feature handlers
            rc = pipeline_invoke_exec_(pipeline, ctx, iflow_start, iflow_end);
        }
       
        // If we are processing Iflow and Rflow in the same
        // context swap the flow objects and invoke pipeline 
        if (ctx.role() == hal::FLOW_ROLE_INITIATOR && \
            rc == PIPELINE_CONTINUE && ctx.valid_rflow()) {
            //Swap the derived flow objects 
            //for the reverse flow
            ctx.swap_flow_objs();

            ctx.set_role(hal::FLOW_ROLE_RESPONDER);
            rc = pipeline_invoke_exec_(pipeline, ctx, rflow_start, rflow_end);

            //Swap back before GFT update happens
            ctx.swap_flow_objs();
        }
    } while (rc == PIPELINE_RESTART);

    return ctx.feature_status();
}

// for unit test code only
void unregister_features_and_pipelines() {
    while (!STAILQ_EMPTY(&g_pipeline_list_)) {
        pipeline_t *pipeline = STAILQ_FIRST(&g_pipeline_list_);
        STAILQ_REMOVE_HEAD(&g_pipeline_list_, entries);
        HAL_FREE(hal::HAL_MEM_ALLOC_FTE, pipeline);
    }

    for (auto it = g_feature_list_.begin(); it != g_feature_list_.end(); ) {
        feature_t *feature = it->second;
        it = g_feature_list_.erase(it);
        HAL_FREE(hal::HAL_MEM_ALLOC_FTE, feature);
    }
}

/*------------------------------------------------------------
    FTE Init API to initialize FTE top-level data structure
--------------------------------------------------------------*/

hal_ret_t init(void) 
{
    return HAL_RET_OK;
}

} // namespace fte
