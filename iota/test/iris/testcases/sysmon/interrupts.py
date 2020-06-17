#! /usr/bin/python3

import iota.harness.api as api
import json

class InterruptField:
    def __init__(self, reg_name, field):
        self._name = reg_name
        self._field = field
        self._count = 0

    def name(self):
        return self._name

    def field(self):
        return self._field

    def count(self):
        return self._count

    def set_count(self, count):
        self._count = count

    def parse_count_cmd_output(self, output):
        return int(output)

    def field_cmp_str(self):
        return self.field().replace('_interrupt', '')

    def set_field_cmd(self):
        return '/bin/echo \'fset ' + self.name() + ' ' + self.field() + '=0x1\' | LD_LIBRARY_PATH=/nic/lib:/platform/lib:$LD_LIBRARY_PATH /platform/bin/capview'

    def get_count_cmd(self):
        return '/bin/grep ' + self.field_cmp_str() + ' /obfl/asicerrord_err.log | wc -l'

class DelphiInterruptField(InterruptField):
    def __init__(self, reg_name, field, delphi_name):
        InterruptField.__init__(self, reg_name, field)
        self._delphi_name = delphi_name

    def delphi_name(self):
        return self._delphi_name

    def get_count_cmd(self):
        return 'LD_LIBRARY_PATH=/platform/lib:/nic/lib:$LD_LIBRARY_PATH /nic/bin/delphictl metrics get ' + self.delphi_name()

    def parse_count_cmd_output(self, output):
        out = None
        try:
            out = json.loads(output)
        except:
            # output is empty if interrupt is not yet triggered
            return 0

        return out[self.delphi_name()][self.field_cmp_str()]


intr_reg_list = [
    ('pr_prd_int_grp2_int_test_set', 'Prprprdintgrp2Metrics', ['rdrsp_axi_interrupt', 'wrrsp_axi_interrupt']),
    ('pt_ptd_int_grp2_int_test_set', 'Ptptptdintgrp2Metrics', ['rdrsp_axi_interrupt', 'wrrsp_axi_interrupt', 'pkt_cmd_eop_no_data_interrupt'])
]

intr_list = []

def Setup(tc):
    tc.Nodes = api.GetNaplesHostnames()
    api.Logger.info("Found {} hosts" .format(len(tc.Nodes)))

    for node in tc.Nodes:
        for reg_name, delphi_name, fields in intr_reg_list:
            for field in fields:
                intr_field = DelphiInterruptField(reg_name, field, delphi_name)
                intr_list.append( (node,intr_field) )
    return api.types.status.SUCCESS

def Trigger(tc):
    api.Logger.info("Starting interrupt test")

    for node, intr in intr_list:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
        api.Trigger_AddNaplesCommand(req, node, intr.get_count_cmd())
        api.Trigger_AddNaplesCommand(req, node, intr.set_field_cmd())
        resp = api.Trigger(req)

        for cmd in resp.commands:
            if cmd.exit_code != 0:
                api.Logger.error("Command failed: {}, Node {}, Interrupt {}, Field {}".format(cmd.command, node, intr.name(), intr.field()))
                return api.types.status.FAILURE

        cmd = resp.commands[0]
        count = intr.parse_count_cmd_output(cmd.stdout)
        if count == -1:
            api.Logger.error("Invalid count: Node {}, Interrupt {}, Field {}".format(node, intr.name(), intr.field()))
            return api.types.status.FAILURE

        api.Logger.info("Set: Node {}, Interrupt {}, Field {}, {} -> {}".format(node, intr.name(), intr.field(), count, count + 1))
        intr.set_count(count)

    return api.types.status.SUCCESS

def Verify(tc):
    rc = api.types.status.SUCCESS

    req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
    api.Trigger_AddNaplesCommand(req, tc.Nodes[0], 'sleep 3')
    resp = api.Trigger(req)

    for node, intr in intr_list:
        req = api.Trigger_CreateExecuteCommandsRequest(serial = True)
        api.Trigger_AddNaplesCommand(req, node, intr.get_count_cmd())
        resp = api.Trigger(req)

        cmd = resp.commands[0]
        if cmd.exit_code != 0:
            api.Logger.error("Command failed: Node {}, Interrupt {}, Field {}".format(node, intr.name(), intr.field()))
            return api.types.status.FAILURE

        expected = intr.count() + 1
        value = intr.parse_count_cmd_output(cmd.stdout)

        if value < expected:
            api.Logger.error("Node {}, Interrupt {}, Field {}, Expected {}, Got {}".format(node, intr.name(), intr.field(), expected, value))
            rc = api.types.status.FAILURE

    return rc

def Teardown(tc):
    return api.types.status.SUCCESS
