#! /usr/bin/python3
import pdb

import vnic_pb2 as vnic_pb2
import types_pb2 as types_pb2
import ipaddress
import utils
import re
import api

class VnicObject():
    def __init__(self, id, subnetid, macaddr, hostifindex, sourceguard=False,
                 fabricencap='VXLAN', fabricencapid=1, rxmirrorid = [],
                 txmirrorid = [], node_uuid=None, primary=False, encap='NONE',
                 vlan1=None, vlan2=None, flow_learn_en=True, hostname=''):
        self.id       = id
        self.uuid     = utils.PdsUuid(self.id, objtype=api.ObjectTypes.VNIC)
        self.primary  = primary
        self.macaddr  = macaddr
        self.subnetid = subnetid
        self.fabricencap = fabricencap
        self.fabricencapid = fabricencapid
        if hostifindex is not None:
            self.hostifindex = int(hostifindex, 16)
            if node_uuid:
                self.hostifuuid = utils.PdsUuid(self.hostifindex, node_uuid)
            else:
                self.hostifuuid = utils.PdsUuid(self.hostifindex)
        else:
            self.hostifuuid = None
        self.sourceguard = sourceguard
        self.rxmirrorid = rxmirrorid
        self.txmirrorid = txmirrorid
        self.encap = encap
        self.vlan1 = vlan1
        self.vlan2 = vlan2
        self.flow_learn_en = flow_learn_en
        self.hostname = hostname
        return

    def GetGrpcCreateMessage(self):
        grpcmsg = vnic_pb2.VnicRequest()
        spec = grpcmsg.Request.add()
        spec.Id = self.uuid.GetUuid()
        spec.Primary = self.primary
        spec.SubnetId = utils.PdsUuid.GetUUIDfromId(self.subnetid,
                                                    objtype=api.ObjectTypes.SUBNET)
        spec.MACAddress = utils.getmac2num(self.macaddr)
        spec.SourceGuardEnable = self.sourceguard
        if re.search( 'VXLAN', self.fabricencap, re.I ):
           spec.FabricEncap.type = types_pb2.ENCAP_TYPE_VXLAN
           spec.FabricEncap.value.Vnid = self.fabricencapid
        if self.hostifuuid is not None:
            spec.HostIf = self.hostifuuid.GetUuid()
        if self.rxmirrorid:
            spec.RxMirrorSessionId.extend(self.rxmirrorid)
        if self.txmirrorid:
            spec.TxMirrorSessionId.extend(self.txmirrorid)
        if re.search( 'DOT1Q', self.encap, re.I ):
            spec.VnicEncap.type = types_pb2.ENCAP_TYPE_DOT1Q
            spec.VnicEncap.value.VlanId = self.vlan1
        elif re.search( 'QinQ', self.encap, re.I ):
            spec.VnicEncap.type = types_pb2.ENCAP_TYPE_QINQ
            spec.VnicEncap.value.QinQ.STag = self.vlan1
            spec.VnicEncap.value.QinQ.CTag = self.vlan2
        spec.FlowLearnEn = self.flow_learn_en
        spec.HostName = self.hostname
        return grpcmsg
