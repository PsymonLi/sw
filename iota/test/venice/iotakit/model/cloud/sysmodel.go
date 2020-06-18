package cloud

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"os"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/api/generated/network"
	"github.com/pensando/sw/api/generated/security"
	iota "github.com/pensando/sw/iota/protos/gogen"
	"github.com/pensando/sw/iota/test/venice/iotakit/cfg/enterprise"
	"github.com/pensando/sw/iota/test/venice/iotakit/cfg/enterprise/base"
	baseModel "github.com/pensando/sw/iota/test/venice/iotakit/model/base"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/iota/test/venice/iotakit/testbed"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
)

//Orchestrator Return orchestrator

const (
	hostToolsDir       = "/pensando/iota"
	penctlPath         = "."
	penctlLinuxBinary  = "penctl.linux"
	penctlPkgName      = "bin/penctl/" + penctlLinuxBinary
	agentAuthTokenFile = "/tmp/auth_token"
)

// SysModel represents a objects.of the system under test
type SysModel struct {
	baseModel.SysModel
	sgpolicies map[string]*objects.NetworkSecurityPolicy // security policies
}

//Init init the testbed
func (sm *SysModel) Init(tb *testbed.TestBed, cfgType enterprise.CfgType, skipSetup bool) error {

	err := sm.SysModel.Init(tb, cfgType, skipSetup)
	if err != nil {
		return err
	}

	sm.RandomTrigger = sm.RunRandomTrigger
	sm.RunVerifySystemHealth = sm.VerifySystemHealth

	sm.AutoDiscovery = true
	if os.Getenv("NO_AUTO_DISCOVERY") != "" {
		sm.AutoDiscovery = false
	}

	sm.NoModeSwitchReboot = true
	sm.NoSetupDataPathAfterSwitch = true

	//Setup license for overlay routing
	sm.Licenses = []string{"OverlayRouting", "SubnetSecurityPolicies"}

	sm.sgpolicies = make(map[string]*objects.NetworkSecurityPolicy)
	sm.CfgModel = enterprise.NewCfgModel(cfgType)
	if sm.CfgModel == nil {
		return errors.New("could not initialize config objects")
	}

	err = sm.SetupVeniceNaples()
	if err != nil {
		return err
	}

	//Venice is up so init config model
	err = sm.InitCfgModel()
	if err != nil {
		return err
	}

	return sm.SetupNodes()
}

//SetupVeniceNaples setups venice and naples
func (sm *SysModel) SetupVeniceNaples() error {
	ctx, cancel := context.WithTimeout(context.TODO(), 30*time.Minute)
	defer cancel()

	// build venice nodes
	for _, nr := range sm.Tb.Nodes {
		if nr.Personality == iota.PersonalityType_PERSONALITY_VENICE {
			// create
			sm.VeniceNodeMap[nr.NodeName] = objects.NewVeniceNode(nr)
		}
	}

	//First Read Naples
	// make cluster & setup auth
	err := sm.SetupConfig(ctx)
	if err != nil {
		sm.Tb.CollectLogs()
		return err
	}

	return nil
}

// Cleanup Do cleanup
func (sm *SysModel) Cleanup() error {
	// collect all log files
	sm.CollectLogs()
	return nil
}

// AddNaplesNodes node on the fly
func (sm *SysModel) AddNaplesNodes(names []string) error {
	//First add to testbed.
	log.Infof("Adding naples nodes : %v", names)
	nodes, err := sm.Tb.AddNodes(iota.PersonalityType_PERSONALITY_NAPLES, names)
	if err != nil {
		return err
	}

	// move naples to managed mode
	err = sm.DoModeSwitchOfNaples(nodes, sm.NoModeSwitchReboot)
	if err != nil {
		log.Errorf("Setting up naples failed. Err: %v", err)
		return err
	}

	// add venice node to naples
	err = sm.SetUpNaplesPostCluster(nodes)
	if err != nil {
		log.Errorf("Setting up naples failed. Err: %v", err)
		return err
	}

	for _, node := range nodes {
		if err := sm.CreateNaples(node); err != nil {
			return err
		}

	}

	//Reassociate hosts as new naples is added now.
	if err := sm.AssociateHosts(); err != nil {
		log.Infof("Error in host association %v", err.Error())
		return err
	}

	return nil

	/*
		log.Infof("Bringing up.Workloads naples nodes : %v", names)
		wc := objects.NewWorkloadCollection(sm.ObjClient(), sm.Tb)
		for _, h := range sm.NaplesHosts {
			for _, node := range nodes {
				if node.GetIotaNode() == h.GetIotaNode() {
					hwc, err := sm.SetupWorkloadsOnHost(h)
					if err != nil {
						return err
					}
					wc.Workloads = append(wc.Workloads, hwc.Workloads...)
				}
			}
		}

		return wc.Bringup(sm.Tb)
	*/
}

