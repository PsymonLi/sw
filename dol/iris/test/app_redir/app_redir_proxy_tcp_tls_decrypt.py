# Testcase definition file.

import time
import socket
import pdb
import copy
import iris.test.tcp_tls_proxy.tcp_proxy as tcp_proxy

import types_pb2                as types_pb2
import crypto_keys_pb2          as crypto_keys_pb2

from iris.config.objects.proxycb_service       import ProxyCbServiceHelper
from iris.config.objects.tcp_proxy_cb          import TcpCbHelper
from iris.config.objects.proxy_redir_cb        import ProxyrCbHelper
from iris.config.objects.proxy_chain_cb        import ProxycCbHelper
import iris.test.callbacks.networking.modcbs as modcbs
import iris.test.tcp_tls_proxy.tcp_tls_proxy as tcp_tls_proxy
from infra.common.objects import ObjectDatabase as ObjectDatabase
from infra.common.logging import logger
import iris.test.app_redir.app_redir_shared as app_redir_shared

rnmdpr_big = 0
proxyrcbid = ""
proxyccbid = ""
proxyrcb = 0
proxyccb = 0
redir_span = False

def Setup(infra, module):
    print("Setup(): Sample Implementation")
    modcbs.Setup(infra, module)
    return

def Teardown(infra, module):
    print("Teardown(): Sample Implementation.")
    modcbs.Teardown(infra, module)
    return

def TestCaseSetup(tc):
    global rnmdpr_big
    global proxyrcbid
    global proxyccbid
    global proxyrcb
    global proxyccb
    global redir_span
    tc.SetRetryEnabled(True)
    redir_span = getattr(tc.module.args, 'redir_span', False)

    tc.pvtdata = ObjectDatabase()
    tcp_proxy.SetupProxyArgs(tc)
    id = ProxyCbServiceHelper.GetFlowInfo(tc.config.flow._FlowObject__session)
    TcpCbHelper.main(id)
    tcbid = "TcpCb%04d" % id
    # 1. Configure TCB in HBM before packet injection
    tcb = tc.infra_data.ConfigStore.objects.db[tcbid]
    tcp_proxy.init_tcb_inorder(tc, tcb)
    # set tcb state to ESTABLISHED(1)
    tcb.state = 1
    tcb.l7_proxy_type = 0
    tcb.debug_dol = 0
    if redir_span:
        tcb.debug_dol_tx |= tcp_proxy.tcp_tx_debug_dol_dont_tx

    tcb.SetObjValPd()

    _proxyrcb_id = id
    ProxyrCbHelper.main(_proxyrcb_id)
    proxyrcbid = "ProxyrCb%04d" % _proxyrcb_id
    # 1. Configure PROXYRCB in HBM before packet injection
    proxyrcb = tc.infra_data.ConfigStore.objects.db[proxyrcbid]
    # let HAL fill in defaults for chain_rxq_base, etc.
    proxyrcb.my_txq_base = 0
    proxyrcb.chain_rxq_base = 0
    proxyrcb.redir_span = redir_span
    proxyrcb.proxyrcb_flags = app_redir_shared.app_redir_dol_pipeline_loopbk_en

    # fill in flow key
    proxyrcb.FlowKeyBuild(tc.config.flow)
    print("vrf %d flow sport %d dport %d" % 
          (proxyrcb.vrf, proxyrcb.sport, proxyrcb.dport))
    proxyrcb.SetObjValPd()

    tlscbid = "TlsCb%04d" % id
    tlscb = copy.deepcopy(tc.infra_data.ConfigStore.objects.db[tlscbid])

    tlscb.debug_dol = tcp_tls_proxy.tls_debug_dol_bypass_proxy
    tlscb.other_fid = 0xffff
    tlscb.l7_proxy_type = tcp_proxy.l7_proxy_type_REDIR
    if redir_span:
        tlscb.l7_proxy_type = tcp_proxy.l7_proxy_type_SPAN
    tlscb.serq_pi = 0
    tlscb.serq_ci = 0

    if tc.module.args.key_size == 16:
        tcp_tls_proxy.tls_aes128_decrypt_setup(tc, tlscb)
    elif tc.module.args.key_size == 32:
        tcp_tls_proxy.tls_aes256_decrypt_setup(tc, tlscb)

    _proxyccb_id = id
    ProxycCbHelper.main(_proxyccb_id)
    proxyccbid = "ProxycCb%04d" % _proxyccb_id
    # 1. Configure PROXYCCB in HBM before packet injection
    proxyccb = tc.infra_data.ConfigStore.objects.db[proxyccbid]
    # let HAL fill in defaults for my_txq_base, etc.
    proxyccb.redir_span = redir_span
    proxyccb.my_txq_base = 0
    proxyccb.chain_txq_base = 0
    proxyccb.chain_txq_lif = app_redir_shared.service_lif_tcp_proxy
    proxyccb.chain_txq_qtype = 0
    proxyccb.chain_txq_qid = id
    proxyccb.chain_txq_ring = 0
    proxyccb.proxyccb_flags = app_redir_shared.app_redir_chain_desc_add_aol_offset | \
                              app_redir_shared.app_redir_dol_skip_chain_doorbell
    proxyccb.SetObjValPd()

    # 2. Clone objects that are needed for verification
    rnmdpr_big = copy.deepcopy(tc.infra_data.ConfigStore.objects.db["RNMDPR_BIG"])
    rnmdpr_big.GetMeta()

    proxyrcb = copy.deepcopy(tc.infra_data.ConfigStore.objects.db[proxyrcbid])
    proxyrcb.GetObjValPd()
    proxyccb = copy.deepcopy(tc.infra_data.ConfigStore.objects.db[proxyccbid])
    proxyccb.GetObjValPd()
    
    return

