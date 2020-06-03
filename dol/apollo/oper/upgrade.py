#! /usr/bin/python3
import pdb

from infra.common.logging import logger

from apollo.config.resmgr import Resmgr

import apollo.config.agent.api as api
import apollo.config.utils as utils
import apollo.config.objects.base as base

import upgrade_pb2 as upgrade_pb2

class LastUpgradeStatus(base.StatusObjectBase):
    def __init__(self):
        self.Status = upgrade_pb2.UPGRADE_STATUS_OK
        self.StatusMsg = None
        return

    def Update(self, status):
        self.Status = getattr(status, 'Status', upgrade_pb2.UPGRADE_STATUS_OK)
        self.StatusMsg = getattr(status, 'StatusMsg', None)
        return

    def GetLastUpgradeStatus(self):
        logger.info(f"Upgrade: Last upgrade status: {self.Status} {self.StatusMsg}")
        return self.Status

class UpgradeObject(base.ConfigObjectBase):
    def __init__(self, node):
        super().__init__(api.ObjectTypes.UPGRADE, node)
        self.SetSingleton(True)
        self.GID("Upgrade")
        ############## PUBLIC ATTRIBUTES OF Upgrade OBJECT #################
        self.ReqType = upgrade_pb2.UPGRADE_REQUEST_START
        self.UpgMode = upgrade_pb2.UPGRADE_MODE_HITLESS
        self.PkgName = "naples_fw.tar"
        ############## PRIVATE ATTRIBUTES OF Upgrade OBJECT ################
        self.Status = LastUpgradeStatus()
        self.Show()
        return

    def __repr__(self):
        return "Upgrade"

    def Show(self):
        logger.info(f"Upgrade Object: {self}")
        logger.info(f" - Node:{self.Node}")
        logger.info(f" - ObjType:{self.ObjType}")
        logger.info(f" - RequestType:{self.ReqType}")
        logger.info(f" - Mode:{self.UpgMode}")
        logger.info(f" - PackageName:{self.PkgName}")
        return

    def PopulateRequest(self, grpcmsg):
        spec = grpcmsg.Request
        spec.RequestType = self.ReqType
        spec.Mode = self.UpgMode
        spec.PackageName = self.PkgName
        logger.info(f"Upgrade Spec: {spec}")
        logger.info(f" - RequestType:{spec.RequestType}")
        logger.info(f" - Mode:{spec.Mode}")
        logger.info(f" - PackageName:{spec.PackageName}")
        return

    def ValidateResponse(self, resps):
        if utils.IsDryRun(): return None
        for r in resps:
            if not r.Status == upgrade_pb2.UPGRADE_STATUS_OK:
                logger.error(f"Upgrade request failed with {r}")
                continue
            self.Status.Update(r)
        return self.Status.GetLastUpgradeStatus()

    def GetGrpcUpgradeRequestMessage(self):
        grpcmsg = upgrade_pb2.UpgradeRequest()
        self.PopulateRequest(grpcmsg)
        return grpcmsg

    def SetPkgName(self, pkgName):
        self.PkgName = pkgName

    def SetUpgMode(self, upgMode=upgrade_pb2.UPGRADE_MODE_HITLESS):
        self.UpgMode = upgMode

    def UpgradeReq(self, ip=None):
        self.Show()
        msg = self.GetGrpcUpgradeRequestMessage()
        resp = api.upgradeClient[self.Node].Request(self.ObjType, 'UpgRequest', [msg])
        return self.ValidateResponse(resp)

class UpgradeObjectsClient(base.ConfigClientBase):
    def __init__(self):
        super().__init__(api.ObjectTypes.UPGRADE, Resmgr.MAX_UPGRADE)
        self.UpgradeObjs = dict()

    def IsReadSupported(self):
        return False

    def GenerateUpgradeObjects(self, node, ip=None):
        # Initialize upgrade client
        api.UpgradeClientInit(node, ip)
        obj = UpgradeObject(node)
        self.UpgradeObjs[node] = obj
        self.Objs[node].update({obj.GID: obj})

    def GenerateObjects(self, node):
        self.GenerateUpgradeObjects(node)

    def GetUpgradeObject(self, node):
        return self.UpgradeObjs[node]

    def GetLastUpgradeStatus(self, node):
        return self.UpgradeObjs[node].LastUpgradeStatus()

client = UpgradeObjectsClient()