//SetupWorkloadsOnHost sets up workload on host
func (sm *SysModel) SetupWorkloadsOnHost(h *objects.Host) (*objects.WorkloadCollection, error) {

	wc := objects.NewWorkloadCollection(sm.ObjClient(), sm.Tb)

	//For now cloud has just 1 DSC
	filter := fmt.Sprintf("spec.type=host-pf,status.dsc=%v", h.Naples.Instances[0].Dsc.Status.PrimaryMAC)
	hostNwIntfs, err := sm.ObjClient().ListNetowrkInterfacesByFilter(filter)
	if err != nil {
		return nil, err
	}

	hostIntfs := sm.Tb.GetHostIntfs(h.GetIotaNode().Name, "")
	numNetworks := len(sm.Tb.GetHostIntfs(h.GetIotaNode().Name, ""))

	if len(hostNwIntfs) != numNetworks {
		msg := fmt.Sprintf("Number of host Nw interfaces (%v) is not equal to actual host interfaces (%v) ",
			len(hostNwIntfs), numNetworks)
		log.Errorf(msg)
		return nil, errors.New(msg)
	}

	wloads, err := sm.ListWorkloadsOnHost(h.VeniceHost)
	if err != nil {
		log.Error("Error finding real.Workloads on hosts.")
		return nil, err
	}

	// List tenant
	tenants, err := sm.ObjClient().ListTenant()
	if err != nil {
		log.Errorf("err: %s", err)
		return nil, err
	}

	nws := []*network.Network{}
	for _, ten := range tenants {
		if ten.Name == globals.DefaultTenant {
			continue
		}

		nws, err = sm.ListNetwork(ten.Name)
		if err != nil {
			log.Error("Error finding networks")
			return nil, err
		}
		//Pick the first tenant
		break
	}

	if len(nws) < numNetworks {
		msg := fmt.Sprintf("Number of Networks (%v) is less than actual interfaces(%v) ",
			len(nws), numNetworks)
		log.Errorf(msg)
		return nil, errors.New(msg)
	}

	sort.SliceStable(hostNwIntfs, func(i, j int) bool {
		return hostNwIntfs[i].Name < hostNwIntfs[j].Name
	})

	sort.SliceStable(hostIntfs, func(i, j int) bool {
		return hostIntfs[i] < hostIntfs[j]
	})

	//Lets attach network host intefaces
	for index, nwIntf := range hostNwIntfs {
		nwIntf.Spec.AttachNetwork = nws[index].Name
		nwIntf.Spec.AttachTenant = nws[index].Tenant
		err := sm.ObjClient().UpdateNetworkInterface(nwIntf)
		if err != nil {
			log.Errorf("Error attaching network to interface")
			return nil, err
		}
		for _, wload := range wloads {
			if wload.GetSpec().Interfaces[0].Network == nws[index].Name {
				//Set to 0 so that we don't create sub-interfaces
				wload.Spec.Interfaces[0].MicroSegVlan = 0
				//For now make sure we don't change the mac address
				wload.Spec.Interfaces[0].MACAddress = ""
				os := h.Naples.GetTestNode().GetNodeOs()
				info, ok := sm.Tb.Topo.WkldInfo[os]
				if !ok {
					log.Errorf("Workload info of type %s not found", os)
					err := fmt.Errorf("Workload info of type %s not found", os)
					return nil, err
				}

				sm.WorkloadsObjs[wload.Name] = objects.NewWorkload(h, wload, info.WorkloadType, info.WorkloadImage, "", nil)
				sm.WorkloadsObjs[wload.Name].SetParentInterface(hostIntfs[index])
				//Cloud supports only single naples
				sm.WorkloadsObjs[wload.Name].SetNaplesUUID(h.VeniceHost.Spec.DSCs[0].MACAddress)
				wc.Workloads = append(wc.Workloads, sm.WorkloadsObjs[wload.Name])
				//Just 1 workload per network
				break
			}
		}
	}

	return wc, nil

}

