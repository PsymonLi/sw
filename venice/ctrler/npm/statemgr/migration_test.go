// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package statemgr

import (
	"fmt"
	"testing"
	"time"

	"github.com/pensando/sw/venice/utils/log"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/events/generated/eventtypes"
	"github.com/pensando/sw/venice/utils/events/recorder"
	mockevtsrecorder "github.com/pensando/sw/venice/utils/events/recorder/mock"
	"github.com/pensando/sw/venice/utils/ref"
	. "github.com/pensando/sw/venice/utils/testutils"
)

var (
	// create mock events recorder
	eventRecorder = mockevtsrecorder.NewRecorder("migration_test",
		log.GetNewLogger(log.GetDefaultConfig("migration_test")))
	_ = recorder.Override(eventRecorder)
)

const (
	InsertionProfile   = "InsertionEnforcePolicy"
	TransparentProfile = "TransparentBasenetPolicy"
)

// TestMigrationWorkloadMigration tests for successful workload migration
// All workload migration requests are received by NPM as a workload update
// which is translated into endpoint update
func TestMigrationWorkloadMigration(t *testing.T) {
	// create network state manager
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	sourceHost := "testHost"
	destHost := "testHost-2"

	setupTopo(stateMgr, sourceHost, destHost, t)

	// workload params
	wr := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "testWorkload",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: workload.WorkloadSpec{
			HostName: "testHost",
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 100,
					ExternalVlan: 1,
				},
			},
		},
	}

	// create the workload
	err = stateMgr.ctrler.Workload().Create(&wr)
	AssertOk(t, err, "Could not create the workload")
	start := time.Now()

	done := false
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindNetwork("default", "Network-Vlan-1")
		if err == nil {
			if !done {
				timeTrack(start, "Network create took")
				done = true
			}
			return true, nil
		}
		return false, nil
	}, "Network not foud", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "5s")

	// verify we can find the endpoint associated with the workload
	foundEp, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
	AssertOk(t, err, "Could not find the endpoint")
	Assert(t, (foundEp.Endpoint.Status.WorkloadName == wr.Name), "endpoint params did not match")

	// update workload external vlan
	nwr := ref.DeepCopy(wr).(workload.Workload)
	nwr.Spec.HostName = "testHost-2"
	nwr.Spec.Interfaces[0].MicroSegVlan = 200
	nwr.Status.HostName = "testHost"
	nwr.Status.MigrationStatus = &workload.WorkloadMigrationStatus{
		Stage:  "migration-start",
		Status: workload.WorkloadMigrationStatus_NONE.String(),
	}
	nwr.Status.Interfaces = []workload.WorkloadIntfStatus{
		{
			MACAddress:   "1001.0203.0405",
			MicroSegVlan: 100,
			ExternalVlan: 1,
		},
	}
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration != nil &&
			ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_START.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "10s")

	// Send final sync
	nwr.Status.MigrationStatus.Stage = "migration-final-sync"
	nwr.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_STARTED.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_FINAL_SYNC.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not in correct stage", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Spec.NodeUUID == ep.Endpoint.Status.NodeUUID {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "20s")

	AssertEventually(t, func() (bool, interface{}) {
		wrk, err := stateMgr.FindWorkload("default", "testWorkload")
		if err == nil && wrk.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_DONE.String() {
			return true, nil
		}
		return false, nil
	}, "Workload not found", "1s", "10s")
}

