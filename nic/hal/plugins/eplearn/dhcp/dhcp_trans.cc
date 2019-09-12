//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

#include "nic/include/pd_api.hpp"
#include "dhcp_trans.hpp"
#include <arpa/inet.h>
#include "dhcp_packet.hpp"
#include "nic/hal/plugins/eplearn/eplearn.hpp"
#include "nic/hal/plugins/cfg/nw/interface_api.hpp"

namespace hal {
namespace eplearn {

void *dhcptrans_get_key_func(void *entry) {
    SDK_ASSERT(entry != NULL);
    return (void *)(((dhcp_trans_t *)entry)->trans_key_ptr());
}

uint32_t dhcptrans_key_size(void) {
    return sizeof(dhcp_trans_key_t);
}

void dhcp_ctx::init(struct packet* dhcp_packet) {
	struct dhcp_packet* raw = dhcp_packet->raw;
	this->xid_ = ntohl(raw->xid);
	memcpy(this->chaddr_, raw->chaddr, 16);
	this->server_identifer_ = 0;
	this->renewal_time_ = 0;
	this->rebinding_time_ = 0;
	this->lease_time_ = 0;
	bzero(&this->yiaddr_, sizeof(struct in_addr));
	bzero(&this->subnet_mask_, sizeof(struct in_addr));
}

/* Make sure only one instance of State machine is present and all transactions
 * should use the same state machine */
dhcp_trans_t::dhcp_fsm_t *dhcp_trans_t::dhcp_fsm_ = new dhcp_fsm_t();
dhcp_trans_t::trans_timer_t *dhcp_trans_t::dhcp_timer_ =
            new trans_timer_t(DHCP_TIMER_ID);
slab *dhcp_trans_t::dhcplearn_slab_ = slab::factory("dhcpLearn", HAL_SLAB_DHCP_LEARN,
                                sizeof(dhcp_trans_t), 16,
                                false, true, true);
ht *dhcp_trans_t::dhcplearn_key_ht_ =
    ht::factory(HAL_MAX_DHCP_TRANS,
                dhcptrans_get_key_func,
                dhcptrans_key_size());

ht *dhcp_trans_t::dhcplearn_ip_entry_ht_ =
    ht::factory(HAL_MAX_DHCP_TRANS,
                trans_get_ip_entry_key_func,
                trans_ip_entry_key_size());

/* For now setting a high time for DOL testing
 * But, all these should be configurable.
 */
#define INIT_TIMEOUT        120 * TIME_MSECS_PER_SEC
#define SELECTING_TIMEOUT   120 * TIME_MSECS_PER_SEC
#define REQUESTING_TIMEOUT  120 * TIME_MSECS_PER_SEC

#define ADD_COMPLETION_HANDLER(__trans, __event, __ep_handle, __new_ip_addr)                 \
    uint32_t __trans_cnt = eplearn_info->trans_ctx_cnt;                                      \
    eplearn_info->trans_ctx[__trans_cnt].trans = __trans;                                    \
    eplearn_info->trans_ctx[__trans_cnt].event = __event;                                    \
    eplearn_info->trans_ctx[__trans_cnt].event_data.dhcp_data.ep_handle = (__ep_handle);     \
    eplearn_info->trans_ctx[__trans_cnt].event_data.dhcp_data.new_ip_addr =  (__new_ip_addr); \
    eplearn_info->trans_ctx_cnt++;                                                             \
    fte_ctx->register_completion_handler(trans_t::trans_completion_handler);


// clang-format off
void dhcp_trans_t::dhcp_fsm_t::_init_state_machine() {
#define SM_FUNC(__func) SM_BIND_NON_STATIC(dhcp_fsm_t, __func)
#define SM_FUNC_ARG_1(__func) SM_BIND_NON_STATIC_ARGS_1(dhcp_fsm_t, __func)
    FSM_SM_BEGIN((sm_def))
        FSM_STATE_BEGIN(DHCP_INIT, INIT_TIMEOUT, NULL, NULL)
            FSM_TRANSITION(DHCP_DISCOVER, SM_FUNC(process_dhcp_discover), DHCP_SELECTING)
            FSM_TRANSITION(DHCP_REQUEST, SM_FUNC(process_dhcp_request), DHCP_REQUESTING)
            FSM_TRANSITION(DHCP_RELEASE, SM_FUNC(process_dhcp_new_release), DHCP_DONE)
            FSM_TRANSITION(DHCP_INFORM, SM_FUNC(process_dhcp_inform), DHCP_BOUND)
            FSM_TRANSITION(DHCP_ERROR, NULL, DHCP_DONE)
        FSM_STATE_END
        FSM_STATE_BEGIN(DHCP_SELECTING, SELECTING_TIMEOUT, NULL, NULL)
            FSM_TRANSITION(DHCP_OFFER, SM_FUNC(process_dhcp_offer), DHCP_SELECTING)
            FSM_TRANSITION(DHCP_INVALID_PACKET, NULL, DHCP_DONE)
            FSM_TRANSITION(DHCP_ERROR, NULL, DHCP_DONE)
            FSM_TRANSITION(DHCP_REQUEST, SM_FUNC(process_dhcp_request), DHCP_REQUESTING)
            FSM_TRANSITION(DHCP_REBIND, SM_FUNC(process_dhcp_request), DHCP_REQUESTING)
        FSM_STATE_END
        FSM_STATE_BEGIN(DHCP_REQUESTING, REQUESTING_TIMEOUT, NULL, NULL)
            FSM_TRANSITION(DHCP_ACK, SM_FUNC(process_dhcp_ack), DHCP_BOUND)
            FSM_TRANSITION(DHCP_REQUEST, SM_FUNC(process_dhcp_request), DHCP_REQUESTING)
            FSM_TRANSITION(DHCP_REBIND, NULL, DHCP_REQUESTING)
            FSM_TRANSITION(DHCP_NACK, SM_FUNC(process_dhcp_end), DHCP_DONE)
            FSM_TRANSITION(DHCP_ERROR, SM_FUNC(process_dhcp_end), DHCP_DONE)
        FSM_STATE_END
        FSM_STATE_BEGIN(DHCP_RENEWING, 0, NULL, NULL)
            FSM_TRANSITION(DHCP_ACK, SM_FUNC(process_dhcp_ack), DHCP_BOUND)
            FSM_TRANSITION(DHCP_REBIND, NULL, DHCP_REBINDING)
            FSM_TRANSITION(DHCP_REQUEST, SM_FUNC(process_dhcp_request_after_bound), DHCP_RENEWING)
            FSM_TRANSITION(DHCP_LEASE_TIMEOUT, SM_FUNC(process_dhcp_bound_timeout), DHCP_DONE)
            FSM_TRANSITION(DHCP_RELEASE, SM_FUNC(process_dhcp_end), DHCP_DONE)
            FSM_TRANSITION(DHCP_NACK, SM_FUNC(process_dhcp_end), DHCP_DONE)
            FSM_TRANSITION(DHCP_ERROR, SM_FUNC(process_dhcp_end), DHCP_DONE)
        FSM_STATE_END
        FSM_STATE_BEGIN(DHCP_REBINDING, 0, NULL, NULL)
            FSM_TRANSITION(DHCP_ACK, SM_FUNC(process_dhcp_ack), DHCP_BOUND)
            FSM_TRANSITION(DHCP_REQUEST, SM_FUNC(process_dhcp_request), DHCP_REQUESTING)
            FSM_TRANSITION(DHCP_REBIND, NULL, DHCP_REBINDING)
            FSM_TRANSITION(DHCP_LEASE_TIMEOUT, SM_FUNC(process_dhcp_bound_timeout), DHCP_DONE)
            FSM_TRANSITION(DHCP_RELEASE, SM_FUNC(process_dhcp_end), DHCP_DONE)
            FSM_TRANSITION(DHCP_NACK, SM_FUNC(process_dhcp_end), DHCP_DONE)
            FSM_TRANSITION(DHCP_ERROR, SM_FUNC(process_dhcp_end), DHCP_DONE)
        FSM_STATE_END
        FSM_STATE_BEGIN(DHCP_BOUND, 0, SM_FUNC_ARG_1(bound_entry_func), NULL)
            FSM_TRANSITION(DHCP_REQUEST, SM_FUNC(process_dhcp_request_after_bound), DHCP_RENEWING)
            FSM_TRANSITION(DHCP_REBIND, NULL, DHCP_REBINDING)
            FSM_TRANSITION(DHCP_INFORM, SM_FUNC(process_dhcp_inform), DHCP_BOUND)
            FSM_TRANSITION(DHCP_IP_ADD, SM_FUNC(add_ip_entry), DHCP_BOUND)
            FSM_TRANSITION(DHCP_ERROR, SM_FUNC(process_dhcp_end), DHCP_DONE)
            FSM_TRANSITION(DHCP_DECLINE, SM_FUNC(process_dhcp_end), DHCP_DONE)
            FSM_TRANSITION(DHCP_RELEASE, SM_FUNC(process_dhcp_end), DHCP_DONE)
            FSM_TRANSITION(DHCP_INVALID_PACKET, SM_FUNC(process_dhcp_end), DHCP_DONE)
            FSM_TRANSITION(DHCP_LEASE_TIMEOUT, SM_FUNC(process_dhcp_bound_timeout), DHCP_DONE)
        FSM_STATE_END
        FSM_STATE_BEGIN(DHCP_DONE, 0, NULL, NULL) //No done timeout, it will be deleted.
        FSM_STATE_END
    FSM_SM_END
    this->set_state_machine(sm_def);
}
// clang-format on


static void lease_timeout_handler(void *timer, uint32_t timer_id, void *ctxt) {
    fsm_state_machine_t* sm_ = reinterpret_cast<fsm_state_machine_t*>(ctxt);
    sm_->reset_timer();
    trans_t* trans =
        reinterpret_cast<trans_t*>(sm_->get_ctx());
    trans_t::process_transaction(trans, DHCP_LEASE_TIMEOUT, NULL);
}

static hal_ret_t
update_flow_fwding(fte::ctx_t *fte_ctx)
{
    fte::flow_update_t flowupd;
    const fte::cpu_rxhdr_t* cpu_rxhdr_ = fte_ctx->cpu_rxhdr();
    hal::l2seg_t *dl2seg;
    ether_header_t * ethhdr;
    ep_t *dep = nullptr;
    if_t *dif;
    hal_ret_t ret = HAL_RET_OK;
    hal::pd::pd_get_object_from_flow_lkupid_args_t args;
    pd::pd_func_args_t pd_func_args = {0};
    hal::hal_obj_id_t obj_id;
    void *obj;

    dep = fte_ctx->dep();

    if (dep != nullptr) {
        /* Destination EP known already return. */
        goto out;
    }

    args.flow_lkupid = cpu_rxhdr_->lkp_vrf;
    args.obj_id = &obj_id;
    args.pi_obj = &obj;
    pd_func_args.pd_get_object_from_flow_lkupid = &args;
    ret = hal::pd::hal_pd_call(hal::pd::PD_FUNC_ID_GET_OBJ_FROM_FLOW_LKPID, &pd_func_args);
    if (ret != HAL_RET_OK && obj_id != hal::HAL_OBJ_ID_L2SEG) {
        HAL_TRACE_ERR("fte: Invalid obj id: {}, ret:{}", obj_id, ret);
        return HAL_RET_L2SEG_NOT_FOUND;
    }
    dl2seg = (hal::l2seg_t *)obj;

    SDK_ASSERT(dl2seg != nullptr);
    ethhdr = (ether_header_t *)(fte_ctx->pkt() + cpu_rxhdr_->l2_offset);
    dep = hal::find_ep_by_l2_key(dl2seg->seg_id, ethhdr->dmac);
    if (dep == nullptr) {
        HAL_TRACE_ERR("Destination endpoint not found.");
        ret = HAL_RET_EP_NOT_FOUND;
        goto out;
    }

    dif = hal::find_if_by_handle(dep->if_handle);
    bzero(&flowupd, sizeof(flowupd));
    flowupd.fwding.dif = dif;
    if (dif && dif->if_type == intf::IF_TYPE_ENIC) {
        hal::lif_t *lif = if_get_lif(dif);
        if (lif == NULL) {
            ret = HAL_RET_LIF_NOT_FOUND;
            goto out;
        }
        flowupd.type =  fte::FLOWUPD_FWDING_INFO;
        flowupd.fwding.qtype = lif_get_qtype(lif, intf::LIF_QUEUE_PURPOSE_RX);
        flowupd.fwding.qid_en = 1;
        flowupd.fwding.qid = 0;
        pd::pd_if_get_lport_id_args_t args;
        args.pi_if = flowupd.fwding.dif;
        pd_func_args.pd_if_get_lport_id = &args;
        ret = pd::hal_pd_call(pd::PD_FUNC_ID_IF_GET_LPORT_ID, &pd_func_args);
        flowupd.fwding.lport = args.lport_id;
        // flowupd.fwding.lport = hal::pd::if_get_lport_id(flowupd.fwding.dif);
        flowupd.fwding.dl2seg = dl2seg;
        ret = fte_ctx->update_flow(flowupd);
        if (ret != HAL_RET_OK) {
            goto out;
        }
        flowupd.type =  fte::FLOWUPD_ACTION;
        flowupd.action = session::FLOW_ACTION_ALLOW;
        ret = fte_ctx->update_flow(flowupd);
    } else {
        HAL_TRACE_ERR("Destination endpoint not found.");
        ret = HAL_RET_IF_NOT_FOUND;
    }

out:
     return ret;
}

bool dhcp_trans_t::dhcp_fsm_t::process_dhcp_discover(fsm_state_ctx ctx,
                                                     fsm_event_data fsm_data) {
    //dhcp_event_data *data = reinterpret_cast<dhcp_event_data*>(fsm_data);
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);


