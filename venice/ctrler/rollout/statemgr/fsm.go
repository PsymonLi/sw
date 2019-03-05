package statemgr

import (
	"strconv"
	"sync/atomic"
	"time"

	"github.com/pensando/sw/api/generated/rollout"

	"github.com/pensando/sw/venice/utils/log"
)

const defaultNumParallel = 2 // if user has not specified parallelism in Spec, we do do many SmartNICs in parallel. We can change this logic in future as needed..

var preUpgradeTimeout = 45 * time.Second

type rofsmEvent uint
type rofsmState uint
type rofsmActionFunc func(ros *RolloutState)

const (
	fsmstInvalid rofsmState = iota
	fsmstStart
	fsmstPreCheckingVenice
	fsmstPreCheckingSmartNIC
	fsmstWaitForSchedule
	fsmstRollingOutVenice
	fsmstRollingOutService
	fsmstRollingoutOutSmartNIC
	fsmstRolloutSuccess
	fsmstRolloutPausing
	fsmstRolloutFail
	fsmstRolloutSuspend
)

func (x rofsmState) String() string {
	switch x {
	case fsmstInvalid:
		return "fsmstInvalid"
	case fsmstStart:
		return "fsmstStart"
	case fsmstPreCheckingVenice:
		return "fsmstPreCheckingVenice"
	case fsmstPreCheckingSmartNIC:
		return "fsmstPreCheckingSmartNIC"
	case fsmstWaitForSchedule:
		return "fsmstWaitForSchedule"
	case fsmstRollingOutVenice:
		return "fsmstRollingOutVenice"
	case fsmstRollingOutService:
		return "fsmstRollingOutService"
	case fsmstRollingoutOutSmartNIC:
		return "fsmstRollingoutOutSmartNIC"
	case fsmstRolloutSuccess:
		return "fsmstRolloutSuccess"
	case fsmstRolloutPausing:
		return "fsmstRolloutPausing"
	case fsmstRolloutFail:
		return "fsmstRolloutFail"
	case fsmstRolloutSuspend:
		return "fsmstRolloutSuspend"
	}
	return "unknownState " + strconv.Itoa(int(x))
}

const (
	fsmEvInvalid rofsmEvent = iota
	fsmEvROCreated
	fsmEvVeniceBypass // bypass venice rollout on user request
	fsmEvOneVenicePreUpgSuccess
	fsmEvOneVenicePreUpgFail
	fsmEvAllVenicePreUpgOK
	fsmEvOneSmartNICPreupgSuccess
	fsmEvOneSmartNICPreupgFail
	fsmEvAllSmartNICPreUpgOK
	fsmEvSomeSmartNICPreUpgFail
	fsmEvScheduleNow
	fsmEvOneVeniceUpgSuccess
	fsmEvOneVeniceUpgFail
	fsmEvAllVeniceUpgOK
	fsmEvServiceUpgFail
	fsmEvServiceUpgOK
	fsmEvOneSmartNICUpgSuccess
	fsmEvOneSmartNICUpgFail
	fsmEvFailThresholdReached
	fsmEvSuspend
	fsmEvFail
	fsmEvSuccess
)

func (x rofsmEvent) String() string {
	switch x {
	case fsmEvInvalid:
		return "fsmEvInvalid"
	case fsmEvROCreated:
		return "fsmEvROCreated"
	case fsmEvVeniceBypass:
		return "fsmEvVeniceBypass"
	case fsmEvOneVenicePreUpgSuccess:
		return "fsmEvOneVenicePreUpgSuccess"
	case fsmEvOneVenicePreUpgFail:
		return "fsmEvOneVenicePreUpgFail"
	case fsmEvAllVenicePreUpgOK:
		return "fsmEvAllVenicePreUpgOK"
	case fsmEvOneSmartNICPreupgSuccess:
		return "fsmEvOneSmartNICPreupgSuccess"
	case fsmEvOneSmartNICPreupgFail:
		return "fsmEvOneSmartNICPreupgFail"
	case fsmEvAllSmartNICPreUpgOK:
		return "fsmEvAllSmartNICPreUpgOK"
	case fsmEvSomeSmartNICPreUpgFail:
		return "fsmEvSomeSmartNICPreUpgFail"
	case fsmEvScheduleNow:
		return "fsmEvScheduleNow"
	case fsmEvOneVeniceUpgSuccess:
		return "fsmEvOneVeniceUpgSuccess"
	case fsmEvOneVeniceUpgFail:
		return "fsmEvOneVeniceUpgFail"
	case fsmEvAllVeniceUpgOK:
		return "fsmEvAllVeniceUpgOK"
	case fsmEvServiceUpgFail:
		return "fsmEvServiceUpgFail"
	case fsmEvServiceUpgOK:
		return "fsmEvServiceUpgOK"
	case fsmEvOneSmartNICUpgSuccess:
		return "fsmEvOneSmartNICUpgSuccess"
	case fsmEvOneSmartNICUpgFail:
		return "fsmEvOneSmartNICUpgFail"
	case fsmEvFailThresholdReached:
		return "fsmEvFailThresholdReached"
	case fsmEvSuspend:
		return "fsmEvSuspend"
	case fsmEvFail:
		return "fsmEvFail"
	case fsmEvSuccess:
		return "fsmEvSuccess"
	}
	return "unknownEvent " + strconv.Itoa(int(x))

}

