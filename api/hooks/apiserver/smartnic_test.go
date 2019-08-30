// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package impl

import (
	"context"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"fmt"
	"reflect"
	"strings"
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/api/utils"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/certs"
	"github.com/pensando/sw/venice/utils/ctxutils"
	"github.com/pensando/sw/venice/utils/diagnostics"
	"github.com/pensando/sw/venice/utils/kvstore/store"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/runtime"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestSmartNICObjectPreCommitHooks(t *testing.T) {
	hooks := &clusterHooks{
		logger: log.SetConfig(log.GetDefaultConfig("SmartNIC-Hooks-Precommit-Test")),
	}
	_, _, err := hooks.smartNICPreCommitHook(context.TODO(), nil, nil, "", apiintf.UpdateOper, false, nil)
	Assert(t, err != nil, "smartNICPreCommitHook did not return error with invalid parameters")
	_, _, err = hooks.hostPreCommitHook(context.TODO(), nil, nil, "key", apiintf.DeleteOper, false, "")
	Assert(t, err != nil, "smartNICPreCommitHook did not return error with invalid parameters")

	schema := runtime.GetDefaultScheme()
	config := store.Config{Type: store.KVStoreTypeMemkv, Servers: []string{""}, Codec: runtime.NewJSONCodec(schema)}
	kv, err := store.New(config)
	AssertOk(t, err, "Error instantiating KVStore")

	nic := cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{
			Kind: "DistributedServiceCard",
		},
		ObjectMeta: api.ObjectMeta{
			Name: "00ae.cd01.0001",
		},
		Spec: cluster.DistributedServiceCardSpec{
			ID: "hostname",
			IPConfig: &cluster.IPConfig{
				IPAddress: "0.0.0.0/0",
			},
			MgmtMode:    cluster.DistributedServiceCardSpec_NETWORK.String(),
			NetworkMode: cluster.DistributedServiceCardSpec_OOB.String(),
		},
		Status: cluster.DistributedServiceCardStatus{
			AdmissionPhase: "UNKNOWN",
			SerialNum:      "TestNIC",
			PrimaryMAC:     "00ae.cd01.0001",
			IPConfig: &cluster.IPConfig{
				IPAddress: "192.168.10.3/32",
			},
		},
	}

	var decomNICUpd cluster.DistributedServiceCard
	nic.Clone(&decomNICUpd)

	ctx := context.TODO()
	key := nic.MakeKey(string(apiclient.GroupCluster))

	err = kv.Create(ctx, key, &nic)
	AssertOk(t, err, fmt.Sprintf("Error creating object in KVStore"))

	// Test delete/decommission restrictions
	// create module for testing smartnic update/delete
	modules := diagnostics.NewNaplesModules(nic.Name, "", "V1")
	kv.Create(ctx, modules[0].MakeKey(string(apiclient.GroupDiagnostics)), modules[0])

	for _, phase := range cluster.DistributedServiceCardStatus_Phase_name {
		nic.Status.AdmissionPhase = phase
		err = kv.Update(ctx, key, &nic)
		AssertOk(t, err, fmt.Sprintf("Error updating object in KVStore, phase: %s", phase))

		// Create and update should always go through
		txn := kv.NewTxn()
		_, r, err := hooks.smartNICPreCommitHook(ctx, kv, txn, key, apiintf.CreateOper, false, nic)
		Assert(t, r == true && err == nil, "smartNICPreCommitHook returned unexpected error, op: CREATE, phase: %s", phase)
		Assert(t, !txn.IsEmpty(), "transaction should contain module object")

		txn = kv.NewTxn()
		_, r, err = hooks.smartNICPreCommitHook(ctx, kv, txn, key, apiintf.UpdateOper, false, nic)
		Assert(t, r == true && err == nil, "smartNICPreCommitHook returned unexpected error, op: UPDATE, phase: %s", phase)
		Assert(t, !txn.IsEmpty(), "transaction should contain module object")

		// MgmtMode change only goes through if phase == ADMITTED
		txn = kv.NewTxn()
		_, r, err = hooks.smartNICPreCommitHook(ctx, kv, txn, key, apiintf.UpdateOper, false, decomNICUpd)
		Assert(t, r == true && (err == nil || phase != cluster.DistributedServiceCardStatus_ADMITTED.String()),
			"smartNICPreCommitHook returned unexpected result on mode change, op: UPDATE, phase: %s, err: %v", phase, err)
		Assert(t, !txn.IsEmpty(), "transaction should contain module object")

		// delete only goes through if phase != ADMITTED
		txn = kv.NewTxn()
		_, r, err = hooks.smartNICPreCommitHook(ctx, kv, txn, key, apiintf.DeleteOper, false, nil)
		Assert(t, r == true && (err == nil || phase == cluster.DistributedServiceCardStatus_ADMITTED.String()),
			"smartNICPreCommitHook returned unexpected result, op: DELETE, phase: %s, err: %v", phase, err)
		Assert(t, phase == cluster.DistributedServiceCardStatus_ADMITTED.String() || !txn.IsEmpty(), "transaction should contain module object")
	}
	// Test unsupported Spec modifications
	tlsKey, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	AssertOk(t, err, "Error generating key")
	tlsCert, err := certs.SelfSign(globals.APIGw, tlsKey, certs.WithValidityDays(1))
	AssertOk(t, err, "Error generating cert")
	userCtx := ctxutils.MakeMockContext(nil, tlsCert)
	Assert(t, apiutils.IsUserRequestCtx(userCtx), "userCtx is not a user request context")

	otherCtx := context.TODO()
	nic.Status.AdmissionPhase = cluster.DistributedServiceCardStatus_PENDING.String()
	err = kv.Update(ctx, key, &nic)
	AssertOk(t, err, "Error updating NIC")

	type ModTest struct {
		fieldName string
		fieldTx   func(cluster.DistributedServiceCard) cluster.DistributedServiceCard
	}

	modTests := []ModTest{
		{"ID", func(n cluster.DistributedServiceCard) cluster.DistributedServiceCard { n.Spec.ID = "NewID"; return n }},
		{"IPConfig", func(n cluster.DistributedServiceCard) cluster.DistributedServiceCard {
			n.Spec.IPConfig.IPAddress = "1.2.3.4"
			return n
		}},
		{"NetworkMode", func(n cluster.DistributedServiceCard) cluster.DistributedServiceCard {
			n.Spec.NetworkMode = "INBAND"
			return n
		}},
		{"MgmtVlan", func(n cluster.DistributedServiceCard) cluster.DistributedServiceCard { n.Spec.MgmtVlan = 1; return n }},
		{"Controllers", func(n cluster.DistributedServiceCard) cluster.DistributedServiceCard {
			n.Spec.Controllers = []string{"abc"}
			return n
		}},
		{"ID, MgmtVlan", func(n cluster.DistributedServiceCard) cluster.DistributedServiceCard {
			n.Spec.ID = "NewID2"
			n.Spec.MgmtVlan = 2
			return n
		}},
	}

	for _, mt := range modTests {
		for _, ctx := range []context.Context{userCtx, otherCtx} {
			for _, oper := range []apiintf.APIOperType{apiintf.CreateOper, apiintf.UpdateOper, apiintf.DeleteOper} {
				var updNIC cluster.DistributedServiceCard
				nic.Clone(&updNIC)
				_, r, err := hooks.smartNICPreCommitHook(ctx, kv, kv.NewTxn(), key, oper, false, mt.fieldTx(updNIC))
				if ctx == otherCtx || oper == apiintf.CreateOper || oper == apiintf.DeleteOper {
					Assert(t, r == true && err == nil,
						"smartNICPreCommitHook returned unexpected error, field: %s, op: %v, ctx: %+v, err: %v", mt.fieldName, oper, ctx, err)
				} else {
					Assert(t, r == true && err != nil && strings.Contains(err.Error(), mt.fieldName),
						"smartNICPreCommitHook did not return expected error, field: %s, op: %v, ctx: %+v, err: %v", mt.fieldName, oper, ctx, err)
				}
			}
		}
	}
	for _, oper := range []apiintf.APIOperType{apiintf.CreateOper, apiintf.UpdateOper} {
		_, _, err := hooks.smartNICPreCommitHook(ctx, kv, kv.NewTxn(), key, oper, false, nil)
		Assert(t, reflect.DeepEqual(err, errInvalidInputType), fmt.Sprintf("unexpected error: %v", err))
	}
	nic.Status.IPConfig = nil
	txn := kv.NewTxn()
	hooks.smartNICPreCommitHook(ctx, kv, txn, key, apiintf.CreateOper, false, nic)
	Assert(t, !txn.IsEmpty(), "module object should be created if IP config is not present in smart nic status")
}
