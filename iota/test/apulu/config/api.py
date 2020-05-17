#! /usr/bin/python3
import random
import copy
#Following come from dol/infra
from apollo.config.generator import ObjectInfo as ObjClient
from apollo.config.agent.api import ObjectTypes as APIObjTypes
import apollo.config.objects.vnic as vnic
import apollo.config.objects.lmapping as lmapping
import apollo.config.objects.nat_pb as nat_pb
import apollo.config.utils as utils
import apollo.config.topo as topo
import vpc_pb2 as vpc_pb2
import iota.harness.api as api
import vpc_pb2 as vpc_pb2

WORKLOAD_PAIR_TYPE_LOCAL_ONLY    = 1
WORKLOAD_PAIR_TYPE_REMOTE_ONLY   = 2
WORKLOAD_PAIR_TYPE_IGW_ONLY      = 3

WORKLOAD_PAIR_SCOPE_INTRA_SUBNET = 1
WORKLOAD_PAIR_SCOPE_INTER_SUBNET = 2
WORKLOAD_PAIR_SCOPE_INTER_VPC    = 3

class Endpoint:
    def __init__(self, vnic_inst, ip_addresses):
        self.name = vnic_inst.GID()
        self.macaddr = vnic_inst.MACAddr.get()
        self.vlan = vnic_inst.VlanId
        self.ip_addresses = ip_addresses
        self.node_name = vnic_inst.Node
        self.has_public_ip = vnic_inst.HasPublicIp
        if api.GlobalOptions.dryrun:
            self.interface = 'dryrun'
        elif vnic_inst.SUBNET.VPC.Type == vpc_pb2.VPC_TYPE_CONTROL:
            self.interface = 'control'
        else:
            self.interface = GetObjClient('interface').GetHostIf(vnic_inst.Node, vnic_inst.SUBNET.HostIfIdx).GetInterfaceName()
        self.vnic = vnic_inst

class VnicRoute:
    def __init__(self, vnic_inst, ip_addresses):
        self.routes = vnic_inst.RemoteRoutes
        self.gw = vnic_inst.SUBNET.VirtualRouterIPAddr[1]
        self.node_name = vnic_inst.Node
        wload = FindWorkloadByVnic(vnic_inst)
        self.wload_name = wload.workload_name
        if ip_addresses:
            self.vnic_ip = ip_addresses[0]
        else:
            self.vnic_ip = None

def FindWorkloadByVnic(vnic_inst):
    return __findWorkloadByVnic(vnic_inst)

def GetEndpoints():
    naplesHosts = api.GetNaplesHostnames()
    eps = []
    for node in naplesHosts:
        vnics = vnic.client.Objects(node)
        for vnic_inst in vnics:
            if vnic_inst.SUBNET.VPC.Type == vpc_pb2.VPC_TYPE_TENANT:
                vnic_addresses = lmapping.client.GetVnicAddresses(vnic_inst)
                ep = Endpoint(vnic_inst, vnic_addresses)
                eps.append(ep)

    return eps

def GetVnicRoutes(vnic_obj=None):
    if vnic_obj:
        naplesHosts = [ vnic_obj.Node ]
    else:
        naplesHosts = api.GetNaplesHostnames()
    vnic_routes = []
    for node in naplesHosts:
        if vnic_obj:
            vnics = [ vnic_obj ]
        else:
            vnics = vnic.client.Objects(node)
        for vnic_inst in vnics:
            vnic_addresses = lmapping.client.GetVnicAddresses(vnic_inst)
            if len(vnic_inst.RemoteRoutes) != 0:
                route = VnicRoute(vnic_inst, vnic_addresses)
                vnic_routes.append(route)
    return vnic_routes


def GetMovableWorkload(node_name=None):
    wloads = []
    for node in api.GetNaplesHostnames():
        if node_name and node != node_name:
            continue
        wloads += [__findWorkloadByVnic(vnic) for vnic in vnic.client.Objects(node) if vnic.Movable]
    return wloads

def GetObjClient(objname):
    return ObjClient[objname]

def __vnics_in_same_segment(vnic1, vnic2):
    if vnic1.SUBNET.GID() == vnic2.SUBNET.GID():
        return True
    return False

def __vnics_in_same_vpc(vnic1, vnic2):
    if vnic1.SUBNET.VPC.GID() == vnic2.SUBNET.VPC.GID():
        return True
    return False

def __vnics_are_dynamic_napt_pair(vnic1, vnic2):
    if vnic1.Node == vnic2.Node:
        return False
    if not vnic1.HasPublicIp and vnic1.VnicType == "local" and vnic2.VnicType == "igw":
        return True
    return False

