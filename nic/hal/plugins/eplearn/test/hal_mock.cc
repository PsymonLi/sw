#include "nic/include/base.hpp"
#include "nic/include/fte_ctx.hpp"
#include "nic/include/fte.hpp"

class flow_t;
class phv_t;

hal::ep_t *temp_ep;
hal::vrf_t *temp_ten;
hal::ep_t *temp_dep;
ipvx_addr_t            sip;          // source IP address
ipvx_addr_t            dip;
hal_ret_t fte::ctx_t::init(cpu_rxhdr_t *cpu_rxhdr, uint8_t *pkt, size_t pkt_len, bool copied_pkt,
                           flow_t iflow[], flow_t rflow[],
                           feature_state_t feature_state[], uint16_t num_features) {
    this->pkt_ = pkt;
    this->pkt_len_ = pkt_len;
    this->copied_pkt_ = copied_pkt;
    this->cpu_rxhdr_ = cpu_rxhdr;
    this->sep_ = temp_ep;
    this->dep_ = temp_dep;
    this->sl2seg_ = nullptr;
    this->dl2seg_ = nullptr;
    this->sep_handle_ = temp_ep->hal_handle;
    this->dep_handle_ = temp_dep ? temp_dep->hal_handle : HAL_HANDLE_INVALID;
    this->svrf_ = this->dvrf_ = temp_ten;
    this->vlan_tag_valid_ = false;
    this->num_features_ = 100;
    this->key_.dip = dip;
    this->key_.sip = sip;
    //this->arm_lifq_ = FLOW_MISS_LIFQ; //hardcoding for flow miss in test
    this->feature_state_ = feature_state; //(feature_state_t *)HAL_CALLOC(hal::HAL_MEM_ALLOC_FTE, sizeof(feature_state_t));
    return HAL_RET_OK;
}

void fte_ctx_init(fte::ctx_t &ctx, hal::vrf_t *ten, hal::ep_t *ep,
        hal::ep_t *dep, ip_addr_t *souce_ip, ip_addr_t *dest_ip, fte::cpu_rxhdr_t *cpu_rxhdr,
        uint8_t *pkt, size_t pkt_len, bool copied_pkt,
        fte::flow_t iflow[], fte::flow_t rflow[], fte::feature_state_t feature_state[])
{
    temp_ep = ep;
    temp_ten = ten;
    temp_dep = dep;
    if (souce_ip) {
        sip = souce_ip->addr;
    }
    if (dest_ip) {
        dip = dest_ip->addr;
    }
    ctx.init(cpu_rxhdr, pkt, pkt_len, copied_pkt, iflow, rflow, feature_state, 0);
}

hal_ret_t fte::ctx_t::process()
{
    this->invoke_completion_handlers(true);
    return HAL_RET_OK;
}

void
fte::ctx_t::invoke_completion_handlers(bool fail)
{
    HAL_TRACE_DEBUG("fte: invoking completion handlers");
    for (int i = 0; i < num_features_; i++) {
        if (feature_state_[i].completion_handler != nullptr) {
            HAL_TRACE_DEBUG("fte: invoking completion handler {:#x}",
                        (void*)(feature_state_[i].completion_handler));
            (*feature_state_[i].completion_handler)(*this, fail);
        }
    }
}

hal_ret_t
fte::fte_asq_send(hal::pd::cpu_to_p4plus_header_t* cpu_header,
             hal::pd::p4plus_to_p4_header_t* p4plus_header,
             uint8_t* pkt, size_t pkt_len) {
    return HAL_RET_OK;
}

hal_ret_t fte::ctx_t::update_flow(const flow_update_t& flowupd,
                   const hal::flow_role_t role)
{
    return HAL_RET_OK;
}


hal_ret_t
mock_create_add_cb(hal::cfg_op_ctxt_t *cfg_ctxt)
{
    return HAL_RET_OK;
}

hal_ret_t
mock_create_commit_cb (hal::cfg_op_ctxt_t *cfg_ctxt)
{
    return HAL_RET_OK;
}

hal_ret_t
mock_create_abort_cb (hal::cfg_op_ctxt_t *cfg_ctxt)
{
    return HAL_RET_OK;
}

hal_ret_t
mock_create_cleanup_cb (hal::cfg_op_ctxt_t *cfg_ctxt)
{
    return HAL_RET_OK;
}
