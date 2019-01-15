# /usr/bin/python3
import pdb

import infra.common.defs        as defs
import infra.common.objects     as objects
import infra.config.base        as base
import iris.config.resmgr            as resmgr

from iris.config.store                      import Store
from infra.common.logging              import logger
from iris.config.objects.swdr               import SwDscrRingHelper
from iris.config.objects.crypto_keys        import CryptoKeyHelper

import iris.config.hal.defs          as haldefs
import iris.config.hal.api           as halapi

class TlsCbObject(base.ConfigObjectBase):
    def __init__(self):
        super().__init__()
        self.Clone(Store.templates.Get('TLSCB'))
        return
        
    # def Init(self, spec_obj):
    def Init(self ,tcpcb):
        self.id = tcpcb.id
        gid = "TlsCb%04d" % self.id
        self.GID(gid)
        logger.info("  - %s" % self)
        self.tcpcb = tcpcb 
        self.serq = SwDscrRingHelper.main("SERQ", gid, self.id)
        self.bsq = SwDscrRingHelper.main("BSQ", gid, self.id)
        self.crypto_key = CryptoKeyHelper.main() 
        self.crypto_hmac_key = CryptoKeyHelper.main() 
        self.debug_dol = 0x1 # bypass barco
        self.is_decrypt_flow = False
        self.other_fid = tcpcb.other_qid
        return

    def PrepareHALRequestSpec(self, req_spec):
        req_spec.key_or_handle.tlscb_id    = self.id
        if req_spec.__class__.__name__ != 'TlsCbGetRequest':
            req_spec.debug_dol                 = self.debug_dol
            req_spec.salt                      = self.salt
            req_spec.explicit_iv               = self.explicit_iv
            req_spec.command                   = self.command
            req_spec.crypto_key_idx            = self.crypto_key_idx
            req_spec.crypto_hmac_key_idx       = self.crypto_hmac_key_idx
            req_spec.is_decrypt_flow           = self.is_decrypt_flow
            req_spec.other_fid                 = self.other_fid
            req_spec.l7_proxy_type             = self.l7_proxy_type
            req_spec.serq_pi                   = self.serq_pi
            req_spec.serq_ci                   = self.serq_ci
        return

    def ProcessHALResponse(self, req_spec, resp_spec):
        logger.info("  - TlsCb %s = %s" %\
                       (self.id, \
                        haldefs.common.ApiStatus.Name(resp_spec.api_status)))
        if resp_spec.__class__.__name__ != 'TlsCbResponse':
            self.serq_pi = resp_spec.spec.serq_pi
            self.serq_ci = resp_spec.spec.serq_ci
            self.bsq_pi = resp_spec.spec.bsq_pi
            self.bsq_ci = resp_spec.spec.bsq_ci
            self.tnmdpr_alloc = resp_spec.spec.tnmdpr_alloc
            self.enc_requests = resp_spec.spec.enc_requests
            self.dec_requests = resp_spec.spec.dec_requests
            self.mac_requests = resp_spec.spec.mac_requests
            self.rnmdpr_free = resp_spec.spec.rnmdpr_free
            self.enc_completions = resp_spec.spec.enc_completions
            self.dec_completions = resp_spec.spec.dec_completions
            self.mac_completions = resp_spec.spec.mac_completions
            self.enc_failures = resp_spec.spec.enc_failures
            self.dec_failures = resp_spec.spec.dec_failures
            self.mac_failures = resp_spec.spec.mac_failures
            self.pre_debug_stage0_7_thread = resp_spec.spec.pre_debug_stage0_7_thread
            self.post_debug_stage0_7_thread = resp_spec.spec.post_debug_stage0_7_thread
            self.other_fid = resp_spec.spec.other_fid
            self.l7_proxy_type = resp_spec.spec.l7_proxy_type
        return

    def IsFilterMatch(self, spec):
        return super().IsFilterMatch(spec.filters)

    def GetObjValPd(self):
        lst = []
        lst.append(self)
        halapi.GetTlsCbs(lst)
        return
    def SetObjValPd(self):
        lst = []
        lst.append(self)
        halapi.UpdateTlsCbs(lst)
        return

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
        logger.info("Configuring %d TlsCbs." % len(objlist)) 
        halapi.ConfigureTlsCbs(objlist)
        return
        
    def __gen_one(self, tcpcb):
        logger.info("Creating TlsCb")
        tlscb_obj = TlsCbObject()
        tlscb_obj.Init(tcpcb)
        Store.objects.Add(tlscb_obj)
        return tlscb_obj

    def Generate(self, tcpcb):
        obj = self.__gen_one(tcpcb)
        self.objlist.append(obj)
        lst = []
        lst.append(obj)
        self.Configure(lst)
        return self.objlist

    def main(self, tcpcb):
        objlist = self.Generate(tcpcb)
        return objlist

TlsCbHelper = TlsCbObjectHelper()
