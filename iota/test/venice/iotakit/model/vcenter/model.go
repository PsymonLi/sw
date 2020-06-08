package vcenter

import (
	"context"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	iota "github.com/pensando/sw/iota/protos/gogen"
	cfgModel "github.com/pensando/sw/iota/test/venice/iotakit/cfg/enterprise"
	"github.com/pensando/sw/iota/test/venice/iotakit/cfg/enterprise/base"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/common"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/enterprise"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/iota/test/venice/iotakit/testbed"
	"github.com/pensando/sw/venice/utils/log"
)

var (
	vcenterSimBinary         = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/bin/orch-sim", os.Getenv("GOPATH"))
	vcenterSimRunnerBinary   = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/bin/orchrunner", os.Getenv("GOPATH"))
	cfgFile                  = "/tmp/scale-cfg.json"
	orchSimIntances          = 10
	orchSimInstanceStartPort = 20000
)

//VcenterSysModel representing model for vcenter
type VcenterSysModel struct {
	enterprise.SysModel
	orchestrator *objects.OrchestratorCollection
	vcenterSim   *objects.CommandNode
}

//GetOrchestrator returns orchestroa
func (sm *VcenterSysModel) getOrchestrators(scale bool) (*objects.OrchestratorCollection, error) {
	port := ""
	if scale {
		port = fmt.Sprintf("%v", orchSimInstanceStartPort)
		name := fmt.Sprintf("vcsim-%s-dc", port)
		collection := objects.NewOrchestrator(sm.ObjClient(),
			name,
			name,
			sm.vcenterSim.GetIotaNode().GetIpAddress(),
			port,
			"user",
			"password",
		)
		for i := 0; i < orchSimIntances; i++ {
			port = fmt.Sprintf("%v", orchSimInstanceStartPort+i)
			name := fmt.Sprintf("vcsim-%s-dc", port)
			newCollection := objects.NewOrchestrator(sm.ObjClient(),
				name,
				name,
				sm.vcenterSim.GetIotaNode().GetIpAddress(),
				port,
				"user",
				"password",
			)
			collection.Merge(newCollection)

		}
		return collection, nil
	}
	for _, node := range sm.Tb.Nodes {
		if node.Personality == iota.PersonalityType_PERSONALITY_VCENTER_NODE {
			return objects.NewOrchestrator(sm.ObjClient(),
				sm.Tb.GetDC(),
				node.NodeName,
				node.NodeMgmtIP,
				port,
				sm.Tb.Params.Provision.Vars["VcenterUsername"],
				sm.Tb.Params.Provision.Vars["VcenterPassword"],
			), nil
		}
	}

	return nil, errors.New("Vcenter orchestrator not found")
}

func (sm *VcenterSysModel) GetOrchestrator() (*objects.OrchestratorCollection, error) {
	return sm.orchestrator, nil
}

func (sm *VcenterSysModel) getOrchRuleCookie() string {
	//For later deletion
	return "iota-orch-disable"
}

//DisconnectVeniceAndOrchestrator disonnect vcenter and venice
func (sm *VcenterSysModel) DisconnectVeniceAndOrchestrator() error {
	trig := sm.Tb.NewTrigger()

	//For later deletion
	cookie := sm.getOrchRuleCookie()

	orchs, _ := sm.GetOrchestrator()

	for _, venice := range sm.VeniceNodeMap {
		for _, orch := range orchs.Orchestrators {
			cmd := fmt.Sprintf("sudo iptables -A INPUT -s %v -j DROP  -m comment --comment %s",
				strings.Split(orch.IP, "/")[0], cookie)
			trig.AddCommand(cmd, venice.Name()+"_venice", venice.Name())
			trig.AddCommandWithRetriesOnFailures(cmd, venice.Name()+"_venice",
				venice.Name(), 3)
		}
	}

	// run the trigger
	resp, err := trig.Run()
	if err != nil {
		log.Errorf("Error running disonnect vcenter command on venice. Err: %v", resp)
		return fmt.Errorf("Error running disonnect  naples command on venice. Err: %v", resp)
	}

	for _, cmd := range resp {
		if cmd.ExitCode != 0 {
			log.Errorf("Error running deny on venice for vcenter . Err: %v", cmd)
			return fmt.Errorf("Error running deny on venice vcenter . Err: %v", cmd)
		}
	}

	return nil
}

