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

// CreateNatPolicy creates a nat policy
func (na *Nagent) CreateNatPolicy(np *netproto.NatPolicy) error {
	// protectCurrentNS needs to be set to false if there is atlease one dependent nat pool in the same namespace
	protectCurrentNS := true
	var dependentNatPools []*netproto.NatPool
	err := na.validateMeta(np.Kind, np.ObjectMeta)
	if err != nil {
		return err
	}
	oldNp, err := na.FindNatPolicy(np.ObjectMeta)
	if err == nil {
		// check if the contents are same
		if !proto.Equal(oldNp, np) {
			log.Errorf("NatPolicy %+v already exists", oldNp)
			return errors.New("nat policy already exists")
		}

		log.Infof("Received duplicate nat policy create {%+v}", np)
		return nil
	}

	// find the corresponding namespace
	ns, err := na.FindNamespace(np.Tenant, np.Namespace)
	if err != nil {
		return err
	}

	for _, rule := range np.Spec.Rules {
		rule.ID, err = na.Store.GetNextID(types.NatRuleID)
		natPool, err := na.findNatPool(np.ObjectMeta, rule.NatPool)
		if err != nil {
			log.Errorf("could not find nat pool for the rule. Rule: {%v}. Err: %v", rule, err)
			return err
		}
		natPoolNS, err := na.FindNamespace(np.Tenant, natPool.Namespace)
		if err != nil {
			log.Errorf("could not find the nat pool namespace. NatPool Namespace: {%v}. Err: %v", natPoolNS, err)
			return err
		}
		poolID := &types.NatPoolRef{
			NamespaceID: natPoolNS.Status.NamespaceID,
			PoolID:      natPool.Status.NatPoolID,
		}
		na.NatPoolLUT[rule.NatPool] = poolID
		dependentNatPools = append(dependentNatPools, natPool)
		if natPoolNS.Status.NamespaceID == ns.Status.NamespaceID {
			protectCurrentNS = false
		}
	}

	np.Status.NatPolicyID, err = na.Store.GetNextID(types.NatPolicyID)

	if err != nil {
		log.Errorf("Could not allocate nat policy id. {%+v}", err)
		return err
	}

	// create it in datapath
	err = na.Datapath.CreateNatPolicy(np, na.NatPoolLUT, ns)
	if err != nil {
		log.Errorf("Error creating nat policy in datapath. NatPolicy {%+v}. Err: %v", np, err)
		return err
	}

	// Add references to all the nat pools that the nat policy depends on.
	for _, n := range dependentNatPools {
		// Add the current Nat Pool as a dependency to the Nat Policy.
		err = na.Solver.Add(n, np)
		if err != nil {
			log.Errorf("Could not add dependency. Parent: %v. Child: %v", n, np)
			return err
		}
	}

	// Check if we need to protect the current namespace from deletion. This is true if none of the dependent nat pools
	// refer to the namespace of the nat policy
	if protectCurrentNS {
		// Add the current Namespace as a dependency to the Nat Policy.
		err = na.Solver.Add(ns, np)
		if err != nil {
			log.Errorf("Could not add dependency. Parent: %v. Child: %v", ns, np)
			return err
		}
	}

	// save it in db
	key := na.Solver.ObjectKey(np.ObjectMeta, np.TypeMeta)
	na.Lock()
	na.NatPolicyDB[key] = np
	na.Unlock()
	err = na.Store.Write(np)

	return err
}

// FindNatPolicy finds a nat policy in local db
func (na *Nagent) FindNatPolicy(meta api.ObjectMeta) (*netproto.NatPolicy, error) {
	typeMeta := api.TypeMeta{
		Kind: "NatPolicy",
	}
	// lock the db
	na.Lock()
	defer na.Unlock()

	// lookup the database
	key := na.Solver.ObjectKey(meta, typeMeta)
	np, ok := na.NatPolicyDB[key]
	if !ok {
		return nil, fmt.Errorf("nat policy not found %v", meta.Name)
	}

	return np, nil
}

