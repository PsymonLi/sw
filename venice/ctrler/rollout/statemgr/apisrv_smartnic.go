// {C} Copyright 2018 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"fmt"
	"reflect"
	"sync"

	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
)

// SmartNICState - Internal state for SmartNIC
type SmartNICState struct {
	Mutex sync.Mutex
	*cluster.DistributedServiceCard
	*Statemgr

	// Local information
}

func (sm *Statemgr) handleSmartNICEvent(et kvstore.WatchEventType, smartNIC *cluster.DistributedServiceCard) {
	switch et {
	case kvstore.Created, kvstore.Updated:
		log.Infof("SetSmartNICState - {%+v}\n", smartNIC)
		err := sm.SetSmartNICState(smartNIC)
		if err != nil {
			log.Errorf("Error SetSmartNICState SmartNIC {%+v}. Err: %v", smartNIC, err)
			return
		}
	case kvstore.Deleted:
		log.Infof("DeleteSmartNICState - %s\n", smartNIC.Name)

		sm.DeleteSmartNICState(smartNIC)
	}
}

// SetSmartNICState to create a SmartNIC Object/update smartNIC isf it already exists in statemgr
func (sm *Statemgr) SetSmartNICState(smartNIC *cluster.DistributedServiceCard) error {
	if smartNIC.GetObjectKind() != kindSmartNIC {
		return fmt.Errorf("unexpected object kind %s", smartNIC.GetObjectKind())
	}
	var smartNICState *SmartNICState

	// print the Log only when the health changes or if object is created
	printMsg := false

	// All parameters are validated (using apiserver hooks) by the time we get here
	obj, err := sm.FindObject(kindSmartNIC, smartNIC.Tenant, smartNIC.Name)
	if err == nil {
		smartNICState, err = SmartNICStateFromObj(obj)
		if err != nil {
			return err
		}
	} else {
		smartNICState = &SmartNICState{
			Statemgr: sm,
		}
		printMsg = true
	}

	smartNICState.Mutex.Lock()
	if !printMsg {
		if len(smartNICState.Status.Conditions) != len(smartNIC.Status.Conditions) {
			printMsg = true
		}
		if len(smartNIC.Status.Conditions) > 0 && len(smartNICState.Status.Conditions) > 0 &&
			smartNICState.Status.Conditions[0].Status != smartNIC.Status.Conditions[0].Status {
			printMsg = true
		}
		if !printMsg && !reflect.DeepEqual(smartNIC.Labels, smartNICState.Labels) {
			printMsg = true
		}
	}
	if printMsg {
		if len(smartNIC.Status.Conditions) > 0 {
			log.Infof("SetSmartNICState - %s Labels:%s Condition[%s]=%s\n", smartNIC.Name, smartNIC.Labels, smartNIC.Status.Conditions[0].Type, smartNIC.Status.Conditions[0].Status)
		} else {
			log.Infof("SetSmartNICState - %s Labels:%s Nil Conditions\n", smartNIC.Name, smartNIC.Labels)
		}
	}

	smartNICState.DistributedServiceCard = smartNIC
	smartNICState.Mutex.Unlock()

	sm.memDB.AddObject(smartNICState)
	return nil
}

// SmartNICStateFromObj converts from memdb object to SmartNIC state
func SmartNICStateFromObj(obj memdb.Object) (*SmartNICState, error) {
	switch nsobj := obj.(type) {
	case *SmartNICState:
		return nsobj, nil
	default:
		return nil, ErrIncorrectObjectType
	}
}

// ListSmartNICs lists all SmartNIC objects
func (sm *Statemgr) ListSmartNICs() ([]*SmartNICState, error) {
	objs := sm.memDB.ListObjects(kindSmartNIC, nil)

	var smartNICStates []*SmartNICState
	for _, obj := range objs {
		nso, err := SmartNICStateFromObj(obj)
		if err != nil {
			return smartNICStates, err
		}

		smartNICStates = append(smartNICStates, nso)
	}

	return smartNICStates, nil
}

// GetSmartNICState : Get the specified smartNIC state
func (sm *Statemgr) GetSmartNICState(tenant, name string) (*SmartNICState, error) {
	dbMs, err := sm.FindObject(kindSmartNIC, tenant, name)
	if err != nil {
		return nil, err
	}
	return dbMs.(*SmartNICState), nil
}

// DeleteSmartNICState - delete smartNIC
func (sm *Statemgr) DeleteSmartNICState(sn *cluster.DistributedServiceCard) {
	smartnicState, err := sm.GetSmartNICState(sn.Tenant, sn.Name)
	if err != nil {
		log.Debugf("Error deleting non-existent smartNIC {%+v}. Err: %v", sn, err)
		return
	}

	log.Infof("Deleting SmartNIC %v", smartnicState.DistributedServiceCard.Name)

	// TODO: may be set state to deleted and leave it db till all the watchers have come to reasonable state

	// delete smartNIC state from DB
	_ = sm.memDB.DeleteObject(smartnicState)
}
