package impl

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/objstore/minio"
	. "github.com/pensando/sw/venice/utils/testutils"

	"github.com/pensando/sw/api/generated/apiclient"

	"github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/utils/kvstore/store"
	"github.com/pensando/sw/venice/utils/runtime"

	"github.com/gogo/protobuf/types"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/venice/utils/log"

	"github.com/pensando/sw/api/generated/rollout"
)

const (
	rolloutName         = "e2e_rollout"
	rolloutParallelName = "e2e_rollout_parallel"
)

var (
	seconds            = time.Now().Unix()
	scheduledStartTime = &api.Timestamp{
		Timestamp: types.Timestamp{
			Seconds: seconds + 30, //Add a scheduled rollout with 30 second delay
		},
	}
	scheduledEndTime = &api.Timestamp{
		Timestamp: types.Timestamp{
			Seconds: seconds + 1800, //30 min window
		},
	}
)

func TestRolloutActionPreCommitHooks(t *testing.T) {

	hooks := &rolloutHooks{
		l: log.SetConfig(log.GetDefaultConfig("Rollout-Precommit-Test")),
	}
	clusterVersion := newClusterObj()

	roa := newRolloutAction(scheduledStartTime, scheduledEndTime, rolloutName, rollout.RolloutPhase_PROGRESSING.String())
	req := newRollout(rolloutName, "2.8.1")

	rolloutCfg := store.Config{
		Type:    store.KVStoreTypeMemkv,
		Codec:   runtime.NewJSONCodec(runtime.NewScheme()),
		Servers: []string{t.Name()},
	}
	kvs, err := store.New(rolloutCfg)
	if err != nil {
		t.Fatalf("unable to create kvstore %s", err)
	}

	now := time.Now()
	nowTimestamp, _ := types.TimestampProto(now)
	credsObj := getMinioCredsWithoutOldKeys(nowTimestamp)
	addCredsToKVStore(t, &credsObj, kvs)

	txn := kvs.NewTxn()
	key2 := req.MakeKey(string(apiclient.GroupRollout))
	err = kvs.Create(context.TODO(), key2, &req)

	ret, skip, err := hooks.doRolloutAction(context.TODO(), kvs, txn, key2, apiintf.CreateOper, false, req)

	if ret != nil || err == nil {
		t.Fatalf("expected failure but commitAction succeeded [%v](%s)", ret, err)
	}
	if skip == true {
		t.Fatalf("kvwrite not enabled on commit")
	}

	clusterKey := clusterVersion.MakeKey(string(apiclient.GroupCluster))
	err = kvs.Create(context.TODO(), clusterKey, clusterVersion)
	if err != nil {
		t.Fatalf("Failed creation of clusterVersion %v", err)
	}

	ret, skip, err = hooks.doRolloutAction(context.TODO(), kvs, txn, key2, apiintf.CreateOper, false, req)

	if ret == nil || err != nil {
		t.Fatalf("failed exec commitAction [%v](%s)", ret, err)
	}
	if skip == false {
		t.Fatalf("kvwrite enabled on commit")
	}
	key := roa.MakeKey(string(apiclient.GroupRollout))
	err = kvs.Create(context.TODO(), key, &roa)

	req2 := newRollout(rolloutParallelName, "2.8")

	ret, skip, err = hooks.doRolloutAction(context.TODO(), kvs, txn, "", apiintf.CreateOper, false, req2)

	if ret == nil || err == nil {
		t.Fatalf("failed exec commitAction [%v](%s)", ret, err)
	}
	if skip != false {
		t.Fatalf("kvwrite enabled on commit")
	}
	ret, skip, err = hooks.stopRolloutAction(context.TODO(), kvs, txn, key2, apiintf.CreateOper, false, req)

	if ret == nil || err != nil {
		t.Fatalf("failed exec commitAction [%v](%s)", ret, err)
	}

	req4 := rollout.Rollout{
		TypeMeta: api.TypeMeta{
			Kind: "Rollout",
		},
		ObjectMeta: api.ObjectMeta{
			Name: rolloutName,
		},
		Spec: rollout.RolloutSpec{
			Version:                   "2.8",
			ScheduledStartTime:        scheduledStartTime,
			ScheduledEndTime:          scheduledEndTime,
			Strategy:                  rollout.RolloutSpec_LINEAR.String(),
			MaxParallel:               0,
			MaxNICFailuresBeforeAbort: 0,
			OrderConstraints:          nil,
			Suspend:                   false,
			DSCsOnly:                  false,
			UpgradeType:               rollout.RolloutSpec_Graceful.String(),
		},
	}

	ret, skip, err = hooks.modifyRolloutAction(context.TODO(), kvs, txn, key2, apiintf.CreateOper, false, req4)

	if ret == nil || err != nil {
		t.Fatalf("failed exec commitAction [%v](%s)", ret, err)
	}
	ret, skip, err = hooks.deleteRolloutAction(context.TODO(), kvs, txn, key2, apiintf.DeleteOper, false, req4)

	if ret == nil || err == nil {
		t.Fatalf("failed exec commitAction [%v](%s)", ret, err)
	}
}

