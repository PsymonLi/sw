// {C} Copyright 2019 Pensando Systems Inc. All rights reserved.

package veniceinteg

import (
	"github.com/pensando/sw/api"

	. "gopkg.in/check.v1"

	"github.com/pensando/sw/api/generated/monitoring"

	"github.com/pensando/sw/venice/globals"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func (it *veniceIntegSuite) TestValidateFwlogPolicy(c *C) {
	ctx, err := it.loggedInCtx()
	AssertOk(c, err, "error in logged in context")

	testFwPolicy := []struct {
		name   string
		fail   bool
		policy *monitoring.FwlogPolicy
	}{
		{
			name: "invalid destination",
			fail: true,
			policy: &monitoring.FwlogPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "fwLogPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Name:   "fwlog-invalid-dest",
					Tenant: globals.DefaultTenant,
				},
				Spec: monitoring.FwlogPolicySpec{
					Targets: []monitoring.ExportConfig{
						{
							Destination: "192.168.100.11",
							Transport:   "tcp/10001",
						},
						{
							Destination: "192.168.1.1.1",
							Transport:   "tcp/10001",
						},
					},
					Format: monitoring.MonitoringExportFormat_SYSLOG_BSD.String(),
					Filter: []string{monitoring.FwlogFilter_FIREWALL_ACTION_ALL.String()},
					Config: &monitoring.SyslogExportConfig{
						FacilityOverride: monitoring.SyslogFacility_LOG_USER.String(),
					},
				},
			},
		},

		{
			name: "create policy",
			fail: false,
			policy: &monitoring.FwlogPolicy{
				TypeMeta: api.TypeMeta{
					Kind: "fwLogPolicy",
				},
				ObjectMeta: api.ObjectMeta{
					Namespace: globals.DefaultNamespace,
					Name:      "fwlog" + globals.DefaultTenant,
					Tenant:    globals.DefaultTenant,
				},
				Spec: monitoring.FwlogPolicySpec{
					VrfName: globals.DefaultVrf,
					Targets: []monitoring.ExportConfig{
						{
							Destination: "192.168.100.11",
							Transport:   "tcp/10001",
						},
						{
							Destination: "192.168.10.1",
							Transport:   "udp/15001",
						},
					},
					Format: monitoring.MonitoringExportFormat_SYSLOG_BSD.String(),
					Filter: []string{monitoring.FwlogFilter_FIREWALL_ACTION_ALL.String()},
					Config: &monitoring.SyslogExportConfig{
						FacilityOverride: monitoring.SyslogFacility_LOG_USER.String(),
					},
				},
			},
		},
	}

	for i := range testFwPolicy {
		_, err := it.restClient.MonitoringV1().FwlogPolicy().Create(ctx, testFwPolicy[i].policy)
		if testFwPolicy[i].fail == true {
			Assert(c, err != nil, "fwlog policy validation failed %+v", testFwPolicy[i])
		} else {
			AssertOk(c, err, "failed to create fwlog policy %+v", testFwPolicy[i])
		}
	}
}
