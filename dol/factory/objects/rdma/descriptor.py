#! /usr/bin/python3

from scapy.all import *

import config.resmgr as resmgr

import infra.factory.base as base
from infra.common.logging   import logger

import model_sim.src.model_wrap as model_wrap

from factory.objects.rdma import buffer as rdmabuffer

from infra.factory.store    import FactoryStore

class RdmaSqDescriptorBase(Packet):
    fields_desc = [
        LongField("wrid", 0),
        BitField("op_type", 0, 4),
        BitField("complete_notify", 0, 1),
        BitField("fence", 0, 1),
        BitField("solicited_event", 0, 1),
        BitField("inline_data_vld", 0, 1),
        ByteField("num_sges", 0),
        BitField("color", 0, 1),
        BitField("rsvd1", 0, 15),
    ]

class RdmaSqDescriptorSend(Packet):
    fields_desc = [
        IntField("imm_data", 0),
        IntField("inv_key", 0),
        IntField("rsvd1", 0),      # rsvd1 is to ensure len field in Sq Send and Write defined at same offset
        IntField("len", 0),
        IntField("rsvd2", 0),
    ]

class RdmaSqDescriptorUDSend(Packet):
    fields_desc = [
        IntField("imm_data", 0),
        IntField("q_key", 0),
        IntField("length", 0),
        X3BytesField("dst_qp", 0),
        ByteField("ah_size", 0),
        IntField("ah_handle", 0),
    ]

class RdmaSqDescriptorWrite(Packet):
    fields_desc = [
        IntField("imm_data", 0),
        LongField("va", 0),
        IntField("len", 0),    # ensure that len is defined at same offset b/w Sq Send and Write descriptors
        IntField("r_key", 0),
    ]

class RdmaSqDescriptorRead(Packet):
    fields_desc = [
        IntField("rsvd", 0),
        LongField("va", 0),
        IntField("len", 0),
        IntField("r_key", 0),
    ]

class RdmaSqDescriptorAtomic(Packet):
    fields_desc = [
        IntField("r_key", 0),
        LongField("va", 0),
        LongField("swapdt", 0),
        LongField("cmpdt", 0),
        LongField("pad", 0),
    ]

class RdmaRrqDescriptorBase(Packet):
    fields_desc = [
        BitField("read_resp_or_atomic", 0, 1),
        BitField("num_sges", 0, 7),
        X3BytesField("psn", 0),
        X3BytesField("msn", 0),
        ByteField("rsvd", 0),
    ]

class RdmaRrqDescriptorRead(Packet):
    fields_desc = [
        IntField("len", 0),
        LongField("wqe_sge_list_ptr", 0),
        IntField("rsvd0", 0),
        IntField("rsvd1", 0),
        IntField("rsvd2", 0),
        IntField("rsvd3", 0),
        IntField("rsvd4", 0),
        IntField("rsvd5", 0),
        IntField("rsvd6", 0),
        IntField("rsvd7", 0),
        IntField("rsvd8", 0),
        IntField("rsvd9", 0),
        IntField("rsvd10", 0),
    ]

class RdmaRrqDescriptorAtomic(Packet):
    fields_desc = [
        LongField("va", 0),
        IntField("len", 0),
        IntField("l_key", 0),
        ByteField("op_type", 0),
        ByteField("rsvd0", 0),
        ByteField("rsvd1", 0),
        ByteField("rsvd2", 0),
        IntField("rsvd3", 0),
        IntField("rsvd4", 0),
        IntField("rsvd5", 0),
        IntField("rsvd6", 0),
        IntField("rsvd7", 0),
        IntField("rsvd8", 0),
        IntField("rsvd9", 0),
        IntField("rsvd10", 0),
        IntField("rsvd11", 0),
        IntField("rsvd12", 0),
    ]

