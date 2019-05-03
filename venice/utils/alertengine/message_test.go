package alertengine

import (
	"testing"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/venice/globals"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/testutils/policygen"
)

// TestGenerateSyslogMessage tests GenerateSyslogMessage(...)
func TestGenerateSyslogMessage(t *testing.T) {
	alertObj := policygen.CreateAlertObj(globals.DefaultTenant, globals.DefaultNamespace, CreateAlphabetString(5), monitoring.AlertState_OPEN, "test-alert1", nil, nil, nil)
	tcs := []struct {
		alert *monitoring.Alert
	}{
		{
			alert: alertObj,
		},
	}

	for _, tc := range tcs {
		syslogMessage := GenerateSyslogMessage(tc.alert)
		Assert(t, syslogMessage.MsgID == alertObj.GetUUID(), "generated message does not match the alert")
		Assert(t, syslogMessage.Msg == alertObj.Status.GetMessage(), "generated message does not match the alert")
	}
}
