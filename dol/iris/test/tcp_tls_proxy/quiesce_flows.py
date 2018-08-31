# Testcase definition file.

import pdb
import copy
import iris.test.tcp_tls_proxy.tcp_proxy as tcp_proxy
import iris.test.tcp_tls_proxy.tcp_tls_proxy as tcp_tls_proxy
import iris.config.hal.api           as halapi
import internal_pb2              as internal_pb2
import types_pb2                as types_pb2

from iris.config.store import Store
from iris.config.objects.proxycb_service import ProxyCbServiceHelper
from iris.config.objects.tcp_proxy_cb import TcpCbHelper
import iris.test.callbacks.networking.modcbs as modcbs
from infra.common.objects import ObjectDatabase as ObjectDatabase
from infra.common.logging import logger
from infra.common.glopts import GlobalOptions
from infra.common.logging import logger as logger

def quience_msg_send():
    stub = internal_pb2.InternalStub(halapi.HalChannel)
    print("Invoking QuiesceMsgSnd API");
    req = types_pb2.Empty()
    stub.QuiesceMsgSnd(req)
    return


def Setup(infra, module):
    modcbs.Setup(infra, module)
    return

def Teardown(infra, module):
    return

def TestCaseSetup(tc):
    tc.pvtdata = ObjectDatabase()
    tc.SetRetryEnabled(True)
    tcp_proxy.SetupProxyArgs(tc)

    id1, id2 = ProxyCbServiceHelper.GetSessionQids(tc.config.flow._FlowObject__session)
    if tc.config.flow.IsIflow():
        id = id1
        other_fid = id2
    else:
        id = id2
        other_fid = id1

    TcpCbHelper.main(id)
    tcbid = "TcpCb%04d" % id
    logger.info("Configuring %s" % tcbid)
    # 1. Configure TCB in HBM before packet injection
    tcb = tc.infra_data.ConfigStore.objects.db[tcbid]
    tcp_proxy.init_tcb_inorder(tc, tcb)
    tcb.SetObjValPd()

    TcpCbHelper.main(other_fid)
    tcbid2 = "TcpCb%04d" % (other_fid)
    logger.info("Configuring %s" % tcbid2)
    tcb2 = tc.infra_data.ConfigStore.objects.db[tcbid2]
    tcp_proxy.init_tcb_inorder2(tc, tcb2)
    tcb2.SetObjValPd()

    # 2. Configure TLS CB in HBM before packet injection
    tlscbid = "TlsCb%04d" % id
    tlscbid2 = "TlsCb%04d" % (other_fid)
    tlscb = tc.infra_data.ConfigStore.objects.db[tlscbid]
    tlscb2 = tc.infra_data.ConfigStore.objects.db[tlscbid2]

    tlscb.debug_dol = 0
    tlscb2.debug_dol = 0
    if tc.pvtdata.bypass_barco:
        print("Bypassing Barco")
        tlscb.is_decrypt_flow = False
        tlscb2.is_decrypt_flow = False
        tlscb.debug_dol |= tcp_tls_proxy.tls_debug_dol_bypass_barco
        tlscb2.debug_dol |= tcp_tls_proxy.tls_debug_dol_bypass_barco
    if tc.pvtdata.same_flow:
        print("Same flow")
        tlscb.debug_dol |= tcp_tls_proxy.tls_debug_dol_bypass_proxy
        tlscb2.debug_dol |= tcp_tls_proxy.tls_debug_dol_bypass_proxy
        tlscb.other_fid = 0xffff
        tlscb2.other_fid = 0xffff
    else:
        print("Other flow")
        tlscb.other_fid = other_fid
        tlscb2.other_fid = id

    tlscb.SetObjValPd()
    tlscb2.SetObjValPd()

    # 3. Clone objects that are needed for verification
    tcpcb = copy.deepcopy(tc.infra_data.ConfigStore.objects.db[tcbid])
    tcpcb.GetObjValPd()
    tc.pvtdata.Add(tcpcb)

    tcpcb2 = copy.deepcopy(tc.infra_data.ConfigStore.objects.db[tcbid2])
    tcpcb2.GetObjValPd()
    tc.pvtdata.Add(tcpcb2)

    tlscb = copy.deepcopy(tc.infra_data.ConfigStore.objects.db[tlscbid])
    tlscb.GetObjValPd()
    tc.pvtdata.Add(tlscb)

    other_tlscb = copy.deepcopy(tc.infra_data.ConfigStore.objects.db[tlscbid2])
    other_tlscb.GetObjValPd()
    tc.pvtdata.Add(other_tlscb)

    rnmdpr_big = copy.deepcopy(tc.infra_data.ConfigStore.objects.db["RNMDPR_BIG"])
    rnmdpr_big.GetMeta()
    tc.pvtdata.Add(rnmdpr_big)

        
    return