//attachSimInterfacesToNetworks
func (sm *SysModel) attachSimInterfacesToNetworks(scale bool) error {
	hosts, err := sm.ListFakeHosts()
	if err != nil {
		return err
	}

	log.Infof("Number of fake hosts %v", len(hosts))
	// List tenant
	tenants, err := sm.ObjClient().ListTenant()
	if err != nil {
		log.Errorf("err: %s", err)
		return err
	}

	if scale {
		type nwIter struct {
			nws   []*network.Network
			index int
		}
		tenNetwowks := make(map[string]*nwIter)

		for index, ten := range tenants {
			if ten.Name == globals.DefaultTenant {
				tenants[index] = tenants[len(tenants)-1]
				tenants = tenants[:len(tenants)-1]
				break
			}
		}
		for _, ten := range tenants {
			nws, err := sm.ListNetwork(ten.Name)
			if err != nil {
				log.Errorf("Error finding networks for tenant %v", ten.Name)
				return err
			}
			tenNetwowks[ten.Name] = &nwIter{nws: nws}
		}

		tenIdx := 0
		for _, h := range hosts {
			//For now cloud has just 1 DSC
			filter := fmt.Sprintf("spec.type=host-pf,status.dsc=%v", h.Naples.Instances[0].Dsc.Status.PrimaryMAC)
			hostNwIntfs, err := sm.ObjClient().ListNetowrkInterfacesByFilter(filter)
			if err != nil {
				return err
			}

			ten := tenants[tenIdx%len(tenants)]
			//Lets attach network host intefaces
			for _, nwIntf := range hostNwIntfs {
				iter := tenNetwowks[ten.Name]
				index := iter.index % len(iter.nws)
				nw := iter.nws[index]
				nwIntf.Spec.AttachNetwork = nw.Name
				nwIntf.Spec.AttachTenant = nw.Tenant
				err := sm.ObjClient().UpdateNetworkInterface(nwIntf)
				if err != nil {
					log.Errorf("Error attaching network to interface")
					return err
				}
				iter.index++
			}
			tenIdx++
		}

	} else {
		nws := []*network.Network{}
		for _, ten := range tenants {
			if ten.Name == globals.DefaultTenant {
				continue
			}

			nws, err = sm.ListNetwork(ten.Name)
			if err != nil {
				log.Error("Error finding networks")
				return err
			}
			//Pick the first tenant
			break
		}

		for _, h := range hosts {
			filter := fmt.Sprintf("spec.type=host-pf,status.dsc=%v", h.Naples.Instances[0].Dsc.Status.PrimaryMAC)
			hostNwIntfs, err := sm.ObjClient().ListNetowrkInterfacesByFilter(filter)
			if err != nil {
				return err
			}

			for index, nwIntf := range hostNwIntfs {
				nwIntf.Spec.AttachNetwork = nws[index].Name
				nwIntf.Spec.AttachTenant = nws[index].Tenant
				err := sm.ObjClient().UpdateNetworkInterface(nwIntf)
				if err != nil {
					log.Errorf("Error attaching network to interface")
					return err
				}
			}

		}
	}

	return nil
}

//BringupWorkloads bring up.Workloads on host
func (sm *SysModel) BringupWorkloads() error {

	wc := objects.NewWorkloadCollection(sm.ObjClient(), sm.Tb)

	for _, wload := range sm.WorkloadsObjs {
		wc.Workloads = append(wc.Workloads, wload)
	}

	skipSetup := os.Getenv("SKIP_SETUP")
	// if we are skipping setup we dont need to bringup the workload
	if skipSetup != "" {
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

		getResp.WorkloadOp = iota.Op_DELETE
		delResp, err := topoClient.DeleteWorkloads(context.Background(), getResp)
		log.Debugf("Got get workload resp: %+v, err: %v", delResp, err)
		if err != nil {
			log.Errorf("Failed to delete old Apps. Err: %v", err)
			return fmt.Errorf("Error deleting IOTA workload. err: %v", err)
		}
	}

	// bringup the.Workloads
	err := wc.Bringup(sm.Tb)
	if err != nil {
		return err
	}

	if os.Getenv("DYNAMIC_IP") != "" {
		return sm.WorkloadsGoGetIPs()
	}
	//Add ping for now
	return sm.WorkloadsSayHelloToDataPath()
}

//SetupWorkloads bring up.Workloads on host
func (sm *SysModel) SetupWorkloads(scale bool) error {

	hosts, err := sm.ListRealHosts()
	if err != nil {
		log.Error("Error finding real hosts to run traffic tests")
		return err
	}

	for _, h := range hosts {
		_, err := sm.SetupWorkloadsOnHost(h)
		if err != nil {
			return err
		}
	}

	return sm.BringupWorkloads()
}

