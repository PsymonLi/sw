#! /usr/bin/python3

from scapy.all import *

import iris.config.resmgr as resmgr

import infra.factory.base as base
from infra.common.logging   import logger

import model_sim.src.model_wrap as model_wrap

from factory.objects.nvme import buffer as nvmebuffer

from infra.factory.store    import FactoryStore

#64B
class NvmeSqDescriptor(Packet):
    fields_desc = [
        #dword 0
        ByteField("opc", 0),
        BitField("fuse", 0, 2),
        BitField("rsvd0", 0, 4),
        BitField("psdt", 0, 2),
        ShortField("cid", 0),
    
        #dword 1
        IntField("nsid", 0),
    
        #dword 2-3
        IntField("rsvd2", 0),
        IntField("rsvd3", 0),

        #dword 4-5
        XLongField("mptr", 0),

        #dword 6-9
        XLongField("prp1", 0),
        XLongField("prp2", 0),

        #dword 10-15
        IntField("cdw10", 0),
        IntField("cdw11", 0),
        IntField("cdw12", 0),
        IntField("cdw13", 0),
        IntField("cdw14", 0),
        IntField("cdw15", 0),
    ]

#40B
class NvmeSqDescriptorBase(Packet):
    fields_desc = [
        #dword 0
        ByteField("opc", 0),
        BitField("fuse", 0, 2),
        BitField("rsvd0", 0, 4),
        BitField("psdt", 0, 2),
        ShortField("cid", 0),
    
        #dword 1
        IntField("nsid", 0),
    
        #dword 2-3
        IntField("rsvd2", 0),
        IntField("rsvd3", 0),

        #dword 4-5
        XLongField("mptr", 0),

        #dword 6-9
        XLongField("prp1", 0),
        XLongField("prp2", 0),
    ]

#24B
class NvmeSqDescriptorWrite(Packet):
    fields_desc = [
        #dword 10-11
        LongField("slba", 0),

        #dword 12
        BitField("nlb", 0, 16),
        BitField("rsvd12", 0, 10),
        BitField("prinfo", 0, 4),
        BitField("fua", 0, 1),
        BitField("lr", 0, 1),

        #dword 13
        BitField("dsm", 0, 8),
        BitField("rsvd13", 0, 24),

        #dword 14
        BitField("ilbrt", 0, 32),

        #dword 15
        BitField("lbat", 0, 16),
        BitField("lbatm", 0, 16),
    ]

#16B
class NvmeCqDescriptor(Packet):
    fields_desc = [
        #dword 0
        IntField("cdw0", 0),

        #dword 1
        IntField("rsvd1", 0), 

        #dword 2
        ShortField("sqhd", 0),
        ShortField("sqid", 0),

        #dword 3
        ShortField("cid", 0),
        BitField("p", 0, 1),
        BitField("sc", 0, 8),
        BitField("sct", 0, 3),
        BitField("rsvd2", 0, 2),
        BitField("m", 0, 1),
        BitField("dnr", 0, 1),
    ]

#TBD, for now reserve 16B
class NvmeArmqDescriptor(Packet):
    fields_desc = [
        BitField("rsvd1", 0, 128),
    ]

#2B
class NvmeSessqDescriptor(Packet):
    fields_desc = [
        ShortField("cid", 0),
    ]

#8B
class NvmeTcprqDescriptor(Packet):
    fields_desc = [
        ShortField("pad", 0),
        BitField("len", 0, 14),
        BitField("addr", 0, 34),
    ]

NVME_OPC_FLUSH = 0
NVME_OPC_WRITE = 1
NVME_OPC_READ  = 2
NVME_OPC_WRITE_UNC = 3
NVME_OPC_WRITE_ZEROES = 4


