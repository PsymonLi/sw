package objects

import (
	"fmt"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/iota/test/venice/iotakit/cfg/objClient"
	"github.com/pensando/sw/iota/test/venice/iotakit/testbed"
	"github.com/pensando/sw/venice/utils/log"
)

// MirrorSession represents mirrorsession
type Vpc struct {
	Obj *network.VirtualRouter
}

// MirrorSessionCollection is list of sessions
type VpcObjCollection struct {
	CollectionCommon
	//err      error
	Objs []*Vpc
}

func (v *VpcObjCollection) AddVPC(vpc *Vpc) {
	v.Objs = append(v.Objs, vpc)
}

func NewVPC(tenant string, name string, rmac string, vni uint32, ipam string,
	client objClient.ObjClient, testbed *testbed.TestBed) *VpcObjCollection {
	return &VpcObjCollection{
		Objs: []*Vpc{
			{
				Obj: &network.VirtualRouter{
					TypeMeta:   api.TypeMeta{Kind: "VirtualRouter"},
					ObjectMeta: api.ObjectMeta{Name: name, Tenant: tenant},
					Spec: network.VirtualRouterSpec{
						Type:             "tenant",
						RouterMACAddress: rmac,
						VxLanVNI:         vni,
						RouteImportExport: &network.RDSpec{
							AddressFamily: "l2vpn-evpn",
							RDAuto:        true,
							RD:            nil,
							ExportRTs: []*network.RouteDistinguisher{
								{
									Type:          "type2",
									AdminValue:    api.RDAdminValue{Format: api.ASNFormatRD, Value: 200},
									AssignedValue: 100,
								},
								{
									Type:          "type2",
									AdminValue:    api.RDAdminValue{Format: api.ASNFormatRD, Value: 200},
									AssignedValue: 101,
								},
							},
							ImportRTs: []*network.RouteDistinguisher{
								{
									Type:          "type2",
									AdminValue:    api.RDAdminValue{Format: api.ASNFormatRD, Value: 100},
									AssignedValue: 100,
								},
							},
						},
						DefaultIPAMPolicy: ipam,
					},
				},
			},
		},
		CollectionCommon: CollectionCommon{Testbed: testbed, Client: client},
	}
}

// Commit writes the VPC config to venice
func (voc *VpcObjCollection) Commit() error {
	if voc.HasError() {
		return voc.err
	}
	for _, vpc := range voc.Objs {
		err := voc.Client.CreateVPC(vpc.Obj)
		if err != nil {
			voc.err = err
			log.Infof("Creating VPC failed %v", err)
			return err
		}

		log.Debugf("Created VPC : %#v", vpc.Obj)
	}

	return nil
}

// Update updates the VPC config to venice for pre-existing VPCs
func (voc *VpcObjCollection) Update() error {
	if voc.HasError() {
		return voc.err
	}
	for _, vpc := range voc.Objs {
		err := voc.Client.UpdateVPC(vpc.Obj)
		if err != nil {
			voc.err = err
			log.Infof("Updating VPC failed %v", err)
			return err
		}

		log.Debugf("Updated VPC : %#v", vpc.Obj)
	}

	return nil
}

// Delete deletes all VPCs in the collection
func (voc *VpcObjCollection) Delete() error {
	if voc.err != nil {
		return voc.err
	}

	// walk all sessions and delete them
	for _, v := range voc.Objs {
		err := voc.Client.DeleteVPC(v.Obj)
		if err != nil {
			return err
		}
	}

	return nil
}

func NewVPCCollection(client objClient.ObjClient, testbed *testbed.TestBed) *VpcObjCollection {
	return &VpcObjCollection{
		CollectionCommon: CollectionCommon{Client: client, Testbed: testbed},
	}
}

// SetIPAM sets IPAM on the VPC
func (vpcc *VpcObjCollection) SetIPAM(ipam string) error {

	for _, vpc := range vpcc.Objs {
		vpc.Obj.Spec.DefaultIPAMPolicy = ipam
		err := vpcc.Client.UpdateVPC(vpc.Obj)
		if err != nil {
			return err
		}
	}
	return nil
}

func (v *Vpc) UpdateRMAC(rmac string) error {

	if v.Obj == nil {
		return fmt.Errorf("VPC object not created\n")
	}

	v.Obj.Spec.RouterMACAddress = rmac

	return nil
}

func TenantVPCCollection(tenant string, client objClient.ObjClient, tb *testbed.TestBed) (*VpcObjCollection, error) {
	vpcs, err := client.ListVPC(tenant)
	if err != nil {
		return nil, err
	}

	if len(vpcs) == 0 {
		return nil, fmt.Errorf("No VPCs on tenant %s", tenant)
	}
	vpcc := NewVPCCollection(client, tb)

	for _, vrf := range vpcs {
		if vrf.Spec.Type == "tenant" {
			vpcc.Objs = append(vpcc.Objs, &Vpc{Obj: vrf})
			return vpcc, nil
		}
	}

	return nil, fmt.Errorf("No tenant VPC on %s", tenant)
}

// verifies status for propagation of VPC objects to DSCs
func (vpcc *VpcObjCollection) VerifyPropagationStatus(dscCount int32) error {
	if vpcc.HasError() {
		return vpcc.Error()
	}
	if len(vpcc.Objs) == 0 {
		return nil
	}

	for _, v := range vpcc.Objs {
		if v.Obj.Status.PropagationStatus.GenerationID != v.Obj.ObjectMeta.GenerationID {
			log.Warnf("Propagation generation id did not match: Meta: %+v, PropagationStatus: %+v",
				v.Obj.ObjectMeta, v.Obj.Status.PropagationStatus)
			return fmt.Errorf("Propagation generation id did not match")
		}
		if (v.Obj.Status.PropagationStatus.Updated != dscCount) || (v.Obj.Status.PropagationStatus.Pending != 0) {
			log.Warnf("Propagation status incorrect: Expected updates: %+v, PropagationStatus: %+v",
				dscCount, v.Obj.Status.PropagationStatus)
			return fmt.Errorf("Propagation status was incorrect")
		}
	}

	return nil
}
