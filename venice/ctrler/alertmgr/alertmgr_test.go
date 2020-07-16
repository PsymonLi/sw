package alertmgr

import (
	"context"
	"fmt"
	"testing"
	"time"

	uuid "github.com/satori/go.uuid"

	"github.com/golang/mock/gomock"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/fields"
	apiservice "github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/monitoring"
	mockapi "github.com/pensando/sw/api/mock"
	"github.com/pensando/sw/events/generated/eventattrs"
	"github.com/pensando/sw/venice/ctrler/alertmgr/alertengine"
	"github.com/pensando/sw/venice/ctrler/alertmgr/exporter"
	objectdb "github.com/pensando/sw/venice/ctrler/alertmgr/objdb"
	"github.com/pensando/sw/venice/ctrler/alertmgr/policyengine"
	"github.com/pensando/sw/venice/ctrler/alertmgr/watcher"
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
	objdb       objectdb.Objdb
	mr          = mockresolver.New()
	mMonitoring = &mockMonitoring{}
	ctrl        *gomock.Controller
	mapi        *mockapi.MockServices
	mpol        *mockAlertPolicy
	malert      *mockAlert
	malertDest  *mockAlertDest
	pol         *monitoring.AlertPolicy
	dsc         *cluster.DistributedServiceCard
	polID       string
	dscRef      *api.ObjectRef
	alert       *monitoring.Alert
	alertDest   *monitoring.AlertDestination
	am          *mgr
	dMon        = &mockMon{}
)

func TestRun(t *testing.T) {
	// Create mock API client.
	ctrl = gomock.NewController(t)
	defer ctrl.Finish()
	mapi = mockapi.NewMockServices(ctrl)

	// Create alert mgr with mock API client.
	aei, err := New(logger, mr)
	AssertOk(t, err, "New alert mgr failed")
	am = aei.(*mgr)
	am.apiClient = mapi

	// Overwrite the engines with mockers.
	am.watcher = mw
	am.policyEngine = mpe
	am.alertEngine = mae
	am.exporter = me

	stop := func() {
		am.Stop()
		time.Sleep(100 * time.Millisecond)
		Assert(t, !am.GetRunningStatus(), "running flag still set")
	}

	// Run alert mgr.
	mapi.EXPECT().MonitoringV1().Return(dMon).Times(3)
	go am.Run(mapi)
	time.Sleep(100 * time.Millisecond)
	Assert(t, am.GetRunningStatus(), "running flag not set")

	time.Sleep(100 * time.Millisecond)

	// Generate an error from watcher engine.
	wErrCh <- fmt.Errorf("engine error")
	time.Sleep(100 * time.Millisecond)
	Assert(t, am.GetRunningStatus(), "running flag not set")

	time.Sleep(1 * time.Second)
	stop()
}

func TestRunWithError(t *testing.T) {
	ctrl = gomock.NewController(t)
	defer ctrl.Finish()
	mapi = mockapi.NewMockServices(ctrl)

	// Create alert mgr with mock API client.
	aei, err := New(logger, mr)
	AssertOk(t, err, "New alert mgr failed")
	am = aei.(*mgr)
	am.apiClient = mapi

	// Now overwrite the engines with mockers.
	am.watcher = mw
	am.policyEngine = mpe
	am.alertEngine = mae
	am.exporter = me

	stop := func() {
		am.Stop()
		time.Sleep(100 * time.Millisecond)
		Assert(t, !am.GetRunningStatus(), "running flag still set")
	}

	// Generate error in watcher engine.
	mw.genError = true
	mapi.EXPECT().MonitoringV1().Return(dMon).Times(3)
	go am.Run(mapi)
	time.Sleep(100 * time.Millisecond)
	Assert(t, !am.GetRunningStatus(), "running flag still set")

	// Remove the error in watcher engine.
	mw.genError = false
	time.Sleep(2 * time.Second)
	Assert(t, am.GetRunningStatus(), "running flag not set")
	time.Sleep(1 * time.Second)
	stop()
}

