from iota.harness.thirdparty.redfish.rest.v1 import ServerDownOrUnreachableError
from iota.harness.thirdparty.redfish.rest.v1 import InvalidCredentialsError
from iota.harness.thirdparty.redfish import redfish_client
import iota.harness.api as api

def get_redfish_obj(cimc_info, mode="dedicated"):
    if mode == "dedicated":
        curr_ilo_ip = cimc_info.GetIp()
    else:
        curr_ilo_ip = cimc_info.GetNcsiIp() 
    
    try:
        # Create a Redfish client object
        api.Logger.info("Connecting to ILO at %s" % curr_ilo_ip)
        RF = redfish_client(base_url="https://%s" % curr_ilo_ip,
                                username=cimc_info.GetUsername(),
                                password=cimc_info.GetPassword(), max_retry=10)
        # Login with the Redfish client
        RF.login()
    except ServerDownOrUnreachableError:
        raise RuntimeError("ILO(%s) not reachable or does not support RedFish"
                           % curr_ilo_ip)
    except InvalidCredentialsError:
        raise RuntimeError("Invalid credentials for ILO(%s) login"
                           % curr_ilo_ip)
    return RF