class NvmeSqDescriptorObject(base.FactoryObjectBase):
    def __init__(self):
        super().__init__()

    def Init(self, spec):
        super().Init(spec)

    def Write(self):
        """
        Creates a Descriptor at "self.address"
        :return:
        """

        opc = self.spec.fields.opc
        if opc in (NVME_OPC_FLUSH, NVME_OPC_WRITE, NVME_OPC_READ, NVME_OPC_WRITE_UNC, NVME_OPC_WRITE_ZEROES):
            #requires data to be specified
            if hasattr(self.spec.fields, 'data'):
                data_buffer = self.spec.fields.data
            else:
                logger.error("Error!! nvme buffer needs to be specified for the descriptor")
                return

            prp1 = data_buffer.prp1
            prp2 = data_buffer.prp2
            nlb  = data_buffer.nlb

        desc = NvmeSqDescriptorBase(opc=self.spec.fields.opc,
                                    cid=self.spec.fields.cid,
                                    nsid=self.spec.fields.nsid,
                                    prp1=prp1, 
                                    prp2=prp2)

        logger.ShowScapyObject(desc)

        self.desc = desc

        if hasattr(self.spec.fields, 'write'):
            slba = self.spec.fields.write.slba if hasattr(self.spec.fields.write, 'slba') else 0
            write = NvmeSqDescriptorWrite(slba=slba,
                                          nlb=nlb)

            self.desc = self.desc / write

            logger.ShowScapyObject(write)

        logger.info("desc_size = %d" %(len(self.desc)))

        resmgr.HostMemoryAllocator.write(self.mem_handle, bytes(desc))

    def Read(self):
        """
        Reads a Descriptor from "self.address"
        :return:
        """
        self.phy_address = resmgr.HostMemoryAllocator.v2p(self.address)
        mem_handle = resmgr.MemHandle(self.address, self.phy_address)
        desc = NvmeSqDescriptor(resmgr.HostMemoryAllocator.read(mem_handle, 64))

        logger.ShowScapyObject(desc)

    def Show(self):
        pass

    def __eq__(self, other):
        logger.info("__eq__ operator invoked on descriptor")
        return  self == other

    def GetBuffer(self):
        logger.info("GetBuffer() operator invoked on descriptor, ignoring for now..")

        nvmebuff = nvmebuffer.NvmeBufferObject()
        nvmebuff.data = None
        nvmebuff.size = 0
        return nvmebuff

class NvmeCqDescriptorObject(base.FactoryObjectBase):
    def __init__(self):
        super().__init__()

    def Init(self, spec):
        super().Init(spec)

    def Write(self):
        """
        Creates a Descriptor at "self.address"
        :return:
        """
        desc = NvmeCqDescriptor(sqhd=self.spec.fields.sqhd,
                                sqid=self.spec.fields.sqid,
                                p=self.spec.fields.p,
                                sc=self.spec.fields.sc,
                                sct=self.spec.fields.sct)
                                
        self.desc = desc

        logger.ShowScapyObject(desc)

        logger.info("desc_size = %d" %(len(desc)))

        resmgr.HostMemoryAllocator.write(self.mem_handle, bytes(desc))

    def Read(self):
        """
        Reads a Descriptor from "self.address"
        :return:
        """
        self.phy_address = resmgr.HostMemoryAllocator.v2p(self.address)
        mem_handle = resmgr.MemHandle(self.address, self.phy_address)
        desc = NvmeCqDescriptor(resmgr.HostMemoryAllocator.read(mem_handle, 64))

        logger.ShowScapyObject(desc)

    def Show(self):
        pass

    def __eq__(self, other):
        logger.info("__eq__ operator invoked on descriptor")
        return  self == other

    def GetBuffer(self):
        logger.info("GetBuffer() operator invoked on descriptor, ignoring for now..")

        nvmebuff = nvmebuffer.NvmeBufferObject()
        nvmebuff.data = None
        nvmebuff.size = 0
        return nvmebuff

