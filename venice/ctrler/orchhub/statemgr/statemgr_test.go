package statemgr

import (
	"context"
	"fmt"
	"os"
	"testing"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/orchestration"
	"github.com/pensando/sw/api/generated/workload"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils"
	"github.com/pensando/sw/venice/ctrler/orchhub/utils/channelqueue"
	"github.com/pensando/sw/venice/utils/log"
	. "github.com/pensando/sw/venice/utils/testutils"
	"github.com/pensando/sw/venice/utils/tsdb"
)

var (
	logger log.Logger
)

func (w *MockInstanceManager) watchOrchestratorConfig() {
	for {
		select {
		case <-w.WatchCtx.Done():
			log.Info("Exiting watch for orchestration configuration")
			return
		case evt, ok := <-w.InstanceManagerCh:
			if ok {
				log.Infof("Instance manager got event. %v", evt)
			}
		}
	}
}

func TestNetworkWatcher(t *testing.T) {
	config := log.GetDefaultConfig("test-network")
	config.LogToStdout = true
	config.Filter = log.AllowAllFilter
	logger := log.SetConfig(config)
	sm, im, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	if im == nil {
		t.Fatalf("Failed to create instance manger.")
		return
	}

	orchName := "TestOrch"
	sm.AddProbeChannel(orchName)

	orch := GetOrchestratorConfig(orchName, "user", "pass")
	orchInfo := []*network.OrchestratorInfo{
		{
			Name:      orch.Name,
			Namespace: orch.Namespace,
		},
	}

	expNets := map[string]bool{}
	_, err = CreateNetwork(sm, "default", "aaaa.aaaa.aaaa-vlan100", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
	Assert(t, (err == nil), "network could not be created")
	expNets["aaaa.aaaa.aaaa-vlan100"] = true

	_, err = CreateNetwork(sm, "default", "aaaa.aaaa.bbbb-vlan200", "10.2.1.0/24", "10.2.1.1", 200, nil, orchInfo)
	Assert(t, (err == nil), "network could not be created")
	expNets["aaaa.aaaa.bbbb-vlan200"] = true

	np := network.Network{
		TypeMeta: api.TypeMeta{Kind: "Network"},
		ObjectMeta: api.ObjectMeta{
			Name:      "duplicate_200",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: network.NetworkSpec{
			Type:          network.NetworkType_Bridged.String(),
			VlanID:        200,
			Orchestrators: orchInfo,
		},
		Status: network.NetworkStatus{
			OperState: network.OperState_Rejected.String(),
		},
	}
	// create a network
	err = sm.ctrler.Network().Create(&np)
	Assert(t, (err == nil), "network could not be created")

	_, err = CreateNetwork(sm, "default", "prod-cece-vlan300", "10.3.1.0/24", "10.3.1.1", 300, nil, orchInfo)
	Assert(t, (err == nil), "network could not be created")
	expNets["prod-cece-vlan300"] = true

	receiveNetworks := func(pChl <-chan channelqueue.QItem, rcvNets map[string]bool) {
		for {
			m, ok := <-pChl
			if !ok {
				logger.Infof("probe chl closed")
				return
			}
			msg := m.(channelqueue.Item)
			logger.Infof("Received net %s on probe chl", msg.ObjMeta.Name)
			rcvNets[msg.ObjMeta.Name] = true
		}
	}
	chQ, err := sm.GetProbeChannel(orchName)
	AssertOk(t, err, "Failed to get probe channel")
	rcvNets := map[string]bool{}
	go receiveNetworks(chQ.ReadCh(), rcvNets)

	AssertEventually(t, func() (bool, interface{}) {
		for net := range expNets {
			if _, found := rcvNets[net]; !found {
				return false, fmt.Errorf("Net %s not found", net)
			}
		}
		for net := range rcvNets {
			if _, found := expNets[net]; !found {
				return false, fmt.Errorf("Net %s not enxpected", net)
			}
		}
		return true, nil
	}, "All networks were not received", "1s", "10s")

	err = DeleteNetwork(sm, &np)
	Assert(t, err == nil, "delete network failed")

	// Increase test coverage
	sm.OnNetworkReconnect()
	return
}

func TestClusterCreateList(t *testing.T) {

	sm, im, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	if im == nil {
		t.Fatalf("Failed to create instance manger.")
		return
	}

	go im.watchOrchestratorConfig()
	defer im.WatchCancel()

	clusterConfig := &cluster.Cluster{
		ObjectMeta: api.ObjectMeta{
			Name: "testCluster",
		},
		TypeMeta: api.TypeMeta{
			Kind: "Cluster",
		},
		Spec: cluster.ClusterSpec{
			AutoAdmitDSCs: true,
		},
	}

	err = sm.Controller().Cluster().Create(clusterConfig)
	AssertOk(t, err, "failed to create cluster config")

	err = sm.Controller().Cluster().Update(clusterConfig)
	AssertOk(t, err, "failed to create cluster config")

	// Increase test coverage
	sm.OnClusterReconnect()
}

func TestNetworkCreateList(t *testing.T) {
	sm, im, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	if im == nil {
		t.Fatalf("Failed to create instance manger.")
		return
	}

	go im.watchOrchestratorConfig()
	defer im.WatchCancel()

	orch := GetOrchestratorConfig("myorchestrator", "user", "pass")
	orchInfo := []*network.OrchestratorInfo{
		{
			Name:      orch.Name,
			Namespace: orch.Namespace,
		},
	}

	clusterConfig := &cluster.Cluster{
		ObjectMeta: api.ObjectMeta{
			Name: "testCluster",
		},
		TypeMeta: api.TypeMeta{
			Kind: "Cluster",
		},
		Spec: cluster.ClusterSpec{
			AutoAdmitDSCs: true,
		},
	}

	err = sm.Controller().Cluster().Create(clusterConfig)
	AssertOk(t, err, "failed to create cluster config")

	_, err = CreateNetwork(sm, "default", "aaaa.aaaa.aaaa-vlan100", "10.1.1.0/24", "10.1.1.1", 100, nil, orchInfo)
	Assert(t, (err == nil), "network could not be created")

	_, err = CreateNetwork(sm, "default", "aaaa.aaaa.bbbb-vlan200", "10.2.1.0/24", "10.2.1.1", 200, nil, orchInfo)
	Assert(t, (err == nil), "network could not be created")

	labels := map[string]string{"color": "green"}
	_, err = CreateNetwork(sm, "default", "dev-caca-vlan300", "10.3.1.0/24", "10.3.1.1", 300, labels, orchInfo)
	Assert(t, (err == nil), "network could not be created")

	nw, err := sm.ctrler.Network().List(context.Background(), &api.ListWatchOptions{})
	AssertEquals(t, 3, len(nw), "did not find all networks")

	opts := api.ListWatchOptions{}
	opts.LabelSelector = "color=green"
	nw, err = sm.ctrler.Network().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "found more networks than expected. [%v]", len(nw))

	meta := api.ObjectMeta{
		Name:      "aaaa.aaaa.bbbb-vlan200",
		Namespace: "default",
		Tenant:    "default",
	}

	nobj, err := sm.Controller().Network().Find(&meta)
	Assert(t, err == nil, "did not find the network")

	nobj.Network.ObjectMeta.Labels = labels
	err = sm.Controller().Network().Update(&nobj.Network)
	Assert(t, err == nil, "unable to update the network object")

	nw, err = sm.Controller().Network().List(context.Background(), &opts)
	Assert(t, len(nw) == 2, "expected 2 networks found [%v]", len(nw))

	err = sm.Controller().Network().Delete(&nobj.Network)
	Assert(t, err == nil, "deletion was not successful")

	nw, err = sm.ctrler.Network().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "expected 1, got %v networks", len(nw))

	clusterItems, err := sm.Controller().Cluster().List(context.Background(), &api.ListWatchOptions{})
	Assert(t, len(clusterItems) == 1, "failed to get cluster config")

	err = sm.Controller().Cluster().Delete(clusterConfig)
	AssertOk(t, err, "failed to create cluster config")

	return
}