class RdmaRqDescriptorBase(Packet):
    fields_desc = [
        LongField("wrid", 0),
        ByteField("num_sges", 0),
        ByteField("rsvd0", 0),
        ShortField("rsvd1", 0),
        IntField("rsvd2", 0),
        LongField("rsvd3", 0),
        LongField("rsvd4", 0),
    ]

class RdmaRsqDescriptorBase(Packet):
    fields_desc = [
        BitField("read_resp_or_atomic", 0, 1),
        BitField("rsvd0", 0, 7),
        X3BytesField("psn", 0),
        LongField("rsvd1", 0),
    ]

class RdmaRsqDescriptorRead(Packet):
    fields_desc = [
        IntField("r_key", 0),
        LongField("va", 0),
        IntField("len", 0),
        IntField("offset", 0),
    ]

class RdmaRsqDescriptorAtomic(Packet):
    fields_desc = [
        IntField("r_key", 0),
        LongField("va", 0),
        LongField("orig_data", 0),
    ]

class RdmaSge(Packet):
    fields_desc = [
        LongField("va", 0),
        IntField("len", 0),
        IntField("l_key", 0),
    ]

class RdmaCqDescriptor(Packet):
    fields_desc = [
        LongField("wrid", 0),
        ByteField("op_type", 0),
        ByteField("status", 0),
        ByteField("rsvd2", 0),
        X3BytesField("qp", 0),
        X3BytesField("src_qp", 0),
        MACField("smac", ETHER_ANY),
        BitField("rkey_inv_vld", 0, 1),
        BitField("imm_data_vld", 0, 1),
        BitField("color", 0, 1),
        BitField("ipv4", 0, 1),
        BitField("rsvd1", 0, 4),
        IntField("imm_data", 0),
        IntField("r_key", 0),
    ]

class RdmaEqDescriptor(Packet):
    fields_desc = [
        X3BytesField("qid", 0),
        BitField("type", 0, 3),
        BitField("code", 0, 4),
        BitField("color", 0, 1),
    ]

