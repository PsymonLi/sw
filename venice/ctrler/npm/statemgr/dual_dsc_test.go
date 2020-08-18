package statemgr

import (
	"context"
	"fmt"
	"testing"
	//"encoding/json"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/utils/log"
	. "github.com/pensando/sw/venice/utils/testutils"
)

func setupDualNIC(t *testing.T) (*Statemgr, *cluster.Host, *cluster.Host) {
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return nil, nil, nil
	}

	// create tenant
	err = createTenant(t, stateMgr, "default")
	AssertOk(t, err, "Error creating the tenant")

	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "insertion.enforced1",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "VIRTUALIZED",
			FeatureSet:       "FLOWAWARE_FIREWALL",
		},
	}

	// create a network
	stateMgr.ctrler.DSCProfile().Create(&dscProfile)
	AssertEventually(t, func() (bool, interface{}) {

		_, err := stateMgr.FindDSCProfile("", "insertion.enforced1")
		if err == nil {
			return true, nil
		}
		fmt.Printf("Error find ten %v\n", err)
		return false, nil
	}, "Profile not found", "1ms", "1s")

	// host params
	host := cluster.Host{
		TypeMeta: api.TypeMeta{Kind: "Host"},
		ObjectMeta: api.ObjectMeta{
			Name:   "testHost",
			Tenant: "default",
		},
		Spec: cluster.HostSpec{
			DSCs: []cluster.DistributedServiceCardID{
				{
					MACAddress: "0001.0203.0405",
				},
				{
					MACAddress: "0001.0203.0a05",
				},
			},
		},
	}

	// create the host
	err = stateMgr.ctrler.Host().Create(&host)
	AssertOk(t, err, "Could not create the host")

	snic1 := cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{Kind: "DistributedServiceCard"},
		ObjectMeta: api.ObjectMeta{
			Name: "0001.0203.0405",
		},
		Spec: cluster.DistributedServiceCardSpec{
			DSCProfile: "insertion.enforced1",
		},
		Status: cluster.DistributedServiceCardStatus{
			PrimaryMAC:     "0001.0203.0405",
			AdmissionPhase: "admitted",
			NumMacAddress:  24,
		},
	}

	// create the smartNic
	err = stateMgr.ctrler.DistributedServiceCard().Create(&snic1)
	AssertOk(t, err, "Could not create the smartNic")

	snic2 := cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{Kind: "DistributedServiceCard"},
		ObjectMeta: api.ObjectMeta{
			Name: "0001.0203.0a05",
		},
		Spec: cluster.DistributedServiceCardSpec{
			DSCProfile: "insertion.enforced1",
		},
		Status: cluster.DistributedServiceCardStatus{
			PrimaryMAC:     "0001.0203.0a05",
			AdmissionPhase: "admitted",
			NumMacAddress:  24,
		},
	}

	// create the smartNic
	err = stateMgr.ctrler.DistributedServiceCard().Create(&snic2)
	AssertOk(t, err, "Could not create the smartNic")

	return stateMgr, &host, nil
}

func TestDualNicGetMacList(t *testing.T) {

	type test struct {
		baseMac  string
		macCount int
		macList  []string
	}

	testcases := []test{
		{
			baseMac:  "00ae.cd00.0001",
			macCount: 4,
			macList:  []string{"00ae.cd00.0001", "00ae.cd00.0002", "00ae.cd00.0003", "00ae.cd00.0004"},
		},
		{
			baseMac:  "00ae.cd00.0001",
			macCount: 2,
			macList:  []string{"00ae.cd00.0001", "00ae.cd00.0002"},
		},
		{
			baseMac:  "00ae.cd00.00FF",
			macCount: 2,
			macList:  []string{"00ae.cd00.00FF", "00ae.cd00.0100"},
		},
	}

	for _, tc := range testcases {
		log.Infof("Running Testcase : %v", tc)
		macList := GetMacList(tc.baseMac, uint32(tc.macCount))
		Assert(t, len(macList) == tc.macCount, "expected %v macs, got %v", tc.macCount, len(macList))
		for _, mac := range tc.macList {
			found := false
			for _, macOut := range tc.macList {
				if mac == macOut {
					found = true
					break
				}
			}
			Assert(t, found, fmt.Sprintf("MAC %v not found in the MAC list", mac))
		}
	}
}

