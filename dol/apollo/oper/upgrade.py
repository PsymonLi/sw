#! /usr/bin/python3
import pdb

from infra.common.logging import logger

from apollo.config.resmgr import Resmgr
from apollo.config.store import EzAccessStore
from apollo.config.store import client as EzAccessStoreClient
from infra.e2e.ns import run as RunCmd

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
            self.Status.Update(r)
            if not r.Status == upgrade_pb2.UPGRADE_STATUS_OK:
                logger.error(f"Upgrade request failed with {r}")
                continue
        return self.Status.GetLastUpgradeStatus()

    def GetGrpcUpgradeRequestMessage(self):
        grpcmsg = upgrade_pb2.UpgradeRequest()
        self.PopulateRequest(grpcmsg)
        return grpcmsg

    def SetPkgName(self, pkgName):
        self.PkgName = pkgName

    def SetUpgMode(self, upgMode=upgrade_pb2.UPGRADE_MODE_HITLESS):
        self.UpgMode = upgMode

    def UpdateUpgradeMode(self, spec=None):
        if hasattr(spec, "UpgMode"):
            mode = getattr(spec, "UpgMode", "hitless")
            self.UpgMode = utils.GetRpcUpgradeMode(mode)
            self.Show()
        return True

    def UpgradeReq(self, ip=None):
        self.Show()
        if utils.IsDryRun():
            return upgrade_pb2.UPGRADE_STATUS_OK
        msg = self.GetGrpcUpgradeRequestMessage()
        resp = api.upgradeClient[self.Node].Request(self.ObjType, 'UpgRequest', [msg])
        return self.ValidateResponse(resp)

    def ConfigReplayReadyCheck(self):
        if utils.IsDryRun():
            return True
        msg = upgrade_pb2.EmptyMsg()
        resp = api.upgradeClient[self.Node].Request(self.ObjType, 'ConfigReplayReadyCheck', [msg])
        return resp.IsReady

    def ConfigReplayStarted(self):
        if utils.IsDryRun():
            return True
        msg = upgrade_pb2.EmptyMsg()
        api.upgradeClient[self.Node].Request(self.ObjType, 'ConfigReplayStarted', [msg])

    def ConfigReplayDone(self):
        if utils.IsDryRun():
            return
        msg = upgrade_pb2.EmptyMsg()
        api.upgradeClient[self.Node].Request(self.ObjType, 'ConfigReplayDone', [msg])

    def TriggerUpgradeReq(self, spec=None):
        if self.UpgradeReq() != upgrade_pb2.UPGRADE_STATUS_OK:
            logger.error("TriggerUpgradeReq: Failed")
            return False
        logger.info("TriggerUpgradeReq: Success")
        return True

    def VerifyUpgradeStatus(self, spec=None):
        if self.Status.GetLastUpgradeStatus() != upgrade_pb2.UPGRADE_STATUS_OK:
            logger.error("VerifyUpgradeStatus: Failed")
            return False
        logger.info("VerifyUpgradeStatus: Success")
        return True

    def PollConfigReplayReady(self, spec=None):
        retry = getattr(spec, "retry", 10)
        sleep_interval = getattr(spec, "sleep_interval", 0.5)
        while retry:
            logger.info(f"retry{retry}: Polling upgrade manager for ConfigReplay state")
            retry -= 1
            if self.ConfigReplayReadyCheck():
                logger.info("PollConfigReplayReady: Success")
                return True
            if not retry:
                logger.info("PollConfigReplayReady: Failed")
                return False
            utils.Sleep(sleep_interval)
        return False

    def TriggerCfgReplay(self, spec=None):
        from apollo.config.node import client as NodeClient
        logger.info("TriggerCfgReplay: Replaying Config ")
        self.ConfigReplayStarted()
        NodeClient.Create(self.Node)
        self.ConfigReplayDone()
        return True

    def ValidateCfgPostUpgrade(self, spec=None):
        from apollo.config.node import client as NodeClient
        logger.info("Validate Config Post Upgrade")
        NodeClient.Read(self.Node)
        return True

    def SetupCfgFilesForUpgrade(self, spec=None):
        # For hardware nothing to setup specifically
        if not utils.IsDol():
            return True
        mode = "hitless"
        if hasattr(spec, "UpgMode"):
            mode = getattr(spec, "UpgMode", "hitless")
        logger.info("Setup Upgrade Config Files for %s mode"%mode)

        # For now cfg file setup done only for hitless mode
        if mode == "hitless":
            # setup hitless upgrade config files
            upg_setup_cmds = "apollo/test/tools/apulu/setup_hitless_upgrade_cfg_sim.sh"
            if not RunCmd(upg_setup_cmds, timeout=20, background=False):
                logger.error("Command Execution Failed: %s"%upg_setup_cmds)
                return False
        return True

    def SetupTestcaseConfig(self, obj):
        obj.root = self
        return

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
        EzAccessStoreClient[node].SetUpgrade(obj)

    def GenerateObjects(self, node):
        self.GenerateUpgradeObjects(node)

    def GetUpgradeObject(self, node):
        return self.UpgradeObjs[node]

    def GetLastUpgradeStatus(self, node):
        return self.UpgradeObjs[node].LastUpgradeStatus()

client = UpgradeObjectsClient()


def GetMatchingObjects(selectors):
    dutNode = EzAccessStore.GetDUTNode()
    upgradeobjs = [EzAccessStoreClient[dutNode].GetUpgrade()]
    return utils.GetFilteredObjects(upgradeobjs, selectors.maxlimits, False)
