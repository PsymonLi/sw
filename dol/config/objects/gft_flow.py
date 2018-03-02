#! /usr/bin/python3

import infra.common.defs        as defs
import infra.config.base        as base
import config.resmgr            as resmgr

import config.hal.api           as halapi
import config.hal.defs          as haldefs

import config.objects.session   as session

from infra.common.glopts        import GlobalOptions
from infra.common.logging       import cfglogger
from config.store               import Store

class GftFlowObject(base.ConfigObjectBase):
    def __init__(self):
        super().__init__()
        self.Clone(Store.templates.Get('GFT_FLOW'))
        self.id = resmgr.GftFlowIdAllocator.get()
        self.GID("GFT_FLOW_%d" % self.id)
        return

    def Init(self, flow):
        self.flow = flow
        self.exm_profile = self.flow.GetSrcSegmentGftExmProfile()
        self.trsp_profile = self.flow.GetGftTranspositionProfile()
        self.Show()
        return

    def Summary(self):
        summary = ""
        summary += "GftFlow:%s" % self.GID()
        summary += "/ExmProfile:%s" % self.exm_profile.GID()
        summary += "/TrspProfile:%s" % self.trsp_profile.GID()
        return summary

    def PrepareHALRequestSpec(self, req_spec):
        req_spec.key_or_handle.flow_entry_id = self.id
        req_spec.exact_match_profile.profile_id = self.exm_profile.id
        req_spec.transposition_profile.profile_id = self.trsp_profile.id

        req_spec.add_in_activated_state = False
        req_spec.rdma_flow = False
        
        self.trsp_profile.FillTranspositionActions(req_spec)

        tts = 'GFT_TABLE_TYPE_' + self.exm_profile.table_type
        req_spec.table_type = haldefs.gft.GftTableType.Value(tts)
        req_spec.vport_id = 0
        req_spec.redirect_vport_id = 0
        req_spec.ttl_one_redirect_vport_id = 0

        for grp in self.exm_profile.groups:
            exm = req_spec.exact_matches.add()
            grp.PrepareGftFlowHALRequestSpec(exm, self.flow)

        for trp_grp in self.trsp_profile.groups:
            grp_spec = req_spec.transpositions.add()
            trp_grp.PrepareGftFlowHALRequestSpec(grp_spec, self.flow)
        return

    def ProcessHALResponse(self, req_spec, resp_spec):
        self.hal_handle = resp_spec.status.flow_entry_handle
        cfglogger.info("- GFT Flow %s = %s(HDL = %x)" %\
                       (self.GID(), haldefs.common.ApiStatus.Name(resp_spec.api_status),
                        self.hal_handle))

        return

    def Show(self):
        cfglogger.info("GFT Flow: %s" % self.GID())
        cfglogger.info("- Match Fields:")
        cfglogger.info("  - Eth Header  : Smac:%s/Dmac:%s/Etype:%#x" %\
                       (self.flow.smac.get(), self.flow.dmac.get(), 0x800))
        cfglogger.info("  - IPv4 Header : Sip:%s/Dip:%s/Proto:%s" %\
                       (self.flow.sip.get(), self.flow.dip.get(), self.flow.proto))
        if self.flow.IsUDP():
            cfglogger.info("  - UDP Header  : Sport:%d/Dport:%d" %\
                           (self.flow.sport, self.flow.dport))
        if self.flow.IsTCP():
            cfglogger.info("  - TCP Header  : Sport:%d/Dport:%d" %\
                           (self.flow.sport, self.flow.dport))
        if self.flow.IsICMP():
            cfglogger.info("  - ICMP Header : Type:%d/Code:%d" %\
                           (self.flow.icmptype, self.flow.icmpcode))
        return


class GftFlowObjectHelper:
    def __init__(self):
        self.objlist = []
        return

    def Configure(self):
        cfglogger.info("Configuring %d GFT Flows." % len(self.objlist))
        halapi.ConfigureGftFlows(self.objlist)
        return

    def Generate(self):
        ssns = Store.objects.GetAllByClass(session.SessionObject)
        for ssn in ssns:
            gft_iflow = GftFlowObject()
            gft_iflow.Init(ssn.iflow)
            self.objlist.append(gft_iflow)
            ssn.iflow.SetGftFlow(gft_iflow)

            gft_rflow = GftFlowObject()
            gft_rflow.Init(ssn.rflow)
            self.objlist.append(gft_rflow)
            ssn.rflow.SetGftFlow(gft_rflow)
        return

    def main(self):
        if not GlobalOptions.gft:
            return
        self.Generate()
        self.Configure()
        cfglogger.info("Adding %d GFT Flows to Store." % len(self.objlist))
        if len(self.objlist) == 0: return
        Store.objects.SetAll(self.objlist)
        return

    def GetAll(self):
        return self.objlist

GftFlowHelper = GftFlowObjectHelper()