// TestMigrationWorkloadMigrationAbort starts migration and later aborts it.
// We check if the status in workload object is correctly set
func TestMigrationWorkloadMigrationAbort(t *testing.T) {
	// create network state manager
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	sourceHost := "testHost"
	destHost := "testHost-2"

	setupTopo(stateMgr, sourceHost, destHost, t)

	// workload params
	wr := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "testWorkload",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: workload.WorkloadSpec{
			HostName: "testHost",
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 100,
					ExternalVlan: 1,
				},
			},
		},
	}

	// create the workload
	err = stateMgr.ctrler.Workload().Create(&wr)
	AssertOk(t, err, "Could not create the workload")
	start := time.Now()

	done := false
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindNetwork("default", "Network-Vlan-1")
		if err == nil {
			if !done {
				timeTrack(start, "Network create took")
				done = true
			}
			return true, nil
		}
		return false, nil
	}, "Network not foud", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "5s")

	// verify we can find the endpoint associated with the workload
	foundEp, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
	AssertOk(t, err, "Could not find the endpoint")
	Assert(t, (foundEp.Endpoint.Status.WorkloadName == wr.Name), "endpoint params did not match")

	// update workload external vlan
	nwr := ref.DeepCopy(wr).(workload.Workload)
	nwr.Spec.HostName = "testHost-2"
	nwr.Spec.Interfaces[0].MicroSegVlan = 200
	nwr.Status.HostName = "testHost"
	nwr.Status.MigrationStatus = &workload.WorkloadMigrationStatus{
		Stage:  "migration-start",
		Status: workload.WorkloadMigrationStatus_NONE.String(),
	}
	nwr.Status.Interfaces = []workload.WorkloadIntfStatus{
		{
			MACAddress:   "1001.0203.0405",
			MicroSegVlan: 100,
			ExternalVlan: 1,
		},
	}
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_START.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "1s")

	eventRecorder.ClearEvents()
	// Abort Migration
	nwr = ref.DeepCopy(wr).(workload.Workload)
	nwr.Spec.HostName = "testHost"
	nwr.Spec.Interfaces[0].MicroSegVlan = 100
	nwr.Status.HostName = "testHost"
	nwr.Status.MigrationStatus = &workload.WorkloadMigrationStatus{
		Stage:  workload.WorkloadMigrationStatus_MIGRATION_ABORT.String(),
		Status: workload.WorkloadMigrationStatus_STARTED.String(),
	}
	nwr.Status.Interfaces = []workload.WorkloadIntfStatus{
		{
			MACAddress:   "1001.0203.0405",
			MicroSegVlan: 100,
			ExternalVlan: 1,
		},
	}
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	time.Sleep(12 * time.Second)

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}

		return false, nil
	}, "Endpoint not found", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		wrk, err := stateMgr.FindWorkload("default", "testWorkload")
		if err == nil && wrk.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_FAILED.String() {
			return true, nil
		}
		return false, nil
	}, "Workload not found", "5s", "2m")
}

// TestMigrationWorkloadMigrationTimeout sets a low timeout value for migration, and checks if the status in workload object is correctly set
func TestMigrationWorkloadMigrationTimeout(t *testing.T) {
	// create network state manager
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	sourceHost := "testHost"
	destHost := "testHost-2"

	setupTopo(stateMgr, sourceHost, destHost, t)

	// workload params
	wr := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "testWorkload",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: workload.WorkloadSpec{
			HostName: "testHost",
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 100,
					ExternalVlan: 1,
				},
			},
			// Set a low migration timeout
			MigrationTimeout: "3s",
		},
	}
	// create the workload
	err = stateMgr.ctrler.Workload().Create(&wr)
	AssertOk(t, err, "Could not create the workload")
	start := time.Now()

	done := false
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindNetwork("default", "Network-Vlan-1")
		if err == nil {
			if !done {
				timeTrack(start, "Network create took")
				done = true
			}
			return true, nil
		}
		return false, nil
	}, "Network not foud", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "5s")

	// verify we can find the endpoint associated with the workload
	foundEp, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
	AssertOk(t, err, "Could not find the endpoint")
	Assert(t, (foundEp.Endpoint.Status.WorkloadName == wr.Name), "endpoint params did not match")

	// update workload external vlan
	nwr := ref.DeepCopy(wr).(workload.Workload)
	nwr.Spec.HostName = "testHost-2"
	nwr.Spec.Interfaces[0].MicroSegVlan = 200
	nwr.Status.HostName = "testHost"
	nwr.Status.MigrationStatus = &workload.WorkloadMigrationStatus{
		Stage:  "migration-start",
		Status: workload.WorkloadMigrationStatus_NONE.String(),
	}
	nwr.Status.Interfaces = []workload.WorkloadIntfStatus{
		{
			MACAddress:   "1001.0203.0405",
			MicroSegVlan: 100,
			ExternalVlan: 1,
		},
	}
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_START.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "1s")

	eventRecorder.ClearEvents()
	time.Sleep(10 * time.Second)

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.WorkloadMigrationStatus_DONE.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		wrk, err := stateMgr.FindWorkload("default", "testWorkload")
		if err == nil && wrk.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_TIMED_OUT.String() {
			for _, evt := range eventRecorder.GetEvents() {
				if evt.EventType == eventtypes.MIGRATION_TIMED_OUT.String() {
					return true, nil
				}
			}
		}
		return false, nil
	}, "Workload not found", "1s", "1s")
}

