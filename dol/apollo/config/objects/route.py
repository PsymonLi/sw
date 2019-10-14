#! /usr/bin/python3
import pdb
import ipaddress
import sys

import infra.config.base as base
import apollo.config.resmgr as resmgr
import apollo.config.agent.api as api
import apollo.config.utils as utils
import apollo.config.objects.lmapping as lmapping
import apollo.config.objects.nexthop as nexthop
import route_pb2 as route_pb2
import tunnel_pb2 as tunnel_pb2
import types_pb2 as types_pb2

from infra.common.logging import logger
from apollo.config.store import Store

class RouteObject(base.ConfigObjectBase):
    def __init__(self, parent, af, routes, routetype, tunobj, vpcpeerid):
        super().__init__()
        ################# PUBLIC ATTRIBUTES OF ROUTE TABLE OBJECT #####################
        if af == utils.IP_VERSION_6:
            self.RouteTblId = next(resmgr.V6RouteTableIdAllocator)
            self.AddrFamily = 'IPV6'
            self.NEXTHOP = nexthop.client.GetV6Nexthop(parent.VPCId)
        else:
            self.RouteTblId = next(resmgr.V4RouteTableIdAllocator)
            self.AddrFamily = 'IPV4'
            self.NEXTHOP = nexthop.client.GetV4Nexthop(parent.VPCId)
        self.GID('RouteTable%d' %self.RouteTblId)
        self.routes = routes
        self.TUNNEL = tunobj
        if self.TUNNEL:
            self.TunnelId = self.TUNNEL.Id
            self.TunIPAddr = tunobj.RemoteIPAddr
            self.TunIP = str(self.TunIPAddr)
        else:
            self.TunnelId = 0
        self.NexthopId = self.NEXTHOP.NexthopId if self.NEXTHOP else 0
        self.VPCId = parent.VPCId
        self.Label = 'NETWORKING'
        self.RouteType = routetype # used for lpm route cases
        self.PeerVPCId = vpcpeerid
        self.AppPort = resmgr.TransportDstPort
        ##########################################################################
        self.deleted = False
        self._derive_oper_info()
        self.Show()
        return

    def _derive_oper_info(self):
        # operational info useful for testspec
        routetype = self.RouteType
        self.HasDefaultRoute = True if 'default' in routetype else False
        self.VPCPeeringEnabled = True if 'vpc_peer' in routetype else False
        self.HasBlackHoleRoute = True if 'blackhole' in routetype else False
        self.HasNexthop = True if self.NEXTHOP else False
        self.HasServiceTunnel = False
        if utils.IsPipelineArtemis():
            if self.TUNNEL and self.TUNNEL.Type == tunnel_pb2.TUNNEL_TYPE_SERVICE:
                self.HasServiceTunnel = True
        if self.HasBlackHoleRoute:
            self.NextHopType = "blackhole"
        elif self.VPCPeeringEnabled:
            self.NextHopType = "vpcpeer"
        else:
            if utils.IsPipelineArtemis() and\
               self.HasServiceTunnel == False:
                self.NextHopType = "nh"
            else:
                self.NextHopType = "tep"
        return

    def __repr__(self):
        return "RouteTableID:%d|VPCId:%d|AddrFamily:%s|NumRoutes:%d|RouteType:%s"\
               %(self.RouteTblId, self.VPCId, self.AddrFamily,\
                 len(self.routes), self.RouteType)

    def GetGrpcCreateMessage(self, cookie):
        grpcmsg = route_pb2.RouteTableRequest()
        grpcmsg.BatchCtxt.BatchCookie = cookie
        spec = grpcmsg.Request.add()
        spec.Id = self.RouteTblId
        spec.Af = utils.GetRpcIPAddrFamily(self.AddrFamily)
        for route in self.routes:
            rtspec = spec.Routes.add()
            utils.GetRpcIPPrefix(route, rtspec.Prefix)
            if self.NextHopType == "vpcpeer":
                rtspec.VPCId = self.PeerVPCId
            elif self.NextHopType == "tep":
                rtspec.TunnelId = self.TunnelId
            elif self.NextHopType == "nh":
                rtspec.NexthopId = self.NexthopId
        return grpcmsg

    def GetGrpcReadMessage(self):
        grpcmsg = route_pb2.RouteTableGetRequest()
        grpcmsg.Id.append(self.RouteTblId)
        return grpcmsg

    def GetGrpcDeleteMessage(self, cookie):
        grpcmsg = route_pb2.RouteTableDeleteRequest()
        grpcmsg.BatchCtxt.BatchCookie = cookie
        grpcmsg.Id.append(self.RouteTblId)
        return grpcmsg

    def ValidateSpec(self, spec):
        if spec.Id != self.RouteTblId:
            return False
        if spec.Af != utils.GetRpcIPAddrFamily(self.AddrFamily):
            return False
        return True

    def ValidateStats(self, stats):
        return True

    def ValidateStatus(self, status):
        return True

    def Create(self, spec=None):
        utils.CreateObject(self, api.ObjectTypes.ROUTE)

    def Read(self, expRetCode=types_pb2.API_STATUS_OK):
        return utils.ReadObject(self, api.ObjectTypes.ROUTE, expRetCode)

    def ReadAfterDelete(self, spec=None):
        return self.Read(types_pb2.API_STATUS_NOT_FOUND)

    def Delete(self, spec=None):
        utils.DeleteObject(self, api.ObjectTypes.ROUTE)

    def Equals(self, obj, spec):
        return True

    def Show(self):
        logger.info("RouteTable object:", self)
        logger.info("- %s" % repr(self))
        logger.info("- HasDefaultRoute:%s|HasBlackHoleRoute:%s"\
                    %(self.HasDefaultRoute, self.HasBlackHoleRoute))
        logger.info("- VPCPeering:%s Peer Vpc%d" %(self.VPCPeeringEnabled, self.PeerVPCId))
        logger.info("- NH : Nexthop%d|Type:%s" %(self.NexthopId, self.NextHopType))
        logger.info("- TEP: Tunnel%d|IP:%s" %(self.TunnelId, self.TunIP))
        for route in self.routes:
            logger.info(" -- %s" % str(route))
        return

    def MarkDeleted(self, flag=True):
        self.deleted = flag

    def IsDeleted(self):
        return self.deleted

    def IsFilterMatch(self, selectors):
        return super().IsFilterMatch(selectors.route.filters)

    def SetupTestcaseConfig(self, obj):
        obj.localmapping = self.l_obj
        obj.route = self
        obj.tunnel = self.TUNNEL
        obj.hostport = Store.GetHostPort()
        obj.switchport = Store.GetSwitchPort()
        obj.devicecfg = Store.GetDevice()
        return

