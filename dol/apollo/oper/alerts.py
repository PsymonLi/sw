#! /usr/bin/python3
import pdb

from infra.common.logging import logger

from apollo.config.resmgr import Resmgr

import apollo.config.agent.api as api
import apollo.config.objects.base as base
import apollo.config.utils as utils

import oper_pb2 as oper_pb2
import types_pb2 as types_pb2

class AlertsObject(base.ConfigObjectBase):
    def __init__(self, node):
        super().__init__(api.PenOperObjectTypes.ALERTS, node)
        self.SetSingleton(True)
        self.GID(f"Alerts-{self.Node}")
        self.Stream = None
        self.Show()
        return

    def __repr__(self):
        return self.GID

    def Show(self):
        logger.info(f"Alerts Object: {self}")
        return

    def GetGrpcAlertsMessage(self):
        grpcmsg = oper_pb2.OperInfoRequest()
        spec = grpcmsg.Request.add()
        spec.InfoType = oper_pb2.OPER_INFO_TYPE_EVENT
        spec.Action = oper_pb2.OPER_INFO_OP_SUBSCRIBE
        return grpcmsg

    def RequestAlertsStream(self):
        def GetOperInfoRequestIterator():
            messages = [self.GetGrpcAlertsMessage()]
            for m in messages:
                yield m

        response = api.penOperClient[self.Node].Request(self.ObjType, 'OperInfoSubscribe', [GetOperInfoRequestIterator()])
        self.Stream = response[0] if response else None
        logger.info(f"received alerts stream {self.Stream} from {self.Node}")

    def GetAlerts(self):
        if not self.Stream:
            return None
        for infoResp in self.Stream:
            assert (infoResp.Status == types_pb2.API_STATUS_OK),\
                   f"Alerts get failed on {self.Node}, rc {infoResp.Status}"
            assert (infoResp.InfoType == oper_pb2.OPER_INFO_TYPE_EVENT),\
                   f"Unexpected oper info {infoResp.InfoType} received on "\
                   f"{self.Node} instead of alerts"
            yield infoResp.EventInfo

    def DrainAlerts(self, spec=None):
        try:
            alerts = self.GetAlerts()
            while True:
                logger.info(f"Draining old alerts, {next(alerts)}")
        except:
            pass
        return True

class AlertsObjectsClient(base.ConfigClientBase):
    def __init__(self):
        super().__init__(api.PenOperObjectTypes.ALERTS, Resmgr.MAX_OPER)
        # one alert object per node
        self.Objs = dict()

    def Objects(self, node):
        return self.Objs.get(node, None)

    def IsReadSupported(self):
        return False

    def GenerateObjects(self, node):
        if utils.IsReconfigInProgress(node):
            return
        obj = AlertsObject(node)
        self.Objs[node] = obj

    def CreateObjects(self, node):
        logger.info(f"Starting {self.ObjType.name} stream from {node}")
        # start alert streams request
        obj = self.Objects(node)
        if not obj:
            return False
        obj.RequestAlertsStream()
        return True

client = AlertsObjectsClient()