func setupTopo(stateMgr *Statemgr, sourceHost, destHost string, t *testing.T) {
	// create tenant
	err := createTenant(t, stateMgr, "default")
	AssertOk(t, err, "Error creating the tenant")

	// create DSC profile
	dscprof := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name: InsertionProfile,
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: cluster.DSCProfileSpec_VIRTUALIZED.String(),
			FeatureSet:       cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String(),
		},
	}

	err = stateMgr.ctrler.DSCProfile().Create(&dscprof)
	AssertOk(t, err, "Could not create the smartNic profile")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindDSCProfile("", InsertionProfile)
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Did not find DSCProfile", "1s", "2s")

	dscprof = cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name: TransparentProfile,
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: cluster.DSCProfileSpec_HOST.String(),
			FeatureSet:       cluster.DSCProfileSpec_SMARTNIC.String(),
		},
	}
	err = stateMgr.ctrler.DSCProfile().Create(&dscprof)
	AssertOk(t, err, "Could not create the smartNic profile")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindDSCProfile("", TransparentProfile)
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Did not find DSCProfile", "1s", "2s")

	// smartNic params
	snic := cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{Kind: "DistributedServiceCard"},
		ObjectMeta: api.ObjectMeta{
			Name: "0001.0203.0405",
		},
		Spec: cluster.DistributedServiceCardSpec{
			DSCProfile: InsertionProfile,
			ID:         "testDistributedServiceCard",
		},
		Status: cluster.DistributedServiceCardStatus{
			AdmissionPhase: cluster.DistributedServiceCardStatus_ADMITTED.String(),
			PrimaryMAC:     "0001.0203.0405",
			Host:           "dsc-source",
			IPConfig: &cluster.IPConfig{
				IPAddress: "10.20.30.11/16",
			},
		},
	}
	// create the smartNic
	err = stateMgr.ctrler.DistributedServiceCard().Create(&snic)
	AssertOk(t, err, "Could not create the smartNic")

	// host params
	host := cluster.Host{
		TypeMeta: api.TypeMeta{Kind: "Host"},
		ObjectMeta: api.ObjectMeta{
			Name:   sourceHost,
			Tenant: "default",
		},
		Spec: cluster.HostSpec{
			DSCs: []cluster.DistributedServiceCardID{
				{
					MACAddress: "0001.0203.0405",
				},
			},
		},
	}

	// create the host
	err = stateMgr.ctrler.Host().Create(&host)
	AssertOk(t, err, "Could not create the host")

	// smartNic params
	snicDest := cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{Kind: "DistributedServiceCard"},
		ObjectMeta: api.ObjectMeta{
			Name: "0001.0203.0406",
		},
		Spec: cluster.DistributedServiceCardSpec{
			DSCProfile: InsertionProfile,
			ID:         "testDistributedServiceCard2",
		},
		Status: cluster.DistributedServiceCardStatus{
			AdmissionPhase: cluster.DistributedServiceCardStatus_ADMITTED.String(),
			PrimaryMAC:     "0001.0203.0406",
			Host:           "dsc-destintion",
			IPConfig: &cluster.IPConfig{
				IPAddress: "10.20.30.12/16",
			},
		},
	}

	// create the smartNic
	err = stateMgr.ctrler.DistributedServiceCard().Create(&snicDest)
	AssertOk(t, err, "Could not create the smartNic")

	// host params
	hostDest := cluster.Host{
		TypeMeta: api.TypeMeta{Kind: "Host"},
		ObjectMeta: api.ObjectMeta{
			Name:   destHost,
			Tenant: "default",
		},
		Spec: cluster.HostSpec{
			DSCs: []cluster.DistributedServiceCardID{
				{
					MACAddress: "0001.0203.0406",
				},
			},
		},
	}

	// create the host
	err = stateMgr.ctrler.Host().Create(&hostDest)
	AssertOk(t, err, "Could not create the destination host")

}

