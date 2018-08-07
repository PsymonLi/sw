#! /usr/bin/python3
import copy
import pdb

import iris.test.firewall.tracker.tracker as base 
import iris.test.fte.alg.tracker.step as step
import iris.test.fte.alg.tracker.connection as connection
import iris.test.firewall.tracker.flowstate as flowstate

from infra.common.logging import logger as logger
from iris.test.fte.alg.tracker.store import TrackerStore

class ALGTrackerObject(base.TrackerObject):
    def __init__(self):
        super().__init__()

    def Init(self, gid, connspec):
        super().Init(gid, connspec)

    def PktTemplate(self):
        return self.step.PktTemplate()

    def IsFteDone(self):
        return self.step.IsFteDone()

    def IsIgnorePktField(self):
        return self.step.IsIgnorePktField()

    def __set_flow_states(self):
        ifstate = self.flowstate.GetState(True)
        rfstate = self.flowstate.GetState(False)
        self.step.SetFlowStates(ifstate, rfstate)
        return

    def SetStep(self, stepspec, tc):
        step = stepspec.step.Get(TrackerStore)
        self.step = copy.copy(step)
        self.step.SetCpuCopyValid(getattr(stepspec, 'cpu', False))
        self.step.SetFTEDone(getattr(stepspec, 'fte_done', False))
        self.step.SetIgnorePktField(getattr(stepspec, 'ignore_pkt_field', False))
        self.__set_flow_states()
        self.step.SetPorts(self.iport, self.rport)
        if 'pkttemplate' in dir(stepspec):
            self.step.SetPktTemplate(getattr(stepspec, 'pkttemplate'))
        self.step.SetIPs(tc)
        self.step.Show()

logger.info("Loading Tracker Connection Specs:")
connection.LoadConnectionSpecs()
logger.info("Loading Tracker Step Specs:")
step.LoadStepSpecs()
