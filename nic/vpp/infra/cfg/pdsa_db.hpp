//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//

#ifndef __VPP_INFRA_CFG_PDSA_DB_HPP__
#define __VPP_INFRA_CFG_PDSA_DB_HPP__

#include <map>
#include <unordered_map>
#include <vector>
#include <list>
#include "nic/sdk/include/sdk/base.hpp"
#include "nic/apollo/core/msg.h"
#include "nic/apollo/api/core/msg.h"

#define foreach_config_data_element                 \
        _(DEVICE, device)                           \
        _(VPC, vpc)                                 \
        _(VNIC, vnic)                               \
        _(SUBNET, subnet)                           \
        _(DHCP_POLICY, dhcp_policy)                 \
        _(NAT_PORT_BLOCK, nat_port_block)           \
        _(SECURITY_PROFILE, security_profile)       \

// callback function prototype
typedef sdk::sdk_ret_t (*pds_cfg_set_cb)(const pds_cfg_msg_t *msg);
typedef sdk::sdk_ret_t (*pds_cfg_del_cb)(const pds_cfg_msg_t *msg);
typedef sdk::sdk_ret_t (*pds_cfg_act_cb)(const pds_cfg_msg_t *msg);
typedef sdk::sdk_ret_t (*pds_cfg_get_cb)(void *info);
typedef sdk::sdk_ret_t (*pds_cfg_dump_cb) ();
typedef void (*pds_cfg_walk_cb)(pds_cfg_msg_t *msg, void *cb_msg);
typedef void (*pds_cfg_notify_cb)(const pds_cfg_msg_t *msg, bool del);

// function prototypes
int pds_cfg_register_callbacks(obj_id_t id,
                               pds_cfg_set_cb set_cb_fn,
                               pds_cfg_del_cb del_cb_fn,
                               pds_cfg_act_cb act_cb_fn,
                               pds_cfg_get_cb get_cb_fn = NULL,
                               pds_cfg_dump_cb dump_cb_fn = NULL);

int
pds_cfg_register_notify_callbacks(obj_id_t id,
                                  pds_cfg_notify_cb notify_cb_fn);

typedef struct {
    obj_id_t      obj_id;
    bool          deleted;
    pds_cfg_msg_t modified;
    pds_cfg_msg_t original;
} batch_op_t;

typedef struct {
    obj_id_t        obj_id;
    pds_cfg_set_cb  set_cb;
    pds_cfg_del_cb  del_cb;
    pds_cfg_act_cb  act_cb;
    pds_cfg_get_cb  get_cb;
    pds_cfg_dump_cb dump_cb;
} object_cbs_t;

typedef struct {
    obj_id_t       obj_id;
    std::vector<pds_cfg_notify_cb> notify_cbs;
} object_notify_cbs_t;

// represents all the configuration data that is required by all VPP plugins.
// this is the authoritative copy, and plugins are notified (through
// registered callbacks) when this config changes
class vpp_config_data {
    // containers are unordered map, instance to cfg msg
#define _(o, d)     \
    std::unordered_map<pds_obj_key_t, pds_##d##_cfg_msg_t, pds_obj_key_hash> d;

    foreach_config_data_element

#undef _

    static vpp_config_data singleton;

public:
    // instance accessor
    static vpp_config_data &get(void) { return singleton; }

    // constant operations
    int size(obj_id_t obj_id) const;
    int objsize(obj_id_t obj_id) const;
    bool exists(pds_cfg_msg_t const& cfg_msg) const;
    bool exists(obj_id_t obj_id, pds_obj_key_t const& key) const;
    bool get(pds_cfg_msg_t &value) const;
    bool get(pds_obj_key_t const& key, pds_cfg_get_rsp_t &reply) const;
    void walk(obj_id_t obj_id, pds_cfg_walk_cb cb, void *cb_msg) const;

    // modifiers
    void set(pds_cfg_msg_t const& cfg_msg);
    void unset(obj_id_t obj_id, pds_cfg_msg_t const& cfg_msg);
};

// list of operations in a single msglist received from
// HAL. each instance in the msglist has a batch_op_t entry in here, which is
// executed once all entries are created.
class vpp_config_batch {
    std::list<batch_op_t> batch_op;
    std::vector<size_t> pool_sz;

    static std::list<object_cbs_t> object_cbs;
    static std::list<object_notify_cbs_t> object_notify_cbs;
    static vpp_config_batch singleton;

private:
    // internal operations
    void init(void);
    void publish(void);
    void unpublish(void);
    bool reserve(pds_cfg_msg_t const& msg);
    sdk::sdk_ret_t add(pds_cfg_msg_t const& cfg_msg);
    void set(pds_cfg_msg_t const& msg);
    void del(obj_id_t obj_id, pds_cfg_msg_t const& msg);

    // constant operations
    bool exists(pds_cfg_msg_t const& msg) const;
    static size_t maxsize(obj_id_t ob_id);

public:
    static void register_cbs(obj_id_t id,
                             pds_cfg_set_cb set_cb_fn,
                             pds_cfg_del_cb del_cb_fn,
                             pds_cfg_act_cb act_cb_fn,
                             pds_cfg_get_cb get_cb_fn,
                             pds_cfg_dump_cb dump_cb_fn);

    static void register_notify_cbs(obj_id_t id,
                                    pds_cfg_notify_cb notify_cb_fn);

    static vpp_config_batch &get(void) { return singleton; }
public:
    sdk::sdk_ret_t create(const pds_msg_list_t *msglist);
    sdk::sdk_ret_t commit(void);
    sdk::sdk_ret_t dump(obj_id_t obj_id);
    sdk::sdk_ret_t read(obj_id_t id, void *info);
    sdk::sdk_ret_t read(pds_cfg_get_rsp_t &response);
    void clear(void);
};


#endif // __VPP_INFRA_CFG_PDSA_DB_HPP__
