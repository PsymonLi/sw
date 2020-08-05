
import os
import json
import time
import iota.harness.api as api
from iota.harness.infra.glopts import GlobalOptions as GlobalOptions

def verify_bmc_logs(node_name, logs, tag=None, save_logs=False):
    # This is a trivial implmenentation of looking for patterns of failure indications
    # in BMC.

    if save_logs:
        base_dir = os.path.join(GlobalOptions.topdir, 'iota', 'logs', 'bmc', node_name)
        if not os.path.isfile(base_dir):
            os.makedirs(base_dir, exist_ok=True)
        with open(os.path.join(base_dir, tag + '.json'), 'w') as f:
            json.dump(logs, f)


    api.Logger.debug(f"Checking bmc-log for {node_name}")
    for log in logs:
        # TODO: Based on vendor, look for specific structures and fields to look for failures.
        if log.get('Severity', 'OK') in ['Critical']:
            api.Logger.error(f"Found errors for node Node: {node_name}: {log['Message']}")
            return api.types.status.FAILURE

    return api.types.status.SUCCESS