func TestMigrationOnCreateWithMigrationStageFinalSync(t *testing.T) {
	// create network state manager
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	sourceHost := "testHost"
	destHost := "testHost-2"

	setupTopo(stateMgr, sourceHost, destHost, t)
	// workload params
	wr := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "testWorkload",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: workload.WorkloadSpec{
			HostName: sourceHost,
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 100,
					ExternalVlan: 1,
				},
			},
			// Simulating case where 5s are left before
			MigrationTimeout: "5s",
		},
		Status: workload.WorkloadStatus{
			HostName: destHost,
			Interfaces: []workload.WorkloadIntfStatus{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 200,
					ExternalVlan: 1,
				},
			},
			MigrationStatus: &workload.WorkloadMigrationStatus{
				StartedAt: &api.Timestamp{},
				Stage:     workload.WorkloadMigrationStatus_MIGRATION_FINAL_SYNC.String(),
				Status:    workload.WorkloadMigrationStatus_STARTED.String(),
			},
		},
	}

	wr.Status.MigrationStatus.StartedAt.SetTime(time.Now())

	// create the workload
	err = stateMgr.ctrler.Workload().Create(&wr)
	AssertOk(t, err, "Could not create the workload")
	start := time.Now()

	done := false
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindNetwork("default", "Network-Vlan-1")
		if err == nil {
			if !done {
				timeTrack(start, "Network create took")
				done = true
			}
			return true, nil
		}
		return false, nil
	}, "Network not foud", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "1s")

	time.Sleep(1 * time.Second)
	wrFound, err := stateMgr.FindWorkload("default", "testWorkload")
	AssertOk(t, err, "Did not find workload")
	logger.Infof("Got Workload %v", wrFound.Workload)
	Assert(t, (wrFound.Workload.Status.MigrationStatus != nil), "workload migration status is nil")
	Assert(t, (wrFound.Workload.Status.MigrationStatus.Stage == workload.WorkloadMigrationStatus_MIGRATION_FINAL_SYNC.String()), fmt.Sprintf("migration final sync state not found %v", wrFound.Workload))

	time.Sleep(1 * time.Second)
	// verify we can find the endpoint associated with the workload
	foundEp, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
	AssertOk(t, err, "Could not find the endpoint")
	Assert(t, (foundEp.Endpoint.Status.WorkloadName == wr.Name), "endpoint params did not match")
	Assert(t, (foundEp.Endpoint.Status.Migration != nil), fmt.Sprintf("Migration status is nil, EP : %v", foundEp.Endpoint))
	Assert(t, (foundEp.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_FINAL_SYNC.String()), fmt.Sprintf("endpoint not in final sync phase. EP : %v", foundEp.Endpoint))

	// Wait for timer to expire
	time.Sleep(10 * time.Second)

	AssertEventually(t, func() (bool, interface{}) {
		wrFound, err := stateMgr.FindWorkload("default", "testWorkload")
		if err != nil {
			log.Errorf("Workload not found. :( Err : %v", err)
			return false, nil
		}

		if wrFound.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_TIMED_OUT.String() {
			return true, nil
		}

		log.Errorf("Workload found is [%v]", wrFound)
		return false, nil
	}, fmt.Sprintf("Workload not in expected status [%v]", workload.WorkloadMigrationStatus_TIMED_OUT.String()), "1s", "20s")
}

func TestMigrationStartTimeoutLastSync(t *testing.T) {
	// create network state manager
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	sourceHost := "testHost"
	destHost := "testHost-2"

	setupTopo(stateMgr, sourceHost, destHost, t)
	// workload params
	wr := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "testWorkload",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: workload.WorkloadSpec{
			HostName: sourceHost,
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 100,
					ExternalVlan: 1,
				},
			},
			// Simulating case where 5s are left before
			MigrationTimeout: "5s",
		},
		Status: workload.WorkloadStatus{
			HostName: destHost,
			Interfaces: []workload.WorkloadIntfStatus{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 200,
					ExternalVlan: 1,
				},
			},
			MigrationStatus: &workload.WorkloadMigrationStatus{
				StartedAt: &api.Timestamp{},
				Stage:     workload.WorkloadMigrationStatus_MIGRATION_START.String(),
				Status:    workload.WorkloadMigrationStatus_NONE.String(),
			},
		},
	}

	wr.Status.MigrationStatus.StartedAt.SetTime(time.Now())

	// create the workload
	err = stateMgr.ctrler.Workload().Create(&wr)
	AssertOk(t, err, "Could not create the workload")
	start := time.Now()

	done := false
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindNetwork("default", "Network-Vlan-1")
		if err == nil {
			if !done {
				timeTrack(start, "Network create took")
				done = true
			}
			return true, nil
		}
		return false, nil
	}, "Network not foud", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "1s")

	// Wait for timer to expire
	time.Sleep(10 * time.Second)

	AssertEventually(t, func() (bool, interface{}) {
		wrFound, err := stateMgr.FindWorkload("default", "testWorkload")
		if err != nil {
			log.Errorf("Workload not found. :( Err : %v", err)
			return false, nil
		}

		if wrFound.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_TIMED_OUT.String() {
			return true, nil
		}

		log.Errorf("Workload found is [%v]", wrFound)
		return false, nil
	}, fmt.Sprintf("Workload not in expected status [%v]", workload.WorkloadMigrationStatus_TIMED_OUT.String()), "1s", "20s")

	nwr := ref.DeepCopy(wr).(workload.Workload)
	// Send final sync
	nwr.Status.MigrationStatus.Stage = workload.WorkloadMigrationStatus_MIGRATION_FINAL_SYNC.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	// Workload should continue to be in TIMED_OUT status
	AssertEventually(t, func() (bool, interface{}) {
		wrFound, err := stateMgr.FindWorkload("default", "testWorkload")
		if err != nil {
			log.Errorf("Workload not found. :( Err : %v", err)
			return false, nil
		}

		if wrFound.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_TIMED_OUT.String() {
			return true, nil
		}

		log.Errorf("Workload found is [%v]", wrFound)
		return false, nil
	}, fmt.Sprintf("Workload not in expected status [%v]", workload.WorkloadMigrationStatus_TIMED_OUT.String()), "1s", "20s")
}

