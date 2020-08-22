//
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// API processing framework/engine functionality
///
//----------------------------------------------------------------------------

#ifndef __FRAMEWORK_API_ENGINE_HPP__
#define __FRAMEWORK_API_ENGINE_HPP__

#include <vector>
#include <unordered_map>
#include <list>
#include <utility>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/sdk/lib/slab/slab.hpp"
#include "nic/sdk/lib/ipc/ipc.hpp"
#include "nic/infra/core/trace.hpp"
#include "nic/infra/core/mem.hpp"
#include "nic/infra/core/msg.hpp"
#include "nic/apollo/framework/api_base.hpp"
#include "nic/apollo/framework/api_msg.hpp"
#include "nic/apollo/framework/state_base.hpp"
#include "nic/apollo/api/include/pds.hpp"
#include "nic/apollo/api/include/pds_batch.hpp"

using std::vector;
using std::unordered_map;
using std::list;
using sdk::ipc::ipc_msg_ptr;

namespace api {

/// \defgroup PDS_API_ENGINE Framework for processing APIs
/// @{

/// \brief Processing stage of the APIs in a given batch
typedef enum api_batch_stage_e {
    API_BATCH_STAGE_NONE,                 ///< Invalid stage
    API_BATCH_STAGE_INIT,                 ///< Initialization stage
    API_BATCH_STAGE_PRE_PROCESS,          ///< Pre-processing stage
    API_BATCH_STAGE_OBJ_DEPENDENCY,       ///< Dependency resolution stage
    API_BATCH_STAGE_RESERVE_RESOURCES,    ///< Reserve resources, if any
    API_BATCH_STAGE_PROGRAM_CONFIG,       ///< Table programming stage
    API_BATCH_STAGE_CONFIG_ACTIVATE,      ///< Epoch activation stage
    API_BATCH_STAGE_ABORT,                ///< Abort stage
} api_batch_stage_t;

/// \brief    per API object context
/// transient information maintained while a batch of APIs are being processed
///
/// \remark
///   api_params is not owned by this structure, so don't free it ... it
///   is owned by api_ctxt_t and hence when api_ctxt_t is being destroyed
///   we should return the api_params_t memory back to slab
typedef struct api_obj_ctxt_s api_obj_ctxt_t;
typedef list<api_obj_ctxt_t *> child_obj_ctxt_list_t;
struct api_obj_ctxt_s {
    api_op_t              api_op;         ///< de-duped/compressed API opcode
    obj_id_t              obj_id;         ///< object identifier
    api_params_t          *api_params;    ///< API specific parameters
    api_base              *cloned_obj;    ///< cloned object, for UPD processing
    child_obj_ctxt_list_t clist;          ///< list of contained/child objects
                                          ///< resulting in the reprogramming
                                          ///< of the container/parent object

    ///< object handlers can save arbitrary state across callbacks here and it
    ///< is opaque to the api engine
    struct {
        void        *cb_ctxt;
        uint64_t    upd_bmap;
    };
    uint8_t rsvd_rscs:1;          ///< true after resource reservation stage
    uint8_t hw_dirty:1;           ///< true if hw entries are updated,
                                  ///< but not yet activated
    uint8_t promoted:1;           ///< object context is due to promotion
                                  ///< affected object list to dirty object list

    api_obj_ctxt_s() {
        api_op = API_OP_INVALID;
        obj_id = OBJ_ID_NONE;
        api_params = NULL;
        cloned_obj = NULL;
        cb_ctxt = NULL;
        upd_bmap = 0;
        rsvd_rscs = FALSE;
        hw_dirty = FALSE;
        promoted = FALSE;
    }