def __vnics_are_static_nat_pair(vnic1, vnic2, direction="tx"):
    if vnic1.Node == vnic2.Node:
        return False
    if direction == "tx":
        if vnic1.HasPublicIp and vnic1.VnicType == "local" and vnic2.VnicType == "igw":
            return True
    else:
        if vnic2.HasPublicIp and vnic2.VnicType == "local" and vnic1.VnicType == "igw":
            return True
    return False


def __vnics_are_dynamic_service_napt_pair(vnic1, vnic2, from_public):
    if vnic1.Node == vnic2.Node:
        return False
    if (from_public and not vnic1.HasPublicIp) or (not from_public and vnic1.HasPublicIp):
        return False
    if vnic1.VnicType == "local" and vnic2.VnicType == "igw_service":
        return True
    return False

def __findWorkloadByVnic(vnic_inst):
    wloads = api.GetWorkloads()
    vnic_mac = vnic_inst.MACAddr.get()
    for wload in wloads:
        if wload.mac_address == vnic_mac:
            return wload
    return None

def __getWorkloadPairsBy(wl_pair_type, wl_pair_scope = WORKLOAD_PAIR_SCOPE_INTRA_SUBNET, \
                         nat_type=None, local_vnic_has_public_ip=None, direction="tx"):
    wl_pairs = []
    naplesHosts = api.GetNaplesHostnames()
    service_vnics_done = []
    vnics = []
    for node in naplesHosts:
        vnics.extend(vnic.client.Objects(node))

    for vnic1 in vnics:
        for vnic2 in vnics:
            if vnic1 == vnic2:
                continue
            if vnic1.SUBNET.VPC.Type != vpc_pb2.VPC_TYPE_TENANT or vnic2.SUBNET.VPC.Type != vpc_pb2.VPC_TYPE_TENANT:
                continue
            if wl_pair_scope == WORKLOAD_PAIR_SCOPE_INTRA_SUBNET and not __vnics_in_same_segment(vnic1, vnic2):
                continue
            if wl_pair_scope == WORKLOAD_PAIR_SCOPE_INTER_SUBNET and\
               (__vnics_in_same_segment(vnic1, vnic2) or not __vnics_in_same_vpc(vnic1, vnic2)) :
                continue
            if wl_pair_type == WORKLOAD_PAIR_TYPE_LOCAL_ONLY and vnic1.Node != vnic2.Node:
                continue
            elif wl_pair_type == WORKLOAD_PAIR_TYPE_REMOTE_ONLY and\
                    ((vnic1.Node == vnic2.Node) or vnic1.IsIgwVnic() or vnic2.IsIgwVnic()):
                continue
            elif wl_pair_type == WORKLOAD_PAIR_TYPE_IGW_ONLY:
                if not nat_type:
                    continue
                if nat_type == "napt":
                    if not __vnics_are_dynamic_napt_pair(vnic1, vnic2):
                        continue
                elif nat_type == "static":
                    if not __vnics_are_static_nat_pair(vnic1, vnic2, direction):
                        continue
                    if direction == "rx":
                        w1 = __findWorkloadByVnic(vnic1)
                        # We need to ping w2 using its public ip, so change its ip address
                        w2 = copy.copy(__findWorkloadByVnic(vnic2))
                        w2.ip_address = lmapping.client.GetVnicPublicAddresses(vnic2)[0]
                        assert(w1 and w2)
                        wl_pairs.append((w1, w2))
                        continue
                if nat_type == "napt_service":
                    if not __vnics_are_dynamic_service_napt_pair(vnic1, vnic2, local_vnic_has_public_ip):
                        continue
                    if len(vnic1.ServiceIPs) == 0:
                        continue
                    if vnic1 in service_vnics_done:
                        continue
                    w1 = __findWorkloadByVnic(vnic1)
                    for service_ip in vnic1.ServiceIPs:
                        # We need to ping w2 using its service ip, so change its ip address
                        w2 = copy.copy(__findWorkloadByVnic(vnic2))
                        w2.ip_address = service_ip
                        assert(w1 and w2)
                        wl_pairs.append((w1, w2))
                    service_vnics_done.append(vnic1)
                    continue

            w1 = __findWorkloadByVnic(vnic1)
            w2 = __findWorkloadByVnic(vnic2)
            assert(w1 and w2)
            wl_pairs.append((w1, w2))

    return wl_pairs

