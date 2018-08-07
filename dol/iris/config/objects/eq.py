#! /usr/bin/python3
import pdb

import infra.common.defs        as defs
import infra.common.objects     as objects
import infra.config.base        as base

import iris.config.resmgr            as resmgr
from iris.config.store               import Store
from infra.common.logging       import logger

import iris.config.hal.api           as halapi
import iris.config.hal.defs          as haldefs
import rdma_pb2                 as rdma_pb2

import iris.config.objects.slab      as slab
import iris.config.objects.mr        as mr

from factory.objects.rdma.descriptor import RdmaSqDescriptorBase
from factory.objects.rdma.descriptor import RdmaSqDescriptorSend
from factory.objects.rdma.descriptor import RdmaRqDescriptorBase
from factory.objects.rdma.descriptor import RdmaRrqDescriptorBase
from factory.objects.rdma.descriptor import RdmaRrqDescriptorRead
from factory.objects.rdma.descriptor import RdmaRrqDescriptorAtomic
from factory.objects.rdma.descriptor import RdmaRsqDescriptorBase
from factory.objects.rdma.descriptor import RdmaRsqDescriptorRead
from factory.objects.rdma.descriptor import RdmaRsqDescriptorAtomic
from factory.objects.rdma.descriptor import RdmaEqDescriptor
from factory.objects.rdma.descriptor import RdmaSge
from infra.common.glopts import GlobalOptions

class EqObject(base.ConfigObjectBase):
    def __init__(self, pd, eq_id, spec):
        super().__init__()
        self.Clone(Store.templates.Get('QSTATE_RDMA_EQ'))
        self.pd = pd
        self.remote = pd.remote
        self.id = eq_id
        self.int_num = resmgr.InterruptAllocator.get()
        self.GID("EQ%04d" % self.id)
        self.spec = spec

        self.num_eq_wqes = self.__roundup_to_pow_2(spec.num_wqes)
        self.eqwqe_size = self.__get_eqwqe_size()
        self.eq_size = self.num_eq_wqes * self.eqwqe_size

        self.intr_tbl_addr = 0
        self.eqe_base_addr = None
        self.eqe_base_addr_phy = None

        if not self.remote:
            self.eq = pd.ep.intf.lif.GetQ('RDMA_EQ', self.id)
    
            if (self.eq is None):
                assert(0)

            # create eq slab
            self.eq_slab = slab.SlabObject(self.pd.ep, self.eq_size)
            self.pd.ep.AddSlab(self.eq_slab)

            # create eq mr
            self.eq_mr = mr.MrObject(self.pd, self.eq_slab)
            self.pd.AddMr(self.eq_mr)

        self.Show()

        return

    def __roundup_to_pow_2(self, x):
        power = 1
        while power < x : 
            power *= 2
        return power
 
    def __get_eqwqe_size(self):
        return  self.__roundup_to_pow_2(len(RdmaEqDescriptor()))

    def Show(self):
        logger.info('EQ: %s PD: %s Remote: %s' %(self.GID(), self.pd.GID(), self.remote))
        logger.info('EQ num_wqes: %d wqe_size: %d' %(self.num_eq_wqes, self.eqwqe_size)) 
        if not self.eqe_base_addr is None:
            logger.info('EQ Intr Base Addr- VA: 0x%x PHY: 0x%x' %(self.eqe_base_addr, self.eqe_base_addr_phy)) 

    def PrepareHALRequestSpec(self, req_spec):
        logger.info("EQ: %s PD: %s Remote: %s LKey: %d LIF: %d" %\
                        (self.GID(), self.pd.GID(), self.remote, 
                         self.eq_mr.lkey, self.pd.ep.intf.lif.hw_lif_id))
        if (GlobalOptions.dryrun): return
        req_spec.eq_id = self.id
        req_spec.int_num = self.int_num
        req_spec.hw_lif_id = self.pd.ep.intf.lif.hw_lif_id
        req_spec.eq_wqe_size = self.eqwqe_size
        req_spec.num_eq_wqes = self.num_eq_wqes
        self.eqe_base_addr = self.eq_slab.address
        logger.info(self.eqe_base_addr, self.eq_slab.address)
        assert(self.eqe_base_addr)
        self.eqe_base_addr_phy = resmgr.HostMemoryAllocator.v2p(self.eqe_base_addr)
        assert(self.eqe_base_addr_phy)

        req_spec.eqe_base_addr_phy = self.eqe_base_addr_phy

    def ProcessHALResponse(self, req_spec, resp_spec):
        self.eq.SetRingParams('EQ', 0, True, False,
                              self.eq_slab.mem_handle,
                              self.eq_slab.address, 
                              self.num_eq_wqes, 
                              self.eqwqe_size)
        logger.info("EQ: %s PD: %s Remote: %s LIF: %d eqcb_addr: 0x%x eqe_base_addr: 0x%x" \
                       " eqe_base_addr_phy: 0x%x" %\
                        (self.GID(), self.pd.GID(), self.remote, 
                         self.pd.ep.intf.lif.hw_lif_id, 
                         self.eq.GetQstateAddr(), self.eqe_base_addr, self.eqe_base_addr_phy));
        self.intr_tbl_addr = resp_spec.eq_intr_tbl_addr
        logger.info("EQ: intr_tbl_addr from HAL: 0x%x" % self.intr_tbl_addr)
        logger.info("HAL Response successful for EQ creation");
        logger.ShowScapyObject(self.eq.qstate.data)
         
class EqObjectHelper:
    def __init__(self):
        self.eqs = []

    def Generate(self, pd, spec):
        self.pd = pd
        if self.pd.remote:
            logger.info("skipping EQ generation for remote PD: %s" %(pd.GID()))
            return

        count = spec.count
        logger.info("Creating %d EQs. for PD:%s" %\
                       (count, pd.GID()))
        for i in range(count):
            eq_id = i if pd.remote else pd.ep.intf.lif.GetEqid()
            eq = EqObject(pd, eq_id, spec)
            self.eqs.append(eq)

    def Configure(self):
        if self.pd.remote:
            logger.info("skipping EQ configuration for remote PD: %s" %(self.pd.GID()))
            return
        logger.info("Configuring %d EQs." % len(self.eqs)) 
        halapi.ConfigureEqs(self.eqs)