    bool operator==(const api_obj_ctxt_s& rhs) const {
        if ((this->api_op == rhs.api_op) &&
            (this->obj_id == rhs.obj_id) &&
            (this->api_params == rhs.api_params) &&
            (this->cloned_obj == rhs.cloned_obj) &&
            (this->cb_ctxt == rhs.cb_ctxt)) {
            return true;
        }
        return false;
    }
};

// objects on which add/del/upd API calls are issued are put in a dirty
// list/map by API framework to de-dup potentially multiple API operations
// issued by caller in same batch
typedef unordered_map<api_base *, api_obj_ctxt_t *> dirty_obj_map_t;
typedef list<api_base *> dirty_obj_list_t;

// objects affected (i.e. dirtied) by add/del/upd operations on other objects
// are called affected/dependent objects and are maintained separately;
// normally these objects need to be either reprogrammed or deleted
typedef unordered_map<api_base *, api_obj_ctxt_t *> dep_obj_map_t;
typedef list<api_base *> dep_obj_list_t;

// per IPC per we need to maintain two lists
// 1. list of API (sync) request/response msgs to be sent
// 2. list of API notification msgs sent as events
typedef struct ipc_peer_api_msg_info_s {
    // number of request/response style IPC messages
    uint32_t num_req_rsp_msgs;
    // request/response style message list
    pds_msg_list_t *req_rsp_msgs;
    // numnber of event notification messages
    uint32_t num_ntfn_msgs;
    // event notification list
    pds_msg_list_t *ntfn_msgs;

    // default constructor
    ipc_peer_api_msg_info_s() {
        num_req_rsp_msgs = 0;
        req_rsp_msgs = NULL;
        num_ntfn_msgs = 0;
        ntfn_msgs = NULL;
        ntfn_msg_idx_ = 0;
        req_rsp_msg_idx_ = 0;
    }

    // add a notification msg to notification msg list
    void add_ntfn_msg(pds_msg_t *msg) {
        SDK_ASSERT(ntfn_msg_idx_ < num_ntfn_msgs);
        ntfn_msgs->msgs[ntfn_msg_idx_++] = *msg;
    }

    // add a req/rsp msg to the req/rsp msg list
    void add_req_rsp_msg(pds_msg_t *msg) {
        SDK_ASSERT(req_rsp_msg_idx_ < num_req_rsp_msgs);
        req_rsp_msgs->msgs[req_rsp_msg_idx_++] = *msg;
    }

private:
    // ntfn_msg_idx_ keeps track of the next ntfn msg to be filled
    uint32_t ntfn_msg_idx_ = 0;
    // req_rsp_msg_idx_ keeps track of next req/rsp msg to be filled
    uint32_t req_rsp_msg_idx_ = 0;

} ipc_peer_api_msg_info_t;

// messages to be sent on per IPC peer basis
typedef unordered_map<pds_ipc_id_t, ipc_peer_api_msg_info_t> pds_msg_map_t;

/// \brief    batch context, which is a list of all API contexts
typedef struct api_batch_ctxt_s {
    ipc_msg_ptr             ipc_msg;    ///< API msg of the current API batch
    pds_epoch_t             epoch;      ///< epoch in progress, passed in
                                        ///< pds_batch_begin()
    api_batch_stage_t       stage;      ///< phase of the batch processing
    vector<api_ctxt_t *>    *api_ctxts; ///< API contexts per batch

    // dirty object map is needed because in the same batch we could have
    // multiple modifications of same object, like security rules change and
    // route changes can happen for same vnic in two different API calls but in
    // same batch and we need to activate them in one write (not one after
    // another)
    dirty_obj_map_t         dom;        ///< dirty object map
    dirty_obj_list_t        dol;        ///< dirty object list
    dep_obj_map_t           aom;        ///< affected/dependent object map
    dep_obj_list_t          aol;        ///< affected/dependent object list
    pds_msg_map_t           msg_map;    ///< API IPC msg map used to
                                        ///< dispatch API msgs/ntfns
    pds_msg_map_t::iterator msg_map_it; ///< iterator to navigate the msg map

    void clear(void) {
        ipc_msg.reset();
        epoch = PDS_EPOCH_INVALID;
        stage = API_BATCH_STAGE_NONE;
        api_ctxts = NULL;
        dom.clear();
        dol.clear();
        aom.clear();
        aol.clear();
        for (msg_map_it = msg_map.begin();
             msg_map_it != msg_map.end(); msg_map_it++) {
            if (msg_map_it->second.req_rsp_msgs) {
                core::pds_msg_list_free(msg_map_it->second.req_rsp_msgs);
            }
            if (msg_map_it->second.ntfn_msgs) {
                core::pds_msg_list_free(msg_map_it->second.ntfn_msgs);
            }
        }
        msg_map.clear();
    }
} api_batch_ctxt_t;

/// \brief API counters maintained by API engine
typedef struct api_counters_s {
    // pre-process stage specific counters
    struct {
        // CREATE specific counters
        struct {
            uint32_t ok;
            uint32_t oom_err;
            uint32_t init_cfg_err;
            uint32_t obj_clone_err;
            uint32_t obj_exists_err;
            uint32_t invalid_op_err;
            uint32_t invalid_upd_err;
        } create;
        // DELETE specific counters
        struct {
            uint32_t ok;
            uint32_t obj_build_err;
            uint32_t not_found_err;
            uint32_t invalid_op_err;
        } del;
        // UPDATE specific counters
        struct {
            uint32_t ok;
            uint32_t obj_build_err;
            uint32_t init_cfg_err;
            uint32_t obj_clone_err;
            uint32_t not_found_err;
            uint32_t invalid_op_err;
            uint32_t invalid_upd_err;
        } upd;
    } preprocess;
    // resource reservation stage specific counters
    struct {
        // CREATE specific counters
        struct {
            uint32_t ok;
            uint32_t err;
        } create;
        // DELETE specific counters
        struct {
            uint32_t ok;
            uint32_t err;
        } del;
        // UPDATE specific counters
        struct {
            uint32_t ok;
            uint32_t err;
        } upd;
    } rsv_rsc;
    // object dependency stage specific counters
    struct {
        struct {
            uint32_t ok;
            uint32_t err;
            uint32_t obj_clone_err;
        } upd;
    } obj_dep;
    // program config stage specific counters
    struct {
        // CREATE specific counters
        struct {
            uint32_t ok;
            uint32_t err;
        } create;
        // DELETE specific counters
        struct {
            uint32_t ok;
            uint32_t err;
        } del;
        // UPDATE specific counters
        struct {
            uint32_t ok;
            uint32_t err;
        } upd;
    } pgm_cfg;
    // activate config stage specific counters
    struct {
        // CREATE specific counters
        struct {
            uint32_t ok;
            uint32_t err;
        } create;
        // DELETE specific counters
        struct {
            uint32_t ok;
            uint32_t err;
        } del;
        // UPDATE specific counters
        struct {
            uint32_t ok;
            uint32_t err;
        } upd;
    } act_cfg;
    // re-program config stage specific counters
    struct {
        struct {
            uint32_t ok;
            uint32_t err;
        } upd;
    } re_pgm_cfg;
    // re-activate config stage specific counters
    struct {
        struct {
            uint32_t ok;
            uint32_t err;
        } upd;
    } re_act_cfg;
    // batch commit stage specific counters
    struct {
        uint32_t ipc_err;
    } commit;;
    // batch abort stage specific counters
    struct {
        uint32_t abort;
        uint32_t txn_end_err;
    } abort;;
} api_counters_t;

/// \brief Encapsulation for all API processing framework
class api_engine {
public:
    /// \brief Constructor
    api_engine() {
        api_obj_ctxt_slab_ = NULL;
    }

