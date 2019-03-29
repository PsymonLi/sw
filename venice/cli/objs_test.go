package vcli

import (
	"fmt"
	"reflect"
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/workload"
)

func TestWriteObj(t *testing.T) {
	ctx := &cliContext{subcmd: "network", tenant: "default", names: []string{"net52"}, labels: map[string]string{"rack": "two", "internal": "true"}}
	if err := populateGenCtx(ctx); err != nil {
		t.Fatalf("unable to populate gen info - %s", err)
	}

	specKvs := make(map[string]cliField)
	specKvs["type"] = cliField{values: []string{"l2"}}
	specKvs["vlan-id"] = cliField{values: []string{"23"}}

	obj := &network.Network{}
	if err := writeObj(ctx, obj, specKvs); err != nil {
		t.Fatalf("error '%s' writing object", err)
	}
	if obj.Kind != "network" || obj.ObjectMeta.Tenant != "default" ||
		obj.ObjectMeta.Name != "net52" || len(obj.ObjectMeta.Labels) != 2 ||
		obj.Spec.Type != "l2" || obj.Spec.VlanID != 23 {
		t.Fatalf("unable to write to networkObj: %+v", obj)
	}
	fmt.Printf("=========== TestWriteObj = %+v\n", obj)

}

func TestPatchObj(t *testing.T) {
	ctx := &cliContext{subcmd: "network", tenant: "default", names: []string{"net55"}, labels: map[string]string{"rack": "two", "internal": "true"}}
	if err := populateGenCtx(ctx); err != nil {
		t.Fatalf("unable to populate gen info - %s", err)
	}

	obj := &network.Network{
		TypeMeta: api.TypeMeta{Kind: "network"},
		ObjectMeta: api.ObjectMeta{
			Name:   "net55",
			Labels: map[string]string{"label-key1": "label-value1", "label-key2": "label=value2"},
		},
		Spec: network.NetworkSpec{
			Type:   "l2",
			VlanID: 23,
		},
	}

	specKvs := make(map[string]cliField)
	specKvs["type"] = cliField{values: []string{"vxlan"}}
	specKvs["vlan-id"] = cliField{values: []string{"101"}}
	if err := writeObj(ctx, obj, specKvs); err != nil {
		t.Fatalf("error '%s' writing object", err)
	}

	if obj.Kind != "network" || obj.ObjectMeta.Tenant != "default" ||
		obj.ObjectMeta.Name != "net55" || len(obj.ObjectMeta.Labels) != 2 ||
		obj.Spec.Type != "vxlan" || obj.Spec.VlanID != 101 {
		t.Fatalf("unable to write to networkObj: %+v", obj)
	}
}

func TestGetKvs(t *testing.T) {
	ctx := &cliContext{subcmd: "workload"}
	if err := populateGenCtx(ctx); err != nil {
		t.Fatalf("unable to populate gen info - %s", err)
	}

	wObj := &workload.Workload{}
	wObj.Kind = "workload"
	wObj.ObjectMeta.Name = "vm445.112"
	wObj.ObjectMeta.Tenant = "default"
	wObj.Spec.HostName = "esxi-343"
	wObj.Spec.Interfaces = []workload.WorkloadIntfSpec{
		{MACAddress: "00.11.22.44.55.66", MicroSegVlan: 23, ExternalVlan: 100},
		{MACAddress: "00.11.33.44.55.66", MicroSegVlan: 33, ExternalVlan: 200}}

	metaKvs := make(map[string]cliField)
	specKvs := make(map[string]cliField)
	if getObjMetaKvs(wObj, metaKvs); len(metaKvs) == 0 {
		t.Fatalf("unable to get meta kvs from obj")
	}
	if metaKvs["name"].values[0] != "vm445.112" || metaKvs["tenant"].values[0] != "default" {
		t.Fatalf("unable to fetch metaKvs: %+v", metaKvs)
	}

	if getKvs(ctx, wObj, "", specKvs); len(specKvs) == 0 {
		t.Fatalf("unable to get spec kvs from obj")
	}
	if !reflect.DeepEqual(specKvs["host-name"].values, []string{"esxi-343"}) {
		t.Fatalf("unable to get specKvs %+v", specKvs)
	}
	if !reflect.DeepEqual(specKvs["mac-address"].values, []string{"00.11.22.44.55.66", "00.11.33.44.55.66"}) {
		t.Fatalf("unable to get specKvs %+v", specKvs)
	}
	if !reflect.DeepEqual(specKvs["micro-seg-vlan"].values, []string{"23", "33"}) {
		t.Fatalf("unable to get specKvs %+v", specKvs)
	}
	if !reflect.DeepEqual(specKvs["external-vlan"].values, []string{"100", "200"}) {
		t.Fatalf("unable to get specKvs %+v", specKvs)
	}
}

