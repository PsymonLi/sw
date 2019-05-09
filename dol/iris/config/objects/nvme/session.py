#! /usr/bin/python3
import pdb

import scapy.all                as scapy
import infra.common.defs        as defs
import infra.common.objects     as objects
import infra.config.base        as base

import iris.config.resmgr            as resmgr
from iris.config.store               import Store
from infra.common.logging       import logger

import iris.config.hal.api           as halapi
import iris.config.hal.defs          as haldefs

from infra.common.glopts        import GlobalOptions

from iris.config.objects.session     import SessionHelper

from scapy.utils import inet_aton, inet_ntoa, inet_pton
import struct
import socket

class NvmeSessionObject(base.ConfigObjectBase):
    def __init__(self):
        super().__init__()
        self.Clone(Store.templates.Get('NVME_SESSION'))
        return

    def Init(self, session):
        self.id = resmgr.NvmeSessionIdAllocator.get()
        self.GID("NvmeSession%d" % self.id)
        self.session = session
        self.IsIPV6 = session.IsIPV6()
        self.sq_id = None
        self.cq_id = None
        self.sq = None
        self.cq = None
        self.ns = None
        self.host_page_size = None
        self.mtu = 4096

    def Configure(self):
        nvme_lif = self.session.initiator.ep.intf.lif.nvme_lif
        self.lif = nvme_lif
        self.nsid = nvme_lif.GetNextNsid()
        self.ns = nvme_lif.GetNs(self.nsid)
        assert self.ns != 0

        #for DOL purpose, map session to a sq/cq.
        #more than one session can map to same sq/cq
        #for now using nsid%2 as the sq/cq number

        self.sqid = self.nsid % 2
        self.cqid = self.sqid

        self.sq = self.session.initiator.ep.intf.lif.GetQ('NVME_SQ', self.sqid)
        self.cq = self.session.initiator.ep.intf.lif.GetQ('NVME_CQ', self.cqid)
        
        nvme_lif.NsSessionAttach(self.nsid, self)
        halapi.NvmeSessionCreate([self])
        return
         
    def Show(self):
        logger.info('Nvme Session: %s' % self.GID())
        logger.info('- Session: %s' % self.session.GID())
        logger.info('- sess_id: %s ' % self.sess_id)
        logger.info('- nsid: %s ' % self.nsid)
        logger.info('- sqid: %s cqid: %s ' %(self.sqid, self.cqid))
        logger.info('- hw_lif_id: %s ' % self.session.initiator.ep.intf.lif.hw_lif_id)
        logger.info('- vrf_id: %s ' % self.session.initiator.ep.tenant.id)
        logger.info('- xtstxbase: 0x%x size: %d' \
                     % (self.tx_xtsq_base, self.tx_xtsq_num_entries))
        logger.info('- dgsttxbase: 0x%x size: %d' \
                     % (self.tx_dgstq_base, self.tx_dgstq_num_entries))
        logger.info('- xtsrxbase: 0x%x size: %d' \
                     % (self.rx_xtsq_base, self.rx_xtsq_num_entries))
        logger.info('- dgstrxbase: 0x%x size: %d' \
                     % (self.rx_dgstq_base, self.rx_dgstq_num_entries))
        logger.info('- sesqbase: 0x%x size: %d' \
                     % (self.tx_sesq_base, self.tx_sesq_num_entries))
        logger.info('- serqbase: 0x%x size: %d' \
                     % (self.rx_serq_base, self.rx_serq_num_entries))
        return

    def PrepareHALRequestSpec(self, req_spec):
        if (GlobalOptions.dryrun): return

        req_spec.vrf_key_handle.vrf_id = self.session.initiator.ep.tenant.id
        req_spec.hw_lif_id = self.session.initiator.ep.intf.lif.hw_lif_id
        req_spec.nsid = self.nsid
        self.session.iflow.PrepareHALRequestFlowKeySpec(req_spec)
        return

    def ProcessHALResponse(self, req_spec, resp_spec):
        logger.info("ProcessHALResponse:: Nvme Session: %s Session: %s "
                     % (self.GID(), self.session.GID()))
    
        self.sess_id = resp_spec.sess_id #LIF local Session ID
        self.txsessprodcb_addr = resp_spec.txsessprodcb_addr #HBM address of Tx Sess Producer CB
        self.rxsessprodcb_addr = resp_spec.rxsessprodcb_addr #HBM address of Rx Sess Producer CB
        self.tx_xtsq_base = resp_spec.tx_xtsq_base #Q base addr of Tx Sess XTSQ
        self.tx_xtsq_num_entries = resp_spec.tx_xtsq_num_entries #Q num_entries of Tx Sess XTSQ
        self.tx_dgstq_base = resp_spec.tx_dgstq_base #Q base addr of Tx Sess DGSTQ
        self.tx_dgstq_num_entries = resp_spec.tx_dgstq_num_entries #Q num_entries of Tx Sess DGSTQ
        self.tx_sesq_base = resp_spec.tx_sesq_base #Q base addr of Tx TCP SESQ
        self.tx_sesq_num_entries = resp_spec.tx_sesq_num_entries #Q num_entries of Tx TCP SESQ
        self.rx_xtsq_base = resp_spec.rx_xtsq_base #Q base addr of Rx Sess XTSQ
        self.rx_xtsq_num_entries = resp_spec.rx_xtsq_num_entries #Q num_entries of Rx Sess XTSQ
        self.rx_dgstq_base = resp_spec.rx_dgstq_base #Q base addr of Rx Sess DGSTQ
        self.rx_dgstq_num_entries = resp_spec.rx_dgstq_num_entries #Q num_entries of Rx Sess DGSTQ
        self.rx_serq_base = resp_spec.rx_serq_base #Q base addr of Rx TCP SERQ
        self.rx_serq_num_entries = resp_spec.rx_serq_num_entries #Q num_entries of Rx TCP SERQ

        # Both R0/R1 of sessxtstx use the same q
        self.tx_xtsq = self.session.initiator.ep.intf.lif.GetQ('NVME_SESS_XTS_TX', self.sess_id)
        self.tx_xtsq.SetRingParams('PREXTS', True, None, self.tx_xtsq_base, self.tx_xtsq_num_entries)
        self.tx_xtsq.SetRingParams('POSTXTS', True, None, self.tx_xtsq_base, self.tx_xtsq_num_entries)

        # Both R0/R1 of sessdgsttx use the same q
        self.tx_dgstq = self.session.initiator.ep.intf.lif.GetQ('NVME_SESS_DGST_TX', self.sess_id)
        self.tx_dgstq.SetRingParams('PREDGST', True, None, self.tx_dgstq_base, self.tx_dgstq_num_entries)
        self.tx_dgstq.SetRingParams('POSTDGST', True, None, self.tx_dgstq_base, self.tx_dgstq_num_entries)

        # Both R0/R1 of sessxtsrx use the same q
        self.rx_xtsq = self.session.initiator.ep.intf.lif.GetQ('NVME_SESS_XTS_RX', self.sess_id)
        self.rx_xtsq.SetRingParams('PREXTS', True, None, self.rx_xtsq_base, self.rx_xtsq_num_entries)
        self.rx_xtsq.SetRingParams('POSTXTS', True, None, self.rx_xtsq_base, self.rx_xtsq_num_entries)

        # R0 of sessdgstrx is same as TCP SERQ
        # R1 of sessdgstrx is the session q
        self.rx_dgstq = self.session.initiator.ep.intf.lif.GetQ('NVME_SESS_DGST_RX', self.sess_id)
        self.rx_dgstq.SetRingParams('PREDGST', True, None, self.rx_serq_base, self.rx_serq_num_entries)
        self.rx_dgstq.SetRingParams('POSTDGST', True, None, self.rx_dgstq_base, self.rx_dgstq_num_entries)

        #TBD: for now there is no modeling object for TCP SESQ
        return

    def IsIPV6(self):
        return self.IsIPV6

    def IsFilterMatch(self, selectors):
        logger.debug('Matching Nvme Session: %s' % self.GID())

        #TBD: add nvme specific filters here
        print(str(selectors))

        if hasattr(selectors.nvmesession, 'base'):
            match = super().IsFilterMatch(selectors.nvmesession.base.filters)
            if match == False: return  match

        if hasattr(selectors.nvmesession, 'session'):
            match = super().IsFilterMatch(selectors.nvmesession.session.filters)
            logger.debug("- IsIPV6 Filter Match =", match)
            if match == False: return  match
        
        return True

    def SetupTestcaseConfig(self, obj):
        obj.nvmesession = self;
        return

    def ShowTestcaseConfig(self, obj):
        logger.info("Config Objects for %s" % self.GID())
        return