    /// \brief Destructor
    ~api_engine() {}

    // API engine initialization function
    sdk_ret_t init(state_base *state) {
        state_ = state;
        api_obj_ctxt_slab_ =
            slab::factory("api-obj-ctxt", PDS_SLAB_ID_API_OBJ_CTXT,
                          sizeof(api_obj_ctxt_t), 512, true, true, true, NULL);
        if (api_obj_ctxt_slab_) {
            return SDK_RET_OK;
        }
        return SDK_RET_OOM;
    }

    /// \brief Handle batch begin by setting up per API batch context
    ///
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t batch_begin(pds_batch_params_t *batch_params);

    /// \brief Commit all the APIs in this batch
    /// Release any temporary state or resources like memory, per API context
    /// info etc.
    /// param[in] ipc_msg  IPC msg that delivered the API batch message
    /// param[in] api_msg  API message corresponding to the API batch
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t batch_commit(api_msg_t *api_msg, ipc_msg_ptr ipc_msg);

    /// \brief    return the IPC msg that delivered the current batch of API
    ///           calls
    /// \return    pointer to the IPC message
    ipc_msg_ptr ipc_msg(void) const { return batch_ctxt_.ipc_msg; }

    /// \brief    given an object, return its cloned object, if the object is
    ///           found in the dirty object map and cloned
    /// param[in] api_obj    API object found in the db
    /// \return cloned object of the given object, if found, or else NULL
    api_base *cloned_obj(api_base *api_obj) {
        if (api_obj->in_dirty_list()) {
            api_obj_ctxt_t *octxt = batch_ctxt_.dom.find(api_obj)->second;
            if (octxt->cloned_obj) {
                return octxt->cloned_obj;
            }
        }
        return NULL;
    }

