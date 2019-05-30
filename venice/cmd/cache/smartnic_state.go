// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package cache

import (
	"context"
	"errors"
	"fmt"
	"strings"
	"sync"

	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/utils"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
)

// SmartNICState security policy state
type SmartNICState struct {
	*sync.RWMutex
	*cluster.SmartNIC // smartnic policy object
}

// SmartNICStateFromObj converts from memdb object to SmartNIC state
func SmartNICStateFromObj(obj memdb.Object) (*SmartNICState, error) {
	switch obj.(type) {
	case *SmartNICState:
		snobj := obj.(*SmartNICState)
		return snobj, nil
	default:
		return nil, errors.New("Incorrect object type")
	}
}

// NewSmartNICState creates a new security policy state object
func NewSmartNICState(sn *cluster.SmartNIC) (*SmartNICState, error) {
	// create smartnic state object
	sns := SmartNICState{
		SmartNIC: sn,
		RWMutex:  new(sync.RWMutex),
	}

	return &sns, nil
}

// FindSmartNIC finds smartNIC object by name
func (sm *Statemgr) FindSmartNIC(name string) (*SmartNICState, error) {
	// find the object
	obj, err := sm.FindObject("SmartNIC", "", name)
	if err != nil {
		return nil, err
	}

	return SmartNICStateFromObj(obj)
}

// ListSmartNICs lists all smartNIC objects
func (sm *Statemgr) ListSmartNICs() ([]*SmartNICState, error) {
	objs := sm.memDB.ListObjects("SmartNIC")

	var sgs []*SmartNICState
	for _, obj := range objs {
		sg, err := SmartNICStateFromObj(obj)
		if err != nil {
			return sgs, err
		}

		sgs = append(sgs, sg)
	}

	return sgs, nil
}

// CreateSmartNIC creates a smartNIC object
func (sm *Statemgr) CreateSmartNIC(sn *cluster.SmartNIC, writeback bool) (*SmartNICState, error) {
	// see if we already have it
	esn, err := sm.FindSmartNIC(sn.ObjectMeta.Name)
	if err == nil {
		// Object exists in cache, but we got a watcher event with event-type:Created
		// and this can happen if there is a watcher error/reset and we need to update
		// the cache to handle it gracefully as an Update.
		esn.Lock()
		defer esn.Unlock()
		log.Infof("Objects exists, updating smartNIC OldState: {%+v}. New state: {%+v}", esn, sn)
		return esn, sm.UpdateSmartNIC(sn, writeback)
	}

	if sn.Spec.ID == "" {
		err = fmt.Errorf("Error creating new smartnic state: SmartNIC has empty hostname: %+v", sn)
		log.Errorf(err.Error())
		return nil, err
	}

	// create new smartnic state
	sns, err := NewSmartNICState(sn)
	if err != nil {
		log.Errorf("Error creating new smartnic state. Err: %v", err)
		return nil, err
	}

	sns.Lock()
	defer sns.Unlock()

	// Rejected SmartNIC objects can have duplicate hostnames.
	// In this case we don't update the map
	if sn.Status.AdmissionPhase != cluster.SmartNICStatus_REJECTED.String() {
		sm.hostnameToSmartNICMapLock.Lock()
		nic, ok := sm.hostnameToSmartNICMap[sn.Spec.ID]
		if ok || nic != nil {
			log.Errorf("Error updating hostnameToSmartNICMap, key %s, exists: %v, value:%+v", sn.Spec.ID, ok, nic)
			// continue anyway
		}
		sm.hostnameToSmartNICMap[sn.Spec.ID] = sn
		sm.hostnameToSmartNICMapLock.Unlock()
	}

	// store it in local DB
	err = sm.memDB.AddObject(sns)
	if err != nil {
		log.Errorf("Error storing the smartnic in memdb. Err: %v", err)
		return nil, err
	}

	if writeback {
		// For creates we want to use a generous timeout and a couple of retries because
		// if we fail to create the SmartNIC object then we need to return an error to NMD
		// and have it retry
		f := func(ctx context.Context) (interface{}, error) {
			nic, err := sm.APIClient().SmartNIC().Create(ctx, sn)
			return nic, err
		}
		_, err := utils.ExecuteWithRetry(f, apiServerRPCTimeout, maxAPIServerWriteRetries)
		if err != nil && !strings.Contains(err.Error(), "exists") {
			log.Errorf("Error creating SmartNIC object %+v: %v", sn.ObjectMeta, err)
			return nil, fmt.Errorf("Error creating SmartNIC object")
		}
	}

	log.Infof("Created SmartNIC state {%+v}, writeback: %v", sns, writeback)
	return sns, nil
}

