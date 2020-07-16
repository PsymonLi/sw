package exporter

import (
	"context"
	"fmt"
	"strings"
	"testing"
	"time"

	"github.com/golang/mock/gomock"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/fields"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/monitoring"
	mockapi "github.com/pensando/sw/api/mock"
	"github.com/pensando/sw/events/generated/eventattrs"
	"github.com/pensando/sw/venice/ctrler/alertmgr/alertengine"
	objectdb "github.com/pensando/sw/venice/ctrler/alertmgr/objdb"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
	"github.com/pensando/sw/venice/utils/ref"
	mockresolver "github.com/pensando/sw/venice/utils/resolver/mock"
	"github.com/pensando/sw/venice/utils/runtime"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/policygen"
	"github.com/pensando/sw/venice/utils/testutils/serviceutils"

	uuid "github.com/satori/go.uuid"
)

var (
	logConfig          = log.GetDefaultConfig(fmt.Sprintf("%s.%s", globals.AlertMgr, "test"))
	logger             = log.SetConfig(logConfig)
	_                  = recorder.Override(mockevtsrecorder.NewRecorder("alertmgr_test", logger))
	objdb              = objectdb.New()
	mr                 = mockresolver.New()
	mMonitoring        = &mockMonitoringV1{}
	ctrl               *gomock.Controller
	mapi               *mockapi.MockServices
	mAlertDest         *mockapi.MockMonitoringV1AlertDestinationInterface
	e                  *exporter
	alertDestBSDSyslog *monitoring.AlertDestination
	pol                *monitoring.AlertPolicy
	dsc                *cluster.DistributedServiceCard
	polID              string
	dscRef             *api.ObjectRef
	alert              *monitoring.Alert
	udpSrv1            chan string
	udpSrv2            chan string
)

