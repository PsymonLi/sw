#! /usr/bin/python3

import pdb
import copy
import inspect
import random

from collections import defaultdict

from infra.common.logging import logger
from infra.common.glopts  import GlobalOptions
import infra.config.base as base

import apollo.config.agent.api as api
import apollo.config.utils as utils
import apollo.config.topo as topo

from apollo.config.store import EzAccessStore
from apollo.config.store import client as EzAccessStoreClient

import types_pb2 as types_pb2

class StatusObjectBase(base.StatusObjectBase):
    def __init__(self, objtype):
        super().__init__()
        self.ObjType = objtype
        self.HwId = -1
        return

    def __repr__(self):
        return f"{self.ObjType.name} Status HwID:{self.HwId}"

    def Show(self):
        logger.info(f"  - {self}")

    def GetHwId(self):
        return self.HwId

    def Update(self, status):
        self.HwId = status.HwId

class StatsObjectBase(base.StatsObjectBase):
    def __init__(self, objtype):
        super().__init__()
        self.ObjType = objtype
        return

    def __repr__(self):
        return f"{self.ObjType.name} Stats"

    def Show(self):
        logger.info(f"  - {self}")
        for stat in dir(self):
            if callable(getattr(self, stat)):
                continue
            if stat.startswith("_"):
                continue;
            if stat == "ObjType":
                continue
            if hasattr(self, stat):
                logger.info("      %s: %d" % (stat, getattr(self, stat)))
            else:
                logger.info(f"      {stat}")
        return

    def Update(self, stats):
        logger.error("Method not implemented by class: %s" % self.__class__)
        assert(0)
        return False

