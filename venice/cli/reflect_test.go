package vcli

import (
	"reflect"
	"testing"

	"github.com/pensando/sw/api/generated/security"
	"github.com/pensando/sw/api/generated/workload"
)

func TestWriteObjStructMap(t *testing.T) {
	ctx := &cliContext{subcmd: "workload"}
	if err := populateGenCtx(ctx); err != nil {
		t.Fatalf("unable to populate gen info - %s", err)
	}

	specKvs := make(map[string]cliField)
	specKvs["host-name"] = cliField{values: []string{"esx-node12"}}
	specKvs["mac-address"] = cliField{values: []string{"0011.2233.4455", "0022.3344.5566"}}
	specKvs["external-vlan"] = cliField{values: []string{"100", "200"}}
	specKvs["micro-seg-vlan"] = cliField{values: []string{"1001", "2001"}}

	obj := getNewObj(ctx)

	if err := writeObjValue(ctx, reflect.ValueOf(obj), specKvs); err != nil {
		t.Fatalf("error '%s' writing object", err)
	}

	wObj := obj.(*workload.Workload)
	if wObj.Spec.HostName != "esx-node12" {
		t.Fatalf("invalid hostname in wObj: %+v", wObj)
	}
	if len(wObj.Spec.Interfaces) != 2 || !reflect.DeepEqual(wObj.Spec.Interfaces,
		[]workload.WorkloadIntfSpec{
			{MACAddress: "0011.2233.4455", ExternalVlan: 100, MicroSegVlan: 1001},
			{MACAddress: "0022.3344.5566", ExternalVlan: 200, MicroSegVlan: 2001}}) {
		t.Fatalf("invalid Interfaces: %+v", wObj.Spec.Interfaces)
	}
}

func TestWriteObjStructSlice(t *testing.T) {
	ctx := &cliContext{subcmd: "networksecuritypolicy"}
	if err := populateGenCtx(ctx); err != nil {
		t.Fatalf("unable to populate gen info - %s", err)
	}

	apps := []string{"tcp/23", "udp/53", "tcp/2300"}
	fromSgs := []string{"sg2", "sg3", "sg4"}
	specKvs := make(map[string]cliField)
	specKvs["attach-groups"] = cliField{values: []string{"sg1", "sg-223"}}
	specKvs["apps"] = cliField{values: apps}
	specKvs["from-security-groups"] = cliField{values: fromSgs}

	obj := getNewObj(ctx)

	if err := writeObjValue(ctx, reflect.ValueOf(obj), specKvs); err != nil {
		t.Fatalf("error '%s' writing object", err)
	}

	sObj := obj.(*security.NetworkSecurityPolicy)
	if !reflect.DeepEqual(sObj.Spec.AttachGroups, []string{"sg1", "sg-223"}) {
		t.Fatalf("unable to find attach groups in networksecuritypolicy: %+v", sObj)
	}
	expectedSGRules := []security.SGRule{
		{Apps: []string{apps[0]}, FromSecurityGroups: []string{fromSgs[0]}},
		{Apps: []string{apps[1]}, FromSecurityGroups: []string{fromSgs[1]}},
		{Apps: []string{apps[2]}, FromSecurityGroups: []string{fromSgs[2]}},
	}
	if len(sObj.Spec.Rules) != 1 && !reflect.DeepEqual(sObj.Spec.Rules, expectedSGRules) {
		t.Fatalf("unable to find Rules:\nGot %d rules %+v\nExpected %d rules %+v",
			len(sObj.Spec.Rules), sObj.Spec.Rules, len(expectedSGRules), expectedSGRules)
	}

}

func TestWalkStruct(t *testing.T) {
	ctx := &cliContext{subcmd: "workload"}
	if err := populateGenCtx(ctx); err != nil {
		t.Fatalf("unable to populate gen info - %s", err)
	}

	walkStruct(ctx.structInfo, 0)

	ctx = &cliContext{subcmd: "networksecuritypolicy"}
	if err := populateGenCtx(ctx); err != nil {
		t.Fatalf("unable to populate gen info - %s", err)
	}

	walkStruct(ctx.structInfo, 0)
}
