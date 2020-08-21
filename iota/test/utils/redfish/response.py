import json
import traceback

import iota.harness.api as api

def get_error_messages(resp):
    err_msgs = [error["Message"].lower() for error in resp.obj['error']['@Message.ExtendedInfo'] if error["Message"]]
    return err_msgs

def validate_resp(resp):
    if resp.status == 400:
        try:
            api.Logger.error(json.dumps(resp.obj['error']\
                                ['@Message.ExtendedInfo'], indent=4, sort_keys=True))
        except:
            api.Logger.error(traceback.format_exc())
            return api.types.status.FAILURE
    elif resp.status not in [200, 204]:
        err_msgs = get_error_messages(resp)
        api.Logger.info("An http response of \'%s\' was returned with %s.\n" % (resp.status, err_msgs))
        return api.types.status.FAILURE
    else:
        return api.types.status.SUCCESS