class ConfigObjectBase(base.ConfigObjectBase):
    def __init__(self, objtype, node):
        super().__init__()
        self.BatchUnaware = False
        self.Origin = topo.OriginTypes.FIXED
        self.UUID = None
        # marked HwHabitant when object is in hw
        self.HwHabitant = False
        self.Singleton = False
        self.ObjType = objtype
        self.Parent = None
        self.Children = []
        self.Deps = defaultdict(list)
        self.Precedent = None
        self.Mutable = False
        # marked dirty when object is already in hw, but
        # there are few updates yet to be pushed to hw
        self.Dirty = False
        self.Node = node
        self.NodeObj = getattr(EzAccessStoreClient.get(node), 'NodeObj', None)
        self.Duplicate = None
        self.Interim = False
        self.Tenant = 'default'
        self.Namespace = 'default'
        return

    def __unimplemented(self):
        UnimplementedError = f"'{inspect.stack()[1].function}' method is NOT "\
                             f"implemented by class: {self.__class__}"
        logger.error(UnimplementedError)
        assert (0), UnimplementedError

    def __get_GrpcMsg(self, op):
        grpcreq = api.client[self.Node].GetGRPCMsgReq(self.ObjType, op)
        if grpcreq is None:
            logger.error("GRPC req method not added for obj:%s op:%s" %(self.ObjType, op))
            assert(0)
        return grpcreq()

    def __populate_BatchContext(self, grpcmsg, cookie):
        if getattr(self, "BatchUnaware", False) is False:
            grpcmsg.BatchCtxt.BatchCookie = cookie
        return

    def GetObjectType(self):
        return self.ObjType

    def IsV4(self):
        af = getattr(self, 'AddrFamily', None)
        if af == 'IPV4':
            return True
        return False

    def IsV6(self):
        af = getattr(self, 'AddrFamily', None)
        if af == 'IPV6':
            return True
        return False

    def AddChild(self, child):
        child.Parent = self
        self.Children.append(child)

    def DeleteChild(self, child):
        child.Parent = None
        self.Children.remove(child)

    def GetDependees(self, node):
        # returns the list of dependees
        dependees = []
        return dependees

    def BuildDependency(self):
        dependees = self.GetDependees(self.Node)
        for dependee in dependees:
            if not dependee:
                continue
            # add ourself as an dependent to dependee
            dependee.AddDependent(self)
        return

    def AddDependent(self, dep):
        self.Deps[dep.ObjType].append(dep)

    def DeleteDependent(self, dep):
        if len(self.Deps[dep.ObjType]):
            self.Deps[dep.ObjType].remove(dep)

    def DeriveOperInfo(self):
        self.BuildDependency()
        return

    def SetHwHabitant(self, value):
        self.HwHabitant = value
        if self.HwHabitant == True and self.HasPrecedent() == True:
             self.Precedent.HwHabitant = False

    def IsHwHabitant(self):
        return self.HwHabitant

    def SetSingleton(self, value):
        self.Singleton = value

    def IsSingleton(self):
        return self.Singleton

    def SetDirty(self, value):
        self.Dirty = value

    def IsDirty(self):
        return self.Dirty

    def SetOrigin(self, origin):
        if origin == 'discovered':
            self.Origin = topo.OriginTypes.DISCOVERED
        elif origin == 'implicitly-created':
            self.Origin = topo.OriginTypes.IMPLICITLY_CREATED
            self.SetHwHabitant(True)
        # anything else is FIXED
        return

    def IsOriginFixed(self):
        return True if (self.Origin == topo.OriginTypes.FIXED) else False

    def IsOriginDiscovered(self):
        return True if (self.Origin == topo.OriginTypes.DISCOVERED) else False

    def IsOriginImplicitlyCreated(self):
        return True if (self.Origin == topo.OriginTypes.IMPLICITLY_CREATED) else False

    def HasPrecedent(self):
         return False if (self.Precedent == None) else True

    def GetPrecedent(self):
         return self.Precedent

    def PostCreateCallback(self, spec):
        return True

    def Create(self, spec=None):
        if not utils.CreateObject(self):
            return False
        return self.PostCreateCallback(spec)

    def Read(self, spec=None):
        if self.IsDirty():
            logger.info("Not reading object from Hw since it is marked Dirty")
            return True
        # set the appropriate expected status
        if self.IsHwHabitant():
            expApiStatus = types_pb2.API_STATUS_OK
        else:
            expApiStatus = types_pb2.API_STATUS_NOT_FOUND
        return utils.ReadObject(self, expApiStatus)

    def Cleanup(self):
        return

    def Delete(self, spec=None):
        return utils.DeleteObject(self)

    def Destroy(self):
        if self.IsHwHabitant():
            logger.error(f"Object {self} still exist in HW can not destroy it")
            return False

        self.Show()
        logger.info(f"Destroying object {self} in {self.Node}")
        # remove the link from parent
        if self.Parent:
            self.Parent.DeleteChild(self)

        # destroy all children
        for obj in self.Children:
            logger.info(f"Destroying object {self} children in {self.Node}")
            if not obj.Destroy():
                return False
        return True

    def UpdateNotify(self, dObj):
        return

    def RollbackMany(self, attrlist):
        if self.HasPrecedent():
            for attr in attrlist:
                setattr(self, attr, getattr(self.Precedent, attr))
        return

    def CopyObject(self):
        clone = copy.copy(self)
        for attr in vars(self):
            val = getattr(self, attr)
            # for non config object and list/dict/tuple attributes, make a complete copy
            if isinstance(val, (list, dict, tuple)) and not isinstance(val, ConfigObjectBase):
                setattr(clone, attr, copy.copy(val))
        return clone

    def GetMutableAttributes(self):
        self.__unimplemented()

    def UpdateAttributes(self, spec):
        if spec:
            self.SpecUpdate(spec)
            #self.AddToReconfigState('update')
        else:
            self.AutoUpdate()
        logger.info("Object Updated")
        self.Show()
        return

    # update the python object
    def ObjUpdate(self, spec, clone=True):
        if self.Mutable:
            if spec is None and self.HasPrecedent():
                logger.info("%s object updated already" % self)
            else:
                logger.info("Updating obj %s" % self)
                if clone:
                    clone = self.CopyObject()
                    clone.Precedent = None
                    self.Precedent = clone
                self.UpdateAttributes(spec)
                logger.info("Updated values -")
                self.Show()
        return True

    # push the update to DSC
    def ProcessUpdate(self):
        self.SetDirty(True)
        return self.CommitUpdate()

    def Update(self, spec=None):
        self.ObjUpdate(spec)
        return self.ProcessUpdate()

    def RollbackUpdate(self, spec=None):
        self.PrepareRollbackUpdate(spec)
        self.CommitUpdate(spec)
        return True

    def PrepareRollbackUpdate(self, spec=None):
        if self.HasPrecedent():
            self.RollbackAttributes()
            self.Precedent = None
            self.SetDirty(True)
            logger.info("Object rolled back to -")
            self.Show()
            return True
        logger.info(f"{self} has no precedent to roll back to")
        return True

    def CommitUpdate(self, spec=None):
        if not self.IsDirty():
            logger.info("No changes on object %s to commit" % self)
        self.SetDirty(False)
        return utils.UpdateObject(self)

    def ValidateJSONSpec(self, spec):
        self.__unimplemented()
        return False

    def GetPdsSpecScalarAttrs(self):
        """
        # returns a list of scalar attributes to be validated on read
        # NOTE: returning an empty list would validate all attributes
        """
        return []

    def GetPdsSpec(self):
        grpcmsg = self.GetGrpcCreateMessage()
        if self.IsSingleton():
            return grpcmsg.Request
        return grpcmsg.Request[0]

    def ValidatePdsSpecScalarAttrs(self, objSpec, spec):
        scalarAttrs = self.GetPdsSpecScalarAttrs()
        mismatchingAttrs = utils.CompareSpec(objSpec, spec, scalarAttrs)
        return mismatchingAttrs

    def ValidatePdsSpecCompositeAttrs(self, objSpec, spec):
        return []

    def ValidateSpec(self, spec):
        objSpec = self.GetPdsSpec()
        mismatchingAttrs = self.ValidatePdsSpecScalarAttrs(objSpec, spec) +\
                           self.ValidatePdsSpecCompositeAttrs(objSpec, spec)
        if len(mismatchingAttrs) != 0:
            logger.error(f"{self} validation failed for {mismatchingAttrs}")
            logger.error(f"Obj spec for {self} - {objSpec}")
            logger.error(f"Pds spec for {self} - {spec}")
            return False
        return True

    def ValidateStats(self, stats):
        return True

    def ValidateStatus(self, status):
        return True

    def ValidateYamlSpec(self, spec):
        self.__unimplemented()
        return False

    def ValidateYamlStats(self, stats):
        return True

    def ValidateYamlStatus(self, status):
        return True

    def Equals(self, obj, spec):
        return True

    def MarkDeleted(self, flag=True):
        self.deleted = flag
        return

    def IsDeleted(self):
        return self.deleted

    def IsInterim(self):
        return self.Interim

    def SetBaseClassAttr(self):
        self.__unimplemented()
        return

    def GetKey(self):
        return self.UUID.GetUuid()

    def PopulateKey(self, grpcmsg):
        self.__unimplemented()
        return

    def PopulateSpec(self, grpcmsg):
        self.__unimplemented()
        return

    def PopulateAgentJson(self):
        self.__unimplemented()
        return

    def GetRESTPath(self):
        return "%s/%s/%s" % (self.Tenant, self.Namespace, self.GID())

    def GetGrpcCreateMessage(self, cookie=0):
        grpcmsg = self.__get_GrpcMsg(api.ApiOps.CREATE)
        self.__populate_BatchContext(grpcmsg, cookie)
        self.PopulateSpec(grpcmsg)
        return grpcmsg

    def GetGrpcReadMessage(self):
        grpcmsg = self.__get_GrpcMsg(api.ApiOps.GET)
        self.PopulateKey(grpcmsg)
        return grpcmsg

    def GetGrpcUpdateMessage(self, cookie=0):
        grpcmsg = self.__get_GrpcMsg(api.ApiOps.UPDATE)
        self.__populate_BatchContext(grpcmsg, cookie)
        self.PopulateSpec(grpcmsg)
        return grpcmsg

    def GetGrpcDeleteMessage(self, cookie=0):
        grpcmsg = self.__get_GrpcMsg(api.ApiOps.DELETE)
        self.__populate_BatchContext(grpcmsg, cookie)
        self.PopulateKey(grpcmsg)
        return grpcmsg
    
    def AddToReconfigState(self, op):
        if utils.IsReconfigInProgress(self.Node):
            if op == 'create':
                self.NodeObj.ReconfigState.Created.append(self)
            elif op == 'update':
                self.NodeObj.ReconfigState.Updated.append(self)
            else:
                assert(0)
        return

    def IsReconfigInProgress(self):
        if self.NodeObj.ReconfigState.InProgress:
            return True
        return False

