#! /usr/bin/python3
import pdb
import test.callbacks.common.pktslicer as pktslicer
import test.telemetry.telemetry as telemetry
from config.store import Store
import config.objects.collector as collector

def GetExpectedPacket(testcase, args):
    case = testcase.pvtdata.span_case
    ssns = telemetry.spanSessionData.getSessions(case)
    if args.sessionid > len(ssns):
        return None
    (id, direc, spantype, pkt, intf, pktlen) = ssns[args.sessionid -1]
    if intf == "AnyLocalEP":
        return None
    if pktlen == 0 or spantype == "ERSPAN":
        return testcase.packets.Get(pkt)
    else:
        pktname = "TRUNCATE_PKT" + str(args.sessionid)
        return testcase.packets.Get(pktname)

def GetExpectedPort(testcase, args):
    case = testcase.pvtdata.span_case
    ssns = telemetry.spanSessionData.getSessions(case)
    if args.sessionid > len(ssns):
        return None
    (id, direc, spantype, pkt, intf, pktlen) = ssns[args.sessionid -1]
    ssn = "session"+str(id)
    erspanid = 1
    if spantype == "ERSPAN":
        if direc == "egress":
            id = id + 3
        tnl = telemetry.data.getErspanSession(id)
        return tnl.ports
 
    if direc == "ingress":
        if 'ports' not in testcase.config.ingress_mirror.__dict__[ssn].intf.__dict__:
            ports = []
            ports.append(testcase.config.ingress_mirror.__dict__[ssn].intf)
            return ports
        else:
            return testcase.config.ingress_mirror.__dict__[ssn].intf.ports
    elif direc == "egress":
        if testcase.config.egress_mirror.__dict__[ssn] is None:
            return None
        if 'ports' not in testcase.config.egress_mirror.__dict__[ssn].intf.__dict__:
            ports = []
            ports.append(testcase.config.egress_mirror.__dict__[ssn].intf)
            return ports
        else:
            return testcase.config.egress_mirror.__dict__[ssn].intf.ports
    return None

def GetRspanVlan(testcase, packet):
    if "session2" in testcase.config.ingress_mirror.__dict__ and testcase.config.ingress_mirror.__dict__["session2"] is not None: 
        return testcase.config.ingress_mirror.__dict__["session2"].segment.vlan_id
    else:
        return 0

def GetTruncatePktSize(testcase, pkt, args):
    case = testcase.pvtdata.span_case
    ssns = telemetry.spanSessionData.getSessions(case)
    if args.sessionid > len(ssns):
        return 0
    (id, direc, spantype, pkt, intf, pktlen) = ssns[args.sessionid -1]
    retpkt = testcase.packets.Get(pkt)
    if pktlen == 0:
        return retpkt.size
    hdrsize = 0
    if spantype == "RSPAN":
        hdrsize = hdrsize + 4
    if spantype == "ERSPAN":
        hdrsize = hdrsize + 54
    if retpkt.size < pktlen:
        return retpkt.size
    testcase.info("GetTruncatePktSize returning ", pktlen + hdrsize)
    return (pktlen + hdrsize)

def truncatePkt(testcase, pktbuf, pktlen, spantype, args):
    args.end = pktlen
    hdrsize = 0
    if spantype == "RSPAN":
        hdrsize = hdrsize + 4
    if spantype == "ERSPAN":
        hdrsize = hdrsize + 54
    args.end = args.end + hdrsize
    if pktlen == 0 or pktbuf.size <= pktlen:
        args.end = pktbuf.size 
    args.pktid = pktbuf.GID()
    testcase.info("truncatePkt truncating %s to size %d" % (args.pktid, args.end))
    return pktslicer.GetPacketSlice(testcase, pktbuf, args)