    dhcp_trans->log_info("Processing dhcp discover.");
    dhcp_trans_t *existing_trans = reinterpret_cast<dhcp_trans_t *>(
        dhcp_trans_t::dhcplearn_key_ht()->lookup(&dhcp_trans->trans_key_));

    if (existing_trans == nullptr) {
        dhcp_trans_t::dhcplearn_key_ht()->insert((void *)dhcp_trans,
                                                &dhcp_trans->ht_ctxt_);
    }

    return true;
}

bool dhcp_trans_t::dhcp_fsm_t::process_dhcp_new_release(fsm_state_ctx ctx,
                                                    fsm_event_data fsm_data) {
    bool rc = true;
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);
    dhcp_trans_t *existing_trans;
    dhcp_event_data *data = reinterpret_cast<dhcp_event_data*>(fsm_data);
    const struct packet *decoded_packet = data->decoded_packet;
    struct dhcp_packet *raw = decoded_packet->raw;
    ip_addr_t ip_addr  = { 0 };
    trans_ip_entry_key_t ip_entry_key;

    ip_addr.addr.v4_addr = ntohl(raw->ciaddr.s_addr);
    HAL_TRACE_INFO("Processing DHCP release for new transaction. {} {}",
            ipaddr2str(&ip_addr), dhcp_trans->trans_key_ptr()->vrf_id);