class ConfigClientBase(base.ConfigClientBase):
    def __init__(self, objtype, maxlimit=0):
        super().__init__()
        self.Objs = defaultdict(dict)
        self.ObjType = objtype
        self.Maxlimit = maxlimit
        self.args = None
        self.ReconfigInProgress = defaultdict(bool)
        return

    def IsReadSupported(self):
        return True

    def IsValidConfig(self, node):
        count = self.GetNumObjects(node)
        if count > self.Maxlimit:
            return False, "%s count %d exceeds allowed limit of %d" % \
                          (self.ObjType, count, self.Maxlimit)
        logger.info(f"Generated {count} {self.ObjType.name} Objects in {node}")
        return True, ""

    def GetKeyfromSpec(self, spec, yaml=False):
        uuid = spec['id'] if yaml else spec.Id
        return utils.PdsUuid.GetIdfromUUID(uuid)

    def Objects(self, node, fixed_only=False):
        objlist = []
        if self.Objs.get(node, None):
            objlist = self.Objs[node].values()
        if not fixed_only:
            return objlist
        flist = [obj for obj in objlist if obj.IsOriginFixed()]
        return flist

    def GetNumHwObjects(self, node):
        count = len(self.Objects(node))
        # TODO can be improved, if object has a reference to gen object
        for obj in self.Objects(node):
            if (obj.HwHabitant == False):
                count = count - 1
        logger.info(f"GetNumHwObjects returned {count} for {self.ObjType.name} in {node}")
        return count

    def GetNumObjects(self, node):
        return len(self.Objects(node))

    def GetObjectByKey(self, node, key):
        return self.Objs[node].get(key, None)

    def GetObjectsByKeys(self, node, keys, filterfn=None):
        return list(filter(filterfn, map(lambda key: self.GetObjectByKey(node, key), keys)))

    def GetObjectType(self):
        return self.ObjType

    def GetPdsctlObjectName(self):
        return self.ObjType.name.lower()

    def ShowObjects(self, node):
        for obj in self.Objects(node):
            obj.Show()
        return

    def __get_GrpcMsg(self, node, op):
        grpcreq = api.client[node].GetGRPCMsgReq(self.ObjType, op)
        if grpcreq is None:
            logger.error("GRPC req method not added for obj:%s op:%s" %(self.ObjType, op))
            assert(0)
        return grpcreq()

    def GetGrpcReadAllMessage(self, node):
        grpcmsg = self.__get_GrpcMsg(node, api.ApiOps.GET)
        return grpcmsg

    def IsGrpcSpecMatching(self, spec):
        return True

    def ValidateGrpcRead(self, node, getResp):
        if utils.IsDryRun(): return True
        numObjs = 0
        for obj in getResp:
            if not utils.ValidateGrpcResponse(obj):
                logger.error("GRPC get request failed for ", obj)
                continue
            #TODO handle singleton object
            resps = obj.Response
            for resp in resps:
                if not self.IsGrpcSpecMatching(resp.Spec):
                    continue
                numObjs += 1
                key = self.GetKeyfromSpec(resp.Spec)
                cfgObj = self.GetObjectByKey(node, key)
                if not utils.ValidateObject(cfgObj, resp):
                    logger.error("GRPC read validation failed for ", obj)
                    if cfgObj:
                        cfgObj.Show()
                    logger.info(f"Key:{key} Spec:{resp.Spec}")
                    return False
                if hasattr(cfgObj, 'Status'):
                    cfgObj.Status.Update(resp.Status)
        logger.info(f"GRPC read count {numObjs} for {self.ObjType.name} in {node}")
        return (numObjs == self.GetNumHwObjects(node))

    def GrpcRead(self, node):
        # read all via grpc
        msg = self.GetGrpcReadAllMessage(node)
        resp = api.client[node].Get(self.ObjType, [msg])
        if not self.ValidateGrpcRead(node, resp):
            logger.critical("Object validation failed for %s" % (self.ObjType))
            return False
        return True

    def ValidatePdsctlRead(self, node, ret, stdout):
        if utils.IsDryRun(): return True
        if not ret:
            logger.error("pdsctl show cmd failed for ", self.ObjType)
            return False
        # split output per object
        cmdop = stdout.split("---")
        assert((len(cmdop) - 1) == self.GetNumHwObjects(node))
        for op in cmdop:
            yamlOp = utils.LoadYaml(op)
            if not yamlOp:
                continue
            key = self.GetKeyfromSpec(yamlOp['spec'], yaml=True)
            cfgObj = self.GetObjectByKey(node, key)
            if not utils.ValidateObject(cfgObj, yamlOp, yaml=True):
                logger.error("pdsctl read validation failed for ", op)
                cfgObj.Show()
                return False
        return True

    def PdsctlRead(self, node):
        # read all via pdsctl
        # TODO: unify pdsctl code & get rid of this import
        if utils.IsDol():
            import apollo.test.utils.pdsctl as pdsctl
        else:
            import iota.test.apulu.utils.pdsctl as pdsctl
        ret, op = pdsctl.GetObjects(node, self.GetPdsctlObjectName(), self.args)
        if not self.ValidatePdsctlRead(node, ret, op):
            logger.critical("Object validation failed for ", self.ObjType, ret, op)
            return False
        return True

    def __getObjectByUUID(self, node, uuid):
        objlist = self.Objects(node)
        for obj in objlist:
            if uuid == obj.UUID.UuidStr:
                return obj
        return None

    def ValidateJSON(self, node, resps):
        for j in resps:
            if hasattr(j['meta'], 'uuid'):
                uuid = j['meta']['uuid']
                obj = self.__getObjectByUUID(node, uuid)
                if not obj:
                    logger.error(f'Failed to find {self.ObjType} object with \
                                 uuid {uuid} on node {node}')
                return False
                if not obj.ValidateJSONSpec(j):
                    return False
            else:
                logger.error("Found an object without uuid - ", j)
        return True

    def ValidateHttpRead(self, node, resps):
        return self.ValidateJSON(node, resps)

    def HttpRead(self, node):
        if not GlobalOptions.netagent:
            return True
        resp = api.client[node].GetHttp(self.ObjType)
        logger.info("HTTP read:", resp)
        if resp and not self.ValidateHttpRead(node, resp):
            logger.error("Http Read validation failed for ", self.ObjType)
            return False
        return True

    def ReadObjects(self, node):
        logger.info(f"Reading {self.ObjType.name} Objects from {node}")
        if GlobalOptions.dryrun:
            return True
        if not self.GrpcRead(node):
            return False
        if not self.PdsctlRead(node):
            return False
        if not self.HttpRead(node):
            return False
        logger.info(f"Read & Validated {self.ObjType.name} Objects from {node}")
        return True

    #TODO: cleanup APIs & deprecate
    def CreateObjects(self, node):
        fixed, discovered, implicitly_created = [], [], []
        for obj in self.Objects(node):
            if obj.IsOriginImplicitlyCreated():
                implicitly_created.append(obj)
            elif obj.IsOriginFixed():
                fixed.append(obj)
            elif obj.IsOriginDiscovered():
                discovered.append(obj)
            else:
                assert(0)

        logger.info("%s objects: fixed: %d discovered %d implicity_created %d" \
                %(self.ObjType.name, len(fixed), len(discovered),\
                len(implicitly_created)))
        # return if there is no fixed object
        if len(fixed) == 0 and len(implicitly_created) == 0:
            logger.info(f"Skip Creating {self.ObjType.name} Objects in {node}")
            return

        self.ShowObjects(node)
        logger.info(f"Creating {len(fixed)} {self.ObjType.name} Objects in {node}")
        if GlobalOptions.netagent:
            api.client[node].Create(self.ObjType, fixed)
            #api.client[node].Update(self.ObjType, implicitly_created)
        else:
            cookie = utils.GetBatchCookie(node)
            msgs = list(map(lambda x: x.GetGrpcCreateMessage(cookie), fixed))
            api.client[node].Create(self.ObjType, msgs)
            #TODO: Add validation for create & based on that set HW habitant
        list(map(lambda x: x.SetHwHabitant(True), fixed))
        return True

    def DeleteObjects(self, node):
        cfgObjects = self.Objs[node].copy().values()
        fixed, discovered, implicitly_created = [], [], []
        for obj in cfgObjects:
            if obj.IsOriginImplicitlyCreated():
                implicitly_created.append(obj)
            elif obj.IsOriginFixed():
                fixed.append(obj)
            elif obj.IsOriginDiscovered():
                discovered.append(obj)
            else:
                assert(0)

        logger.info("%s objects: fixed: %d discovered %d implicity_created %d" \
                %(self.ObjType.name, len(fixed), len(discovered),\
                len(implicitly_created)))
        if len(fixed) == 0 and len(implicitly_created) == 0:
            logger.info(f"Skip Deleting {self.ObjType.name} Objects in {node}")
            return

        logger.info(f"Deleting {len(fixed)} {self.ObjType.name} Objects in {node}")
        result = list(map(lambda x: x.Delete(), fixed))
        if not all(result):
            logger.info(f"Deleting {len(fixed)} {self.ObjType.name} Objects FAILED in {node}")
            return False
        return True

    def RestoreObjects(self, node):
        temp = self.Objs[node].copy().values()
        cfgObjects = list(filter(lambda x: not(x.IsInterim()), temp))
        logger.info(f"Restoring {len(cfgObjects)} {self.ObjType.name} Objects in {node}")
        result = list(map(lambda x: x.Create(), cfgObjects))
        if not all(result):
            logger.info(f"Restoring {len(cfgObjects)} {self.ObjType.name} Objects FAILED in {node}")
            return False
        return True

    def UpdateObjects(self, node):
        cfgObjects = self.Objects(node)
        logger.info(f"Updating {len(cfgObjects)} {self.ObjType.name} Objects in {node}")
        result = list(map(lambda x: x.Update(), cfgObjects))
        if not all(result):
            logger.info(f"Updating {len(cfgObjects)} {self.ObjType.name} Objects FAILED in {node}")
            return False
        return True

    def RollbackUpdateObjects(self, node):
        cfgObjects = self.Objects(node)
        logger.info(f"RollbackUpdate {len(cfgObjects)} {self.ObjType.name} Objects in {node}")
        result = list(map(lambda x: x.RollbackUpdate(), cfgObjects))
        if not all(result):
            logger.info(f"RollbackUpdate {len(cfgObjects)} {self.ObjType.name} Objects FAILED in {node}")
            return False
        return True

    def OperateObjects(self, node, oper, cfgObjects=None):
        if not cfgObjects:
            cfgObjects = self.Objects(node)
        logger.info(f"{oper} {len(cfgObjects)} {self.ObjType.name} Objects in {node}")
        result = list(map(lambda x: getattr(x, oper)(), cfgObjects))
        if not all(result):
            logger.info(f"{oper} {len(cfgObjects)} {self.ObjType.name} Objects FAILED in {node}")
            return False
        return True

    def Reconfig(self, node, spec):
        if not spec:
            return
        id = getattr(spec, 'id', None)
        if id:
            obj = self.GetObjectByKey(node, id)
            obj.Update(spec)
            #obj.AddToReconfigState('update')
        else:
            for obj in self.Objects(node):
                obj.Update(spec)
                #obj.AddToReconfigState('update')
        return
    