// UpdateSmartNIC updates a smartNIC object
// Caller is responsible for acquiring the lock before invocation and releasing it afterwards
func (sm *Statemgr) UpdateSmartNIC(updObj *cluster.SmartNIC, writeback bool) error {
	obj, err := sm.FindObject("SmartNIC", "", updObj.ObjectMeta.Name)
	if err != nil {
		log.Errorf("Can not find the smartnic %s err: %v", updObj.ObjectMeta.Name, err)
		return fmt.Errorf("SmartNIC not found")
	}

	if updObj.Spec.ID == "" {
		err = fmt.Errorf("Error updating smartnic state: SmartNIC has empty hostname: %+v", updObj)
		log.Errorf(err.Error())
		return err
	}

	cachedState, err := SmartNICStateFromObj(obj)
	if err != nil {
		log.Errorf("Wrong object type in memdb! Expected SmartNIC, got %T", obj)
		return err
	}

	sm.hostnameToSmartNICMapLock.Lock()
	if cachedState.Spec.ID != updObj.Spec.ID &&
		cachedState.Status.AdmissionPhase != cluster.SmartNICStatus_REJECTED.String() {
		_, ok := sm.hostnameToSmartNICMap[cachedState.Spec.ID]
		if !ok || cachedState.Spec.ID == "" {
			log.Errorf("Error updating hostnameToSmartNICMap, key %s does not exist or is empty", cachedState.Spec.ID)
			// continue anyway
		}
		delete(sm.hostnameToSmartNICMap, cachedState.Spec.ID)
	}
	// update hostnameToSmartNICMap entry even if there is no hostname change so the map
	// and the cache stay in sync and we don't keep old objects around
	if updObj.Status.AdmissionPhase != cluster.SmartNICStatus_REJECTED.String() {
		sm.hostnameToSmartNICMap[updObj.Spec.ID] = updObj
	}
	sm.hostnameToSmartNICMapLock.Unlock()

	cachedState.SmartNIC = updObj

	// store it in local DB
	err = sm.memDB.UpdateObject(cachedState)
	if err != nil {
		log.Errorf("Error storing smartnic in memdb. Err: %v", err)
		return err
	}

	if writeback {
		nicObj := updObj
		ok := false
		for i := 0; i < maxAPIServerWriteRetries; i++ {
			ctx, cancel := context.WithTimeout(context.Background(), apiServerRPCTimeout)
			_, err = sm.APIClient().SmartNIC().Update(ctx, nicObj)
			if err == nil {
				ok = true
				cancel()
				log.Infof("Updated SmartNIC object in ApiServer: %+v", nicObj)
				break
			}
			log.Errorf("Error updating SmartNIC object %+v: %v", nicObj.ObjectMeta, err)
			// Write error -- fetch updated Spec + Meta and retry
			updObj, err := sm.APIClient().SmartNIC().Get(ctx, &nicObj.ObjectMeta)
			if err == nil {
				updObj.Status = nicObj.Status
				nicObj = updObj
				// retain Status as that's what we are trying to update
			}
			cancel()
		}
		if !ok {
			log.Errorf("Error updating SmartNIC object %+v in ApiServer, retries exhausted", nicObj.ObjectMeta)
		}
	}

	log.Debugf("Updated SmartNIC state {%+v}", cachedState)
	return nil
}

// DeleteSmartNIC deletes a smartNIC state
func (sm *Statemgr) DeleteSmartNIC(sn *cluster.SmartNIC) error {
	if sn.Status.AdmissionPhase != cluster.SmartNICStatus_REJECTED.String() {
		sm.hostnameToSmartNICMapLock.Lock()
		_, ok := sm.hostnameToSmartNICMap[sn.Spec.ID]
		if !ok || sn.Spec.ID == "" {
			log.Errorf("Error updating hostnameToSmartNICMap, key %s does not exist or is empty", sn.Spec.ID)
			// continue anyway
		}
		delete(sm.hostnameToSmartNICMap, sn.Spec.ID)
		sm.hostnameToSmartNICMapLock.Unlock()
	}

	// delete it from the DB
	err := sm.memDB.DeleteObject(sn)
	if err != nil {
		log.Errorf("Error deleting SmartNIC state %+v: %v", sn.ObjectMeta, err)
		return err
	}

	// Deletes are never written back to ApiServer
	log.Infof("Deleted SmartNIC state {%+v}", sn.ObjectMeta)
	return nil
}

// GetSmartNICByID returns the SmartNIC object for a given hostname
func (sm *Statemgr) GetSmartNICByID(hostname string) *cluster.SmartNIC {
	sm.hostnameToSmartNICMapLock.RLock()
	defer sm.hostnameToSmartNICMapLock.RUnlock()
	return sm.hostnameToSmartNICMap[hostname]
}