    /// \brief    given an object, return its cloned object, if the object is
    ///           found in the dirty object map and cloned or else return the
    ///           same object - essentially return the object that framework is
    ///           operating on in the current batch
    /// NOTE: this object may not be in the dirty list at all, in which case
    ///       its not part of current batch of API objects being processed
    ///       (i.e., it is a committed object)
    /// param[in] api_obj    API object found in the db
    /// \return framework object of the given object (i.e., either same or
    ///         cloned object)
    api_base *framework_obj(api_base *api_obj) {
        api_base *clone;

        clone = cloned_obj(api_obj);
        return clone ? clone : api_obj;
    }

    /// \brief Add given api object to dependent object list if its not
    //         in the dirty object list and dependent object list already
    /// \param[in] obj_id    object id identifying the API object
    /// \param[in] api_op    API operation due to which this call is triggered
    /// \param[in] api_obj   API object being added to the dependent list
    /// \param[in] upd_bmap  bitmap indicating which attributes of the API
    ///                      object are being updated
    /// \return pointer to the API object context containing cumulative update
    ///         bitmap, if there is any new update or else NULL. If any new
    ///         update has been detected, it needs to trigger the dependency
    ///         chain of the objects depending on it
    api_obj_ctxt_t *add_to_deps_list(api_op_t api_op,
                                     obj_id_t obj_id_a, api_base *api_obj_a,
                                     obj_id_t obj_id_b, api_base *api_obj_b,
                                     uint64_t upd_bmap) {
                                     //obj_id_t obj_id, api_op_t api_op,
                                     //api_base *api_obj, uint64_t upd_bmap) {
        api_obj_ctxt_t *octxt = NULL;
        api_obj_ctxt_t *octxt_a, *octxt_b;

        if (api_obj_b->in_dirty_list()) {
            PDS_TRACE_DEBUG("%s already in DoL", api_obj_b->key2str().c_str());
            // if needed, update the update bitmap to indicate that more udpates
            // are seen on an object thats being updated in this batch
            octxt_b = batch_ctxt_.dom[api_obj_b];
            if (octxt_b->cloned_obj) {
                if ((octxt_b->upd_bmap & upd_bmap) != upd_bmap) {
                    octxt_b->upd_bmap |= upd_bmap;
                    PDS_TRACE_DEBUG("%s upd bmap updated to 0x%lx",
                                    api_obj_b->key2str().c_str(),
                                    octxt_b->upd_bmap);
                    octxt = octxt_b;
                    goto end;
                }
                // entry exists and the update was already known, no need to
                // trigger further recursive dependency updates
                goto end;
            }
#if 0
            // NOTE: re-ordering objects here will break functionality, so
            //       commenting out !!!
            // if the object is in dirty list already, move it to the end
            // as this update can trigger other updates
            batch_ctxt_.dol.remove(api_obj_b);
            batch_ctxt_.dol.push_back(api_obj_b);
            // fall thru
#endif
            goto end;
        } else if (api_obj_b->in_deps_list()) {
            PDS_TRACE_DEBUG("%s already in AoL", api_obj_b->key2str().c_str());
            // if needed, update the update bitmap
            octxt_b = batch_ctxt_.aom[api_obj_b];
            if ((octxt_b->upd_bmap & upd_bmap) != upd_bmap) {
                octxt_b->upd_bmap |= upd_bmap;
                PDS_TRACE_DEBUG("%s already in AoL, update upd bmap to 0x%lx",
                                api_obj_b->key2str().c_str(), octxt_b->upd_bmap);
                octxt = octxt_b;
                goto end;
            }
            // if the object is in deps list already, push it to the end
            // NOTE: this reshuffling doesn't help as the whole dependent
            //       list is processed after the dirty object list with updated
            //       specs anyway
            batch_ctxt_.aol.remove(api_obj_b);
            batch_ctxt_.aol.push_back(api_obj_b);
            // entry exists and the update was already noted
            goto end;
        }

        // object not in dirty list or dependent list, add it now to
        // affected/dependent object list/map
        octxt = octxt_b = api_obj_ctxt_alloc_();
        octxt->api_op = api_op;
        octxt->obj_id = obj_id_b;
        octxt->upd_bmap = upd_bmap;
        api_obj_b->set_in_deps_list();
        batch_ctxt_.aom[api_obj_b] = octxt;
        batch_ctxt_.aol.push_back(api_obj_b);
        PDS_TRACE_DEBUG("Added %s to AoL, update bitmap 0x%lx",
                        api_obj_b->key2str().c_str(), upd_bmap);

end:

        // if A is an element of B, A must be added to the clist of B
        // NOTE: A could be in dirty object list or dependent object list
        //       by this time (route-table chg can trigger vpc -> subnet -> vnic
        //       changes, A could be subnet and B could vnic and A is in
        //       aol list)
        if (api_obj_a->in_dirty_list()) {
            octxt_a = batch_ctxt_.dom[api_obj_a];
        } else if (api_obj_a->in_deps_list()) {
            octxt_a = batch_ctxt_.aom[api_obj_a];
        }
        if (api_base::is_contained_in(obj_id_a, obj_id_b)) {
            octxt_b->clist.push_back(octxt_a);
        }
        return octxt;
    }