class RdmaSqDescriptorObject(base.FactoryObjectBase):
    def __init__(self):
        super().__init__()
        self.sges = []

    def Init(self, spec):
        super().Init(spec)
        if hasattr(self.spec.fields, 'wrid'):
            self.wrid = self.spec.fields.wrid

    def Write(self):
        """
        Creates a Descriptor at "self.address"
        :return:
        """
        inline_data_vld = self.spec.fields.inline_data_vld if hasattr(self.spec.fields, 'inline_data_vld') else 0
        num_sges = self.spec.fields.num_sges if hasattr(self.spec.fields, 'num_sges') else 0
        color = self.spec.fields.color if hasattr(self.spec.fields, 'color') else 0
        fence = self.spec.fields.fence if hasattr(self.spec.fields, 'fence') else 0
        logger.info("Writing SQ Descriptor @0x%x = op_type: %d wrid: 0x%x inline_data_vld: %d num_sges: %d color: %d fence: %d" % 
                       (self.address, self.spec.fields.op_type, self.wrid, inline_data_vld, num_sges, color, fence))
        desc = RdmaSqDescriptorBase(op_type=self.spec.fields.op_type, wrid=self.wrid,
                                    inline_data_vld = inline_data_vld, num_sges=num_sges, color=color, fence=fence)
        inline_data_len = 0
        inline_data = None

        # Make sure Inline data is specificied only for Send and Write, assert in other operations
        if hasattr(self.spec.fields, 'send'):
           logger.info("Reading Send")
           imm_data = self.spec.fields.send.imm_data if hasattr(self.spec.fields.send, 'imm_data') else 0
           inv_key = self.spec.fields.send.inv_key if hasattr(self.spec.fields.send, 'inv_key') else 0
           data_len = self.spec.fields.send.len if hasattr(self.spec.fields.send, 'len') else 0
           if inline_data_vld:
               inline_data_len = data_len
               # Create the Inline data of size provided
               inline_data = bytearray(inline_data_len)
               inline_data[0:inline_data_len] = self.spec.fields.send.inline_data[0:inline_data_len]
           send = RdmaSqDescriptorSend(imm_data=imm_data, inv_key=inv_key, len=data_len)
           desc = desc/send

        if hasattr(self.spec.fields, 'ud_send'):
           logger.info("Reading UD Send")
           dst_qp = self.spec.fields.ud_send.dst_qp if hasattr(self.spec.fields.ud_send, 'dst_qp') else 0
           q_key = self.spec.fields.ud_send.q_key if hasattr(self.spec.fields.ud_send, 'q_key') else 0
           ah_handle = self.spec.fields.ud_send.ah_handle if hasattr(self.spec.fields.ud_send, 'ah_handle') else 0
           #right shift ah_handle by 3 bits to keep it within 32 bits
           ah_handle >>= 3
           ah_size = self.spec.fields.ud_send.ah_size if hasattr(self.spec.fields.ud_send, 'ah_size') else 0
           imm_data = self.spec.fields.ud_send.imm_data if hasattr(self.spec.fields.ud_send, 'imm_data') else 0
           send = RdmaSqDescriptorUDSend(imm_data=imm_data, q_key=q_key, dst_qp=dst_qp, ah_size=ah_size, ah_handle=ah_handle)
           desc = desc/send

        if hasattr(self.spec.fields, 'write'):
           logger.info("Reading Write")
           imm_data = self.spec.fields.write.imm_data if hasattr(self.spec.fields.write, 'imm_data') else 0
           va = self.spec.fields.write.va if hasattr(self.spec.fields.write, 'va') else 0
           dma_len = self.spec.fields.write.len if hasattr(self.spec.fields.write, 'len') else 0
           if inline_data_vld:
               inline_data_len = dma_len
               # Create the Inline data of size provided
               inline_data = bytearray(inline_data_len)
               inline_data[0:inline_data_len] = self.spec.fields.write.inline_data[0:inline_data_len]
           r_key = self.spec.fields.write.r_key if hasattr(self.spec.fields.write, 'r_key') else 0
           write = RdmaSqDescriptorWrite(imm_data=imm_data, va=va, len=dma_len, r_key=r_key)
           desc = desc/write

        if hasattr(self.spec.fields, 'read'):
           logger.info("Reading Read")
           assert(inline_data_vld == 0)
           rsvd = self.spec.fields.read.rsvd if hasattr(self.spec.fields.read, 'rsvd') else 0
           va = self.spec.fields.read.va if hasattr(self.spec.fields.read, 'va') else 0
           data_len = self.spec.fields.read.len if hasattr(self.spec.fields.read, 'len') else 0
           r_key = self.spec.fields.read.r_key if hasattr(self.spec.fields.read, 'r_key') else 0
           read = RdmaSqDescriptorRead(rsvd=rsvd, va=va, len=data_len, r_key=r_key)
           desc = desc/read

        if hasattr(self.spec.fields, 'atomic'):
           logger.info("Reading Atomic")
           assert(inline_data_vld == 0)
           r_key = self.spec.fields.atomic.r_key if hasattr(self.spec.fields.atomic, 'r_key') else 0
           va = self.spec.fields.atomic.va if hasattr(self.spec.fields.atomic, 'va') else 0
           cmpdt = self.spec.fields.atomic.cmpdt if hasattr(self.spec.fields.atomic, 'cmpdt') else 0
           swapdt = self.spec.fields.atomic.swapdt if hasattr(self.spec.fields.atomic, 'swapdt') else 0
           atomic = RdmaSqDescriptorAtomic(r_key=r_key, va=va, cmpdt=cmpdt, swapdt=swapdt)
           desc = desc/atomic

        if inline_data_vld:
           logger.info("Inline Data: %s " % bytes(inline_data[0:inline_data_len]))
           desc = desc/bytes(inline_data)
        elif (num_sges and (num_sges > 0)):
            for sge in self.spec.fields.sges:
                sge_entry = RdmaSge(va=sge.va, len=sge.len, l_key=sge.l_key)
                logger.info("Read Sge[] = va: 0x%x len: %d l_key: %d" % 
                               (sge.va, sge.len, sge.l_key))
                desc = desc/sge_entry
        
        logger.ShowScapyObject(desc)
        if self.mem_handle:
            resmgr.HostMemoryAllocator.write(self.mem_handle, bytes(desc))
        else:
            model_wrap.write_mem_pcie(self.address, bytes(desc), len(desc))

        self.Read()

    def Read(self):
        """
        Reads a Descriptor from "self.address"
        :return:
        """
        if self.mem_handle:
            self.phy_address = resmgr.HostMemoryAllocator.v2p(self.address)
            mem_handle = resmgr.MemHandle(self.address, self.phy_address)
            desc = RdmaSqDescriptorBase(resmgr.HostMemoryAllocator.read(mem_handle, 32))
        else:
            hbm_addr = self.address
            desc = RdmaSqDescriptorBase(model_wrap.read_mem(hbm_addr, 32))

        logger.ShowScapyObject(desc)
        self.wrid = desc.wrid
        self.num_sges = desc.num_sges
        self.op_type = desc.op_type
        self.fence   = desc.fence
        logger.info("Read Desciptor @0x%x = wrid: 0x%x num_sges: %d op_type: %d fence: %d" % 
                       (self.address, self.wrid, self.num_sges, self.op_type, self.fence))
        self.sges = []
        if self.mem_handle:
            mem_handle.va += 32
            #for atomic descriptor, skip 16 bytes to access SGE
            if self.op_type in [6, 7]:
                mem_handle.va += 16
        else:
            hbm_addr += 32
            if self.op_type in [6, 7]:
                hbm_addr += 16

        for i in range(self.num_sges):
            
            if self.mem_handle:
                self.sges.append(RdmaSge(resmgr.HostMemoryAllocator.read(mem_handle, 16)))
            else:
                self.sges.append(RdmaSge(model_wrap.read_mem(hbm_addr, 16)))

            #sge = RdmaSge(resmgr.HostMemoryAllocator.read(mem_handle, 16))
            #self.sges.append(sge)
            #logger.info("Read Sge[%d] = va: 0x%x len: %d l_key: %d" % 
            #               (i, sge.va, sge.len, sge.l_key))
            if self.mem_handle:
                mem_handle.va += 16
            else:
                hbm_addr += 16

        for sge in self.sges:
            logger.info('%s' % type(sge))
            logger.info('0x%x' % sge.va)
        logger.info('id: %s' %id(self))


    def Show(self):
        desc = RdmaSqDescriptor(wrid=self.wrid, op_type=self.op_type, num_sges=self.num_sges)
        logger.ShowScapyObject(desc)
        #TODO: Check if we need to show SGEs

    def __eq__(self, other):
        logger.info("__eq__ operator invoked on descriptor, ignoring for now..")
        return True

    def GetBuffer(self):
        logger.info("GetBuffer() operator invoked on descriptor")

        #if hasattr(self.spec.fields, 'buff'):
        if not hasattr(self, 'address'):
            logger.info("Reading from buff")
            return self.spec.fields.buff

        rdmabuff = rdmabuffer.RdmaBufferObject()
        logger.info("wrid: %d num_sges: %d len: %d" % (self.wrid, self.num_sges, len(self.sges)));
        total_data = bytearray()
        total_size = 0 
        for idx in range(self.num_sges):
            sge = self.sges[idx]
            logger.info("Reading sge content : 0x%x  len: %d" %(sge.va, sge.len))
            mem_handle = resmgr.MemHandle(sge.va,
                                          resmgr.HostMemoryAllocator.v2p(sge.va))
            sge_data = resmgr.HostMemoryAllocator.read(mem_handle, sge.len)
            logger.info("     sge data: %s" % bytes(sge_data))
            total_data.extend(sge_data)
            total_size += sge.len
        rdmabuff.data = bytes(total_data)
        rdmabuff.size = total_size
        logger.info("Total data: %s" % bytes(total_data))
        return rdmabuff

