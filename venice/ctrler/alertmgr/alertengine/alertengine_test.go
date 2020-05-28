package alertengine

import (
	"context"
	"fmt"
	"testing"
	"time"

	gogo "github.com/gogo/protobuf/types"
	"github.com/golang/mock/gomock"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/fields"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/monitoring"
	mockapi "github.com/pensando/sw/api/mock"
	"github.com/pensando/sw/events/generated/eventattrs"
	objectdb "github.com/pensando/sw/venice/ctrler/alertmgr/objdb"
	"github.com/pensando/sw/venice/ctrler/alertmgr/policyengine"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	mockresolver "github.com/pensando/sw/venice/utils/resolver/mock"
	"github.com/pensando/sw/venice/utils/runtime"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/policygen"
)

var (
	logConfig   = log.GetDefaultConfig(fmt.Sprintf("%s.%s", globals.AlertMgr, "test"))
	logger      = log.SetConfig(logConfig)
	_           = recorder.Override(mockevtsrecorder.NewRecorder("alertmgr_test", logger))
	objdb       = objectdb.New()
	mr          = mockresolver.New()
	mMonitoring = &mockMonitoringV1{}
	ctrl        *gomock.Controller
	mapi        *mockapi.MockServices
	mAlert      *mockapi.MockMonitoringV1AlertInterface
	ae          *alertEngine
	pol         *monitoring.AlertPolicy
	dsc         *cluster.DistributedServiceCard
	polID       string
	dscRef      *api.ObjectRef
	alert       *monitoring.Alert
)

type mockMonitoringV1 struct {
	mAlert monitoring.MonitoringV1AlertInterface
}

func (m *mockMonitoringV1) EventPolicy() monitoring.MonitoringV1EventPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) FwlogPolicy() monitoring.MonitoringV1FwlogPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) FlowExportPolicy() monitoring.MonitoringV1FlowExportPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) Alert() monitoring.MonitoringV1AlertInterface {
	return m.mAlert
}
func (m *mockMonitoringV1) AlertPolicy() monitoring.MonitoringV1AlertPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) StatsAlertPolicy() monitoring.MonitoringV1StatsAlertPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) AlertDestination() monitoring.MonitoringV1AlertDestinationInterface {
	return nil
}
func (m *mockMonitoringV1) MirrorSession() monitoring.MonitoringV1MirrorSessionInterface {
	return nil
}
func (m *mockMonitoringV1) TroubleshootingSession() monitoring.MonitoringV1TroubleshootingSessionInterface {
	return nil
}
func (m *mockMonitoringV1) TechSupportRequest() monitoring.MonitoringV1TechSupportRequestInterface {
	return nil
}
func (m *mockMonitoringV1) ArchiveRequest() monitoring.MonitoringV1ArchiveRequestInterface {
	return nil
}
func (m *mockMonitoringV1) AuditPolicy() monitoring.MonitoringV1AuditPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) Watch(ctx context.Context, options *api.AggWatchOptions) (kvstore.Watcher, error) {
	return nil, nil
}

func setup(t *testing.T) {
	tLogger := logger.WithContext("t_name", t.Name())

	// Create mock API client.
	ctrl = gomock.NewController(t)
	mAlert = mockapi.NewMockMonitoringV1AlertInterface(ctrl)
	mMonitoring.mAlert = mAlert
	mapi = mockapi.NewMockServices(ctrl)

	// Create alert engine.
	aei, err := New(tLogger, mr, objdb)
	AssertOk(t, err, "New alert engine failed")
	ae = aei.(*alertEngine)
	ae.apiClient = mapi

	// Create test alert policy.
	req := []*fields.Requirement{&fields.Requirement{Key: "status.primary-mac", Operator: "in", Values: []string{"00ae.cd00.1142"}}}
	pol = policygen.CreateAlertPolicyObj(globals.DefaultTenant, "", CreateAlphabetString(5), "DistributedServiceCard", eventattrs.Severity_INFO, "DSC mac check", req, []string{})
	polID = fmt.Sprintf("%s/%s", pol.GetName(), pol.GetUUID())
	ae.objdb.Add(pol)

	// Create test dsc object.
	dsc = policygen.CreateSmartNIC("00ae.cd00.1142", "admitted", "naples-1", &cluster.DSCCondition{Type: "healthy", Status: "true", LastTransitionTime: ""})
	ae.objdb.Add(dsc)

	// Get ref of dsc object.
	m, _ := runtime.GetObjectMeta(dsc)
	dscRef = &api.ObjectRef{
		Tenant:    globals.DefaultTenant,
		Namespace: globals.DefaultNamespace,
		Kind:      dsc.GetObjectKind(),
		Name:      m.GetName(),
		URI:       m.GetSelfLink(),
	}

	// Create test alert object.
	alert = &monitoring.Alert{TypeMeta: api.TypeMeta{Kind: "Alert"}}
	alert.Status.ObjectRef = dscRef
	alert.Status.Reason.PolicyID = polID
}