// Mock monitoring type.
// Only AlertPolicy().List() and Alert().List() methods are implemented. Both return nil objects.
type mockMon struct {
	*mockapi.MockMonitoringV1Interface
}

func (m *mockMon) AlertPolicy() monitoring.MonitoringV1AlertPolicyInterface {
	return dpol
}

func (m *mockMon) Alert() monitoring.MonitoringV1AlertInterface {
	return dal
}

func (m *mockMon) AlertDestination() monitoring.MonitoringV1AlertDestinationInterface {
	return ddestPol
}

// Mock AlertPolicy type.
type mockAP struct {
	*mockapi.MockMonitoringV1AlertPolicyInterface
}

func (mAl *mockAP) List(ctx context.Context, options *api.ListWatchOptions) ([]*monitoring.AlertPolicy, error) {
	return nil, nil
}

// Mock Alert type.
type mockAl struct {
	*mockapi.MockMonitoringV1AlertInterface
}

func (mAl *mockAl) List(ctx context.Context, options *api.ListWatchOptions) ([]*monitoring.Alert, error) {
	return nil, nil
}

// Mock AlertPolicy type.
type mockAD struct {
	*mockapi.MockMonitoringV1AlertDestinationInterface
}

func (mAD *mockAD) List(ctx context.Context, options *api.ListWatchOptions) ([]*monitoring.AlertDestination, error) {
	return nil, nil
}

// Mock watcher engine.
type mockWatcher struct {
	watcher.Interface
	status   bool
	genError bool
}

func (mw *mockWatcher) Run(ctx context.Context, apiClient apiservice.Services) (<-chan *kvstore.WatchEvent, <-chan error, error) {
	if mw.genError {
		return nil, nil, fmt.Errorf("error testing")
	}
	mw.status = true
	return nil, wErrCh, nil
}

func (mw *mockWatcher) Stop() {
	mw.status = false
}

func (mw *mockWatcher) GetRunningStatus() bool {
	return mw.status
}

// Mock policy engine.
type mockPolicyengine struct {
	policyengine.Interface
	status   bool
	genError bool
}

func (mpe *mockPolicyengine) Run(ctx context.Context, inchan <-chan *kvstore.WatchEvent) (<-chan *policyengine.PEOutput, <-chan error, error) {
	if mpe.genError {
		return nil, nil, fmt.Errorf("error testing")
	}
	mpe.status = true
	return nil, pErrCh, nil
}

func (mpe *mockPolicyengine) Stop() {
	mpe.status = false
}

func (mpe *mockPolicyengine) GetRunningStatus() bool {
	return mpe.status
}

// Mock alert engine.
type mockAlertengine struct {
	alertengine.Interface
	status   bool
	genError bool
}

func (mae *mockAlertengine) Run(context.Context, apiservice.Services, <-chan *policyengine.PEOutput) (<-chan *alertengine.AEOutput, <-chan error, error) {
	if mae.genError {
		return nil, nil, fmt.Errorf("error testing")
	}
	mae.status = true
	return nil, aErrCh, nil
}

func (mae *mockAlertengine) Stop() {
	mae.status = false
}

func (mae *mockAlertengine) GetRunningStatus() bool {
	return mae.status
}

// Mock exporter.
type mockExporter struct {
	exporter.Interface
	status   bool
	genError bool
}

func (me *mockExporter) Run(ctx context.Context, apiCl apiservice.Services, inchan <-chan *alertengine.AEOutput) (<-chan error, error) {
	if me.genError {
		return nil, fmt.Errorf("error testing")
	}
	me.status = true
	return eErrCh, nil
}

func (me *mockExporter) Stop() {
	me.status = false
}

func (me *mockExporter) GetRunningStatus() bool {
	return me.status
}

// Variable of mock types.
var dpol = &mockAP{}
var dal = &mockAl{}
var ddestPol = &mockAD{}
var mw = &mockWatcher{}
var mpe = &mockPolicyengine{}
var mae = &mockAlertengine{}
var me = &mockExporter{}
var wErrCh = make(chan error)
var pErrCh = make(chan error)
var aErrCh = make(chan error)
var eErrCh = make(chan error)