type fsmNode struct {
	nextSt rofsmState
	actFn  rofsmActionFunc
}

var roFSM = [][]fsmNode{
	fsmstStart: {
		fsmEvROCreated: {nextSt: fsmstPreCheckingVenice, actFn: fsmAcCreated},
		fsmEvSuspend:   {nextSt: fsmstRolloutSuspend, actFn: fsmAcRolloutSuspend},
	},

	fsmstPreCheckingVenice: {
		fsmEvOneVenicePreUpgSuccess: {nextSt: fsmstPreCheckingVenice, actFn: fsmAcOneVenicePreupgSuccess},
		fsmEvOneVenicePreUpgFail:    {nextSt: fsmstRolloutFail, actFn: fsmAcRolloutFail},
		fsmEvAllVenicePreUpgOK:      {nextSt: fsmstPreCheckingSmartNIC, actFn: fsmAcPreUpgSmartNIC},
		fsmEvVeniceBypass:           {nextSt: fsmstPreCheckingSmartNIC, actFn: fsmAcPreUpgSmartNIC},
	},
	fsmstPreCheckingSmartNIC: {
		fsmEvAllSmartNICPreUpgOK:      {nextSt: fsmstWaitForSchedule, actFn: fsmAcWaitForSchedule},
		fsmEvSomeSmartNICPreUpgFail:   {nextSt: fsmstRolloutFail, actFn: fsmAcRolloutFail},
		fsmEvOneSmartNICPreupgSuccess: {nextSt: fsmstPreCheckingSmartNIC}, // TODO
		fsmEvOneSmartNICPreupgFail:    {nextSt: fsmstPreCheckingSmartNIC}, // TODO
	},
	fsmstWaitForSchedule: {
		fsmEvScheduleNow: {nextSt: fsmstRollingOutVenice, actFn: fsmAcIssueNextVeniceRollout},
	},
	fsmstRollingOutVenice: {
		fsmEvOneVeniceUpgSuccess: {nextSt: fsmstRollingOutVenice, actFn: fsmAcIssueNextVeniceRollout},
		fsmEvAllVeniceUpgOK:      {nextSt: fsmstRollingOutService, actFn: fsmAcIssueServiceRollout},
		fsmEvVeniceBypass:        {nextSt: fsmstRollingoutOutSmartNIC, actFn: fsmAcRolloutSmartNICs},
		fsmEvSuspend:             {nextSt: fsmstRolloutSuspend, actFn: fsmAcRolloutSuspend},
	},
	fsmstRollingOutService: {
		fsmEvServiceUpgOK: {nextSt: fsmstRollingoutOutSmartNIC, actFn: fsmAcRolloutSmartNICs},
	},
	fsmstRollingoutOutSmartNIC: {
		fsmEvSuccess:               {nextSt: fsmstRolloutSuccess, actFn: fsmAcRolloutSuccess},
		fsmEvOneSmartNICUpgFail:    {nextSt: fsmstRollingoutOutSmartNIC}, // TODO
		fsmEvOneSmartNICUpgSuccess: {nextSt: fsmstRollingoutOutSmartNIC}, // TODO
		fsmEvFail:                  {nextSt: fsmstRolloutFail, actFn: fsmAcRolloutFail},
		fsmEvSuspend:               {nextSt: fsmstRolloutSuspend, actFn: fsmAcRolloutSuspend},
	},
}

func (ros *RolloutState) runFSM() {
	defer ros.Done()
	for {
		select {
		case <-ros.stopChan:
			return
		case evt := <-ros.eventChan:
			log.Infof("In State %s got Event %s", ros.currentState, evt)
			nstate := ros.fsm[ros.currentState][evt].nextSt
			action := ros.fsm[ros.currentState][evt].actFn
			if action != nil {
				action(ros)
			}
			ros.currentState = nstate
		}
	}
}