// ListNatPolicy returns the list of nat policys
func (na *Nagent) ListNatPolicy() []*netproto.NatPolicy {
	var natPolicyList []*netproto.NatPolicy
	// lock the db
	na.Lock()
	defer na.Unlock()

	for _, np := range na.NatPolicyDB {
		natPolicyList = append(natPolicyList, np)
	}

	return natPolicyList
}

// UpdateNatPolicy updates a nat policy
func (na *Nagent) UpdateNatPolicy(np *netproto.NatPolicy) error {
	// find the corresponding namespace
	ns, err := na.FindNamespace(np.Tenant, np.Namespace)
	if err != nil {
		return err
	}
	existingNp, err := na.FindNatPolicy(np.ObjectMeta)
	if err != nil {
		log.Errorf("NatPolicy %v not found", np.ObjectMeta)
		return err
	}

	if proto.Equal(np, existingNp) {
		log.Infof("Nothing to update.")
		return nil
	}

	err = na.Datapath.UpdateNatPolicy(np, ns)
	key := na.Solver.ObjectKey(np.ObjectMeta, np.TypeMeta)
	na.Lock()
	na.NatPolicyDB[key] = np
	na.Unlock()
	err = na.Store.Write(np)
	return err
}

// DeleteNatPolicy deletes a nat policy
func (na *Nagent) DeleteNatPolicy(np *netproto.NatPolicy) error {
	protectCurrentNS := true
	err := na.validateMeta(np.Kind, np.ObjectMeta)
	if err != nil {
		return err
	}
	// find the corresponding namespace
	ns, err := na.FindNamespace(np.Tenant, np.Namespace)
	if err != nil {
		return err
	}

	existingNatPolicy, err := na.FindNatPolicy(np.ObjectMeta)
	if err != nil {
		log.Errorf("NatPolicy %+v not found", np.ObjectMeta)
		return errors.New("nat policy not found")
	}

	// check if the current nat policy has any objects referring to it
	err = na.Solver.Solve(existingNatPolicy)
	if err != nil {
		log.Errorf("Found active references to %v. Err: %v", existingNatPolicy.Name, err)
		return err
	}

	// delete it in the datapath
	err = na.Datapath.DeleteNatPolicy(existingNatPolicy, ns)
	if err != nil {
		log.Errorf("Error deleting nat policy {%+v}. Err: %v", np, err)
	}

	// Remove parent references

	for _, rule := range existingNatPolicy.Spec.Rules {
		natPool, err := na.findNatPool(np.ObjectMeta, rule.NatPool)
		if err != nil {
			log.Errorf("could not find nat pool for the rule. Rule: {%v}. Err: %v", rule, err)
			return err
		}
		natPoolNS, err := na.FindNamespace(np.Tenant, natPool.Namespace)
		if err != nil {
			log.Errorf("could not find the nat pool namespace. NatPool Namespace: {%v}. Err: %v", natPoolNS, err)
			return err
		}
		if natPoolNS.Status.NamespaceID == ns.Status.NamespaceID {
			protectCurrentNS = false
		}
		// Remove the reference to the current nat pool
		err = na.Solver.Remove(natPool, existingNatPolicy)
		if err != nil {
			log.Errorf("Could not remove the reference to the nat pool: %v. Err: %v", natPool.Name, existingNatPolicy)
			return err
		}
	}

	// protectCurrentNS is true if we have added a dependency to the current namespace of the nat policy.
	// In such a case we should remove it during deletion
	if protectCurrentNS {
		err = na.Solver.Remove(ns, existingNatPolicy)
		if err != nil {
			log.Errorf("Could not remove the reference to the namespace: %v. Err: %v", ns.Name, existingNatPolicy)
			return err
		}
	}

	// delete from db
	key := na.Solver.ObjectKey(np.ObjectMeta, np.TypeMeta)
	na.Lock()
	delete(na.NatPolicyDB, key)
	na.Unlock()
	err = na.Store.Delete(np)

	return err
}