class RouteObjectClient:
    def __init__(self):
        self.__objs = dict()
        self.__v4objs = {}
        self.__v6objs = {}
        self.__v4iter = {}
        self.__v6iter = {}
        return

    def __internet_tunnel_get(self, nat, teptype=None):
        if teptype is not None and "service" in teptype:
            if "remoteservice" == teptype:
                return resmgr.RemoteSvcTunAllocator.rrnext()
            return resmgr.SvcTunAllocator.rrnext()
        if nat is False:
            return resmgr.RemoteInternetNonNatTunAllocator.rrnext()
        else:
            return resmgr.RemoteInternetNatTunAllocator.rrnext()

    def Objects(self):
        return self.__objs.values()

    def GetRouteTableObject(self, routetableid):
        return self.__objs.get(routetableid, None)

    def GetRouteV4Tables(self, vpcid):
        return self.__v4objs.get(vpcid, None)

    def GetRouteV6Tables(self, vpcid):
        return self.__v6objs.get(vpcid, None)

    def IsValidConfig(self):
        count = sum(list(map(lambda x: len(x.values()), self.__v4objs.values())))
        if  count > resmgr.MAX_ROUTE_TABLE:
            return False, "V4 Route Table count %d exceeds allowed limit of %d" %\
                          (count, resmgr.MAX_ROUTE_TABLE)
        count = sum(list(map(lambda x: len(x.values()), self.__v6objs.values())))
        if  count > resmgr.MAX_ROUTE_TABLE:
            return False, "V6 Route Table count %d exceeds allowed limit of %d" %\
                          (count, resmgr.MAX_ROUTE_TABLE)
        #TODO: check route table count equals subnet count in that VPC
        #TODO: check scale of routes per route table
        return True, ""

    def GetRouteV4Table(self, vpcid, routetblid):
        return self.__v4objs[vpcid].get(routetblid, None)

    def GetRouteV6Table(self, vpcid, routetblid):
        return self.__v6objs[vpcid].get(routetblid, None)

    def GetRouteV4TableId(self, vpcid):
        if self.__v4objs[vpcid]:
            return self.__v4iter[vpcid].rrnext().RouteTblId
        return 0

    def GetRouteV6TableId(self, vpcid):
        if self.__v6objs[vpcid]:
            return self.__v6iter[vpcid].rrnext().RouteTblId
        return 0

    def GenerateObjects(self, parent, vpc_spec_obj, vpcpeerid):
        vpcid = parent.VPCId
        stack = parent.Stack
        isV4Stack = True if ((stack == "dual") or (stack == 'ipv4')) else False
        isV6Stack = True if ((stack == "dual") or (stack == 'ipv6')) else False
        self.__v4objs[vpcid] = dict()
        self.__v6objs[vpcid] = dict()
        self.__v4iter[vpcid] = None
        self.__v6iter[vpcid] = None

        if resmgr.RemoteInternetNonNatTunAllocator == None and resmgr.RemoteInternetNatTunAllocator == None:
            logger.info("Skipping route creation as there are no Internet tunnels")
            return

        def __get_adjacent_routes(base, count):
            routes = []
            c = 1
            routes.append(ipaddress.ip_network(base))
            while c < count:
                routes.append(utils.GetNextSubnet(routes[c-1]))
                c += 1
            return routes

        def __get_overlap(basepfx, base, count):
            # for overlap, add user specified base prefix with original prefixlen
            routes = __get_user_specified_routes([basepfx])
            routes.extend(__get_adjacent_routes(base, count))
            return routes

        def __get_first_subnet(ip, prefixlen):
            for ip in ip.subnets(new_prefix=prefixlen):
                return (ip)
            return

        def __add_v4routetable(v4routes, routetype):
            obj = RouteObject(parent, utils.IP_VERSION_4, v4routes, routetype, tunobj, vpcpeerid)
            self.__v4objs[vpcid].update({obj.RouteTblId: obj})
            self.__objs.update({obj.RouteTblId: obj})

        def __add_v6routetable(v6routes, routetype):
            obj = RouteObject(parent, utils.IP_VERSION_6, v6routes, routetype, tunobj, vpcpeerid)
            self.__v6objs[vpcid].update({obj.RouteTblId: obj})
            self.__objs.update({obj.RouteTblId: obj})

        def __get_user_specified_routes(routespec):
            routes = []
            if routespec:
                for route in routespec:
                    routes.append(ipaddress.ip_network(route.replace('\\', '/')))
            return routes

        def __add_user_specified_routetable(routetablespec, routetype):
            if isV4Stack:
                __add_v4routetable(__get_user_specified_routes(routetablespec.v4routes), routetype)

            if isV6Stack:
                __add_v6routetable(__get_user_specified_routes(routetablespec.v6routes), routetype)

        def __get_valid_route_count_per_route_table(count):
            if count > resmgr.MAX_ROUTES_PER_ROUTE_TBL:
                return resmgr.MAX_ROUTES_PER_ROUTE_TBL
            return count

        def __get_nat_teptype_from_spec(routetablespec):
            nat = getattr(routetbl_spec_obj, 'nat', False)
            teptype = getattr(routetbl_spec_obj, 'teptype', None)
            return nat, teptype

        for routetbl_spec_obj in vpc_spec_obj.routetbl:
            routetbltype = routetbl_spec_obj.type
            routetype = routetbl_spec_obj.routetype
            nat, teptype = __get_nat_teptype_from_spec(routetbl_spec_obj)
            tunobj = self.__internet_tunnel_get(nat, teptype)
            if routetbltype == "specific":
                __add_user_specified_routetable(routetbl_spec_obj, routetype)
                continue
            routetablecount = routetbl_spec_obj.count
            v4routecount = __get_valid_route_count_per_route_table(routetbl_spec_obj.nv4routes)
            v6routecount = __get_valid_route_count_per_route_table(routetbl_spec_obj.nv6routes)
            v4prefixlen = routetbl_spec_obj.v4prefixlen
            v6prefixlen = routetbl_spec_obj.v6prefixlen
            v4base = __get_first_subnet(ipaddress.ip_network(routetbl_spec_obj.v4base.replace('\\', '/')), v4prefixlen)
            v6base = __get_first_subnet(ipaddress.ip_network(routetbl_spec_obj.v6base.replace('\\', '/')), v6prefixlen)
            # get user specified routes if any for 'base' routetbltype
            user_specified_v4routes = __get_user_specified_routes(routetbl_spec_obj.v4routes)
            user_specified_v6routes = __get_user_specified_routes(routetbl_spec_obj.v6routes)
            v4routecount -= len(user_specified_v4routes)
            v6routecount -= len(user_specified_v6routes)
            if 'overlap' in routetype:
                v4routecount -= 1
                v6routecount -= 1
            for i in range(routetablecount):
                if 'adjacent' in routetype:
                    if isV4Stack:
                        routes = user_specified_v4routes + __get_adjacent_routes(v4base, v4routecount)
                        __add_v4routetable(routes, routetype)
                        v4base = utils.GetNextSubnet(routes[-1])
                    if isV6Stack:
                        routes = user_specified_v6routes + __get_adjacent_routes(v6base, v6routecount)
                        __add_v6routetable(routes, routetype)
                        v6base = utils.GetNextSubnet(routes[-1])

                elif 'overlap' in routetype:
                    if isV4Stack:
                        routes = user_specified_v4routes + __get_overlap(routetbl_spec_obj.v4base, v4base, v4routecount)
                        __add_v4routetable(routes, routetype)
                        v4base = utils.GetNextSubnet(routes[-1])
                    if isV6Stack:
                        routes = user_specified_v6routes + __get_overlap(routetbl_spec_obj.v6base, v6base, v6routecount)
                        __add_v6routetable(routes, routetype)
                        v6base = utils.GetNextSubnet(routes[-1])

        if self.__v6objs[vpcid]:
            self.__v6iter[vpcid] = utils.rrobiniter(self.__v6objs[vpcid].values())

        if self.__v4objs[vpcid]:
            self.__v4iter[vpcid] = utils.rrobiniter(self.__v4objs[vpcid].values())

    def CreateObjects(self):
        cookie = utils.GetBatchCookie()
        msgs = list(map(lambda x: x.GetGrpcCreateMessage(cookie), self.__objs.values()))
        api.client.Create(api.ObjectTypes.ROUTE, msgs)
        return

    def GetGrpcReadAllMessage(self):
        grpcmsg = route_pb2.RouteTableGetRequest()
        return grpcmsg

    def ReadObjects(self):
        msg = self.GetGrpcReadAllMessage()
        resp = api.client.Get(api.ObjectTypes.ROUTE, [msg])
        result = self.ValidateObjects(resp)
        if result is False:
            logger.critical("Route table object validation failed!!!")
            sys.exit(1)
        return

    def ValidateObjects(self, getResp):
        if utils.IsDryRun(): return True
        for obj in getResp:
            if not utils.ValidateGrpcResponse(obj):
                logger.error("Route table get request failed for ", obj)
                return False
            for resp in obj.Response:
                spec = resp.Spec
                routeTable = self.GetRouteTableObject(spec.Id)
                if not utils.ValidateObject(routeTable, resp):
                    logger.error("Route table validation failed for ", obj)
                    routeTable.Show()
                    return False
        return True