func TestMigrationStartTimeoutAbort(t *testing.T) {
	// create network state manager
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	sourceHost := "testHost"
	destHost := "testHost-2"

	setupTopo(stateMgr, sourceHost, destHost, t)
	// workload params
	wr := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "testWorkload",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: workload.WorkloadSpec{
			HostName: sourceHost,
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 100,
					ExternalVlan: 1,
				},
			},
			// Simulating case where 5s are left before
			MigrationTimeout: "5s",
		},
		Status: workload.WorkloadStatus{
			HostName: destHost,
			Interfaces: []workload.WorkloadIntfStatus{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 200,
					ExternalVlan: 1,
				},
			},
			MigrationStatus: &workload.WorkloadMigrationStatus{
				StartedAt: &api.Timestamp{},
				Stage:     workload.WorkloadMigrationStatus_MIGRATION_START.String(),
				Status:    workload.WorkloadMigrationStatus_NONE.String(),
			},
		},
	}

	wr.Status.MigrationStatus.StartedAt.SetTime(time.Now())

	// create the workload
	err = stateMgr.ctrler.Workload().Create(&wr)
	AssertOk(t, err, "Could not create the workload")
	start := time.Now()

	done := false
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindNetwork("default", "Network-Vlan-1")
		if err == nil {
			if !done {
				timeTrack(start, "Network create took")
				done = true
			}
			return true, nil
		}
		return false, nil
	}, "Network not foud", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "1s")

	// Wait for timer to expire
	time.Sleep(10 * time.Second)

	AssertEventually(t, func() (bool, interface{}) {
		wrFound, err := stateMgr.FindWorkload("default", "testWorkload")
		if err != nil {
			log.Errorf("Workload not found. :( Err : %v", err)
			return false, nil
		}

		if wrFound.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_TIMED_OUT.String() {
			return true, nil
		}

		log.Errorf("Workload found is [%v]", wrFound)
		return false, nil
	}, fmt.Sprintf("Workload not in expected status [%v]", workload.WorkloadMigrationStatus_TIMED_OUT.String()), "1s", "20s")

	nwr := ref.DeepCopy(wr).(workload.Workload)
	// Send Abort
	nwr.Status.MigrationStatus.Stage = workload.WorkloadMigrationStatus_MIGRATION_ABORT.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	// Workload should continue to be in TIMED_OUT status
	AssertEventually(t, func() (bool, interface{}) {
		wrFound, err := stateMgr.FindWorkload("default", "testWorkload")
		if err != nil {
			log.Errorf("Workload not found. :( Err : %v", err)
			return false, nil
		}

		if wrFound.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_TIMED_OUT.String() {
			return true, nil
		}

		log.Errorf("Workload found is [%v]", wrFound)
		return false, nil
	}, fmt.Sprintf("Workload not in expected status [%v]", workload.WorkloadMigrationStatus_TIMED_OUT.String()), "1s", "20s")
}

func TestMigrationStartFinalSyncAbortAfterDone(t *testing.T) {
	// create network state manager
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	sourceHost := "testHost"
	destHost := "testHost-2"

	setupTopo(stateMgr, sourceHost, destHost, t)
	// workload params
	wr := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "testWorkload",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: workload.WorkloadSpec{
			HostName: sourceHost,
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 100,
					ExternalVlan: 1,
				},
			},
			// Simulating case where 5s are left before
			MigrationTimeout: "60s",
		},
		Status: workload.WorkloadStatus{
			HostName: destHost,
			Interfaces: []workload.WorkloadIntfStatus{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 200,
					ExternalVlan: 1,
				},
			},
			MigrationStatus: &workload.WorkloadMigrationStatus{
				StartedAt: &api.Timestamp{},
				Stage:     workload.WorkloadMigrationStatus_MIGRATION_START.String(),
				Status:    workload.WorkloadMigrationStatus_NONE.String(),
			},
		},
	}

	wr.Status.MigrationStatus.StartedAt.SetTime(time.Now())

	// create the workload
	err = stateMgr.ctrler.Workload().Create(&wr)
	AssertOk(t, err, "Could not create the workload")
	start := time.Now()

	done := false
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindNetwork("default", "Network-Vlan-1")
		if err == nil {
			if !done {
				timeTrack(start, "Network create took")
				done = true
			}
			return true, nil
		}
		return false, nil
	}, "Network not foud", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "1s")

	nwr := ref.DeepCopy(wr).(workload.Workload)
	// Send final sync
	nwr.Status.MigrationStatus.Stage = workload.WorkloadMigrationStatus_MIGRATION_FINAL_SYNC.String()
	nwr.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_STARTED.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_FINAL_SYNC.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not in correct stage", "1s", "5s")

	// Wait for migration to be done
	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_DONE.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not in correct stage", "1s", "20s")

	// Send Abort
	nwr.Status.MigrationStatus.Stage = workload.WorkloadMigrationStatus_MIGRATION_ABORT.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	// Endpoint should continue to be in DONE stage
	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_DONE.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not in correct stage", "1s", "5s")
}