func teardown() {
	ctrl.Finish()
}

func TestRun(t *testing.T) {
	setup(t)
	defer teardown()
	Assert(t, ae != nil, "alert engine nil")
	ae.ctx, ae.cancel = context.WithCancel(context.Background())

	defer func() {
		ae.cancel()
		time.Sleep(100 * time.Millisecond)
		Assert(t, !ae.GetRunningStatus(), "running flag still set")
	}()

	inCh := make(chan *policyengine.PEOutput)
	outCh, errCh, err := ae.Run(ae.ctx, nil, inCh)
	time.Sleep(100 * time.Millisecond)

	AssertOk(t, err, "Error running alert engine")
	Assert(t, outCh != nil, "out channel nil")
	Assert(t, errCh != nil, "error channel nil")
	Assert(t, ae.GetRunningStatus(), "running flag not set")

	time.Sleep(1 * time.Second)
}

func TestRestart(t *testing.T) {
	tLogger := logger.WithContext("t_name", t.Name())

	// Create test alert policy.
	req := []*fields.Requirement{&fields.Requirement{Key: "status.primary-mac", Operator: "in", Values: []string{"00ae.cd00.1142"}}}
	pol = policygen.CreateAlertPolicyObj(globals.DefaultTenant, "", CreateAlphabetString(5), "DistributedServiceCard", eventattrs.Severity_INFO, "DSC mac check", req, []string{})
	polID = fmt.Sprintf("%s/%s", pol.GetName(), pol.GetUUID())
	objdb.Add(pol)

	// Create test dsc object.
	dsc = policygen.CreateSmartNIC("00ae.cd00.1142", "admitted", "naples-1", &cluster.DSCCondition{Type: "healthy", Status: "true", LastTransitionTime: ""})
	objdb.Add(dsc)

	// Get ref of dsc object.
	m, _ := runtime.GetObjectMeta(dsc)
	dscRef = &api.ObjectRef{
		Tenant:    globals.DefaultTenant,
		Namespace: globals.DefaultNamespace,
		Kind:      dsc.GetObjectKind(),
		Name:      m.GetName(),
		URI:       m.GetSelfLink(),
	}

	// Create test alert object.
	alert = &monitoring.Alert{TypeMeta: api.TypeMeta{Kind: "Alert"}}
	alert.Status.ObjectRef = dscRef
	alert.Status.Reason.PolicyID = polID
	err := objdb.Add(alert)
	AssertOk(t, err, "err: %v", err)

	// Create alert engine.
	aei, err := New(tLogger, mr, objdb)
	AssertOk(t, err, "New alert engine failed")
	ae = aei.(*alertEngine)

	verifyAlertInCache(t, dscRef, polID, alert, alertStateOpenUnacked)
}

func verifyAlertInCache(t *testing.T, objRef *api.ObjectRef, polID string, alert *monitoring.Alert, state alertState) *monitoring.Alert {
	a, ok := ae.cache.alertsByObjectAndPolicy[objRef.String()][polID]
	Assert(t, ok, "no entry for alert in ae.cache.alertsByObjectAndPolicy")
	if alert != nil {
		Assert(t, a == alert, "incorrect alert")
	}

	var found bool
	for _, polAlert := range ae.cache.alertsByPolicy[polID] {
		if polAlert == a {
			found = true
			break
		}
	}
	Assert(t, found, "alertByPolicy not found")

	aState, ok := ae.cache.alertsState[a]
	Assert(t, ok, "no entry for alert in ae.cache.alertsState")
	Assert(t, aState == state, "expected state: %d, got %d", state, aState)

	return a
}

func verifyAlertNotInCache(t *testing.T, objRef *api.ObjectRef, polID string) {
	oMap, ok := ae.cache.alertsByObjectAndPolicy[objRef.String()]
	if ok {
		_, found := oMap[polID]
		Assert(t, !found, "alert still exists in ae.cache.alertsByObjectAndPolicy")
	}

	found := false
	for _, a := range ae.cache.alertsByPolicy[polID] {
		if a == alert {
			found = true
			break
		}
	}
	Assert(t, !found, "alertByPolicy still found")

	_, ok = ae.cache.alertsState[alert]
	Assert(t, !ok, "alert still exists in ae.cache.alertsState")
}

