# Testcase definition file.

import pdb
import copy

import types_pb2                as types_pb2
import crypto_keys_pb2          as crypto_keys_pb2
#import crypto_keys_pb2_grpc     as crypto_keys_pb2_grpc

from iris.config.store               import Store
from infra.common.objects import ObjectDatabase as ObjectDatabase
from infra.common.logging import logger
from infra.common.glopts import GlobalOptions
from iris.config.objects.proxycb_service    import ProxyCbServiceHelper
from iris.config.objects.ipsec_proxy_cb      import IpsecCbHelper

rnmdr = 0
ipseccbq = 0
ipseccb = 0
iv = 0
seq = 0

def Setup(infra, module):
    print("Setup(): Sample Implementation")
    elem = module.iterator.Get()
    module.testspec.selectors.flow.Extend(elem.flow)
    return

def Teardown(infra, module):
    print("Teardown(): Sample Implementation.")
    return

def TestCaseSetup(tc):
    global ipseccbq
    global ipseccb
    global rnmdr
    global iv
    global seq

    tc.pvtdata = ObjectDatabase()
    print("TestCaseSetup(): Sample Implementation.")
    # 1. Configure IPSECCB in HBM before packet injection
    id = ProxyCbServiceHelper.GetFlowInfo(tc.config.flow._FlowObject__session)
    IpsecCbHelper.main(id)
    ipsecid = "IPSECCB%04d" % id
    ipseccb = tc.infra_data.ConfigStore.objects.db[ipsecid]

    key_type = types_pb2.CRYPTO_KEY_TYPE_AES128
    key_size = 16
    key = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
    ipseccb.crypto_key.Update(key_type, key_size, key)

    ipseccb.tunnel_sip4               = 0x0A010001
    ipseccb.tunnel_dip4               = 0x0A010002
    ipseccb.iv_size                   = 8
    ipseccb.icv_size                  = 16
    ipseccb.block_size                = 16
    ipseccb.key_index                 = 0
    ipseccb.barco_enc_cmd             = 0x30000000
    ipseccb.iv                        = 0xaaaaaaaaaaaaaaaa
    ipseccb.iv_salt                   = 0xbbbbbbbb
    ipseccb.esn_hi                    = 0
    ipseccb.esn_lo                    = 0
    ipseccb.spi                       = 0
    ipseccb.is_nat_t                  = 1
    ipseccb.is_v6                     = 0
    ipseccb.key_index                 = ipseccb.crypto_key.keyindex
    ipseccb.SetObjValPd()

    seq = ipseccb.esn_lo
    iv = ipseccb.iv

    # 2. Clone objects that are needed for verification
    rnmdr = copy.deepcopy(tc.infra_data.ConfigStore.objects.db["IPSEC_RNMDR"])
    rnmdr.GetMeta()
    rnmdr.GetRingEntries([rnmdr.pi, rnmdr.pi + 1])
    rnmdr.GetRingEntryAOL([rnmdr.pi, rnmdr.pi + 1])
    ipsec_cbq_id = ipsecid + "_IPSECCBQ"
    ipseccbq = copy.deepcopy(tc.infra_data.ConfigStore.objects.db[ipsec_cbq_id])
    ipseccb = tc.infra_data.ConfigStore.objects.db[ipsecid]

    rnmpr = copy.deepcopy(tc.infra_data.ConfigStore.objects.db["IPSEC_RNMPR"])
    rnmpr.GetMeta()
    rnmpr.GetRingEntries([rnmpr.pi])
    tnmdr = copy.deepcopy(tc.infra_data.ConfigStore.objects.db["IPSEC_TNMDR"])
    tnmdr.GetMeta()
    tnmdr.GetRingEntries([tnmdr.pi])
    tnmpr = copy.deepcopy(tc.infra_data.ConfigStore.objects.db["IPSEC_TNMPR"])
    tnmpr.GetMeta()
    tnmpr.GetRingEntries([tnmpr.pi])

    brq = copy.deepcopy(tc.infra_data.ConfigStore.objects.db["BRQ_ENCRYPT_GCM"])
    brq.GetMeta()
    brq.GetRingEntries([brq.pi])

    tc.pvtdata.Add(rnmdr)
    tc.pvtdata.Add(rnmpr)
    tc.pvtdata.Add(tnmdr)
    tc.pvtdata.Add(tnmpr)
    tc.pvtdata.Add(brq)
    return

