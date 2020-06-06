#! /usr/bin/python3

import apollo.config.objects.lmapping as lmapping
import apollo.config.objects.policy as policy
import apollo.config.utils as utils
from apollo.config.store import EzAccessStore

class PolicyObjectHelper:
    def __init__(self):
        return

    def __is_policyID_applied_to_subnet(self, policy_id, subnet, af, direction):
        if af == 'IPV6':
            policy_ids = subnet.IngV6SecurityPolicyIds if direction == 'ingress' else subnet.EgV6SecurityPolicyIds
        else:
            policy_ids = subnet.IngV4SecurityPolicyIds if direction == 'ingress' else subnet.EgV4SecurityPolicyIds
        return policy_id in policy_ids

    def __is_lmapping_match(self, policyobj, lobj):
        if lobj.VNIC.SUBNET.VPC.VPCId != policyobj.VPCId:
            return False
        if lobj.AddrFamily == policyobj.AddrFamily:
            return self.__is_policyID_applied_to_subnet(policyobj.PolicyId, lobj.VNIC.SUBNET, policyobj.AddrFamily, policyobj.Direction)
        return False

    def GetMatchingConfigObjects(self, selectors):
        objs = []
        dutNode = EzAccessStore.GetDUTNode()
        policyobjs = filter(lambda x: x.IsFilterMatch(selectors), policy.client.Objects(dutNode))
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