// createWorkload utility function to create a Workload
func createWorkload(stateMgr *Statemgr, tenant, name string, labels map[string]string) error {
	// Workload params
	np := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      name,
			Namespace: "default",
			Tenant:    "default",
			Labels:    labels,
		},
		Spec:   workload.WorkloadSpec{},
		Status: workload.WorkloadStatus{},
	}

	// create a Workload
	return stateMgr.ctrler.Workload().Create(&np)
}

func TestWorkloadCreateList(t *testing.T) {
	sm, im, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	if im == nil {
		t.Fatalf("Failed to create instance manger.")
		return
	}

	go im.watchOrchestratorConfig()
	defer im.WatchCancel()

	err = createWorkload(sm, "default", "aaaa.aaaa.aaaa", nil)
	Assert(t, (err == nil), "Workload could not be created")

	err = createWorkload(sm, "default", "aaaa.aaaa.bbbb", nil)
	Assert(t, (err == nil), "Workload could not be created")

	labels := map[string]string{"color": "green"}
	err = createWorkload(sm, "default", "dev-caca", labels)
	Assert(t, (err == nil), "Workload could not be created")

	nw, err := sm.ctrler.Workload().List(context.Background(), &api.ListWatchOptions{})
	AssertEquals(t, 3, len(nw), "did not find all Workloads")

	opts := api.ListWatchOptions{}
	opts.LabelSelector = "color=green"
	nw, err = sm.ctrler.Workload().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "found more Workloads than expected. [%v]", len(nw))

	meta := api.ObjectMeta{
		Name:      "aaaa.aaaa.bbbb",
		Namespace: "default",
		Tenant:    "default",
	}

	nobj, err := sm.Controller().Workload().Find(&meta)
	Assert(t, err == nil, "did not find the Workload")

	nobj.Workload.ObjectMeta.Labels = labels
	err = sm.Controller().Workload().Update(&nobj.Workload)
	Assert(t, err == nil, "unable to update the Workload object")

	nw, err = sm.Controller().Workload().List(context.Background(), &opts)
	Assert(t, len(nw) == 2, "expected 2 Workloads found [%v]", len(nw))

	err = sm.Controller().Workload().Delete(&nobj.Workload)
	Assert(t, err == nil, "deletion was not successful")

	nw, err = sm.ctrler.Workload().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "expected 1, got %v Workloads", len(nw))

	sm.OnWorkloadReconnect()
	select {
	case kind := <-sm.ctkitReconnectCh:
		AssertEquals(t, "Workload", kind, "Wrong kind sent over channel")
	default:
		t.Fatalf("Channel had no event")
	}
	return
}

// createHost utility function to create a Host
func createHost(stateMgr *Statemgr, tenant, name, dscMac string, labels map[string]string) error {
	// Host params
	np := cluster.Host{
		TypeMeta: api.TypeMeta{Kind: "Host"},
		ObjectMeta: api.ObjectMeta{
			Name:      name,
			Namespace: "default",
			Tenant:    tenant,
			Labels:    labels,
		},
		Spec: cluster.HostSpec{
			DSCs: []cluster.DistributedServiceCardID{
				{
					MACAddress: dscMac,
				},
			},
		},
		Status: cluster.HostStatus{},
	}

	// create a Host
	return stateMgr.ctrler.Host().Create(&np)
}

func TestHostCreateList(t *testing.T) {
	sm, im, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	if im == nil {
		t.Fatalf("Failed to create instance manger.")
		return
	}

	go im.watchOrchestratorConfig()
	defer im.WatchCancel()

	prodMap := make(map[string]string)
	prodMap[utils.OrchNameKey] = "prod"
	prodMap[utils.NamespaceKey] = "dev"

	err = createHost(sm, "default", "aaaa.aaaa.aaaa", "", prodMap)
	Assert(t, (err == nil), "Host could not be created")

	err = createHost(sm, "default", "aaaa.aaaa.bbbb", "", prodMap)
	Assert(t, (err == nil), "Host could not be created")

	labels := map[string]string{"color": "green"}
	err = createHost(sm, "default", "dev-caca", "", labels)
	Assert(t, (err == nil), "Host could not be created")

	nw, err := sm.ctrler.Host().List(context.Background(), &api.ListWatchOptions{})
	AssertEquals(t, 3, len(nw), "did not find all Hosts")

	opts := api.ListWatchOptions{}
	opts.LabelSelector = "color=green"
	nw, err = sm.ctrler.Host().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "found more Hosts than expected. [%v]", len(nw))

	meta := api.ObjectMeta{
		Name:      "aaaa.aaaa.bbbb",
		Namespace: "default",
	}

	nobj, err := sm.Controller().Host().Find(&meta)
	Assert(t, err == nil, "did not find the Host")

	nobj.Host.ObjectMeta.Labels = labels
	err = sm.Controller().Host().Update(&nobj.Host)
	Assert(t, err == nil, "unable to update the Host object")

	nw, err = sm.Controller().Host().List(context.Background(), &opts)
	Assert(t, len(nw) == 2, "expected 2 Hosts found [%v]", len(nw))

	err = sm.Controller().Host().Delete(&nobj.Host)
	Assert(t, err == nil, "deletion was not successful")

	nw, err = sm.ctrler.Host().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "expected 1, got %v Hosts", len(nw))

	nw, err = sm.ctrler.Host().List(context.Background(), &api.ListWatchOptions{})
	Assert(t, len(nw) == 2, "expected 2, got %v Hosts", len(nw))

	err = sm.DeleteHostByNamespace("prod", "dev")
	Assert(t, err == nil, "did not delete the Host")

	nw, err = sm.ctrler.Host().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "expected 1, got %v Hosts", len(nw))

	sm.OnHostReconnect()
	select {
	case kind := <-sm.ctkitReconnectCh:
		AssertEquals(t, "Host", kind, "Wrong kind sent over channel")
	default:
		t.Fatalf("Channel had no event")
	}

	return
}