def GetPingableWorkloadPairs(wl_pair_type = WORKLOAD_PAIR_TYPE_REMOTE_ONLY):
    return __getWorkloadPairsBy(wl_pair_type=wl_pair_type)

def GetWorkloadPairs(wl_pair_type, wl_pair_scope,
                     nat_type=None, local_vnic_has_public_ip=None, direction="tx"):
    return __getWorkloadPairsBy(wl_pair_type, wl_pair_scope, nat_type, \
                                local_vnic_has_public_ip, direction)

def __getObjects(objtype):
    objs = list()
    naplesHosts = api.GetNaplesHostnames()
    objClient = GetObjClient(objtype)
    for node in naplesHosts:
        if objtype == 'nexthop':
            objs.extend(objClient.GetUnderlayNexthops(node))
        elif objtype == 'vpc':
            vpcobjs = objClient.Objects(node, True)
            # metaswitch does not allow deletion of Underlay VPC, hence skip it
            objs.extend(list(filter(lambda x: x.Type != vpc_pb2.VPC_TYPE_UNDERLAY, vpcobjs)))
            return objs
        elif objtype == 'interface':
            interfaceobjs = objClient.Objects(node, True)
            objs.extend(list(filter(lambda x: x.Type != topo.InterfaceTypes.LOOPBACK, interfaceobjs)))
        else:
            obj = objClient.Objects(node, True)
            objs.extend(obj)
    return objs

def __getMaxLimit(objtype, objlist):
    return len(objlist)

def __getRandomSamples(objlist, num = None):
    limit = len(objlist)
    if num == None or num == 0 or limit < num:
        num = limit
    return random.sample(objlist, k=num)

def SetupConfigObjects(objtype, allnode=False):
    objlist = __getObjects(objtype)
    maxlimit = __getMaxLimit(objtype, objlist)
    select_count = int(maxlimit / 2) if maxlimit >= 2 else maxlimit

    # For allnode case if the given objtype's maxlimit is 1,
    # then return all entries in the objlist
    if allnode:
        objClient = GetObjClient(objtype)
        if objtype == 'security_profile' and objClient.Maxlimit == 1:
            select_count = maxlimit
    select_objs = __getRandomSamples(objlist, select_count)

    api.Logger.verbose(f"selected_objs: {select_objs}")
    return select_objs

def ProcessObjectsByOperation(oper, select_objs, spec=None):
    supported_ops = [ 'Create', 'Read', 'Delete', 'Update' ]
    res = api.types.status.SUCCESS
    if oper is None or oper not in supported_ops:
        return res
    for obj in select_objs:
        if getattr(obj, oper)(spec):
            if not getattr(obj, 'Read')():
                api.Logger.error(f"read after {oper} failed for object: {obj}")
                res = api.types.status.FAILURE
        else:
            api.Logger.error(f"{oper} failed for object: {obj}")
            res = api.types.status.FAILURE
        if oper == 'Delete':
            if not getattr(obj, 'VerifyDepsOperSt')(oper):
                api.Logger.error(f"Dependent object oper state not as expected after {oper} on {obj}")
                res = api.types.status.FAILURE
    return res

def FindVnicObjectByWorkload(wl):
    return wl.vnic

def __getLocalMappingObjectByWorkload(workload):
    lmapping_ = []
    lmapping_.extend(__getObjects('lmapping'))
    for lmp in lmapping_:
        if lmp.AddrFamily == 'IPV4' and lmp.IP == str(workload.ip_address):
            return lmp
        if lmp.AddrFamily == 'IPV6' and lmp.IP == str(workload.ipv6_address):
            return lmp
    return None

def __getRemoteMappingObjectByWorkload(workload):
    rmaps = list()
    rmaps.extend(__getObjects('rmapping'))
    for rmap in rmaps:
        if rmap.AddrFamily == 'IPV4' and rmap.IP == str(workload.ip_address):
            return rmap
        if rmap.AddrFamily == 'IPV6' and rmap.IP == str(workload.ipv6_address):
            return rmap
    return None

