# /usr/bin/python3
import socket
import pdb

import infra.common.defs        as defs
import infra.common.objects     as objects
import infra.config.base        as base
import iris.config.resmgr            as resmgr
import iris.config.objects.flow      as flow

from iris.config.store               import Store
from infra.common.logging       import logger
from iris.config.objects.swdr        import SwDscrRingHelper

import iris.config.hal.defs          as haldefs
import iris.config.hal.api           as halapi

class ProxyrCbObject(base.ConfigObjectBase):
    def __init__(self):
        super().__init__()
        self.Clone(Store.templates.Get('PROXYRCB'))
        return
        
    #def Init(self, spec_obj):
    def Init(self, qid):
        if halapi.IsHalDisabled(): qid = resmgr.ProxyrCbIdAllocator.get()
        self.id = qid
        gid = "ProxyrCb%04d" % qid
        self.GID(gid)
        # self.spec = spec_obj
        # logger.info("  - %s" % self)

        # self.uplinks = objects.ObjectDatabase()
        # for uplink_spec in self.spec.uplinks:
            # uplink_obj = uplink_spec.Get(Store)
            # self.uplinks.Set(uplink_obj.GID(), uplink_obj)

        # assert(len(self.uplinks) > 0)
        logger.info("  - %s" % self)

        self.proxyrcbq = SwDscrRingHelper.main("PROXYRCBQ", gid, self.id)
        return

    def FlowKeyBuild(self, flow):
        self.vrf = flow._FlowObject__session.initiator.ep.tenant.id
        self.sport = flow.sport
        self.dport = flow.dport
        self.ip_proto = socket.IPPROTO_TCP

        if flow.SepIsRemote():
            # FLOW_DIR_FROM_UPLINK
            self.dir = 1
        else:
            # FLOW_DIR_FROM_ENIC
            self.dir = 0

        if flow.IsIflow():
            # FLOW_ROLE_INITIATOR
            self.role = 0
        else:
            # FLOW_ROLE_RESPONDER
            self.role = 1

        if flow.IsIPV4():
            self.af = socket.AF_INET
        else:
            self.af = socket.AF_INET6

        self.ip_sa = flow.sip
        self.ip_da = flow.dip
        return

    def PrepareHALRequestSpec(self, req_spec):
        #req_spec.meta.proxyrcb_id          = self.id
        req_spec.key_or_handle.proxyrcb_id  = self.id
        if req_spec.__class__.__name__ != 'ProxyrCbGetRequest':
           req_spec.proxyrcb_flags               = self.proxyrcb_flags
           req_spec.redir_span                   = self.redir_span
           req_spec.my_txq_base                  = self.my_txq_base
           req_spec.my_txq_ring_size_shift       = self.my_txq_ring_size_shift
           req_spec.my_txq_entry_size_shift      = self.my_txq_entry_size_shift

           req_spec.chain_rxq_base               = self.chain_rxq_base
           req_spec.chain_rxq_ring_indices_addr  = self.chain_rxq_ring_indices_addr
           req_spec.chain_rxq_ring_size_shift    = self.chain_rxq_ring_size_shift
           req_spec.chain_rxq_entry_size_shift   = self.chain_rxq_entry_size_shift
           req_spec.chain_rxq_ring_index_select  = self.chain_rxq_ring_index_select

           req_spec.af                           = self.af
           if self.af == socket.AF_INET:
               req_spec.ip_sa.ip_af              = haldefs.common.IP_AF_INET
               req_spec.ip_sa.v4_addr            = socket.htonl(self.ip_sa.getnum())
               req_spec.ip_da.ip_af              = haldefs.common.IP_AF_INET
               req_spec.ip_da.v4_addr            = socket.htonl(self.ip_da.getnum())
           elif self.af == socket.AF_INET6:
               req_spec.ip_sa.ip_af              = haldefs.common.IP_AF_INET6
               req_spec.ip_sa.v6_addr            = self.ip_sa.getnum().to_bytes(16, 'big')
               req_spec.ip_da.ip_af              = haldefs.common.IP_AF_INET6
               req_spec.ip_da.v6_addr            = self.ip_da.getnum().to_bytes(16, 'big')

           req_spec.sport                        = socket.htons(self.sport)
           req_spec.dport                        = socket.htons(self.dport)
           req_spec.vrf                          = socket.htons(self.vrf)
           req_spec.ip_proto                     = self.ip_proto

           req_spec.dir                          = self.dir
           req_spec.role                         = self.role
           req_spec.rev_cb_id                    = self.rev_cb_id

        return

    def ProcessHALResponse(self, req_spec, resp_spec):
        logger.info("  - ProxyrCb %s = %s" %\
                       (self.id, \
                        haldefs.common.ApiStatus.Name(resp_spec.api_status)))
        if resp_spec.__class__.__name__ != 'ProxyrCbResponse':
            self.proxyrcb_flags               = resp_spec.spec.proxyrcb_flags
            self.redir_span                   = resp_spec.spec.redir_span
            self.my_txq_base                  = resp_spec.spec.my_txq_base
            self.my_txq_ring_size_shift       = resp_spec.spec.my_txq_ring_size_shift
            self.my_txq_entry_size_shift      = resp_spec.spec.my_txq_entry_size_shift

            self.chain_rxq_base               = resp_spec.spec.chain_rxq_base
            self.chain_rxq_ring_indices_addr  = resp_spec.spec.chain_rxq_ring_indices_addr
            self.chain_rxq_ring_size_shift    = resp_spec.spec.chain_rxq_ring_size_shift
            self.chain_rxq_entry_size_shift   = resp_spec.spec.chain_rxq_entry_size_shift
            self.chain_rxq_ring_index_select  = resp_spec.spec.chain_rxq_ring_index_select
            self.pi                           = resp_spec.spec.pi
            self.ci                           = resp_spec.spec.ci

            if resp_spec.spec.af == socket.AF_INET:
                self.ip_sa.v4_addr            = socket.ntohl(resp_spec.spec.ip_sa.v4_addr)
                self.ip_da.v4_addr            = socket.ntohl(resp_spec.spec.ip_da.v4_addr)
            elif resp_spec.spec.af == socket.AF_INET6:
                self.ip_sa.v6_addr            = objects.Ipv6Address(integer=resp_spec.spec.ip_sa.v6_addr)
                self.ip_da.v6_addr            = objects.Ipv6Address(integer=resp_spec.spec.ip_da.v6_addr)

            self.af                           = resp_spec.spec.af
            self.sport                        = socket.ntohs(resp_spec.spec.sport)
            self.dport                        = socket.ntohs(resp_spec.spec.dport)
            self.vrf                          = socket.ntohs(resp_spec.spec.vrf)
            self.ip_proto                     = resp_spec.spec.ip_proto

            self.dir                          = resp_spec.spec.dir
            self.role                         = resp_spec.spec.role
            self.rev_cb_id                    = resp_spec.spec.rev_cb_id

            self.stat_pkts_redir              = resp_spec.spec.stat_pkts_redir
            self.stat_pkts_discard            = resp_spec.spec.stat_pkts_discard
            self.stat_cb_not_ready            = resp_spec.spec.stat_cb_not_ready
            self.stat_null_ring_indices_addr  = resp_spec.spec.stat_null_ring_indices_addr
            self.stat_aol_err                 = resp_spec.spec.stat_aol_err
            self.stat_rxq_full                = resp_spec.spec.stat_rxq_full
            self.stat_txq_empty               = resp_spec.spec.stat_txq_empty
            self.stat_sem_alloc_full          = resp_spec.spec.stat_sem_alloc_full
            self.stat_sem_free_full           = resp_spec.spec.stat_sem_free_full

        return

    def GetObjValPd(self):
        lst = []
        lst.append(self)
        halapi.GetProxyrCbs(lst)
        return

    def SetObjValPd(self):
        lst = []
        lst.append(self)
        halapi.UpdateProxyrCbs(lst)
        return

    def IsFilterMatch(self, spec):
        return super().IsFilterMatch(spec.filters)

    def Read(self):
        return

    def Write(self):
        return

    #
    # Print proxyrcb statistics
    #
    def StatsPrint(self):
        logger.info("PROXYRCB %d stat_pkts_redir %d" % 
              (self.id, self.stat_pkts_redir))
        logger.info("PROXYRCB %d stat_pkts_discard %d" % 
              (self.id, self.stat_pkts_discard))
        logger.info("PROXYRCB %d stat_cb_not_ready %d" % 
              (self.id, self.stat_cb_not_ready))
        logger.info("PROXYRCB %d stat_null_ring_indices_addr %d" % 
              (self.id, self.stat_null_ring_indices_addr))
        logger.info("PROXYRCB %d stat_aol_err %d" % 
              (self.id, self.stat_aol_err))
        logger.info("PROXYRCB %d stat_rxq_full %d" % 
              (self.id, self.stat_rxq_full))
        logger.info("PROXYRCB %d stat_txq_empty %d" % 
              (self.id, self.stat_txq_empty))
        logger.info("PROXYRCB %d stat_sem_alloc_full %d" % 
              (self.id, self.stat_sem_alloc_full))
        logger.info("PROXYRCB %d stat_sem_free_full %d" % 
              (self.id, self.stat_sem_free_full))
        return


# Helper Class to Generate/Configure/Manage ProxyrCb Objects.
class ProxyrCbObjectHelper:
    def __init__(self):
        self.objlist = []
        return

    def Configure(self, objlist = None):
        if objlist == None:
            objlist = Store.objects.GetAllByClass(ProxyrCbObject)
        logger.info("Configuring %d ProxyrCbs." % len(objlist)) 
        halapi.ConfigureProxyrCbs(objlist)
        return
        
    def __gen_one(self, qid):
        logger.info("Creating ProxyrCb")
        proxyrcb_obj = ProxyrCbObject()
        proxyrcb_obj.Init(qid)
        Store.objects.Add(proxyrcb_obj)
        return proxyrcb_obj

    def Generate(self, qid):
        obj = self.__gen_one(qid)
        self.objlist.append(obj)
        lst = []
        lst.append(obj)
        self.Configure(lst)
        return self.objlist

    def main(self, qid):
        gid = "ProxyrCb%04d" % qid
        if Store.objects.IsKeyIn(gid):
            return Store.objects.Get(gid)
        objlist = self.Generate(qid)
        return objlist

ProxyrCbHelper = ProxyrCbObjectHelper()