// createDistributedServiceCard utility function to create a DistributedServiceCard
func createDistributedServiceCard(stateMgr *Statemgr, name, id, host string, labels map[string]string) error {
	// DistributedServiceCard params
	np := cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{Kind: "DistributedServiceCard"},
		ObjectMeta: api.ObjectMeta{
			Name:   name,
			Labels: labels,
		},
		Spec: cluster.DistributedServiceCardSpec{
			DSCProfile: "default",
			ID:         id,
		},
		Status: cluster.DistributedServiceCardStatus{
			PrimaryMAC: name,
			Host:       host,
		},
	}

	// create a DistributedServiceCard
	return stateMgr.ctrler.DistributedServiceCard().Create(&np)
}

func TestDistributedServiceCardCreateList(t *testing.T) {
	sm, im, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	if im == nil {
		t.Fatalf("Failed to create instance manger.")
		return
	}

	go im.watchOrchestratorConfig()
	defer im.WatchCancel()

	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "default",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "VIRTUALIZED",
			FeatureSet:       "FLOWAWARE_FIREWALL",
		},
	}

	// create DSC Profile
	sm.ctrler.DSCProfile().Create(&dscProfile)
	AssertEventually(t, func() (bool, interface{}) {

		_, err := sm.FindDSCProfile("", "default")
		if err == nil {
			return true, nil
		}
		fmt.Printf("Error find ten %v\n", err)
		return false, nil
	}, "Profile not found", "1ms", "1s")

	err = createDistributedServiceCard(sm, "aaaa.aa00.0000", "", "", nil)
	Assert(t, (err == nil), "DistributedServiceCard could not be created")

	err = createDistributedServiceCard(sm, "aaaa.aa0f.ffff", "", "", nil)
	Assert(t, (err == nil), "DistributedServiceCard could not be created")

	labels := map[string]string{"color": "green"}
	err = createDistributedServiceCard(sm, "aaaa.aa01.12ff", "", "", labels)
	Assert(t, (err == nil), "DistributedServiceCard could not be created")

	// Create card with numMacs of 32
	np := cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{Kind: "DistributedServiceCard"},
		ObjectMeta: api.ObjectMeta{
			Name:   "bbbb.0000.0000",
			Labels: map[string]string{},
		},
		Spec:   cluster.DistributedServiceCardSpec{DSCProfile: "default", ID: "id"},
		Status: cluster.DistributedServiceCardStatus{PrimaryMAC: "bbbb.0000.0000", NumMacAddress: 32},
	}

	// create a DistributedServiceCard
	sm.ctrler.DistributedServiceCard().Create(&np)

	nw, err := sm.ctrler.DistributedServiceCard().List(context.Background(), &api.ListWatchOptions{})
	AssertEquals(t, 4, len(nw), "did not find all DistributedServiceCards")

	opts := api.ListWatchOptions{}
	opts.LabelSelector = "color=green"
	nw, err = sm.ctrler.DistributedServiceCard().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "found more DistributedServiceCards than expected. [%v]", len(nw))

	// Verify DscMap
	AssertEquals(t, (24*3)+32, len(sm.DscMap), "dsc map had incorrect amount of entries")
	entry := sm.DscMap["aaaa.aa00.0017"]
	AssertEquals(t, "aaaa.aa00.0000", entry, "mac entry did not match")

	entry = sm.DscMap["aaaa.aa10.0016"]
	AssertEquals(t, "aaaa.aa0f.ffff", entry, "mac entry did not match")

	entry = sm.DscMap["bbbb.0000.001f"]
	AssertEquals(t, "bbbb.0000.0000", entry, "mac entry did not match")

	meta := api.ObjectMeta{
		Name: "aaaa.aa0f.ffff",
		//DSC is cluster object, tenant not required
		//Tenant:    "default",
	}

	nobj, err := sm.Controller().DistributedServiceCard().Find(&meta)
	Assert(t, err == nil, "did not find the DistributedServiceCard")

	nobj.DistributedServiceCard.ObjectMeta.Labels = labels
	err = sm.Controller().DistributedServiceCard().Update(&nobj.DistributedServiceCard)
	Assert(t, err == nil, "unable to update the DistributedServiceCard object")

	nw, err = sm.Controller().DistributedServiceCard().List(context.Background(), &opts)
	Assert(t, len(nw) == 2, "expected 2 DistributedServiceCards found [%v]", len(nw))

	err = sm.Controller().DistributedServiceCard().Delete(&nobj.DistributedServiceCard)
	Assert(t, err == nil, "deletion was not successful")

	AssertEquals(t, (24*2)+32, len(sm.DscMap), "dsc map had incorrect amount of entries")
	_, ok := sm.DscMap["aaaa.aa10.0016"]
	Assert(t, !ok, "mac entry should have been removed")

	nw, err = sm.ctrler.DistributedServiceCard().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "expected 1, got %v DistributedServiceCards", len(nw))

	// Increase test coverage
	sm.OnDistributedServiceCardReconnect()

	return
}