class NvmeArmqDescriptorObject(base.FactoryObjectBase):
    def __init__(self):
        super().__init__()

    def Init(self, spec):
        super().Init(spec)

    def Write(self):
        """
        Creates a Descriptor at "self.address"
        :return:
        """
        desc = NvmeArmqDescriptor()
                                
        self.desc = desc

        logger.ShowScapyObject(desc)

        logger.info("desc_size = %d" %(len(desc)))

        if self.mem_handle:
            resmgr.HostMemoryAllocator.write(self.mem_handle, bytes(desc))
        else:
            model_wrap.write_mem_pcie(self.address, bytes(desc), len(desc))

        #TBD: Do we need it ?
        self.Read()

    def Read(self):
        """
        Reads a Descriptor from "self.address"
        :return:
        """
        if self.mem_handle:
            self.phy_address = resmgr.HostMemoryAllocator.v2p(self.address)
            mem_handle = resmgr.MemHandle(self.address, self.phy_address)
            desc = NvmeArmqDescriptor(resmgr.HostMemoryAllocator.read(mem_handle, 64))
        else:
            hbm_addr = self.address
            desc = NvmeArmqDescriptor(model_wrap.read_mem(hbm_addr, 64))

        logger.ShowScapyObject(desc)

    def Show(self):
        pass

    def __eq__(self, other):
        logger.info("__eq__ operator invoked on descriptor")
        return  self == other

    def GetBuffer(self):
        logger.info("GetBuffer() operator invoked on descriptor, ignoring for now..")

        nvmebuff = nvmebuffer.NvmeBufferObject()
        nvmebuff.data = None
        nvmebuff.size = 0
        return nvmebuff

class NvmeSessqDescriptorObject(base.FactoryObjectBase):
    def __init__(self):
        super().__init__()

    def Init(self, spec):
        super().Init(spec)

    def Write(self):
        """
        Creates a Descriptor at "self.address"
        :return:
        """
        desc = NvmeSessqDescriptor()
                                
        self.desc = desc

        logger.ShowScapyObject(desc)

        logger.info("desc_size = %d" %(len(desc)))

        if self.mem_handle:
            resmgr.HostMemoryAllocator.write(self.mem_handle, bytes(desc))
        else:
            model_wrap.write_mem_pcie(self.address, bytes(desc), len(desc))

        #TBD: Do we need it ?
        self.Read()

    def Read(self):
        """
        Reads a Descriptor from "self.address"
        :return:
        """
        if self.mem_handle:
            self.phy_address = resmgr.HostMemoryAllocator.v2p(self.address)
            mem_handle = resmgr.MemHandle(self.address, self.phy_address)
            desc = NvmeSessqDescriptor(resmgr.HostMemoryAllocator.read(mem_handle, 64))
        else:
            hbm_addr = self.address
            desc = NvmeSessqDescriptor(model_wrap.read_mem(hbm_addr, 64))

        logger.ShowScapyObject(desc)

    def Show(self):
        pass

    def __eq__(self, other):
        logger.info("__eq__ operator invoked on descriptor")
        return  self == other

    def GetBuffer(self):
        logger.info("GetBuffer() operator invoked on descriptor, ignoring for now..")

        nvmebuff = nvmebuffer.NvmeBufferObject()
        nvmebuff.data = None
        nvmebuff.size = 0
        return nvmebuff

class NvmeTcprqDescriptorObject(base.FactoryObjectBase):
    def __init__(self):
        super().__init__()

    def Init(self, spec):
        super().Init(spec)

    def Write(self):
        """
        Creates a Descriptor at "self.address"
        :return:
        """
        desc = NvmeTcprqDescriptor()
                                
        self.desc = desc

        logger.ShowScapyObject(desc)

        logger.info("desc_size = %d" %(len(desc)))

        if self.mem_handle:
            resmgr.HostMemoryAllocator.write(self.mem_handle, bytes(desc))
        else:
            model_wrap.write_mem_pcie(self.address, bytes(desc), len(desc))

        #TBD: Do we need it ?
        self.Read()

    def Read(self):
        """
        Reads a Descriptor from "self.address"
        :return:
        """
        if self.mem_handle:
            self.phy_address = resmgr.HostMemoryAllocator.v2p(self.address)
            mem_handle = resmgr.MemHandle(self.address, self.phy_address)
            desc = NvmeTcprqDescriptor(resmgr.HostMemoryAllocator.read(mem_handle, 64))
        else:
            hbm_addr = self.address
            desc = NvmeTcprqDescriptor(model_wrap.read_mem(hbm_addr, 64))

        logger.ShowScapyObject(desc)

    def Show(self):
        pass

    def __eq__(self, other):
        logger.info("__eq__ operator invoked on descriptor")
        return  self == other

    def GetBuffer(self):
        logger.info("GetBuffer() operator invoked on descriptor, ignoring for now..")

        nvmebuff = nvmebuffer.NvmeBufferObject()
        nvmebuff.data = None
        nvmebuff.size = 0
        return nvmebuff

