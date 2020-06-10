package statemgr

import (
	"fmt"
	"sync"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/ctkit"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/venice/utils/kvstore"
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
	_, ok := sm.probeQs[w.Orchestrator.Name]
	if ok {
		return fmt.Errorf("vc probe channel already created")
	}

	err := sm.AddProbeChannel(w.Orchestrator.GetName())
	if err != nil {
		return err
	}
	sm.instanceManagerCh <- &kvstore.WatchEvent{Object: &w.Orchestrator, Type: kvstore.Created}
	o, err := NewOrchestratorState(w, sm)
	if err != nil {
		return err
	}

	return o.checkAndUpdateDSCList()
}

// OnOrchestratorUpdate handles update event
func (sm *Statemgr) OnOrchestratorUpdate(w *ctkit.Orchestrator, nw *orchestration.Orchestrator) error {
	sm.instanceManagerCh <- &kvstore.WatchEvent{Object: nw, Type: kvstore.Updated}
	_, err := OrchestratorStateFromObj(w)

	return err
}

// OnOrchestratorDelete deletes a orchestrator
func (sm *Statemgr) OnOrchestratorDelete(w *ctkit.Orchestrator) error {
	sm.instanceManagerCh <- &kvstore.WatchEvent{Object: &w.Orchestrator, Type: kvstore.Deleted}
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
	for d := range o.incompatibleDscs {
		newList = append(newList, d)
	}

	o.Orchestrator.Status.IncompatibleDSCs = newList
	return o.Orchestrator.Write()
}

// AddIncompatibleDSC adds DSC MAC address to orchestrator incompatible list
func (o *OrchestratorState) AddIncompatibleDSC(dsc string) error {
	o.Lock()
	defer o.Unlock()

	o.incompatibleDscs[dsc] = true
	return o.updateIncompatibleList()
}

// RemoveIncompatibleDSC removes DSC from orchestrator incompatible list
func (o *OrchestratorState) RemoveIncompatibleDSC(dsc string) error {
	o.Lock()
	defer o.Unlock()

	delete(o.incompatibleDscs, dsc)
	return o.updateIncompatibleList()
}

func (o *OrchestratorState) checkAndUpdateDSCList() error {
	for dsc := range o.incompatibleDscs {
		dscState := o.stateMgr.FindDSC(dsc, "")
		// If DSC is now compatible, remove from the Incompatible list
		if dscState.isOrchestratorCompatible() {
			delete(o.incompatibleDscs, dsc)
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