//AllowVeniceAndOrchestrator allows venice to be connected from venice
func (sm *VcenterSysModel) AllowVeniceAndOrchestrator() error {
	trig := sm.Tb.NewTrigger()
	//For later deletion
	cookie := sm.getOrchRuleCookie()

	for _, venice := range sm.VeniceNodeMap {
		cmd := fmt.Sprintf("sudo iptables -L INPUT --line-numbers | grep  -w %v | grep -i drop | awk '{print $1}' | sort -n -r | xargs -n 1 sudo iptables -D INPUT $1",
			cookie)
		trig.AddCommandWithRetriesOnFailures(cmd, venice.Name()+"_venice",
			venice.Name(), 3)
	}

	// run the trigger should serial as we are modifying ipables.
	resp, err := trig.Run()
	if err != nil {
		log.Errorf("Error running disonnect command on venice. Err: %v", resp)
		//Ignore error as the rule may not be there.
	}

	for _, cmd := range resp {
		if cmd.ExitCode != 0 {
			log.Errorf("Error running allow on venice for naples . Err: %v", cmd)
			return fmt.Errorf("Error running allow on venice naples . Err: %v", cmd)
		}
	}

	return nil
}

//deleteAllWorkloads deletes all workloads
func (sm *VcenterSysModel) deleteAllWorkloads() error {

	// first get a list of all existing.Workloads from iota
	gwlm := &iota.WorkloadMsg{
		ApiResponse: &iota.IotaAPIResponse{},
		WorkloadOp:  iota.Op_GET,
	}
	topoClient := iota.NewTopologyApiClient(sm.Tb.Client().Client)
	getResp, err := topoClient.GetWorkloads(context.Background(), gwlm)
	log.Debugf("Got get workload resp: %+v, err: %v", getResp, err)
	if err != nil {
		log.Errorf("Failed to instantiate Apps. Err: %v", err)
		return fmt.Errorf("Error creating IOTA workload. err: %v", err)
	} else if getResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		log.Errorf("Failed to instantiate Apps. resp: %+v.", getResp.ApiResponse)
		return fmt.Errorf("Error creating IOTA workload. Resp: %+v", getResp.ApiResponse)
	}

	if len(getResp.Workloads) != 0 {
		getResp.WorkloadOp = iota.Op_DELETE
		delResp, err := topoClient.DeleteWorkloads(context.Background(), getResp)
		log.Debugf("Got get workload delete resp: %+v, err: %v", delResp, err)
		if err != nil {
			log.Errorf("Failed to delete old workloads. Err: %v", err)
			return fmt.Errorf("Error deleting IOTA workload. err: %v", err)
		}

	}

	return nil
}

//SetupWorkloads bring up.Workloads on host
func (sm *VcenterSysModel) SetupWorkloads(scale bool) error {

	log.Infof("Skipping setup of workloads on vcenter model")
	return nil
}

func (sm *VcenterSysModel) Init(tb *testbed.TestBed, cfgType cfgModel.CfgType, skipSetup bool) error {
	err := sm.SysModel.Init(tb, cfgType, skipSetup)
	if err != nil {
		return err
	}

	sm.RunVerifySystemHealth = sm.VerifySystemHealth
	return nil

}

func (sm *VcenterSysModel) setupInsertionMode() error {

	cfgClient := sm.ConfigClient()
	dscProfile := cluster.DSCProfile{
		TypeMeta: api.TypeMeta{Kind: "DSCProfile"},
		ObjectMeta: api.ObjectMeta{
			Name:      "insertion.enforced",
			Namespace: "",
			Tenant:    "",
		},
		Spec: cluster.DSCProfileSpec{
			DeploymentTarget: "VIRTUALIZED",
			FeatureSet:       "FLOWAWARE_FIREWALL",
		},
	}
	cfgClient.CreateDscProfile(&dscProfile)
	dscObject, err := cfgClient.ListSmartNIC()

	if err != nil {
		return err
	}

	for _, dsc := range dscObject {
		dsc.Spec.DSCProfile = "insertion.enforced"
		cfgClient.UpdateSmartNIC(dsc)
	}

	return nil
}

