#! /usr/bin/python3
import pdb
import copy
import enum
import ipaddress
import random
import json
import requests
from collections import defaultdict

from infra.common.logging import logger

from apollo.config.store import EzAccessStore
from apollo.config.store import client as EzAccessStoreClient
from infra.common.glopts  import GlobalOptions

import apollo.config.agent.api as api
from apollo.config.resmgr import client as ResmgrClient
from apollo.config.resmgr import Resmgr
from apollo.config.objects.security_profile import client as SecurityProfileClient
from apollo.config.objects.lmapping import client as LmappingClient
from apollo.config.objects.rmapping import client as RmappingClient

import apollo.config.objects.base as base
import apollo.config.objects.lmapping as lmapping
import apollo.config.objects.tag as tag
import apollo.config.utils as utils
import apollo.config.topo as topo

import policy_pb2 as policy_pb2
import types_pb2 as types_pb2

class RulePriority(enum.IntEnum):
    MIN = 0
    MAX = 1022

class SupportedIPProtos(enum.IntEnum):
    ICMP = 1
    TCP = 6
    UDP = 17

class L4MatchObject:
    def __init__(self, valid=False,\
                 sportlow=None,sporthigh=None,dportlow=None,dporthigh=None,\
                 icmptype=utils.ICMPTYPE_MIN, icmpcode=utils.ICMPCODE_MIN):
        self.valid = valid
        self.SportLow = sportlow
        self.SportHigh = sporthigh
        self.DportLow = dportlow
        self.DportHigh = dporthigh
        if icmptype == 'any':
            self.IcmpType = types_pb2.MATCH_ANY
        else:
            self.IcmpType = icmptype
        if icmpcode == 'any':
            self.IcmpCode = types_pb2.MATCH_ANY
        else:
            self.IcmpCode = icmpcode

    def Show(self):
        if self.valid:
            if self.SportLow != None and self.SportHigh != None:
                logger.info("    SrcPortRange:%d - %d"\
                            %(self.SportLow, self.SportHigh))
            if self.DportLow != None and self.DportHigh != None:
                logger.info("    DstPortRange:%d - %d"\
                            %(self.DportLow, self.DportHigh))
            logger.info("    Icmp Type:%d Code:%d"\
                        %(self.IcmpType, self.IcmpCode))
        else:
            logger.info("    No L4Match")

    def ValidateSpec(self, spec=None):
        default = lambda x,y: True
        def __validate_ports(l4match, spec):
            if spec.Ports.SrcPortRange.PortLow != l4match.SportLow or \
               spec.Ports.SrcPortRange.PortHigh != l4match.SportHigh:
                return False
            if spec.Ports.DstPortRange.PortLow != l4match.DportLow or \
               spec.Ports.DstPortRange.PortHigh != l4match.DportHigh:
                return False
            return True

        def __validate_type_code(l4match, spec):
            def __validate_type_num(l4match, spec):
                return False if spec.TypeCode.TypeNum != l4match.IcmpType else True

            def __validate_type_wildcard(l4match, spec):
                return False if spec.TypeCode.TypeWildcard != l4match.IcmpType else True

            def __validate_code_num(l4match, spec):
                return False if spec.TypeCode.CodeNum != l4match.IcmpCode else True

            def __validate_code_wildcard(l4match, spec):
                return False if spec.TypeCode.CodeWildcard != l4match.IcmpCode else True

            typematch_switcher = {
                "TypeNum"      : __validate_type_num,
                "TypeWildcard" : __validate_type_wildcard
            }
            codematch_switcher = {
                "CodeNum"      : __validate_code_num,
                "CodeWildcard" : __validate_code_wildcard
            }
            if not typematch_switcher.get(spec.TypeCode.WhichOneof("typematch"), default)(l4match, spec):
                return False
            if not codematch_switcher.get(spec.TypeCode.WhichOneof("codematch"), default)(l4match, spec):
                return False
            return True

        l4info_switcher = {
            "Ports"    : __validate_ports,
            "TypeCode" : __validate_type_code
        }
        return l4info_switcher.get(spec.WhichOneof("l4info"), default)(self, spec)

class L3MatchObject:
    def __init__(self, valid=False, proto=utils.L3PROTO_MIN,\
                 srcpfx=None, dstpfx=None,\
                 srciplow=None, srciphigh=None, dstiplow=None,\
                 dstiphigh=None, srctag=None, dsttag=None,\
                 srctype=topo.L3MatchType.PFX, dsttype=topo.L3MatchType.PFX):
        self.valid = valid
        self.Proto = proto
        self.SrcType = srctype
        self.DstType = dsttype
        self.SrcPrefix = srcpfx
        self.DstPrefix = dstpfx
        if srciplow is None and srcpfx:
            pfx = ipaddress.ip_network(srcpfx, False)
            self.SrcIPLow = pfx.network_address
            self.SrcIPHigh = pfx.network_address + (pfx.num_addresses - 1)
        else:
            self.SrcIPLow = srciplow
            self.SrcIPHigh = srciphigh
        if dstiplow is None and dstpfx:
            pfx = ipaddress.ip_network(dstpfx, False)
            self.DstIPLow = pfx.network_address
            self.DstIPHigh = pfx.network_address + (pfx.num_addresses - 1)
        else:
            self.DstIPLow = dstiplow
            self.DstIPHigh = dstiphigh
        self.SrcTag = srctag
        self.DstTag = dsttag

    def Show(self):
        def __get_tag_id(tagObj):
            return tagObj.TagId if tagObj else None

        if self.valid:
            logger.info("    Proto:%s(%d)"\
                        %(utils.GetIPProtoName(self.Proto), self.Proto))
            logger.info("    SrcType:%s DstType:%s"\
                        %(self.SrcType, self.DstType))
            logger.info("    SrcPrefix:%s DstPrefix:%s"\
                        %(self.SrcPrefix, self.DstPrefix))
            logger.info("    SrcIPLow:%s SrcIPHigh:%s"\
                        %(self.SrcIPLow, self.SrcIPHigh))
            logger.info("    DstIPLow:%s DstIPHigh:%s"\
                        %(self.DstIPLow, self.DstIPHigh))
            if utils.IsPipelineArtemis():
                logger.info("    SrcTag:%s DstTag:%s"\
                            %(__get_tag_id(self.SrcTag), \
                            __get_tag_id(self.DstTag)))
            elif utils.IsPipelineApulu():
                logger.info("    SrcTag:%s DstTag:%s" \
                            %(self.SrcTag, self.DstTag))
        else:
            logger.info("    No L3Match")

    def ValidateSpec(self, spec):
        def __validate_src_range(l3match, spec):
            srcrange = types_pb2.AddressRange()
            utils.GetRpcIPRange(l3match.SrcIPLow, l3match.SrcIPHigh, srcrange)
            return False if spec.SrcRange != srcrange else True

        def __validate_src_prefix(l3match, spec):
            srcprefix = types_pb2.IPPrefix()
            utils.GetRpcIPPrefix(l3match.SrcPrefix, srcprefix)
            return False if spec.SrcPrefix != srcprefix else True

        def __validate_src_tag(l3match, spec):
            if utils.IsPipelineArtemis():
                return False if spec.SrcTag != l3match.SrcTag.TagId else True
            elif utils.IsPipelineApulu():
                return False if spec.SrcTag != l3match.SrcTag else True

        def __validate_dst_range(l3match, spec):
            dstrange = types_pb2.AddressRange()
            utils.GetRpcIPRange(l3match.DstIPLow, l3match.DstIPHigh, dstrange)
            return False if spec.DstRange != dstrange else True

        def __validate_dst_prefix(l3match, spec):
            dstprefix = types_pb2.IPPrefix()
            utils.GetRpcIPPrefix(l3match.DstPrefix, dstprefix)
            return False if spec.DstPrefix != dstprefix else True

        def __validate_dst_tag(l3match, spec):
            if utils.IsPipelineArtemis():
                return False if spec.DstTag != l3match.DstTag.TagId else True
            elif utils.IsPipelineApulu():
                return False if spec.DstTag != l3match.DstTag else True

        def __validate_proto_num(l3match, spec):
            return False if spec.ProtoNum != self.Proto else True

        def __validate_proto_wildcard(l3match, spec):
            # TODO : This doesnt look correct
            return False if spec.ProtoWildcard != self.Proto else True

        protomatch_switcher = {
            "ProtoNum"      : __validate_proto_num,
            "ProtoWildcard" : __validate_proto_wildcard
        }
        srcmatch_switcher = {
            "SrcPrefix"     : __validate_src_prefix,
            "SrcRange"      : __validate_src_range,
            "SrcTag"        : __validate_src_tag
        }
        dstmatch_switcher = {
            "DstPrefix"     : __validate_dst_prefix,
            "DstRange"      : __validate_dst_range,
            "DstTag"        : __validate_dst_tag
        }
        default = lambda x,y: True

        if not protomatch_switcher.get(spec.WhichOneof("protomatch"), default)(self, spec):
            return False
        if not srcmatch_switcher.get(spec.WhichOneof("srcmatch"), default)(self, spec):
            return False
        if not dstmatch_switcher.get(spec.WhichOneof("dstmatch"), default)(self, spec):
            return False
        return True

