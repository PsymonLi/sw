#! /usr/bin/python3
import pdb

import infra.common.defs        as defs
import infra.common.objects     as objects
import infra.config.base        as base

import config.resmgr            as resmgr
from config.store               import Store
from infra.common.logging       import logger

import config.hal.api           as halapi
import config.hal.defs          as haldefs
import rdma_pb2                 as rdma_pb2

import config.objects.slab      as slab
import config.objects.mr        as mr

from factory.objects.rdma.descriptor import RdmaSqDescriptorBase
from factory.objects.rdma.descriptor import RdmaSqDescriptorSend
from factory.objects.rdma.descriptor import RdmaRqDescriptorBase
from factory.objects.rdma.descriptor import RdmaRrqDescriptorBase
from factory.objects.rdma.descriptor import RdmaRrqDescriptorRead
from factory.objects.rdma.descriptor import RdmaRrqDescriptorAtomic
from factory.objects.rdma.descriptor import RdmaRsqDescriptorBase
from factory.objects.rdma.descriptor import RdmaRsqDescriptorRead
from factory.objects.rdma.descriptor import RdmaRsqDescriptorAtomic
from factory.objects.rdma.descriptor import RdmaCqDescriptor
from factory.objects.rdma.descriptor import RdmaSge
from infra.common.glopts import GlobalOptions

class CqObject(base.ConfigObjectBase):
    def __init__(self, pd, cq_id, spec):
        super().__init__()
        self.Clone(Store.templates.Get('QSTATE_RDMA_CQ'))
        self.pd = pd
        self.remote = pd.remote
        self.id = cq_id
        self.GID("CQ%04d" % self.id)
        self.spec = spec

        self.hostmem_pg_size = spec.hostmem_pg_size
        self.num_cq_wqes = self.__roundup_to_pow_2(spec.num_wqes)
        self.cqwqe_size = self.__get_cqwqe_size()
        self.cq_size = self.num_cq_wqes * self.cqwqe_size

        if not self.remote:
            self.cq = pd.ep.intf.lif.GetQ('RDMA_CQ', self.id)
    
            if (self.cq is None):
                assert(0)

            # create cq slab
            self.cq_slab = slab.SlabObject(self.pd.ep, self.cq_size)
            self.pd.ep.AddSlab(self.cq_slab)

        self.Show()

        return

    def __roundup_to_pow_2(self, x):
        power = 1
        while power < x : 
            power *= 2
        return power
 
    def __get_cqwqe_size(self):
        return  self.__roundup_to_pow_2(len(RdmaCqDescriptor()))

    def Show(self):
        logger.info('CQ: %s PD: %s Remote: %s' %(self.GID(), self.pd.GID(), self.remote))
        logger.info('CQ num_wqes: %d wqe_size: %d, EQ-ID: %d' 
                       %(self.num_cq_wqes, self.cqwqe_size, self.pd.id)) 

    def PrepareHALRequestSpec(self, req_spec):
        logger.info("CQ: %s PD: %s Remote: %s LIF: %d" %\
                        (self.GID(), self.pd.GID(), self.remote, 
                         self.pd.ep.intf.lif.hw_lif_id))
        if (GlobalOptions.dryrun): return
        req_spec.cq_num = self.id
        req_spec.eq_id = self.pd.id  # PD id is the EQ number
        req_spec.hw_lif_id = self.pd.ep.intf.lif.hw_lif_id
        req_spec.cq_wqe_size = self.cqwqe_size
        req_spec.num_cq_wqes = self.num_cq_wqes
        req_spec.hostmem_pg_size = self.hostmem_pg_size
        req_spec.cq_va_pages_phy_addr[:] = self.cq_slab.phy_address

    def ProcessHALResponse(self, req_spec, resp_spec):
        self.cq.SetRingParams('CQ', 0, True, False,
                              self.cq_slab.mem_handle,
                              self.cq_slab.address, 
                              self.num_cq_wqes, 
                              self.cqwqe_size)
        #repeat same mem_handle, and other params for these rings,
        #but they are never used.
        #TODO: find a way to create LiteRings, where there is no
        #associated FIFO, but just CI/PI
        self.cq.SetRingParams('CQ_ARM', 1, True, False,
                              self.cq_slab.mem_handle,
                              self.cq_slab.address, 
                              0,
                              0)
        self.cq.SetRingParams('CQ_SARM', 2, True, False,
                              self.cq_slab.mem_handle,
                              self.cq_slab.address, 
                              0,
                              0)
        logger.info("CQ: %s PD: %s Remote: %s LIF: %d cqcb_addr: 0x%x cq_base_addr: 0x%x" %\
                        (self.GID(), self.pd.GID(), self.remote, 
                         self.pd.ep.intf.lif.hw_lif_id, 
                         self.cq.GetQstateAddr(), self.cq_slab.address))
        #logger.ShowScapyObject(self.cq.qstate.data)
         
class CqObjectHelper:
    def __init__(self):
        self.cqs = []

    def Generate(self, pd, spec):
        self.pd = pd
        if self.pd.remote:
            logger.info("skipping CQ generation for remote PD: %s" %(pd.GID()))
            return

        count = spec.count
        logger.info("Creating %d Cqs. for PD:%s" %\
                       (count, pd.GID()))
        for i in range(count):
            cq_id = i if pd.remote else pd.ep.intf.lif.GetCqid()
            cq = CqObject(pd, cq_id, spec)
            self.cqs.append(cq)

    def Configure(self):
        if self.pd.remote:
            logger.info("skipping CQ configuration for remote PD: %s" %(self.pd.GID()))
            return
        logger.info("Configuring %d CQs." % len(self.cqs)) 
        halapi.ConfigureCqs(self.cqs)