client = RouteObjectClient()

class RouteTableObjectHelper:
    def __init__(self):
        return

    def __is_lmapping_match(self, route_obj, lobj):
        if lobj.AddrFamily == 'IPV4':
            return lobj.AddrFamily == route_obj.AddrFamily and\
               lobj.VNIC.SUBNET.V4RouteTableId == route_obj.RouteTblId
        if lobj.AddrFamily == 'IPV6':
            return lobj.AddrFamily == route_obj.AddrFamily and\
               lobj.VNIC.SUBNET.V6RouteTableId == route_obj.RouteTblId

    def GetMatchingConfigObjects(self, selectors, all_objs):
        objs = []
        rtype = selectors.route.GetValueByKey('RouteType')
        for route_obj in client.Objects():
            if not route_obj.IsFilterMatch(selectors):
                continue
            if rtype != 'empty' and 0 == len(route_obj.routes):
                # skip route tables with no routes
                continue
            for lobj in lmapping.GetMatchingObjects(selectors):
                if self.__is_lmapping_match(route_obj, lobj):
                    route_obj.l_obj = lobj
                    objs.append(route_obj)
                    break
        if all_objs is True:
            maxlimits = 0
        else:
            maxlimits = selectors.maxlimits
        return utils.GetFilteredObjects(objs, maxlimits)


RouteTableHelper = RouteTableObjectHelper()

def GetMatchingObjects(selectors):
    return RouteTableHelper.GetMatchingConfigObjects(selectors, False)

def GetAllMatchingObjects(selectors):
    return RouteTableHelper.GetMatchingConfigObjects(selectors, True)