func (sm *VcenterSysModel) setupVmotionNetwork() error {

	if sm.vcenterSim != nil {
		return nil
	}

	nc := sm.Networks("")
	if len(nc.Subnets()) < 1 {
		return fmt.Errorf("Insufficient networks to enable vmotion")
	}

	vmotionNet := nc.Subnets()[0].Name

	log.Infof("Create vmk interface for vmotion using network %s on hosts %v", vmotionNet,
		sm.Hosts().Names())
	networkSpec := common.NetworkSpec{
		Name:   vmotionNet,
		Switch: "",
		Nodes:  sm.Hosts().Names(),
		NwType: common.VmotionNetworkType,
	}

	return sm.AddNetworks(networkSpec)
}

// SetupDefaultConfig sets up a default config for the system
func (sm *VcenterSysModel) SetupDefaultConfig(ctx context.Context, scale, scaleData bool) error {
	sm.Scale = scale
	sm.ScaleData = scaleData

	err := sm.setupInsertionMode()
	if err != nil {
		return err
	}

	err = sm.InitConfig(scale, scaleData)
	if err != nil {
		return err
	}

	bkCtx, cancelFunc := context.WithTimeout(context.Background(), 10*time.Minute)
	defer cancelFunc()
	//Give it some time for hosts to be discovered.
L:
	for true {
		select {
		case <-bkCtx.Done():
			return fmt.Errorf("Error associating hosts: %s", err)
		default:
			err = sm.AssociateHosts()
			if err == nil && len(sm.NaplesHosts)+len(sm.FakeHosts) == len(sm.NaplesNodes)+len(sm.FakeNaples) {
				break L
			}
			log.Errorf("Error associating hosts: %s", err)
			time.Sleep(2 * time.Second)
		}
	}

	if err := sm.setupVmotionNetwork(); err != nil {
		return fmt.Errorf("Setting up vmotion network %v", err.Error())
	}

	//objects.NewOrchestrator(
	if err := sm.SetupDefaultCommon(ctx, scale, scaleData); err != nil {
		return err
	}

	//objects.NewOrchestrator(
	if err := sm.SetupDefaultCommon(ctx, scale, scaleData); err != nil {
		return err
	}

	//Reassociate workloads to hosts
	hc := sm.Hosts()
	wloads, err := sm.ListWorkload()
	if err != nil {
		return err
	}

	newHostMap := make(map[string]string)
	hostUsedMap := make(map[string]bool)
	for _, wl := range wloads {
		if newHost, ok := newHostMap[wl.Spec.HostName]; ok {
			wl.Spec.HostName = newHost
			continue
		}
		for _, host := range hc.Hosts {
			if _, ok := hostUsedMap[host.VeniceHost.Name]; !ok {
				hostUsedMap[host.VeniceHost.Name] = true
				newHostMap[wl.Spec.HostName] = host.VeniceHost.Name
				wl.Spec.HostName = host.VeniceHost.Name
				break
			}
		}
	}

	return sm.SetupWorkloads(scale)
}

// deleteWorkload deletes a workload
func (sm *VcenterSysModel) deleteWorkload(wr *objects.Workload) error {
	// FIXME: free mac addr for the workload
	// FIXME: free useg vlan
	// FIXME: free ip address for the workload

	// delete venice workload not need in venice
	//sm.CfgModel.DeleteWorkloads([]*workload.Workload{wr.VeniceWorkload})
	//Ignore the error as we don't care

	wrkLd := &iota.WorkloadMsg{
		ApiResponse: &iota.IotaAPIResponse{},
		WorkloadOp:  iota.Op_DELETE,
		Workloads:   []*iota.Workload{wr.GetIotaWorkload()},
	}

	topoClient := iota.NewTopologyApiClient(sm.Tb.Client().Client)
	appResp, err := topoClient.DeleteWorkloads(context.Background(), wrkLd)

	if err != nil || appResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		log.Errorf("Failed to instantiate Apps. %v/%v", err, appResp.ApiResponse.ApiStatus)
		return fmt.Errorf("Error deleting IOTA workload. Resp: %+v, err: %v", appResp.ApiResponse, err)
	}

	delete(sm.WorkloadsObjs, wr.VeniceWorkload.Name)

	return nil
}

func (sm *VcenterSysModel) TeardownWorkloads(wc *objects.WorkloadCollection) error {
	// Teardown the workloads
	for _, wrk := range wc.Workloads {
		err := sm.deleteWorkload(wrk)
		if err != nil {
			log.Infof("Delete workload failed : %v", err.Error())
			return err
		}
	}

	log.Info("Delete workload successful")
	return nil
}