AppIdx=1

class RuleObject:
    def __init__(self, l3match, l4match, priority=0, action=types_pb2.SECURITY_RULE_ACTION_ALLOW, stateful=False):
        self.Stateful = stateful
        self.L3Match = copy.deepcopy(l3match)
        self.L4Match = copy.deepcopy(l4match)
        self.Priority = priority
        self.Action = action
        self.AppName = ''

    def Show(self):
        def __get_action_str(action):
            if action == types_pb2.SECURITY_RULE_ACTION_ALLOW:
                return "allow"
            return "deny"

        logger.info(" -- Stateful:%s Priority:%d Action:%s"\
                    %(self.Stateful, self.Priority, __get_action_str(self.Action)))
        self.L3Match.Show()
        self.L4Match.Show()

    def CreateAppObject(self, node):
        global AppIdx
        appname = f"App{AppIdx}"
        appspec = {
            "kind": "App",
            "meta": {
                "name": appname,
                "namespace": "default",
                "tenant": "default",
                "uuid" : "00000001-00FF-%04X-4242-022222111111"%AppIdx,
            },
            "spec": {
                "alg": {
                    "icmp": {
                        "type": self.L4Match.IcmpType,
                        "code": self.L4Match.IcmpCode
                    }
                }
            }
        }
        AppIdx += 1
        logger.info("PostData: %s"%json.dumps(appspec))
        # TODO: Fix netagent client APIs
        url=f"{api.client[node].GetRestURL()}/api/apps/"
        logger.info(f"Obj:{appname} Posting to {url}")
        if not utils.IsDryRun():
            requests.post(url, json.dumps(appspec), verify=False)
        return appname

    def GetJson(self, node):
        ret = {}
        if self.Action == types_pb2.SECURITY_RULE_ACTION_ALLOW:
            ret['action'] = 'PERMIT'
        else:
            ret['action'] = 'DENY'
        ret['source'] = {}
        ret['destination'] = {}
        if self.L4Match.SportLow != None and self.L4Match.SportHigh != None:
            ret['source']['proto-ports']: [{
                "protocol": utils.GetIPProtoName(self.L3Match.Proto),
                "port": f"{self.L4Match.SportLow}-{self.L4Match.SportHigh}"
                }]
        else:
            if len(self.AppName) == 0:
                self.AppName = self.CreateAppObject(node)
            ret['app-name'] = self.AppName

        if self.L3Match.SrcIPLow and self.L3Match.SrcIPHigh:
            ret['source']['addresses'] = [
                    f"{self.L3Match.SrcIPLow}-{self.L3Match.SrcIPHigh}"
                    ]
        elif self.L3Match.SrcIPLow:
            ret['source']['addresses'] = [ f"{self.L3Match.SrcIPLow}" ]
        if self.L4Match.DportLow != None and self.L4Match.DportHigh != None:
            ret['destination']["proto-ports"] = [{
                "protocol": utils.GetIPProtoName(self.L3Match.Proto),
                "port": f"{self.L4Match.DportLow}-{self.L4Match.DportHigh}"
                }]
        if self.L3Match.DstIPLow and self.L3Match.DstIPHigh:
            ret['destination']['addresses'] = [
                    "%s-%s"%(self.L3Match.DstIPLow, self.L3Match.DstIPHigh)
                    ]
        elif self.L3Match.DstIPLow:
            ret['destination']['addresses'] = [ f"{self.L3Match.DstIPLow}" ]

        return ret

    def ValidateSpec(self, spec):
        if spec.Attrs.Stateful != self.Stateful:
            return False
        if spec.Attrs.Priority != self.Priority:
            return False
        if spec.Attrs.Action != self.Action:
            return False

        if spec.Attrs.Match.L3Match and not self.L3Match:
            return False
        if self.L3Match and not self.L3Match.ValidateSpec(spec.Attrs.Match.L3Match):
            return False

        if spec.Attrs.Match.L4Match and not self.L4Match:
            return False
        if self.L4Match and not self.L4Match.ValidateSpec(spec.Attrs.Match.L4Match):
            return False

        return True