class MappingObjectBase(ConfigObjectBase):
    def __init__(self, objtype, node):
        super().__init__(objtype, node)
        self.Tags = []
        self.TagBase = None
        self.MaxTags = 5

    def RemoveTag(self, spec=None):
        tag = getattr(spec, "tag", None) if spec else None
        if tag:
            self.Tags.remove(tag)
        elif self.Tags:
            tag = self.Tags.pop()
        else:
            logger.error(f"{self} Tags empty")
            return None
        logger.info(f"{self}: Removed the Tag {tag}")
        return tag if utils.UpdateObject(self) else None

    def AppendTag(self, spec=None):
        tag = getattr(spec, "tag", None) if spec else None
        if len(self.Tags) >= self.MaxTags:
            logger.error(f"{self} Tags full")
            return None
        if tag:
            pass
        elif self.Tags:
            tag = self.Tags[-1]+1
        else:
            tag = self.TagBase if self.TagBase else self.GetNextTag()
        self.Tags.append(tag)
        logger.info(f"{self}: Appended the Tag {tag}")
        return tag if utils.UpdateObject(self) else None

    def ReplaceTag(self, spec):
        tags = getattr(spec, "tags", None)
        if not tags:
            return None
        old_list = self.Tags
        self.Tags = tags
        logger.info(f"{self}: Replaced the Tag {tags}")
        return old_list if utils.UpdateObject(self) else None

    def GenerateTags(self, tags_type, no_of_tags, tags=None):
        def __unique_tags():
            assert(no_of_tags<=self.MaxTags)
            flag = True
            for i in range(no_of_tags):
                tag = self.GetNextTag()
                if flag:
                    self.TagBase = tag
                    flag = False
                self.Tags.append(tag)

        def __overlapping_tags():
            assert(no_of_tags<=self.MaxTags)
            self.TagBase = self.GetNextTag()
            self.Tags = list(range(self.TagBase, self.TagBase + no_of_tags))

        def __specific_tags():
            assert(len(tags)<=self.MaxTags)
            self.Tags = tags

        def __random_tags():
            assert(no_of_tags<=self.MaxTags)
            r = range(1,0xffffffff)
            for i in range(no_of_tags):
                self.Tags.append(random.choice(r))

        def __default_tags():
            assert(0)

        tags_switcher = {
            "overlapping" : __overlapping_tags,
            "unique"      : __unique_tags,
            "specific"    : __specific_tags,
            "random"      : __random_tags,
        }
        return tags_switcher.get(tags_type, __default_tags)()

