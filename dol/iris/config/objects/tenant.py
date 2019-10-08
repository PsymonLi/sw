# /usr/bin/python3
import pdb
import copy

import infra.common.defs        as defs
import infra.common.objects     as objects
import infra.config.base        as base

import iris.config.resmgr            as resmgr
import iris.config.objects.segment   as segment
import iris.config.objects.lif       as lif
import iris.config.objects.tunnel    as tunnel
import iris.config.objects.span      as span
import iris.config.objects.collector as collector
import iris.config.objects.l4lb      as l4lb

import iris.config.objects.security_group    as security_group
import iris.config.objects.dos_policy        as dos_policy

from iris.config.store               import Store
from infra.common.logging       import logger
from infra.common.glopts        import GlobalOptions

import iris.config.hal.defs          as haldefs
import iris.config.hal.api           as halapi
import iris.config.agent.api         as agentapi

class AgentTenantObject(base.AgentObjectBase):
    def __init__(self, tenant):
        super().__init__("Tenant", tenant.GID(), tenant.GID())
        return

class TenantObject(base.ConfigObjectBase):
    def __init__(self):
        super().__init__()
        self.Clone(Store.templates.Get('TENANT'))
        return

    def Init(self, spec, entryspec, topospec):
        self.id = resmgr.TenIdAllocator.get()
        gid = "Ten%04d" % self.id
        self.GID(gid)

        self.hostpinned = GlobalOptions.hostpin
        self.spec = copy.deepcopy(spec)
        self.type = self.spec.type.upper()
        self.qos_enable = getattr(topospec, 'qos', False)
        self.ep_learn = None
        fte = getattr(self.spec , 'fte', None)
        if fte:
            self.ep_learn = getattr(fte, 'ep_learn', None)

        self.label = getattr(spec, 'label', None)
        self.overlay = spec.overlay.upper()
        self.security_profile = None
        self.bhseg = None

        if getattr(spec, 'security_profile', None) is not None:
            self.security_profile = spec.security_profile.Get(Store)
        if self.IsInfra():
            self.subnet     = resmgr.TepIpSubnetAllocator.get()
            self.ipv4_pool  = resmgr.CreateIpv4AddrPool(self.subnet.get())
            self.local_tep  = self.ipv4_pool.get()
            self.gipo_prefix= resmgr.GIPoAddressAllocator.get()
            self.gipo_len   = 16; # Hardcoding the GIPo Prefix Length

        # ClassicNicMode Specific Stuff
        if GlobalOptions.classic:
            # Process Pinned Uplinks
            self.pinif = getattr(entryspec, 'uplink', None)
            self.pinif = self.pinif.Get(Store)
            self.classic_enics = None
        self.Show()

        # Process LIFs
        self.lifns = entryspec.lifns
        self.obj_helper_lif = lif.LifObjectHelper()
        self.__create_lifs()
        # Process Segments
        self.obj_helper_segment = segment.SegmentObjectHelper()
        self.__create_segments()
        # Process L4LbServices
        self.obj_helper_l4lb = l4lb.L4LbServiceObjectHelper()
        self.__create_l4lb_services()
        # Process Tunnels
        if self.IsInfra():
            self.obj_helper_tunnel = tunnel.TunnelObjectHelper()
            self.__create_infra_tunnels()
        elif 'tunnels' in self.spec.__dict__ and len(self.spec.tunnels) > 0:
            # this is not infra
            self.obj_helper_tunnel = tunnel.TunnelObjectHelper()
            self.__create_tenant_tunnels()

        # Process Span Sessions
        if self.IsSpan():
            self.obj_helper_span = span.SpanSessionObjectHelper()
            self.__create_span_sessions()
            self.obj_helper_collector = collector.CollectorHelper

        self.obj_helper_sg = security_group.SecurityGroupObjectHelper()
        self.obj_helper_sg.main(self)
        self.obj_helper_dosp = dos_policy.DosPolicyObjectHelper()
        #self.obj_helper_dosp.main(self)
        self.obj_helper_dosp.Generate(self)
        return

    def SetClassicEnics(self, enics):
        self.enic_config_done = False
        self.classic_enics = enics
        return

    def GetClassicEnics(self):
        return self.classic_enics

    def GetPromiscuousEnic(self):
        for e in self.classic_enics:
            if e.IsPromiscous():
                return e
        pdb.set_trace()
        return None

    def GetClassicEnicsForConfig(self):
        if self.enic_config_done is False:
            self.enic_config_done = True
            return self.classic_enics
        return None

    def __copy__(self):
        ten = TenantObject()
        ten.id = self.id
        ten.security_profile_handle = self.security_profile_handle
        ten.security_profile = self.security_profile
        return ten


    def Equals(self, other, lgh):
        if not isinstance(other, self.__class__):
            return False
        fields = ["id", "security_profile_handle"]
        if not self.CompareObjectFields(other, fields, lgh):
            return False
        return True

    def Show(self):
        logger.info("Tenant  : %s" % self.GID())
        logger.info("- Type          : %s" % self.type)
        logger.info("- HostPinned    : %s" % self.hostpinned)
        if self.IsInfra():
            logger.info("- LocalTep  : %s" % self.local_tep.get())
            logger.info("- GIPo      : %s/%d" % (self.gipo_prefix.get(), self.gipo_len))
        if GlobalOptions.classic:
            logger.info("- ClassicNicMode: True")
            logger.info("- Pinned IF     : %s" % self.pinif.GID())
        return

    def Summary(self):
        summary = ''
        summary += 'GID:%s' % self.GID()
        summary += '/Type:%s' % self.type
        if self.label:
            summary += '/Label:%s' % self.label
        if self.IsInfra():
            summary += '/LocTep:%s' % self.local_tep.get()
            summary += '/GIPo:%s/%d' % (self.gipo_prefix.get(), self.gipo_len)
        return summary

    def IsInfra(self):
        return self.type == 'INFRA'
    def IsSpan(self):
        return self.type == 'SPAN'
    def IsTenant(self):
        return self.type == 'TENANT'
    def IsHostPinned(self):
        return self.hostpinned
    def IsQosEnabled(self):
        return self.qos_enable

    # Comment this before removing. Vxlan or Vlan is a segment level prop.
    #def IsOverlayVxlan(self):
    #    return self.overlay == 'VXLAN'
    #def IsOverlayVlan(self):
    #    return self.overlay == 'VLAN'

    def IsL4LbEnabled(self):
        return self.l4lb_enable == True
    def AllocL4LbBackend(self, remote, tnnled, pick_ep_in_l2seg_vxlan):
        return self.obj_helper_segment.AllocL4LbBackend(remote, tnnled, pick_ep_in_l2seg_vxlan)

    def IsIPV4EpLearnEnabled(self):
        if GlobalOptions.rtl:
            return False
        return self.ep_learn and self.ep_learn.ipv4

    def IsIPV6EpLearnEnabled(self):
        if GlobalOptions.rtl:
            return False
        return self.ep_learn and self.ep_learn.ipv6

    def __create_l4lb_services(self):
        if 'l4lb' not in self.spec.__dict__:
            self.l4lb_enable = False
            return

        self.l4lb_enable = True
        spec = self.spec.l4lb.Get(Store)
        self.obj_helper_l4lb.Generate(self, spec)
        return

    def __create_segments(self):
        for entry in self.spec.segments:
            spec = entry.spec.Get(Store)
            self.obj_helper_segment.Generate(self, spec, entry.count)
        self.obj_helper_segment.AddToStore()
        self.bhseg = self.obj_helper_segment.GetBlackholeSegment()
        return

    def __create_lifs(self):
        enic_spec = getattr(self.spec, 'enics', None)
        n_prom = 0
        n_allmc = 0
        if enic_spec:
            n_prom = getattr(enic_spec, 'promiscuous', 0)
            n_allmc = getattr(enic_spec, 'allmulti', 0)
        self.spec.lif = self.spec.lif.Get(Store)
        self.obj_helper_lif.Generate(self, self.spec.lif,
                                     self.lifns, n_prom, n_allmc)
        self.obj_helper_lif.Configure()
        return

    def __create_infra_tunnels(self):
        #Don't create tunnels if Learning enabled for now.
        if self.IsIPV4EpLearnEnabled() or self.IsIPV6EpLearnEnabled():
            return
        for entry in self.spec.tunnels:
            spec = entry.spec.Get(Store)
            self.obj_helper_tunnel.Generate(self, spec, self.GetEps())
        self.obj_helper_tunnel.AddToStore()
        return

    def __create_tenant_tunnels(self):
        #Don't create tunnels if Learning enabled for now.
        if self.IsIPV4EpLearnEnabled() or self.IsIPV6EpLearnEnabled():
            return
        reps = self.GetRemoteEps()
        leps = self.GetLocalEps()
        id = 0
        lid = 0
        for entry in self.spec.tunnels:
            spec = entry.spec.Get(Store)
            isLocal = entry.local
            if id >= len(reps):
                return
            eps = []
            if isLocal is True:
                eps.append(leps[lid])
                lid = lid + 1
            else:
                eps.append(reps[id])
                id = id + 1
            self.obj_helper_tunnel.Generate(self, spec, eps)
        self.obj_helper_tunnel.AddToStore()


    def __create_span_sessions(self):
        self.obj_helper_span.main(self, self.spec)
        return

    def ConfigureCollectors(self):
        if self.IsSpan():
            self.obj_helper_collector.main(self, self.spec)
        return

    def AllocLif(self):
        return self.obj_helper_lif.Alloc()

    def ConfigureSegments(self):
        return self.obj_helper_segment.Configure()

    def ConfigureSegmentsPhase2(self):
        return self.obj_helper_segment.ConfigurePhase2()

    def ConfigureTunnels(self):
        if self.IsInfra() or ('tunnels' in self.spec.__dict__ and len(self.spec.tunnels) > 0):
            self.obj_helper_tunnel.Configure()
        return

    def ConfigureDosPolicies(self):
        self.obj_helper_dosp.Configure()
        return

    def ConfigureL4LbServices(self):
        return self.obj_helper_l4lb.Configure()

    def GetSegments(self):
        return self.obj_helper_segment.segs

    def GetSpanSegment(self):
        for seg in self.obj_helper_segment.segs:
            if seg.IsSpanSegment():
                return seg
        return None

    def GetL4LbServices(self):
        return self.obj_helper_l4lb.svcs

    def GetEps(self, backend = False):
        return self.obj_helper_segment.GetEps(backend)

    def GetRemoteEps(self, backend = False):
        return self.obj_helper_segment.GetRemoteEps(backend)

    def GetLocalEps(self, backend = False):
        return self.obj_helper_segment.GetLocalEps(backend)

    def GetPinIf(self):
        return self.pinif

    def GetSecurityGroups(self):
        return self.obj_helper_sg.sgs
    def GetLocalSecurityGroups(self):
        return self.obj_helper_sg.GetLocal()
    def GetRemoteSecurityGroups(self):
        return self.obj_helper_sg.GetRemote()

    def PrepareHALRequestSpec(self, reqspec):
        reqspec.key_or_handle.vrf_id = self.id
        if self.security_profile:
            reqspec.security_key_handle.profile_handle = self.security_profile.hal_handle
            self.security_profile_handle = self.security_profile.hal_handle
        if self.IsInfra():
            reqspec.vrf_type = haldefs.common.VRF_TYPE_INFRA
            reqspec.mytep_ip.ip_af = haldefs.common.IP_AF_INET
            reqspec.mytep_ip.v4_addr = self.local_tep.getnum()
            reqspec.gipo_prefix.address.ip_af = haldefs.common.IP_AF_INET
            reqspec.gipo_prefix.address.v4_addr = self.gipo_prefix.getnum()
            reqspec.gipo_prefix.prefix_len = self.gipo_len
        else:
            if GlobalOptions.classic:
                reqspec.vrf_type = haldefs.common.VRF_TYPE_INBAND_MANAGEMENT
            else:
                reqspec.vrf_type = haldefs.common.VRF_TYPE_CUSTOMER

        return

    def ProcessHALResponse(self, req_spec, resp_spec):
        logger.info("  - Tenant %s = %s" %\
                       (self.GID(), \
                        haldefs.common.ApiStatus.Name(resp_spec.api_status)))
        return

    def PrepareHALGetRequestSpec(self, get_req_spec):
        get_req_spec.key_or_handle.vrf_id = self.id
        return

    def ProcessHALGetResponse(self, get_req_spec, get_resp_spec):
        if get_resp_spec.api_status == haldefs.common.ApiStatus.Value('API_STATUS_OK'):
            self.id = get_resp_spec.spec.key_or_handle.vrf_id
            self.security_profile_handle = get_resp_spec.spec.security_profile_handle
        else:
            self.security_profile_handle = None
        return

    def PrepareAgentObject(self):
        return AgentTenantObject(self)

    def Get(self):
        halapi.GetTenants([self])

    def IsFilterMatch(self, spec):
        return super().IsFilterMatch(spec.filters)