func TestAlertCacheOps(t *testing.T) {
	setup(t)
	defer teardown()
	Assert(t, ae != nil, "alert engine nil")

	ae.createAlertInCache(alert)
	verifyAlertInCache(t, dscRef, polID, alert, alertStateOpenUnacked)

	// Debounce alert.
	ae.debounceAlertInCache(alert)
	aState, _ := ae.cache.alertsState[alert]
	Assert(t, aState == alertStateOpenDebounceTimerRunning, "expected state: %d, got %d", alertStateOpenDebounceTimerRunning, aState)

	// Bounce alert.
	ae.bounceAlertInCache(alert)
	aState, _ = ae.cache.alertsState[alert]
	Assert(t, aState == alertStateOpenUnacked, "expected state: %d, got %d", alertStateOpenUnacked, aState)

	// Resolve alert.
	found := false
	ae.debounceAlertInCache(alert)
	mAlert.EXPECT().Update(gomock.Any(), gomock.Any()).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	ae.processDebounceTick()
	var dbncInfo *debounceInfo
	for _, dbncInfo = range ae.cache.alertsInDebounceInterval {
		a := dbncInfo.alert
		if a == alert {
			found = true
		}
	}
	Assert(t, found == true, "alert not found in debounce interval list")
	expNumTicks := (2 * 1000 / 500) - 1
	Assert(t, dbncInfo.numTicks == expNumTicks, "expected numTicks %d, got %d", expNumTicks, dbncInfo.numTicks)

	dbncInfo.numTicks = 1
	ae.processDebounceTick()
	aState, _ = ae.cache.alertsState[alert]
	Assert(t, aState == alertStateResolved, "expected state: %d, got %d", alertStateResolved, aState)

	// Reopen alert.
	ae.reopenAlertInCache(alert)
	aState, _ = ae.cache.alertsState[alert]
	Assert(t, aState == alertStateOpenUnacked, "expected state: %d, got %d", alertStateOpenUnacked, aState)

	// Garbage collect alert.
	ae.debounceAlertInCache(alert)
	var pos int
	for pos, dbncInfo = range ae.cache.alertsInDebounceInterval {
		a := dbncInfo.alert
		if a == alert {
			found = true
		}
	}
	Assert(t, found == true, "alert not found in debounce interval list")
	ae.resolveAlertInCache(alert, pos)
	tm, _ := gogo.TimestampProto(time.Now().Add(-25 * time.Hour))
	alert.Status.Resolved.Time = &api.Timestamp{Timestamp: *tm}

	mAlert.EXPECT().Delete(gomock.Any(), gomock.Any()).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)

	ae.garbageCollect()
	time.Sleep(1 * time.Second)
	verifyAlertNotInCache(t, dscRef, polID)

	// Delete by policy.
	ae.createAlertInCache(alert)
	verifyAlertInCache(t, dscRef, polID, alert, alertStateOpenUnacked)
	ae.deleteAlertsInCacheByPolicy(polID)
	verifyAlertNotInCache(t, dscRef, polID)

	// Delete by objref.
	ae.createAlertInCache(alert)
	verifyAlertInCache(t, dscRef, polID, alert, alertStateOpenUnacked)
	ae.deleteAlertsInCacheByObject(dscRef.String())
	verifyAlertNotInCache(t, dscRef, polID)

	// Delete alert in debounce state.
	ae.createAlertInCache(alert)
	verifyAlertInCache(t, dscRef, polID, alert, alertStateOpenUnacked)
	ae.debounceAlertInCache(alert)
	ae.deleteAlertInCache(alert)
	verifyAlertNotInCache(t, dscRef, polID)
}