def IsAnyConfigDeleted(workload_pair):
    w1 = workload_pair[0]
    w2 = workload_pair[1]

    vnic1 = FindVnicObjectByWorkload(w1)
    vnic2 = FindVnicObjectByWorkload(w2)
    vnics = [ vnic1, vnic2 ]
    for vnic_ in vnics:
        w1 = __findWorkloadByVnic(vnic_)
        nexthop_ = None
        nexthopgroup_ = None
        interface_ = None
        vpc_ = None
        v4routetable_ = None
        v6routetable_ = None
        subnet_ = getattr(vnic_, "SUBNET", None)
        if subnet_:
            vpc_ = getattr(vnic_.SUBNET, "VPC", None)
            v4routetable_ = subnet_.V4RouteTable
            v6routetable_ = subnet_.V6RouteTable
        lmapping_ = __getLocalMappingObjectByWorkload(w1)
        rmapping_ = __getRemoteMappingObjectByWorkload(w2)
        tunnel_ = getattr(rmapping_, 'TUNNEL', None)
        if tunnel_ is None:
            pass
        elif tunnel_.IsUnderlayEcmp():
            nexthopgroup_ = getattr(tunnel_, "NEXTHOPGROUP", None)
        elif tunnel_.IsUnderlay():
            nexthop_ = getattr(tunnel_, "NEXTHOP", None)
            interface_ = getattr(nexthop_, "L3Interface", None)
        objs = [vpc_, subnet_, vnic_, lmapping_, v4routetable_, v6routetable_ ]
        objs.extend([rmapping_, tunnel_, nexthop_, nexthopgroup_, interface_])
        objs.extend(GetObjClient('device').Objects(w1.node_name))
        values = list(map(lambda x: not(x.IsHwHabitant()), list(filter(None, objs))))
        if any(values):
            return True
        w2 = __findWorkloadByVnic(vnic_)
    return False

def ReadConfigObject(obj):
    return obj.Read()

def RestoreObjects(oper, objlist):
    supported_ops = [ 'Delete', 'Update' ]
    if oper is None or oper not in supported_ops:
        return True
    else:
        rs = True
        if oper == 'Delete':
            for obj in objlist:
                obj.Create()
                if ReadConfigObject(obj) is False:
                    api.Logger.error(f"RestoreObjects:Read object failed for {obj} after {oper} operation")
                    rs = False
        elif oper == 'Update':
            for obj in objlist:
                obj.RollbackUpdate()
                if ReadConfigObject(obj) is False:
                    api.Logger.error(f"RestoreObjects:Read object failed for {obj} after {oper} operation")
                    rs = False
    return rs

def GetPolicyObjectsByWorkload(wl):
    return GetObjClient(APIObjTypes.POLICY.name.lower()).Objects(wl.node_name)

def GetUnderlayWorkloadPairs():
    naplesHosts = api.GetNaplesHostnames()
    workloads = []
    bgppeers = []
    for node in naplesHosts:
        bgppeers = bgp_peer.client.Objects(node)
        #TODO - get workloads from bgp peer objects
        return workloads

def GetVpcNatPortBlocks(wl, addr_type):
    vnic = FindVnicObjectByWorkload(wl)
    vpc_key = vnic.SUBNET.VPC.GetKey()
    if addr_type == "public":
        return nat_pb.client.GetVpcNatPortBlocks(utils.NAT_ADDR_TYPE_PUBLIC, vpc_key)
    else:
        return nat_pb.client.GetVpcNatPortBlocks(utils.NAT_ADDR_TYPE_SERVICE, vpc_key)

def GetAllNatPortBlocks():
    return nat_pb.client.GetAllNatPortBlocks()


def __getControlEPPairsBy(vnic_pair_type):
    ep_pairs = []
    naplesHosts = api.GetNaplesHostnames()
    vnics = []
    for node in naplesHosts:
        vnics.extend(vnic.client.Objects(node))

    for vnic1 in vnics:
        for vnic2 in vnics:
            if vnic1 == vnic2:
                continue
            if vnic1.SUBNET.VPC.Type != vpc_pb2.VPC_TYPE_CONTROL or vnic2.SUBNET.VPC.Type != vpc_pb2.VPC_TYPE_CONTROL:
                continue
            if vnic_pair_type == WORKLOAD_PAIR_TYPE_LOCAL_ONLY and vnic1.Node != vnic2.Node:
                continue
            elif vnic_pair_type == WORKLOAD_PAIR_TYPE_REMOTE_ONLY and\
                    vnic1.Node == vnic2.Node:
                continue

            vnic_addresses1 = lmapping.client.GetVnicIPs(vnic1)
            ep1 = Endpoint(vnic1, vnic_addresses1)
            vnic_addresses2 = lmapping.client.GetVnicIPs(vnic2)
            ep2 = Endpoint(vnic2, vnic_addresses2)

            ep_pairs.append((ep1, ep2))

    return ep_pairs

def GetControlEPPairs(pair_type = WORKLOAD_PAIR_TYPE_REMOTE_ONLY):
    return __getControlEPPairsBy(vnic_pair_type=pair_type)