func (sm *SysModel) modifyConfig() error {

	log.Infof("Modifying config as per model spec")
	cfgObjects := sm.GetCfgObjects()

	if os.Getenv("DYNAMIC_IP") != "" {
		// Workloads to get IP dynamically. reset static IP
		for _, workload := range cfgObjects.Workloads {
			workload.Spec.Interfaces[0].IpAddresses[0] = ""
		}
	}

	// Workloads DHCP server for all testbeds is configured with 20.20.<testbed-id>.1
	server := "20.20.0.1"
	id, iderr := strconv.ParseInt(sm.Tb.ID(), 10, 64)
	if iderr == nil {
		server = fmt.Sprintf("20.20.%v.1", id)
	}

	for _, ipam := range cfgObjects.Ipams {
		ipam.Spec.DHCPRelay.Servers[0].IPAddress = server
		log.Infof("IPAM %v's DHCPServer: %v\n", ipam.Name, ipam.Spec.DHCPRelay.Servers[0].IPAddress)
	}

	return nil
}

func convertToVeniceFormatMac(s string) string {
	mac := strings.Replace(s, ":", "", -1)
	var buffer bytes.Buffer
	i := 1
	for _, rune := range mac {
		buffer.WriteRune(rune)
		if i != 0 && i%4 == 0 && i != len(mac) {
			buffer.WriteRune('.')
		}
		i++
	}
	return buffer.String()
}

// InitConfig sets up a default config for the system
func (sm *SysModel) InitConfig(scale, scaleData bool) error {
	skipSetup := os.Getenv("SKIP_SETUP")
	skipConfig := os.Getenv("SKIP_CONFIG")
	cfgParams := &base.ConfigParams{
		Scale:                         scale,
		Regenerate:                    skipSetup == "",
		Vlans:                         sm.Tb.AllocatedVlans(),
		NumberOfInterfacesPerWorkload: 1,
	}
	cfgParams.NaplesLoopBackIPs = make(map[string]string)
	for _, naples := range sm.NaplesNodes {
		dscs := []*cluster.DistributedServiceCard{}
		for _, inst := range naples.Instances {
			dscs = append(dscs, inst.Dsc)
		}
		cfgParams.Dscs = append(cfgParams.Dscs, dscs)
		for index, ncfg := range naples.GetTestNode().NaplesConfigs.Configs {
			naples.Instances[index].LoopbackIP = sm.Tb.GetLoopBackIP(naples.GetIotaNode().Name, index+1)
			cfgParams.NaplesLoopBackIPs[convertToVeniceFormatMac(ncfg.NodeUuid)] = naples.Instances[index].LoopbackIP
		}
	}

	for _, naples := range sm.FakeNaples {
		//cfgParams.Dscs = append(cfgParams.Dscs, naples.SmartNic)
		cfgParams.FakeDscs = append(cfgParams.FakeDscs, naples.Instances[0].Dsc)
		//node uuid already in format
		cfgParams.NaplesLoopBackIPs[naples.Instances[0].Dsc.Status.PrimaryMAC] = naples.IP()
		naples.Instances[0].LoopbackIP = naples.IP()
	}

	for _, node := range sm.VeniceNodeMap {
		cfgParams.VeniceNodes = append(cfgParams.VeniceNodes, node.ClusterNode)
	}

	err := sm.PopulateConfig(cfgParams)
	if err != nil {
		return err
	}

	if skipConfig == "" {
		err = sm.CleanupAllConfig()
		if err != nil {
			return err
		}

		err = sm.modifyConfig()
		if err != nil {
			return err
		}

		err = sm.PushConfig()
		if err != nil {
			return err
		}

		ok, err := sm.IsConfigPushComplete()
		if !ok || err != nil {
			return err
		}

	} else {
		log.Info("Skipping config")
	}

	return nil
}

// SetupDefaultConfig sets up a default config for the system
func (sm *SysModel) SetupDefaultConfig(ctx context.Context, scale, scaleData bool) error {

	log.Infof("Setting up default config...")

	sm.Scale = scale
	sm.ScaleData = scaleData

	err := sm.InitConfig(scale, scaleData)
	if err != nil {
		return err
	}

	err = sm.AssociateHosts()
	if err != nil {
		return fmt.Errorf("Error associating hosts: %s", err)
	}

	for _, sw := range sm.Tb.DataSwitches {
		_, err := sm.CreateSwitch(sw)
		if err != nil {
			log.Errorf("Error creating switch: %#v. Err: %v", sw, err)
			return err
		}
	}

	err = sm.attachSimInterfacesToNetworks(scale)
	if err != nil {
		return err
	}
	//Setup any default objects
	return sm.SetupWorkloads(scale)
}