// Mock monitoring.
type mockMonitoringV1 struct {
	mAlertDest monitoring.MonitoringV1AlertDestinationInterface
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
	return nil
}
func (m *mockMonitoringV1) AlertPolicy() monitoring.MonitoringV1AlertPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) StatsAlertPolicy() monitoring.MonitoringV1StatsAlertPolicyInterface {
	return nil
}
func (m *mockMonitoringV1) AlertDestination() monitoring.MonitoringV1AlertDestinationInterface {
	return m.mAlertDest
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
	// Create mock API client.
	ctrl = gomock.NewController(t)
	mAlertDest = mockapi.NewMockMonitoringV1AlertDestinationInterface(ctrl)
	mMonitoring.mAlertDest = mAlertDest
	mapi = mockapi.NewMockServices(ctrl)

	// Create exporter
	tLogger := logger.WithContext("t_name", t.Name())
	ei, err := New(tLogger, mr, objdb)
	AssertOk(t, err, "New exporter engine failed")
	e = ei.(*exporter)
	e.apiClient = mapi

	// Create UDP syslog server - 1.
	pConn1, udp1, err := serviceutils.StartUDPServer(":0")
	AssertOk(t, err, "failed to start UDP server, err: %v", err)
	defer pConn1.Close()
	tmp1 := strings.Split(pConn1.LocalAddr().String(), ":")
	udpSrv1 = udp1

	// Create UDP syslog server - 2.
	pConn2, udp2, err := serviceutils.StartUDPServer(":0")
	AssertOk(t, err, "failed to start UDP server, err: %v", err)
	defer pConn2.Close()
	tmp2 := strings.Split(pConn2.LocalAddr().String(), ":")
	udpSrv2 = udp2

	// Create alert destination policy.
	alertDestUUID := uuid.NewV1().String()
	alertDestBSDSyslog = policygen.CreateAlertDestinationObj(globals.DefaultTenant, globals.DefaultNamespace, alertDestUUID,
		&monitoring.SyslogExport{
			Format: monitoring.MonitoringExportFormat_vname[int32(monitoring.MonitoringExportFormat_SYSLOG_BSD)],
			Targets: []*monitoring.ExportConfig{
				{
					Destination: "127.0.0.1",
					Transport:   fmt.Sprintf("udp/%s", tmp1[len(tmp1)-1]),
				},
				{
					Destination: "127.0.0.1",
					Transport:   fmt.Sprintf("udp/%s", tmp2[len(tmp2)-1]),
				},
			},
			Config: &monitoring.SyslogExportConfig{
				Prefix:           "pen-alertmgr-exporter-test",
				FacilityOverride: monitoring.SyslogFacility_vname[int32(monitoring.SyslogFacility_LOG_SYSLOG)],
			},
		})
	alertDestBSDSyslog.ObjectMeta.ModTime = api.Timestamp{}
	alertDestBSDSyslog.ObjectMeta.CreationTime = api.Timestamp{}

	// Create test alert policy.
	req := []*fields.Requirement{&fields.Requirement{Key: "status.primary-mac", Operator: "in", Values: []string{"00ae.cd00.1142"}}}
	pol = policygen.CreateAlertPolicyObj(globals.DefaultTenant, "", CreateAlphabetString(5), "DistributedServiceCard", eventattrs.Severity_INFO, "DSC mac check", req, []string{alertDestUUID})
	polID = fmt.Sprintf("%s/%s", pol.GetName(), pol.GetUUID())
	e.objdb.Add(pol)

	// Create test dsc object.
	dsc = policygen.CreateSmartNIC("00ae.cd00.1142", "admitted", "naples-1", &cluster.DSCCondition{Type: "healthy", Status: "true", LastTransitionTime: ""})
	e.objdb.Add(dsc)

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

func TestExporter(t *testing.T) {
	setup(t)
	defer teardown()
	Assert(t, e != nil, "exporter engine nil")

	e.ctx, e.cancel = context.WithCancel(context.Background())

	// Cleanup on exit.
	defer func() {
		mapi.EXPECT().Close().Times(1)
		e.Stop()
		time.Sleep(100 * time.Millisecond)
		Assert(t, !e.GetRunningStatus(), "running flag still set")
	}()

	// Run exporter.
	inCh := make(chan *alertengine.AEOutput)
	errCh, err := e.Run(e.ctx, mapi, inCh)
	time.Sleep(100 * time.Millisecond)
	AssertOk(t, err, "Error running exporter engine")
	Assert(t, errCh != nil, "error channel nil")
	Assert(t, e.GetRunningStatus(), "running flag not set")

	// Add alert destination policy.
	err = objdb.Add(alertDestBSDSyslog)
	AssertOk(t, err, "Add object failed %v", err)
	time.Sleep(1 * time.Second)

	// Create 3 alerts with different severity levels.
	alert2 := ref.DeepCopy(alert).(*monitoring.Alert)
	alert2.Status.Severity = eventattrs.Severity_WARN.String()
	alert3 := ref.DeepCopy(alert).(*monitoring.Alert)
	alert3.Status.Severity = eventattrs.Severity_CRITICAL.String()
	alerts := []*monitoring.Alert{alert, alert2, alert3}

	// Inject alerts.
	for _, a := range alerts {
		go func(a *monitoring.Alert) {
			inCh <- &alertengine.AEOutput{Policy: pol, Alert: a}
		}(a)
	}

	// Receive syslog messages at the UDP servers.
	go func() {
		for {
			select {
			case <-udpSrv1:
			case <-udpSrv2:
			case <-e.ctx.Done():
				return
			}
		}
	}()

	// Verify expected monitoring APIs are called.
	mapi.EXPECT().MonitoringV1().Return(mMonitoring).Times(6)
	mAlertDest.EXPECT().Get(gomock.Any(), gomock.Any()).Return(alertDestBSDSyslog, nil).Times(3)
	mAlertDest.EXPECT().Update(gomock.Any(), gomock.Any()).Times(3)

	time.Sleep(1 * time.Second)

	// Delete alert destination object.
	err = objdb.Delete(alertDestBSDSyslog)
	AssertOk(t, err, "Add object failed %v", err)

	time.Sleep(1 * time.Second)
}
