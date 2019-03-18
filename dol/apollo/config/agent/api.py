#! /usr/bin/python3
import os
import sys
import pdb
import grpc
import enum

import batch_pb2_grpc as batch_pb2_grpc
import device_pb2_grpc as device_pb2_grpc
import pcn_pb2_grpc as pcn_pb2_grpc
import subnet_pb2_grpc as subnet_pb2_grpc
import tunnel_pb2_grpc as tunnel_pb2_grpc

import infra.common.defs as defs

from infra.common.glopts  import GlobalOptions
from infra.common.logging import logger

MAX_BATCH_SIZE = 64

class ApiOps(enum.IntEnum):
    NONE = 0
    CREATE = 1
    DELETE = 2
    UPDATE = 3
    RETRIEVE = 4
    START = 5
    COMMIT = 6
    ABORT = 7
    MAX = 8

class ObjectTypes(enum.IntEnum):
    NONE = 0
    BATCH = 1
    SWITCH = 2
    PCN = 3
    SUBNET = 4
    TUNNEL = 5
    MAX = 6

class ClientStub:
    def __init__(self, stubclass, channel, rpc_prefix):
        self.__stub = stubclass(channel)
        self.__rpcs = [None] * ApiOps.MAX
        self.__set_rpcs(rpc_prefix)
        return

    def __set_one_rpc(self, op, rpc):
        self.__rpcs[op] = getattr(self.__stub, rpc, None)
        if not self.__rpcs[op]:
            logger.warn("%s is None for OP: %d" % (rpc, op))
        return

    def __set_rpcs(self, p):
        self.__set_one_rpc(ApiOps.CREATE, "%sCreate"%p)
        self.__set_one_rpc(ApiOps.DELETE, "%sDelete"%p)
        self.__set_one_rpc(ApiOps.UPDATE, "%sUpdate"%p)
        self.__set_one_rpc(ApiOps.RETRIEVE, "%sRetrieve"%p)
        self.__set_one_rpc(ApiOps.START, "%sStart"%p)
        self.__set_one_rpc(ApiOps.COMMIT, "%sCommit"%p)
        self.__set_one_rpc(ApiOps.ABORT, "%sAbort"%p)
        return

    def Rpc(self, op, objs):
        resps = []
        for obj in objs:
            resps.append(self.__rpcs[op](obj))
        return resps

class ApolloAgentClientRequest:
    def __init__(self, stub, req,):
        self.__stub = stub
        return

class ApolloAgentClient:
    def __init__(self):
        self.__channel = None
        self.__stubs = [None] * ObjectTypes.MAX
        self.__connect()
        self.__create_stubs()
        return

    def __get_agent_port(self):
        try:
            port = os.environ['AGENT_GRPC_PORT']
        except:
            port = '9999'
        return port;

    def __get_agent_ip(self):
        try:
            agentip = os.environ['AGENT_GRPC_IP']
        except:
            agentip = 'localhost'
        return agentip
       

    def __connect(self):
        if GlobalOptions.dryrun: return
        endpoint = "%s:%s" % (self.__get_agent_ip(), self.__get_agent_port())
        logger.info("Connecting to Agent %s" % endpoint)
        self.__channel = grpc.insecure_channel(endpoint)
        logger.info("Waiting for Agent to be ready ...")
        grpc.channel_ready_future(self.__channel).result()
        logger.info("Connected to Agent!")
        return

    def __create_stubs(self):
        if GlobalOptions.dryrun: return
        self.__stubs[ObjectTypes.BATCH] = ClientStub(batch_pb2_grpc.BatchSvcStub,
                                                     self.__channel, 'Batch')
        self.__stubs[ObjectTypes.SWITCH] = ClientStub(device_pb2_grpc.DeviceSvcStub,
                                                      self.__channel, 'Device')
        self.__stubs[ObjectTypes.PCN] = ClientStub(pcn_pb2_grpc.PCNSvcStub,
                                                   self.__channel, 'PCN')
        self.__stubs[ObjectTypes.SUBNET] = ClientStub(subnet_pb2_grpc.SubnetSvcStub,
                                                      self.__channel, 'Subnet')
        self.__stubs[ObjectTypes.TUNNEL] = ClientStub(tunnel_pb2_grpc.TunnelSvcStub,
                                                      self.__channel, 'Tunnel')
        return

    def Create(self, objtype, objs):
        if GlobalOptions.dryrun: return
        return self.__stubs[objtype].Rpc(ApiOps.CREATE, objs)

    def Delete(self, objtype, objs):
        if GlobalOptions.dryrun: return
        return self.__stubs[objtype].Rpc(ApiOps.DELETE, objs)

    def Update(self, objtype, objs):
        if GlobalOptions.dryrun: return
        return self.__stubs[objtype].Rpc(ApiOps.UPDATE, objs)

    def Retrieve(self, objtype, objs):
        if GlobalOptions.dryrun: return
        return self.__stubs[objtype].Rpc(ApiOps.RETRIEVE, objs)

    def Start(self, objtype, obj):
        if GlobalOptions.dryrun: return
        return self.__stubs[objtype].Rpc(ApiOps.START, [ obj ])

    def Abort(self, objtype, obj):
        if GlobalOptions.dryrun: return
        return self.__stubs[objtype].Rpc(ApiOps.ABORT, [ obj ])

    def Commit(self, objtype, obj):
        if GlobalOptions.dryrun: return
        return self.__stubs[objtype].Rpc(ApiOps.COMMIT, [ obj ])

client = None
def Init():
    global client
    client = ApolloAgentClient()