func TestGetKvsSmartNIC(t *testing.T) {
	ctx := &cliContext{subcmd: "smartnic"}
	if err := populateGenCtx(ctx); err != nil {
		t.Fatalf("unable to populate gen info - %s", err)
	}

	snic := cluster.SmartNIC{
		TypeMeta: api.TypeMeta{Kind: "SmartNIC"},
		ObjectMeta: api.ObjectMeta{
			Name: fmt.Sprintf("snic-host-23"),
		},
		Spec: cluster.SmartNICSpec{
			MgmtMode:    "NETWORK",
			NetworkMode: "OOB",
		},
		Status: cluster.SmartNICStatus{
			AdmissionPhase: "ADMITTED",
			PrimaryMAC:     "00:ae:ed:33:33:33",
		},
	}

	objKvs := make(map[string]cliField)
	if getKvs(ctx, snic, "", objKvs); len(objKvs) == 0 {
		t.Fatalf("unable to get spec kvs from obj")
	}
	if !reflect.DeepEqual(objKvs["mgmt-mode"].values, []string{"NETWORK"}) {
		t.Fatalf("unable to get objKvs %+v", objKvs)
	}
	if !reflect.DeepEqual(objKvs["network-mode"].values, []string{"OOB"}) {
		t.Fatalf("unable to get objKvs %+v", objKvs)
	}
	if !reflect.DeepEqual(objKvs["admission-phase"].values, []string{"ADMITTED"}) {
		t.Fatalf("unable to get objKvs %+v", objKvs)
	}
	if !reflect.DeepEqual(objKvs["primary-mac"].values, []string{"00:ae:ed:33:33:33"}) {
		t.Fatalf("unable to get objKvs %+v", objKvs)
	}
}

func TestGetNumItems(t *testing.T) {
	wObjList := &workload.WorkloadList{TypeMeta: api.TypeMeta{Kind: "workload"}, Items: []*workload.Workload{{}, {}, {}}}

	if numItems := getNumItems(wObjList); numItems != 3 {
		t.Fatalf("found invalid #items from a list %d", numItems)
	}

	if numItems := getNumItems(&workload.Workload{}); numItems != 1 {
		t.Fatalf("get number items returned invalid value %d", numItems)
	}

	if numItems := getNumItems([]workload.Workload{}); numItems > 0 {
		t.Fatalf("get number items returned non zero value %d", numItems)
	}
}

func TestGetAllKvs(t *testing.T) {
	ctx := &cliContext{subcmd: "workload"}
	if err := populateGenCtx(ctx); err != nil {
		t.Fatalf("unable to populate gen info - %s", err)
	}

	workload1 := &workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "workload"},
		ObjectMeta: api.ObjectMeta{
			Name:   "workload1",
			Labels: map[string]string{"label-key1": "label-value1", "label-key2": "label=value2"},
		},
		Spec: workload.WorkloadSpec{
			HostName: "node021",
			Interfaces: []workload.WorkloadIntfSpec{
				{MACAddress: "11:11:11:11:11:11", ExternalVlan: 11, MicroSegVlan: 1000},
				{MACAddress: "22:22:22:22:22:22", ExternalVlan: 22, MicroSegVlan: 2000},
				{MACAddress: "33:33:33:33:33:33", ExternalVlan: 33, MicroSegVlan: 3000},
			},
		},
	}
	workload2 := &workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "workload"},
		ObjectMeta: api.ObjectMeta{
			Name:   "workload2",
			Labels: map[string]string{"label-key1": "label-value3", "label-key2": "label=value4"},
		},
		Spec: workload.WorkloadSpec{
			HostName: "node021",
			Interfaces: []workload.WorkloadIntfSpec{
				{MACAddress: "44:44:44:44:44:44", ExternalVlan: 11, MicroSegVlan: 4000},
			},
		},
	}
	workload3 := &workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "workload"},
		ObjectMeta: api.ObjectMeta{
			Name:   "workload3",
			Labels: map[string]string{"label-key1": "label-value5"},
		},
		Spec: workload.WorkloadSpec{
			HostName: "node021",
		},
	}
	wObjList := &workload.WorkloadList{
		TypeMeta: api.TypeMeta{
			Kind: "workload"},
		Items: []*workload.Workload{workload1, workload2, workload3},
	}
	objmKvs, specKvs, objmValidKvs, specValidKvs := getAllKvs(ctx, len(wObjList.Items), wObjList)

	if !objmValidKvs["labels"] || !objmValidKvs["name"] {
		t.Fatalf("unable to find valid objmValidKvs: %+v", objmValidKvs)
	}
	if !specValidKvs["host-name"] || !specValidKvs["mac-address"] || !specValidKvs["micro-seg-vlan"] || !specValidKvs["external-vlan"] {
		t.Fatalf("unable to find valid specValidKvs: %+v", specValidKvs)
	}
	if len(objmKvs) != len(wObjList.Items) {
		t.Fatalf("invalid objmKvs: %+v", objmKvs)
	}
	if len(specKvs) != len(wObjList.Items) {
		t.Fatalf("invalid specKvs: %+v", specKvs)
	}
}