// createSnapshot utility function to create a snapshotRestore
func createSnapshot(stateMgr *Statemgr) error {
	// Orchestrator params
	snapshot := cluster.SnapshotRestore{
		TypeMeta: api.TypeMeta{Kind: "SnapshotRestore"},
		ObjectMeta: api.ObjectMeta{
			Name: "OrchSnapshot",
		},
		Spec: cluster.SnapshotRestoreSpec{
			SnapshotPath: "test",
		},
		Status: cluster.SnapshotRestoreStatus{
			Status: cluster.SnapshotRestoreStatus_Unknown.String(),
		},
	}

	return stateMgr.ctrler.SnapshotRestore().Create(&snapshot)
}

func TestSnapshotCreateList(t *testing.T) {
	sm, im, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	if im == nil {
		t.Fatalf("Failed to create instance manger.")
		return
	}

	go im.watchOrchestratorConfig()
	defer im.WatchCancel()

	err = createSnapshot(sm)
	AssertOk(t, err, "Snapshot could not be created")

	snapshots, err := sm.ctrler.SnapshotRestore().List(context.Background(), &api.ListWatchOptions{})
	AssertOk(t, err, "failed to list snapshots")

	err = sm.ctrler.SnapshotRestore().Update(&(snapshots[0].SnapshotRestore))
	AssertOk(t, err, "failed to update")

	err = sm.ctrler.SnapshotRestore().Delete(&(snapshots[0].SnapshotRestore))
	AssertOk(t, err, "failed to delete")

	// Increase test coverage
	sm.OnSnapshotRestoreReconnect()
}

// createOrchestrator utility function to create a Orchestrator
func createOrchestrator(stateMgr *Statemgr, tenant, name, namespaceVal string, labels map[string]string) error {
	ns := "testNamespace"
	if len(namespaceVal) > 0 {
		ns = namespaceVal
	}
	// Orchestrator params
	np := orchestration.Orchestrator{
		TypeMeta: api.TypeMeta{Kind: "Orchestrator"},
		ObjectMeta: api.ObjectMeta{
			Name:      name,
			Namespace: "default",
			Tenant:    "default",
			Labels:    labels,
		},
		Spec: orchestration.OrchestratorSpec{
			Namespaces: []*orchestration.NamespaceSpec{
				&orchestration.NamespaceSpec{
					Name: ns,
					Mode: orchestration.NamespaceSpec_Managed.String(),
				},
			},
		},
		Status: orchestration.OrchestratorStatus{},
	}

	// create a Orchestrator
	return stateMgr.ctrler.Orchestrator().Create(&np)
}

func TestOrchestratorCreateList(t *testing.T) {
	sm, im, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	if im == nil {
		t.Fatalf("Failed to create instance manger.")
		return
	}

	go im.watchOrchestratorConfig()
	defer im.WatchCancel()

	err = createOrchestrator(sm, "default", "aaaa.aaaa.aaaa", "", nil)
	AssertOk(t, err, "Orchestrator could not be created")

	err = createOrchestrator(sm, "default", "aaaa.aaaa.bbbb", "", nil)
	AssertOk(t, err, "Orchestrator could not be created")

	labels := map[string]string{"color": "green"}
	err = createOrchestrator(sm, "default", "dev-caca", "", labels)
	AssertOk(t, err, "Orchestrator could not be created")

	nw, err := sm.ctrler.Orchestrator().List(context.Background(), &api.ListWatchOptions{})
	AssertEquals(t, 3, len(nw), "did not find all Orchestrators")

	opts := api.ListWatchOptions{}
	opts.LabelSelector = "color=green"
	nw, err = sm.ctrler.Orchestrator().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "found more Orchestrators than expected. [%v]", len(nw))

	meta := api.ObjectMeta{
		Name:      "aaaa.aaaa.bbbb",
		Namespace: "default",
		//Orch is cluster object, tenant not required
		//Tenant: "default",
	}

	nobj, err := sm.Controller().Orchestrator().Find(&meta)
	Assert(t, err == nil, "did not find the Orchestrator")

	nobj.Orchestrator.ObjectMeta.Labels = labels
	err = sm.Controller().Orchestrator().Update(&nobj.Orchestrator)
	Assert(t, err == nil, "unable to update the Orchestrator object")

	nw, err = sm.Controller().Orchestrator().List(context.Background(), &opts)
	Assert(t, len(nw) == 2, "expected 2 Orchestrators found [%v]", len(nw))

	err = sm.Controller().Orchestrator().Delete(&nobj.Orchestrator)
	Assert(t, err == nil, "deletion was not successful")

	nw, err = sm.ctrler.Orchestrator().List(context.Background(), &opts)
	Assert(t, len(nw) == 1, "expected 1, got %v Orchestrators", len(nw))

	_, err = sm.GetProbeChannel(nw[0].GetName())
	AssertOk(t, err, "Failed to get probe channel")

	// Increase test coverage
	sm.OnDistributedServiceCardReconnect()

	return
}

func TestUpdateDSCProfile(t *testing.T) {
	sm, _, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "default",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "VIRTUALIZED",
			FeatureSet:       "FLOWAWARE_FIREWALL",
		},
	}

	// Create DSC before profile
	err = createDistributedServiceCard(sm, "aaaa.aaaa.aaaa", "", "", nil)
	Assert(t, (err == nil), "DistributedServiceCard could not be created")

	// create DSC Profile
	sm.ctrler.DSCProfile().Create(&dscProfile)
	AssertEventually(t, func() (bool, interface{}) {

		_, err := sm.FindDSCProfile("", "default")
		if err == nil {
			return true, nil
		}
		fmt.Printf("Error find ten %v\n", err)
		return false, nil
	}, "Profile not found", "1ms", "1s")

	dsp, err := sm.FindDSCProfile("", "default")
	AssertOk(t, err, "Failed to get profile")
	AssertEquals(t, 1, len(dsp.dscs), "profile did not have dsc state populated")

	dscProfile.Spec.DeploymentTarget = cluster.DSCProfileSpec_HOST.String()
	dscProfile.Spec.FeatureSet = cluster.DSCProfileSpec_SMARTNIC.String()
	err = sm.ctrler.DSCProfile().Update(&dscProfile)
	Assert(t, (err == nil), "Failed to update dscprofile")

	dscs, err := sm.ListDSCProfiles()
	Assert(t, (err == nil), "List DSC profiles failed")
	Assert(t, len(dscs) == 1, "incorrect number of dscs")

	err = sm.ctrler.DSCProfile().Delete(&dscProfile)
	Assert(t, (err == nil), "Failed to delete dscprofile")
}

