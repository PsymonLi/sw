#! /usr/bin/python3
import enum
import pdb
import infra.config.base as base
from apollo.config.resmgr import client as ResmgrClient
import apollo.config.utils as utils
import apollo.config.agent.api as api
import batch_pb2 as batch_pb2
import types_pb2 as types_pb2

from infra.common.logging import logger
from apollo.config.store import EzAccessStore

INVALID_BATCH_COOKIE = 0

class BatchObject(base.ConfigObjectBase):
    def __init__(self, node):
        super().__init__()
        self.Node = node
        self.GID('Batch')
        self.epoch = next(ResmgrClient[node].EpochAllocator)
        self.cookie = INVALID_BATCH_COOKIE
        self.commitstatus = types_pb2.API_STATUS_OK
        return

    def __repr__(self):
        return "Batch/epoch:%d/cookie:0x%x" % (self.epoch, self.cookie)

    def GetBatchSpec(self):
        batchspec = batch_pb2.BatchSpec()
        batchspec.epoch = self.epoch
        return batchspec

    def GetInvalidBatchSpec(self):
        batchspec = batch_pb2.BatchSpec()
        batchspec.epoch = 0
        return batchspec

    def GetBatchContext(self):
        batchctx = types_pb2.BatchCtxt()
        batchctx.BatchCookie = self.cookie
        return batchctx

    def GetBatchCookie(self):
        return self.cookie

    def SetBatchCookie(self, batchCookie):
        self.cookie = batchCookie

    def Epoch(self):
        return self.epoch

    def SetNextEpoch(self):
        self.epoch += 1
        return

    def GetBatchCommitStatus(self):
        return self.commitstatus

    def SetBatchCommitStatus(self, status):
        self.commitstatus = status[0].ApiStatus
        return

    def Show(self):
        logger.info("Batch Object:", self)
        logger.info("- %s" % repr(self))
        return

class BatchObjectClient:
    def __init__(self):
        self.__objs = dict()
        # Temporary fix for artemis to generate flows and sessions for the
        # created mappings
        if utils.IsFlowInstallationNeeded():
            self.__commit_for_flows = True
        else:
            self.__commit_for_flows = False
        return

    def GetObjectByKey(self, key):
        return self.__objs.get(key, None)

    def Objects(self):
        return self.__objs.values()

    def GenerateObjects(self, node, topospec):
        obj = BatchObject(node)
        obj.Show()
        self.__objs.update({node: obj})
        return

    def __updateObject(self, node, batchStatus, commitStatus = None):
        if batchStatus is None:
            cookie = INVALID_BATCH_COOKIE
        else:
            cookie = batchStatus[0].BatchContext.BatchCookie
        # logger.info("Setting Batch cookie to ", cookie)
        self.GetObjectByKey(node).SetBatchCookie(cookie)
        if commitStatus:
            self.GetObjectByKey(node).SetBatchCommitStatus(commitStatus)
        return

    def Start(self, node):
        batchObj = self.GetObjectByKey(node)
        batchObj.SetNextEpoch()
        logger.info(f"Starting a batch for config objects with epoch {batchObj.Epoch()}")
        status = api.client[node].Start(api.ObjectTypes.BATCH, batchObj.GetBatchSpec())
        # update batch context
        self.__updateObject(node, status)
        return status

    def Commit(self, node):
        batchObj = self.GetObjectByKey(node)
        if self.__commit_for_flows:
            api.client[node].Start(api.ObjectTypes.BATCH, batchObj.GetInvalidBatchSpec())
            self.__commit_for_flows = False
        logger.info(f"Committing the batch with cookie 0x{batchObj.GetBatchCookie():0x}")
        status = api.client[node].Commit(api.ObjectTypes.BATCH, batchObj.GetBatchContext())
        # invalidate batch context
        self.__updateObject(node, None, status)
        return

client = BatchObjectClient()
EzAccessStore.SetBatchClient(client)