class RdmaRqDescriptorObject(base.FactoryObjectBase):
    def __init__(self):
        super().__init__()
        self.sges = []

    def Init(self, spec):
        super().Init(spec)
        if hasattr(self.spec.fields, 'wrid'):
            self.wrid = self.spec.fields.wrid

    def Write(self):
        """
        Creates a Descriptor at "self.address"
        :return:
        """
        if self.mem_handle:
            logger.info("Writing RQ Descriptor @(va:0x%x, pa:0x%x) = wrid: 0x%x num_sges: %d" % 
                       (self.mem_handle.va, self.mem_handle.pa, self.wrid, self.spec.fields.num_sges))
        else:
            logger.info("Writing RQ Descriptor @(address:0x%x) = wrid: 0x%x num_sges: %d" % 
                       (self.address, self.wrid, self.spec.fields.num_sges))

        desc = RdmaRqDescriptorBase(wrid=self.wrid,
                                    num_sges=self.spec.fields.num_sges)
        for sge in self.spec.fields.sges:
            logger.info("sge: va: 0x%x len: %d l_key: %d" %(sge.va, sge.len, sge.l_key))
            sge_entry = RdmaSge(va=sge.va, len=sge.len, l_key=sge.l_key)
            desc = desc/sge_entry
        
        logger.ShowScapyObject(desc)
        if self.mem_handle:
            resmgr.HostMemoryAllocator.write(self.mem_handle, bytes(desc))
        else:
            model_wrap.write_mem_pcie(self.address, bytes(desc), len(desc))

        self.Read()

    def Read(self):
        """
        Reads a Descriptor from "self.address"
        :return:
        """

        if self.mem_handle:
            self.phy_address = resmgr.HostMemoryAllocator.v2p(self.address)
            mem_handle = resmgr.MemHandle(self.address, self.phy_address)
            desc = RdmaRqDescriptorBase(resmgr.HostMemoryAllocator.read(mem_handle, 32))
        else:
            hbm_addr = self.address
            desc = RdmaRqDescriptorBase(model_wrap.read_mem(hbm_addr, 32))

        logger.ShowScapyObject(desc)
        self.wrid = desc.wrid
        self.num_sges = desc.num_sges
        logger.info("Read Desciptor @0x%x = wrid: 0x%x num_sges: %d" % 
                       (self.address, self.wrid, self.num_sges))

        self.sges = []
        if self.mem_handle:
            mem_handle.va += 32
        else:
            hbm_addr += 32

        for i in range(self.num_sges):
            
            if self.mem_handle:
                self.sges.append(RdmaSge(resmgr.HostMemoryAllocator.read(mem_handle, 16)))
            else:
                self.sges.append(RdmaSge(model_wrap.read_mem(hbm_addr, 16)))

            if self.mem_handle:
                mem_handle.va += 16
            else:
                hbm_addr += 16

        for sge in self.sges:
            logger.info('%s' % type(sge))
            logger.info('va: 0x%x' % sge.va)
            logger.info('len: 0x%x' % sge.len)
            logger.info('lkey: 0x%x' % sge.l_key)
        logger.info('id: %s' %id(self))

    def Show(self):
        desc = RdmaRqDescriptor(addr=self.wrid, len=self.num_sges)
        logger.ShowScapyObject(desc)
        #TODO: Check if we need to show SGEs

    def __eq__(self, other):
        logger.info("__eq__ operator invoked on descriptor, ignoring for now..")
        return True

    def GetBuffer(self):
        logger.info("GetBuffer() operator invoked on descriptor")

        #if hasattr(self.spec.fields, 'buff'):
        if not hasattr(self, 'address'):
            return self.spec.fields.buff

        rdmabuff = rdmabuffer.RdmaBufferObject()
        logger.info("wrid: %d num_sges: %d len: %d" % (self.wrid, self.num_sges, len(self.sges)));
        total_data = bytearray()
        total_size = 0 
        for idx in range(self.num_sges):
            sge = self.sges[idx]
            logger.info("Reading sge content : 0x%x  len: %d" %(sge.va, sge.len))
            mem_handle = resmgr.MemHandle(sge.va,
                                          resmgr.HostMemoryAllocator.v2p(sge.va))
            sge_data = resmgr.HostMemoryAllocator.read(mem_handle, sge.len)
            logger.info("     sge data: %s" % bytes(sge_data))
            total_data.extend(sge_data)
            total_size += sge.len
        rdmabuff.data = bytes(total_data)
        rdmabuff.size = total_size
        logger.info("Total data: %s" % bytes(total_data))
        return rdmabuff