def GetTruncatePacketPayload(testcase, pkt, args):
    case = testcase.pvtdata.span_case
    ssns = telemetry.spanSessionData.getSessions(case)
    if args.sessionid > len(ssns):
        args.end = 0
        args.pktid = "RAW_PKT1"
        return pktslicer.GetPacketSlice(testcase, pkt, args)
    (id, direc, spantype, pkt, intf, pktlen) = ssns[args.sessionid -1]
    retpkt = testcase.packets.Get(pkt)
    args.end = pktlen
    hdrsize = 0
    if spantype == "RSPAN":
        hdrsize = hdrsize + 4
    if spantype == "ERSPAN":
        hdrsize = hdrsize + 54
    args.end = args.end + hdrsize
    if retpkt.size >= pktlen:
        args.end = retpkt.size
    args.pktid = pkt
    testcase.info("GetTruncatePacketPayload truncating %s to size %d" % (args.pktid, args.end))
    return pktslicer.GetPacketSlice(testcase, retpkt, args)

def GetTunnelSrcMac(testcase, pkt, args):
    id = (args.id)
    if args.dir == "egress":
        id = id + 3
    tnl = None
    testcase.info("getting tunnel source for ", id)
    obj = telemetry.data.getLocalSessionBySession(id)
    if obj is not None:
        if 'remote_ep' in obj.__dict__:
            testcase.info("getting tunnel source is Tunnel ", id)
            tnl = obj
    if tnl is None:
        tnl = telemetry.data.getErspanSession(id)
       
    if tnl is not None:
        testcase.info("returning Tunnel Src mac ", tnl.remote_ep.segment.macaddr.get())
        return tnl.remote_ep.segment.macaddr
    return None


def GetTunnelDstMac(testcase, pkt, args):
    id = (args.id)
    if args.dir == "egress":
        id = id + 3
    tnl = None
    obj = telemetry.data.getLocalSessionBySession(id)
    if obj is not None:
        if 'remote_tep' in obj.__dict__:
            tnl = obj
    if tnl is None:
        tnl = telemetry.data.getErspanSession(id)
    if tnl is not None:
        testcase.info("returning Tunnel Dst mac ", tnl.remote_ep.macaddr.get())
        return tnl.remote_ep.macaddr
    return None

def GetTunnelVlanEncap(testcase, pkt, args):
    id = (args.id)
    if args.dir == "egress":
        id = id + 3
    tnl = None
    obj = telemetry.data.getLocalSessionBySession(id)
    if obj is not None:
        if 'remote_tep' in obj.__dict__:
            tnl = obj
    if tnl is None:
        tnl = telemetry.data.getErspanSession(id)
    if tnl is None:
        return None
    if tnl.local_dest:
        testcase.info("returning Tunnel vlan encap ", tnl.remote_ep.intf.encap_vlan_id)
        return tnl.remote_ep.intf.encap_vlan_id
    testcase.info("returning Tunnel vlan encap ", tnl.remote_ep.segment.vlan_id)
    return tnl.remote_ep.segment.vlan_id

def GetTunnelSourceIP(testcase, pkt, args):
    id = (args.id)
    if args.dir == "egress":
        id = id + 3
    tnl = None
    obj = telemetry.data.getLocalSessionBySession(id)
    if obj is not None:
        if 'remote_tep' in obj.__dict__:
            tnl = obj
    if tnl is None:
        tnl = telemetry.data.getErspanSession(id)
    if tnl is not None:
        testcase.info("returning Tunnel Source IP ", tnl.GetSrcIp().get())
        return tnl.GetSrcIp()
    return None

def GetTunnelDestIP(testcase, pkt, args):
    id = (args.id)
    if args.dir == "egress":
        id = id + 3
    tnl = None
    obj = telemetry.data.getLocalSessionBySession(id)
    if obj is not None:
        if 'remote_tep' in obj.__dict__:
            tnl = obj
    if tnl is None:
        tnl = telemetry.data.getErspanSession(id)
    if tnl is not None:
        testcase.info("returning Tunnel Dest IP ", tnl.GetDestIp().get())
        return tnl.GetDestIp()
    return None