func TestFindDSC(t *testing.T) {
	sm, _, err := NewMockStateManager()
	Assert(t, (err == nil), "Statemanager could not be created")

	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "default",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "VIRTUALIZED",
			FeatureSet:       "FLOWAWARE_FIREWALL",
		},
	}

	// create DSC Profile
	sm.ctrler.DSCProfile().Create(&dscProfile)
	AssertEventually(t, func() (bool, interface{}) {

		_, err := sm.FindDSCProfile("", "default")
		if err == nil {
			return true, nil
		}
		fmt.Printf("Error find ten %v\n", err)
		return false, nil
	}, "Profile not found", "1ms", "1s")

	err = createDistributedServiceCard(sm, "aaaa.aaaa.aaaa", "beef-id", "", nil)
	Assert(t, (err == nil), "DistributedServiceCard could not be created")

	d := sm.FindDSC("", "prod-tee")
	Assert(t, d == nil, "DSC found when none expected")

	d = sm.FindDSC("", "beef-id")
	fmt.Printf("D : %v", d)
	Assert(t, d != nil, "DSC not found")

	d = sm.FindDSC("aaaa.aaaa.aaaa", "")
	Assert(t, d != nil, "DSC not found")

	err = sm.RemoveIncompatibleDSCFromOrch("no-dsc", "no-orch")
	Assert(t, err != nil, "Removing Incompatible DSC orch")

	err = sm.RemoveIncompatibleDSCFromOrch("aaaa.aaaa.aaaa", "default")
	Assert(t, err != nil, "Removing Incompatible DSC orch")
}

func TestWorkload(t *testing.T) {
	sm, _, err := NewMockStateManager()
	Assert(t, (err == nil), "Statemanager could not be created")

	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "default",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "VIRTUALIZED",
			FeatureSet:       "FLOWAWARE_FIREWALL",
		},
	}

	// create DSC Profile
	sm.ctrler.DSCProfile().Create(&dscProfile)
	AssertEventually(t, func() (bool, interface{}) {

		_, err := sm.FindDSCProfile("", "default")
		if err == nil {
			return true, nil
		}
		fmt.Printf("Error find ten %v\n", err)
		return false, nil
	}, "Profile not found", "1ms", "1s")

	err = createDistributedServiceCard(sm, "aaaa.aaaa.aaaa", "beef-id", "", nil)
	Assert(t, (err == nil), "DistributedServiceCard could not be created")

	err = createWorkload(sm, "default", "aaaa.aaaa.bbbb", nil)
	Assert(t, err == nil, fmt.Sprintf("failed to create workload. Err : %v", err))

	np := workload.Workload{
		TypeMeta: api.TypeMeta{Kind: "Workload"},
		ObjectMeta: api.ObjectMeta{
			Name:      "aaaa.aaaa.bbbb",
			Namespace: "default",
			Tenant:    "default",
		},
		Spec:   workload.WorkloadSpec{},
		Status: workload.WorkloadStatus{},
	}

	np.Status.MigrationStatus = &workload.WorkloadMigrationStatus{Status: "STARTED"}
	err = sm.ctrler.Workload().Update(&np)
	Assert(t, (err == nil), "Could not update workload object")

	np.Status.MigrationStatus = &workload.WorkloadMigrationStatus{Status: "DONE"}
	err = sm.ctrler.Workload().Update(&np)
	Assert(t, (err == nil), "Could not update workload object")
}

func TestHostMigrationCompat(t *testing.T) {
	sm, _, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	dscMac := "00ae.cd00.1234"
	hostName := "test-host"
	namespaceVal := "dev"
	orchNameVal := "prod"

	prodMap := make(map[string]string)
	prodMap[utils.OrchNameKey] = orchNameVal
	prodMap[utils.NamespaceKey] = namespaceVal

	err = createHost(sm, "default", hostName, dscMac, prodMap)
	Assert(t, (err == nil), "Host could not be created")

	err = sm.CheckHostMigrationCompliance(hostName)
	Assert(t, err != nil, fmt.Sprintf("host %v was expected to be non-compliant", hostName))

	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "default",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: cluster.DSCProfileSpec_VIRTUALIZED.String(),
			FeatureSet:       cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String(),
		},
	}

	// create DSC Profile
	sm.ctrler.DSCProfile().Create(&dscProfile)
	AssertEventually(t, func() (bool, interface{}) {

		_, err := sm.FindDSCProfile("", "default")
		if err == nil {
			return true, nil
		}
		fmt.Printf("Error find ten %v\n", err)
		return false, nil
	}, "Profile not found", "1ms", "1s")

	err = createDistributedServiceCard(sm, dscMac, dscMac, hostName, prodMap)
	Assert(t, (err == nil), "DistributedServiceCard could not be created")

	meta := api.ObjectMeta{
		Name:      dscMac,
		Namespace: "default",
	}

	nobj, err := sm.Controller().DistributedServiceCard().Find(&meta)
	Assert(t, err == nil, "did not find the DistributedServiceCard")

	// ADMIT DSC
	nobj.DistributedServiceCard.Status.AdmissionPhase = cluster.DistributedServiceCardStatus_ADMITTED.String()
	err = sm.Controller().DistributedServiceCard().Update(&nobj.DistributedServiceCard)
	Assert(t, err == nil, "unable to update the DistributedServiceCard object")

	err = createOrchestrator(sm, "default", orchNameVal, namespaceVal, nil)
	AssertOk(t, err, "Orchestrator could not be created")

	err = sm.CheckHostMigrationCompliance(hostName)
	Assert(t, err == nil, fmt.Sprintf("host %v was expected to be compliant", hostName))

	// DE-ADMIT and retry
	nobj.DistributedServiceCard.Status.AdmissionPhase = cluster.DistributedServiceCardStatus_PENDING.String()
	err = sm.Controller().DistributedServiceCard().Update(&nobj.DistributedServiceCard)
	Assert(t, err == nil, "unable to update the DistributedServiceCard object")

	err = sm.CheckHostMigrationCompliance(hostName)
	Assert(t, err != nil, fmt.Sprintf("host %v was expected to be non-compliant", hostName))
	Assert(t, fmt.Sprintf("%v", err) == "DSC for host test-host is not admitted", "Got wrong error message")

	// ADMIT DSC
	nobj.DistributedServiceCard.Status.AdmissionPhase = cluster.DistributedServiceCardStatus_ADMITTED.String()
	err = sm.Controller().DistributedServiceCard().Update(&nobj.DistributedServiceCard)
	Assert(t, err == nil, "unable to update the DistributedServiceCard object")

	err = sm.CheckHostMigrationCompliance(hostName)
	Assert(t, err == nil, fmt.Sprintf("host %v was expected to be compliant", hostName))

	// Change Profile to be FLOWAWARE
	dscProfile.Spec.FeatureSet = cluster.DSCProfileSpec_FLOWAWARE.String()
	err = sm.ctrler.DSCProfile().Create(&dscProfile)
	AssertOk(t, err, fmt.Sprintf("DSCProfile update should have worked, but failed with %v", err))

	err = sm.CheckHostMigrationCompliance(hostName)
	Assert(t, err != nil, fmt.Sprintf("host %v was expected to be non-compliant", hostName))
	Assert(t, fmt.Sprintf("%v", err) == "dsc 00ae.cd00.1234 is not orchestration compatible", "wrong error message received")
}

