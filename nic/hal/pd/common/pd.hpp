#ifndef __HAL_COMMON_PD_HPP__
#define __HAL_COMMON_PD_HPP__

namespace hal {
namespace pd {

typedef struct pd_vrf_s pd_vrf_t;
typedef struct pd_nwsec_profile_s pd_nwsec_profile_t;
typedef struct pd_dos_policy_s pd_dos_policy_t;
typedef struct pd_l2seg_s pd_l2seg_t;
typedef struct pd_lif_s pd_lif_t;
typedef struct pd_uplinkif_s pd_uplinkif_t;
typedef struct pd_uplinkpc_s pd_uplinkpc_t;
typedef struct pd_enicif_s pd_enicif_t;
typedef struct pd_if_l2seg_entry_s pd_if_l2seg_entry_t;
typedef struct pd_cpuif_s pd_cpuif_t;
typedef struct pd_tunnelif_s pd_tunnelif_t;
typedef struct pd_if_s pd_if_t;
typedef struct pd_ep_s pd_ep_t;
typedef struct pd_ep_ip_entry_s pd_ep_ip_entry_t;
typedef struct pd_flow_s pd_flow_t;
typedef struct pd_session_s pd_session_t;
typedef struct pd_tlscb_s pd_tlscb_t;
typedef struct pd_tcpcb_s pd_tcpcb_t;
typedef struct pd_buf_pool_s pd_buf_pool_t;
typedef struct pd_queue_s pd_queue_t;
typedef struct pd_policer_s pd_policer_t;
typedef struct pd_acl_s pd_acl_t;
typedef struct pd_wring_s pd_wring_t;
typedef struct pd_wring_meta_s pd_wring_meta_t; 
typedef struct pd_ipseccb_encrypt_s pd_ipseccb_encrypt_t;
typedef struct pd_ipseccb_decrypt_s pd_ipseccb_decrypt_t;
typedef struct pd_l4lb_s pd_l4lb_t;
typedef struct pd_cpucb_s pd_cpucb_t;
typedef struct pd_rawrcb_s pd_rawrcb_t;
typedef struct pd_rawccb_s pd_rawccb_t;
typedef struct pd_mc_entry_s pd_mc_entry_t;

}    // namespace pd
}    // namespace hal

#endif    // __HAL_COMMON_PD_HPP__