class NvmeSessionObjectHelper:
    def __init__(self):
        self.nvme_sessions = []
        return

    def Generate(self):
        logger.info("Generating NVME sessions..")
        self.nw_sessions = SessionHelper.GetAllMatchingLabel('NVME-PROXY')
        for nw_s in self.nw_sessions:

            ep1 = nw_s.initiator.ep
            ep2 = nw_s.responder.ep

            # create nvme sessions only for nw sessions between 'local' initiator
            # and 'remote' responder which has TCP port 4420
            if ep1.remote: continue
            if not ep2.remote: continue
            if not nw_s.responder.proto == 'TCP': continue
            if not nw_s.responder.port == 4420: continue

            logger.info("Nvme PICKED: EP1 %s (%s) EP2 %s (%s) IPv6 %d " 
                         % (ep1.GID(), not ep1.remote, ep2.GID(), not ep2.remote, 
                            nw_s.IsIPV6()))

            Nvme_s = NvmeSessionObject()
            Nvme_s.Init(nw_s)
            self.nvme_sessions.append(Nvme_s)

        return

    def Configure(self):
        logger.info("Configuring NVME sessions..")
        if GlobalOptions.agent: return
        if GlobalOptions.dryrun: return
        for Nvme_s in self.nvme_sessions:
            logger.info('Walking Nvme Session: %s ' % 
                        (Nvme_s.GID()))
            Nvme_s.Configure()

    def main(self):
        self.Generate()
        self.Configure()
        if len(self.nvme_sessions):
            Store.objects.SetAll(self.nvme_sessions) 

        [s.Show() for s in self.nvme_sessions]

    def __get_matching_sessions(self, selectors = None):
        ssns = []
        for ssn in self.nvme_sessions:
            if ssn.IsFilterMatch(selectors):
                ssns.append(ssn)
        return ssns

    def GetMatchingConfigObjects(self, selectors = None):
        return self.__get_matching_sessions(selectors)

NvmeSessionHelper = NvmeSessionObjectHelper()
def GetMatchingObjects(selectors):
    return NvmeSessionHelper.GetMatchingConfigObjects(selectors)