func TestMigrationProfileDowngrade(t *testing.T) {
	// create network state manager
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	sourceHost := "testHost"
	destHost := "testHost-2"

	setupTopo(stateMgr, sourceHost, destHost, t)

	// workload params
	wr := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "testWorkload",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: workload.WorkloadSpec{
			HostName: "testHost",
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 100,
					ExternalVlan: 1,
				},
			},
		},
	}

	// create the workload
	err = stateMgr.ctrler.Workload().Create(&wr)
	AssertOk(t, err, "Could not create the workload")
	start := time.Now()

	done := false
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindNetwork("default", "Network-Vlan-1")
		if err == nil {
			if !done {
				timeTrack(start, "Network create took")
				done = true
			}
			return true, nil
		}
		return false, nil
	}, "Network not found", "1ms", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1ms", "5s")

	// verify we can find the endpoint associated with the workload
	foundEp, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
	AssertOk(t, err, "Could not find the endpoint")
	Assert(t, (foundEp.Endpoint.Status.WorkloadName == wr.Name), "endpoint params did not match")

	// update workload external vlan
	nwr := ref.DeepCopy(wr).(workload.Workload)
	nwr.Spec.HostName = "testHost-2"
	nwr.Spec.Interfaces[0].MicroSegVlan = 200
	nwr.Status.HostName = "testHost"
	nwr.Status.MigrationStatus = &workload.WorkloadMigrationStatus{
		Stage:  "migration-start",
		Status: workload.WorkloadMigrationStatus_NONE.String(),
	}
	nwr.Status.Interfaces = []workload.WorkloadIntfStatus{
		{
			MACAddress:   "1001.0203.0405",
			MicroSegVlan: 100,
			ExternalVlan: 1,
		},
	}
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_START.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1ms", "10s")

	ep, _ := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
	sourceNodeUUID := ep.Endpoint.Status.NodeUUID

	// Downgrade Profile
	// create DSC profile
	dscprof := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name: InsertionProfile,
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: cluster.DSCProfileSpec_VIRTUALIZED.String(),
			FeatureSet:       cluster.DSCProfileSpec_FLOWAWARE.String(),
		},
	}

	err = stateMgr.ctrler.DSCProfile().Update(&dscprof)
	AssertOk(t, err, "Could not create the smartNic profile")

	AssertEventually(t, func() (bool, interface{}) {
		wrk, err := stateMgr.FindWorkload("default", "testWorkload")
		if err == nil && wrk.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_FAILED.String() {
			return true, nil
		}

		if err != nil {
			return false, err
		}

		return false, fmt.Errorf("wrong migration status [%v]  in the workload [testWorkload]", wrk.Workload.Status.MigrationStatus.Status)
	}, "Workload not found", "1ms", "1m")

	// EP move must be aborted
	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_ABORTED.String() {
			return true, nil
		}
		return false, fmt.Errorf("unexpected migration status [%v] for EP testWorkload-1001.0203.0405", ep.Endpoint.Status.Migration.Status)
	}, "Endpoint not in correct stage.", "1ms", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Spec.NodeUUID == ep.Endpoint.Status.NodeUUID && ep.Endpoint.Spec.NodeUUID == sourceNodeUUID {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "20s")

	// Start Another migration, and in the middle deadmit the DSC
	dscprof.Spec.FeatureSet = cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String()
	err = stateMgr.ctrler.DSCProfile().Update(&dscprof)
	AssertOk(t, err, "Could not create the smartNic profile")

	nwr.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_NONE.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err != nil {
			return false, err
		}

		if ep.Endpoint.Status.Migration.Status != workload.EndpointMigrationStatus_START.String() {
			return false, fmt.Errorf("unexpected migration status %v", ep.Endpoint.Status.Migration.Status)
		}

		return true, nil
	}, "Endpoint not found", "1ms", "10s")

	dscState, err := stateMgr.FindDistributedServiceCardByMacAddr("0001.0203.0405")
	AssertOk(t, err, "Could not get the DSC")

	dscState.DistributedServiceCard.Status.AdmissionPhase = cluster.DistributedServiceCardStatus_DECOMMISSIONED.String()
	// decommission the card
	err = stateMgr.ctrler.DistributedServiceCard().Update(&dscState.DistributedServiceCard.DistributedServiceCard)
	AssertOk(t, err, "Could not create the DSC")

	AssertEventually(t, func() (bool, interface{}) {
		wrk, err := stateMgr.FindWorkload("default", "testWorkload")
		if err == nil && wrk.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_FAILED.String() {
			return true, nil
		}

		if err != nil {
			return false, err
		}

		return false, fmt.Errorf("wrong migration status [%v]  in the workload [testWorkload]", wrk.Workload.Status.MigrationStatus.Status)
	}, "Workload not found", "1ms", "1m")

	// EP move must be aborted
	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_ABORTED.String() {
			return true, nil
		}
		return false, fmt.Errorf("unexpected migration status [%v] for EP testWorkload-1001.0203.0405", ep.Endpoint.Status.Migration.Status)
	}, "Endpoint not in correct stage.", "1ms", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Spec.NodeUUID == ep.Endpoint.Status.NodeUUID && ep.Endpoint.Spec.NodeUUID == sourceNodeUUID {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "20s")

	// Successful Migration after aborted migrations
	dscState.DistributedServiceCard.Status.AdmissionPhase = cluster.DistributedServiceCardStatus_ADMITTED.String()
	// admit the card
	err = stateMgr.ctrler.DistributedServiceCard().Update(&dscState.DistributedServiceCard.DistributedServiceCard)
	AssertOk(t, err, "Could not create the DSC")

	nwr.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_NONE.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err != nil {
			return false, err
		}

		if ep.Endpoint.Status.Migration.Status != workload.EndpointMigrationStatus_START.String() {
			return false, fmt.Errorf("unexpected migration status %v", ep.Endpoint.Status.Migration.Status)
		}

		return true, nil
	}, "Endpoint not found", "1ms", "10s")

	// Send final sync
	nwr.Status.MigrationStatus.Stage = "migration-final-sync"
	nwr.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_STARTED.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_FINAL_SYNC.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not in correct stage", "1ms", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Spec.NodeUUID == ep.Endpoint.Status.NodeUUID {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "20s")

	AssertEventually(t, func() (bool, interface{}) {
		wrk, err := stateMgr.FindWorkload("default", "testWorkload")
		if err == nil && wrk.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_DONE.String() {
			return true, nil
		}
		return false, nil
	}, "Workload not found", "1ms", "10s")
}

