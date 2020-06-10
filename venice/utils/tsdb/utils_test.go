package tsdb

import (
	"testing"

	"github.com/pensando/sw/api"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func TestGetKeys(t *testing.T) {
	ep := endpoint{}
	ep.ObjectMeta.Tenant = "dept1"
	ep.TypeMeta.Kind = "endpoint"
	ep.ObjectMeta.Name = "vm-zone22"

	keys := make(map[string]string)
	tableName, err := getKeys(&ep, keys)
	AssertOk(t, err, "unable to get keys")
	Assert(t, tableName == "endpoint", "invalid table name: %s", tableName)
	Assert(t, keys["tenant"] == "dept1", "invalid tenant: %s", keys["tenant"])
	Assert(t, keys["name"] == "vm-zone22", "invalid name: %s", keys["name"])

	ep.ObjectMeta.Namespace = "prod"
	tableName, err = getKeys(&ep, keys)
	AssertOk(t, err, "unable to get keys")
	Assert(t, tableName == "endpoint", "invalid table name: %s", tableName)
	Assert(t, keys["tenant"] == "dept1", "invalid tenant: %s", keys["tenant"])
	Assert(t, keys["name"] == "vm-zone22", "invalid name: %s", keys["name"])
	Assert(t, keys["namespace"] == "prod", "invalid namespace: %s", keys["namespace"])
}

func TestFillFields(t *testing.T) {
	table := &iObj{}
	table.fields = make(map[string]interface{})

	epm := &endpointMetric{}

	err := fillFields(table, epm)
	AssertOk(t, err, "filling fields from metric object")

	_, ok := table.fields["OutgoingConns"].(api.Counter)
	Assert(t, ok, "unable to find field")
	_, ok = epm.OutgoingConns.(api.Counter)
	Assert(t, ok, "unable to find field")

	_, ok = table.fields["IncomingConns"].(api.Counter)
	Assert(t, ok, "unable to find field")
	_, ok = epm.IncomingConns.(api.Counter)
	Assert(t, ok, "unable to find field")

	_, ok = table.fields["Bandwidth"].(api.PrecisionGauge)
	Assert(t, ok, "unable to find field")
	_, ok = epm.Bandwidth.(api.PrecisionGauge)
	Assert(t, ok, "unable to find field")

	_, ok = table.fields["PacketErrors"].(api.Counter)
	Assert(t, ok, "unable to find field")
	_, ok = epm.PacketErrors.(api.Counter)
	Assert(t, ok, "unable to find field")

	_, ok = table.fields["Violations"].(api.Counter)
	Assert(t, ok, "unable to find field")
	_, ok = epm.Violations.(api.Counter)
	Assert(t, ok, "unable to find field")

	_, ok = table.fields["LinkUp"].(api.Bool)
	Assert(t, ok, "unable to find field")
	_, ok = epm.LinkUp.(api.Bool)
	Assert(t, ok, "unable to find field")

	_, ok = table.fields["WorkloadName"].(api.String)
	Assert(t, ok, "unable to find field")
	_, ok = epm.WorkloadName.(api.String)
	Assert(t, ok, "unable to find field")

	_, ok = table.fields["RxPacketSize"].(api.Histogram)
	Assert(t, ok, "unable to find field")
	_, ok = epm.RxPacketSize.(api.Histogram)
	Assert(t, ok, "unable to find field")

	_, ok = table.fields["TxPacketSize"].(api.Histogram)
	Assert(t, ok, "unable to find field")
	_, ok = epm.TxPacketSize.(api.Histogram)
	Assert(t, ok, "unable to find field")

	_, ok = table.fields["RxBandwidth"].(api.Summary)
	Assert(t, ok, "unable to find field")
	_, ok = epm.RxBandwidth.(api.Summary)
	Assert(t, ok, "unable to find field")

	_, ok = table.fields["TxBandwidth"].(api.Summary)
	Assert(t, ok, "unable to find field")
	_, ok = epm.TxBandwidth.(api.Summary)
	Assert(t, ok, "unable to find field")
}