    init_ip_entry_key((&ip_addr),
                      dhcp_trans->trans_key_ptr()->vrf_id,
                      &ip_entry_key);

    existing_trans = reinterpret_cast<dhcp_trans_t *>(
        dhcp_trans_t::dhcplearn_ip_entry_ht()->lookup(
                (&ip_entry_key)));

    if ((existing_trans != nullptr) && (existing_trans != dhcp_trans) &&
            (memcmp(existing_trans->trans_key_ptr()->mac_addr,
            dhcp_trans->trans_key_ptr()->mac_addr, sizeof(mac_addr_t)) == 0)) {
        /* We found a transaction with same mac address */
        HAL_TRACE_INFO("Initiating DHCP release.");
        dhcp_trans_t::process_transaction(existing_trans, DHCP_RELEASE,
                fsm_data);
    } else {
        HAL_TRACE_INFO("Dhcp transaction not found {}",
                macaddr2str(dhcp_trans->trans_key_ptr()->mac_addr));
    }

    return rc;
}

bool dhcp_trans_t::dhcp_fsm_t::process_dhcp_inform(fsm_state_ctx ctx,
                                                    fsm_event_data fsm_data) {
    dhcp_event_data *data = reinterpret_cast<dhcp_event_data*>(fsm_data);
    const struct packet *decoded_packet = data->decoded_packet;
    fte::ctx_t *fte_ctx = data->fte_ctx;
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);
    struct dhcp_packet *raw = decoded_packet->raw;
    dhcp_ctx *dhcp_ctx = &dhcp_trans->ctx_;
    struct option_data option_data;
    ep_t *ep_entry = fte_ctx->sep();
    eplearn_info_t *eplearn_info = (eplearn_info_t*)\
                fte_ctx->feature_state(FTE_FEATURE_EP_LEARN);
    ip_addr_t ip_addr = {0};
    hal_ret_t ret;
    bool rc = true;
    dhcp_trans_t *existing_trans;

    ip_addr.addr.v4_addr = ntohl(raw->yiaddr.s_addr);

    ret = dhcp_lookup_option(decoded_packet,
                             DHO_DHCP_SERVER_IDENTIFIER, &option_data);
    if (ret != HAL_RET_OK) {
        dhcp_trans->sm_->throw_event(DHCP_INVALID_PACKET, NULL);
        rc = false;
        goto out;
    }

    if (ep_entry == nullptr) {
        HAL_TRACE_ERR("DHCP client Endpoint entry not found.");
        dhcp_trans->sm_->throw_event(DHCP_INVALID_PACKET, NULL);
        rc = false;
        goto out;
    }

    memcpy(&(dhcp_ctx->server_identifer_), option_data.data,
           sizeof(dhcp_ctx->server_identifer_));

    existing_trans = reinterpret_cast<dhcp_trans_t *>(
        dhcp_trans_t::dhcplearn_key_ht()->lookup(&dhcp_trans->trans_key_));

    if (existing_trans == nullptr) {
        dhcp_trans_t::dhcplearn_key_ht()->insert((void *)dhcp_trans,
                                                &dhcp_trans->ht_ctxt_);
    }

    if (!memcmp(&(dhcp_trans->ip_entry_key_ptr()->ip_addr),
            &ip_addr, sizeof(ip_addr)) == 0) {
        /*
         * Modify only if the IP address is different from the previous one.
         */
        if (data->in_fte_pipeline) {
            ADD_COMPLETION_HANDLER(dhcp_trans, DHCP_IP_ADD,
                    ep_entry->hal_handle, ip_addr);
        } else {
            rc = add_ip_entry(ctx, fsm_data);
            if (!rc) {
                goto out;
            }
        }
    }