def TestCaseVerify(tc):
    global rnmdpr_big
    global proxyrcbid
    global proxyccbid
    global proxyrcb
    global proxyccb
    global redir_span

    num_pkts = 1
    if hasattr(tc.module.args, 'num_pkts'):
        num_pkts = int(tc.module.args.num_pkts)

    proxyr_meta_pages = 0
    if hasattr(tc.module.args, 'proxyr_meta_pages'):
        proxyr_meta_pages = int(tc.module.args.proxyr_meta_pages)

    num_flow_miss_pkts = 0
    if hasattr(tc.module.args, 'num_flow_miss_pkts'):
        num_flow_miss_pkts = int(tc.module.args.num_flow_miss_pkts)

    id = ProxyCbServiceHelper.GetFlowInfo(tc.config.flow._FlowObject__session)

    # Verify chain_rxq_base
    proxyrcb_cur = tc.infra_data.ConfigStore.objects.db[proxyrcbid]
    proxyrcb_cur.GetObjValPd()
    if proxyrcb_cur.chain_rxq_base == 0:
        print("chain_rxq_base not set")
        return False

    print("chain_rxq_base value post-sync from HBM 0x%x" % proxyrcb_cur.chain_rxq_base)

    # Verify my_txq_base
    proxyccb_cur = tc.infra_data.ConfigStore.objects.db[proxyccbid]
    proxyccb_cur.GetObjValPd()
    if proxyccb_cur.my_txq_base == 0:
        print("my_txq_base not set")
        return False

    print("my_txq_base value post-sync from HBM 0x%x" % proxyccb_cur.my_txq_base)

    # Fetch current values from Platform
    rnmdpr_big_cur = tc.infra_data.ConfigStore.objects.db["RNMDPR_BIG"]
    rnmdpr_big_cur.GetMeta()

    # Verify PI for RNMDPR_BIG got incremented
    # when span is in effect, TLS would have allocated 2 descs per packet,
    # one for forwarding to TCP and one for L7
    num_exp_descs = num_pkts
    if redir_span:
        num_exp_descs *= 2

    if (rnmdpr_big_cur.pi != rnmdpr_big.pi+num_exp_descs):
        print("RNMDPR_BIG pi check failed old %d new %d expected %d" %
                     (rnmdpr_big.pi, rnmdpr_big_cur.pi, rnmdpr_big.pi+num_exp_descs))
        proxyrcb_cur.StatsPrint()
        proxyccb_cur.StatsPrint()
        return False
    print("RNMDPR_BIG pi old %d new %d" % (rnmdpr_big.pi, rnmdpr_big_cur.pi))

    # Rx: verify # packets redirected
    num_redir_pkts = num_pkts - num_flow_miss_pkts
    if (proxyrcb_cur.stat_pkts_redir != proxyrcb.stat_pkts_redir+num_redir_pkts):
        print("stat_pkts_redir check failed old %d new %d expected %d" %
              (proxyrcb.stat_pkts_redir, proxyrcb_cur.stat_pkts_redir, proxyrcb.stat_pkts_redir+num_redir_pkts))
        proxyrcb_cur.StatsPrint()
        proxyccb_cur.StatsPrint()
        return False
    print("stat_pkts_redir old %d new %d" % 
          (proxyrcb.stat_pkts_redir, proxyrcb_cur.stat_pkts_redir))

    # Tx: verify PI for PROXYCCB got incremented
    time.sleep(1)
    proxyccb_cur.GetObjValPd()

    num_exp_proxyccb_pkts = num_redir_pkts
    if redir_span:
        num_exp_proxyccb_pkts = 0

    if (proxyccb_cur.pi != proxyccb.pi+num_exp_proxyccb_pkts):
        print("PROXYCCB pi check failed old %d new %d expected %d" %
                      (proxyccb.pi, proxyccb_cur.pi, proxyccb.pi+num_exp_proxyccb_pkts))
        proxyrcb_cur.StatsPrint()
        proxyccb_cur.StatsPrint()
        return False
    print("PROXYCCB pi old %d new %d" % (proxyccb.pi, proxyccb_cur.pi))

    # Tx: verify # packets chained
    if (proxyccb_cur.stat_pkts_chain != proxyccb.stat_pkts_chain+num_exp_proxyccb_pkts):
        print("stat_pkts_chain check failed old %d new %d expected %d" %
              (proxyccb.stat_pkts_chain, proxyccb_cur.stat_pkts_chain, proxyccb.stat_pkts_chain+num_exp_proxyccb_pkts))
        proxyrcb_cur.StatsPrint()
        proxyccb_cur.StatsPrint()
        return False
    print("stat_pkts_chain old %d new %d" % 
          (proxyccb.stat_pkts_chain, proxyccb_cur.stat_pkts_chain))

    proxyrcb_cur.StatsPrint()
    proxyccb_cur.StatsPrint()
    return True

def TestCaseTeardown(tc):
    print("TestCaseTeardown(): Sample Implementation.")
    modcbs.TestCaseTeardown(tc)
    return

def TestCaseStepSetup(tc, step):
    return modcbs.TestCaseStepSetup(tc, step)

def TestCaseStepTrigger(tc, step):
    return modcbs.TestCaseStepTrigger(tc, step)

def TestCaseStepVerify(tc, step):
    return modcbs.TestCaseStepVerify(tc, step)

def TestCaseStepTeardown(tc, step):
    return modcbs.TestCaseStepTeardown(tc, step)