func TestMigrationWorkloadBackToBack(t *testing.T) {
	// create network state manager
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	sourceHost := "testHost"
	destHost := "testHost-2"

	setupTopo(stateMgr, sourceHost, destHost, t)

	// workload params
	wr := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "testWorkload",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: workload.WorkloadSpec{
			HostName: "testHost",
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 100,
					ExternalVlan: 1,
				},
			},
		},
	}

	// create the workload
	err = stateMgr.ctrler.Workload().Create(&wr)
	AssertOk(t, err, "Could not create the workload")
	start := time.Now()

	done := false
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindNetwork("default", "Network-Vlan-1")
		if err == nil {
			if !done {
				timeTrack(start, "Network create took")
				done = true
			}
			return true, nil
		}
		return false, nil
	}, "Network not foud", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "5s")

	// verify we can find the endpoint associated with the workload
	foundEp, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
	AssertOk(t, err, "Could not find the endpoint")
	Assert(t, (foundEp.Endpoint.Status.WorkloadName == wr.Name), "endpoint params did not match")

	// update workload external vlan
	nwr := ref.DeepCopy(wr).(workload.Workload)
	nwr.Spec.HostName = "testHost-2"
	nwr.Spec.Interfaces[0].MicroSegVlan = 200
	nwr.Status.HostName = "testHost"
	nwr.Status.MigrationStatus = &workload.WorkloadMigrationStatus{
		Stage:  "migration-start",
		Status: workload.WorkloadMigrationStatus_NONE.String(),
	}
	nwr.Status.Interfaces = []workload.WorkloadIntfStatus{
		{
			MACAddress:   "1001.0203.0405",
			MicroSegVlan: 100,
			ExternalVlan: 1,
		},
	}
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		wrk, err := stateMgr.FindWorkload("default", "testWorkload")
		if err == nil && wrk.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_STARTED.String() {
			return true, nil
		}
		return false, nil
	}, "Workload not found", "1s", "10s")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_START.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "10s")

	// retrigger move - must be rejected
	nwr.Spec.HostName = "testHost"
	nwr.Status.HostName = "testHost-2"
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		wrk, err := stateMgr.FindWorkload("default", "testWorkload")
		if err == nil && wrk.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_STARTED.String() {
			return true, nil
		}
		return false, nil
	}, "Workload not found", "1s", "10s")
}