func TestIncompatList(t *testing.T) {
	sm, _, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	orchName := "prod"
	namespaceVal := "dev"

	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "default",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: cluster.DSCProfileSpec_VIRTUALIZED.String(),
			FeatureSet:       cluster.DSCProfileSpec_FLOWAWARE.String(),
		},
	}

	// create DSC Profile
	sm.ctrler.DSCProfile().Create(&dscProfile)
	AssertEventually(t, func() (bool, interface{}) {

		_, err := sm.FindDSCProfile("", "default")
		if err == nil {
			return true, nil
		}
		fmt.Printf("Error find ten %v\n", err)
		return false, nil
	}, "Profile not found", "1ms", "1s")

	err = createOrchestrator(sm, "default", orchName, namespaceVal, nil)
	AssertOk(t, err, "Orchestrator could not be created")

	dscList := []string{"00ae.cd00.1234", "00ae.cd00.5678", "00ae.cd00.abcd"}
	hostList := []string{"test-host-1", "test-host-2", "test-host-3"}

	prodMap := make(map[string]string)
	prodMap[utils.OrchNameKey] = orchName
	prodMap[utils.NamespaceKey] = namespaceVal

	for i, hostName := range hostList {
		err := createHost(sm, "default", hostName, dscList[i], prodMap)
		Assert(t, (err == nil), "Host could not be created")
	}

	for i, dscMac := range dscList {
		err = createDistributedServiceCard(sm, dscMac, dscMac, hostList[i], prodMap)
		Assert(t, (err == nil), "DistributedServiceCard could not be created")
	}

	orchMeta := api.ObjectMeta{
		Name:      orchName,
		Namespace: "default",
	}

	nobj, err := sm.Controller().Orchestrator().Find(&orchMeta)
	Assert(t, err == nil, "did not find the Orchestrator object")
	Assert(t, len(nobj.Orchestrator.Status.IncompatibleDSCs) == 0, fmt.Sprintf("Expected 0 in incompatible list, found %d", len(nobj.Orchestrator.Status.IncompatibleDSCs)))

	for _, dscMac := range dscList {
		orchMeta := api.ObjectMeta{
			Name:      dscMac,
			Namespace: "default",
		}

		nobj, err := sm.Controller().DistributedServiceCard().Find(&orchMeta)
		Assert(t, err == nil, "did not find the DistributedServiceCard")

		nobj.DistributedServiceCard.Status.AdmissionPhase = cluster.DistributedServiceCardStatus_ADMITTED.String()
		err = sm.Controller().DistributedServiceCard().Update(&nobj.DistributedServiceCard)
		Assert(t, err == nil, "Failed to admit DistributedServiceCard")
	}

	nobj, err = sm.Controller().Orchestrator().Find(&orchMeta)
	Assert(t, err == nil, "did not find the Orchestrator object")
	Assert(t, len(nobj.Orchestrator.Status.IncompatibleDSCs) == len(dscList), fmt.Sprintf("Expected %d in incompatible list, found %d", len(dscList), len(nobj.Orchestrator.Status.IncompatibleDSCs)))

	// Change Profile to be FLOWAWARE_FIREWALL
	dscProfile.Spec.FeatureSet = cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String()
	err = sm.ctrler.DSCProfile().Update(&dscProfile)
	AssertOk(t, err, fmt.Sprintf("DSCProfile update should have worked, but failed with %v", err))

	AssertEventually(t, func() (bool, interface{}) {
		nobj, err = sm.Controller().Orchestrator().Find(&orchMeta)
		if err != nil {
			return false, err
		}

		if len(nobj.Orchestrator.Status.IncompatibleDSCs) != 0 {
			return false, fmt.Errorf("Expected 0 in incompatible list, found %d", len(nobj.Orchestrator.Status.IncompatibleDSCs))
		}

		return true, nil
	}, "1s", "10s")

	// Change Profile to be FLOWAWARE
	dscProfile.Spec.FeatureSet = cluster.DSCProfileSpec_FLOWAWARE.String()
	err = sm.ctrler.DSCProfile().Update(&dscProfile)
	AssertOk(t, err, fmt.Sprintf("DSCProfile update should have worked, but failed with %v", err))

	AssertEventually(t, func() (bool, interface{}) {
		nobj, err = sm.Controller().Orchestrator().Find(&orchMeta)
		if err != nil {
			return false, err
		}

		if len(nobj.Orchestrator.Status.IncompatibleDSCs) != len(dscList) {
			return false, fmt.Errorf("Expected %v in incompatible list, found %d", len(dscList), len(nobj.Orchestrator.Status.IncompatibleDSCs))
		}

		return true, nil
	}, "1s", "10s")

	// Change Profile to be FLOWAWARE_FIREWALL
	dscProfile.Spec.FeatureSet = cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String()
	err = sm.ctrler.DSCProfile().Update(&dscProfile)
	AssertOk(t, err, fmt.Sprintf("DSCProfile update should have worked, but failed with %v", err))

	AssertEventually(t, func() (bool, interface{}) {
		nobj, err = sm.Controller().Orchestrator().Find(&orchMeta)
		if err != nil {
			return false, err
		}

		if len(nobj.Orchestrator.Status.IncompatibleDSCs) != 0 {
			return false, fmt.Errorf("Expected 0 in incompatible list, found %d", len(nobj.Orchestrator.Status.IncompatibleDSCs))
		}

		return true, nil
	}, "1s", "10s")

	dscProfile2 := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "flowaware",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: cluster.DSCProfileSpec_VIRTUALIZED.String(),
			FeatureSet:       cluster.DSCProfileSpec_FLOWAWARE.String(),
		},
	}

	// create DSC Profile
	sm.ctrler.DSCProfile().Create(&dscProfile2)
	AssertEventually(t, func() (bool, interface{}) {

		_, err := sm.FindDSCProfile("", "flowaware")
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Profile not found", "1ms", "1s")

	meta := api.ObjectMeta{
		Name:      dscList[0],
		Namespace: "default",
	}

	dscObj, err := sm.Controller().DistributedServiceCard().Find(&meta)
	Assert(t, err == nil, "did not find the DistributedServiceCard")

	dscObj.DistributedServiceCard.Spec.DSCProfile = "flowaware"
	err = sm.Controller().DistributedServiceCard().Update(&dscObj.DistributedServiceCard)
	Assert(t, err == nil, "Failed to update DistributedServiceCard")

	AssertEventually(t, func() (bool, interface{}) {
		nobj, err = sm.Controller().Orchestrator().Find(&orchMeta)
		if err != nil {
			return false, err
		}

		if len(nobj.Orchestrator.Status.IncompatibleDSCs) != 1 && nobj.Orchestrator.Status.IncompatibleDSCs[0] != dscList[0] {
			return false, fmt.Errorf("Expected 1 in incompatible list, found %d. Expected DSC %v found %v", len(nobj.Orchestrator.Status.IncompatibleDSCs), dscList[0], nobj.Orchestrator.Status.IncompatibleDSCs[0])
		}
		return true, nil
	}, "1s", "10s")
}

func TestIncompat2(t *testing.T) {
	sm, _, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	orchName := "prod"
	namespaceVal := "dev"

	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "default",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: cluster.DSCProfileSpec_VIRTUALIZED.String(),
			FeatureSet:       cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String(),
		},
	}

	// create DSC Profile
	sm.ctrler.DSCProfile().Create(&dscProfile)
	AssertEventually(t, func() (bool, interface{}) {

		_, err := sm.FindDSCProfile("", "default")
		if err == nil {
			return true, nil
		}
		fmt.Printf("Error find ten %v\n", err)
		return false, nil
	}, "Profile not found", "1ms", "1s")

	dscList := []string{"00ae.cd00.1234", "00ae.cd00.5678", "00ae.cd00.abcd"}
	hostList := []string{"test-host-1", "test-host-2", "test-host-3"}

	prodMap := make(map[string]string)
	prodMap[utils.OrchNameKey] = orchName
	prodMap[utils.NamespaceKey] = namespaceVal

	for i, hostName := range hostList {
		err := createHost(sm, "default", hostName, dscList[i], prodMap)
		Assert(t, (err == nil), "Host could not be created")
	}

	for i, dscMac := range dscList {
		err = createDistributedServiceCard(sm, dscMac, dscMac, hostList[i], prodMap)
		Assert(t, (err == nil), "DistributedServiceCard could not be created")
	}

	for _, dscMac := range dscList {
		orchMeta := api.ObjectMeta{
			Name: dscMac,
		}

		nobj, err := sm.Controller().DistributedServiceCard().Find(&orchMeta)
		Assert(t, err == nil, "did not find the DistributedServiceCard")

		nobj.DistributedServiceCard.Status.AdmissionPhase = cluster.DistributedServiceCardStatus_ADMITTED.String()
		err = sm.Controller().DistributedServiceCard().Update(&nobj.DistributedServiceCard)
		Assert(t, err == nil, "Failed to admit DistributedServiceCard")

		AssertEventually(t, func() (bool, interface{}) {
			nobj, err := sm.Controller().DistributedServiceCard().Find(&orchMeta)
			if err != nil || nobj == nil {
				return false, fmt.Errorf("Failed to find DSC")
			}

			if nobj.DistributedServiceCard.Status.AdmissionPhase != cluster.DistributedServiceCardStatus_ADMITTED.String() {
				return false, fmt.Errorf("Failed to admit DSC %v", orchMeta)
			}

			return true, nil
		}, "DSC not admitted")
	}

	np := orchestration.Orchestrator{
		TypeMeta: api.TypeMeta{Kind: "Orchestrator"},
		ObjectMeta: api.ObjectMeta{
			Name:      orchName,
			Namespace: "default",
			Tenant:    "default",
		},
		Spec: orchestration.OrchestratorSpec{
			Namespaces: []*orchestration.NamespaceSpec{
				&orchestration.NamespaceSpec{
					Name: namespaceVal,
					Mode: orchestration.NamespaceSpec_Managed.String(),
				},
			},
		},
		Status: orchestration.OrchestratorStatus{
			IncompatibleDSCs: dscList,
		},
	}

	err = sm.ctrler.Orchestrator().Create(&np)
	AssertOk(t, err, "failed to create orchestrator")

	AssertEventually(t, func() (bool, interface{}) {
		obj, err := sm.FindObject("Orchestrator", "", "", orchName)
		if err != nil {
			return false, err
		}

		nobj, err := OrchestratorStateFromObj(obj)
		if err != nil {
			return false, err
		}

		incompatCount := 0
		for _, v := range nobj.incompatibleDscs {
			if v {
				incompatCount = incompatCount + 1
			}
		}

		if incompatCount != 0 {
			return false, fmt.Errorf("Expected 0 in incompatible list, found %d", incompatCount)
		}

		return true, nil
	}, "1s", "10s")
}