out:
    return rc;
}


bool dhcp_trans_t::dhcp_fsm_t::process_dhcp_end(fsm_state_ctx ctx,
                                                fsm_event_data fsm_data) {
    dhcp_event_data *data = reinterpret_cast<dhcp_event_data*>(fsm_data);
    fte::ctx_t *fte_ctx;
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);
    ep_t *ep_entry;
    eplearn_info_t *eplearn_info;
    ip_addr_t ip_addr = {0};
    bool rc = true;

    if (data->in_fte_pipeline) {
        fte_ctx = data->fte_ctx;
        ep_entry = fte_ctx->sep();
        eplearn_info = (eplearn_info_t*)\
                        fte_ctx->feature_state(FTE_FEATURE_EP_LEARN);
        ADD_COMPLETION_HANDLER(dhcp_trans, data->event,
                ep_entry->hal_handle, ip_addr);
        rc = false;
        /* Don't change state, done soon after pipeline completed */
        goto out;
    }

out:
    return rc;
}

bool dhcp_trans_t::dhcp_fsm_t::process_dhcp_request(fsm_state_ctx ctx,
                                                    fsm_event_data fsm_data) {
    dhcp_event_data *data = reinterpret_cast<dhcp_event_data*>(fsm_data);
    const struct packet *decoded_packet = data->decoded_packet;
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);
    dhcp_ctx *dhcp_ctx = &dhcp_trans->ctx_;
    struct option_data option_data;
    hal_ret_t ret;

    dhcp_trans->log_info("Processing dhcp request...");
    ret = dhcp_lookup_option(decoded_packet,
                            DHO_DHCP_SERVER_IDENTIFIER, &option_data);
    if (ret != HAL_RET_OK) {
        dhcp_trans->sm_->throw_event(DHCP_INVALID_PACKET, NULL);
        return false;
    }

    memcpy(&(dhcp_ctx->server_identifer_), option_data.data,
           sizeof(dhcp_ctx->server_identifer_));

    dhcp_trans_t *existing_trans = reinterpret_cast<dhcp_trans_t *>(
        dhcp_trans_t::dhcplearn_key_ht()->lookup(&dhcp_trans->trans_key_));

    if (existing_trans == nullptr) {
        dhcp_trans_t::dhcplearn_key_ht()->insert((void *)dhcp_trans,
                                                &dhcp_trans->ht_ctxt_);
    }

    return true;
}

