// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package state

import (
	"errors"
	"fmt"

	"github.com/gogo/protobuf/proto"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/netagent/state/types"
	"github.com/pensando/sw/venice/ctrler/npm/rpcserver/netproto"
	"github.com/pensando/sw/venice/utils/log"
)

// CreateTCPProxyPolicy creates a tcp proxy policy
func (na *Nagent) CreateTCPProxyPolicy(tcp *netproto.TCPProxyPolicy) error {
	err := na.validateMeta(tcp.Kind, tcp.ObjectMeta)
	if err != nil {
		return err
	}
	// check if tcp proxy policy already exists
	oldTun, err := na.FindTCPProxyPolicy(tcp.ObjectMeta)
	if err == nil {
		// check if tcp proxy policy contents are same
		if !proto.Equal(oldTun, tcp) {
			log.Errorf("TCPProxyPolicy %+v already exists", oldTun)
			return errors.New("tcp proxy policy already exists")
		}

		log.Infof("Received duplicate tcp proxy policy create for {%+v}", tcp)
		return nil
	}
	// find the corresponding namespace
	ns, err := na.FindNamespace(tcp.Tenant, tcp.Namespace)
	if err != nil {
		return err
	}

	tcp.Status.TCPProxyPolicyID, err = na.Store.GetNextID(types.TCPProxyPolicyID)
	if err != nil {
		log.Errorf("Could not allocate tcp proxy policy id. {%+v}", err)
		return err
	}

	// create it in datapath
	err = na.Datapath.CreateTCPProxyPolicy(tcp, ns)
	if err != nil {
		log.Errorf("Error creating tcp proxy policy in datapath. Nw {%+v}. Err: %v", tcp, err)
		return err
	}

	// Add the current tcp proxy policy as a dependency to the namespace.
	err = na.Solver.Add(ns, tcp)
	if err != nil {
		log.Errorf("Could not add dependency. Parent: %v. Child: %v", ns, tcp)
		return err
	}

	// save it in db
	key := na.Solver.ObjectKey(tcp.ObjectMeta, tcp.TypeMeta)
	na.Lock()
	na.TCPProxyPolicyDB[key] = tcp
	na.Unlock()
	err = na.Store.Write(tcp)

	return err
}

// ListTCPProxyPolicy returns the list of tcp proxy policys
func (na *Nagent) ListTCPProxyPolicy() []*netproto.TCPProxyPolicy {
	var tcpProxyPolicyList []*netproto.TCPProxyPolicy

	// lock the db
	na.Lock()
	defer na.Unlock()

	// walk all tcp proxy policys
	for _, tcp := range na.TCPProxyPolicyDB {
		tcpProxyPolicyList = append(tcpProxyPolicyList, tcp)
	}

	return tcpProxyPolicyList
}

// FindTCPProxyPolicy dins a tcp proxy policy in local db
func (na *Nagent) FindTCPProxyPolicy(meta api.ObjectMeta) (*netproto.TCPProxyPolicy, error) {
	typeMeta := api.TypeMeta{
		Kind: "TCPProxyPolicy",
	}
	// lock the db
	na.Lock()
	defer na.Unlock()

	// lookup the database
	key := na.Solver.ObjectKey(meta, typeMeta)
	tcp, ok := na.TCPProxyPolicyDB[key]
	if !ok {
		return nil, fmt.Errorf("tcp proxy policy not found %v", meta.Name)
	}

	return tcp, nil
}

// UpdateTCPProxyPolicy updates a tcp proxy policy. ToDo implement tcp proxy policy updates in datapath
func (na *Nagent) UpdateTCPProxyPolicy(tcp *netproto.TCPProxyPolicy) error {
	// find the corresponding namespace
	ns, err := na.FindNamespace(tcp.Tenant, tcp.Namespace)
	if err != nil {
		return err
	}

	existingTCPProxyPolicy, err := na.FindTCPProxyPolicy(tcp.ObjectMeta)
	if err != nil {
		log.Errorf("TCPProxyPolicy %v not found", tcp.ObjectMeta)
		return err
	}

	if proto.Equal(tcp, existingTCPProxyPolicy) {
		log.Infof("Nothing to update.")
		return nil
	}

	err = na.Datapath.UpdateTCPProxyPolicy(existingTCPProxyPolicy, ns)
	if err != nil {
		log.Errorf("Error updating the tcp proxy policy {%+v} in datapath. Err: %v", existingTCPProxyPolicy, err)
		return err
	}
	key := na.Solver.ObjectKey(tcp.ObjectMeta, tcp.TypeMeta)
	na.Lock()
	na.TCPProxyPolicyDB[key] = tcp
	na.Unlock()
	err = na.Store.Write(tcp)
	return err
}

// DeleteTCPProxyPolicy deletes a tcp proxy policy. ToDo implement tcp proxy policy deletes in datapath
func (na *Nagent) DeleteTCPProxyPolicy(tn, namespace, name string) error {
	tcp := &netproto.TCPProxyPolicy{
		TypeMeta: api.TypeMeta{Kind: "TCPProxyPolicy"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    tn,
			Namespace: namespace,
			Name:      name,
		},
	}
	err := na.validateMeta(tcp.Kind, tcp.ObjectMeta)
	if err != nil {
		return err
	}
	// find the corresponding namespace
	ns, err := na.FindNamespace(tcp.Tenant, tcp.Namespace)
	if err != nil {
		return err
	}

	// check if existingTCPProxyPolicy already exists
	existingTCPProxyPolicy, err := na.FindTCPProxyPolicy(tcp.ObjectMeta)
	if err != nil {
		log.Errorf("TCPProxyPolicy %+v not found", tcp.ObjectMeta)
		return errors.New("tcp proxy policy not found")
	}

	// check if the current tcp proxy policy has any objects referring to it
	err = na.Solver.Solve(existingTCPProxyPolicy)
	if err != nil {
		log.Errorf("Found active references to %v. Err: %v", existingTCPProxyPolicy.Name, err)
		return err
	}

	// delete the existingTCPProxyPolicy in datapath
	err = na.Datapath.DeleteTCPProxyPolicy(existingTCPProxyPolicy, ns)
	if err != nil {
		log.Errorf("Error deleting tcp proxy policy {%+v}. Err: %v", tcp, err)
		return err
	}

	// update parent references
	err = na.Solver.Remove(ns, existingTCPProxyPolicy)
	if err != nil {
		log.Errorf("Could not remove the reference to the namespace: %v. Err: %v", ns.Name, err)
		return err
	}

	// delete from db
	key := na.Solver.ObjectKey(tcp.ObjectMeta, tcp.TypeMeta)
	na.Lock()
	delete(na.TCPProxyPolicyDB, key)
	na.Unlock()
	err = na.Store.Delete(tcp)

	return err
}
