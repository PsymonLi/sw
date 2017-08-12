#! /usr/bin/python3
import pdb
import copy

import infra.common.defs        as defs
import infra.common.objects     as objects
import infra.common.utils       as utils
import config.resmgr            as resmgr
import infra.config.base        as base

from config.store               import Store
from infra.common.logging       import cfglogger

import config.hal.api            as halapi
import config.hal.defs           as haldefs

class SecurityProfileObject(base.ConfigObjectBase):
    def __init__(self):
        super().__init__()
        self.Clone(Store.templates.Get('SECURITY_PROFILE'))
        self.id = resmgr.SecProfIdAllocator.get()
        return

    def Init(self, spec):
        if spec.fields:
            self.Clone(spec.fields) 
        self.GID(spec.id)
        self.Show()
        return

    def Show(self):
        cfglogger.info("- Security Profile  : %s (id: %d)" %\
                       (self.GID(), self.id))
        return
    
    def __getEnumValue(self, val):
        valstr = "NORM_ACTION_" + val
        return haldefs.nwsec.NormalizationAction.Value(valstr)

    def PrepareHALRequestSpec(self, req_spec):
        req_spec.key_or_handle.profile_id = self.id
        req_spec.cnxn_tracking_en = self.cnxn_tracking_en
        req_spec.ipsg_en = self.ipsg_en
        req_spec.tcp_rtt_estimate_en = self.tcp_rtt_estimate_en
        req_spec.session_idle_timeout = self.session_idle_timeout
        req_spec.tcp_cnxn_setup_timeout = self.tcp_cnxn_setup_timeout
        req_spec.tcp_close_timeout = self.tcp_close_timeout
        req_spec.tcp_close_wait_timeout = self.tcp_close_wait_timeout
        req_spec.ip_normalization_en = self.ip_normalization_en
        req_spec.tcp_normalization_en = self.tcp_normalization_en
        req_spec.icmp_normalization_en = self.icmp_normalization_en
        req_spec.ip_ttl_change_detect_en = self.ip_ttl_change_detect_en
        req_spec.ip_rsvd_flags_action = self.__getEnumValue(self.ip_rsvd_flags_action)
        req_spec.ip_df_action = self.__getEnumValue(self.ip_df_action)
        req_spec.ip_options_action = self.__getEnumValue(self.ip_options_action)
        req_spec.ip_invalid_len_action = self.__getEnumValue(self.ip_invalid_len_action)
        req_spec.ip_normalize_ttl = self.ip_normalize_ttl
        req_spec.icmp_invalid_code_action = self.__getEnumValue(self.icmp_invalid_code_action)
        req_spec.icmp_deprecated_msgs_drop = self.icmp_deprecated_msgs_drop
        req_spec.icmp_redirect_msg_drop = self.icmp_redirect_msg_drop
        req_spec.tcp_non_syn_first_pkt_drop = self.tcp_non_syn_first_pkt_drop
        req_spec.tcp_syncookie_en = self.tcp_syncookie_en
        req_spec.tcp_split_handshake_detect_en = self.tcp_split_handshake_detect_en
        req_spec.tcp_split_handshake_drop = self.tcp_split_handshake_drop
        req_spec.tcp_rsvd_flags_action = self.__getEnumValue(self.tcp_rsvd_flags_action)
        req_spec.tcp_unexpected_mss_action = self.__getEnumValue(self.tcp_unexpected_mss_action)
        req_spec.tcp_unexpected_win_scale_action = self.__getEnumValue(self.tcp_unexpected_win_scale_action)
        req_spec.tcp_urg_ptr_not_set_action = self.__getEnumValue(self.tcp_urg_ptr_not_set_action)
        req_spec.tcp_urg_flag_not_set_action = self.__getEnumValue(self.tcp_urg_flag_not_set_action)
        req_spec.tcp_urg_payload_missing_action = self.__getEnumValue(self.tcp_urg_payload_missing_action)
        req_spec.tcp_rst_with_data_action = self.__getEnumValue(self.tcp_rst_with_data_action)
        req_spec.tcp_data_len_gt_mss_action = self.__getEnumValue(self.tcp_data_len_gt_mss_action)
        req_spec.tcp_data_len_gt_win_size_action = self.__getEnumValue(self.tcp_data_len_gt_win_size_action)
        req_spec.tcp_unexpected_ts_option_action = self.__getEnumValue(self.tcp_unexpected_ts_option_action)
        req_spec.tcp_unexpected_echo_ts_action = self.__getEnumValue(self.tcp_unexpected_echo_ts_action)
        req_spec.tcp_ts_not_present_drop = self.tcp_ts_not_present_drop
        req_spec.tcp_invalid_flags_drop = self.tcp_invalid_flags_drop
        req_spec.tcp_flags_nonsyn_noack_drop = self.tcp_flags_nonsyn_noack_drop
        return

    def ProcessHALResponse(self, req_spec, resp_spec):
        self.hal_handle = resp_spec.profile_status.profile_handle
        cfglogger.info("  - SecurityProfile %s = %s (HDL = 0x%x)" %\
                       (self.GID(), \
                        haldefs.common.ApiStatus.Name(resp_spec.api_status),
                        self.hal_handle))
        return

# Helper Class to Generate/Configure/Manage Security Profile Objects
class SecurityProfileObjectHelper:
    def __init__(self):
        self.sps = []
        return

    def Configure(self):
        cfglogger.info("Configuring %d SecurityProfiles." % len(self.sps)) 
        halapi.ConfigureSecurityProfiles(self.sps)
        return
        
    def Generate(self):
        spec = Store.specs.Get('SECURITY_PROFILES')
        cfglogger.info("Creating %d SecurityProfiles." % len(spec.profiles))
        for p in spec.profiles:
            profile = SecurityProfileObject()
            profile.Init(p)
            self.sps.append(profile)
        Store.objects.SetAll(self.sps)
        return

    def main(self):
        self.Generate()
        self.Configure()
        return

SecurityProfileHelper = SecurityProfileObjectHelper()