bool dhcp_trans_t::dhcp_fsm_t::process_dhcp_request_after_bound(
    fsm_state_ctx ctx, fsm_event_data fsm_data) {
    dhcp_event_data *data = reinterpret_cast<dhcp_event_data*>(fsm_data);
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);
    fte::ctx_t *fte_ctx = data->fte_ctx;
    bool rc = true;

    if (is_broadcast(*fte_ctx)) {
        dhcp_trans->sm_->throw_event(DHCP_REBIND, fsm_data);
        rc = false;
        goto out;
    }

out:
    return rc;
}

void dhcp_trans_t::reset() {
    hal_ret_t ret;
    ep_t * ep_entry;

    dhcp_trans_t::dhcplearn_key_ht()->remove(&this->trans_key_);
    dhcp_trans_t::dhcplearn_ip_entry_ht()->remove(this->ip_entry_key_ptr());

    this->stop_lease_timer();
    ep_entry = this->get_ep_entry();
    if (ep_entry != NULL) {
        this->log_info("Deleting IP entry from EP DB");
        ret = endpoint_update_ip_delete(ep_entry,
                &this->ip_entry_key_ptr()->ip_addr, EP_FLAGS_LEARN_SRC_DHCP);
        if (ret != HAL_RET_OK) {
            this->log_error("Failed Deleting IP entry from EP DB");
        }
        this->log_info("Successfully Deleted IP entry from EP DB");
    }

    this->sm_->stop_state_timer();
    this->log_info("Reset Complete.");
}