class RdmaCqDescriptorObject(base.FactoryObjectBase):
    def __init__(self):
        super().__init__()
        self.Clone(FactoryStore.templates.Get('DESC_RDMA_CQ'))

    def Init(self, spec):
        super().Init(spec)
        if hasattr(spec.fields, 'wrid'):
            self.wrid = spec.fields.wrid
        if hasattr(spec.fields, 'msn'):
            self.msn = spec.fields.msn
        if hasattr(spec.fields, 'op_type'):
            self.op_type = spec.fields.op_type
        if hasattr(spec.fields, 'status'):
            self.status = spec.fields.status
        if hasattr(spec.fields, 'rkey_inv_vld'):
            self.rkey_inv_vld = spec.fields.rkey_inv_vld
        if hasattr(spec.fields, 'imm_data_vld'):
            self.imm_data_vld = spec.fields.imm_data_vld
        if hasattr(spec.fields, 'color'):
            self.color = spec.fields.color
        if hasattr(spec.fields, 'qp'):
            self.qp = spec.fields.qp
        if hasattr(spec.fields, 'imm_data'):
            self.imm_data = spec.fields.imm_data
        if hasattr(spec.fields, 'r_key'):
            self.r_key = spec.fields.r_key
        if hasattr(spec.fields, 'ipv4'):
            self.ipv4 = spec.fields.ipv4
        if hasattr(spec.fields, 'src_qp'):
            self.src_qp = spec.fields.src_qp
        if hasattr(spec.fields, 'smac'):
            self.smac = bytes(
                spec.fields.smac)
            

        self.__create_desc()

    def __create_desc(self):
        self.desc = RdmaCqDescriptor(
            wrid=self.wrid,
            op_type=self.op_type,
            status=self.status,
            rkey_inv_vld=self.rkey_inv_vld,
            imm_data_vld=self.imm_data_vld,
            color=self.color,
            qp=self.qp,
            imm_data=self.imm_data,
            r_key=self.r_key,
            ipv4=self.ipv4,
            src_qp=self.src_qp,
            smac=self.smac)
        
    def __set_desc(self, desc):
        self.desc = desc
    
    def Write(self):
        """
        Creates a Descriptor at "self.address"
        :return:
        """
        logger.info("Writing CQ Desciptor @0x%x = wrid: 0x%x " % 
                       (self.address, self.wrid))
        resmgr.HostMemoryAllocator.write(self.mem_handle, 
                                         bytes(self.desc))

    def Read(self):
        """
        Reads a Descriptor from "self.address"
        :return:
        """
        logger.info("Reading CQ Desciptor @0x%x = wrid: 0x%x " % 
                       (self.address, self.wrid))
        self.phy_address = resmgr.HostMemoryAllocator.v2p(self.address)
        mem_handle = resmgr.MemHandle(self.address, self.phy_address)
        self.__set_desc(RdmaCqDescriptor(resmgr.HostMemoryAllocator.read(mem_handle, len(RdmaCqDescriptor()))))

    def Show(self):
        logger.ShowScapyObject(self.desc)

    def __eq__(self, other):
        logger.info("__eq__ operator invoked on cq descriptor..")

        logger.info('self(expected):')
        self.Show()
        logger.info('other(actual):')
        other.Show()

        if self.desc.status == 0: #CQ_STATUS_SUCCESS
            return self.desc == other.desc

        logger.info('status is not 0\n')

        if self.desc.wrid != other.desc.wrid:
            return False

        logger.info('wrid matched\n')

        if self.desc.op_type != other.desc.op_type:
            return False

        logger.info('op_type matched\n')

        if self.desc.status != other.desc.status:
            return False

        logger.info('status matched\n')

        return True

        #no need to compare other params as they are meaningful only incase of SUCCESS


    def GetBuffer(self):
        logger.info("GetBuffer() operator invoked on cq descriptor")
        # CQ is not associated with any buffer and hence simply create
        # default RDMABuffer object so that ebuf == abuf check passes
        return rdmabuffer.RdmaBufferObject()

