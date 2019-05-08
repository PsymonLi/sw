// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package cache

import (
	"context"
	"errors"
	"fmt"
	"sync"

	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/memdb"
)

// HostState security policy state
type HostState struct {
	*sync.Mutex
	*cluster.Host // host object
}

// HostStateFromObj converts from memdb object to Host state
func HostStateFromObj(obj memdb.Object) (*HostState, error) {
	switch obj.(type) {
	case *HostState:
		snobj := obj.(*HostState)
		return snobj, nil

	default:
		return nil, errors.New("Incorrect object type")
	}
}

// NewHostState creates a new security policy state object
func NewHostState(sn *cluster.Host) (*HostState, error) {
	// create host state object
	sns := HostState{
		Host:  sn,
		Mutex: new(sync.Mutex),
	}
	return &sns, nil
}

// FindHost finds Host object by name
func (sm *Statemgr) FindHost(name string) (*HostState, error) {
	// find the object
	obj, err := sm.FindObject("Host", "", name)
	if err != nil {
		return nil, err
	}
	return HostStateFromObj(obj)
}

// ListHosts lists all Host objects
func (sm *Statemgr) ListHosts() ([]*HostState, error) {
	var sgs []*HostState
	objs := sm.memDB.ListObjects("Host")
	for _, obj := range objs {
		sg, err := HostStateFromObj(obj)
		if err != nil {
			return sgs, err
		}

		sgs = append(sgs, sg)
	}
	return sgs, nil
}

// CreateHost creates a Host object
func (sm *Statemgr) CreateHost(host *cluster.Host) (*HostState, error) {
	// see if we already have it
	ehs, err := sm.FindHost(host.ObjectMeta.Name)
	if err == nil {
		// Object exists in cache, but we got a watcher event with event-type:Created
		// and this can happen if there is a watcher error/reset and we need to update
		// the cache to handle it gracefully as an Update.
		log.Infof("Objects exists, updating Host OldState: {%+v}. New state: {%+v}", ehs, host)
		ehs.Lock()
		defer ehs.Unlock()
		return ehs, sm.UpdateHost(host, false)
	}

	// create new Host state
	hostState, err := NewHostState(host)
	if err != nil {
		log.Errorf("Error creating new Host state. Err: %v", err)
		return nil, err
	}

	// store it in local DB
	err = sm.memDB.AddObject(hostState)
	if err != nil {
		log.Errorf("Error storing the Host state in memdb. Err: %v", err)
		return nil, err
	}

	// Host creates are never written back to ApiServer
	log.Infof("Created Host state {%+v}", host.ObjectMeta)
	return hostState, nil
}

// UpdateHost updates a Host object
// Caller is responsible for acquiring the lock before invocation and releasing it afterwards
func (sm *Statemgr) UpdateHost(host *cluster.Host, writeback bool) error {
	obj, err := sm.FindObject("Host", "", host.ObjectMeta.Name)
	if err != nil {
		log.Errorf("Can not find the Host %s err: %v", host.ObjectMeta.Name, err)
		return fmt.Errorf("Host not found")
	}

	hostState, err := HostStateFromObj(obj)
	if err != nil {
		log.Errorf("Wrong object type in memdb! Expected Host, got %T", obj)
		return err
	}
	hostState.Host = host

	// store it in local DB
	err = sm.memDB.UpdateObject(hostState)
	if err != nil {
		log.Errorf("Error storing Host in memdb. Err: %v", err)
		return err
	}

	if writeback {
		hostObj := host
		ok := false
		for i := 0; i < maxAPIServerWriteRetries; i++ {
			ctx, cancel := context.WithTimeout(context.Background(), apiServerRPCTimeout)
			_, err = sm.APIClient().Host().Update(ctx, hostObj)
			if err == nil {
				ok = true
				log.Infof("Updated Host object in ApiServer: %+v", hostObj)
				cancel()
				break
			}
			log.Errorf("Error updating Host object %+v: %v", hostObj.ObjectMeta, err)
			// Write error -- fetch updated Spec + Meta and retry
			updObj, err := sm.APIClient().Host().Get(ctx, &hostObj.ObjectMeta)
			if err == nil {
				// retain Status as that's what we are trying to update
				updObj.Status = hostObj.Status
				hostObj = updObj
			}
			cancel()
		}
		if !ok {
			log.Errorf("Error updating Host object %+v in ApiServer, retries exhausted", hostObj.ObjectMeta)
		}
	}

	log.Debugf("Updated Host state {%+v}", host)
	return nil
}

// DeleteHost deletes a Host state
func (sm *Statemgr) DeleteHost(host *cluster.Host) error {
	// delete it from the DB
	err := sm.memDB.DeleteObject(host)
	if err != nil {
		log.Errorf("Error deleting Host state %+v: %v", host.ObjectMeta, err)
		return err
	}

	// Host deletes are never written back to ApiServer
	log.Infof("Deleted Host state {%+v}", host.ObjectMeta)
	return nil
}