// NetworkSecurityPolicy finds an SG policy by name
func (sm *SysModel) NetworkSecurityPolicy(name string) *objects.NetworkSecurityPolicyCollection {
	pol, ok := sm.sgpolicies[name]
	if !ok {
		pol := objects.NewNetworkSecurityPolicyCollection(nil, sm.ObjClient(), sm.Tb)
		pol.SetError(fmt.Errorf("Policy %v not found", name))
		log.Infof("Error %v", pol.Error())
		return pol
	}

	policyCollection := objects.NewNetworkSecurityPolicyCollection(pol, sm.ObjClient(), sm.Tb)
	policyCollection.Policies = []*objects.NetworkSecurityPolicy{pol}
	return policyCollection
}

//DefaultNetworkSecurityPolicy no default policies
func (sm *SysModel) DefaultNetworkSecurityPolicy() *objects.NetworkSecurityPolicyCollection {
	return nil
}

// NewNetworkSecurityPolicy nodes on the fly
func (sm *SysModel) NewNetworkSecurityPolicy(name string) *objects.NetworkSecurityPolicyCollection {
	policy := &objects.NetworkSecurityPolicy{
		VenicePolicy: &security.NetworkSecurityPolicy{
			TypeMeta: api.TypeMeta{Kind: "NetworkSecurityPolicy"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: "default",
				Name:      name,
			},
			Spec: security.NetworkSecurityPolicySpec{
				AttachTenant: true,
			},
		},
	}
	sm.sgpolicies[name] = policy
	return objects.NewNetworkSecurityPolicyCollection(policy, sm.ObjClient(), sm.Tb)
}

// FindFwlogForWorkloadPairsFromObjStore find fwlog pairs
func (sm *SysModel) FindFwlogForWorkloadPairsFromObjStore(tenantName,
	protocol string, port uint32, fwaction string, wpc *objects.WorkloadPairCollection) error {
	return fmt.Errorf("not implemented")
}

// FindFwlogForWorkloadPairsFromElastic find fwlog pairs
func (sm *SysModel) FindFwlogForWorkloadPairsFromElastic(tenantName,
	protocol string, port uint32, fwaction string, wpc *objects.WorkloadPairCollection) error {
	return fmt.Errorf("not implemented")
}

// GetFwLogObjectCount gets the object count for firewall logs under the bucket with the given name
func (sm *SysModel) GetFwLogObjectCount(tenantName string, bucketName string, objectKeyPrefix string, nodeIpsToSkipFromQuery ...string) (int, error) {
	return 0, fmt.Errorf("not implemented")
}

// NewFwlogPolicy creates new monitoring.FwlogPolicy
func (sm *SysModel) NewFwlogPolicy(name string) *objects.FwlogPolicyCollection {
	return nil
}

// FwlogPolicy finds an existing monitoring.FwlogPolicy
func (sm *SysModel) FwlogPolicy(name string) *objects.FwlogPolicyCollection {
	return nil
}

//TeardownWorkloads teardown workloads
func (sm *SysModel) TeardownWorkloads(wc *objects.WorkloadCollection) error {

	wrkLd := &iota.WorkloadMsg{
		ApiResponse: &iota.IotaAPIResponse{},
		WorkloadOp:  iota.Op_DELETE,
	}

	// Teardown the workloads
	for _, wrk := range wc.Workloads {
		wrkLd.Workloads = append(wrkLd.Workloads, wrk.GetIotaWorkload())
		delete(sm.WorkloadsObjs, wrk.VeniceWorkload.Name)
	}

	topoClient := iota.NewTopologyApiClient(sm.Tb.Client().Client)
	appResp, err := topoClient.DeleteWorkloads(context.Background(), wrkLd)

	if err != nil || appResp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		log.Errorf("Failed to instantiate Apps. %v/%v", err, appResp.ApiResponse.ApiStatus)
		return fmt.Errorf("Error deleting IOTA workload. Resp: %+v, err: %v", appResp.ApiResponse, err)
	}

	log.Info("Delete workload successful")
	return nil
}

//TriggerDeleteAddConfig triggers link flap
func (sm *SysModel) TriggerDeleteAddConfig(percent int) error {

	err := sm.CleanupAllConfig()
	if err != nil {
		return err
	}

	err = sm.TeardownWorkloads(sm.Workloads())
	if err != nil {
		return err
	}

	return sm.SetupDefaultConfig(context.Background(), sm.Scale, sm.ScaleData)
}

func (sm *SysModel) CollectLogs() error {
	return sm.DownloadTechsupport("")
}