    /// \brief dump api engine counters
    /// \param[in] fd     file descriptor to print counters to
    sdk_ret_t dump_api_counters(int fd);

private:

    /// \brief    allocate and return an api object context entry
    /// \return    allocated API object context entry or NULL
    api_obj_ctxt_t *api_obj_ctxt_alloc_(void) {
        api_obj_ctxt_t *octxt;

        octxt = (api_obj_ctxt_t *)api_obj_ctxt_slab_->alloc();
        if (octxt) {
            new (octxt) api_obj_ctxt_t();
        }
        return octxt;
    }

    /// \brief    free given object context entry back to its slab
    /// \param[in]    API object context pointer to be freed
    void api_obj_ctxt_free_(api_obj_ctxt_t *octxt) {
        if (octxt) {
            if (octxt->promoted) {
                api_params_free(octxt->api_params,
                                octxt->obj_id, octxt->api_op);
            }
            octxt->~api_obj_ctxt_t();
            api_obj_ctxt_slab_->free(octxt);
        }
    }

    /// \brief de-dup given API operation
    /// This is based on the currently computed operation and new API operation
    /// seen on the object
    /// \param[in] curr_op Current outstanding API operation on the object
    /// \param[in] new_op Newly encountered API operation on the object
    /// \return De-duped/compressed API operation
    api_op_t api_op_(api_op_t curr_op, api_op_t new_op);

    /// \brief pre-process create operation and form effected list of objs
    /// \param[in] api_ctxt Transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t pre_process_create_(api_ctxt_t *api_ctxt);

    /// \brief pre-process delete operation and form effected list of objs
    /// \param[in] api_ctxt Transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t pre_process_delete_(api_ctxt_t *api_ctxt);

    /// \brief pre-process update operation and form effected list of objs
    /// \param[in] api_ctxt Transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t pre_process_update_(api_ctxt_t *api_ctxt);

    /// \brief process an API and form effected list of objs
    /// \param[in] api_ctxt Transient state associated with this API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t pre_process_api_(api_ctxt_t *api_ctxt);