func TestRolloutActionPreCommitHooks_withCredentialsObjContainingOldKeys(t *testing.T) {

	hooks := &rolloutHooks{
		//svc: apisrvpkg.MustGetAPIServer()
		l: log.SetConfig(log.GetDefaultConfig("Rollout-Precommit-Test")),
	}

	clusterVersion := newClusterObj()

	roa := newRolloutAction(scheduledStartTime, scheduledEndTime, rolloutName, rollout.RolloutPhase_COMPLETE.String())

	kvs := newInMemKVStore(t)
	txn := kvs.NewTxn()

	clusterKey := clusterVersion.MakeKey(string(apiclient.GroupCluster))
	err := kvs.Create(context.TODO(), clusterKey, clusterVersion)
	AssertOk(t, err, "Failed creation of clusterVersion %v", err)

	key := roa.MakeKey(string(apiclient.GroupRollout))
	err = kvs.Create(context.TODO(), key, &roa)

	req := newRollout(rolloutParallelName, "2.8")

	now := time.Now()
	nowTimestamp, _ := types.TimestampProto(now)
	credsObj := getMinioCredentialsObj(nowTimestamp)
	credsKey := addCredsToKVStore(t, &credsObj, kvs)

	ret, kvwriteEnabled, err := hooks.doRolloutAction(context.TODO(), kvs, txn, "", apiintf.CreateOper, false, req)

	Assert(t, ret != nil && err == nil, "failed exec commitAction [%v](%s)", ret, err)
	Assert(t, kvwriteEnabled, "kvwrite not enabled on commit")
	_, err = txn.Commit(context.TODO())
	assertCredsObjState(t, kvs, credsKey, "", "", roa.CreationTime.Timestamp)
}

func TestRolloutActionPreCommitHooks_CredentialsObjWithoutOldKeys(t *testing.T) {

	hooks := &rolloutHooks{
		//svc: apisrvpkg.MustGetAPIServer()
		l: log.SetConfig(log.GetDefaultConfig("Rollout-Precommit-Test")),
	}

	clusterVersion := newClusterObj()
	roa := newRolloutAction(scheduledStartTime, scheduledEndTime, rolloutName, rollout.RolloutPhase_COMPLETE.String())

	kvs := newInMemKVStore(t)
	txn := kvs.NewTxn()

	clusterKey := clusterVersion.MakeKey(string(apiclient.GroupCluster))
	err := kvs.Create(context.TODO(), clusterKey, clusterVersion)
	AssertOk(t, err, "Failed creation of clusterVersion %v", err)

	key := roa.MakeKey(string(apiclient.GroupRollout))
	err = kvs.Create(context.TODO(), key, &roa)

	req := newRollout(rolloutParallelName, "2.8")

	now := time.Now()
	nowTimestamp, _ := types.TimestampProto(now)
	credsObj := getMinioCredsWithoutOldKeys(nowTimestamp)
	credsKey := addCredsToKVStore(t, &credsObj, kvs)

	//rollout attempt (when credentials object has no old keys) should succeed and has to leave credentials object untouched
	expectedCredsObj := getCredsObjFromKVStore(t, kvs, credsKey)
	err = expectedCredsObj.ApplyStorageTransformer(context.TODO(), false)
	AssertOk(t, err, "storage transform expected to succeed")
	ret, kvwriteEnabled, err := hooks.doRolloutAction(context.TODO(), kvs, txn, "", apiintf.CreateOper, false, req)
	Assert(t, ret != nil && err == nil, "failed exec commitAction [%v](%s)", ret, err)
	Assert(t, kvwriteEnabled, "kvwrite not enabled on commit")
	_, err = txn.Commit(context.TODO())
	actualCredsObj := getCredsObjFromKVStore(t, kvs, credsKey)
	err = actualCredsObj.ApplyStorageTransformer(context.TODO(), false)
	AssertOk(t, err, "storage transform expected to succeed")
	AssertEquals(t, expectedCredsObj.Spec, actualCredsObj.Spec, "unexpected creds object state")
}

func TestRolloutActionPreCommitHooks_WhenClearCredentialsOpFails(t *testing.T) {

	hooks := &rolloutHooks{
		//svc: apisrvpkg.MustGetAPIServer()
		l: log.SetConfig(log.GetDefaultConfig("Rollout-Precommit-Test")),
	}

	clusterVersion := newClusterObj()
	roa := newRolloutAction(scheduledStartTime, scheduledEndTime, rolloutName, rollout.RolloutPhase_COMPLETE.String())

	kvs := newInMemKVStore(t)
	txn := kvs.NewTxn()

	clusterKey := clusterVersion.MakeKey(string(apiclient.GroupCluster))
	err := kvs.Create(context.TODO(), clusterKey, clusterVersion)
	AssertOk(t, err, "Failed creation of clusterVersion %v", err)

	key := roa.MakeKey(string(apiclient.GroupRollout))
	err = kvs.Create(context.TODO(), key, &roa)

	req := newRollout(rolloutParallelName, "2.8")

	//rollout attempt (when credentials object is not present) should fail
	_, kvwriteEnabled, err := hooks.doRolloutAction(context.TODO(), kvs, txn, "", apiintf.CreateOper, false, req)
	AssertError(t, err, "doRolloutAction is expected to fail")
	Assert(t, !kvwriteEnabled, "kvwrite enabled on commit")
}