class MappingClientBase(ConfigClientBase):
    def __init__(self, objtype, maxlimit=0):
        super().__init__(objtype, maxlimit)
        self.v6tags = defaultdict(dict)
        self.v4tags = defaultdict(dict)
        return

    def GetTags(self, node, af="v4"):
        return self.v4tags[node] if af == "v4" else self.v6tags[node]

    def PopulateTagCache(self, obj, tags=None):
        node = obj.Node
        tag_dict = self.v4tags[node] if obj.AddrFamily == "IPV4" else self.v6tags[node]
        tags = tags if tags else obj.Tags
        for tag in tags:
            tag_dict.setdefault(tag, []).append(obj)
        return True

    def DelTagCache(self, obj, tags=None):
        node = obj.Node
        tag_dict = self.v4tags[node] if obj.AddrFamily == "IPV4" else self.v6tags[node]
        tags = tags if tags else obj.Tags
        for tag in tags:
            tag_list = tag_dict.get(tag, None)
            if not tag_list or obj not in tag_list:
                logger.error(f"Cound not find the {obj} in Tag List")
                return False
            tag_list.remove(obj)
            if not tag_list:
                del tag_dict[tag]
        return True

    def ShowTagCache(self, node, af="v4"):
        tag_dict = self.v4tags[node] if af == "v4" else self.v6tags[node]
        for tag, prefixes in tag_dict.items():
            logger.info(f"{self.GetObjectType().name}{af} and value {tag} {prefixes}")