    /// \brief allocate any sw & hw resources for the given object and operation
    /// \param[in] api_obj API object being processed
    /// \param[in] obj_ctxt Transient information maintained to process the API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t reserve_resources_(api_base *api_obj, api_obj_ctxt_t *obj_ctxt);

    /// \brief process given object from the dirty list
    /// This is done by doing add/update of corresponding h/w entries, based
    /// on accumulated configuration without activating the epoch
    /// \param[in] api_obj API object being processed
    /// \param[in] obj_ctxt Transient information maintained to process the API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t program_config_(api_base *api_obj, api_obj_ctxt_t *obj_ctxt);

    /// \brief add objects that are dependent on given object to dependent
    ///        object list
    /// \param[in] api_obj API object being processed
    /// \param[in] obj_ctxt Transient information maintained to process the API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t add_deps_(api_base *api_obj, api_obj_ctxt_t *obj_ctxt);

    /// \brief activate configuration by switching to new epoch
    /// If object has effected any stage 0 datapath table(s), switch to new
    /// epoch in this stage NOTE: NO failures must happen in this stage
    /// \param[in] api_obj API object being processed
    /// \param[in] it    iterator position of api obj to be deleted
    /// \param[in] obj_ctxt    transient information maintained to process API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_config_(dirty_obj_list_t::iterator it,
                               api_base *api_obj, api_obj_ctxt_t *obj_ctxt);

    /// \brief abort all changes made to an object, rollback to its prev state
    /// NOTE: this is not expected to fail and also epoch is not activated if
    /// we are here
    /// \param[in] it    iterator position of api obj to be deleted
    /// \param[in] api_obj API object being processed
    /// \param[in] obj_ctxt    transient information maintained to process API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t rollback_config_(dirty_obj_list_t::iterator it,
                               api_base *api_obj, api_obj_ctxt_t *obj_ctxt);

    /// \brief given an API object id, check which all IPC peers are interested
    ///        in this object and perform bookkeeping
    /// \param[in] obj_id API object id
    /// \param[in] api_obj API object being processed
    /// \param[in] obj_ctxt    transient information maintained to process API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t count_api_msg_(obj_id_t obj_id, api_base *api_obj,
                             api_obj_ctxt_t *obj_ctxt);

    /// \brief allocate all necessary IPC API msgs to send API
    ///        object request/response msgs and/or event notification messages
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t allocate_api_msgs_(void);

    /// \brief given an API object, populate the corresponding ntfn or req/rsp
    ///        msg in the msg buffers of all interested IPC endpoints
    /// \param[in] api_obj  API object being processed
    /// \param[in] obj_ctxt    transient information maintained to process API
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t populate_api_msg_(api_base *obj, api_obj_ctxt_t *obj_ctxt);

    /// \brief Pre-process all API calls in a given batch
    /// Form a dirty list of effected obejcts as a result
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t pre_process_stage_(void);

    /// \brief compute list of affected objects that need to reprogrammed
    ///        because of API operations on dirty list objects
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t obj_dependency_computation_stage_(void);

    /// \brief move container objects from dependency list to dirty list
    ///        so we can go through proper resource reservation
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t promote_container_objs_(void);

    /// \brief reserve all needed resources for programming the config
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t resource_reservation_stage_(void);

    /// \brief Datapath table update stage
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t program_config_stage_(void);

    /// \brief Final epoch activation stage
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t activate_config_stage_(void);

    /// \brief    perform 1st phase of batch commit process
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t batch_commit_phase1_(void);

    /// \brief    perform 2nd phase of batch commit process
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t batch_commit_phase2_(void);

    /// \brief Abort all the APIs in this batch
    /// Release any temporary state or resources like memory, per API context
    /// info etc.
    ///
    /// \return #SDK_RET_OK on success, failure status code on error
    sdk_ret_t batch_abort_(void);

    /// \brief add given api object to dirty list of the API batch
    /// \param[in] api_obj API object being processed
    /// \param[in] obj_ctxt Transient information maintained to process the API
    void add_to_dirty_list_(api_base *api_obj, api_obj_ctxt_t *obj_ctxt) {
        api_obj->set_in_dirty_list();
        batch_ctxt_.dom[api_obj] = obj_ctxt;
        batch_ctxt_.dol.push_back(api_obj);
    }

    /// \brief delete given api object from dirty list of the API batch
    /// \param[in] it iterator position of api obj to be deleted
    /// \param[in] api_obj API object being processed
    void del_from_dirty_list_(dirty_obj_list_t::iterator it,
                              api_base *api_obj) {
        batch_ctxt_.dol.erase(it);
        batch_ctxt_.dom.erase(api_obj);
        api_obj->clear_in_dirty_list();
    }

    /// \brief delete given api object from dependent object list
    /// \param[in] it iterator position of api obj to be deleted
    /// \param[in] api_obj API object being processed
    void del_from_deps_list_(dep_obj_list_t::iterator it,
                             api_base *api_obj) {
        batch_ctxt_.aol.erase(it);
        batch_ctxt_.aom.erase(api_obj);
        api_obj->clear_in_deps_list();
    }

    /// \brief    helper callback function thats invoked when IPC
    ///           response is received from other components like VPP
    ///           for the config messages
    /// \param[in] msg     ipc response message received
    /// \param[in] ctxt    opaque context passed back to the callback
    static void process_ipc_async_result_(ipc_msg_ptr msg, const void *ctxt);

    /// \brief    helper function to send batch of config messages to other
    ///           components (e.g., VPP) and receive a response
    /// \param[in] api_msg_info    config msg subscription information
    /// \return    #SDK_RET_OK on success, failure status code on error
    static sdk_ret_t send_req_rsp_msgs_(pds_ipc_id_t ipc_id,
                                        ipc_peer_api_msg_info_t *api_msg_info);

    /// \brief    helper function to send batch of config notification messages
    ///           to other components (e.g., VPP) and receive a response
    /// \return    #SDK_RET_OK on success, failure status code on error
    sdk_ret_t send_ntfn_msgs_(void);