def TestCaseVerify(tc):
    global ipseccbq
    global ipseccb

    # 1. Verify pi/ci got update got updated
    id = ProxyCbServiceHelper.GetFlowInfo(tc.config.flow._FlowObject__session)
    ipsecid = "IPSECCB%04d" % id
    ipsec_cbq_id = ipsecid + "_IPSECCBQ"
    ipseccb_cur = tc.infra_data.ConfigStore.objects.db[ipsecid]

    print("pre-sync: ipseccb_cur.pi %d ipseccb_cur.ci %d" % (ipseccb_cur.pi, ipseccb_cur.ci))
    ipseccb_cur.GetObjValPd()
    print("post-sync: ipseccb_cur.pi %d ipseccb_cur.ci %d" % (ipseccb_cur.pi, ipseccb_cur.ci))
    if (ipseccb_cur.pi != ipseccb.pi or ipseccb_cur.ci != ipseccb.ci):
        print("pi/ci not as expected")
        return False
    # 3. Fetch current values from Platform
    ipseccb_cur = tc.infra_data.ConfigStore.objects.db[ipsecid]
    ipseccbqq_cur = tc.infra_data.ConfigStore.objects.db[ipsec_cbq_id]
    ipseccbqq_cur.Configure()

    rnmdr = tc.pvtdata.db["IPSEC_RNMDR"]
    rnmpr = tc.pvtdata.db["IPSEC_RNMPR"]
    tnmdr = tc.pvtdata.db["IPSEC_TNMDR"]
    tnmpr = tc.pvtdata.db["IPSEC_TNMPR"]

    rnmdr_cur = tc.infra_data.ConfigStore.objects.db["IPSEC_RNMDR"]
    rnmdr_cur.GetMeta()
    rnmdr_cur.GetRingEntries([rnmdr_cur.pi, rnmdr_cur.pi + 1])
    rnmdr_cur.GetRingEntryAOL([rnmdr_cur.pi, rnmdr_cur.pi + 1])

    rnmpr_cur = tc.infra_data.ConfigStore.objects.db["IPSEC_RNMPR"]
    rnmpr_cur.GetMeta()
    rnmpr_cur.GetRingEntries([rnmpr.pi,rnmpr_cur.pi])

    tnmdr_cur = tc.infra_data.ConfigStore.objects.db["IPSEC_TNMDR"]
    tnmdr_cur.GetMeta()
    tnmdr_cur.GetRingEntries([tnmdr.pi,tnmdr_cur.pi])

    tnmpr_cur = tc.infra_data.ConfigStore.objects.db["IPSEC_TNMPR"]
    tnmpr_cur.GetMeta()
    tnmpr_cur.GetRingEntries([tnmpr.pi,tnmpr_cur.pi])

    # 2. Verify PI for IPSEC_RNMDR got incremented by 1
    if (rnmdr_cur.pi - rnmdr.pi > 2):
        print("IPSEC_RNMDR pi check failed old %d new %d" % (rnmdr.pi, rnmdr_cur.pi))
        return False

    # 4. Verify pi/ci got update got updated for BRQ
    brq = tc.pvtdata.db["BRQ_ENCRYPT_GCM"]
    brq_cur = tc.infra_data.ConfigStore.objects.db["BRQ_ENCRYPT_GCM"]
    print("pre-sync: brq_cur.pi %d brq_cur.ci %d" % (brq_cur.pi, brq_cur.ci))
    brq_cur.GetMeta()
    brq_cur.GetRingEntries([brq.pi, brq_cur.pi])
    print("post-sync: brq_cur.pi %d brq_cur.ci %d" % (brq_cur.pi, brq_cur.ci))
    if (brq_cur.pi != (brq.pi+1)):
        print("brq pi/ci not as expected")
        #needs fix in HAL and support in model/p4+ for this check to work/pass
        return False

    print("BRQ:")
    print("ilist_addr 0x%x" % brq_cur.ring_entries[brq.pi].ilist_addr)
    print("olist_addr 0x%x" % brq_cur.ring_entries[brq.pi].olist_addr)
    print("command 0x%x" % brq_cur.ring_entries[brq.pi].command)
    print("key_desc_index 0x%x" % brq_cur.ring_entries[brq.pi].key_desc_index)
    print("iv_addr 0x%x" % brq_cur.ring_entries[brq.pi].iv_addr)
    print("status_addr 0x%x" % brq_cur.ring_entries[brq.pi].status_addr)
    # There is an offset of 64 to go past scratch when queuing to barco. Pls modify
    # this when this offset is removed.
    #maxflows check should be reverted when we remove the hardcoding for idx 0 with pi/ci for BRQ
    # 5. Verify brq input desc and rnmdr
    print("IPSEC_RNMDR Entry: 0x%x, BRQ ILIST: 0x%x" % (rnmdr.ringentries[rnmdr.pi].handle, brq_cur.ring_entries[brq.pi].ilist_addr))
    #if (rnmdr.ringentries[rnmdr.pi].handle != (brq_cur.ring_entries[brq.pi].ilist_addr - 0x40)):
    #    print("Descriptor handle not as expected in ringentries 0x%x 0x%x" % (rnmdr.ringentries[rnmdr.pi].handle, brq_cur.ring_entries[brq.pi].ilist_addr))
    #    return False

    # 6. Verify descriptor
    #if rnmdr.ringentries[rnmdr.pi].handle != ipseccbqq_cur.ringentries[ipseccb_cur.pi-1].handle:
    #    print("Descriptor handle not as expected in ringentries 0x%x 0x%x" % (rnmdr.ringentries[rnmdr.pi].handle, ipseccbqq_cur.ringentries[ipseccb_cur.pi-1].handle))
    #    return False
    #print("Descriptor handle expected in ringentries 0x%x 0x%x" % (rnmdr.ringentries[rnmdr.pi].handle, ipseccbqq_cur.ringentries[ipseccb_cur.pi-1].handle))

    # 7. Verify descriptor
    #if rnmdr.swdre_list[rnmdr.pi].DescAddr != ipseccbqq_cur.swdre_list[ipseccb_cur.pi-1].DescAddr:
    #    print("Descriptor handle not as expected in swdre_list 0x%x 0x%x" % (rnmdr.swdre_list[rnmdr.pi].DescAddr, ipseccbqq_cur.swdre_list[ipseccb_cur.pi-1].DescAddr))
    #    return False
    #print("Descriptor handle expected in swdre_list 0x%x 0x%x" % (rnmdr.swdre_list[rnmdr.pi].DescAddr, ipseccbqq_cur.swdre_list[ipseccb_cur.pi-1].DescAddr))

    # 10. Verify SeqNo increment
    if (ipseccb_cur.esn_lo != seq+1):
        print ("seq_no 0x%x 0x%x" % (ipseccb_cur.esn_lo, ipseccb.esn_lo))
        return False

    # 11. Verify IV increment
    if (ipseccb_cur.iv != iv+1):
        print ("iv : 0x%x 0x%x" % (ipseccb_cur.iv, ipseccb.iv))
        return False
    #if (rnmdr.ringentries[rnmdr.pi].handle != (brq_cur.ring_entries[brq.pi].status_addr - 56)):
    #    print("Status Addr not as expected in BRQ Req 0x%x 0x%x" % (rnmdr.ringentries[rnmdr.pi].handle, brq_cur.ring_entries[brq.pi].status_addr))
    #    return False

    return True

def TestCaseTeardown(tc):
    print("TestCaseTeardown(): Sample Implementation.")
    return