class RdmaEqDescriptorObject(base.FactoryObjectBase):
    def __init__(self):
        super().__init__()
        self.Clone(FactoryStore.templates.Get('DESC_RDMA_EQ'))

    def Init(self, spec):
        super().Init(spec)
        if hasattr(spec.fields, 'qid'):
            self.qid = spec.fields.qid
        if hasattr(spec.fields, 'type'):
            self.type = spec.fields.type
        if hasattr(spec.fields, 'code'):
            self.code = spec.fields.code
        if hasattr(spec.fields, 'color'):
            self.color = spec.fields.color
        self.__create_desc()

    def __create_desc(self):
        self.desc = RdmaEqDescriptor(
            qid=self.qid,
            type=self.type,
            code=self.code,
            color=self.color)
        
    def __set_desc(self, desc):
        self.desc = desc
    
    def Write(self):
        """
        Creates a Descriptor at "self.address"
        :return:
        """
        logger.info("Writing EQ Desciptor @0x%x = qid: %d " % 
                       (self.address, self.qid))
        resmgr.HostMemoryAllocator.write(self.mem_handle, 
                                         bytes(self.desc))

    def Read(self):
        """
        Reads a Descriptor from "self.address"
        :return:
        """
        logger.info("Reading EQ Desciptor @ 0x%x " % (self.address))
        self.phy_address = resmgr.HostMemoryAllocator.v2p(self.address)
        mem_handle = resmgr.MemHandle(self.address, self.phy_address)
        self.__set_desc(RdmaEqDescriptor(resmgr.HostMemoryAllocator.read(mem_handle, len(RdmaEqDescriptor()))))

    def Show(self):
        logger.ShowScapyObject(self.desc)

    def __eq__(self, other):
        logger.info("__eq__ operator invoked on Eq descriptor..")

        logger.info('\nself(expected):')
        self.Show()
        logger.info('\nother(actual):')
        other.Show()

        if self.desc.qid != other.desc.qid:
            return False

        logger.info('qid matched\n')

        if self.desc.color != other.desc.color:
            return False

        logger.info('color matched\n')

        if self.desc.type != other.desc.type:
            return False

        logger.info('type matched\n')

        if self.desc.code != other.desc.code:
            return False

        logger.info('code matched\n')

        logger.info('EQ descriptor matched\n')
        return True

        #no need to compare other params as they are meaningful only incase of SUCCESS


    def GetBuffer(self):
        logger.info("GetBuffer() operator invoked on EQ descriptor")
        # EQ is not associated with any buffer and hence simply create
        # default RDMABuffer object so that ebuf == abuf check passes
        return rdmabuffer.RdmaBufferObject()