private:
    friend api_obj_ctxt_t;

    /// \brief API operation de-dup matrix
    api_op_t dedup_api_op_[API_OP_INVALID][API_OP_INVALID] = {
        // API_OP_NONE
        {API_OP_INVALID, API_OP_CREATE, API_OP_INVALID, API_OP_INVALID },
        // API_OP_CREATE
        {API_OP_INVALID, API_OP_INVALID, API_OP_NONE, API_OP_CREATE },
        // API_OP_DELETE
        {API_OP_INVALID, API_OP_UPDATE, API_OP_DELETE, API_OP_INVALID },
        // API_OP_UPDATE
        {API_OP_INVALID, API_OP_INVALID, API_OP_DELETE, API_OP_UPDATE },
    };
    api_batch_ctxt_t    batch_ctxt_;
    api_counters_t      counters_;
    slab                *api_obj_ctxt_slab_;
    state_base          *state_;
};

/// \brief    initialize the API engine
/// \param[in] state    pointer to application specific state (base) class
///                     instance
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t api_engine_init(state_base *state);

/// \brief    return the API engine instance
/// \return    pointer to the (singleton) API engine instance
api_engine *api_engine_get(void);

/// \brief    given an api object, add it to the dependent object list and
///           recursively add any other objects that may needed to be updated
/// \param[in] api_op       API operation due to which this call is triggered
/// \param[in] obj_id_a     object id of the affecting API object
/// \param[in] api_obj_a    affecting API object
/// \param[in] obj_id_b     object id of the affected API object
/// \param[in] api_obj_b    affected API object, i.e. API object being added to
///                         the dependent list
/// \param[in] upd_bmap     bitmap indicating which attributes of the affected
///                         API object are being updated
/// \return #SDK_RET_OK on success, failure status code on error
sdk_ret_t api_obj_add_to_deps(api_op_t api_op,
                              obj_id_t obj_id_a, api_base *api_obj_a,
                              obj_id_t obj_id_b, api_base *api_obj_b,
                              uint64_t upd_bmap);

/// \@}

}    // namespace api

using api::api_obj_ctxt_t;

#endif    // __FRAMEWORK_API_ENGINE_HPP__