# Helper Class to Generate/Configure/Manage Tenant Objects.
class TenantObjectHelper:
    def __init__(self):
        self.tens = []
        return

    def Configure(self):
        logger.info("Configuring %d Tenants." % len(self.tens))
        if GlobalOptions.agent:
            agentapi.ConfigureTenants(self.tens)
        else:
            halapi.ConfigureTenants(self.tens)

        for ten in self.tens:
            ten.ConfigureSegments()
            ten.ConfigureL4LbServices()
        return

    def ConfigurePhase2(self):
        for ten in self.tens:
            ten.ConfigureSegmentsPhase2()
            ten.ConfigureTunnels()
            ten.ConfigureDosPolicies()
            ten.ConfigureCollectors()
        return

    def Generate(self, topospec):
        tenspec = getattr(topospec, 'tenants', None)
        if tenspec is None:
            return
        for entry in topospec.tenants:
            spec = entry.spec.Get(Store)
            logger.info("Creating %d Tenants" % entry.count)
            for c in range(entry.count):
                ten = TenantObject()
                ten.Init(spec, entry, topospec)
                self.tens.append(ten)
        Store.objects.SetAll(self.tens)
        return

    def main(self, topospec):
        self.Generate(topospec)
        self.Configure()
        return


TenantHelper = TenantObjectHelper()

def GetMatchingObjects(selectors):
    tenants =  Store.objects.GetAllByClass(TenantObject)
    return [ten for ten in tenants if ten.IsFilterMatch(selectors.tenant)]
