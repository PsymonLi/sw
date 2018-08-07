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
from infra.common.glopts import GlobalOptions

class SlabObject(base.ConfigObjectBase):
    def __init__(self, ep, size):
        super().__init__()
        self.ep = ep
        self.id = self.ep.GetSlabid()
        self.GID("SLAB%04d" % self.id)
        self.size = size
        self.page_size = 4096
        self.address = None
        self.mem_handle = None
        return

    def Configure(self):
        if (GlobalOptions.dryrun): return
        self.mem_handle = resmgr.HostMemoryAllocator.get(self.size)
        assert(self.mem_handle != None)
        self.address = self.mem_handle.va
        assert(self.address % self.page_size == 0)

        self.phy_address = []
        for i in range(0, self.size, self.page_size):
            phy_addr = resmgr.HostMemoryAllocator.v2p(self.address + i)
            self.phy_address.append(phy_addr)
            
        self.Show()

    def Show(self):
        if (GlobalOptions.dryrun): return
        logger.info('SLAB: %s EP: %s' %(self.GID(), self.ep.GID()))
        logger.info('size: %d address: 0x%x' %(self.size, self.address))
        i = 0
        for phy_addr in self.phy_address:
            i += 1
        

class SlabObjectHelper:
    def __init__(self):
        self.slabs = []

    def Generate(self, ep, spec):
        count = spec.count
        logger.info("Creating %d Slabs. for EP:%s" %\
                       (count, ep.GID()))
        for slab_id in range(count):
            slab = SlabObject(ep, spec.size)
            self.slabs.append(slab)

    def AddSlab(self, slab):
        self.slabs.append(slab)

    def Configure(self):
        logger.info("Configuring %d Slabs." % len(self.slabs)) 
        for slab in self.slabs:
            slab.Configure()
