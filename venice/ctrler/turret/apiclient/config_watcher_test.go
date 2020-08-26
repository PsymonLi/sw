package apiclient

import (
	"context"
	"fmt"
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/apiserver"
	types "github.com/pensando/sw/venice/cmd/types/protos"
	"github.com/pensando/sw/venice/ctrler/turret/memdb"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	mockresolver "github.com/pensando/sw/venice/utils/resolver/mock"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/serviceutils"
)

var (
	logConfig = log.GetDefaultConfig(fmt.Sprintf("%s.%s", globals.Turret, "config-watcher"))
	logger    = log.SetConfig(logConfig)

	// create mock events recorder
	_ = recorder.Override(mockevtsrecorder.NewRecorder("turret_test", logger))
)

func setup(t *testing.T) (*ConfigWatcher, apiserver.Server) {
	tLogger := logger.WithContext("t_name", t.Name())

	// create mock resolver
	mr := mockresolver.New()

	// create API server
	apiServer, apiServerURL, err := serviceutils.StartAPIServer("", t.Name(), tLogger, []string{})
	AssertOk(t, err, "failed to create API Server")
	mr.AddServiceInstance(&types.ServiceInstance{
		TypeMeta: api.TypeMeta{
			Kind: "ServiceInstance",
		},
		ObjectMeta: api.ObjectMeta{
			Name: globals.APIServer,
		},
		Service: globals.APIServer,
		URL:     apiServerURL,
	})

	cfgWatcher := NewConfigWatcher(context.Background(), mr, memdb.NewMemDb(),
		log.SetConfig(log.GetDefaultConfig(t.Name())))
	Assert(t, cfgWatcher != nil, "failed to create config watcher")
	return cfgWatcher, apiServer
}

// TestConfigWatcher tests config watcher
func TestConfigWatcher(t *testing.T) {
	cfgWatcher, apiServer := setup(t)
	defer apiServer.Stop()
	defer cfgWatcher.Stop()

	cfgWatcher.StartWatcher()
	time.Sleep(10 * time.Second)
	Assert(t, cfgWatcher.APIClient() != nil, "config watcher did not create API client yet")
}

// TestStatsAlertPolicyOnMemdb tests the mem DB CRUD on stats alert policy objects
func TestStatsAlertPolicyOnMemdb(t *testing.T) {
	cfgWatcher := &ConfigWatcher{logger: log.SetConfig(log.GetDefaultConfig(t.Name())), memDb: memdb.NewMemDb()}

	pol := &monitoring.StatsAlertPolicy{
		TypeMeta:   api.TypeMeta{Kind: "StatsAlertPolicy"},
		ObjectMeta: api.ObjectMeta{Name: "test-ap1", Tenant: "test-ten1"},
		Spec: monitoring.StatsAlertPolicySpec{
			Metric: monitoring.MetricIdentifier{
				Kind:      "Cluster",
				FieldName: "AdmittedNICs",
			},
			MeasurementCriteria: &monitoring.MeasurementCriteria{
				Window:   "5m",
				Function: "mean",
			},
			Thresholds: monitoring.Thresholds{
				Operator: ">",
				Values: []monitoring.Threshold{
					monitoring.Threshold{
						RaiseValue: "100",
					},
				},
			},
		},
	}

	err := cfgWatcher.processStatsAlertPolicy(kvstore.Created, pol)
	AssertOk(t, err, "failed to process stats alert policy, err: %v", err)

	// find stats alert policy in mem DB
	oPol, err := cfgWatcher.memDb.FindObject(pol.GetKind(), pol.GetObjectMeta())
	AssertOk(t, err, "stats alert policy not found, err: %v", err)
	Assert(t, pol.GetObjectKind() == oPol.GetObjectKind(),
		"stats alert policy kind didn't match, expected: %v, got: %v", pol.GetObjectKind(), oPol.GetObjectKind())
	Assert(t, reflect.DeepEqual(pol.GetObjectMeta(), oPol.GetObjectMeta()),
		"stats alert policy meta didn't match, expected: %+v, got %+v", pol.GetObjectMeta(), oPol.GetObjectMeta())
	ap := oPol.(*monitoring.StatsAlertPolicy)
	Assert(t, ap.GetSpec().Metric.Kind == "Cluster",
		"stats alert policy resource didn't match, expected: %v, got: %v", "Cluster", ap.GetSpec().Metric.Kind)

	// update policy
	pol.Spec.MeasurementCriteria.Window = "10m"
	err = cfgWatcher.processStatsAlertPolicy(kvstore.Updated, pol)
	AssertOk(t, err, "failed to update stats alert policy")
	oPol, err = cfgWatcher.memDb.FindObject(pol.GetKind(), pol.GetObjectMeta())
	AssertOk(t, err, "stats alert policy not found, err: %v", err)
	Assert(t, pol.GetObjectKind() == oPol.GetObjectKind(),
		"stats alert policy kind didn't match, expected: %v, got: %v", pol.GetObjectKind(), oPol.GetObjectKind())
	Assert(t, reflect.DeepEqual(pol.GetObjectMeta(), oPol.GetObjectMeta()),
		"stats alert policy meta didn't match, expected: %+v, got %+v", pol.GetObjectMeta(), oPol.GetObjectMeta())
	ap = oPol.(*monitoring.StatsAlertPolicy)
	Assert(t, "10m" == ap.Spec.MeasurementCriteria.Window,
		"stats alert policy resource didn't match, expected: %v, got: %v", "10m", ap.Spec.MeasurementCriteria.Window)

	// delete policy
	err = cfgWatcher.processStatsAlertPolicy(kvstore.Deleted, pol)
	AssertOk(t, err, "failed to delete stats alert policy")
	_, err = cfgWatcher.memDb.FindObject(pol.GetKind(), pol.GetObjectMeta())
	Assert(t, err != nil, "stats alert policy found after delete")

	// invalid
	err = cfgWatcher.processStatsAlertPolicy(kvstore.WatcherError, nil)
	Assert(t, err != nil && strings.Contains(err.Error(), "invalid stats alert policy watch event"), "expected failure, but succeeded")
}