class PolicyObject(base.ConfigObjectBase):
    def __init__(self, node, vpcid, af, direction, rules, policytype, overlaptype, level='subnet',
                 defaultfwaction='allow', fwdmode=None):

        super().__init__(api.ObjectTypes.POLICY, node)
        ################# PUBLIC ATTRIBUTES OF POLICY OBJECT #####################
        self.VPCId = vpcid
        self.Direction = direction
        self.FwdMode = fwdmode
        if af == utils.IP_VERSION_6:
            self.PolicyId = next(ResmgrClient[node].V6SecurityPolicyIdAllocator)
            self.AddrFamily = 'IPV6'
        else:
            self.PolicyId = next(ResmgrClient[node].V4SecurityPolicyIdAllocator)
            self.AddrFamily = 'IPV4'
        self.GID('Policy%d'%self.PolicyId)
        self.UUID = utils.PdsUuid(self.PolicyId, self.ObjType)
        ################# PRIVATE ATTRIBUTES OF POLICY OBJECT #####################
        self.PolicyType = policytype
        self.Level = level
        self.OverlapType = overlaptype
        self.rules = copy.deepcopy(rules)
        self.DeriveOperInfo()
        self.Mutable = True if (utils.IsUpdateSupported() and self.IsOriginFixed()) else False
        self.DefaultFWAction = defaultfwaction
        self.Show()
        return

    def __repr__(self):
        return "Policy: %s " % (self.UUID)

    def Show(self):
        logger.info("Policy Object:", self)
        logger.info("- %s" % repr(self))
        logger.info("- Level:%s" % self.Level)
        logger.info("- Vpc%d" % self.VPCId)
        logger.info("- Direction:%s|AF:%s" % (self.Direction, self.AddrFamily))
        logger.info("- PolicyType:%s" % self.PolicyType)
        logger.info("- OverlapType:%s" % self.OverlapType)
        logger.info("- DefaultFwAction:%s" % self.DefaultFWAction)
        logger.info("- Number of rules:%d" % len(self.rules))
        for rule in self.rules:
            rule.Show()
        return

    def IsFilterMatch(self, selectors):
        return super().IsFilterMatch(selectors.policy.filters)

    def CopyObject(self):
        clone = copy.copy(self)
        clone.rules = self.rules
        self.rules = copy.deepcopy(clone.rules)
        return clone

    def GetMutableAttributes(self):
        return ['DefaultFWAction', 'Action']

    def GetUpdateAttributes(self, spec=None):
        if spec is None:
            return self.GetMutableAttributes()
        updateAttrList = []
        if getattr(spec, 'Action', None):
            updateAttrList.append('Action')
        if getattr(spec, 'DefaultFWAction', None):
            updateAttrList.append('DefaultFWAction')
        return updateAttrList

    def ChangeDefaultFWAction(self, spec):
        if spec is not None:
            self.DefaultFWAction = spec.DefaultFWAction
            return
        self.DefaultFWAction = 'deny' if self.DefaultFWAction == 'allow' else 'allow'

    def ChangeRulesAction(self, spec):
        action = None
        if spec is not None:
            action = utils.GetRpcSecurityRuleAction(spec.Action)
        for rule in self.rules:
            if action is not None:
                rule.Action = action
            elif rule.Action == types_pb2.SECURITY_RULE_ACTION_ALLOW:
                rule.Action = types_pb2.SECURITY_RULE_ACTION_DENY
            else:
                rule.Action = types_pb2.SECURITY_RULE_ACTION_ALLOW

    def UpdateAttributes(self, spec):
        updateAttrList = self.GetUpdateAttributes(spec)
        if 'DefaultFWAction' in updateAttrList:
            self.ChangeDefaultFWAction(spec)
        if 'Action' in updateAttrList:
            self.ChangeRulesAction(spec)
        return

    def RollbackAttributes(self):
        attrlist = ['DefaultFWAction', 'rules']
        self.RollbackMany(attrlist)

    def FillRuleSpec(self, spec, rule):
        proto = 0
        specrule = spec.Rules.add()
        specrule.Attrs.Stateful = rule.Stateful
        specrule.Attrs.Priority = rule.Priority
        specrule.Attrs.Action = rule.Action
        l3match = rule.L3Match
        if l3match and l3match.valid:
            proto = l3match.Proto

            if proto == utils.L3PROTO_MAX:
                specrule.Attrs.Match.L3Match.ProtoWildcard = types_pb2.MATCH_ANY
            else:
                specrule.Attrs.Match.L3Match.ProtoNum = proto

            if l3match.SrcIPLow and l3match.SrcIPHigh:
                utils.GetRpcIPRange(l3match.SrcIPLow, l3match.SrcIPHigh, specrule.Attrs.Match.L3Match.SrcRange)
            if l3match.DstIPLow and l3match.DstIPHigh:
                utils.GetRpcIPRange(l3match.DstIPLow, l3match.DstIPHigh, specrule.Attrs.Match.L3Match.DstRange)
            if l3match.SrcTag:
                if utils.IsPipelineArtemis():
                    specrule.Attrs.Match.L3Match.SrcTag = l3match.SrcTag.TagId
                elif utils.IsPipelineApulu():
                    specrule.Attrs.Match.L3Match.SrcTag = l3match.SrcTag
            if l3match.DstTag:
                if utils.IsPipelineArtemis():
                    specrule.Attrs.Match.L3Match.DstTag = l3match.DstTag.TagId
                elif utils.IsPipelineApulu():
                    specrule.Attrs.Match.L3Match.DstTag = l3match.DstTag
            if l3match.SrcPrefix is not None:
                utils.GetRpcIPPrefix(l3match.SrcPrefix, specrule.Attrs.Match.L3Match.SrcPrefix)
            if l3match.DstPrefix is not None:
                utils.GetRpcIPPrefix(l3match.DstPrefix, specrule.Attrs.Match.L3Match.DstPrefix)
        l4match = rule.L4Match
        if l4match and l4match.valid:
            if utils.IsICMPProtocol(proto):
                if l4match.IcmpType == types_pb2.MATCH_ANY:
                    specrule.Attrs.Match.L4Match.TypeCode.TypeWildcard = types_pb2.MATCH_ANY
                else:
                    specrule.Attrs.Match.L4Match.TypeCode.TypeNum = l4match.IcmpType

                if l4match.IcmpCode == types_pb2.MATCH_ANY:
                    specrule.Attrs.Match.L4Match.TypeCode.CodeWildcard = types_pb2.MATCH_ANY
                else:
                    specrule.Attrs.Match.L4Match.TypeCode.CodeNum = l4match.IcmpCode
            else:
                specrule.Attrs.Match.L4Match.Ports.SrcPortRange.PortLow = l4match.SportLow
                specrule.Attrs.Match.L4Match.Ports.SrcPortRange.PortHigh = l4match.SportHigh
                specrule.Attrs.Match.L4Match.Ports.DstPortRange.PortLow = l4match.DportLow
                specrule.Attrs.Match.L4Match.Ports.DstPortRange.PortHigh = l4match.DportHigh

    def PopulateKey(self, grpcmsg):
        grpcmsg.Id.append(self.GetKey())
        return

    def PopulateSpec(self, grpcmsg):
        spec = grpcmsg.Request.add()
        spec.Id = self.GetKey()
        spec.AddrFamily = utils.GetRpcIPAddrFamily(self.AddrFamily)
        spec.DefaultFWAction = utils.GetRpcSecurityRuleAction(self.DefaultFWAction)
        for rule in self.rules:
            self.FillRuleSpec(spec, rule)
        return

    def FillJsonRuleSpec(self):
        spec = []
        for rule in self.rules:
            spec.append(rule.GetJson(self.Node))
        return spec

    def PopulateAgentJson(self):
        spec = {
            "kind": "NetworkSecurityPolicy",
            "meta": {
                "name": self.GID(),
                "namespace": self.Namespace,
                "tenant": self.Tenant,
                "uuid" : self.UUID.UuidStr,
            },
            "spec": {
                "vrf-name": f"Vpc{self.VPCId}",
                "attach-tenant": True,
                "policy-rules": self.FillJsonRuleSpec()
            }
        }
        return json.dumps(spec)

    def ValidateSpec(self, spec):
        if spec.Id != self.GetKey():
            return False
        if spec.AddrFamily != utils.GetRpcIPAddrFamily(self.AddrFamily):
            return False
        # if spec.DefaultFWAction != utils.GetRpcSecurityRuleAction(self.DefaultFwAction):
        #     return False
        if len(spec.Rules) != len(self.rules):
            return False
        for rrule,erule in zip(spec.Rules,self.rules):
            if not erule.ValidateSpec(rrule):
                logger.error(f"Validation failed for {self.GID()}")
                #return False
        return True

    def __get_random_rule(self):
        rules = self.rules
        numrules = len(rules)
        if numrules == 0:
            return None
        # TODO: Disabling randomness for debug - remove once rfc job is stable
        # return random.choice(rules)
        return rules[0]

    def __get_non_default_random_rule(self):
        """
            returns a random rule without default prefix
        """
        rules = self.rules
        numrules = len(rules)
        if numrules == 0:
            return None
        elif numrules == 1:
            rule = None
            if self.Direction == types_pb2.RULE_DIR_INGRESS:
                pfx = rules[0].L3Match.SrcPrefix
            else:
                pfx = rules[0].L3Match.DstPrefix
            if not utils.isDefaultRoute(pfx):
                rule = rules[0]
            return rule
        while True:
            rule = random.choice(rules)
            if self.Direction == types_pb2.RULE_DIR_INGRESS:
                pfx = rule.L3Match.SrcPrefix
            else:
                pfx = rule.L3Match.DstPrefix
            if not utils.isDefaultRoute(pfx):
                break
        return rule

    def IsIngressPolicy(self):
        return self.Direction == 'ingress'

    def IsEgressPolicy(self):
        return self.Direction == 'egress'

    def SetupTestcaseConfig(self, obj):
        obj.localmapping = self.l_obj
        obj.remotemapping = None
        obj.policy = self
        obj.root = self
        obj.root.FwdMode = self.FwdMode
        obj.route = self.l_obj.VNIC.SUBNET.V6RouteTable if self.AddrFamily == 'IPV6' else self.l_obj.VNIC.SUBNET.V4RouteTable
        obj.tunnel = obj.route.TUNNEL
        obj.hostport = EzAccessStoreClient[self.Node].GetHostPort()
        obj.switchport = EzAccessStoreClient[self.Node].GetSwitchPort()
        obj.devicecfg = EzAccessStoreClient[self.Node].GetDevice()
        obj.securityprofile = EzAccessStoreClient[self.Node].GetSecurityProfile()

        # select a random rule for this testcase
        if utils.IsPipelineApollo():
            # TODO: move apollo also to random rule
            obj.tc_rule = self.__get_non_default_random_rule()
        else:
            obj.tc_rule = self.__get_random_rule()
        #set tag dbs for apulu pipeline
        if utils.IsPipelineApulu():
            obj.v4ltags = LmappingClient.GetLmappingV4Tags(self.Node)
            obj.v6ltags = LmappingClient.GetLmappingV6Tags(self.Node)
            obj.v4rtags = RmappingClient.GetRmappingV4Tags(self.Node)
            obj.v6rtags = RmappingClient.GetRmappingV6Tags(self.Node)
        utils.DumpTestcaseConfig(obj)
        return