func TestIncompatHostProfileChecks(t *testing.T) {
	type runSetup struct {
		DeployentTarget      string
		FeatureSet           string
		ManagementMode       string
		ExpectedIncompatible int
	}

	testCases := []runSetup{
		{
			ManagementMode:       orchestration.NamespaceSpec_Managed.String(),
			ExpectedIncompatible: 1,
		},
		{
			ManagementMode:       orchestration.NamespaceSpec_Monitored.String(),
			ExpectedIncompatible: 0,
		},
		{
			ManagementMode:       orchestration.NamespaceSpec_Managed.String(),
			ExpectedIncompatible: 1,
		},
		{
			ManagementMode:       orchestration.NamespaceSpec_Monitored.String(),
			ExpectedIncompatible: 0,
		},
		{
			DeployentTarget:      cluster.DSCProfileSpec_VIRTUALIZED.String(),
			FeatureSet:           cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String(),
			ExpectedIncompatible: 1,
		},
		{
			ManagementMode:       orchestration.NamespaceSpec_Managed.String(),
			ExpectedIncompatible: 0,
		},
		{
			ManagementMode:       orchestration.NamespaceSpec_Monitored.String(),
			ExpectedIncompatible: 1,
		},
		{
			DeployentTarget:      cluster.DSCProfileSpec_HOST.String(),
			FeatureSet:           cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String(),
			ExpectedIncompatible: 0,
		},
		{
			DeployentTarget:      cluster.DSCProfileSpec_HOST.String(),
			FeatureSet:           cluster.DSCProfileSpec_FLOWAWARE.String(),
			ExpectedIncompatible: 0,
		},
		{
			DeployentTarget:      cluster.DSCProfileSpec_VIRTUALIZED.String(),
			FeatureSet:           cluster.DSCProfileSpec_FLOWAWARE.String(),
			ExpectedIncompatible: 1,
		},
		{
			ManagementMode:       orchestration.NamespaceSpec_Managed.String(),
			ExpectedIncompatible: 1,
		},
		{
			DeployentTarget:      cluster.DSCProfileSpec_VIRTUALIZED.String(),
			FeatureSet:           cluster.DSCProfileSpec_FLOWAWARE_FIREWALL.String(),
			ExpectedIncompatible: 0,
		},
	}

	profileName := "test-profile"
	hostName := "testHost"
	dscMac := "00ae.cd00.0101"
	orchName := "prod"
	namespaceVal := "dev"

	sm, _, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}

	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      profileName,
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: cluster.DSCProfileSpec_HOST.String(),
			FeatureSet:       cluster.DSCProfileSpec_SMARTNIC.String(),
		},
	}

	// create DSC Profile
	sm.ctrler.DSCProfile().Create(&dscProfile)
	AssertEventually(t, func() (bool, interface{}) {

		_, err := sm.FindDSCProfile("", profileName)
		if err == nil {
			return true, nil
		}
		return false, nil
	}, "Profile not found", "1ms", "1s")

	prodMap := make(map[string]string)
	prodMap[utils.OrchNameKey] = orchName
	prodMap[utils.NamespaceKey] = namespaceVal

	dsc := cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{Kind: "DistributedServiceCard"},
		ObjectMeta: api.ObjectMeta{
			Name:   dscMac,
			Labels: map[string]string{},
		},
		Spec: cluster.DistributedServiceCardSpec{DSCProfile: profileName, ID: dscMac},
		Status: cluster.DistributedServiceCardStatus{PrimaryMAC: dscMac,
			NumMacAddress:  24,
			AdmissionPhase: cluster.DistributedServiceCardStatus_ADMITTED.String(),
			Host:           hostName,
		},
	}

	// create a DistributedServiceCard
	sm.ctrler.DistributedServiceCard().Create(&dsc)

	np := orchestration.Orchestrator{
		TypeMeta: api.TypeMeta{Kind: "Orchestrator"},
		ObjectMeta: api.ObjectMeta{
			Name:      orchName,
			Namespace: "",
			Tenant:    "",
		},
		Spec: orchestration.OrchestratorSpec{
			Namespaces: []*orchestration.NamespaceSpec{
				&orchestration.NamespaceSpec{
					Name: namespaceVal,
					Mode: orchestration.NamespaceSpec_Monitored.String(),
				},
			},
		},
		Status: orchestration.OrchestratorStatus{
			IncompatibleDSCs: []string{},
		},
	}

	err = sm.ctrler.Orchestrator().Create(&np)
	AssertOk(t, err, "failed to create orchestrator")

	err = createHost(sm, "default", hostName, dscMac, prodMap)
	Assert(t, (err == nil), "Host could not be created")

	for _, tc := range testCases {
		log.Infof("Running testcase : %v", tc)
		if len(tc.ManagementMode) > 0 {

			np.Spec.Namespaces = []*orchestration.NamespaceSpec{
				&orchestration.NamespaceSpec{
					Name: namespaceVal,
					Mode: tc.ManagementMode,
				},
			}

			err = sm.ctrler.Orchestrator().Update(&np)
			AssertOk(t, err, "failed to update orchestrator")
		}

		if len(tc.DeployentTarget) > 0 {
			dscProfile.Spec.DeploymentTarget = tc.DeployentTarget
			dscProfile.Spec.FeatureSet = tc.FeatureSet

			err = sm.ctrler.DSCProfile().Update(&dscProfile)
			AssertOk(t, err, "failed to update dscprofile")
		}

		AssertEventually(t, func() (bool, interface{}) {
			obj, err := sm.FindObject("Orchestrator", "", "", orchName)
			if err != nil {
				log.Infof("Returning - Err : %v", err)
				return false, err
			}

			nobj, err := OrchestratorStateFromObj(obj)
			if err != nil {
				log.Infof("Returning - Err : %v", err)
				return false, err
			}

			log.Infof("Got orchestrator object %v", nobj)
			isIncompatible, ok := nobj.incompatibleDscs[dscMac]
			if !ok {
				return false, fmt.Errorf("did not find the DSC %v", dscMac)
			}

			if tc.ExpectedIncompatible > 0 && !isIncompatible {
				return false, fmt.Errorf("expected DSC %v to be incompatible", dscMac)
			}

			if tc.ExpectedIncompatible == 0 && isIncompatible {
				return false, fmt.Errorf("expected DSC %v to be compatible", dscMac)
			}

			return true, nil
		}, "1s", "10s")
	}
}

func TestStateMgr(t *testing.T) {
	sm, _, err := NewMockStateManager()
	if err != nil {
		t.Fatalf("Failed creating state manager. Err : %v", err)
		return
	}
	sm.startWatchers()

	sm.StopWatchersOnRestore()
	sm.ctrler.Network().StopWatch(sm)

	sm.RestartWatchersOnRestore()
	sm.ctrler.Network().StopWatch(sm)
}

func TestMain(m *testing.M) {
	tsdb.Init(context.Background(), &tsdb.Opts{})

	config := log.GetDefaultConfig("orchhub-statemgr-test")
	config.LogToStdout = true
	config.Filter = log.AllowAllFilter
	logger = log.SetConfig(config)

	os.Exit(m.Run())
}