func getThirdPartyNic(name, mac string) *cluster.DistributedServiceCard {

	return &cluster.DistributedServiceCard{
		TypeMeta: api.TypeMeta{
			Kind: "DistributedServiceCard",
		},
		ObjectMeta: api.ObjectMeta{
			Name: name,
		},
		Spec: cluster.DistributedServiceCardSpec{
			ID: "host-1",
			IPConfig: &cluster.IPConfig{
				IPAddress: "1.2.3.4/32",
			},
			MgmtMode:    "NETWORK",
			NetworkMode: "OOB",
		},
		Status: cluster.DistributedServiceCardStatus{
			AdmissionPhase: "ADMITTED",
			PrimaryMAC:     mac,
			IPConfig: &cluster.IPConfig{
				IPAddress: "1.2.3.4",
			},
		},
	}
}

func (sm *VcenterSysModel) modifyConfig() error {

	cfgObjects := sm.GetCfgObjects()

	orch, err := sm.GetOrchestrator()
	if err != nil {
		log.Errorf("Get orchestrator failed %v", err)
		return err
	}
	//Modify netwrork object with scope
	for _, nw := range cfgObjects.Networks {
		nw.Spec.Orchestrators = []*network.OrchestratorInfo{}
		for _, orch := range orch.Orchestrators {
			nw.Spec.Orchestrators = append(nw.Spec.Orchestrators, &network.OrchestratorInfo{
				Name:      orch.Name,
				Namespace: orch.DC,
			})
		}

		for i := range cfgObjects.Workloads {
			for j := range cfgObjects.Workloads[i].Spec.Interfaces {
				if cfgObjects.Workloads[i].Spec.Interfaces[j].ExternalVlan == nw.Spec.VlanID {
					cfgObjects.Workloads[i].Spec.Interfaces[j].Network = getVcenterNetworkName(nw)
				}
			}
		}
	}

	if sm.vcenterSim == nil {
		//Set mac address to be empty as venter and orch will figure out for themselves
		for _, wl := range cfgObjects.Workloads {
			for index := range wl.Spec.Interfaces {
				wl.Spec.Interfaces[index].MACAddress = ""
			}
		}
	}

	return nil
}

func (sm *VcenterSysModel) restartSim() error {

	trig := sm.Tb.NewTrigger()
	trig.AddCommand(fmt.Sprintf("pkill -9 %v", filepath.Base(vcenterSimBinary)), "", sm.vcenterSim.GetIotaNode().Name)
	trig.AddCommand(fmt.Sprintf("pkill -9 %v", filepath.Base(vcenterSimRunnerBinary)), "", sm.vcenterSim.GetIotaNode().Name)
	_, err := trig.Run()
	if err != nil {
		return fmt.Errorf("Error running command %v", err.Error())
	}
	//Copy binary as well as config files
	err = sm.Tb.CopyToNode(sm.vcenterSim.GetIotaNode().Name, []string{vcenterSimBinary, vcenterSimRunnerBinary, cfgFile}, "")
	if err != nil {
		log.Errorf("Error copying vcenter scale sim  %v", err.Error())
		return err
	}

	trig = sm.Tb.NewTrigger()
	trig.AddBackgroundCommand(fmt.Sprintf("./%v -c %v -o ./%v -n %v -p %v", filepath.Base(vcenterSimRunnerBinary), filepath.Base(cfgFile), filepath.Base(vcenterSimBinary), orchSimIntances, orchSimInstanceStartPort), "", sm.vcenterSim.GetIotaNode().Name)
	_, err = trig.Run()
	if err != nil {
		return fmt.Errorf("Error running command %v", err.Error())
	}

	return nil
}

func TimeTrack(start time.Time, name string) time.Duration {
	elapsed := time.Since(start)
	log.Infof("%s took %s\n", name, elapsed)
	return elapsed
}