class PolicyObjectClient(base.ConfigClientBase):
    def __init__(self):
        def __isObjSupported():
            return True
        def __isIPv6PolicySupported():
            if utils.IsPipelineApulu():
                return False
            return True
        super().__init__(api.ObjectTypes.POLICY, Resmgr.MAX_POLICY)
        self.__v4ingressobjs = defaultdict(dict)
        self.__v6ingressobjs = defaultdict(dict)
        self.__v4egressobjs = defaultdict(dict)
        self.__v6egressobjs = defaultdict(dict)
        self.__v4ipolicyiter = defaultdict(dict)
        self.__v6ipolicyiter = defaultdict(dict)
        self.__v4epolicyiter = defaultdict(dict)
        self.__v6epolicyiter = defaultdict(dict)
        self.__supported = __isObjSupported()
        self.__v6supported = __isIPv6PolicySupported()
        return

    def PdsctlRead(self, node):
        # pdsctl show not supported for policy
        return True

    def GetPolicyObject(self, node, policyid):
        return self.GetObjectByKey(node, policyid)

    def IsValidConfig(self, node):
        v4count = 0
        v6count = 0
        policies = self.Objects(node)
        for policy in policies:
            numRules = len(policy.rules)
            if policy.IsV4():
                v4count += 1
                if numRules > Resmgr.MAX_RULES_PER_V4_POLICY:
                    return False, f"{policy}'s rules count {numRules} exceeds "\
                                  f"allowed limit of "\
                                  f"{Resmgr.MAX_RULES_PER_V4_POLICY} in {node}"
            else:
                v6count += 1
                if numRules > Resmgr.MAX_RULES_PER_V6_POLICY:
                    return False, f"{policy}'s rules count {numRules} exceeds "\
                                  f"allowed limit of "\
                                  f"{Resmgr.MAX_RULES_PER_V6_POLICY} in {node}"

        if  v4count > self.Maxlimit:
            return False, f"V4 Security Policy count {v4count} "\
                          f"exceeds allowed limit of {self.Maxlimit} in {node}"
        elif v4count != 0 and not self.__supported:
            return False, f"IPv4 {self.ObjType.name} is unsupported "\
                          f"- {v4count} found in {node}"
        if  v6count > self.Maxlimit:
            return False, f"V6 Security Policy count {v6count} "\
                          f"exceeds allowed limit of {self.Maxlimit} in {node}"
        elif v6count != 0 and not self.__v6supported:
            return False, f"IPv6 {self.ObjType.name} is unsupported "\
                          f"- {v6count} found in {node}"
        logger.info(f"Generated {v4count} IPv4 {self.ObjType.name} "\
                    f"Objects in {node}")
        logger.info(f"Generated {v6count} IPv6 {self.ObjType.name} "\
                    f"Objects in {node}")
        return True, ""

    def ModifyPolicyRules(self, node, policyid, subnetobj):
        if utils.IsPipelineApollo():
            # apollo does not support both sip & dip match in rules
            return

        def __is_default_l3_attr(matchtype=topo.L3MatchType.PFX, pfx=None,\
                                 iplow=None, iphigh=None, tag=None):
            if matchtype == topo.L3MatchType.PFX:
                return utils.isDefaultRoute(pfx)
            elif matchtype == topo.L3MatchType.PFXRANGE:
                return utils.isDefaultAddrRange(iplow, iphigh)
            elif matchtype == topo.L3MatchType.TAG:
                if utils.IsPipelineArtemis():
                    return utils.isTagWithDefaultRoute(tag)
                else:
                    return True
            return False

        def __get_l3_attr(l3matchtype, newpfx, newtag):
            pfx = None
            startaddr = None
            endaddr = None
            tag = None
            if l3matchtype == topo.L3MatchType.PFX:
                pfx = newpfx
            elif l3matchtype == topo.L3MatchType.PFXRANGE:
                startaddr = newpfx.network_address
                endaddr = startaddr + newpfx.num_addresses - 1
            elif l3matchtype == topo.L3MatchType.TAG:
                tag = newtag
            return pfx, startaddr, endaddr, tag

        def __modify_l3_match(direction, l3matchobj, subnetpfx, subnettag):
            if not l3matchobj.valid:
                # nothing to do in case of wildcard
                return
            if (direction == 'egress'):
                if __is_default_l3_attr(l3matchobj.SrcType, l3matchobj.SrcPrefix, l3matchobj.SrcIPLow, l3matchobj.SrcIPHigh, l3matchobj.SrcTag):
                    # no need of modification if it is already a default route
                    return
                l3matchobj.SrcPrefix, l3matchobj.SrcIPLow, l3matchobj.SrcIPHigh, l3matchobj.SrcTag = __get_l3_attr(l3matchobj.SrcType, subnetpfx, subnettag)
            else:
                if __is_default_l3_attr(l3matchobj.DstType, l3matchobj.DstPrefix, l3matchobj.DstIPLow, l3matchobj.DstIPHigh, l3matchobj.DstTag):
                    return
                l3matchobj.DstPrefix, l3matchobj.DstIPLow, l3matchobj.DstIPHigh, l3matchobj.DstTag = __get_l3_attr(l3matchobj.DstType, subnetpfx, subnettag)
            return

        policy = self.GetPolicyObject(node, policyid)
        direction = policy.Direction
        af = utils.GetIPVersion(policy.AddrFamily)
        subnetpfx = subnetobj.IPPrefix[1] if af == utils.IP_VERSION_4 else subnetobj.IPPrefix[0]
        subnettag = None
        if utils.IsTagSupported():
            if utils.IsPipelineArtemis():
                subnettag = tag.client.GetCreateTag(policy.VPCId, af, subnetpfx)
        for rule in policy.rules:
            __modify_l3_match(direction, rule.L3Match, subnetpfx, subnettag)
        return

    def GetIngV4SecurityPolicyId(self, node, vpcid):
        if len(self.__v4ingressobjs[node][vpcid]) == 0:
            return 0
        return self.__v4ipolicyiter[node][vpcid].rrnext().PolicyId

    def GetIngV6SecurityPolicyId(self, node, vpcid):
        if len(self.__v6ingressobjs[node][vpcid]) == 0:
            return 0
        return self.__v6ipolicyiter[node][vpcid].rrnext().PolicyId

    def GetEgV4SecurityPolicyId(self, node, vpcid):
        if len(self.__v4egressobjs[node][vpcid]) == 0:
            return 0
        return self.__v4epolicyiter[node][vpcid].rrnext().PolicyId

    def GetEgV6SecurityPolicyId(self, node, vpcid):
        if len(self.__v6egressobjs[node][vpcid]) == 0:
            return 0
        return self.__v6epolicyiter[node][vpcid].rrnext().PolicyId

    def Add_V4Policy(self, node, vpcid, direction, v4rules, policytype, overlaptype, level='subnet',
                     defaultfwaction='allow', fwdmode=None):
        obj = PolicyObject(node, vpcid, utils.IP_VERSION_4, direction, v4rules, policytype, overlaptype, level,
                           defaultfwaction, fwdmode)
        if direction == 'ingress':
            self.__v4ingressobjs[node][vpcid].append(obj)
        else:
            self.__v4egressobjs[node][vpcid].append(obj)
        self.Objs[node].update({obj.PolicyId: obj})
        return obj.PolicyId

    def Add_V6Policy(self, node, vpcid, direction, v6rules, policytype, overlaptype, level='subnet',
                     defaultfwaction='allow', fwdmode=None):
        obj = PolicyObject(node, vpcid, utils.IP_VERSION_6, direction, v6rules, policytype, overlaptype, level,
                           defaultfwaction, fwdmode)
        if direction == 'ingress':
            self.__v6ingressobjs[node][vpcid].append(obj)
        else:
            self.__v6egressobjs[node][vpcid].append(obj)
        self.Objs[node].update({obj.PolicyId: obj})
        return obj.PolicyId

    def Generate_Allow_All_Rules(self, spfx, dpfx, priority=RulePriority.MAX):
        rules = []
        # allow all ports
        l4AllPorts = L4MatchObject(True,\
                 sportlow=utils.L4PORT_MIN, sporthigh=utils.L4PORT_MAX,\
                 dportlow=utils.L4PORT_MIN, dporthigh=utils.L4PORT_MAX,\
                )
        # TODO: Add wildcards for all
        # allow icmp reply
        l4IcmpReply = L4MatchObject(True, icmptype=0, icmpcode=0)
        # allow icmp request
        l4IcmpRequest = L4MatchObject(True, icmptype=8, icmpcode=0)
        icmpL4Matches = [l4IcmpReply, l4IcmpRequest]
        for proto in SupportedIPProtos:
            l3match = L3MatchObject(True, proto, srcpfx=spfx, dstpfx=dpfx)
            if proto == SupportedIPProtos.ICMP:
                for icmpl4match in icmpL4Matches:
                    rule = RuleObject(l3match, icmpl4match, priority)
                    rules.append(rule)
            else:
                rule = RuleObject(l3match, l4AllPorts, priority)
                rules.append(rule)
        return rules

    def Fill_Default_Rules(self, policyobj, subnetpfx):
        rules = []
        pfx = None
        # Netagent expects a valid IP address, so if we're in netagent mode,
        # use the subnet prefix, otherwise, default values
        if policyobj.AddrFamily == 'IPV4':
            count = Resmgr.MAX_RULES_PER_V4_POLICY
            if not GlobalOptions.netagent:
                pfx = utils.IPV4_DEFAULT_ROUTE
            else:
                pfx = ipaddress.ip_network(subnetpfx[1])
        else:
            count = Resmgr.MAX_RULES_PER_V6_POLICY
            if not GlobalOptions.netagent:
                pfx = utils.IPV6_DEFAULT_ROUTE
            else:
                pfx = ipaddress.ip_network(subnetpfx[0])
        policyobj.rules = self.Generate_Allow_All_Rules(pfx, pfx)

    def GetRandomRules(self, sPfx, dPfx, count):
        # TODO: randomize proto once wildcard support for ICMP is in
        proto = SupportedIPProtos.TCP
        l4AllPorts = L4MatchObject(True,
                                   sportlow=utils.L4PORT_MIN,
                                   sporthigh=utils.L4PORT_MAX,
                                   dportlow=utils.L4PORT_MIN,
                                   dporthigh=utils.L4PORT_MAX)
        rules = []
        for i in range(count):
            priority = random.randint(RulePriority.MIN+1, RulePriority.MAX)
            l3match = L3MatchObject(True, proto,
                                    srcpfx=utils.GetRandomPrefix(sPfx),
                                    dstpfx=utils.GetRandomPrefix(dPfx))
            rule = RuleObject(l3match, l4AllPorts, priority)
            rules.append(rule)
        return rules

    def Generate_random_rules_from_nacl(self, naclobj, subnetpfx, af):
        # TODO: randomize - Add allow all with default pfx & low prio for now
        if af == utils.IP_VERSION_6:
            pfx = utils.IPV6_DEFAULT_ROUTE
            count = Resmgr.MAX_RULES_PER_V6_POLICY
        else:
            pfx = utils.IPV4_DEFAULT_ROUTE
            count = Resmgr.MAX_RULES_PER_V4_POLICY
        # TODO: randomize action & priority for DOL
        defRules = self.Generate_Allow_All_Rules(pfx, pfx)
        randRules = self.GetRandomRules(pfx, pfx, count-len(defRules))
        return defRules + randRules

    def GenerateVnicPolicies(self, numPolicy, subnet, direction, is_v6=False):
        if not self.__supported:
            return []

        vpcid = subnet.VPC.VPCId
        if is_v6:
            if not self.__v6supported:
                return []
            af = utils.IP_VERSION_6
            add_policy = self.Add_V6Policy
        else:
            af = utils.IP_VERSION_4
            add_policy = self.Add_V4Policy
        subnetpfx = subnet.IPPrefix[1] if af == utils.IP_VERSION_4 else subnet.IPPrefix[0]
        naclid = subnet.GetNaclId(direction, af)
        node = subnet.Node
        naclobj = self.GetPolicyObject(node, naclid)

        vnic_policies = []
        for i in range(numPolicy):
            overlaptype = naclobj.OverlapType
            policytype = naclobj.PolicyType
            rules = self.Generate_random_rules_from_nacl(naclobj, subnetpfx, af)
            policyid = add_policy(node, vpcid, direction, rules, policytype, overlaptype, 'vnic')
            vnic_policies.append(policyid)
        return vnic_policies

    def GenerateSubnetPolicies(self, subnet, naclid, numPolicy, direction, is_v6=False):
        if not self.__supported:
            return []

        vpcid = subnet.VPC.VPCId
        if is_v6:
            if not self.__v6supported:
                return [naclid]
            af = utils.IP_VERSION_6
            add_policy = self.Add_V6Policy
        else:
            af = utils.IP_VERSION_4
            add_policy = self.Add_V4Policy

        node = subnet.Node
        naclobj = self.GetPolicyObject(node, naclid)
        subnetpfx = subnet.IPPrefix[1] if af == utils.IP_VERSION_4 else subnet.IPPrefix[0]

        subnet_policies = [naclid]
        for i in range(numPolicy -1):
            overlaptype = naclobj.OverlapType
            policytype = naclobj.PolicyType
            rules = self.Generate_random_rules_from_nacl(naclobj, subnetpfx, af)
            policyid = add_policy(node, vpcid, direction, rules, policytype, overlaptype)
            subnet_policies.append(policyid)
        return subnet_policies

    def GenerateObjects(self, node, parent, vpc_spec_obj):
        if not self.__supported:
            return

        vpcid = parent.VPCId
        isV4Stack = utils.IsV4Stack(parent.Stack)
        isV6Stack = utils.IsV6Stack(parent.Stack) and self.__v6supported

        self.__v4ingressobjs[node][vpcid] = []
        self.__v6ingressobjs[node][vpcid] = []
        self.__v4egressobjs[node][vpcid] = []
        self.__v6egressobjs[node][vpcid] = []
        self.__v4ipolicyiter[node][vpcid] = None
        self.__v6ipolicyiter[node][vpcid] = None
        self.__v4epolicyiter[node][vpcid] = None
        self.__v6epolicyiter[node][vpcid] = None

        def __get_l4_rule(af, rulespec):
            sportlow = getattr(rulespec, 'sportlow', None)
            dportlow = getattr(rulespec, 'dportlow', None)
            sporthigh = getattr(rulespec, 'sporthigh', None)
            dporthigh = getattr(rulespec, 'dporthigh', None)
            icmptype = getattr(rulespec, 'icmptype', utils.ICMPTYPE_MIN)
            icmpcode = getattr(rulespec, 'icmpcode', utils.ICMPCODE_MIN)
            l4match = any([sportlow, sporthigh, dportlow, dporthigh, icmptype, icmpcode])
            obj = L4MatchObject(l4match, sportlow, sporthigh, dportlow, dporthigh, icmptype, icmpcode)
            return obj

        def __get_any_other_proto(protocol):
            proto = utils.GetIPProtoByName(protocol)
            proto_list = []
            for p in SupportedIPProtos:
                if p != proto and p != utils.GetIPProtoByName("icmp"):
                    proto_list.append(p)
            res = random.choice(proto_list)
            return random.choice(proto_list)

        def __get_l3_proto_from_rule(af, rulespec):
            proto = getattr(rulespec, 'protocol', utils.L3PROTO_MIN)
            if proto:
                if proto != "any":
                    if 'no-' in proto:
                        proto = __get_any_other_proto(proto[3:])
                    else:
                        if proto == "icmp" and af == utils.IP_VERSION_6:
                            proto = "ipv6-" + proto
                        proto = utils.GetIPProtoByName(proto)
                else:
                    proto = utils.L3PROTO_MAX
            return proto

        def __get_l3_match_type(rulespec, attr):
            matchtype = topo.L3MatchType.PFX
            if hasattr(rulespec, attr):
                matchval = getattr(rulespec, attr)
                if matchval == "pfxrange":
                    matchtype = topo.L3MatchType.PFXRANGE
                elif matchval == "tag":
                    matchtype = topo.L3MatchType.TAG
            return matchtype

        def __get_l3_match_type_from_rule(rulespec):
            srctype = __get_l3_match_type(rulespec, 'srctype')
            dsttype = __get_l3_match_type(rulespec, 'dsttype')
            return srctype, dsttype

        def __get_pfx_from_rule(af, rulespec, attr, ispfx=True):
            prefix = None
            if af == utils.IP_VERSION_4:
                prefix = getattr(rulespec, 'v4' + attr, None)
            else:
                prefix = getattr(rulespec, 'v6' + attr, None)
            if prefix is not None:
                if ispfx:
                    prefix = ipaddress.ip_network(prefix.replace('\\', '/'))
                else:
                    prefix = ipaddress.ip_address(prefix)
            return prefix

        def __get_l3_pfx_from_rule(af, rulespec):
            pfx = __get_pfx_from_rule(af, rulespec, 'pfx')
            srcpfx = pfx
            dstpfx = pfx
            pfx = __get_pfx_from_rule(af, rulespec, 'srcpfx')
            if pfx is not None:
                srcpfx = pfx
            pfx = __get_pfx_from_rule(af, rulespec, 'dstpfx')
            if pfx is not None:
                dstpfx = pfx
            return srcpfx, dstpfx

        def __get_l3_pfx_range_from_rule(af, rulespec):
            srciplow = __get_pfx_from_rule(af, rulespec, 'srciplow', False)
            srciphigh = __get_pfx_from_rule(af, rulespec, 'srciphigh', False)
            dstiplow = __get_pfx_from_rule(af, rulespec, 'dstiplow', False)
            dstiphigh = __get_pfx_from_rule(af, rulespec, 'dstiphigh', False)
            return srciplow, srciphigh, dstiplow, dstiphigh

        def __get_l3_tag_from_rule(af, rulespec, srctype, dsttype):
            srctag = None
            dsttag = None
            if srctype != topo.L3MatchType.TAG and dsttype != topo.L3MatchType.TAG:
                # no need to create tag if none of src,dst is of tag type
                return srctag, dsttag
            #get pfx from rule and configure tag on the fly to tagtable of af in this vpc
            if utils.IsPipelineArtemis():
                pfx = __get_pfx_from_rule(af, rulespec, 'pfx')
                tagObj = tag.client.GetCreateTag(vpcid, af, pfx)
                if srctype == topo.L3MatchType.TAG:
                    srctag = tagObj
                if dsttype == topo.L3MatchType.TAG:
                    dsttag = tagObj
            elif utils.IsPipelineApulu():
                srctag = getattr(rulespec, 'srctag', None)
                dsttag = getattr(rulespec, 'dsttag', None)
            return srctag, dsttag

        def __get_l3_rule(af, rulespec):
            def __is_l3_matchtype_supported(matchtype):
                if matchtype == topo.L3MatchType.TAG:
                    return utils.IsTagSupported()
                elif matchtype == topo.L3MatchType.PFXRANGE:
                    return utils.IsPfxRangeSupported()
                elif matchtype == topo.L3MatchType.PFX:
                    return True
                return False

            def __is_proto_supported(proto):
                if proto == utils.L3PROTO_MAX:
                    return True
                if utils.IsICMPProtocol(proto):
                    if utils.IsPipelineApollo():
                        # Apollo does NOT support icmp proto
                        return False
                return True

            def __convert_tag2pfx(matchtype):
                return topo.L3MatchType.PFX if matchtype is topo.L3MatchType.TAG else matchtype

            proto = __get_l3_proto_from_rule(af, rulespec)
            if not __is_proto_supported(proto):
                logger.error("policy rule do not support", proto)
                return None
            srctype, dsttype = __get_l3_match_type_from_rule(rulespec)
            if not utils.IsTagSupported():
                srctype = __convert_tag2pfx(srctype)
                dsttype = __convert_tag2pfx(dsttype)
            else:
                srctag = getattr(rulespec, 'srctag', None)
                dsttag = getattr(rulespec, 'dsttag', None)
                if srctag == None:
                    srctype = __convert_tag2pfx(srctype)
                if dsttag == None:
                    dsttype = __convert_tag2pfx(dsttype)
            if (not __is_l3_matchtype_supported(srctype)) or (not __is_l3_matchtype_supported(dsttype)):
                logger.error("Unsupported l3match type ", srctype, dsttype)
                return None
            srcpfx, dstpfx = __get_l3_pfx_from_rule(af, rulespec)
            srciplow, srciphigh, dstiplow, dstiphigh = __get_l3_pfx_range_from_rule(af, rulespec)
            srctag, dsttag = __get_l3_tag_from_rule(af, rulespec, srctype, dsttype)
            l3match = any([proto, srcpfx, dstpfx, srciplow, srciphigh, dstiplow, dstiphigh, srctag, dsttag])
            obj = L3MatchObject(l3match, proto, srcpfx, dstpfx, srciplow, srciphigh, dstiplow, dstiphigh, srctag, dsttag, srctype, dsttype)
            return obj

        def __get_rule_action(rulespec):
            actionVal = getattr(rulespec, 'action', None)
            if actionVal == "deny":
                action = types_pb2.SECURITY_RULE_ACTION_DENY
            elif actionVal == "random":
                action = random.choice([types_pb2.SECURITY_RULE_ACTION_DENY, types_pb2.SECURITY_RULE_ACTION_ALLOW])
            else:
                action = types_pb2.SECURITY_RULE_ACTION_ALLOW
            return action

        def __get_valid_priority(prio):
            if prio < RulePriority.MIN:
                return RulePriority.MIN
            elif prio > RulePriority.MAX:
                return RulePriority.MAX
            else:
                return prio

        def __get_rule_priority(rulespec, basePrio=0):
            prio = getattr(rulespec, 'priority', -3)
            if prio == -1:
                # increasing priority
                basePrio = basePrio + 1
                priority = basePrio
            elif prio == -2:
                # decreasing priority
                basePrio = basePrio - 1
                priority = basePrio
            elif prio == -3:
                # random priority
                priority = random.randint(RulePriority.MIN, RulePriority.MAX)
            else:
                if isinstance(prio, str):
                    try:
                        specprio = prio.replace("MAXPRIO", str(RulePriority.MAX))
                        specprio = specprio.replace("MINPRIO", str(RulePriority.MIN))
                        priority = int(eval(specprio))
                    except:
                        logger.error("Invalid policy priority in spec ", prio)
                        priority = 0
                else:
                    # configured priority
                    priority = prio
            return __get_valid_priority(priority), basePrio

        def __get_l3_rules_from_rule_base(af, rulespec, overlaptype):

            def __get_first_subnet(ippfx, pfxlen):
                for ip in ippfx.subnets(new_prefix=pfxlen):
                    return (ip)
                return

            def __get_user_specified_pfxs(pfxspec):
                pfxlist = []
                if pfxspec:
                    for pfx in pfxspec:
                        pfxlist.append(ipaddress.ip_network(pfx.replace('\\', '/')))
                return pfxlist

            def __get_adjacent_pfxs(basepfx, count):
                pfxlist = []
                c = 1
                pfxlist.append(ipaddress.ip_network(basepfx))
                while c < count:
                    pfxlist.append(utils.GetNextSubnet(pfxlist[c-1]))
                    c += 1
                return pfxlist

            def __get_overlap_pfxs(basepfx, base, count):
                # for overlap, add user specified base prefix with original prefixlen
                pfxlist = [ basepfx ]
                pfxlist.extend(__get_adjacent_pfxs(base, count))
                return pfxlist

            if af == utils.IP_VERSION_4:
                pfxcount = getattr(rulespec, 'nv4prefixes', 0)
                pfxlen = getattr(rulespec, 'v4prefixlen', 24)
                basepfx = ipaddress.ip_network(rulespec.v4base.replace('\\', '/'))
                user_specified_routes = __get_user_specified_pfxs(getattr(rulespec, 'v4prefixes', None))
            else:
                pfxcount = getattr(rulespec, 'nv6prefixes', 0)
                pfxlen = getattr(rulespec, 'v6prefixlen', 120)
                basepfx = ipaddress.ip_network(rulespec.v6base.replace('\\', '/'))
                user_specified_routes = __get_user_specified_pfxs(getattr(rulespec, 'v6prefixes', None))

            newbase = __get_first_subnet(basepfx, pfxlen)
            pfxlist = user_specified_routes
            if 'adjacent' in overlaptype:
                pfxlist +=  __get_adjacent_pfxs(newbase, pfxcount)
            elif 'overlap' in overlaptype:
                pfxlist += __get_overlap_pfxs(basepfx, newbase, pfxcount-1)

            l3rules = []
            proto = __get_l3_proto_from_rule(af, rulespec)
            for pfx in pfxlist:
                obj = L3MatchObject(True, proto, pfx, pfx)
                l3rules.append(obj)
            return l3rules

        def __get_rules(af, policyspec, overlaptype):
            rules = []
            if not hasattr(policyspec, 'rule'):
                return rules
            policy_spec_type = policyspec.type
            for rulespec in policyspec.rule:
                stateful = getattr(rulespec, 'stateful', False)
                l4match = __get_l4_rule(af, rulespec)
                if policy_spec_type == 'base':
                    prioritybase = getattr(rulespec, 'prioritybase', 0)
                    objs = __get_l3_rules_from_rule_base(af, rulespec, overlaptype)
                    if len(objs) == 0:
                        return None
                    for l3match in objs:
                        priority, prioritybase = __get_rule_priority(rulespec, prioritybase)
                        action = __get_rule_action(rulespec)
                        rule = RuleObject(l3match, l4match, priority, action, stateful)
                        rules.append(rule)
                else:
                    l3match = __get_l3_rule(af, rulespec)
                    if l3match is None:
                        # L3 match is mandatory for a rule
                        return None
                    priority, prioritybase = __get_rule_priority(rulespec)
                    action = __get_rule_action(rulespec)
                    rule = RuleObject(l3match, l4match, priority, action, stateful)
                    rules.append(rule)
            return rules

        def __add_v4policy(direction, v4rules, policytype, overlaptype, defaultfwaction, fwdmode=None):
            obj = PolicyObject(node, vpcid, utils.IP_VERSION_4, direction, v4rules, policytype, overlaptype,
                               defaultfwaction=defaultfwaction, fwdmode=fwdmode)
            if direction == 'ingress':
                self.__v4ingressobjs[node][vpcid].append(obj)
            else:
                self.__v4egressobjs[node][vpcid].append(obj)
            self.Objs[node].update({obj.PolicyId: obj})

        def __add_v6policy(direction, v6rules, policytype, overlaptype, defaultfwaction, fwdmode=None):
            obj = PolicyObject(node, vpcid, utils.IP_VERSION_6, direction, v6rules, policytype, overlaptype,
                               defaultfwaction=defaultfwaction, fwdmode=fwdmode)
            if direction == 'ingress':
                self.__v6ingressobjs[node][vpcid].append(obj)
            else:
                self.__v6egressobjs[node][vpcid].append(obj)
            self.Objs[node].update({obj.PolicyId: obj})

        def __add_user_specified_policy(policyspec, policytype, overlaptype, defaultfwaction, fwdmode=None):
            direction = policyspec.direction
            if isV4Stack:
                v4rules = __get_rules(utils.IP_VERSION_4, policyspec, overlaptype)
                if v4rules is None and policytype != "empty":
                    return
                if direction == 'bidir':
                    #For bidirectional, add policy in both directions
                    policyobj = __add_v4policy('ingress', v4rules, policytype, overlaptype, defaultfwaction, fwdmode)
                    policyobj = __add_v4policy('egress', v4rules, policytype, overlaptype, defaultfwaction, fwdmode)
                else:
                    policyobj = __add_v4policy(direction, v4rules, policytype, overlaptype, defaultfwaction, fwdmode)

            if isV6Stack:
                v6rules = __get_rules(utils.IP_VERSION_6, policyspec, overlaptype)
                if v6rules is None and policytype != "empty":
                    return
                if direction == 'bidir':
                    #For bidirectional, add policy in both directions
                    policyobj = __add_v6policy('ingress', v6rules, policytype, overlaptype, defaultfwaction, fwdmode)
                    policyobj = __add_v6policy('egress', v6rules, policytype, overlaptype, defaultfwaction, fwdmode)
                else:
                    policyobj = __add_v6policy(direction, v6rules, policytype, overlaptype, defaultfwaction, fwdmode)

        def __get_num_subnets(vpc_spec_obj):
            count = 0
            if (hasattr(vpc_spec_obj, 'subnet')):
                for subnet_obj in vpc_spec_obj.subnet:
                    count += subnet_obj.count
            return count

        def __add_default_policies(vpc_spec_obj, policyspec, defaultfwaction, fwdmode=None):
            policycount = getattr(policyspec, 'count', 0)
            if policycount == 0:
                # use number of subnets instead
                policycount = __get_num_subnets(vpc_spec_obj)
            for i in range(policycount):
                __add_user_specified_policy(policyspec, policyspec.policytype, None, defaultfwaction, fwdmode)

        for policy_spec_obj in vpc_spec_obj.policy:
            policy_spec_type = policy_spec_obj.type
            policytype = policy_spec_obj.policytype
            defaultfwaction = getattr(policy_spec_obj, 'defaultfwaction', "allow")
            fwdmode = getattr(policy_spec_obj, 'fwdmode', None)
            if policy_spec_type == "specific":
                if policytype == 'default' or policytype == 'subnet':
                    __add_default_policies(vpc_spec_obj, policy_spec_obj, defaultfwaction, fwdmode)
                else:
                    overlaptype = getattr(policy_spec_obj, 'overlaptype', None)
                    __add_user_specified_policy(policy_spec_obj, \
                                                policytype, overlaptype, defaultfwaction, fwdmode)
            elif policy_spec_type == "base":
                overlaptype = getattr(policy_spec_obj, 'overlaptype', 'none')
                __add_user_specified_policy(policy_spec_obj, policytype, overlaptype,
                                            defaultfwaction, fwdmode)

        if len(self.__v4ingressobjs[node][vpcid]) != 0:
            self.__v4ipolicyiter[node][vpcid] = utils.rrobiniter(self.__v4ingressobjs[node][vpcid])

        if len(self.__v6ingressobjs[node][vpcid]) != 0:
            self.__v6ipolicyiter[node][vpcid] = utils.rrobiniter(self.__v6ingressobjs[node][vpcid])

        if len(self.__v4egressobjs[node][vpcid]) != 0:
            self.__v4epolicyiter[node][vpcid] = utils.rrobiniter(self.__v4egressobjs[node][vpcid])

        if len(self.__v6egressobjs[node][vpcid]) != 0:
            self.__v6epolicyiter[node][vpcid] = utils.rrobiniter(self.__v6egressobjs[node][vpcid])

        return