bool dhcp_trans_t::dhcp_fsm_t::process_dhcp_offer(fsm_state_ctx ctx,
                                                fsm_event_data fsm_data) {
    dhcp_event_data *data = reinterpret_cast<dhcp_event_data*>(fsm_data);
    fte::ctx_t *fte_ctx = data->fte_ctx;
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);
    hal_ret_t ret;

    dhcp_trans->log_info("Processing DHCP OFfer.");
    /*
     * Update flow forwarding information.
     */
    ret = update_flow_fwding(fte_ctx);

    if (ret != HAL_RET_OK) {
        dhcp_trans->log_error("Unable to update flow fwding info, dropping transaction");
        dhcp_trans->sm_->throw_event(DHCP_ERROR, NULL);
        return false;
    }


    return true;
}

bool dhcp_trans_t::dhcp_fsm_t::add_ip_entry(fsm_state_ctx ctx,
                                          fsm_event_data fsm_data) {
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);
    dhcp_event_data *data = reinterpret_cast<dhcp_event_data*>(fsm_data);
    hal_ret_t ret;
    bool rc = true;
    ip_addr_t zero_ip = { 0 };


    ret = eplearn_ip_move_process(data->ep_handle,
            &dhcp_trans->ip_entry_key_ptr()->ip_addr, DHCP_LEARN);

    if (ret != HAL_RET_OK) {
        dhcp_trans->log_error("IP move process failed, skipping IP add.");
        dhcp_trans->sm_->throw_event(DHCP_ERROR, NULL);
       return false;
    }

    /* Do a re-lookup as EP entry would have changed */
    ep_t *ep_entry = find_ep_by_handle(data->ep_handle);
    if ((memcmp(&(dhcp_trans->ip_entry_key_ptr()->ip_addr),
            &zero_ip, sizeof(ip_addr_t))) &&
        (memcmp(&(dhcp_trans->ip_entry_key_ptr()->ip_addr),
                   (&data->new_ip_addr), sizeof(ip_addr_t)))) {
        /*
         * Restart Transaction if IP address is different.
         * */
        restart_transaction(ctx, fsm_data);
        /* Do a lookup again to make sure we have updated EP reference. */
        ep_entry = find_ep_by_handle(data->ep_handle);
    }

    init_ip_entry_key((&data->new_ip_addr),
                      dhcp_trans->trans_key_ptr()->vrf_id,
                      dhcp_trans->ip_entry_key_ptr());

    ret = endpoint_update_ip_add(ep_entry,
            &dhcp_trans->ip_entry_key_ptr()->ip_addr,
            EP_FLAGS_LEARN_SRC_DHCP);

    if (ret != HAL_RET_OK) {
        dhcp_trans->log_error("IP add update failed");
        dhcp_trans->sm_->throw_event(DHCP_ERROR, NULL);
       /* We should probably drop this packet as this DHCP request may not be valid one */
        rc = false;
    } else {
        dhcp_trans->log_info("IP add update successful");
    }

    dhcp_trans_t::dhcplearn_ip_entry_ht()->insert(
        (void *)dhcp_trans, &dhcp_trans->ip_entry_ht_ctxt_);

    dhcp_trans->start_lease_timer();

    return rc;
}

