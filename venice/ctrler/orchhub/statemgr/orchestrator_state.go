package statemgr

import (
	"fmt"
	"sync"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/ref"
	"github.com/pensando/sw/venice/utils/runtime"
)

// OrchestratorState is a wrapper for orchestration object
type OrchestratorState struct {
	sync.Mutex
	Orchestrator     *ctkit.Orchestrator
	stateMgr         *Statemgr
	incompatibleDscs map[string]bool
}

//GetOrchestratorWatchOptions gets options
func (sm *Statemgr) GetOrchestratorWatchOptions() *api.ListWatchOptions {
	opts := api.ListWatchOptions{}
	opts.FieldChangeSelector = []string{"Spec"}
	return &opts
}

// OnOrchestratorCreate creates a orchestrator based on watch event
func (sm *Statemgr) OnOrchestratorCreate(w *ctkit.Orchestrator) error {
	if sm.RestoreActive {
		sm.logger.Infof("Snapshot restore active, skipping orch event")
		return nil
	}
	_, ok := sm.probeQs[w.Orchestrator.Name]
	if ok {
		sm.logger.Errorf("vc probe channel already created")
		return nil
	}

	err := sm.AddProbeChannel(w.Orchestrator.GetName())
	if err != nil {
		return err
	}
	// Send copy
	obj := ref.DeepCopy(w.Orchestrator).(orchestration.Orchestrator)
	sm.instanceManagerCh <- &kvstore.WatchEvent{Object: &obj, Type: kvstore.Created}
	o, err := NewOrchestratorState(w, sm)
	if err != nil {
		return err
	}

	return o.checkAndUpdateDSCList()
}

// OnOrchestratorUpdate handles update event
func (sm *Statemgr) OnOrchestratorUpdate(w *ctkit.Orchestrator, nw *orchestration.Orchestrator) error {
	obj := ref.DeepCopy(*nw).(orchestration.Orchestrator)
	sm.instanceManagerCh <- &kvstore.WatchEvent{Object: &obj, Type: kvstore.Updated}
	o, err := OrchestratorStateFromObj(w)
	if err != nil {
		return err
	}
	o.Orchestrator.Spec = nw.Spec
	return o.checkAndUpdateDSCList()
}

// OnOrchestratorDelete deletes a orchestrator
func (sm *Statemgr) OnOrchestratorDelete(w *ctkit.Orchestrator) error {
	obj := ref.DeepCopy(w.Orchestrator).(orchestration.Orchestrator)
	sm.instanceManagerCh <- &kvstore.WatchEvent{Object: &obj, Type: kvstore.Deleted}
	err := sm.RemoveProbeChannel(w.Orchestrator.Name)
	return err
}

// OnOrchestratorReconnect is called when ctkit reconnects to apiserver
func (sm *Statemgr) OnOrchestratorReconnect() {
	return
}

// OrchestratorStateFromObj converts from memdb object to orchestration state
func OrchestratorStateFromObj(obj runtime.Object) (*OrchestratorState, error) {
	switch obj.(type) {
	case *ctkit.Orchestrator:
		nobj := obj.(*ctkit.Orchestrator)
		switch nobj.HandlerCtx.(type) {
		case *OrchestratorState:
			nts := nobj.HandlerCtx.(*OrchestratorState)
			return nts, nil
		default:
			return nil, fmt.Errorf("Wrong type")
		}
	default:
		return nil, fmt.Errorf("Wrong type")
	}
}

// NewOrchestratorState create new orchestration state
func NewOrchestratorState(wrk *ctkit.Orchestrator, stateMgr *Statemgr) (*OrchestratorState, error) {
	w := &OrchestratorState{
		Orchestrator:     wrk,
		stateMgr:         stateMgr,
		incompatibleDscs: make(map[string]bool),
	}

	for _, dsc := range wrk.Orchestrator.Status.IncompatibleDSCs {
		w.incompatibleDscs[dsc] = true
	}

	wrk.HandlerCtx = w

	return w, nil
}

func (o *OrchestratorState) updateIncompatibleList() error {
	newList := []string{}
	for d, ok := range o.incompatibleDscs {
		if ok {
			newList = append(newList, d)
		}
	}

	o.Orchestrator.Status.IncompatibleDSCs = newList
	return o.Orchestrator.Write()
}

// AddIncompatibleDSC adds DSC MAC address to orchestrator incompatible list
func (o *OrchestratorState) AddIncompatibleDSC(dsc string) error {
	o.Orchestrator.Lock()
	defer o.Orchestrator.Unlock()
	o.Lock()
	defer o.Unlock()

	o.incompatibleDscs[dsc] = true
	return o.updateIncompatibleList()
}

// RemoveIncompatibleDSC removes DSC from orchestrator incompatible list
func (o *OrchestratorState) RemoveIncompatibleDSC(dsc string) error {
	o.Orchestrator.Lock()
	defer o.Orchestrator.Unlock()
	o.Lock()
	defer o.Unlock()

	o.incompatibleDscs[dsc] = false
	return o.updateIncompatibleList()
}

func (o *OrchestratorState) checkAndUpdateDSCList() error {
	// called from onOrchestratorCreate, do NOT take ctkit object lock
	o.Lock()
	defer o.Unlock()
	o.stateMgr.logger.Infof("Check and update DSC list %v", o.incompatibleDscs)

	for dsc := range o.incompatibleDscs {
		dscState := o.stateMgr.FindDSC(dsc, "")
		if dscState == nil {
			o.stateMgr.logger.Errorf("Failed to find DSC %v in statemgr.", dsc)
			o.incompatibleDscs[dsc] = true
			continue
		}

		// If DSC is now compatible, remove from the Incompatible list
		if err := dscState.isOrchestratorCompatible(); err == nil {
			o.incompatibleDscs[dsc] = false
		} else {
			o.stateMgr.logger.Infof("DSC %v is incompatible. Err : %v", dsc, err)
			o.incompatibleDscs[dsc] = true
		}
	}

	return o.updateIncompatibleList()
}

// AddIncompatibleDSCToOrch add incompat dsc to orch
func (sm *Statemgr) AddIncompatibleDSCToOrch(dsc, orch string) error {
	obj, err := sm.FindObject("Orchestrator", "", "", orch)
	if err != nil {
		sm.logger.Errorf("Failed to find object. Err :%v", err)
		return err
	}

	oState, err := OrchestratorStateFromObj(obj)
	if err != nil {
		return err
	}

	oState.AddIncompatibleDSC(dsc)
	return nil
}

// RemoveIncompatibleDSCFromOrch remove incompat dsc from orch
func (sm *Statemgr) RemoveIncompatibleDSCFromOrch(dsc, orch string) error {
	obj, err := sm.FindObject("Orchestrator", "", "", orch)
	if err != nil {
		sm.logger.Errorf("Failed to find object. Err :%v", err)
		return err
	}

	oState, err := OrchestratorStateFromObj(obj)
	if err != nil {
		return err
	}

	oState.RemoveIncompatibleDSC(dsc)
	return nil
}

// FindOrchestrator gets the orchestrator object from statemanager
func (sm *Statemgr) FindOrchestrator(orchName string) (*OrchestratorState, error) {
	// TODO : Make this function tenanted
	orch, err := sm.FindObject("Orchestrator", "", "", orchName)
	if err != nil {
		return nil, err
	}

	oState, err := OrchestratorStateFromObj(orch)
	if err != nil {
		return nil, err
	}

	return oState, nil
}
