# /usr/bin/python3
import pdb

import infra.common.defs        as defs
import infra.common.objects     as objects
import infra.config.base        as base
import config.resmgr            as resmgr

from config.store               import Store
from infra.common.logging       import cfglogger
from config.objects.swdr        import SwDscrRingHelper

import config.hal.defs          as haldefs
import config.hal.api           as halapi

class TlsCbObject(base.ConfigObjectBase):
    def __init__(self):
        super().__init__()
        self.Clone(Store.templates.Get('TLSCB'))
        return
        
    # def Init(self, spec_obj):
    def Init(self ,tcpcb):
        self.id = resmgr.TlsCbIdAllocator.get()
        gid = "TlsCb%04d" % self.id
        self.GID(gid)
        cfglogger.info("  - %s" % self)
        self.tcpcb = tcpcb 
        self.serq = SwDscrRingHelper.main("SERQ", gid, self.id)
        self.bsq = SwDscrRingHelper.main("BSQ", gid, self.id)
        return

    def PrepareHALRequestSpec(self, reqspec):
        reqspec.key_or_handle.tlscb_id    = self.id
        return

    def ProcessHALResponse(self, req_spec, resp_spec):
        cfglogger.info("  - TlsCb %s = %s" %\
                       (self.id, \
                        haldefs.common.ApiStatus.Name(resp_spec.api_status)))
        return

    def IsFilterMatch(self, spec):
        return super().IsFilterMatch(spec.filters)

    def Read(self):
        return

    def Write(self):
        return



# Helper Class to Generate/Configure/Manage TlsCb Objects.
class TlsCbObjectHelper:
    def __init__(self):
        self.objlist = []
        return

    def Configure(self, objlist = None):
        if objlist == None:
            objlist = Store.objects.GetAllByClass(TlsCbObject)
        cfglogger.info("Configuring %d TlsCbs." % len(objlist)) 
        halapi.ConfigureTlsCbs(objlist)
        return
        
    def __gen_one(self, tcpcb):
        cfglogger.info("Creating TlsCb")
        tlscb_obj = TlsCbObject()
        tlscb_obj.Init(tcpcb)
        Store.objects.Add(tlscb_obj)
        return tlscb_obj

    def Generate(self, tcpcb):
        self.objlist.append(self.__gen_one(tcpcb))
        return self.objlist

    def main(self, tcpcb):
        objlist = self.Generate(tcpcb)
        self.Configure(self.objlist)
        return objlist

TlsCbHelper = TlsCbObjectHelper()
