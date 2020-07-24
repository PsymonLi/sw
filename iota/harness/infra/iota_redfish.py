from iota.harness.thirdparty.redfish.rest.v1 import ServerDownOrUnreachableError
from iota.harness.thirdparty.redfish.rest.v1 import InvalidCredentialsError
from iota.harness.thirdparty.redfish import redfish_client
import iota.harness.api as api
import iota.harness.infra.types as types
import pdb
import traceback

class IotaRedfish(object):

    def __init__(self, cimcInfo, vendor=None):
        self.ip = cimcInfo.GetIp()
        self.username = cimcInfo.GetUsername()
        self.password = cimcInfo.GetPassword()
        try:
            api.Logger.debug("creating redfish object for ip {0}".format(self.ip))
            self.rfc = redfish_client(base_url="https://%s" % self.ip, username=self.username, password=self.password, max_retry=10)
            self.rfc.login()
        except ServerDownOrUnreachableError:
            raise RuntimeError("ILO(%s) not reachable or does not support RedFish" % self.ip)
        except InvalidCredentialsError:
            raise RuntimeError("Invalid credentials for ILO(%s) login" % self.ip)
        if vendor:
            self.vendor = vendor
        else:
            managers_uri = self.rfc.root['Managers']['@odata.id']
            managers_response = self.rfc.get(managers_uri)
            managers_members_uri = next(iter(managers_response.obj['Members']))['@odata.id']
            managers_members_response = self.rfc.get(managers_members_uri)
            keys = list(managers_members_response.obj.Oem.keys())
            if not keys:
                raise Exception("failed to find any keys in obj.Oem")
            if keys[0] == "Dell":
                self.vendor = types.vendors.DELL
            elif keys[0] == "Cisco":
                self.vendor = types.vendors.CISCO
            elif keys[0] == "Hpe":
                self.vendor = types.vendors.HPE
            else:
                raise Exception("failed to determine server vendor from key {0}".format(keys[0]))

    def Close(self):
        try: self.rfc.logout()
        except: 
            api.Logger.warn("failed redfish logout from server {0}".format(self.ip))
            api.Logger.debug(traceback.format_exc())

    def GetRedfishClient(self):
        return self.rfc

    def GetFirstMember(self):
        mgrUri = self.rfc.root['Managers']['@odata.id']
        resp = self.rfc.get(mgrUri)
        return next(iter(resp.obj['Members']))

    def GetOem(self):
        member = self.GetFirstMember()
        if not member:
            raise Exception("failed to find first member for {0}".format(self.ip))
        resp = self.rfc.get(member['@odata.id'])
        return resp.obj.Oem

    def RedfishPowerCommand(self, param):
        data = {"ResetType":param}
        req = "/redfish/v1/Systems/1/Actions/ComputerSystem.Reset"
        api.Logger.debug("sending request {0} with data {1}".format(req, data))
        resp = self.rfc.post(req, body=data)
        if resp.status != 0:
            raise Exception("failed to issue power cycle operation {0}. return code was: {1}".format(param, resp.status))