// InitConfig sets up a default config for the system
func (sm *VcenterSysModel) InitConfig(scale, scaleData bool) error {
	skipSetup := os.Getenv("SKIP_SETUP")
	skipConfig := os.Getenv("SKIP_CONFIG")
	cfgParams := &base.ConfigParams{
		Scale:                         scale,
		Regenerate:                    skipSetup == "",
		Vlans:                         sm.Tb.AllocatedVlans(),
		NumberOfInterfacesPerWorkload: 2,
	}
	for _, naples := range sm.NaplesNodes {
		dscs := []*cluster.DistributedServiceCard{}
		for _, inst := range naples.Instances {
			dscs = append(dscs, inst.Dsc)
		}
		cfgParams.Dscs = append(cfgParams.Dscs, dscs)

	}

	if scale && len(sm.CommandNodes) > 0 {
		for _, cn := range sm.CommandNodes {
			sm.vcenterSim = cn
			break
		}
	}

	index := 0
	for name, node := range sm.ThirdPartyNodes {
		node.UUID = "50df.9ac7.c24" + fmt.Sprintf("%v", index)
		n := getThirdPartyNic(name, node.UUID)
		cfgParams.Dscs = append(cfgParams.Dscs, []*cluster.DistributedServiceCard{n})
		index++
	}

	cfgParams.NaplesLoopBackIPs = make(map[string]string)
	for _, naples := range sm.FakeNaples {
		//cfgParams.Dscs = append(cfgParams.Dscs, naples.SmartNic)
		cfgParams.FakeDscs = append(cfgParams.FakeDscs, naples.Instances[0].Dsc)
		//node uuid already in format
		cfgParams.NaplesLoopBackIPs[naples.UUID] = naples.IP()
		naples.Instances[0].LoopbackIP = naples.IP()
	}

	err := sm.PopulateConfig(cfgParams)
	if err != nil {
		return err
	}

	if skipConfig == "" {

		orch, err := sm.getOrchestrators(scale)
		if err != nil {
			return err
		}
		sm.orchestrator = orch

		err = sm.DisconnectVeniceAndOrchestrator()
		if err != nil {
			return err
		}
		defer sm.AllowVeniceAndOrchestrator()

		time.Sleep(2 * time.Second)

		err = sm.CleanupAllConfig()
		if err != nil {
			return err
		}

		err = orch.Delete()
		if err != nil {
			//Ignore as it may not be present
			log.Infof("Error deleting orch config %v", err)
		}

		err = sm.deleteAllWorkloads()
		if err != nil {
			log.Infof("Error Removing existing workloads %v", err)
			return err
		}

		//Clean up networks too
		err = sm.RemoveNetworks(sm.Tb.GetSwitch())
		if err != nil {
			log.Infof("Error removing networks from switch %v", err)
			return err
		}

		if scale && sm.vcenterSim != nil {
			if err := sm.restartSim(); err != nil {
				return err
			}
		}

		time.Sleep(120 * time.Second)

		err = orch.Commit()
		if err != nil {
			log.Errorf("Error commiting orchestration config %v", err)
			return err
		}

		err = sm.modifyConfig()
		if err != nil {
			return err
		}

		sm.AllowVeniceAndOrchestrator()

		bkCtx, cancelFunc := context.WithTimeout(context.Background(), 2*time.Minute)
		defer cancelFunc()
	L:
		for true {
			select {
			case <-bkCtx.Done():
				return fmt.Errorf("Error Connecting vcenter %s", err)
			default:

				if connected, err := orch.Connected(); err == nil && connected {
					break L
				}
				log.Infof("Orchestrator not connected to vcenter error : %v", err)
				time.Sleep(2 * time.Second)
			}
		}

		defer TimeTrack(time.Now(), "Config Genenration and Propogation")
		err = sm.PushConfig()
		if err != nil {
			return err
		}

		if scale {
			bkCtx, cancelFunc := context.WithTimeout(context.Background(), 20*time.Minute)
			defer cancelFunc()
			for true {
				select {
				case <-bkCtx.Done():
					return fmt.Errorf("Config push timed out %s", err)
				default:
					ok, err := sm.IsConfigPushComplete()
					if err == nil && ok {
						return nil
					}
					time.Sleep(10 * time.Second)
				}
			}
		}

	}

	return nil
}

func getVcenterNetworkName(n *network.Network) string {
	return "#Pen-PG-" + n.Name
}

// Networks returns a list of subnets
func (sm *VcenterSysModel) Networks(tenant string) *objects.NetworkCollection {
	snc := objects.NetworkCollection{}
	nws, err := sm.CfgModel.ListNetwork(tenant)
	if err != nil {
		log.Errorf("Error listing networks %v", err)
		return nil
	}
	for _, sn := range nws {
		snc.AddSubnet(&objects.Network{Name: getVcenterNetworkName(sn), VeniceNetwork: sn})
	}

	return &snc
}