void dhcp_trans_t::start_lease_timer() {

    if (this->lease_timer_ctx) {
        dhcp_timer_->delete_timer(this->lease_timer_ctx);
    }
    if (this->ctx_.lease_time_) {
        HAL_TRACE_INFO("Starting lease timer of {} seconds",
                this->ctx_.lease_time_);
        this->lease_timer_ctx  = dhcp_timer_->add_timer_with_custom_handler(
                this->ctx_.lease_time_ * TIME_MSECS_PER_SEC,
                this->sm_, lease_timeout_handler);
    } else {
        HAL_TRACE_INFO("Lease timer is not set.");
        this->lease_timer_ctx = nullptr;
    }
}

void dhcp_trans_t::stop_lease_timer() {

    if (this->lease_timer_ctx) {
        dhcp_timer_->delete_timer(this->lease_timer_ctx);
    }
    this->lease_timer_ctx = nullptr;
}

bool dhcp_trans_t::dhcp_fsm_t::restart_transaction(fsm_state_ctx ctx,
                                          fsm_event_data fsm_data)
{
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);

    dhcp_trans->log_info("Restarting DHCP Transaction.");
    dhcp_trans->reset();
    dhcp_trans_t::dhcplearn_key_ht()->insert((void *)dhcp_trans,
                                            &dhcp_trans->ht_ctxt_);

    return true;
}


bool dhcp_trans_t::dhcp_fsm_t::process_dhcp_ack(fsm_state_ctx ctx,
                                                fsm_event_data fsm_data) {
    dhcp_event_data *data = reinterpret_cast<dhcp_event_data*>(fsm_data);
    const struct packet *decoded_packet = data->decoded_packet;
    fte::ctx_t *fte_ctx = data->fte_ctx;
    struct dhcp_packet *raw = decoded_packet->raw;
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);
    dhcp_ctx *dhcp_ctx = &dhcp_trans->ctx_;
    eplearn_info_t *eplearn_info = (eplearn_info_t*)\
                fte_ctx->feature_state(FTE_FEATURE_EP_LEARN);
    struct option_data option_data;
    hal_ret_t ret;
    ep_t *ep_entry;
    ip_addr_t ip_addr = { 0 };
    bool rc = true;

    ip_addr.addr.v4_addr = ntohl(raw->yiaddr.s_addr);

    dhcp_trans->log_info("Processing dhcp ack.");
    HAL_TRACE_INFO("Received IP {:#x}, {}", ip_addr, ipaddr2str(&ip_addr));
    ep_entry = fte_ctx->sep();
    if (ep_entry == nullptr) {
        dhcp_trans->log_error("DHCP Server Endpoint entry not found.");
        dhcp_trans->sm_->throw_event(DHCP_INVALID_PACKET, NULL);
        return false;
    }

    /* This is DHCP ack, find the corresponding EP by l2 key */
    ep_entry = find_ep_by_l2_key(ep_entry->l2_key.l2_segid,
            dhcp_trans->trans_key_ptr()->mac_addr);
    if (ep_entry == nullptr) {
        dhcp_trans->log_error("DHCP client endpoint entry not found.");
        dhcp_trans->sm_->throw_event(DHCP_ERROR, NULL);
        return false;
    }

    dhcp_ctx->yiaddr_ = raw->yiaddr;

    ret = dhcp_lookup_option(decoded_packet, DHO_DHCP_LEASE_TIME, &option_data);
    if (ret != HAL_RET_OK) {
        dhcp_trans->log_error("Invalid DHCP packet, lease time not found.");
        dhcp_trans->sm_->throw_event(DHCP_INVALID_PACKET, NULL);
        return false;
    }

    dhcp_ctx->lease_time_ = *((uint32_t*)(option_data.data));
    dhcp_ctx->lease_time_  = ntohl(dhcp_ctx->lease_time_);
    //memcpy(&(dhcp_ctx->lease_time_), option_data.data,
      //     sizeof(dhcp_ctx->lease_time_));

    ret = dhcp_lookup_option(decoded_packet, DHO_ROUTERS, &option_data);
    if (ret == HAL_RET_OK) {
        memcpy(&(dhcp_ctx->default_gateway_), option_data.data,
                sizeof(dhcp_ctx->default_gateway_));
    }

    if (memcmp(&(dhcp_trans->ip_entry_key_ptr()->ip_addr),
            &ip_addr, sizeof(ip_addr)) == 0) {
        dhcp_trans->log_info("Ip address is the same, restarting lease timer.");
        dhcp_trans->start_lease_timer();
    } else {
        if (data->in_fte_pipeline) {
            ADD_COMPLETION_HANDLER(dhcp_trans, DHCP_IP_ADD,
                    ep_entry->hal_handle, ip_addr);
        } else {
            rc = add_ip_entry(ctx, fsm_data);
            if (!rc) {
                goto out;
            }
        }
    }

    /*
     * Finally update flow forwarding information.
     */
    ret = update_flow_fwding(fte_ctx);

    if (ret != HAL_RET_OK) {
        dhcp_trans->log_error("Unable to update flow fwding info, dropping transaction");
        dhcp_trans->sm_->throw_event(DHCP_ERROR, NULL);
        rc = false;
        goto out;
    }

 out:
    return rc;
}