func TestInit(t *testing.T) {
	setup(t)
	defer teardown()

	// Reopen alert when alert policy matches DSC object again.
	alert.Spec.State = monitoring.AlertState_RESOLVED.String()
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(5)
	mapi.EXPECT().ClusterV1().Return(Mcluster).Times(1)
	err := am.init()
	AssertOk(t, err, "init() failed, err: %v", err)
	Assert(t, alert.Spec.State == monitoring.AlertState_OPEN.String(), "expected state: %s, got state %s", monitoring.AlertState_OPEN.String(), alert.Spec.State)

	// Clean up objdb.
	am.objdb.Delete(pol)
	am.objdb.Delete(dsc)
	am.objdb.Delete(alert)

	// Resolve alert when alert policy does not match DSC object anymore.
	alert.Spec.State = monitoring.AlertState_OPEN.String()
	req := []*fields.Requirement{&fields.Requirement{Key: "status.primary-mac", Operator: "in", Values: []string{"00ae.cd00.1111"}}}
	pol.Spec.Requirements = req
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(3)
	mapi.EXPECT().ClusterV1().Return(Mcluster).Times(1)
	err = am.init()
	AssertOk(t, err, "init() failed, err: %v", err)
	Assert(t, alert.Spec.State == monitoring.AlertState_RESOLVED.String(), "expected state: %s, got state %s", monitoring.AlertState_RESOLVED.String(), alert.Spec.State)
}

func TestKvOp(t *testing.T) {
	setup(t)
	defer teardown()

	// Set up obj db.
	am.objdb.Add(pol)
	am.objdb.Add(dsc)

	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	err := am.kvOp(alert, alertengine.AlertOpReopen)
	AssertOk(t, err, "kvOp() failed, err: %v", err)
	Assert(t, alert.Spec.State == monitoring.AlertState_OPEN.String(), "expected state: %s, got state %s", monitoring.AlertState_OPEN.String(), alert.Spec.State)

	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	err = am.kvOp(alert, alertengine.AlertOpResolve)
	AssertOk(t, err, "kvOp() failed, err: %v", err)
	Assert(t, alert.Spec.State == monitoring.AlertState_RESOLVED.String(), "expected state: %s, got state %s", monitoring.AlertState_RESOLVED.String(), alert.Spec.State)

	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(1)
	err = am.kvOp(alert, alertengine.AlertOpDelete)
	AssertOk(t, err, "kvOp() failed, err: %v", err)
}

