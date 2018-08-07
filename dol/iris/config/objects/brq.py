
import pdb

import infra.common.defs        as defs
import infra.common.objects     as objects
import infra.config.base        as base

import iris.config.resmgr            as resmgr

import iris.config.hal.defs          as haldefs
import iris.config.hal.api           as halapi

import types_pb2            as types_pb2

from iris.config.store               import Store
from infra.common.logging       import logger

import wring_pb2            as wring_pb2
#import wring_pb2_grpc           as wring_pb2_grpc

class BRQEntryObject(base.ConfigObjectBase):
    def __init__(self, brq_ring_name, brq_ring_type, brq_entry_defn, entry_idx):
        super().__init__()
        self.Clone(Store.templates.Get(brq_entry_defn))
        self.ring_type = brq_ring_type
        self.entry_idx = entry_idx
        self.entry_defn = brq_entry_defn
        self.GID("%s_ENTRY%04d" % (brq_ring_name, entry_idx))
        return
    def Sync(self):
        ring_entries = []
        ring_entries.append(self)
        halapi.GetBarcoRingEntries(ring_entries)
        return
    def Configure(self):
        self.Sync()
        return
    def PrepareHALRequestSpec(self, reqspec):
        reqspec.ring_type = self.ring_type
        reqspec.slot_index = self.entry_idx
        #logger.info("PrepareHALRequestSpec Entry: %s, type: %d, index: %d" % (self.ID() , reqspec.ring_type , reqspec.slot_index ))
        return
    def ProcessHALResponse(self, req_spec, resp_spec):
        logger.info("Entry : %s : T: %d I:%d" % (self.ID(), resp_spec.ring_type, resp_spec.slot_index))
        if (resp_spec.ring_type != req_spec.ring_type):
            assert(0)
        if (resp_spec.slot_index != req_spec.slot_index):
            assert(0)
        logger.info("Field set %s" % resp_spec.WhichOneof("ReqDescrMsg"))

        if (resp_spec.HasField("symm_req_descr")):
            self.ilist_addr = resp_spec.symm_req_descr.ilist_addr
            self.olist_addr = resp_spec.symm_req_descr.olist_addr
            self.command = resp_spec.symm_req_descr.command
            self.key_desc_index = resp_spec.symm_req_descr.key_desc_index
            self.second_key_desc_index = resp_spec.symm_req_descr.second_key_desc_index
            self.iv_addr = resp_spec.symm_req_descr.iv_addr
            self.status_addr = resp_spec.symm_req_descr.status_addr
            self.doorbell_addr = resp_spec.symm_req_descr.doorbell_addr
            self.doorbell_data = resp_spec.symm_req_descr.doorbell_data
            self.salt = resp_spec.symm_req_descr.salt
            self.explicit_iv = resp_spec.symm_req_descr.explicit_iv
            self.barco_status = resp_spec.symm_req_descr.barco_status
            self.header_size = resp_spec.symm_req_descr.header_size
            logger.info("Entry(%s): ila: %x. ola: %x, cmd: %x, kdi: %x, skdi: %x, ia: %x, sa: %x, da: %x, dd: %x" %
                            (self.ID(), self.ilist_addr, self.olist_addr, self.command, self.key_desc_index, self.second_key_desc_index,
                            self.iv_addr, self.status_addr, self.doorbell_addr, self.doorbell_data))
        else:
            assert(0)
        return
    def Show(self):
        logger.info("Entry: %s" % self.ID())
        return



class BRQObject(base.ConfigObjectBase):
    def __init__(self, brq_instance):
        super().__init__()
        self.Clone(Store.templates.Get(brq_instance.defn))
        self.GID("%s" % brq_instance.name)
        self.ring_entries = []
        self.count = brq_instance.count
        self.type = brq_instance.type
        self.defn = brq_instance.defn
        for ring_entry_idx in range(brq_instance.count):
            ring_entry = BRQEntryObject(self.ID(), brq_instance.type, brq_instance.entry_defn, ring_entry_idx)
            self.ring_entries.append(ring_entry)
        Store.objects.SetAll(self.ring_entries)
        return
    def Sync(self):
        rings = []
        rings.append(self)
        halapi.GetBarcoRingMeta(rings)
        halapi.GetBarcoRingEntries(self.ring_entries)
        return
    def Configure(self):
        self.Sync()
        return
    def GetMeta(self):
        # Get ring meta state
        lst = []
        lst.append(self) 
        halapi.GetBarcoRingMeta(lst)
        return

    def GetRingEntries(self, indices):
        # Get generic ring entries for given slot indices
        ringentries = []
        for i in indices:
            ringentries.append(self.ring_entries[i])
        halapi.GetBarcoRingEntries(ringentries)
        return
    def Show(self):
        logger.info("Ring: %s" % (self.ID()))
        for ring_entry in self.ring_entries:
            ring_entry.Show()
        return
    def PrepareHALGetRequestSpec(self, reqspec):
        reqspec.ring_type = self.type
        return
    def ProcessHALGetResponse(self, req_spec, resp_spec):
        logger.info("Entry : %s : T: %d" % (self.ID(), resp_spec.ring_type))
        self.pi = resp_spec.pi
        self.ci = resp_spec.ci
        logger.info("Entry : %s : pi %s ci %s" % (self.ID(), self.pi, self.ci))
        return


class BRQObjectHelper:
    def __init__(self):
        self.brq_list = []
        return

    def Sync(self):
        for brq in self.brq_list:
            brq.Sync()
        return

    def Generate(self):
        spec = Store.specs.Get("BRQ_DEF")
        for brq_instance in spec.entries:
            brq = BRQObject(brq_instance.entry)
            self.brq_list.append(brq)
        Store.objects.SetAll(self.brq_list)
        return

    def Show(self):
        for brq in self.brq_list:
            brq.Show()
        return

    def main(self):
        self.Generate()
        self.Sync()
        self.Show()


BRQHelper = BRQObjectHelper()