func addCredsToKVStore(t *testing.T, credsObj *cluster.Credentials, kvs kvstore.Interface) string {
	credsKey := credsObj.MakeKey(string(apiclient.GroupCluster))
	err := credsObj.ApplyStorageTransformer(context.TODO(), true)
	AssertOk(t, err, "storage transform expected to succeed")

	err = kvs.Create(context.TODO(), credsKey, credsObj)
	AssertOk(t, err, "credential object creation expected to succeed")
	return credsKey
}

func newClusterObj() *cluster.Version {
	return &cluster.Version{
		TypeMeta: api.TypeMeta{
			Kind:       "Version",
			APIVersion: "v1",
		},
		ObjectMeta: api.ObjectMeta{
			Name: globals.DefaultVersionName,
		},
		Spec: cluster.VersionSpec{},
		Status: cluster.VersionStatus{
			BuildVersion:        "2.1.3",
			VCSCommit:           "",
			BuildDate:           "",
			RolloutBuildVersion: "",
		},
	}
}

func newRollout(rolloutName, version string) rollout.Rollout {
	return rollout.Rollout{
		TypeMeta: api.TypeMeta{
			Kind: "Rollout",
		},
		ObjectMeta: api.ObjectMeta{
			Name: rolloutName,
		},
		Spec: rollout.RolloutSpec{
			Version:                   version,
			ScheduledStartTime:        scheduledStartTime,
			ScheduledEndTime:          scheduledEndTime,
			Strategy:                  rollout.RolloutSpec_LINEAR.String(),
			MaxParallel:               0,
			MaxNICFailuresBeforeAbort: 0,
			OrderConstraints:          nil,
			Suspend:                   false,
			DSCsOnly:                  false,
			DSCMustMatchConstraint:    true, // hence venice upgrade only
			UpgradeType:               rollout.RolloutSpec_Graceful.String(),
		},
	}
}

func newRolloutAction(scheduledStartTime, scheduledEndTime *api.Timestamp, name, rolloutState string) rollout.RolloutAction {
	return rollout.RolloutAction{
		TypeMeta: api.TypeMeta{
			Kind: "RolloutAction",
		},
		ObjectMeta: api.ObjectMeta{
			Name: name,
		},
		Spec: rollout.RolloutSpec{
			Version:                   "2.8.1",
			ScheduledStartTime:        scheduledStartTime,
			ScheduledEndTime:          scheduledEndTime,
			Strategy:                  rollout.RolloutSpec_LINEAR.String(),
			MaxParallel:               0,
			MaxNICFailuresBeforeAbort: 0,
			OrderConstraints:          nil,
			Suspend:                   false,
			DSCsOnly:                  false,
			DSCMustMatchConstraint:    true, // hence venice upgrade only
			UpgradeType:               rollout.RolloutSpec_Graceful.String(),
		},
		Status: rollout.RolloutActionStatus{
			OperationalState: rolloutState,
		},
	}
}

func newInMemKVStore(t *testing.T) kvstore.Interface {
	rolloutCfg := store.Config{
		Type:    store.KVStoreTypeMemkv,
		Codec:   runtime.NewJSONCodec(runtime.NewScheme()),
		Servers: []string{t.Name()},
	}
	kvs, err := store.New(rolloutCfg)
	AssertOk(t, err, "unable to create kvstore %s", err)
	return kvs
}

func getCredsObjFromKVStore(t *testing.T, kvs kvstore.Interface, credsKey string) *cluster.Credentials {
	credsObj := &cluster.Credentials{}
	err := kvs.Get(context.TODO(), credsKey, credsObj)
	AssertOk(t, err, "Credentials object should have been found")
	return credsObj
}

func assertCredsObjState(t *testing.T, kvs kvstore.Interface, credsKey, expectedOldAccessKey, expectedOldSecretKey string, rolloutCreationTime types.Timestamp) {
	credentials := &cluster.Credentials{}
	err := kvs.Get(context.TODO(), credsKey, credentials)
	AssertOk(t, err, "Credentials object should have been found")
	err = credentials.ApplyStorageTransformer(context.TODO(), false)
	AssertOk(t, err, "Credentials object should have been decrypted successfully")
	Assert(t, rolloutCreationTime.Compare(credentials.ModTime.Timestamp) <= 0, "update operation should have updated the last modified time of credentials object")
	minioCreds, err := minio.GetMinioKeys(credentials)
	AssertOk(t, err, "Invalid credentials object attempted to be created")
	Assert(t, len(minioCreds.AccessKey) > 0, "access key can not be empty")
	Assert(t, len(minioCreds.SecretKey) > 0, "secret key can not be empty")
	fmt.Println(minioCreds.OldAccessKey)
	Assert(t, minioCreds.OldAccessKey == expectedOldAccessKey, "Old access key not in expected state")
	Assert(t, minioCreds.OldSecretKey == expectedOldSecretKey, "Old secret key not in expected state")
}