func TestProcessAlertPolicy(t *testing.T) {
	setup(t)
	defer teardown()
	Assert(t, ae != nil, "alert engine nil")

	// Create op.
	peResult := policyengine.PEOutput{Object: pol, Op: "Created", WasPolicyApplied: true, MatchingPolicies: []policyengine.MatchingPolicy{}, MatchingObj: policyengine.MatchingObject{Obj: dsc, Reqs: nil}}
	mAlert.EXPECT().Create(gomock.Any(), gomock.Any()).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	err := ae.processInput(&peResult)
	AssertOk(t, err, "processAlertPolicy failed, err: %v", err)
	time.Sleep(1 * time.Second)

	a := verifyAlertInCache(t, dscRef, polID, nil, alertStateOpenUnacked)
	m, _ := runtime.GetObjectMeta(dsc)
	dscName := m.GetName()
	Assert(t, a.Status.ObjectRef.Kind == dsc.GetObjectKind(), "bad object ref in alert")
	Assert(t, a.Status.ObjectRef.Name == dscName, "bad object ref in alert")
	Assert(t, a.Status.Reason.PolicyID == polID, "bad policy ref in alert")

	// Delete op.
	peResult = policyengine.PEOutput{Object: pol, Op: "Deleted", WasPolicyApplied: true, MatchingPolicies: []policyengine.MatchingPolicy{}, MatchingObj: policyengine.MatchingObject{Obj: dsc, Reqs: nil}}
	mAlert.EXPECT().Delete(gomock.Any(), gomock.Any()).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	err = ae.processInput(&peResult)
	AssertOk(t, err, "processAlertPolicy failed, err: %v", err)
	time.Sleep(1 * time.Second)
	verifyAlertNotInCache(t, dscRef, polID)
}

func TestProcessObject(t *testing.T) {
	setup(t)
	defer teardown()
	Assert(t, ae != nil, "alert engine nil")

	// Incorrect op.
	peResult := policyengine.PEOutput{Object: pol, Op: "Deleted", WasPolicyApplied: true, MatchingPolicies: []policyengine.MatchingPolicy{}, MatchingObj: policyengine.MatchingObject{Obj: dsc, Reqs: nil}}
	err := ae.processInput(&peResult)
	AssertOk(t, err, "processObject failed for incorrect op, err: %v", err)

	// Create op.
	mpl := []policyengine.MatchingPolicy{{Policy: pol, Reqs: nil}}
	peResult = policyengine.PEOutput{Object: dsc, Op: "Created", WasPolicyApplied: true, MatchingPolicies: mpl, MatchingObj: policyengine.MatchingObject{}}
	mAlert.EXPECT().Create(gomock.Any(), gomock.Any()).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	err = ae.processInput(&peResult)
	AssertOk(t, err, "processAlertPolicy failed, err: %v", err)
	time.Sleep(1 * time.Second)

	a := verifyAlertInCache(t, dscRef, polID, nil, alertStateOpenUnacked)
	m, _ := runtime.GetObjectMeta(dsc)
	dscName := m.GetName()
	Assert(t, a.Status.ObjectRef.Kind == dsc.GetObjectKind(), "bad object ref in alert")
	Assert(t, a.Status.ObjectRef.Name == dscName, "bad object ref in alert")
	Assert(t, a.Status.Reason.PolicyID == polID, "bad policy ref in alert")

	// Try creating the same alert again.
	err = ae.processInput(&peResult)
	AssertOk(t, err, "processAlertPolicy failed, err: %v", err)
	a = verifyAlertInCache(t, dscRef, polID, nil, alertStateOpenUnacked)

	// Create another policy.
	req := []*fields.Requirement{&fields.Requirement{Key: "admission-phase", Operator: "in", Values: []string{"admitted"}}}
	pol2 := policygen.CreateAlertPolicyObj(globals.DefaultTenant, "", CreateAlphabetString(5), "DistributedServiceCard", eventattrs.Severity_INFO, "DSC mac check", req, []string{})
	pol2ID := fmt.Sprintf("%s/%s", pol2.GetName(), pol2.GetUUID())
	ae.objdb.Add(pol2)

	mpl = []policyengine.MatchingPolicy{{Policy: pol2, Reqs: nil}}
	peResult = policyengine.PEOutput{Object: dsc, Op: "Created", WasPolicyApplied: true, MatchingPolicies: mpl, MatchingObj: policyengine.MatchingObject{}}
	mAlert.EXPECT().Create(gomock.Any(), gomock.Any()).Times(1)
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	err = ae.processInput(&peResult)
	AssertOk(t, err, "processAlertPolicy failed, err: %v", err)
	time.Sleep(1 * time.Second)

	_ = verifyAlertInCache(t, dscRef, pol2ID, nil, alertStateOpenUnacked)
	a = verifyAlertInCache(t, dscRef, polID, nil, alertStateOpenDebounceTimerRunning)

	// Bounce back alert.
	mpl = []policyengine.MatchingPolicy{{Policy: pol, Reqs: nil}, {Policy: pol2, Reqs: nil}}
	peResult = policyengine.PEOutput{Object: dsc, Op: "Created", WasPolicyApplied: true, MatchingPolicies: mpl, MatchingObj: policyengine.MatchingObject{}}
	err = ae.processInput(&peResult)
	a = verifyAlertInCache(t, dscRef, polID, nil, alertStateOpenUnacked)
}