func TestAbortStart(t *testing.T) {
	// create network state manager
	stateMgr, err := newStatemgr()
	if err != nil {
		t.Fatalf("Could not create network manager. Err: %v", err)
		return
	}
	sourceHost := "testHost"
	destHost := "testHost-2"

	setupTopo(stateMgr, sourceHost, destHost, t)

	// workload params
	wr := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "testWorkload",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: workload.WorkloadSpec{
			HostName: "testHost",
			Interfaces: []workload.WorkloadIntfSpec{
				{
					MACAddress:   "1001.0203.0405",
					MicroSegVlan: 100,
					ExternalVlan: 1,
				},
			},
		},
	}

	// create the workload
	err = stateMgr.ctrler.Workload().Create(&wr)
	AssertOk(t, err, "Could not create the workload")
	start := time.Now()

	done := false
	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindNetwork("default", "Network-Vlan-1")
		if err == nil {
			if !done {
				timeTrack(start, "Network create took")
				done = true
			}
			return true, nil
		}
		return false, nil
	}, "Network not foud", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		_, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "5s")

	// verify we can find the endpoint associated with the workload
	foundEp, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
	AssertOk(t, err, "Could not find the endpoint")
	Assert(t, (foundEp.Endpoint.Status.WorkloadName == wr.Name), "endpoint params did not match")

	// update workload external vlan
	nwr := ref.DeepCopy(wr).(workload.Workload)
	nwr.Spec.HostName = "testHost-2"
	nwr.Spec.Interfaces[0].MicroSegVlan = 200
	nwr.Status.HostName = "testHost"
	nwr.Status.MigrationStatus = &workload.WorkloadMigrationStatus{
		Stage:  "migration-start",
		Status: workload.WorkloadMigrationStatus_NONE.String(),
	}
	nwr.Status.Interfaces = []workload.WorkloadIntfStatus{
		{
			MACAddress:   "1001.0203.0405",
			MicroSegVlan: 100,
			ExternalVlan: 1,
		},
	}
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_START.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "10s")

	// Send Abort
	nwr.Status.MigrationStatus.Stage = workload.WorkloadMigrationStatus_MIGRATION_ABORT.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	// Endpoint should continue to be in DONE stage
	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err != nil {
			return false, err
		}

		if ep.Endpoint.Status.Migration.Status != workload.EndpointMigrationStatus_ABORTED.String() {
			return false, fmt.Errorf("Unexpected migration stage %v, expected %v", ep.Endpoint.Status.Migration.Status, workload.EndpointMigrationStatus_ABORTED.String())
		}

		if ep.Endpoint.Status.NodeUUID != ep.Endpoint.Spec.NodeUUID {
			return false, fmt.Errorf("NodeUUID is different in spec[%v] and status[%v]", ep.Endpoint.Spec.NodeUUID, ep.Endpoint.Status.NodeUUID)
		}

		if ep.Endpoint.Spec.NodeUUID != "0001.0203.0405" {
			return false, fmt.Errorf("unexpected NodeUUID %v found. Expected 0001.0203.0405", ep.Endpoint.Spec.NodeUUID)
		}

		return true, nil
	}, "Endpoint not in correct stage", "1s", "5s")

	nwr.Status.MigrationStatus.Stage = "migration-start"
	nwr.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_NONE.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_START.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "10s")

	// Send final sync
	nwr.Status.MigrationStatus.Stage = "migration-final-sync"
	nwr.Status.MigrationStatus.Status = workload.WorkloadMigrationStatus_STARTED.String()
	err = stateMgr.ctrler.Workload().Update(&nwr)
	AssertOk(t, err, "Could not update the workload")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Status.Migration.Status == workload.EndpointMigrationStatus_FINAL_SYNC.String() {
			return true, nil
		}
		return false, nil
	}, "Endpoint not in correct stage", "1s", "1s")

	AssertEventually(t, func() (bool, interface{}) {
		ep, err := stateMgr.FindEndpoint("default", "testWorkload-1001.0203.0405")
		if err == nil && ep.Endpoint.Spec.NodeUUID == ep.Endpoint.Status.NodeUUID {
			return true, nil
		}
		return false, nil
	}, "Endpoint not found", "1s", "20s")

	AssertEventually(t, func() (bool, interface{}) {
		wrk, err := stateMgr.FindWorkload("default", "testWorkload")
		if err == nil && wrk.Workload.Status.MigrationStatus.Status == workload.WorkloadMigrationStatus_DONE.String() {
			return true, nil
		}
		return false, nil
	}, "Workload not found", "1s", "10s")
}