client = PolicyObjectClient()

class PolicyObjectHelper:
    def __init__(self):
        return

    def __get_policyID_from_subnet(self, subnet, af, direction):
        if af == 'IPV6':
            return subnet.IngV6SecurityPolicyIds[0] if direction == 'ingress' else subnet.EgV6SecurityPolicyIds[0]
        else:
            return subnet.IngV4SecurityPolicyIds[0] if direction == 'ingress' else subnet.EgV4SecurityPolicyIds[0]

    def __is_lmapping_match(self, policyobj, lobj):
        if lobj.VNIC.SUBNET.VPC.VPCId != policyobj.VPCId:
            return False
        if lobj.AddrFamily == policyobj.AddrFamily:
            return (policyobj.PolicyId == self.__get_policyID_from_subnet(lobj.VNIC.SUBNET, policyobj.AddrFamily, policyobj.Direction))
        return False

    def GetMatchingConfigObjects(self, selectors):
        objs = []
        dutNode = EzAccessStore.GetDUTNode()
        policyobjs = filter(lambda x: x.IsFilterMatch(selectors), client.Objects(dutNode))
        for policyObj in policyobjs:
            for lobj in lmapping.client.Objects(dutNode):
                if self.__is_lmapping_match(policyObj, lobj):
                    policyObj.l_obj = lobj
                    objs.append(policyObj)
                    break
        return utils.GetFilteredObjects(objs, selectors.maxlimits, False)

PolicyHelper = PolicyObjectHelper()

def GetMatchingObjects(selectors):
    return PolicyHelper.GetMatchingConfigObjects(selectors)