bool dhcp_trans_t::dhcp_fsm_t::process_dhcp_bound_timeout(fsm_state_ctx ctx,
                                                          fsm_event_data data) {
    dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);

    dhcp_trans->log_info("Bound timed out.");
    dhcp_trans->lease_timer_ctx = nullptr;
    return true;
}

void dhcp_trans_t::dhcp_fsm_t::bound_entry_func(fsm_state_ctx ctx) {
    //dhcp_trans_t *dhcp_trans = reinterpret_cast<dhcp_trans_t *>(ctx);
    //dhcp_trans->sm_->set_current_state_timeout(dhcp_ctx->lease_time_);
    //TODO : ADD Bound timer here.
}

dhcp_trans_t::dhcp_trans_t(struct packet *dhcp_packet, fte::ctx_t &ctx) {
    this->ctx_.init(dhcp_packet);
    this->sm_ = new fsm_state_machine_t(get_sm_def_func, DHCP_INIT, DHCP_DONE,
                                      DHCP_TIMEOUT, DHCP_REMOVE,
                                      (fsm_state_ctx)this, get_timer_func);
    init_dhcp_trans_key(dhcp_packet->raw->chaddr, this->ctx_.xid_,
                        ctx.sep(), &this->trans_key_);
}

void dhcp_trans_t::init_dhcp_trans_key(const uint8_t *chaddr, uint32_t xid,
                                       const ep_t *ep,
                                       dhcp_trans_key_t *trans_key_) {
    for (uint32_t i = 0; i < ETHER_ADDR_LEN; ++i) {
        trans_key_->mac_addr[i] = chaddr[i];
    }
    trans_key_->xid = xid;
    vrf_t *vrf = vrf_lookup_by_handle(ep->vrf_handle);
    if (vrf == NULL) {
        HAL_ABORT(0);
    }
    trans_key_->vrf_id = vrf->vrf_id;
}

void dhcp_trans_t::process_event(dhcp_fsm_event_t event, fsm_event_data data) {
    this->sm_->process_event(event, data);
}

dhcp_trans_t::~dhcp_trans_t() {
    this->log_info("Deleting transaction..");
    this->reset();
    delete this->sm_;
}

hal_ret_t
dhcp_process_ip_move(hal_handle_t ep_handle, const ip_addr_t *ip_addr) {

    return dhcp_trans_t::process_ip_move(ep_handle, ip_addr,
            dhcp_trans_t::get_ip_ht());
}


}  // namespace eplearn
}  // namespace hal