def GetTriggerObj(testcase, args):
    ret = telemetry.data.getLocalSession(args.sessionid)
    if ret is not None:
        desc = "SDESC" + str(args.sessionid)
        return testcase.descriptors.Get(desc)
    return None

def GetExpectObj(testcase, args):
    ret = telemetry.data.getLocalSession(args.sessionid)
    if ret is not None:
        desc = "EXP_DESC" + str(args.sessionid)
        return testcase.descriptors.Get(desc)
    return None

def GetTriggerRing(testcase, args):
    ret = telemetry.data.getLocalSession(args.sessionid)
    if ret is not None:
        (sess, direc, spantype, pkt, pktlen, obj) = ret
        if spantype == "ERSPAN":
            testcase.info("returning Trigger ring for tnl dest ip %s lif %d" % (obj.GetDestIp().get(), obj.remote_ep.intf.lif.id))
            return obj.remote_ep.intf.lif.queue_types.db["RX"].queues.db["Q0"].rings.db["R0"]
        else:
            testcase.info("returning Trigger ring for tnl dest ip %s lif %d" % (obj.GID(), obj.lif.id))
            return obj.lif.queue_types.db["RX"].queues.db["Q0"].rings.db["R0"]
    return None

def GetExpectRing(testcase, args):
    ret = telemetry.data.getLocalSession(args.sessionid)
    if ret is not None:
        (sess, direc, spantype, pkt, pktlen, obj) = ret
        if spantype == "ERSPAN":
            testcase.info("returning Expect ring for tnl dest ip %s lif %d" % (obj.GetDestIp().get(), obj.remote_ep.intf.lif.id))
            return obj.remote_ep.intf.lif.queue_types.db["RX"].queues.db["Q0"].rings.db["R1"]
        else:
            testcase.info("returning Expect ring for tnl dest ip %s lif %d" % (obj.GID(), obj.lif.id))
            return obj.lif.queue_types.db["RX"].queues.db["Q0"].rings.db["R1"]
    return None

def GetExpectBufSize(testcase, pkt, args):
    ret = telemetry.data.getLocalSession(args.sessionid)
    if ret is not None:
        (sess, direc, spantype, pkt, pktlen, obj) = ret
        pktbuf = testcase.packets.Get(pkt)
        if pktlen != 0:
            hdrsize = 0
            if spantype == "ERSPAN":
                hdrsize = 54
            if pktbuf.size < pktlen:
                testcase.info("GetExpectBufSize returning %s:%d / %d"% (pktbuf.GID(), pktbuf.size, pktlen))
                return pktbuf.size
            testcase.info("GetExpectBufSize returning %s:%d/%d" % (pktbuf.GID(), pktbuf.size, pktlen))
            return pktlen + hdrsize
        testcase.info("GetExpectBufSize returning %s:%d/%d" % (pktbuf.GID(), pktbuf.size, pktlen))
        return pktbuf.size
    return 0

def GetExpectBufData(testcase, pkt, args):
    ret = telemetry.data.getLocalSession(args.sessionid)
    if ret is not None:
        (sess, direc, spantype, pkt, pktlen, obj) = ret
        pktbuf = testcase.packets.Get(pkt)
        tpkt = truncatePkt(testcase, pktbuf, pktlen, spantype, args)
        return bytes(tpkt)
    return None

def GetTriggerRing1(testcase, args):
    case = testcase.pvtdata.span_case
    ssns = telemetry.spanSessionData.getSessions(case)
    if args.sessionid > len(ssns):
        return None
    (id, direc, spantype, pkt, intf, pktlen) = ssns[args.sessionid -1]
    if spantype == "ERSPAN":
        ep = telemetry.data.getLocalErspanSession(args.sessionid).remote_ep
    else:
        ep = telemetry.data.getLocalEpDest(args.sessionid)
    if ep is not None:
        return ep.intf.lif.queue_types.db["RX"].queues.db["Q0"].rings.db["R0"]
    return None