func TestDualNicWorkloadOps(t *testing.T) {
	stateMgr, host, _ := setupDualNIC(t)
	Assert(t, stateMgr != nil, "Failed to create Dual NIC setup")
	defer stateMgr.Stop()
	type test struct {
		workloadName string
		interfaces   int
		epCount      int
		// Equivalent to the uplink mac which orchhub will populate in the
		// Interface spec for the Workload
		dscInterfaces []string
		// The PrimaryMAC of the DSC corresponding to the uplinks
		dscs []string

		// Parameters to test update
		testUpdate       bool
		updInterfaces    int
		updEpCount       int
		updDscInterfaces []string
		updDscs          []string
	}

	testCases := []test{
		{
			workloadName:  "testWorkload",
			interfaces:    1,
			epCount:       1,
			dscInterfaces: []string{},
		},
		{
			workloadName:  "testWorkload",
			interfaces:    1,
			epCount:       1,
			dscInterfaces: []string{"0001.0203.0405"},
			dscs:          []string{"0001.0203.0405"},
		},
		{
			workloadName:  "testWorkload",
			interfaces:    1,
			epCount:       1,
			dscInterfaces: []string{"0001.0203.040a"},
			dscs:          []string{"0001.0203.0405"},
		},
		{
			workloadName:  "testWorkload",
			interfaces:    1,
			epCount:       1,
			dscInterfaces: []string{"0001.0203.040a", "0001.0203.0a0a"},
			dscs:          []string{"0001.0203.0405", "0001.0203.0a05"},
		},
		{
			workloadName:  "testWorkload",
			interfaces:    4,
			epCount:       4,
			dscInterfaces: []string{"0001.0203.040a"},
			dscs:          []string{"0001.0203.0405"},
		},
		{
			workloadName:  "testWorkload",
			interfaces:    2,
			epCount:       2,
			dscInterfaces: []string{"0001.0203.040a", "0001.0203.0a0a"},
			dscs:          []string{"0001.0203.0405", "0001.0203.0a05"},
		},
		{
			workloadName:  "testWorkload",
			interfaces:    16,
			epCount:       16,
			dscInterfaces: []string{"0001.0203.040a", "0001.0203.0a0a"},
			dscs:          []string{"0001.0203.0405", "0001.0203.0a05"},
		},
		{
			workloadName:     "testWorkload",
			interfaces:       1,
			epCount:          1,
			dscInterfaces:    []string{},
			testUpdate:       true,
			updDscInterfaces: []string{"0001.0203.040a"},
			updDscs:          []string{"0001.0203.0405"},
			updInterfaces:    1,
			updEpCount:       1,
		},
		{
			workloadName:     "testWorkload",
			interfaces:       1,
			epCount:          1,
			dscInterfaces:    []string{},
			testUpdate:       true,
			updDscInterfaces: []string{"0001.0203.040a", "0001.0203.0a0a"},
			updDscs:          []string{"0001.0203.0405", "0001.0203.0a05"},
			updInterfaces:    1,
			updEpCount:       1,
		},
		{
			workloadName:     "testWorkload",
			interfaces:       1,
			epCount:          1,
			dscInterfaces:    []string{"0001.0203.0a0a"},
			dscs:             []string{"0001.0203.0a05"},
			testUpdate:       true,
			updDscInterfaces: []string{"0001.0203.040a"},
			updDscs:          []string{"0001.0203.0405"},
			updInterfaces:    1,
			updEpCount:       1,
		},
		{
			workloadName:     "testWorkload",
			interfaces:       2,
			epCount:          2,
			dscInterfaces:    []string{"0001.0203.0a0a"},
			dscs:             []string{"0001.0203.0a05"},
			testUpdate:       true,
			updDscInterfaces: []string{"0001.0203.040a"},
			updDscs:          []string{"0001.0203.0405"},
			updInterfaces:    1,
			updEpCount:       1,
		},
		{
			workloadName:     "testWorkload",
			interfaces:       1,
			epCount:          1,
			dscInterfaces:    []string{"0001.0203.0a0a"},
			dscs:             []string{"0001.0203.0a05"},
			testUpdate:       true,
			updDscInterfaces: []string{"0001.0203.040a", "0001.0203.0a0a"},
			updDscs:          []string{"0001.0203.0405", "0001.0203.0a05"},
			updInterfaces:    8,
			updEpCount:       8,
		},
	}

	for i, tc := range testCases {
		log.Infof("======= Running testcase[%v] : %v", i, tc)
		ifs := []workload.WorkloadIntfSpec{}
		for i := 0; i < tc.interfaces; i++ {
			ifs = append(ifs, workload.WorkloadIntfSpec{
				MACAddress:    fmt.Sprintf("0011.2222.33%02x", i),
				MicroSegVlan:  100 + uint32(i),
				ExternalVlan:  1,
				DSCInterfaces: tc.dscInterfaces,
			})
		}

		// workload params
		wr := workload.Workload{
			TypeMeta: api.TypeMeta{Kind: "Workload"},
			ObjectMeta: api.ObjectMeta{
				Name:      tc.workloadName,
				Namespace: "default",
				Tenant:    "default",
			},
			Spec: workload.WorkloadSpec{
				HostName:   host.Name,
				Interfaces: ifs,
			},
		}

		// create the workload
		err := stateMgr.ctrler.Workload().Create(&wr)
		AssertOk(t, err, "Could not create the workload")

		// verify network got created
		AssertEventually(t, func() (bool, interface{}) {
			listEP, err := stateMgr.ctrler.Endpoint().List(context.Background(), &api.ListWatchOptions{})
			if err != nil {
				return false, err
			}

			if len(listEP) != tc.epCount {
				return false, fmt.Errorf("Expected %v EPs, found %v EPs", tc.epCount, len(listEP))
			}

			return true, nil
		}, "Incorrect number EPs found", "1ms", "1s")

		listEP, err := stateMgr.ctrler.Endpoint().List(context.Background(), &api.ListWatchOptions{})
		AssertOk(t, err, "failed to get list of EPs %v", err)

		for _, e := range ifs {
			epName := wr.Name + "-" + e.MACAddress
			ep, err := stateMgr.FindEndpoint("default", epName)
			AssertOk(t, err, "Unable to find EP %v", epName)
			for _, d := range tc.dscs {
				found := false
				for _, dEp := range ep.Endpoint.Spec.NodeUUIDList {
					if dEp == d {
						found = true
						break
					}
				}
				Assert(t, found, "DSC %v not found in EP Spec %v", d, ep)

				found = false
				for _, dEp := range ep.Endpoint.Status.NodeUUIDList {
					if dEp == d {
						found = true
						break
					}
				}
				Assert(t, found, "DSC %v not found in EP Status %v", d, ep)
			}
		}

		if tc.testUpdate {
			ifs := []workload.WorkloadIntfSpec{}
			for i := 0; i < tc.updInterfaces; i++ {
				ifs = append(ifs, workload.WorkloadIntfSpec{
					MACAddress:    fmt.Sprintf("0011.2222.33%02x", i),
					MicroSegVlan:  100 + uint32(i),
					ExternalVlan:  1,
					DSCInterfaces: tc.updDscInterfaces,
				})
			}

			// create the workload
			wr.Spec.Interfaces = ifs
			err := stateMgr.ctrler.Workload().Update(&wr)
			AssertOk(t, err, "Could not create the workload")

			// verify network got created
			AssertEventually(t, func() (bool, interface{}) {
				listEP, err := stateMgr.ctrler.Endpoint().List(context.Background(), &api.ListWatchOptions{})
				if err != nil {
					return false, err
				}

				if len(listEP) != tc.updEpCount {
					return false, fmt.Errorf("Expected %v EPs, found %v EPs", tc.updEpCount, len(listEP))
				}

				return true, nil
			}, "Incorrect number EPs found", "1ms", "1s")

			listEP, err := stateMgr.ctrler.Endpoint().List(context.Background(), &api.ListWatchOptions{})
			Assert(t, len(listEP) == tc.updEpCount, "Expected %v EPs, found %v EPs", tc.updEpCount, len(listEP))

			for _, e := range ifs {
				epName := wr.Name + "-" + e.MACAddress
				ep, err := stateMgr.FindEndpoint("default", epName)
				AssertOk(t, err, "Unable to find EP %v", epName)
				for _, d := range tc.updDscs {
					found := false
					for _, dEp := range ep.Endpoint.Spec.NodeUUIDList {
						if dEp == d {
							found = true
							break
						}
					}
					Assert(t, found, "DSC %v not found in EP Spec %v", d, ep)

					found = false
					for _, dEp := range ep.Endpoint.Status.NodeUUIDList {
						if dEp == d {
							found = true
							break
						}
					}
					Assert(t, found, "DSC %v not found in EP Status %v", d, ep)
				}
			}
		}

		err = stateMgr.ctrler.Workload().Delete(&wr)
		AssertOk(t, err, "Could not delete the workload")

		listEP, err = stateMgr.ctrler.Endpoint().List(context.Background(), &api.ListWatchOptions{})
		Assert(t, len(listEP) == 0, "Expected %v EPs, found %v EPs", 0, len(listEP))
	}
}