def TestCaseTrigger(tc):
    if GlobalOptions.dryrun:
        return True
    if tc.pvtdata.test_timer:
        timer = tc.infra_data.ConfigStore.objects.db['FAST_TIMER']
        timer.Step(41)

    return

def TestCaseVerify(tc):

    if GlobalOptions.dryrun:
        return True


    id = ProxyCbServiceHelper.GetFlowInfo(tc.config.flow._FlowObject__session)
    if tc.config.flow.IsIflow():
        print("This is iflow")
        tcbid = "TcpCb%04d" % id
        tlscbid = "TlsCb%04d" % id
        other_tcbid = "TcpCb%04d" % (id + 1)
        other_tlscbid = "TlsCb%04d" % (id + 1)
    else:
        print("This is rflow")
        tcbid = "TcpCb%04d" % (id + 1)
        tlscbid = "TlsCb%04d" % (id + 1)
        other_tcbid = "TcpCb%04d" % id
        other_tlscbid = "TlsCb%04d" % id

    same_flow = False
    if tc.pvtdata.same_flow:
        other_tcbid = tcbid
        other_tlscbid = tlscbid
        same_flow = True

    tcpcb = tc.pvtdata.db[tcbid]
    tcpcb_cur = tc.infra_data.ConfigStore.objects.db[tcbid]
    tcpcb_cur.GetObjValPd()

    other_tcpcb = tc.pvtdata.db[other_tcbid]
    other_tcpcb_cur = tc.infra_data.ConfigStore.objects.db[other_tcbid]
    other_tcpcb_cur.GetObjValPd()

    tlscb = tc.pvtdata.db[tlscbid]
    tlscb_cur = tc.infra_data.ConfigStore.objects.db[tlscbid]
    tlscb_cur.GetObjValPd()

    other_tlscb = tc.pvtdata.db[other_tlscbid]
    other_tlscb_cur = tc.infra_data.ConfigStore.objects.db[other_tlscbid]
    other_tlscb_cur.GetObjValPd()

    rnmdpr_big = tc.pvtdata.db["RNMDPR_BIG"]
    rnmdpr_big_cur = tc.infra_data.ConfigStore.objects.db["RNMDPR_BIG"]
    rnmdpr_big_cur.GetMeta()

    # Print stats
    if same_flow:
        other_tcpcb = tcpcb
        other_tcpcb_cur = tcpcb_cur
    else:
        print("bytes_rcvd = %d:" % tcpcb_cur.bytes_rcvd)
        print("pkts_rcvd = %d:" % tcpcb_cur.pkts_rcvd)
        print("debug_num_phv_to_mem = %d:" % tcpcb_cur.debug_num_phv_to_mem)
        print("debug_num_pkt_to_mem = %d:" % tcpcb_cur.debug_num_pkt_to_mem)

    print("bytes_sent = %d:" % other_tcpcb_cur.bytes_sent)
    print("pkts_sent = %d:" % other_tcpcb_cur.pkts_sent)
    print("debug_num_phv_to_pkt = %d:" % other_tcpcb_cur.debug_num_phv_to_pkt)
    print("debug_num_mem_to_pkt = %d:" % other_tcpcb_cur.debug_num_mem_to_pkt)
    return True

def TestCaseStepVerify(tc, step):
    if halapi.IsHalDisabled():
        return True
    print("step")
    print(step.step_id)
    if step.step_id == 0:
        resp = quience_msg_send()
    return True

def TestCaseTeardown(tc):
    if GlobalOptions.dryrun:
        return True
    if tc.pvtdata.test_timer:
        timer = tc.infra_data.ConfigStore.objects.db['FAST_TIMER']
        timer.Step(0)
    return