def GetExpectRing1(testcase, args):
    case = testcase.pvtdata.span_case
    ssns = telemetry.spanSessionData.getSessions(case)
    if args.sessionid > len(ssns):
        return None
    (id, direc, spantype, pkt, intf, pktlen) = ssns[args.sessionid -1]
    if spantype == "ERSPAN":
        ep = telemetry.data.getLocalErspanSession(args.sessionid).remote_ep
    else:
        ep = telemetry.data.getLocalEpDest(args.sessionid)
    if ep is not None:
        testcase.info("Returning EP %s Lif %s for session %d" % (ep.GID(), ep.intf.lif.id, args.sessionid))
        return ep.intf.lif.queue_types.db["RX"].queues.db["Q0"].rings.db["R1"]
    return None

def GetCollectorSrcMac(testcase, args):
    cs = Store.objects.GetAllByClass(collector.CollectorObject)
    if len(cs) < 1:
        return None
    return cs[0].src_ep.segment.macaddr.get()

def GetCollectorDstMac(testcase, args):
    cs = Store.objects.GetAllByClass(collector.CollectorObject)
    if len(cs) < 1:
        return None
    return cs[0].dst_ep.macaddr.get()

def GetCollectorSrcIp(testcase, args):
    cs = Store.objects.GetAllByClass(collector.CollectorObject)
    if len(cs) < 1:
        return None
    return cs[0].source_ip.get()

def GetCollectorDstIp(testcase, args):
    cs = Store.objects.GetAllByClass(collector.CollectorObject)
    if len(cs) < 1:
        return None
    return cs[0].dest_ip.get()

def GetCollectorVlan(testcase, args):
    cs = Store.objects.GetAllByClass(collector.CollectorObject)
    if len(cs) < 1:
        return None
    return cs[0].vlan

def GetErspanPayload(testcase, inpkt, args):
    basepkt = testcase.packets.Get(args.basepkt)
    case = testcase.pvtdata.span_case
    ssns = telemetry.spanSessionData.getSessions(case)
    plen = 0
    found = False
    for (id, direc, spantype, pkt, intf, pktlen) in ssns:
        if pkt == args.pktid:
            plen = pktlen
            found = True
            break
    if not found:
        plen = 0
    args.end = plen
    if plen == 0 or basepkt.size <= plen:
        args.end = basepkt.size
    args.pktid = args.basepkt
    testcase.info("GetErspanPayload returning %s size %d" % (args.pktid, args.end))
    return pktslicer.GetPacketSlice(testcase, basepkt, args)

def GetErspanPktSize(testcase, inpkt, args):
    basepkt = testcase.packets.Get(args.basepkt)
    case = testcase.pvtdata.span_case
    ssns = telemetry.spanSessionData.getSessions(case)
    plen = 0
    for (id, direc, spantype, pkt, intf, pktlen) in ssns:
        if pkt == args.pktid:
            plen = pktlen
            break
    if plen == 0:
        return basepkt.size
    hdrsize = 0
    if spantype == "RSPAN":
        hdrsize = hdrsize + 4
    if spantype == "ERSPAN":
        hdrsize = hdrsize + 54
    if basepkt.size < pktlen:
        testcase.info("GetErspanPktSize returning ", basepkt.size + hdrsize)
        return (basepkt.size + hdrsize)
    testcase.info("GetErspanPktSize returning ", plen + hdrsize)
    return (plen + hdrsize)

def GetErspanPayloadSize(testcase, inpkt, args):
    basepkt = testcase.packets.Get(args.basepkt)
    case = testcase.pvtdata.span_case
    ssns = telemetry.spanSessionData.getSessions(case)
    plen = 0
    for (id, direc, spantype, pkt, intf, pktlen) in ssns:
        if pkt == args.pktid:
            plen = pktlen
            break
    if plen == 0:
        return basepkt.size
    if basepkt.size < pktlen:
        return basepkt.size
    testcase.info("GetErspanPktSize returning ", plen)
    return plen