func setup(t *testing.T) {
	// Create mock API client.
	ctrl = gomock.NewController(t)
	mpol = &mockAlertPolicy{mockapi.NewMockMonitoringV1AlertPolicyInterface(ctrl)}
	malert = &mockAlert{mockapi.NewMockMonitoringV1AlertInterface(ctrl)}
	malertDest = &mockAlertDest{mockapi.NewMockMonitoringV1AlertDestinationInterface(ctrl)}
	mapi = mockapi.NewMockServices(ctrl)

	// Create alert mgr.
	aei, err := New(logger, mr)
	AssertOk(t, err, "New alert mgr failed")
	am = aei.(*mgr)
	am.apiClient = mapi

	// Create test alert destination policy.
	alertDestUUID := uuid.NewV1().String()
	alertDest = policygen.CreateAlertDestinationObj(globals.DefaultTenant, globals.DefaultNamespace, alertDestUUID,
		&monitoring.SyslogExport{
			Format: monitoring.MonitoringExportFormat_vname[int32(monitoring.MonitoringExportFormat_SYSLOG_BSD)],
			Targets: []*monitoring.ExportConfig{
				{
					Destination: "127.0.0.1",
					Transport:   fmt.Sprintf("udp/%s", "1234"),
				},
			},
			Config: &monitoring.SyslogExportConfig{
				Prefix:           "pen-alertmgr-exporter-test",
				FacilityOverride: monitoring.SyslogFacility_vname[int32(monitoring.SyslogFacility_LOG_SYSLOG)],
			},
		})
	alertDest.ObjectMeta.ModTime = api.Timestamp{}
	alertDest.ObjectMeta.CreationTime = api.Timestamp{}

	// Create test alert policy.
	req := []*fields.Requirement{&fields.Requirement{Key: "status.primary-mac", Operator: "in", Values: []string{"00ae.cd00.1142"}}}
	pol = policygen.CreateAlertPolicyObj(globals.DefaultTenant, "", CreateAlphabetString(5), "DistributedServiceCard", eventattrs.Severity_INFO, "DSC mac check", req, []string{alertDestUUID})
	polID = fmt.Sprintf("%s/%s", pol.GetName(), pol.GetUUID())

	// Create test dsc object.
	dsc = policygen.CreateSmartNIC("00ae.cd00.1142", "admitted", "naples-1", &cluster.DSCCondition{Type: "healthy", Status: "true", LastTransitionTime: ""})

	// Get ref of dsc object.
	m, _ := runtime.GetObjectMeta(dsc)
	dscRef = &api.ObjectRef{
		Tenant:    "", //`globals.DefaultTenant,
		Namespace: "", //globals.DefaultNamespace,
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

// Mock monitoring type.
// AlertPolicy.List() and Alert.List() return valid policy, alert objects.
type mockMonitoring struct {
	*mockapi.MockMonitoringV1Interface
}

func (m *mockMonitoring) AlertPolicy() monitoring.MonitoringV1AlertPolicyInterface {
	return mpol
}

func (m *mockMonitoring) Alert() monitoring.MonitoringV1AlertInterface {
	return malert
}

func (m *mockMonitoring) AlertDestination() monitoring.MonitoringV1AlertDestinationInterface {
	return malertDest
}

// Mock AlertPolicy type.
type mockAlertPolicy struct {
	*mockapi.MockMonitoringV1AlertPolicyInterface
}

func (mAP *mockAlertPolicy) List(ctx context.Context, options *api.ListWatchOptions) ([]*monitoring.AlertPolicy, error) {
	return []*monitoring.AlertPolicy{pol}, nil
}

// Mock Alert type.
type mockAlert struct {
	*mockapi.MockMonitoringV1AlertInterface
}

func (ma *mockAlert) Update(ctx context.Context, in *monitoring.Alert) (*monitoring.Alert, error) {
	return nil, nil
}

func (ma *mockAlert) Delete(ctx context.Context, objMeta *api.ObjectMeta) (*monitoring.Alert, error) {
	return nil, nil
}

func (ma *mockAlert) List(ctx context.Context, options *api.ListWatchOptions) ([]*monitoring.Alert, error) {
	return []*monitoring.Alert{alert}, nil
}

// Mock Alert Destination type.
type mockAlertDest struct {
	*mockapi.MockMonitoringV1AlertDestinationInterface
}

func (ma *mockAlertDest) Get(ctx context.Context, in *api.ObjectMeta) (*monitoring.AlertDestination, error) {
	return nil, nil
}

func (ma *mockAlertDest) Update(ctx context.Context, in *monitoring.AlertDestination) (*monitoring.AlertDestination, error) {
	return nil, nil
}

func (ma *mockAlertDest) Delete(ctx context.Context, objMeta *api.ObjectMeta) (*monitoring.AlertDestination, error) {
	return nil, nil
}

func (ma *mockAlertDest) List(ctx context.Context, options *api.ListWatchOptions) ([]*monitoring.AlertDestination, error) {
	return []*monitoring.AlertDestination{alertDest}, nil
}

// Mock cluster type.
type MockCluster struct {
	cluster.ClusterV1Interface
}

func (*MockCluster) Cluster() cluster.ClusterV1ClusterInterface {
	return Mclustercluster
}

func (mc *MockCluster) Node() cluster.ClusterV1NodeInterface {
	return MclusterNode
}

func (mc *MockCluster) Host() cluster.ClusterV1HostInterface {
	return MclusterHost
}

func (mc *MockCluster) DistributedServiceCard() cluster.ClusterV1DistributedServiceCardInterface {
	return MclusterDistributedServiceCard
}

func (mc *MockCluster) Tenant() cluster.ClusterV1TenantInterface {
	return MclusterTenant
}

func (mc *MockCluster) Version() cluster.ClusterV1VersionInterface {
	return MclusterVersion
}

func (mc *MockCluster) ConfigurationSnapshot() cluster.ClusterV1ConfigurationSnapshotInterface {
	return MclusterConfigurationSnapshot
}

func (mc *MockCluster) SnapshotRestore() cluster.ClusterV1SnapshotRestoreInterface {
	return MclusterSnapshotRestore
}

func (mc *MockCluster) License() cluster.ClusterV1LicenseInterface {
	return MclusterLicense
}

func (mc *MockCluster) DSCProfile() cluster.ClusterV1DSCProfileInterface {
	return MclusterDSCProfile
}

func (mc *MockCluster) Credentials() cluster.ClusterV1CredentialsInterface {
	return MclusterCredentials
}

func (mc *MockCluster) Watch(ctx context.Context, options *api.AggWatchOptions) (kvstore.Watcher, error) {
	return nil, nil
}

type MockClusterCluster struct {
	cluster.ClusterV1ClusterInterface
}

func (*MockClusterCluster) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.Cluster, error) {
	return nil, nil
}

type MockClusterNode struct {
	cluster.ClusterV1NodeInterface
}

func (mcc *MockClusterNode) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.Node, error) {
	return nil, nil
}

