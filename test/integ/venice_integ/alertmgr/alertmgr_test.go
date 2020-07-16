package alertmgr

import (
	"context"
	"fmt"
	"testing"
	"time"

	uuid "github.com/satori/go.uuid"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/fields"
	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/events/generated/eventattrs"
	testutils "github.com/pensando/sw/test/utils"
	"github.com/pensando/sw/venice/globals"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/policygen"
)

func TestAlertmgrStates(t *testing.T) {
	ti := tInfo{}
	AssertOk(t, ti.setup(t), "failed to setup test")
	defer ti.teardown()

	// setup authn and get authz token
	userCreds := &auth.PasswordCredential{Username: testutils.TestLocalUser, Password: testutils.TestLocalPassword, Tenant: testutils.TestTenant}
	err := testutils.SetupAuth(ti.apiServerAddr, true, nil, nil, userCreds, ti.logger)
	AssertOk(t, err, "failed to setup authN service, err: %v", err)
	defer testutils.CleanupAuth(ti.apiServerAddr, true, false, userCreds, ti.logger)
	//authzHeader, err := testutils.GetAuthorizationHeader(apiGwAddr, userCreds)
	AssertOk(t, err, "failed to get authZ header, err: %v", err)

	// make sure there are no outstanding alerts
	alerts, err := ti.apiClient.MonitoringV1().Alert().List(context.Background(),
		&api.ListWatchOptions{
			ObjectMeta: api.ObjectMeta{Tenant: globals.DefaultTenant}})
	AssertOk(t, err, "failed to list alerts{ap1-*}, err: %v", err)
	Assert(t, len(alerts) == 0, "expected 0 outstanding alerts, got %d alerts", len(alerts))

	// alert destination - 1: BSD style syslog export
	alertDestBSDSyslog := policygen.CreateAlertDestinationObj(globals.DefaultTenant, globals.DefaultNamespace, uuid.NewV1().String(),
		&monitoring.SyslogExport{
			Format: monitoring.MonitoringExportFormat_SYSLOG_BSD.String(),
			Targets: []*monitoring.ExportConfig{
				{
					Destination: "127.0.0.1",
					Transport:   fmt.Sprintf("TCP/%s", ti.tcpSyslogPort), // TCP or tcp should work
				},
			},
		})
	alertDestBSDSyslog, err = ti.apiClient.MonitoringV1().AlertDestination().Create(context.Background(), alertDestBSDSyslog)
	AssertOk(t, err, "failed to create BSD style syslog export policy, err: %v", err)
	defer ti.apiClient.MonitoringV1().AlertDestination().Delete(context.Background(), alertDestBSDSyslog.GetObjectMeta())
	ti.logger.Infof("Created BSD style syslog export policy")

	// alert destination - 2: RFC5424 style syslog export
	alertDestRFC5424Syslog := policygen.CreateAlertDestinationObj(globals.DefaultTenant, globals.DefaultNamespace, uuid.NewV1().String(),
		&monitoring.SyslogExport{
			Format: monitoring.MonitoringExportFormat_SYSLOG_RFC5424.String(),
			Targets: []*monitoring.ExportConfig{
				{
					Destination: "127.0.0.1",
					Transport:   fmt.Sprintf("tcp/%s", ti.tcpSyslogPort),
				},
			},
		})
	alertDestRFC5424Syslog, err = ti.apiClient.MonitoringV1().AlertDestination().Create(context.Background(), alertDestRFC5424Syslog)
	AssertOk(t, err, "failed to create RFC5424 style syslog export policy, err: %v", err)
	defer ti.apiClient.MonitoringV1().AlertDestination().Delete(context.Background(), alertDestRFC5424Syslog.GetObjectMeta())
	ti.logger.Infof("Created RFC5424 style syslog export policy")

	// add object based alert policy
	req := []*fields.Requirement{&fields.Requirement{Key: "status.version-mismatch", Operator: "equals", Values: []string{"true"}}}
	alertPolicy := policygen.CreateAlertPolicyObj(globals.DefaultTenant, globals.DefaultNamespace, fmt.Sprintf("ap1-%s", uuid.NewV4().String()), "DistributedServiceCard", eventattrs.Severity_INFO, "DSC mac check", req, []string{alertDestBSDSyslog.GetName(), alertDestRFC5424Syslog.GetName()})
	alertPolicy, err = ti.apiClient.MonitoringV1().AlertPolicy().Create(context.Background(), alertPolicy)
	AssertOk(t, err, "failed to add alert policy{ap1-*}, err: %v", err)

	time.Sleep(1 * time.Second)

	// add matching DSC object
	dsc := policygen.CreateSmartNIC("00ae.cd00.1142", "admitted", "naples-1", &cluster.DSCCondition{Type: "healthy", Status: "true", LastTransitionTime: ""})
	dsc.Status.VersionMismatch = true
	dsc, err = ti.apiClient.ClusterV1().DistributedServiceCard().Create(context.Background(), dsc)
	AssertOk(t, err, "failed to add dsc{ap1-*}, err: %v", err)

	// make sure alert got generated
	AssertEventually(t, func() (bool, interface{}) {
		alerts, err := ti.apiClient.MonitoringV1().Alert().List(context.Background(),
			&api.ListWatchOptions{})
		//ObjectMeta:    api.ObjectMeta{Tenant: globals.DefaultTenant}})
		if err != nil {
			return false, err
		}

		// there should be just one, open alert
		if len(alerts) == 1 {
			alert := alerts[0]
			if alert.Spec.State == monitoring.AlertState_OPEN.String() && alert.Status.Resolved == nil {
				return true, nil
			}
		}

		return false, fmt.Sprintf("expected: 1 alert, obtained: %v", len(alerts))
	}, "did not receive the expected alert", string("100ms"), string("5s"))

	// make sure alert got exported
	AssertEventually(t, func() (bool, interface{}) {
		aDest, err := ti.apiClient.MonitoringV1().AlertDestination().Get(context.Background(), alertDestBSDSyslog.GetObjectMeta())
		if err != nil {
			return false, err
		}

		// there should be just one alert
		if aDest.Status.TotalNotificationsSent == 1 {
			return true, nil
		}

		return false, fmt.Sprintf("expected: 1 export, obtained: %v", aDest.Status.TotalNotificationsSent)
	}, "did not export expected number of alerts", string("100ms"), string("10s"))

	AssertEventually(t, func() (bool, interface{}) {
		aDest, err := ti.apiClient.MonitoringV1().AlertDestination().Get(context.Background(), alertDestRFC5424Syslog.GetObjectMeta())
		if err != nil {
			return false, err
		}

		// there should be just one alert
		if aDest.Status.TotalNotificationsSent == 1 {
			return true, nil
		}

		return false, fmt.Sprintf("expected: 1 export, obtained: %v", aDest.Status.TotalNotificationsSent)
	}, "did not export expected number of alerts", string("100ms"), string("10s"))

	// update DSC object such that it no longer matches the alert policy
	dsc.Status.VersionMismatch = false
	dsc, err = ti.apiClient.ClusterV1().DistributedServiceCard().Update(context.Background(), dsc)
	AssertOk(t, err, "failed to update dsc{ap1-*}, err: %v", err)

	// Wait for debounce interval.
	time.Sleep(3 * time.Second)

	// make sure alert is resolved
	AssertEventually(t, func() (bool, interface{}) {
		alerts, err := ti.apiClient.MonitoringV1().Alert().List(context.Background(),
			&api.ListWatchOptions{})
		//ObjectMeta:    api.ObjectMeta{Tenant: globals.DefaultTenant}})
		if err != nil {
			return false, err
		}

		// there should be just one, resolved alert
		if len(alerts) == 1 {
			alert := alerts[0]
			if alert.Spec.State == monitoring.AlertState_RESOLVED.String() && alert.Status.Resolved != nil {
				return true, nil
			}
		}

		return false, fmt.Sprintf("alert not resolved, number of alerts %v", len(alerts))
	}, "alert not in correct state", string("100ms"), string("5s"))

	// update DSC object such that it matches the alert policy again
	dsc.Status.VersionMismatch = true
	dsc, err = ti.apiClient.ClusterV1().DistributedServiceCard().Update(context.Background(), dsc)
	AssertOk(t, err, "failed to update dsc{ap1-*}, err: %v", err)

	// delete dsc before exiting
	defer func() {
		dsc.Spec.Admit = false
		dsc.Status.AdmissionPhase = cluster.DistributedServiceCardStatus_PENDING.String()
		dsc, err := ti.apiClient.ClusterV1().DistributedServiceCard().Update(context.Background(), dsc)
		AssertOk(t, err, "failed to update dsc{ap1-*}, err: %v", err)
		ti.apiClient.ClusterV1().DistributedServiceCard().Delete(context.Background(), dsc.GetObjectMeta())
	}()

	// make sure alert is reopened
	AssertEventually(t, func() (bool, interface{}) {
		alerts, err := ti.apiClient.MonitoringV1().Alert().List(context.Background(),
			&api.ListWatchOptions{})
		//ObjectMeta:    api.ObjectMeta{Tenant: globals.DefaultTenant}})
		if err != nil {

			return false, err
		}

		// there should be just one, open alert
		if len(alerts) == 1 {
			alert := alerts[0]
			if alert.Spec.State == monitoring.AlertState_OPEN.String() && alert.Status.Resolved == nil {
				return true, nil
			}
		}

		return false, fmt.Sprintf("expected alert in open state, number of alerts: %v", len(alerts))
	}, "did not receive the expected alert", string("100ms"), string("5s"))

	// delete alert policy
	ti.apiClient.MonitoringV1().AlertPolicy().Delete(context.Background(), alertPolicy.GetObjectMeta())

	// make sure alert is deleted
	AssertEventually(t, func() (bool, interface{}) {
		alerts, err := ti.apiClient.MonitoringV1().Alert().List(context.Background(),
			&api.ListWatchOptions{})
		//ObjectMeta:    api.ObjectMeta{Tenant: globals.DefaultTenant}})
		if err != nil {
			return false, err
		}

		// there should be zero alerts
		if len(alerts) == 0 {
			return true, nil
		}

		return false, fmt.Sprintf("expected 0 alerts, received %v alerts", len(alerts))
	}, "received alert(s) when none expected", string("100ms"), string("5s"))
}