func fsmAcCreated(ros *RolloutState) {
	ros.Status.OperationalState = rollout.RolloutStatus_RolloutOperationalState_name[int32(rollout.RolloutStatus_PROGRESSING)]
	//TODO Set the rollout Object Previous version to that of bundle version
	ros.setPreviousVersion("TODO_PREV_VERSION")
	if ros.Spec.GetSuspend() {
		log.Infof("Rollout object created with state SUSPENDED.")
		ros.Status.OperationalState = rollout.RolloutStatus_RolloutOperationalState_name[int32(rollout.RolloutStatus_SUSPENDED)]
		ros.eventChan <- fsmEvSuspend
		return
	}

	if ros.Spec.ScheduledStartTime == nil {
		ros.setStartTime()
	}

	if ros.Spec.SmartNICsOnly {
		ros.eventChan <- fsmEvVeniceBypass
	} else {
		ros.preCheckNextVeniceNode()
	}
}
func fsmAcOneVenicePreupgSuccess(ros *RolloutState) {
	numPendingPrecheck, err := ros.preCheckNextVeniceNode()
	if err != nil {
		log.Errorf("Error %s issuing precheck to next venice", err)
		return
	}
	if numPendingPrecheck == 0 { // all venice have been issued pre-check
		if !ros.allVenicePreCheckSuccess() {
			ros.eventChan <- fsmEvFail
		} else {
			ros.eventChan <- fsmEvAllVenicePreUpgOK
		}
	}
}

func fsmAcPreUpgSmartNIC(ros *RolloutState) {

	ros.writer.WriteRollout(ros.Rollout)
	ros.Add(1)

	go func() {
		defer ros.Done()
		ros.preUpgradeSmartNICs()
		if ros.numPreUpgradeFailures == 0 {
			ros.eventChan <- fsmEvAllSmartNICPreUpgOK
		} else {
			ros.eventChan <- fsmEvSomeSmartNICPreUpgFail
		}
	}()
}

func fsmAcWaitForSchedule(ros *RolloutState) {
	if ros.Spec.ScheduledStartTime == nil {
		ros.eventChan <- fsmEvScheduleNow
		return
	}
	t, err := ros.Spec.ScheduledStartTime.Time()
	now := time.Now()
	if err != nil || now.After(t) { // specified time is in the past
		ros.eventChan <- fsmEvScheduleNow
		return
	}
	d := t.Sub(now)
	time.Sleep(d)
	// TODO: Provide a way to cancel this when user Stops (or Deletes) Rollout
	ros.eventChan <- fsmEvScheduleNow
	return
}

func fsmAcIssueNextVeniceRollout(ros *RolloutState) {

	if ros.Spec.GetSuspend() {
		log.Infof("Rollout is SUSPENDED. Returning without further controller node Rollout.")
		ros.Status.OperationalState = rollout.RolloutStatus_RolloutOperationalState_name[int32(rollout.RolloutStatus_SUSPENDED)]
		ros.eventChan <- fsmEvSuspend
		return
	}
	if ros.Status.StartTime == nil {
		ros.setStartTime()
	}
	if ros.Spec.SmartNICsOnly {
		ros.eventChan <- fsmEvVeniceBypass
		return
	}
	numPendingRollout, err := ros.startNextVeniceRollout()
	if err != nil {
		log.Errorf("Error %s issuing rollout to next venice", err)
		return
	}

	if numPendingRollout == 0 {
		if !ros.allVeniceRolloutSuccess() { // some venice rollout failed
			ros.eventChan <- fsmEvFail
		} else {
			ros.eventChan <- fsmEvAllVeniceUpgOK
		}
	}
}
func fsmAcIssueServiceRollout(ros *RolloutState) {
	log.Infof("fsmAcIssueServiceRollout..")
	serviceRolloutPending, err := ros.issueServiceRollout()
	if err != nil {
		log.Errorf("Error %s issuing service rollout", err)
		return
	}

	log.Infof("fsmAcIssueServiceRollout.. checking pending service Rollout")
	if !serviceRolloutPending {
		log.Infof("issue fsmEvServiceUpgOK")
		ros.eventChan <- fsmEvServiceUpgOK
	}
	log.Infof("fsmAcIssueServiceRollout.. returning from fsmAcIssueServiceRollout")
}
func fsmAcRolloutSmartNICs(ros *RolloutState) {
	if ros.Spec.GetSuspend() {
		log.Infof("Rollout is SUSPENDED. Returning without smartNIC Rollout.")
		ros.Status.OperationalState = rollout.RolloutStatus_RolloutOperationalState_name[int32(rollout.RolloutStatus_SUSPENDED)]
		ros.eventChan <- fsmEvSuspend
		return
	}
	ros.writer.WriteRollout(ros.Rollout)
	ros.Add(1)
	go func() {
		defer ros.Done()
		ros.doUpdateSmartNICs()

		numFailures := atomic.LoadUint32(&ros.numFailuresSeen)
		if numFailures <= ros.Spec.MaxNICFailuresBeforeAbort {
			ros.eventChan <- fsmEvSuccess
		} else {
			ros.eventChan <- fsmEvFail
		}
	}()
}
func fsmAcRolloutSuccess(ros *RolloutState) {
	ros.setEndTime()
	ros.saveStatus()
}
func fsmAcRolloutFail(ros *RolloutState) {
	ros.setEndTime()
	ros.saveStatus()
}
func fsmAcRolloutSuspend(ros *RolloutState) {
	ros.setEndTime()
	ros.saveStatus()
}