type MockClusterHost struct {
	cluster.ClusterV1HostInterface
}

func (mcc *MockClusterHost) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.Host, error) {
	return nil, nil
}

type MockClusterDistributedServiceCard struct {
	cluster.ClusterV1DistributedServiceCardInterface
}

func (mcc *MockClusterDistributedServiceCard) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.DistributedServiceCard, error) {
	return []*cluster.DistributedServiceCard{dsc}, nil
}

type MockClusterTenant struct {
	cluster.ClusterV1TenantInterface
}

func (mcc *MockClusterTenant) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.Tenant, error) {
	return nil, nil
}

type MockClusterVersion struct {
	cluster.ClusterV1VersionInterface
}

func (mcc *MockClusterVersion) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.Version, error) {
	return nil, nil
}

type MockClusterConfigurationSnapshot struct {
	cluster.ClusterV1ConfigurationSnapshotInterface
}

func (mcc *MockClusterConfigurationSnapshot) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.ConfigurationSnapshot, error) {
	return nil, nil
}

type MockClusterSnapshotRestore struct {
	cluster.ClusterV1SnapshotRestoreInterface
}

func (mcc *MockClusterSnapshotRestore) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.SnapshotRestore, error) {
	return nil, nil
}

type MockClusterLicense struct {
	cluster.ClusterV1LicenseInterface
}

func (mcc *MockClusterLicense) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.License, error) {
	return nil, nil
}

type MockClusterDSCProfile struct {
	cluster.ClusterV1DSCProfileInterface
}

func (mcc *MockClusterDSCProfile) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.DSCProfile, error) {
	return nil, nil
}

type MockClusterCredentials struct {
	cluster.ClusterV1CredentialsInterface
}

func (mcc *MockClusterCredentials) List(ctx context.Context, options *api.ListWatchOptions) ([]*cluster.Credentials, error) {
	return nil, nil
}

// Mock variables.
var Mcluster = &MockCluster{}
var Mclustercluster = &MockClusterCluster{}
var MclusterNode = &MockClusterNode{}
var MclusterHost = &MockClusterHost{}
var MclusterDistributedServiceCard = &MockClusterDistributedServiceCard{}
var MclusterTenant = &MockClusterTenant{}
var MclusterVersion = &MockClusterVersion{}
var MclusterConfigurationSnapshot = &MockClusterConfigurationSnapshot{}
var MclusterSnapshotRestore = &MockClusterSnapshotRestore{}
var MclusterLicense = &MockClusterLicense{}
var MclusterDSCProfile = &MockClusterDSCProfile{}
var MclusterCredentials = &MockClusterCredentials{}