func TestAlertmgrObjDelete(t *testing.T) {
	ti := tInfo{}
	AssertOk(t, ti.setup(t), "failed to setup test")
	defer ti.teardown()

	// setup authn and get authz token
	userCreds := &auth.PasswordCredential{Username: testutils.TestLocalUser, Password: testutils.TestLocalPassword, Tenant: testutils.TestTenant}
	err := testutils.SetupAuth(ti.apiServerAddr, true, nil, nil, userCreds, ti.logger)
	AssertOk(t, err, "failed to setup authN service, err: %v", err)
	defer testutils.CleanupAuth(ti.apiServerAddr, true, false, userCreds, ti.logger)
	//authzHeader, err := testutils.GetAuthorizationHeader(apiGwAddr, userCreds)
	AssertOk(t, err, "failed to get authZ header, err: %v", err)

	// add object based alert policy
	req := []*fields.Requirement{&fields.Requirement{Key: "status.version-mismatch", Operator: "equals", Values: []string{"true"}}}
	alertPolicy := policygen.CreateAlertPolicyObj(globals.DefaultTenant, globals.DefaultNamespace, fmt.Sprintf("ap1-%s", uuid.NewV4().String()), "DistributedServiceCard", eventattrs.Severity_INFO, "DSC mac check", req, []string{})
	alertPolicy, err = ti.apiClient.MonitoringV1().AlertPolicy().Create(context.Background(), alertPolicy)
	AssertOk(t, err, "failed to add alert policy{ap1-*}, err: %v", err)

	defer ti.apiClient.MonitoringV1().AlertPolicy().Delete(context.Background(), alertPolicy.GetObjectMeta())

	time.Sleep(1 * time.Second)

	// add matching DSC object
	dsc := policygen.CreateSmartNIC("00ae.cd00.1142", "admitted", "naples-1", &cluster.DSCCondition{Type: "healthy", Status: "true", LastTransitionTime: ""})
	dsc.Status.VersionMismatch = true
	dsc, err = ti.apiClient.ClusterV1().DistributedServiceCard().Create(context.Background(), dsc)
	AssertOk(t, err, "failed to add dsc{ap1-*}, err: %v", err)

	// make sure alert got generated
	AssertEventually(t, func() (bool, interface{}) {
		alerts, err := ti.apiClient.MonitoringV1().Alert().List(context.Background(),
			&api.ListWatchOptions{})
		//ObjectMeta:    api.ObjectMeta{Tenant: globals.DefaultTenant}})
		if err != nil {
			return false, err
		}

		// there should be just one, open alert
		if len(alerts) == 1 {
			alert := alerts[0]
			if alert.Spec.State == monitoring.AlertState_OPEN.String() && alert.Status.Resolved == nil {
				return true, nil
			}
		}

		return false, fmt.Sprintf("expected: 1 alert, obtained: %v", len(alerts))
	}, "did not receive the expected alert", string("100ms"), string("5s"))

	// delete DSC object
	dsc.Spec.Admit = false
	dsc.Status.AdmissionPhase = cluster.DistributedServiceCardStatus_PENDING.String()
	dsc, err = ti.apiClient.ClusterV1().DistributedServiceCard().Update(context.Background(), dsc)
	AssertOk(t, err, "failed to delete dsc{ap1-*}, err: %v", err)
	ti.apiClient.ClusterV1().DistributedServiceCard().Delete(context.Background(), dsc.GetObjectMeta())

	// make sure alert is deleted
	AssertEventually(t, func() (bool, interface{}) {
		alerts, err := ti.apiClient.MonitoringV1().Alert().List(context.Background(),
			&api.ListWatchOptions{})
		//ObjectMeta:    api.ObjectMeta{Tenant: globals.DefaultTenant}})
		if err != nil {
			return false, err
		}

		// there should be zero alerts
		if len(alerts) == 0 {
			return true, nil
		}

		return false, fmt.Sprintf("expected 0 alerts, received %v alerts", len(alerts))
	}, "received alert(s) when none expected", string("100ms"), string("5s"))
}
