#! /usr/bin/python3
import iota.harness.api as api

def validate_error_counters(errors_counters):
    is_error = False
    for counter in errors_counters:
        val = int(counter.split(":")[1])
        pname = counter.split(":")[0]
        if 0 < val < 10:
            api.Logger.warn("%s is occurred with counter %s " % (pname, val))
        elif val > 10:
            api.Logger.error("%s is occurred with counter %s " % (pname, val))
            is_error = True
    return is_error


def validate_full_reset_counters(errors_counters):
    errors = []
    for counter in errors_counters:
        val = int(counter.split(":")[1])
        if val != 0:
            errors.append(counter)
    return errors

def Setup(tc):
    tc.naples_nodes = api.GetNaplesNodes()
    if len(tc.naples_nodes) == 0:
        api.Logger.error("No naples node found")
        return api.types.status.ERROR
    return api.types.status.SUCCESS

def Trigger(tc):
    req = api.Trigger_CreateExecuteCommandsRequest(serial=True)
    for n in tc.naples_nodes:
        api.Trigger_AddNaplesCommand(req, n.Name(), "export LD_LIBRARY_PATH=/platform/lib:/nic/lib && export PATH=$PATH:/platform/bin && mctputil -i")
    tc.resp = api.Trigger(req)
    return api.types.status.SUCCESS

def Verify(tc):
    for cmd in tc.resp.commands:
        api.PrintCommandResults(cmd)
        if cmd.exit_code == 0:
            if cmd.stdout:
                removed_hash = [row for row in cmd.stdout.split("\n") if "#" not in row]
                errors_counters = [row for row in removed_hash if "error" in row]
                full_reset_counters = [row for row in removed_hash if "full_reset" in row]
                is_error_counter = validate_error_counters(errors_counters)
                err_full_reset_counters = validate_full_reset_counters(full_reset_counters)
                if is_error_counter:
                    return api.types.status.FAILURE
                elif err_full_reset_counters:
                    api.Logger.error("Counter is occurred with parameter %s" % ", ".join(err_full_reset_counters))
                    return api.types.status.FAILURE
        else:
            return api.types.status.FAILURE
    return api.types.status.SUCCESS

def Teardown(tc):
    return api.types.status.SUCCESS