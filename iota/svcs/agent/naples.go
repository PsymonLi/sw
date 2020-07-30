package agent

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net"
	"os"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"
	"unicode/utf8"

	"github.com/pkg/errors"
	"golang.org/x/sync/errgroup"
	"gopkg.in/yaml.v2"

	iota "github.com/pensando/sw/iota/protos/gogen"
	Cmd "github.com/pensando/sw/iota/svcs/agent/command"
	Utils "github.com/pensando/sw/iota/svcs/agent/utils"
	Workload "github.com/pensando/sw/iota/svcs/agent/workload"
	Common "github.com/pensando/sw/iota/svcs/common"
)

const (
	naplesVMBringUpScript   = "naples_vm_bringup.py"
	naplesCfgDir            = "/naples/nic/conf"
	hntapCfgFile            = "hntap-cfg.json"
	naplesDataDir           = Common.DstIotaAgentDir + "/naples"
	arpTimeout              = 3000 //3 seconds
	arpAgeTimeout           = 3000 //3000 seconds
	sshPort                 = 22
	naplesSimHostIntfPrefix = "lif"
	naplesBsdHosIntfPrefix  = "ionic"
	naplesPciDevicePrefix   = "Device"
	bareMetalWorkloadName   = "bareMetalWorkload"
	intelPciDevicePrefix    = "Ethernet"
	maxStdoutSize           = 1024 * 1024 * 2
	naplesStatusURL         = "http://localhost:9008/api/v1/naples/"
	naplesSimBaseMacFile    = Common.DstIotaDBDir + "/sim-base-mac.ini"
)

var (
	naplesProcessess = [...]string{"hal", "netagent", "nic_infra_hntap", "cap_model"}
	hntapCfgTempFile = "/tmp/hntap-cfg.json"
	hntapProcessName = "nic_infra_hntap"
	apiSuccess       = &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_STATUS_OK, ErrorMsg: "Api success"}
	naplesHwUUIDFile = "/tmp/fru.json"
)

var workloadTypeMap = map[iota.WorkloadType]string{
	iota.WorkloadType_WORKLOAD_TYPE_CONTAINER:                 Workload.WorkloadTypeContainer,
	iota.WorkloadType_WORKLOAD_TYPE_VM:                        Workload.WorkloadTypeESX,
	iota.WorkloadType_WORKLOAD_TYPE_BARE_METAL:                Workload.WorkloadTypeBareMetal,
	iota.WorkloadType_WORKLOAD_TYPE_BARE_METAL_MAC_VLAN:       Workload.WorkloadTypeMacVlan,
	iota.WorkloadType_WORKLOAD_TYPE_CONTAINER_MAC_VLAN:        Workload.WorkloadTypeContainerMacVlan,
	iota.WorkloadType_WORKLOAD_TYPE_BARE_METAL_MAC_VLAN_ENCAP: Workload.WorkloadTypeMacVlanEncap,
}

var interfaceTypeMap = map[iota.InterfaceType]string{
	iota.InterfaceType_INTERFACE_TYPE_NONE:      Workload.InterfaceTypeNone,
	iota.InterfaceType_INTERFACE_TYPE_NATIVE:    Workload.InterfaceTypeNative,
	iota.InterfaceType_INTERFACE_TYPE_VSS:       Workload.InterfaceTypeVSS,
	iota.InterfaceType_INTERFACE_TYPE_SRIOV:     Workload.InterfaceTypeSRIOV,
	iota.InterfaceType_INTERFACE_TYPE_DVS:       Workload.InterfaceTypeDVS,
	iota.InterfaceType_INTERFACE_TYPE_DVS_PVLAN: Workload.InterfaceTypeDVS_PVLAN,
	iota.InterfaceType_INTERFACE_TYPE_BOND:      Workload.InterfaceTypeBond,
	iota.InterfaceType_INTERFACE_TYPE_MGMT:      Workload.InterfaceTypeMgmt,
}

var nodOSMap = map[iota.TestBedNodeOs]string{
	iota.TestBedNodeOs_TESTBED_NODE_OS_LINUX:   "linux",
	iota.TestBedNodeOs_TESTBED_NODE_OS_FREEBSD: "freebsd",
	iota.TestBedNodeOs_TESTBED_NODE_OS_ESX:     "esx",
	iota.TestBedNodeOs_TESTBED_NODE_OS_WINDOWS: "windows",
}

type commandNode struct {
	iotaNode
	bgCmds     *sync.Map
	bgCmdIndex uint64
}

type dataNode struct {
	iotaNode
	//entityMap     map[string]iotaWorkload
	entityMap     *sync.Map
	hostEntityKey string
}

type naplesHwNode struct {
	dataNode
	naplesEntityKey        []string
	windowsPortNameMapping map[string]map[string]string
}

type naplesBondHwNode struct {
	naplesHwNode
}

type naplesMultiSimNode struct {
	dataNode
}

type naplesControlSimNode struct {
	dataNode
}

type naplesSimNode struct {
	dataNode
	container       *Utils.Container
	passThroughMode bool
}

type naplesQemuNode struct {
	naplesSimNode
}

type thirdPartyDataNode struct {
	dataNode
}

func (dnode *dataNode) GetMsg() *iota.Node {
	return dnode.nodeMsg
}

func (dnode *dataNode) configureWorkloadInHntap(in *iota.Workload) error {
	return nil
}

func (naples *naplesSimNode) bringUpNaples(name string, image string, ctrlIntf string,
	ctrlIP string, dataIntfs []string, dataIPs []string, naplesIPs []string,
	veniceIPs []string, withQemu bool, passThroughMode bool) error {

	curDir, _ := os.Getwd()
	defer os.Chdir(curDir)
	if err := os.Chdir(Common.ImageArtificatsDirectory); err != nil {
		return err
	}
	dir, _ := os.Getwd()
	naples.logger.Println("Untar image : " + dir + "/" + image)
	untar := []string{"tar", "-xvzf", image}
	if _, stdout, err := Utils.Run(untar, 0, false, false, nil); err != nil {
		return errors.Wrap(err, stdout)
	}
	naples.logger.Println("Untar successfull")
	env := []string{"NAPLES_HOME=" + Common.DstIotaAgentDir, "NAPLES_DATA_DIR=" + naplesDataDir}
	cmd := []string{"sudo", "-E", "python", naplesVMBringUpScript,
		"--data-intfs", strings.Join(dataIntfs, ","), "--name", name}

	if ctrlIntf != "" {
		cmd = append(cmd, "--control-intf")
		cmd = append(cmd, ctrlIntf)
		cmd = append(cmd, "--control-ip")
		cmd = append(cmd, ctrlIP)
	}

	if len(dataIPs) != 0 {
		cmd = append(cmd, "--data-ips")
		cmd = append(cmd, strings.Join(dataIPs, ","))
	}

	if len(naplesIPs) != 0 {
		cmd = append(cmd, "--naples-ips")
		cmd = append(cmd, strings.Join(naplesIPs, ","))
	}

	if len(veniceIPs) != 0 {
		cmd = append(cmd, "--venice-ips")
		cmd = append(cmd, strings.Join(veniceIPs, ","))
	}

	if withQemu {
		cmd = append(cmd, "--qemu")
	}

	cmd = append(cmd, "--nmd-hostname")
	cmd = append(cmd, name)

	cmd = append(cmd, "--hntap-mode")
	if passThroughMode {
		cmd = append(cmd, "passthrough")
	} else {
		cmd = append(cmd, "tunnel")
	}

	if retCode, stdout, err := Utils.Run(cmd, nodeAddTimeout, false, false, env); err != nil || retCode != 0 {
		naples.logger.Println(stdout)
		msg := "Naples bring script up failed."
		naples.logger.Error(msg)
		return errors.Wrap(err, msg)
	}

	naples.logger.Println("Naples bring script up succesfull.")

	naples.name = name

	var err error
	if naples.container, err = Utils.GetContainer(name, "", name, "", Workload.ContainerPrivileged, nil); err != nil {
		return errors.Wrap(err, "Naples sim not running!")
	}

	naples.logger.Println("Naples bring up successfull")

	if err := naples.container.WaitForHealthy(naplesHealthyTimeout); err != nil {
		naples.container = nil
		return errors.Wrap(err, "Naples healthy timeout exceeded")
	}

	naples.passThroughMode = passThroughMode
	return nil
}

func (naples *naplesSimNode) setArpTimeouts() error {
	cmd := []string{"sysctl", "-w", "net.ipv4.neigh.default.retrans_time_ms=" + strconv.Itoa(arpTimeout)}
	if retCode, _, _ := Utils.Run(cmd, 0, false, false, nil); retCode != 0 {
		return errors.New("ARP retrans timeout set failed")
	}

	cmd = []string{"sysctl", "-w", "net.ipv4.neigh.default.gc_stale_time=" + strconv.Itoa(arpAgeTimeout)}
	if retCode, _, _ := Utils.Run(cmd, 0, false, false, nil); retCode != 0 {
		return errors.New("ARP entry age timeout set failed")
	}

	return nil
}

func (dnode *dataNode) init(in *iota.Node) {
	dnode.entityMap = new(sync.Map)
	dnode.nodeMsg = in
}

func (naples *naplesSimNode) addNodeEntities(in *iota.Node) error {
	for _, entityEntry := range in.GetEntities() {
		var wload Workload.Workload
		if entityEntry.GetType() == iota.EntityType_ENTITY_TYPE_NAPLES {
			wload = Workload.NewWorkload(Workload.WorkloadTypeContainer, entityEntry.GetName(), naples.name, naples.logger)
		} else if entityEntry.GetType() == iota.EntityType_ENTITY_TYPE_HOST {
			wload = Workload.NewWorkload(Workload.WorkloadTypeBareMetal, entityEntry.GetName(), naples.name, naples.logger)
		}

		wDir := Common.DstIotaEntitiesDir + "/" + entityEntry.GetName()
		wload.SetBaseDir(wDir)
		if err := wload.BringUp(in.GetName(), ""); err != nil {
			naples.logger.Errorf("Naples sim entity type add failed")
			return err
		}
		if wload != nil {
			naples.entityMap.Store(entityEntry.GetName(), iotaWorkload{workload: wload, name: entityEntry.GetName()})
		}
	}
	return nil
}

func (naples *naplesSimNode) init(in *iota.Node, withQemu bool) (resp *iota.Node, err error) {

	naples.iotaNode.name = in.GetName()
	naples.dataNode.init(in)
	naples.logger.Println("Bring up request received for : " + in.GetName())

	nodeuuid := ""
	naplesConfig := in.GetNaplesConfigs().GetConfigs()[0]
	if naplesConfig.GetControlIntf() != "" {
		var err error
		nodeuuid, err = Utils.GetIntfMacAddress(naplesConfig.ControlIntf)
		if err != nil {
			msg := fmt.Sprintf("Control interface %s not found", naplesConfig.ControlIntf)
			naples.logger.Errorf(msg)
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST,
				ErrorMsg: msg}}, err
		}
		naplesConfig.NodeUuid = nodeuuid
	}

	if err := naples.setArpTimeouts(); err != nil {
		msg := fmt.Sprintf("Sysctl set failed : %s : %s", in.GetName(), err.Error())
		naples.logger.Error(msg)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
	}

	if err := naples.bringUpNaples(in.GetName(),
		in.GetImage(), naplesConfig.GetControlIntf(),
		naplesConfig.GetControlIp(),
		naplesConfig.GetDataIntfs(), nil, nil,
		naplesConfig.GetVeniceIps(), withQemu, true); err != nil {
		resp := "Naples bring up failed : " + err.Error()
		naples.logger.Error(resp)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
	}
	naples.logger.Println("Naples bring up succesfull")

	//if in.GetNaplesConfig() == nil {
	//	in.NodeInfo = &iota.Node_NaplesConfig{NaplesConfigs: &iota.NaplesConfig{}}
	//}

	naplesConfig.HostIntfs = append(naplesConfig.HostIntfs,
		&iota.Interfaces{
			Type:       iota.InterfaceType_INTERFACE_TYPE_NATIVE,
			Interfaces: Utils.GetIntfsMatchingPrefix(naplesSimHostIntfPrefix),
		})

	naples.logger.Println("Naples sim host interfaces : ", naplesConfig.HostIntfs[0].Interfaces)

	/* Finally add entity type */
	if err := naples.addNodeEntities(in); err != nil {
		msg := fmt.Sprintf("Adding node entities failed : %s", err.Error())
		naples.logger.Error(msg)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
	}

	return &iota.Node{NodeStatus: apiSuccess,
		Name: in.GetName(), IpAddress: in.GetIpAddress(), Type: in.GetType(),
		NodeInfo: &iota.Node_NaplesConfigs{NaplesConfigs: in.GetNaplesConfigs()}}, nil
}

//Init initialize node type
func (naples *naplesSimNode) Init(in *iota.Node) (resp *iota.Node, err error) {
	return naples.init(in, false)
}

func (naples *naplesSimNode) configureWorkloadInHntap(in *iota.Workload) error {

	if !naples.passThroughMode {
		/* no configuraiton if not in pass through mode */
		return nil
	}
	hntapFile := naplesCfgDir + "/" + hntapCfgFile

	cpCmd := []string{"docker", "cp", naples.name + ":" + hntapFile, hntapCfgTempFile}

	if retCode, stdout, err := Utils.Run(cpCmd, 0, false, false, nil); err != nil || retCode != 0 {
		naples.logger.Error(stdout)
		return errors.Wrapf(err, "Hntap copy failed")
	}

	fmt.Println(hntapCfgTempFile)

	dir, _ := os.Getwd()
	fmt.Println(dir)
	file, e := ioutil.ReadFile(hntapCfgTempFile)
	if e != nil {
		naples.logger.Error("Error in reading hntap file")
		return errors.Wrap(e, "Error opening hntap cfg file")
	}

	var hntapData map[string]interface{}
	var err error

	err = json.Unmarshal(file, &hntapData)
	if err != nil {
		naples.logger.Errorf("Error in unmarshaling hntap cfg file")
		return errors.Wrap(err, "Error in unmarshalling hntap cfg file")
	}

	if _, ok := hntapData["switch"]; !ok {
		naples.logger.Errorf("No switch section in hntap")
		return errors.New("Switch section not found in hntap cfg file")
	}

	switchData := hntapData["switch"].(map[string]interface{})
	if _, ok := switchData["passthrough-mode"]; !ok {
		naples.logger.Errorf("No passthrough section in hntap")
		return errors.New("Passthrough mode section not found")
	}

	passThroughModeData := switchData["passthrough-mode"].(map[string]interface{})
	if _, ok := passThroughModeData["allowed-macs"]; !ok {
		naples.logger.Errorf("No allowed-macs section in hntap")
		return errors.New("Allowed Macs section not found")
	}

	allowedMacs := passThroughModeData["allowed-macs"].(map[string]interface{})

	macData := make(map[string]uint32)
	macData["port"] = in.Interfaces[0].GetPinnedPort()
	macData["vlan"] = in.Interfaces[0].GetUplinkVlan()
	allowedMacs[in.Interfaces[0].GetMacAddress()] = macData

	hntapJSON, _ := json.MarshalIndent(hntapData, "", "\t")
	if err := ioutil.WriteFile(hntapCfgTempFile, hntapJSON, 0644); err != nil {
		naples.logger.Println("Error in hntap file write")
		return errors.New("Allowed Macs section not found")
	}

	cpCmd = []string{"docker", "cp", hntapCfgTempFile, naples.name + ":" + hntapFile}
	if retCode, stdout, err := Utils.Run(cpCmd, 0, false, false, nil); err != nil || retCode != 0 {
		naples.logger.Println(stdout)
		naples.logger.Println("Hntap copy failed")
		return err
	}

	noitfyHntapCmd := []string{"pkill", "-SIGUSR1", hntapProcessName}
	if naples.container != nil {

		cmdHandle, _ := naples.container.SetUpCommand(noitfyHntapCmd, "", false, false)
		cmdResp, err := naples.container.RunCommand(cmdHandle, 0)

		if err != nil || cmdResp.RetCode != 0 {
			naples.logger.Println("Error in sending signal to Hntap")
		}
	}

	naples.logger.Println("Update hntap configuration for the workload")

	//Give it 5 seconds to make sure Hntap restarted and updated.
	time.Sleep(5 * time.Second)
	return nil
}

func (dnode *dataNode) configureWorkload(wload Workload.Workload, in *iota.Workload) (*iota.Workload, error) {

	//var intf string

	for index, intf := range in.Interfaces {
		spec := Workload.InterfaceSpec{
			IntfType:      interfaceTypeMap[intf.GetInterfaceType()],
			Parent:        intf.GetParentInterface(),
			Name:          intf.GetInterface(),
			Mac:           intf.GetMacAddress(),
			IPV4Address:   intf.GetIpPrefix(),
			IPV6Address:   intf.GetIpv6Prefix(),
			PrimaryVlan:   int(intf.GetEncapVlan()),
			SecondaryVlan: int(intf.GetSecondaryEncapVlan()),
			Switch:        intf.GetSwitchName(),
			NetworkName:   intf.GetNetworkName(),
			Index:         int(intf.GetInterfaceIndex()),
		}
		attachedIntf, err := wload.AddInterface(spec)
		if err != nil {
			msg := fmt.Sprintf("Error in Interface attachment %s : %s", in.GetWorkloadName(), err.Error())
			dnode.logger.Error(msg)
			resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}
			return resp, err
		}

		err = wload.AddSecondaryIpv4Addresses(attachedIntf, intf.GetSecIpPrefix())
		if err != nil {
			msg := fmt.Sprintf("Error Adding Secondary IPv4 addresses %s : %s", in.GetWorkloadName(), err.Error())
			resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}
			return resp, err
		}

		err = wload.AddSecondaryIpv6Addresses(attachedIntf, intf.GetSecIpv6Prefix())
		if err != nil {
			msg := fmt.Sprintf("Error Adding Secondary IPv6 addresses %s : %s", in.GetWorkloadName(), err.Error())
			resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}
			return resp, err
		}

		/* For SRIOV case, move the parent interface inside the workload so that it is not shared */
		if intf.GetInterfaceType() == iota.InterfaceType_INTERFACE_TYPE_SRIOV {
			wload.MoveInterface(intf.GetInterface())
		}
		in.Interfaces[index].Interface = attachedIntf

	}
	resp := *in
	resp.WorkloadStatus = apiSuccess
	return &resp, nil
}

func (dnode *dataNode) setupWorkload(wload Workload.Workload, in *iota.Workload) (*iota.Workload, error) {
	/* Create working directory and set that as base dir */
	wDir := Common.DstIotaEntitiesDir + "/" + in.GetWorkloadName()

	ports := ""
	for _, wport := range in.ExposedPorts {
		ports = ports + wport.Port + ":" + wport.Proto + ","
	}
	wload.SetBaseDir(wDir)
	if err := wload.BringUp(in.GetWorkloadName(), in.GetWorkloadImage(), ports); err != nil {
		msg := fmt.Sprintf("Error in workload image bring up : %s : %s", in.GetWorkloadName(), err.Error())
		dnode.logger.Error(msg)
		resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}
		return resp, err
	}
	dnode.logger.Printf("Bring up workload : %s done", in.GetWorkloadName())

	return dnode.configureWorkload(wload, in)
}

func (dnode *dataNode) isSimNode() bool {
	return dnode.nodeMsg.GetType() == iota.PersonalityType_PERSONALITY_NAPLES_SIM
}

// AddWorkload brings up a workload type on a given node
func (dnode *dataNode) AddWorkloads(in *iota.WorkloadMsg) (*iota.WorkloadMsg, error) {

	addWorkload := func(in *iota.Workload) *iota.Workload {

		if _, ok := iota.WorkloadType_name[(int32)(in.GetWorkloadType())]; !ok {
			msg := fmt.Sprintf("Workload Type %d not supported", in.GetWorkloadType())
			resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST, ErrorMsg: msg}}
			return resp
		}

		wloadKey := in.GetWorkloadName()
		var iotaWload iotaWorkload
		dnode.logger.Printf("Adding workload : %s", in.GetWorkloadName())
		if item, ok := dnode.entityMap.Load(wloadKey); ok {
			msg := fmt.Sprintf("Trying to add workload %s, which already exists ", wloadKey)
			dnode.logger.Error(msg)
			resp, _ := dnode.configureWorkload(item.(iotaWorkload).workload, in)
			return resp

		}

		wlType, ok := workloadTypeMap[in.GetWorkloadType()]
		if !ok {
			msg := fmt.Sprintf("Workload type %v not found", in.GetWorkloadType())
			dnode.logger.Error(msg)
			resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST, ErrorMsg: msg}}
			return resp
		}

		iotaWload.workload = Workload.NewWorkload(wlType, in.GetWorkloadName(), dnode.name, dnode.logger)

		if iotaWload.workload == nil {
			msg := fmt.Sprintf("Trying to add workload of invalid type : %v", wlType)
			dnode.logger.Error(msg)
			resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST, ErrorMsg: msg}}
			return resp
		}

		dnode.logger.Printf("Setting up workload : %s", in.GetWorkloadName())
		resp, err := dnode.setupWorkload(iotaWload.workload, in)

		if err != nil || resp.GetWorkloadStatus().GetApiStatus() != iota.APIResponseType_API_STATUS_OK {
			iotaWload.workload.TearDown()
			return resp
		}

		for _, intf := range resp.Interfaces {
			dnode.logger.Infof("Sending ARP probe with ip %v intf %v", strings.Split(intf.GetIpPrefix(), "/")[0], intf.GetInterface())
			if err := iotaWload.workload.SendArpProbe(strings.Split(intf.GetIpPrefix(), "/")[0], intf.GetInterface(),
				0); err != nil {
				msg := fmt.Sprintf("Error in sending arp probe : %s", err.Error())
				dnode.logger.Error(msg)
				resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}
				dnode.entityMap.Delete(in.GetWorkloadName())
				iotaWload.workload.TearDown()
				return resp
			}
		}

		iotaWload.workloadMsg = in
		dnode.entityMap.Store(wloadKey, iotaWload)
		dnode.logger.Printf("Added workload : %s (%s)", in.GetWorkloadName(), in.GetWorkloadType())
		return resp
	}

	pool, _ := errgroup.WithContext(context.Background())
	maxParallelThreads := 128
	currThreads := 0
	scheduleWloads := []*iota.Workload{}
	wloadIndex := []int{}

	for index, wload := range in.Workloads {
		currThreads++
		scheduleWloads = append(scheduleWloads, wload)
		wloadIndex = append(wloadIndex, index)
		if currThreads == maxParallelThreads-1 || index+1 == len(in.Workloads) {
			for thread, wl := range scheduleWloads {
				wload := wl
				index := wloadIndex[thread]
				pool.Go(func() error {
					resp := addWorkload(wload)
					in.Workloads[index] = resp
					return nil
				})
			}
			pool.Wait()
			scheduleWloads = []*iota.Workload{}
			wloadIndex = []int{}
			currThreads = 0
		}
	}

	pool.Wait()
	for _, wload := range in.Workloads {
		if wload.WorkloadStatus.ApiStatus != iota.APIResponseType_API_STATUS_OK {
			in.ApiResponse = wload.WorkloadStatus
			return in, errors.New(wload.WorkloadStatus.ErrorMsg)
		}
	}
	in.ApiResponse = &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_STATUS_OK}

	return in, nil

}

// AddWorkload brings up a workload type on a given node
func (naples *naplesSimNode) AddWorkloads(in *iota.WorkloadMsg) (*iota.WorkloadMsg, error) {

	resp, err := naples.dataNode.AddWorkloads(in)
	if err != nil || resp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		return resp, nil
	}

	simAddWorkload := func(in *iota.Workload) *iota.Workload {
		wloadKey := in.GetWorkloadName()
		item, _ := naples.dataNode.entityMap.Load(wloadKey)
		iotaWload := item.(iotaWorkload)
		/* Notify Hntap of the workload */
		if err := naples.configureWorkloadInHntap(in); err != nil {
			msg := fmt.Sprintf("Error in configuring workload to hntap %s", err.Error())
			naples.logger.Error(msg)
			resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}
			naples.entityMap.Delete(in.GetWorkloadName())
			iotaWload.workload.TearDown()
			return resp

		}

		for _, intf := range in.Interfaces {
			//Set vlan 0 as interface is already set before
			if err := iotaWload.workload.SendArpProbe(strings.Split(intf.GetIpPrefix(), "/")[0], intf.GetInterface(), 0); err != nil {
				msg := fmt.Sprintf("Error in sending arp probe %s", err.Error())
				naples.logger.Errorf(msg)
				resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}
				naples.entityMap.Delete(in.GetWorkloadName())
				iotaWload.workload.TearDown()
				return resp
			}
		}
		in.WorkloadStatus = &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_STATUS_OK}
		return in

	}

	for index, wload := range resp.Workloads {
		sresp := simAddWorkload(wload)
		resp.Workloads[index] = sresp
	}

	for _, wload := range in.Workloads {
		if wload.WorkloadStatus.ApiStatus != iota.APIResponseType_API_STATUS_OK {
			resp.ApiResponse = wload.WorkloadStatus
			return resp, errors.New(wload.WorkloadStatus.ErrorMsg)
		}
	}

	return resp, nil

}

// DeleteWorkload deletes a given workload
func (dnode *dataNode) DeleteWorkloads(in *iota.WorkloadMsg) (*iota.WorkloadMsg, error) {

	deleteWorkload := func(in *iota.Workload) *iota.Workload {
		var ok bool
		var item interface{}
		dnode.logger.Printf("Deleting workload : %s", in.GetWorkloadName())

		wloadKey := in.GetWorkloadName()

		if item, ok = dnode.entityMap.Load(wloadKey); !ok {
			msg := fmt.Sprintf("Trying to delete workload %s which does not exist", wloadKey)
			dnode.logger.Error(msg)
			resp := &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST, ErrorMsg: msg}}
			return resp
		}

		item.(iotaWorkload).workload.TearDown()
		dnode.entityMap.Delete(wloadKey)
		dnode.logger.Printf("Deleted workload : %s", in.GetWorkloadName())
		resp := &iota.Workload{WorkloadStatus: apiSuccess}
		return resp
	}
	pool, _ := errgroup.WithContext(context.Background())
	for index, wload := range in.Workloads {
		wload := wload
		index := index
		pool.Go(func() error {
			resp := deleteWorkload(wload)
			in.Workloads[index] = resp
			return nil
		})
	}

	pool.Wait()
	for _, wload := range in.Workloads {
		if wload.WorkloadStatus.ApiStatus != iota.APIResponseType_API_STATUS_OK {
			in.ApiResponse = wload.WorkloadStatus
			return in, errors.New(wload.WorkloadStatus.ErrorMsg)
		}
	}
	in.ApiResponse = &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_STATUS_OK}

	return in, nil
}

// Trigger invokes the workload's trigger. It could be ping, start client/server etc..
func (dnode *dataNode) triggerValidate(in *iota.TriggerMsg) (*iota.TriggerMsg, error) {
	dnode.logger.Println("Trigger message received for validation.")

	validate := func() error {
		for _, cmd := range in.Commands {
			wloadKey := cmd.GetEntityName()
			if _, ok := dnode.entityMap.Load(wloadKey); !ok {
				msg := fmt.Sprintf("Workload %s does not exist on node %s", cmd.GetEntityName(), dnode.NodeName())
				return errors.New(msg)
			}
		}
		return nil
	}

	if err := validate(); err != nil {
		return &iota.TriggerMsg{ApiResponse: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST, ErrorMsg: err.Error()}}, err
	}

	return in, nil
}

// Trigger invokes the workload's trigger. It could be ping, start client/server etc..
func (dnode *dataNode) Trigger(in *iota.TriggerMsg) (*iota.TriggerMsg, error) {
	dnode.logger.Println("Trigger message received.")
	if _, err := dnode.triggerValidate(in); err != nil {
		return &iota.TriggerMsg{ApiResponse: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST, ErrorMsg: err.Error()}}, nil
	}

	runCommand := func(cmd *iota.Command) error {
		var err error
		var cmdKey string
		var cmdResp *Cmd.CmdCtx
		wloadKey := cmd.GetEntityName()

		item, _ := dnode.entityMap.Load(wloadKey)
		if in.TriggerOp == iota.TriggerOp_EXEC_CMDS {
			cmdResp, cmdKey, err = item.(iotaWorkload).workload.RunCommand(strings.Split(cmd.GetCommand(), " "),
				cmd.GetRunningDir(), cmd.GetRetriesOnFailure(), cmd.GetForegroundTimeout(),
				cmd.GetMode() == iota.CommandMode_COMMAND_BACKGROUND, true)
		} else if in.TriggerOp == iota.TriggerOp_WAIT_FOR_CMDS {
			cmdResp, err = item.(iotaWorkload).workload.WaitCommand(cmd.Handle)
		} else {
			cmdResp, err = item.(iotaWorkload).workload.StopCommand(cmd.Handle)
			cmdKey = cmd.Handle
		}

		if cmdResp != nil {
			cmd.ExitCode, cmd.Stdout, cmd.Stderr, cmd.Handle, cmd.TimedOut = cmdResp.ExitCode, cmdResp.Stdout, cmdResp.Stderr, cmdKey, cmdResp.TimedOut
		} else {
			cmd.ExitCode = 1
			cmd.Stderr = fmt.Sprintf("Failed to run command (Infra error %v)", err)
		}

		if cmd.StderrOnErr && cmd.ExitCode == 0 {
			cmd.Stderr = ""
		}

		if cmd.StdoutOnErr && cmd.ExitCode == 0 {
			cmd.Stdout = ""
		}

		dnode.logger.Println("Command error :", err)
		dnode.logger.Println("Command exit code :", cmd.ExitCode)
		dnode.logger.Println("Command timed out :", cmd.TimedOut)
		dnode.logger.Println("Command handle  :", cmd.Handle)
		dnode.logger.Println("Command stdout :", cmd.Stdout)
		dnode.logger.Println("Command stderr:", cmd.Stderr)

		fixUtf := func(r rune) rune {
			if r == utf8.RuneError {
				return -1
			}
			return r
		}

		cmd.Stdout = strings.Map(fixUtf, cmd.Stdout)
		cmd.Stderr = strings.Map(fixUtf, cmd.Stderr)
		if len(cmd.Stdout) > maxStdoutSize || len(cmd.Stderr) > maxStdoutSize {
			cmd.Stdout = ""
			cmd.Stderr = "Stdout/Stderr output limit Exceeded."
			cmd.ExitCode = 127
		}
		return nil
	}

	if in.GetTriggerOp() == iota.TriggerOp_TERMINATE_ALL_CMDS || in.GetTriggerMode() == iota.TriggerMode_TRIGGER_NODE_PARALLEL {
		maxParallelThreads := 128
		currThreads := 0
		scheduleCmds := []*iota.Command{}
		for cmdIndex, cmd := range in.Commands {
			currThreads++
			scheduleCmds = append(scheduleCmds, cmd)
			if currThreads == maxParallelThreads-1 || cmdIndex+1 == len(in.Commands) {
				pool, _ := errgroup.WithContext(context.Background())
				for thread, cmd := range scheduleCmds {
					cmd := cmd
					dnode.logger.Println("Started thread :", thread)
					pool.Go(func() error {
						runCommand(cmd)
						return nil
					})
				}
				pool.Wait()
				scheduleCmds = []*iota.Command{}
				currThreads = 0
			}

		}

	} else {
		for _, cmd := range in.Commands {
			runCommand(cmd)
		}
	}

	dnode.logger.Println("Completed running trigger.")
	in.ApiResponse = apiSuccess
	return in, nil
}

func (naples *naplesSimNode) allNaplesProcessRunning() bool {
	for _, process := range naplesProcessess {
		if !naples.container.CheckProcessRunning(process) {
			naples.logger.Printf("Process : %s is not running", process)
			return false
		}
		naples.logger.Printf("Process : %s is running", process)
	}

	return true
}

// CheckHealth returns the node health
func (naples *naplesSimNode) CheckHealth(in *iota.NodeHealth) (*iota.NodeHealth, error) {
	naples.logger.Println("Checking node health")
	if naples.container == nil || !naples.container.IsHealthy() {
		msg := fmt.Sprintf("Naples  :%s is down or unhealthy", naples.name)
		naples.logger.Printf(msg)
		return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_NAPLES_DOWN}, nil
	}

	if err := naples.EntitiesHealthy(); err != nil {
		return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_APP_DOWN}, nil
	}

	if !naples.allNaplesProcessRunning() {
		naples.logger.Println("Not all processs running as expected")
		return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_NAPLES_DOWN}, nil
	}
	naples.logger.Println("Node healthy")
	return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_HEALTH_OK}, nil
}

func (naples *naplesSimNode) Destroy(in *iota.Node) (*iota.Node, error) {

	wlmsg := &iota.WorkloadMsg{}
	for _, wl := range naples.GetWorkloadMsgs() {
		wlmsg.Workloads = append(wlmsg.Workloads, wl)
	}
	naples.DeleteWorkloads(wlmsg)

	naples.iotaNode.Destroy(in)

	if naples.container != nil {
		naples.logger.Println("Stopping naples container.")
		naples.container.Stop()
		naples.container = nil
	}

	naples.logger.Println("Destroying naples personality.")
	return &iota.Node{NodeStatus: apiSuccess}, nil
}

//NodeType return node type
func (naples *naplesSimNode) NodeType() iota.PersonalityType {
	return iota.PersonalityType_PERSONALITY_NAPLES_SIM
}

//Init initialize node type
func (naples *naplesQemuNode) Init(in *iota.Node) (resp *iota.Node, err error) {
	return naples.init(in, true)
}

func (naples *naplesQemuNode) Destroy(in *iota.Node) (*iota.Node, error) {
	naples.naplesSimNode.Destroy(in)
	killCmd := []string{"pkill", "-9", "-f", "qemu-run"}
	Utils.Run(killCmd, 0, false, false, nil)
	killCmd = []string{"pkill", "-9", "-f", "qemu-system"}
	Utils.Run(killCmd, 0, false, false, nil)
	return &iota.Node{NodeStatus: apiSuccess}, nil
}

// AddWorkload brings up a workload type on a given node
func (naples *naplesQemuNode) AddWorkload(*iota.Workload) (*iota.Workload, error) {
	return nil, nil
}

// DeleteWorkload deletes a given workload
func (naples *naplesQemuNode) DeleteWorkloads(*iota.WorkloadMsg) (*iota.WorkloadMsg, error) {
	return nil, nil
}

// Trigger invokes the workload's trigger. It could be ping, start client/server etc..
func (naples *naplesQemuNode) Trigger(*iota.TriggerMsg) (*iota.TriggerMsg, error) {
	return nil, nil
}

// CheckHealth returns the node health
func (naples *naplesQemuNode) CheckHealth(in *iota.NodeHealth) (*iota.NodeHealth, error) {
	return naples.naplesSimNode.CheckHealth(in)
}

func (naples *naplesHwNode) addNodeEntities(in *iota.Node) error {
	for _, entityEntry := range in.GetEntities() {
		var wload Workload.Workload
		if entityEntry.GetType() == iota.EntityType_ENTITY_TYPE_NAPLES {
			/*It is like running in a vm as its accesible only by ssh */
			wload = Workload.NewWorkload(Workload.WorkloadTypeRemote, entityEntry.GetName(), naples.name, naples.logger)
		} else if entityEntry.GetType() == iota.EntityType_ENTITY_TYPE_HOST {
			wload = Workload.NewWorkload(Workload.WorkloadTypeBareMetal, entityEntry.GetName(), naples.name, naples.logger)
			naples.hostEntityKey = entityEntry.GetName()
			if err := wload.BringUp(); err != nil {
				naples.logger.Errorf("Host Hw entity type add failed %v", err.Error())
			}
			wDir := Common.DstIotaEntitiesDir + "/" + entityEntry.GetName()
			wload.SetBaseDir(wDir)
			naples.entityMap.Store(entityEntry.GetName(), iotaWorkload{workload: wload})
			continue
		}

		wDir := Common.DstIotaEntitiesDir + "/" + entityEntry.GetName()
		wload.SetBaseDir(wDir)

		//in.GetNaplesConfig().
		for _, naplesCfg := range in.GetNaplesConfigs().GetConfigs() {
			if naplesCfg.Name != entityEntry.Name {
				continue
			}
			ip := naplesCfg.NaplesIpAddress
			if naplesCfg.NaplesSecondaryIpAddress != "" {
				ip = naplesCfg.NaplesSecondaryIpAddress
			}
			if err := wload.BringUp(ip,
				strconv.Itoa(sshPort), naplesCfg.GetNaplesUsername(), naplesCfg.GetNaplesPassword()); err != nil {
				naples.logger.Errorf("Naples Hw entity type add failed %v", err.Error())
				//Ignore error as runner might copy the pub key to remote entity later.
				//return err
			}
			if wload != nil {
				naples.entityMap.Store(entityEntry.GetName(), iotaWorkload{workload: wload})
			}
			naples.naplesEntityKey = append(naples.naplesEntityKey, entityEntry.GetName())
		}
	}
	return nil
}

func (naples *naplesHwNode) getHwUUID(naplesEntityKey string) (uuid string, err error) {

	naplesEntity, ok := naples.entityMap.Load(naplesEntityKey)
	if !ok {
		return "", errors.Errorf("Naples entity not added : %s", naplesEntityKey)
	}

	cmd := []string{"cat", naplesHwUUIDFile}
	cmdResp, _, _ := naplesEntity.(iotaWorkload).workload.RunCommand(cmd, "", 0, 0, false, true)
	naples.logger.Printf("naples hw uuid out %s", cmdResp.Stdout)
	naples.logger.Printf("naples hw uuid err %s", cmdResp.Stderr)
	naples.logger.Printf("naples hw uuid exit code %d", cmdResp.ExitCode)

	if cmdResp.ExitCode != 0 {
		return "", errors.Errorf("Error executing command")
	}

	var deviceJSON map[string]interface{}
	if err := json.Unmarshal([]byte(cmdResp.Stdout), &deviceJSON); err != nil {
		return "", errors.Errorf("Error reading %s file", naplesHwUUIDFile)
	}

	if val, ok := deviceJSON["mac-address"]; ok {
		return val.(string), nil
	}

	return "", errors.Errorf("Mac address not present in  %s file", naplesHwUUIDFile)

}

var hostIntfCmd = func(osType string, nicType, hint string) []string {
	fullCmd := []string{Common.DstIotaNicFinderCommand, "--mac-hint", hint, "--intf-type", "data-nic", "--op", "intfs", "--os", osType}

	return fullCmd
}

var virtualFuncCmd = func(osType string, nicType, hint string) []string {
	fullCmd := []string{Common.DstIotaNicFinderCommand, "--mac-hint", hint, "--intf-type", "virt-funcs", "--op", "intfs", "--os", osType}

	return fullCmd
}

var bondIntfCmd = func(osType string, nicType, hint string) []string {
	fullCmd := []string{Common.DstIotaNicFinderCommand, "--mac-hint", hint, "--intf-type", "bond-intf", "--op", "intfs", "--os", osType}

	return fullCmd
}

func (naples *naplesHwNode) getHostInterfaces(osType string, nicType, hint string) ([]string, error) {
	var hostIntfs []string

	if hint == "" {
		return naples.dataNode.getHostInterfaces(osType, nicType, hint)
	}
	fullCmd := hostIntfCmd(osType, nicType, hint)

	_, stdout, err := Utils.RunCmd(fullCmd, 0, false, true, nil)
	if err != nil {
		msg := fmt.Sprintf("Failed to run command to discover interfaces %v", strings.Join(fullCmd, " "))
		return nil, errors.New(msg)
	}
	naples.logger.Printf("Host interfaces cmd : %v", strings.Join(fullCmd, " "))
	naples.logger.Printf("Host interfaces output : %v", strings.Split(stdout, "\n"))

	for _, intf := range strings.Split(stdout, "\n") {
		if intf != "" {
			hostIntfs = append(hostIntfs, intf)
		}

	}
	return hostIntfs, nil
}

func (naples *naplesHwNode) getVirtualFunctions(osType string, nicType, hint string) ([]string, error) {
	var virtualFunctions []string

	naples.logger.Printf("Search Virtual-functions for OS %v and hint: %v", osType, hint)
	if hint == "" {
		return virtualFunctions, nil
	}
	fullCmd := virtualFuncCmd(osType, nicType, hint)
	_, stdout, err := Utils.RunCmd(fullCmd, 0, false, true, nil)
	if err != nil {
		return nil, err
	}
	naples.logger.Printf("Host virtual-functions cmd : %v", strings.Join(fullCmd, " "))
	naples.logger.Printf("Host virtual-function output: %v", strings.Split(stdout, "\n"))

	for _, intf := range strings.Split(stdout, "\n") {
		if intf != "" {
			virtualFunctions = append(virtualFunctions, intf)
		}

	}
	naples.logger.Printf("Found %v virtual-functions", len(virtualFunctions))
	return virtualFunctions, nil
}

var naplesIPCmd = func(osType string, nicType, hint string) []string {
	fullCmd := []string{Common.DstIotaNicFinderCommand, "--mac-hint", hint, "--intf-type", "int-mnic", "--op", "mnic-ip", "--os", osType}

	return fullCmd
}

var naplesMgmtIntfCmd = func(osType string, nicType, hint string) []string {
	fullCmd := []string{Common.DstIotaNicFinderCommand, "--mac-hint", hint, "--intf-type", "int-mnic", "--op", "intfs", "--os", osType}

	return fullCmd
}

func (naples *naplesHwNode) getNaplesMgmtIP(osType string, nicType, hint string) (string, error) {

	if hint == "" {
		return Common.NaplesMnicIP, nil
	}

	fullCmd := naplesIPCmd(osType, nicType, hint)
	exitCode, stdout, err := Utils.RunCmd(fullCmd, 0, false, true, nil)
	if err != nil {
		msg := fmt.Sprintf("Failed to run command to discover naples mgmt IP %v", strings.Join(fullCmd, " "))
		return "", errors.New(msg)
	}
	naples.logger.Printf("Naple Mgmt  IP cmd : %v", strings.Join(fullCmd, " "))
	naples.logger.Printf("Naple Mgmt output  cmd : %v", strings.Split(stdout, "\n"))

	if exitCode != 0 {
		return "", errors.Errorf("Running command failed %s: %s", strings.Join(fullCmd, " "), stdout)
	}

	return strings.TrimSpace(strings.Split(stdout, "\n")[0]), nil

}

func (naples *naplesHwNode) getNaplesMgmtInterface(osType string, nicType, hint string) (string, error) {

	if hint == "" {
		return "", nil
	}

	fullCmd := naplesMgmtIntfCmd(osType, nicType, hint)
	exitCode, stdout, err := Utils.RunCmd(fullCmd, 0, false, true, nil)
	if err != nil {
		msg := fmt.Sprintf("Failed to run command to discover naples mgmt IP %v", strings.Join(fullCmd, " "))
		return "", errors.New(msg)
	}
	naples.logger.Printf("Naple Mgmt Interface cmd : %v", strings.Join(fullCmd, " "))
	naples.logger.Printf("Naple Mgmt Interface output  cmd : %v", strings.Split(stdout, "\n"))

	if exitCode != 0 {
		return "", errors.Errorf("Running command failed %s: %s", strings.Join(fullCmd, " "), stdout)
	}

	return strings.TrimSpace(strings.Split(stdout, "\n")[0]), nil

}

func (naples *naplesHwNode) initNaplesMgmtInterface(nodeOs iota.TestBedNodeOs, naplesConfig *iota.NaplesConfig) error {
	//IF IP is already set, then skip this procedure
	if naplesConfig.NaplesIpAddress == "" {
		naplesIPddress, err := naples.getNaplesMgmtIP(nodOSMap[nodeOs], naplesConfig.GetNicType(), naplesConfig.GetNicHint())
		if err != nil {
			msg := fmt.Sprintf("Error in reading naples mgmt IP address : %s", err.Error())
			naples.logger.Error(msg)
		}
		naplesConfig.NaplesIpAddress = naplesIPddress
		naples.logger.Printf("Naples Mgmt IP is : %v", naplesIPddress)

		naplesMgmtIntf, err := naples.getNaplesMgmtInterface(nodOSMap[nodeOs], naplesConfig.GetNicType(), naplesConfig.GetNicHint())
		if err != nil {
			msg := fmt.Sprintf("Error in reading naples mgmt IP address : %s", err.Error())
			naples.logger.Error(msg)
		}
		naples.logger.Printf("Naples Mgmt intf is : %v", naplesMgmtIntf)

		if naplesMgmtIntf != "" {
			intfIP := incrementIP(naplesIPddress)
			if nodeOs != iota.TestBedNodeOs_TESTBED_NODE_OS_WINDOWS {
				intfIP += "/24"
				cmd := []string{"sudo", "ifconfig", naplesMgmtIntf, intfIP, "up"}
				naples.logger.Infof("Bringing up intf : %v with IP %v with %v",
					naplesMgmtIntf, intfIP, strings.Join(cmd, " "))
				if _, stdout, err := Utils.Run(cmd, 0, false, true, nil); err != nil {
					msg := fmt.Sprintf("Failed to bring interface %s up err : %s", naplesMgmtIntf, stdout)
					naples.logger.Error(msg)
					//return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
				}
			} else {
				ifName := naples.windowsPortNameMapping[naplesMgmtIntf]["Name"]
				naples.logger.Infof("Bringing up intf : %v with IP %v", ifName, intfIP)
				cmd := []string{Common.WindowsPowerShell, "Enable-NetAdapter \"" + ifName + "\" -Confirm:$false"}
				if _, stdout, err := Utils.Run(cmd, 0, false, false, nil); err != nil {
					msg := fmt.Sprintf("Failed to bring mgmt interface %s up err : %s", naplesMgmtIntf, stdout)
					naples.logger.Error(msg)
					//return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
				}
				cmd = []string{Common.WindowsPowerShell, "Set-NetIPAddress -InterfaceAlias '" + ifName + "' -IPAddress " + intfIP + " -PrefixLength 24"}
				if _, stdout, err := Utils.Run(cmd, 0, false, false, nil); err != nil {
					msg := fmt.Sprintf("Failed to configure IP for mgmt interface %s err : %s", naplesMgmtIntf, stdout)
					naples.logger.Error(msg)
					//return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
				}
			}
			naplesConfig.HostIntfs = append(naplesConfig.HostIntfs,
				&iota.Interfaces{
					Type:       iota.InterfaceType_INTERFACE_TYPE_MGMT,
					Interfaces: []string{naplesMgmtIntf},
				})
		}
	}

	return nil
}

//Init initialize node type
func (naples *naplesHwNode) Init(in *iota.Node) (*iota.Node, error) {

	naples.init(in)
	naples.iotaNode.name = in.GetName()
	/*if in.GetNaplesConfig() == nil {
		in.NodeInfo = &iota.Node_NaplesConfig{NaplesConfig: &iota.NaplesConfig{}}
	}*/

	//cmd := []string{naplesScript}
	for index, naplesConfig := range in.GetNaplesConfigs().GetConfigs() {
		if in.StartupScript != "" {
			options := ""
			switch naplesConfig.GetNaplesMode() {
			case iota.NaplesOperationMode_NAPLES_MODE_BITW:
				options += " --no-mgmt "
			case iota.NaplesOperationMode_NAPLES_MODE_CLASSIC:
				options += " "
			}
			_, stdout, err := Utils.Run(strings.Split(in.StartupScript, options), 0, false, true, nil)
			if err != nil {
				msg := fmt.Sprintf("Start up script failed %v up err : %v", err, stdout)
				naples.logger.Error(msg)
				// Don't return error as start up would have been completed before.
				//return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
			}
		}

		switch naplesConfig.GetNaplesMode() {
		case iota.NaplesOperationMode_NAPLES_MODE_CLASSIC:

			if naplesConfig.GetNaplesMode() != iota.NaplesOperationMode_NAPLES_MODE_BITW {
				hostIntfs, err := naples.getHostInterfaces(nodOSMap[in.GetOs()], naplesConfig.GetNicType(), naplesConfig.GetNicHint())
				if err != nil {
					naples.logger.Error(err.Error())
					return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: err.Error()}}, err
				}
				naples.logger.Printf("Naples host interfaces : %v", hostIntfs)
				naplesConfig.HostIntfs = append(naplesConfig.HostIntfs,
					&iota.Interfaces{
						Type:       iota.InterfaceType_INTERFACE_TYPE_NATIVE,
						Interfaces: hostIntfs,
					})

				// Gather virtual-functions
				virtualFuncs, err := naples.getVirtualFunctions(nodOSMap[in.GetOs()], naplesConfig.GetNicType(), naplesConfig.GetNicHint())
				if err == nil && len(virtualFuncs) > 0 {
					naplesConfig.HostIntfs = append(naplesConfig.HostIntfs,
						&iota.Interfaces{
							Type:       iota.InterfaceType_INTERFACE_TYPE_SRIOV,
							Interfaces: virtualFuncs,
						})
				} else if err != nil {
					naples.logger.Error(err.Error())
				}
			}
		default:
			naples.logger.Printf("Skipping host interface detection on naples")
		}
		if in.GetOs() == iota.TestBedNodeOs_TESTBED_NODE_OS_WINDOWS && naples.windowsPortNameMapping == nil {
			f, err := os.Open(Common.WindowsPortMappingFile)
			if err != nil {
				return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: err.Error()}}, err
			}
			err = json.NewDecoder(f).Decode(&naples.windowsPortNameMapping)
			f.Close()
			if err != nil {
				return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: err.Error()}}, err
			}
		}

		if err := naples.initNaplesMgmtInterface(in.GetOs(), naplesConfig); err != nil {
			naples.logger.Error("Failed to initialize mgmt interface")
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR}}, err
		}

		for _, naplesHostIfs := range naplesConfig.HostIntfs {
			if naplesHostIfs.Type == iota.InterfaceType_INTERFACE_TYPE_NATIVE {
				for _, intf := range naplesHostIfs.Interfaces {
					var cmd []string
					naples.logger.Info("Bringing up host intf " + intf)
					enableShell := true

					if in.GetOs() != iota.TestBedNodeOs_TESTBED_NODE_OS_WINDOWS {
						cmd = append(cmd, "ifconfig", intf, "up")
					} else {
						enableShell = false
						ifName := naples.windowsPortNameMapping[intf]["Name"]
						cmd = []string{Common.WindowsPowerShell, "Enable-NetAdapter \"" + ifName + "\" -Confirm:$false"}
					}
					if _, stdout, err := Utils.Run(cmd, 0, false, enableShell, nil); err != nil {
						msg := fmt.Sprintf("Failed to bring host interface %s up err : %s", intf, stdout)
						naples.logger.Error(msg)
						return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
					}
				}
			}
		}

		// enable k8s node
		if naplesConfig.KubeInfo != nil {
			// run k8s installation script
			cmd := []string{"sh", "-c", "/pensando/iota/kube-install.sh"}
			if stderr, stdout, err := Utils.Run(cmd, 0, false, true, nil); err != nil {
				msg := fmt.Sprintf("Failed to install kube package: %v/%v", stdout, stderr)
				naples.logger.Error(msg)
				return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
			}

			confFile := "/etc/kubernetes/join.conf"
			// update config file with master IP & token
			cmd = []string{"sed", "-i", "s/kube-master-address/" + naplesConfig.KubeInfo.K8SMasterIp + "/g", confFile}
			if stderr, stdout, err := Utils.Run(cmd, 0, false, false, nil); err != nil {
				msg := fmt.Sprintf("Failed to update config file %s : %v\n%v", naplesConfig.KubeInfo.K8SMasterIp, stdout, stderr)
				naples.logger.Error(msg)
				return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
			}

			cmd = []string{"sed", "-i", "s/kube-master-token/" + naplesConfig.KubeInfo.K8SMasterToken + "/g", confFile}
			if stderr, stdout, err := Utils.Run(cmd, 0, false, false, nil); err != nil {
				msg := fmt.Sprintf("Failed to update config file %s : %v\n%v", naplesConfig.KubeInfo.K8SMasterIp, stdout, stderr)
				naples.logger.Error(msg)
				return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
			}

			cmd = []string{"sed", "-i", "s/node-name/" + in.GetName() + "/g", confFile}
			if stderr, stdout, err := Utils.Run(cmd, 0, false, false, nil); err != nil {
				msg := fmt.Sprintf("Failed to update config file %s : %v\n%v", naplesConfig.KubeInfo.K8SMasterIp, stdout, stderr)
				naples.logger.Error(msg)
				return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
			}

			// check etc/hosts exists, centos doesn't have it but k8s needs it
			if _, err := os.Stat("/etc/hosts"); err != nil {
				// create this file
				f, err2 := os.Create("/etc/hosts")
				if err2 != nil {
					msg := fmt.Sprintf("Failed to create /etc/hosts : %v", err2)
					naples.logger.Printf("Failed to create /etc/hosts: %v", err2.Error())
					return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err2
				}
				output := fmt.Sprintf("%s %s\n", in.IpAddress, in.GetName())
				_, err2 = f.WriteString(output)
				if err2 != nil {
					msg := fmt.Sprintf("Failed to write /etc/hosts : %v", err2)
					naples.logger.Printf("Failed to write to /etc/hosts: %v", err2.Error())
					return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err2
				}
				f.Sync()
				f.Close()
			}

			cmd = []string{"kubeadm", "join", "--config", confFile, "--ignore-preflight-errors=FileAvailable--etc-kubernetes-pki-ca.crt,Service-Docker"}
			if stderr, stdout, err := Utils.Run(cmd, 0, false, false, nil); err != nil {
				msg := fmt.Sprintf("Failed to join k8s master %s : %v\n%v", naplesConfig.KubeInfo.K8SMasterIp, stdout, stderr)
				naples.logger.Error(msg)
				return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
			}
		}

		/* Finally add entity type */
		if err := naples.addNodeEntities(in); err != nil {
			msg := fmt.Sprintf("Adding node entities failed : %s", err.Error())
			naples.logger.Error(msg)
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
		}

		if index < len(naples.naplesEntityKey) {
			nodeUUID, err := naples.getHwUUID(naples.naplesEntityKey[index])
			if err != nil {
				msg := fmt.Sprintf("Error in reading naples hw uuid : %s", err.Error())
				naples.logger.Error(msg)
				//Don't return error, let Harness figure out.
				//return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
			}
			naplesConfig.NodeUuid = nodeUUID
		}
	}
	return &iota.Node{NodeStatus: apiSuccess,
		Name: in.GetName(), IpAddress: in.GetIpAddress(), Type: in.GetType(),
		NodeInfo: &iota.Node_NaplesConfigs{NaplesConfigs: in.GetNaplesConfigs()}}, nil
}

// AddWorkload brings up a workload type on a given node
func (naples *naplesHwNode) AddWorkloads(in *iota.WorkloadMsg) (*iota.WorkloadMsg, error) {

	resp, err := naples.dataNode.AddWorkloads(in)
	if err != nil || resp.ApiResponse.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		return resp, nil
	}

	for _, wload := range in.Workloads {
		if wload.WorkloadStatus.ApiStatus != iota.APIResponseType_API_STATUS_OK {
			in.ApiResponse = wload.WorkloadStatus
			return in, errors.New(wload.WorkloadStatus.ErrorMsg)
		}
	}

	return in, nil

}

//NodeType return node type
func (naplesHwNode) NodeType() iota.PersonalityType {
	return iota.PersonalityType_PERSONALITY_NAPLES
}

//NodeType return node type
func (naples *naplesQemuNode) NodeType() iota.PersonalityType {
	return iota.PersonalityType_PERSONALITY_NAPLES_SIM_WITH_QEMU
}

func (dnode *dataNode) addNodeEntities(in *iota.Node) error {
	for _, entityEntry := range in.GetEntities() {
		var wload Workload.Workload
		if entityEntry.GetType() == iota.EntityType_ENTITY_TYPE_HOST {
			wload = Workload.NewWorkload(Workload.WorkloadTypeBareMetal, entityEntry.GetName(), dnode.name, dnode.logger)
			dnode.hostEntityKey = entityEntry.GetName()
			if err := wload.BringUp(); err != nil {
				dnode.logger.Errorf("Host Hw entity type add failed %v", err.Error())
			}
			wDir := Common.DstIotaEntitiesDir + "/" + entityEntry.GetName()
			wload.SetBaseDir(wDir)
			dnode.entityMap.Store(entityEntry.GetName(), iotaWorkload{workload: wload})
			continue
		}

		wDir := Common.DstIotaEntitiesDir + "/" + entityEntry.GetName()
		wload.SetBaseDir(wDir)

		for _, naplesCfg := range in.GetNaplesConfigs().GetConfigs() {
			if err := wload.BringUp(naplesCfg.GetNaplesIpAddress(),
				strconv.Itoa(sshPort), naplesCfg.GetNaplesUsername(), naplesCfg.GetNaplesPassword()); err != nil {
				dnode.logger.Errorf("Naples Hw entity type add failed %v", err.Error())
				//Ignore error as runner might copy the pub key to remote entity later.
				//return err
			}
			if wload != nil {
				dnode.entityMap.Store(entityEntry.GetName(), iotaWorkload{workload: wload})
			}
		}
	}
	time.Sleep(1 * time.Second)
	return nil
}

//Init initialize node type
func (dnode *dataNode) Init(in *iota.Node) (resp *iota.Node, err error) {
	return nil, nil
}

// EntitiesHealthy checks workloads healthy
func (dnode *dataNode) EntitiesHealthy() error {
	var err error
	dnode.entityMap.Range(func(key interface{}, item interface{}) bool {
		wl := item.(iotaWorkload)
		if !wl.workload.IsHealthy() {
			dnode.logger.Printf("Workload :%s down", wl.workload.Name())
			err = errors.Errorf("Workload %s down", wl.workload.Name())
			return false
		}
		return true
	})
	return err
}

// CheckHealth returns the node health
func (dnode *dataNode) CheckHealth(in *iota.NodeHealth) (*iota.NodeHealth, error) {
	if err := dnode.EntitiesHealthy(); err != nil {
		return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_APP_DOWN}, nil
	}

	dnode.logger.Println("Node healthy")
	return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_HEALTH_OK}, nil
}

//NodeType return node type
func (dnode *dataNode) NodeType() iota.PersonalityType {
	return iota.PersonalityType_PERSONALITY_NONE
}

func (dnode *dataNode) getInterfaceFindCommand(osType string, nicType string) (string, error) {
	var data interface{}
	var ok bool
	file, e := ioutil.ReadFile(Common.DstNicFinderConf)
	if e != nil {
		return "", errors.New("Error opening Nic finder config file")
	}
	var deviceYAML map[string]interface{}
	res := make(map[string]interface{})
	if err := yaml.Unmarshal(file, &deviceYAML); err != nil {
		return "", errors.New("Failed to parse  Nic finder config file")
	}

	if data, ok = deviceYAML[osType]; !ok {
		return "", errors.New("Could not find OS type in nic finder config file")
	}
	for k, v := range data.(map[interface{}]interface{}) {
		res[fmt.Sprintf("%v", k)] = v
	}

	if data, ok = res[nicType]; !ok {
		msg := fmt.Sprintf("Could not find type %s finder config file", nicType)
		return "", errors.New(msg)
	}

	return data.(string), nil
}

func (dnode *dataNode) getHostInterfaces(osType string, nicType, hint string) ([]string, error) {
	var hostIntfs []string
	cmd, err := dnode.getInterfaceFindCommand(osType, nicType)

	if err != nil {
		return nil, err
	}

	fullCmd := []string{cmd}
	_, stdout, err := Utils.RunCmd(fullCmd, 0, false, true, nil)
	if err != nil {
		msg := fmt.Sprintf("Failed to run command to discover interfaces %v", cmd)
		return nil, errors.New(msg)
	}
	dnode.logger.Printf("Host interfaces cmd : %v", cmd)
	dnode.logger.Printf("Host interfaces output : %v", strings.Split(stdout, "\n"))

	for _, intf := range strings.Split(stdout, "\n") {
		if intf != "" {
			hostIntfs = append(hostIntfs, intf)
		}

	}
	return hostIntfs, nil
}

//Init initalize node type
func (thirdParty *thirdPartyDataNode) Init(in *iota.Node) (resp *iota.Node, err error) {

	thirdParty.init(in)
	thirdParty.iotaNode.name = in.GetName()

	if in.GetThirdPartyNicConfig() == nil {
		msg := fmt.Sprintf("Third Party config not specified")
		thirdParty.logger.Errorf(msg)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, errors.New(msg)
	}

	hostIntfs, err := thirdParty.getHostInterfaces(nodOSMap[in.GetOs()], in.GetThirdPartyNicConfig().GetNicType(), "")
	if err != nil {
		thirdParty.logger.Error(err.Error())
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: err.Error()}}, err

	}
	in.GetThirdPartyNicConfig().HostIntfs = hostIntfs
	thirdParty.logger.Printf("Third Party interfaces : %v", in.GetThirdPartyNicConfig().HostIntfs)

	/* Finally add entity type */
	if err := thirdParty.addNodeEntities(in); err != nil {
		thirdParty.logger.Error("Adding node entities failed")
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR}}, err
	}

	return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_STATUS_OK},
		Name: in.GetName(), IpAddress: in.GetIpAddress(), Type: in.GetType(),
		NodeInfo: &iota.Node_ThirdPartyNicConfig{ThirdPartyNicConfig: in.GetThirdPartyNicConfig()}}, nil
}

func (bondNaples *naplesBondHwNode) Init(in *iota.Node) (resp *iota.Node, err error) {

	bondNaples.logger.Printf("Init naples in bond mode")
	resp, err = bondNaples.naplesHwNode.Init(in)
	if err != nil {
		return resp, err
	}

	// Process bond_config to create bond-interface - and update the response.

	bondConfigs := in.GetBondConfigs().GetConfigs()

	if len(bondConfigs) == 0 {
		msg := fmt.Sprintf("Bond config not specified")
		bondNaples.logger.Errorf(msg)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, errors.New(msg)
	}

	for _, bondConfig := range bondConfigs {
		nicHints := bondConfig.GetNicHints()
		if len(nicHints) == 0 {
			msg := fmt.Sprintf("Naples not specified in bond config")
			bondNaples.logger.Errorf(msg)
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, errors.New(msg)
		}

		var allBondMembers []string
		for _, nicHint := range nicHints {
			hostIntfs, err := bondNaples.getHostInterfaces(nodOSMap[in.GetOs()], "", nicHint)
			if err != nil {
				bondNaples.logger.Error(err.Error())
				return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: err.Error()}}, err
			}
			allBondMembers = append(allBondMembers, hostIntfs...)
		}

		// Create bond interface
		var createCmd []string
		if in.GetOs() != iota.TestBedNodeOs_TESTBED_NODE_OS_WINDOWS {
			createCmd = []string{"echo"}
		} else {
			var winMembers []string
			for _, bondMember := range allBondMembers {
				if winIntf, ok := Workload.WindowsPortNameMapping[bondMember]["Name"]; ok {
					winMembers = append(winMembers, winIntf)
				}
			}
			allMembers := strings.Join(winMembers, "\",\"")
			teamCmd := fmt.Sprintf("'New-NetLbfoTeam -Name \"%s\" -TeamMembers \"%s\" -TeamingMode \"%s\" -LoadBalancingAlgorithm \"%s\" -Confirm:$false'", bondConfig.GetBondInterfaceName(), allMembers, bondConfig.GetTeamingMode(), bondConfig.GetLoadBalancingAlgorithm())
			createCmd = append(createCmd, Common.WindowsPowerShell)
			createCmd = append(createCmd, teamCmd)
			bondNaples.logger.Printf("Creating bond interface %v out of %v", bondConfig.GetBondInterfaceName(), allMembers)

			// Update Windows Port-Name Mapping
			Workload.WindowsPortNameMapping[bondConfig.GetBondInterfaceName()] = make(map[string]string)
			Workload.WindowsPortNameMapping[bondConfig.GetBondInterfaceName()]["Name"] = bondConfig.GetBondInterfaceName()
		}
		if _, stdout, err := Utils.Run(createCmd, 0, false, true, nil); err != nil {
			msg := fmt.Sprintf("Failed to bring bond interface %s up err : %s", bondConfig.GetBondInterfaceName(), stdout)
			bondNaples.logger.Error(msg)
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
		}

		// Disable Dhcp on bond interface
		var dhcpCmd []string
		if in.GetOs() != iota.TestBedNodeOs_TESTBED_NODE_OS_WINDOWS {
			dhcpCmd = []string{"echo"}
		} else {
			// Set-NetIPInterface -InterfaceAlias inb_bond0 -Dhcp Disabled
			dhcpDisableCmd := fmt.Sprintf("'Set-NetIPInterface -InterfaceAlias \"%s\" -Dhcp Disabled'", bondConfig.GetBondInterfaceName())
			dhcpCmd = append(dhcpCmd, Common.WindowsPowerShell)
			dhcpCmd = append(dhcpCmd, dhcpDisableCmd)
			bondNaples.logger.Printf("Disabling DHCP on bond interface %v", bondConfig.GetBondInterfaceName())
		}
		if _, stdout, err := Utils.Run(dhcpCmd, 0, false, true, nil); err != nil {
			msg := fmt.Sprintf("Failed to disable dhcp on bond interface %s up err : %s", bondConfig.GetBondInterfaceName(), stdout)
			bondNaples.logger.Error(msg)
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
		}
	}
	return resp, err
}

func (dnode *dataNode) GetWorkloadMsgs() []*iota.Workload {

	wloadMsgs := []*iota.Workload{}
	dnode.entityMap.Range(func(key interface{}, item interface{}) bool {
		wl := item.(iotaWorkload)
		if wl.workloadMsg == nil {
			wl.workloadMsg = &iota.Workload{WorkloadName: wl.workload.Name()}
		}
		wloadMsgs = append(wloadMsgs, wl.workloadMsg)
		return true
	})

	return wloadMsgs
}

// AddWorkload brings up a workload type on a given node
func (node *commandNode) AddWorkloads(in *iota.WorkloadMsg) (*iota.WorkloadMsg, error) {
	node.logger.Println("Add workload on command not supported.")
	return &iota.WorkloadMsg{ApiResponse: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST}}, nil
}

// DeleteWorkloads deletes a given workloads
func (node *commandNode) DeleteWorkloads(*iota.WorkloadMsg) (*iota.WorkloadMsg, error) {
	node.logger.Println("Delete workload on command not supported.")
	return &iota.WorkloadMsg{ApiResponse: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST}}, nil
}

//Init initalize node type
func (node *commandNode) Init(in *iota.Node) (*iota.Node, error) {
	node.logger.Println("Bring script up successful.")

	node.bgCmds = new(sync.Map)
	node.name = in.GetName()

	dir := Common.DstIotaEntitiesDir + "/" + in.GetName()
	os.Mkdir(dir, 0765)
	os.Chmod(dir, 0777)

	return &iota.Node{Name: in.Name, IpAddress: in.IpAddress, Type: in.GetType(),
		NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_STATUS_OK}}, nil
}

// Trigger invokes the workload's trigger. It could be ping, start client/server etc..
func (node *commandNode) Trigger(in *iota.TriggerMsg) (*iota.TriggerMsg, error) {
	runCommand := func(cmd *iota.Command) error {

		var err error
		var cmdKey string
		var cmdResp *Cmd.CmdCtx

		if in.TriggerOp == iota.TriggerOp_EXEC_CMDS {
			cmdResp, cmdKey, err = node.RunCommand(strings.Split(cmd.GetCommand(), " "),
				cmd.GetRunningDir(), cmd.GetRetriesOnFailure(), cmd.GetForegroundTimeout(),
				cmd.GetMode() == iota.CommandMode_COMMAND_BACKGROUND, true)

		} else if in.TriggerOp == iota.TriggerOp_WAIT_FOR_CMDS {
			cmdResp, err = node.WaitBgCommand(cmd.Handle)
		} else {
			cmdResp, err = node.StopCommand(cmd.Handle)
			cmdKey = cmd.Handle
		}

		if err == nil {
			cmd.ExitCode, cmd.Stdout, cmd.Stderr, cmd.Handle, cmd.TimedOut = cmdResp.ExitCode, cmdResp.Stdout, cmdResp.Stderr, cmdKey, cmdResp.TimedOut
			node.logger.Println("Command error :", err)
			node.logger.Println("Command exit code :", cmd.ExitCode)
			node.logger.Println("Command timed out :", cmd.TimedOut)
			node.logger.Println("Command handle  :", cmd.Handle)
			node.logger.Println("Command stdout :", cmd.Stdout)
			node.logger.Println("Command stderr:", cmd.Stderr)

			if cmd.StderrOnErr && cmd.ExitCode == 0 {
				cmd.Stderr = ""
			}

			if cmd.StdoutOnErr && cmd.ExitCode == 0 {
				cmd.Stdout = ""
			}

			fixUtf := func(r rune) rune {
				if r == utf8.RuneError {
					return -1
				}
				return r
			}
			cmd.Stdout = strings.Map(fixUtf, cmd.Stdout)
			cmd.Stderr = strings.Map(fixUtf, cmd.Stderr)

		} else {
			cmd.ExitCode = 1
			cmd.Stderr = fmt.Sprintf("Error running command %v", err.Error())
		}

		if len(cmd.Stdout) > maxStdoutSize || len(cmd.Stderr) > maxStdoutSize {
			cmd.Stdout = ""
			cmd.Stderr = "Stdout/Stderr output limit Exceeded."
			cmd.ExitCode = 127
		}
		return nil
	}

	if in.GetTriggerOp() == iota.TriggerOp_TERMINATE_ALL_CMDS || in.GetTriggerMode() == iota.TriggerMode_TRIGGER_NODE_PARALLEL {
		maxParallelThreads := 8
		currThreads := 0
		scheduleCmds := []*iota.Command{}
		for cmdIndex, cmd := range in.Commands {
			currThreads++
			scheduleCmds = append(scheduleCmds, cmd)
			if currThreads == maxParallelThreads || cmdIndex+1 == len(in.Commands) {
				pool, _ := errgroup.WithContext(context.Background())
				for thread, cmd := range scheduleCmds {
					cmd := cmd
					node.logger.Println("Started thread :", thread)
					pool.Go(func() error {
						runCommand(cmd)
						return nil
					})
				}
				pool.Wait()
				scheduleCmds = []*iota.Command{}
				currThreads = 0
			}

		}

	} else {
		for _, cmd := range in.Commands {
			runCommand(cmd)
		}
	}

	node.logger.Println("Completed running trigger.")
	in.ApiResponse = apiSuccess
	return in, nil

}

// CheckHealth returns the node health
func (node *commandNode) CheckHealth(in *iota.NodeHealth) (*iota.NodeHealth, error) {
	return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_HEALTH_OK}, nil
}

//NodeType return node type
func (node *commandNode) NodeType() iota.PersonalityType {
	return iota.PersonalityType_PERSONALITY_COMMAND_NODE
}

//GetMsg node msg
func (node *commandNode) GetMsg() *iota.Node {
	return node.nodeMsg
}

//GetWorkloadMsgs get workloads
func (node *commandNode) GetWorkloadMsgs() []*iota.Workload {
	return nil
}

// RunCommand runs a command on node nodes
func (node *commandNode) RunCommand(cmd []string, dir string, retries, timeout uint32, background bool, shell bool) (*Cmd.CmdCtx, string, error) {
	handleKey := ""

	runDir := Common.DstIotaEntitiesDir + "/" + node.name
	if dir != "" {
		runDir = runDir + "/" + dir
	}

	node.logger.Println("base dir ", runDir, dir)

	node.logger.Println("Running cmd ", strings.Join(cmd, " "))
	var cmdInfo *Cmd.CmdInfo

	for i := (uint32)(0); i <= retries; i++ {
		cmdInfo, _ = Cmd.ExecCmd(cmd, runDir, (int)(timeout), background, shell, nil)
		if cmdInfo.Ctx.ExitCode == 0 {
			break
		}
		node.logger.Info("Command failed, retrying")
	}

	if background {
		handleKey = fmt.Sprintf("node-bg-cmd-%v", node.bgCmdIndex)
		node.bgCmds.Store(handleKey, cmdInfo)
		atomic.AddUint64(&node.bgCmdIndex, 1)
		//Give it a second for bg command to be scheduled.
		time.Sleep(1 * time.Second)
	}

	return cmdInfo.Ctx, handleKey, nil
}

// StopCommand stops a running command
func (node *commandNode) StopCommand(commandHandle string) (*Cmd.CmdCtx, error) {
	item, ok := node.bgCmds.Load(commandHandle)
	if !ok {
		return &Cmd.CmdCtx{ExitCode: -1, Stdout: "", Stderr: "", Done: true}, nil
	}

	cmdInfo := item.(*Cmd.CmdInfo)

	node.logger.Printf("Stopping bare metal Running cmd %v %v\n", cmdInfo.Ctx.Stdout, cmdInfo.Handle)

	Cmd.StopExecCmd(cmdInfo)
	node.bgCmds.Delete(commandHandle)

	return cmdInfo.Ctx, nil
}

// WaitBgCommand waits until bg command is complete
func (node *commandNode) WaitBgCommand(commandHandle string) (*Cmd.CmdCtx, error) {
	item, ok := node.bgCmds.Load(commandHandle)
	if !ok {
		return &Cmd.CmdCtx{ExitCode: -1, Stdout: "", Stderr: "", Done: true}, nil
	}

	cmdInfo := item.(*Cmd.CmdInfo)

	node.logger.Printf("Waiting bare metal Running cmd %v %v\n", cmdInfo.Ctx.Stdout, cmdInfo.Handle)

	Cmd.WaitForExecCmd(cmdInfo)
	node.bgCmds.Delete(commandHandle)

	return cmdInfo.Ctx, nil
}

func incrementMacAddress(mac string, offset int) (string, error) {
	macAddr, err := net.ParseMAC(mac)
	if err != nil {
		return "", errors.New("Mac Address Parse error")
	}
	for i := len(macAddr) - 1; i > 1; i-- {
		if ((int)(macAddr[i]))+offset < 255 {
			macAddr[i] += (byte)(offset)
			break
		} else {
			macAddr[i] = 1
			macAddr[i-1] += (byte)(offset)
			break
		}
	}
	return macAddr.String(), nil
}

func (naples *naplesControlSimNode) bringUpNaples(name string, macAddress string,
	image string, controlIntf string, veniceIPs []string) (string, error) {

	maxAttempts := 3
	var err error
	controlIntfAlias := "eth1"

	naplesHome := Common.DstIotaAgentDir + "/" + name
	naplesData := naplesHome + "/" + "data"

	os.RemoveAll(naplesHome)
	os.RemoveAll(naplesData)

	os.Mkdir(naplesHome, 0755)
	os.Mkdir(naplesData, 0755)

	env := []string{"NAPLES_IMAGE_DIR=" + Common.ImageArtificatsDirectory, "NAPLES_HOME=" + naplesHome, "NAPLES_DATA_DIR=" + naplesData}
	cmd := []string{"sudo", "-E", "python", Common.ImageArtificatsDirectory + "/" + naplesVMBringUpScript,
		"--name", name, "--sysuuid", macAddress}

	cmd = append(cmd, "--disable-portmap")

	//Disable datapath for naples sim scale
	cmd = append(cmd, "--disable-datapath")

	//Skip image load
	cmd = append(cmd, "--skip-image-load")

	cmd = append(cmd, "--control-intf")
	cmd = append(cmd, controlIntf)
	cmd = append(cmd, "--control-intf-alias")
	cmd = append(cmd, controlIntfAlias)

	naples.logger.Println("Bringing up naples : ", name)

	i := 0
	for true {
		if retCode, stdout, err := Utils.Run(cmd, nodeAddTimeout, false, false, env); err != nil || retCode != 0 {
			naples.logger.Println(stdout)
			msg := "Naples bring script up failed." + stdout
			naples.logger.Error(msg)
			if i+1 <= maxAttempts {
				i++
				continue
			}
			return "", errors.Wrap(err, msg)
		}
		break
	}

	naples.logger.Println("Naples bring script up succesfull.")

	time.Sleep(5 * time.Second)

	if _, err = Utils.GetContainer(name, "", name, "", Workload.ContainerPrivileged, nil); err != nil {
		return "", errors.Wrap(err, "Naples sim not running!")
	}

	naples.logger.Println("Successfull naples bring up : ", name)

	wload := Workload.NewWorkload(Workload.WorkloadTypeContainer, name, name, naples.logger)

	wDir := Common.DstIotaEntitiesDir + "/" + name
	wload.SetBaseDir(wDir)
	if err := wload.BringUp(name, ""); err != nil {
		naples.logger.Errorf("Naples sim entity type add failed")
		return "", err
	}
	naples.dataNode.entityMap.Store(name, iotaWorkload{workload: wload, name: name})

	getIPAddr := func() (string, error) {
		cmd = []string{"ip", "-o", "-f", "inet", "addr", "show", "|", "grep", "eth1", "|", "grep", "-v",
			"secondary", "|", "awk", "'/scope global/ {print $4}'"}

		cmdResp, _, rerr := wload.RunCommand(cmd, "", 0, 0, false, false)
		if rerr != nil || cmdResp.ExitCode != 0 {
			msg := fmt.Sprintf("error getting the ip address of the interface %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
			naples.logger.Println(msg)
			return "", errors.New(msg)
		}
		return strings.TrimSuffix(strings.Split(cmdResp.Stdout, " ")[0], "\r\n"), nil

	}

	getGWIP := func() (string, error) {
		cmd = []string{"ip", "route", "|", "grep", "default", "|", "awk", "'/default/ {print $3}'"}

		cmdResp, _, rerr := wload.RunCommand(cmd, "", 0, 0, false, false)
		if rerr != nil || cmdResp.ExitCode != 0 {
			msg := fmt.Sprintf("error getting the default gw of the interface %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
			naples.logger.Println(msg)
			return "", errors.New(msg)
		}
		return strings.TrimSuffix(strings.Split(cmdResp.Stdout, " ")[0], "\r\n"), nil

	}

	modeSwitch := func(mac, mgmtIP, gw string) error {
		cmd = []string{"NAPLES_URL=http://localhost LD_LIBRARY_PATH=/naples/nic/lib64", "/naples/nic/bin/penctl", "update", "naples",
			"--managed-by", "network", "--management-network", "oob", "--primary-mac", mac}

		if len(veniceIPs) != 0 {
			cmd = append(cmd, "--controllers")
			cmd = append(cmd, strings.Join(veniceIPs, ","))
		}

		if gw != "" {
			cmd = append(cmd, "--default-gw")
			cmd = append(cmd, gw)

		}

		cmd = append(cmd, "--mgmt-ip")
		cmd = append(cmd, mgmtIP)
		cmd = append(cmd, "--id")
		cmd = append(cmd, name)
		cmdResp, _, rerr := wload.RunCommand(cmd, "", 0, 0, false, false)
		if rerr != nil || cmdResp.ExitCode != 0 || cmdResp.Stderr != "" {
			msg := fmt.Sprintf("Error running mode switch command on %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
			naples.logger.Println(msg)
			return errors.New(msg)
		}

		naples.logger.Println("Mode switch complete on : ", name)
		//Delete default route of docker.
		cmd = []string{"route", "delete", "default"}
		cmdResp, _, rerr = wload.RunCommand(cmd, "", 0, 0, false, false)
		if rerr != nil || cmdResp.ExitCode != 0 || cmdResp.Stderr != "" {
			msg := fmt.Sprintf("Error running delete default route command on %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
			naples.logger.Println(msg)
			return errors.New(msg)
		}
		cmd = []string{"route", "add", "default", "gw", gw}
		cmdResp, _, rerr = wload.RunCommand(cmd, "", 0, 0, false, false)
		if rerr != nil || cmdResp.ExitCode != 0 || cmdResp.Stderr != "" {
			msg := fmt.Sprintf("Error running default route command on %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
			naples.logger.Println(msg)
			return errors.New(msg)
		}
		return nil
	}

	for i := 0; i < 6; i++ {
		ipAddress, err := getIPAddr()
		if err != nil {
			return "", err
		}

		gwIP, err := getGWIP()
		if err != nil {
			return "", err
		}

		err = modeSwitch(macAddress, ipAddress, gwIP)
		if err == nil {
			return ipAddress, nil
		}
		time.Sleep(30 * time.Second)
	}

	return "", errors.New("Mode Switch failed")
}

type simNetworkSpec struct {
	dockerNW string
	ipaddr   string
	gw       string
	parent   string
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

func (naples *naplesMultiSimNode) bringUpNaples(index uint32, name string, macAddress string,
	image string, nwSpec simNetworkSpec, veniceIPs []string, defaultGW string, reload bool) (string, error) {

	maxAttempts := 3
	//var naplesContainer *Utils.Container
	var err error

	naplesHome := Common.DstIotaAgentDir + "/" + name
	naplesData := naplesHome + "/" + "data"

	os.RemoveAll(naplesHome)
	os.RemoveAll(naplesData)

	os.Mkdir(naplesHome, 0755)
	os.Mkdir(naplesData, 0755)

	env := []string{"NAPLES_IMAGE_DIR=" + Common.ImageArtificatsDirectory, "NAPLES_HOME=" + naplesHome, "NAPLES_DATA_DIR=" + naplesData}
	cmd := []string{"sudo", "-E", "python", Common.ImageArtificatsDirectory + "/" + naplesVMBringUpScript,
		"--name", name, "--sysuuid", macAddress}

	cmd = append(cmd, "--disable-portmap")

	//Disable datapath for naples sim scale
	cmd = append(cmd, "--disable-datapath")

	//Skip image load
	cmd = append(cmd, "--skip-image-load")

	naples.logger.Println("Bringing up naples : ", name)

	i := 0
	for true {
		if retCode, stdout, err := Utils.Run(cmd, nodeAddTimeout, false, false, env); err != nil || retCode != 0 {
			naples.logger.Println(stdout)
			msg := "Naples bring script up failed."
			naples.logger.Error(msg)
			if i+1 <= maxAttempts {
				i++
				continue
			}
			return "", errors.Wrap(err, msg)
		}
		break
	}

	naples.logger.Println("Naples bring script up succesfull.")

	time.Sleep(5 * time.Second)

	if _, err = Utils.GetContainer(name, "", name, "", Workload.ContainerPrivileged, nil); err != nil {
		return "", errors.Wrap(err, "Naples sim not running!")
	}

	naples.logger.Println("Successfull naples bring up : ", name)

	wload := Workload.NewWorkload(Workload.WorkloadTypeContainerMacVlan, name, name, naples.logger)

	wDir := Common.DstIotaEntitiesDir + "/" + name
	wload.SetBaseDir(wDir)
	if err := wload.BringUp(name, ""); err != nil {
		naples.logger.Errorf("Naples sim entity type add failed")
		return "", err
	}

	intf, err := wload.AddInterface(Workload.InterfaceSpec{
		IPV4Address: nwSpec.ipaddr,
		Parent:      nwSpec.parent,
		Name:        "eth1" + "_" + strconv.Itoa(int(index)),
		IntfRename:  "eth1",
	})
	if err != nil {
		naples.logger.Errorf("Failed ot add mac vlan %v %v", intf, err.Error())
		return "", err
	}
	naples.dataNode.entityMap.Store(name, iotaWorkload{workload: wload, name: name})
	/*i = 0
	for true {
		//Try to delete eth1 as it might have been created by retry.
		cmd = []string{"ip", "link", "del", "eth1"}

		wload.RunCommand(cmd, "", 0, 0, false, false)

		if err := Utils.ConnectToDockerNetwork(naplesContainer, nwSpec.dockerNW, nwSpec.ipaddr, nwSpec.gw); err == nil {
			break
		}
		if i+1 <= maxAttempts {
			i++
			time.Sleep(time.Second)
			continue
		}

		return "", errors.Wrap(err, "Failed to connect to docker network")
	}

	/*
		cmd = []string{"ifconfig", "eth1", nwSpec.ipaddr, "up"}
		cmdResp, _, rerr := wload.RunCommand(cmd, "", 0, 0, false, false)
		if rerr != nil || cmdResp.ExitCode != 0 {
			msg := fmt.Sprintf("error setting the ip address of the interface %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
			naples.logger.Println(msg)
			return strings.TrimSuffix(strings.Split(cmdResp.Stdout, " ")[0], "\r\n"), nil
		} */

	cmd = []string{"ip", "route", "delete", "default"}
	cmdResp, _, rerr := wload.RunCommand(cmd, "", 0, 0, false, false)
	if rerr != nil || cmdResp.ExitCode != 0 {
		msg := fmt.Sprintf("error deleting default route %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
		naples.logger.Println(msg)
	}
	cmd = []string{"ip", "route", "add", "default", "via", nwSpec.gw, "dev", "eth1"}
	cmdResp, _, rerr = wload.RunCommand(cmd, "", 0, 0, false, false)
	if rerr != nil || cmdResp.ExitCode != 0 {
		msg := fmt.Sprintf("error setting default route %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
		naples.logger.Println(msg)
		return strings.TrimSuffix(strings.Split(cmdResp.Stdout, " ")[0], "\r\n"), nil
	}

	getIPAddr := func() (string, error) {
		cmd = []string{"ip", "-o", "-f", "inet", "addr", "show", "|", "grep", "eth1", "|", "grep", "-v",
			"secondary", "|", "awk", "'/scope global/ {print $4}'"}

		cmdResp, _, rerr := wload.RunCommand(cmd, "", 0, 0, false, false)
		if rerr != nil || cmdResp.ExitCode != 0 {
			msg := fmt.Sprintf("error getting the ip address of the interface %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
			naples.logger.Println(msg)
			return "", errors.New(msg)
		}
		return strings.TrimSuffix(strings.Split(cmdResp.Stdout, " ")[0], "\r\n"), nil

	}

	modeSwitch := func(mac, mgmtIP, gw string) error {
		cmd = []string{"NAPLES_URL=http://localhost LD_LIBRARY_PATH=/naples/nic/lib64", "/naples/nic/bin/penctl", "update", "naples",
			"--managed-by", "network", "--management-network", "oob", "--primary-mac", mac}

		if len(veniceIPs) != 0 {
			cmd = append(cmd, "--controllers")
			cmd = append(cmd, strings.Join(veniceIPs, ","))
		}

		if defaultGW != "" {
			cmd = append(cmd, "--default-gw")
			cmd = append(cmd, defaultGW)

		}

		cmd = append(cmd, "--mgmt-ip")
		cmd = append(cmd, mgmtIP)
		cmd = append(cmd, "--id")
		cmd = append(cmd, convertToVeniceFormatMac(mac))
		cmdResp, _, rerr := wload.RunCommand(cmd, "", 0, 0, false, false)

		if rerr != nil || cmdResp.ExitCode != 0 || cmdResp.Stderr != "" {
			msg := fmt.Sprintf("Error running mode switch command on %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
			naples.logger.Println(msg)
			if err := naples.checkNaplesModeSwitch(wload); err != nil {
				return errors.New(msg)
			}
			naples.logger.Print("Naples admission success even though penctl failed")
		}

		naples.logger.Println("Mode switch complete on : ", name)
		//Delete default route of docker.
		cmd = []string{"route", "delete", "default"}
		cmdResp, _, rerr = wload.RunCommand(cmd, "", 0, 0, false, false)
		if rerr != nil || cmdResp.ExitCode != 0 || cmdResp.Stderr != "" {
			msg := fmt.Sprintf("Error running delete default route command on %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
			naples.logger.Println(msg)
			return errors.New(msg)
		}
		cmd = []string{"route", "add", "default", "gw", defaultGW}
		cmdResp, _, rerr = wload.RunCommand(cmd, "", 0, 0, false, false)
		if rerr != nil || cmdResp.ExitCode != 0 || cmdResp.Stderr != "" {
			msg := fmt.Sprintf("Error running default route command on %v : %v %v", name, cmdResp.Stdout, cmdResp.Stderr)
			naples.logger.Println(msg)
			//return errors.New(msg)
		}
		return nil
	}

	for i := 0; i < 6; i++ {
		ipAddress, err := getIPAddr()
		if err != nil {
			return "", err
		}

		err = modeSwitch(macAddress, ipAddress, defaultGW)
		if err == nil {
			return ipAddress, nil
		}
		time.Sleep(30 * time.Second)
	}

	return "", errors.New("Mode Switch failed")
}

func (naples *naplesMultiSimNode) addNodeEntities(in *iota.Node) error {
	for _, entityEntry := range in.GetEntities() {
		var wload Workload.Workload
		if entityEntry.GetType() == iota.EntityType_ENTITY_TYPE_HOST {
			wload = Workload.NewWorkload(Workload.WorkloadTypeBareMetal, entityEntry.GetName(), naples.name, naples.logger)
		} else {
			//Ignore rest of the node entities.
			continue
		}

		wDir := Common.DstIotaEntitiesDir + "/" + entityEntry.GetName()
		wload.SetBaseDir(wDir)
		if err := wload.BringUp(in.GetName(), ""); err != nil {
			naples.logger.Errorf("Naples sim entity type add failed")
			return err
		}
		if wload != nil {
			naples.entityMap.Store(entityEntry.GetName(), iotaWorkload{workload: wload, name: entityEntry.GetName()})
		}
	}

	//HACK, add all multisim nodes

	for i := 0; i < int(in.GetNaplesMultiSimConfig().NumInstances); i++ {

		wloadName := in.GetName() + "-sim-" + strconv.Itoa(i)
		wload := Workload.NewWorkload(Workload.WorkloadTypeContainer, wloadName, naples.name, naples.logger)
		wDir := Common.DstIotaEntitiesDir + "/" + wloadName
		wload.SetBaseDir(wDir)

		if err := wload.BringUp(wloadName, ""); err != nil {
			naples.logger.Errorf("Naples sim entity type add failed %v", err)
			return err
		}
		if wload != nil {
			naples.entityMap.Store(wloadName, iotaWorkload{workload: wload, name: wloadName})
		}
	}

	return nil
}

type simInstance struct {
	id         uint32
	name       string
	macAddress string
	ipAddr     string
}

func (dnode *dataNode) getBaseMacAddresss() (string, error) {

	b, err := ioutil.ReadFile(naplesSimBaseMacFile)
	if err != nil {
		intfs, _ := net.Interfaces()
		for _, intf := range intfs {
			if intf.Name != "lo" && intf.Name != "docker0" &&
				!strings.Contains(intf.Name, "veth") {
				macAddress, err := Utils.GetIntfMacAddress(intf.Name)
				if err != nil {
					return "", err
				}
				dnode.logger.Infof("Using mac address from interface %v %v", intf.Name, macAddress)
				//Write to file as mac might change
				err = ioutil.WriteFile(naplesSimBaseMacFile, []byte(macAddress), 0644)
				if err != nil {
					return "", err
				}
				return macAddress, nil
			}
		}
	}
	return string(b), nil
}

func (naples *naplesMultiSimNode) init(in *iota.Node) (resp *iota.Node, err error) {

	var dockerNW string
	naples.dataNode.init(in)

	//Sleep as reload have to wait
	naples.logger.Println("Sleeping, Bring up request received for : " + in.GetName())
	time.Sleep(3 * time.Minute)
	naples.iotaNode.name = in.GetName()
	naples.logger.Println("Bring up request received for : " + in.GetName())

	createDockerNetwork := func() error {
		if err = Utils.DeleteDockerNetworkByName(in.GetName()); err != nil {
			return nil
		}

		parentIntf := in.GetNaplesMultiSimConfig().GetParent()
		if parentIntf == "" {
			hostIntfs, herr := naples.getHostInterfaces(nodOSMap[in.GetOs()], in.GetNaplesMultiSimConfig().GetNicType(), "")
			if herr != nil {
				naples.logger.Error(herr.Error())
				return herr
			}

			if len(hostIntfs) == 0 {
				return errors.New("Could not find host interfaces")
			}
			parentIntf = hostIntfs[0]
		}

		/*
			dockerNW, err = Utils.CreateMacVlanDockerNetwork(in.GetName(), parentIntf, "", "", "")
			if err != nil {
				return err
			} */

		return nil
	}

	untarImage := func() error {

		curDir, _ := os.Getwd()
		defer os.Chdir(curDir)
		if err = os.Chdir(Common.ImageArtificatsDirectory); err != nil {
			return nil
		}
		dir, _ := os.Getwd()
		naples.logger.Println("Untar image : " + dir + "/" + in.GetImage())
		untar := []string{"tar", "-xvzf", in.GetImage(), "&&", "sync"}
		if _, stdout, err := Utils.Run(untar, 0, false, true, nil); err != nil {
			naples.logger.Printf("Untar unsuccessfull %v %v", err.Error(), stdout)
			return err
		}
		naples.logger.Println("Untar successfull")

		//TODO : get image from tgz search
		loadImage := []string{"docker", "load", "--input", "naples-docker-v1.tgz", "&&", "sync"}
		if _, stdout, err := Utils.Run(loadImage, 0, false, true, nil); err != nil {
			naples.logger.Printf("Image load failed : %v %v", err.Error(), stdout)
			return err
		}

		naples.logger.Println("Image load successful")
		fileMax := []string{"echo", "300000", ">", "/proc/sys/fs/file-max"}
		if _, stdout, err := Utils.Run(fileMax, 0, false, true, nil); err != nil {
			naples.logger.Printf("Failed to increase filemax: %v %v", err.Error(), stdout)
			return err
		}

		return nil
	}

	if err = createDockerNetwork(); err != nil {
		resp := "Failed to create docker network : " + err.Error()
		naples.logger.Error(resp)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
	}

	if err = untarImage(); err != nil {
		resp := "Failed to utar : " + err.Error()
		naples.logger.Error(resp)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
	}

	if in.GetNaplesMultiSimConfig() == nil || in.GetNaplesMultiSimConfig().GetNumInstances() == 0 {
		resp := "Insufficient config to spin up multiple sims"
		naples.logger.Error(resp)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err

	}

	var macAddress string
	var ipaddress, mask string
	if in.GetNaplesMultiSimConfig().MacAddressStart == "" {
		macAddress, err = naples.getBaseMacAddresss()
		if err != nil {
			resp := "Could get Mac address of interface"
			naples.logger.Error(resp)
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
		}
	} else {
		macAddress = in.GetNaplesMultiSimConfig().MacAddressStart
	}

	splitIPs := strings.Split(in.GetNaplesMultiSimConfig().IpAddrRange, "/")
	if len(splitIPs) != 2 {
		resp := "IP address range not in right format"
		naples.logger.Error(resp)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
	}

	splitNW := strings.Split(in.GetNaplesMultiSimConfig().Network, "/")
	if len(splitNW) != 2 {
		resp := "IP Network range not in right format"
		naples.logger.Error(resp)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
	}

	ipaddress = splitIPs[0]
	mask = splitNW[1]
	pool, _ := errgroup.WithContext(context.Background())
	maxParallelThreads := 50
	currThreads := 0
	scheduleInstances := []*simInstance{}

	simConfig := &iota.Node_NaplesMultiSimConfig{
		NaplesMultiSimConfig: &iota.NaplesMultiSimConfig{
			VeniceIps:    in.GetNaplesMultiSimConfig().VeniceIps,
			NumInstances: in.GetNaplesMultiSimConfig().NumInstances,
			IpAddrRange:  in.GetNaplesMultiSimConfig().GetIpAddrRange(),
			NicType:      in.GetNaplesMultiSimConfig().NicType,
			Gateway:      in.GetNaplesMultiSimConfig().Gateway}}
	simConfig.NaplesMultiSimConfig.SimsInfo = []*iota.NaplesSimInfo{}
	for i := 0; i < (int)(in.GetNaplesMultiSimConfig().GetNumInstances()); i++ {
		currThreads++
		macAddress, err = incrementMacAddress(macAddress, 1)
		if err != nil {
			resp := "Could not increment  Mac address of interface"
			naples.logger.Error(resp)
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
		}
		ipaddress, err = Utils.IncrementIP(ipaddress, in.GetNaplesMultiSimConfig().GetIpAddrRange(), 1)
		if err != nil {
			resp := "Could not increment IP address "
			naples.logger.Error(resp)
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
		}
		scheduleInstances = append(scheduleInstances, &simInstance{id: (uint32)(i), name: in.GetName() + "-sim-" + strconv.Itoa(i),
			macAddress: macAddress, ipAddr: ipaddress + "/" + mask})

		if currThreads == maxParallelThreads-1 || i+1 == (int)(in.GetNaplesMultiSimConfig().GetNumInstances()) {
			for _, instance := range scheduleInstances {
				instance := instance
				simInfo := &iota.NaplesSimInfo{Name: instance.name, NodeUuid: instance.macAddress}
				simConfig.NaplesMultiSimConfig.SimsInfo = append(simConfig.NaplesMultiSimConfig.SimsInfo,
					simInfo)
				pool.Go(func() error {
					simInfo := simInfo
					ip, err := naples.bringUpNaples(instance.id, instance.name, instance.macAddress,
						in.GetImage(), simNetworkSpec{dockerNW: dockerNW, parent: in.GetNaplesMultiSimConfig().Parent, gw: in.GetNaplesMultiSimConfig().Gateway, ipaddr: instance.ipAddr},
						in.GetNaplesMultiSimConfig().GetVeniceIps(),
						in.GetNaplesMultiSimConfig().Gateway, in.Reload)
					simInfo.IpAddress = ip
					return err
				})
			}
			if err := pool.Wait(); err != nil {
				resp := "Naples bring up failed : " + err.Error()
				naples.logger.Error(resp)
				return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
			}

			scheduleInstances = []*simInstance{}
			currThreads = 0
		}

	}

	if err := naples.addNodeEntities(in); err != nil {
		msg := fmt.Sprintf("Adding node entities failed : %s", err.Error())
		naples.logger.Error(msg)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
	}

	return &iota.Node{NodeStatus: apiSuccess,
		Name: in.GetName(), IpAddress: in.GetIpAddress(), Type: in.GetType(),
		NodeInfo: simConfig}, nil
}

func (naples *naplesMultiSimNode) checkNaplesModeSwitch(wl Workload.Workload) error {

	var err error
	cmd := []string{"curl", naplesStatusURL}
	cmdResp, _, rerr := wl.RunCommand(cmd, "", 0, 0, false, false)
	if rerr != nil || cmdResp.ExitCode != 0 {
		rerr = errors.New("Error getting naples status")
		if cmdResp != nil {
			msg := fmt.Sprintf("Error getting naples status %v", cmdResp.Stderr)
			naples.logger.Println(msg)
		} else {
			msg := fmt.Sprintf("Error getting naples status with error %v", rerr)
			naples.logger.Println(msg)
		}
		return rerr
	}

	var statusJSON map[string]interface{}
	err = json.Unmarshal([]byte(cmdResp.Stdout), &statusJSON)
	if err != nil {
		return errors.Wrap(err, "Error unmarshalling naples  status")
	}

	status, ok := statusJSON["status"].(map[string]interface{})
	if !ok {
		return errors.Wrap(err, "Status not present in Naples status ")
	}

	if controllers, ok := status["controllers"]; !ok || controllers == nil || len(controllers.([]interface{})) == 0 {
		msg := fmt.Sprintf("Naples %v not admitted \n", wl.Name())
		naples.logger.Println(msg)
		return errors.New(msg)
	}

	if state, ok := status["transition-phase"]; !ok || (state != "VENICE_REGISTRATION_DONE" && state != "VENICE_UNREACHABLE") {
		msg := fmt.Sprintf("Naples %v not admitted \n", wl.Name())
		naples.logger.Println(msg)
		return errors.New(msg)
	}

	return nil
}

// Make sure all containers are admitted
func (naples *naplesMultiSimNode) checkAdmitted() error {

	var err error
	naples.logger.Println("Checking naples admission .")

	count := 0
	naples.dataNode.entityMap.Range(func(key interface{}, item interface{}) bool {
		wl := item.(iotaWorkload)
		if wl.workload.Type() == Workload.WorkloadTypeContainer {
			count++
		}
		return true
	})
	waitCh := make(chan error, count)

	naples.dataNode.entityMap.Range(func(key interface{}, item interface{}) bool {
		wl := item.(iotaWorkload)
		if wl.workload.Type() == Workload.WorkloadTypeContainer {
			wl := wl
			go func(wl iotaWorkload) {
				err = naples.checkNaplesModeSwitch(wl.workload)
				waitCh <- nil
			}(wl)

		}
		return true
	})

	for ii := 0; ii < count; ii++ {
		err = <-waitCh
		if err != nil {
			naples.logger.Println(err)
		}
	}
	return err
}

// CheckHealth returns the node health
func (naples *naplesMultiSimNode) CheckHealth(in *iota.NodeHealth) (*iota.NodeHealth, error) {
	/*if err := dnode.EntitiesHealthy(); err != nil {
		return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_APP_DOWN}, nil
	}*/

	if in.GetClusterDone() {
		if err := naples.checkAdmitted(); err != nil {
			return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_NODE_DOWN}, err
		}
	}

	naples.logger.Println("Node healthy")
	return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_HEALTH_OK}, nil
}

func (naples *naplesMultiSimNode) Destroy(in *iota.Node) (*iota.Node, error) {

	wlmsg := &iota.WorkloadMsg{}
	for _, wl := range naples.GetWorkloadMsgs() {
		wlmsg.Workloads = append(wlmsg.Workloads, wl)
	}
	naples.DeleteWorkloads(wlmsg)

	naples.logger.Println("Destroying naples multi-sim personality.")
	return &iota.Node{NodeStatus: apiSuccess}, nil
}

//Init initalize node type
func (naples *naplesMultiSimNode) Init(in *iota.Node) (resp *iota.Node, err error) {
	return naples.init(in)
}

// CheckHealth returns the node health
func (naples *naplesControlSimNode) CheckHealth(in *iota.NodeHealth) (*iota.NodeHealth, error) {
	/*if err := dnode.EntitiesHealthy(); err != nil {
		return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_APP_DOWN}, nil
	}*/
	naples.logger.Println("Node healthy")
	return &iota.NodeHealth{NodeName: in.GetNodeName(), HealthCode: iota.NodeHealth_HEALTH_OK}, nil
}

func (naples *naplesControlSimNode) init(in *iota.Node) (resp *iota.Node, err error) {

	naples.dataNode.init(in)

	//Sleep as reload have to wait
	naples.logger.Println("Sleeping, Bring up request received for : " + in.GetName())
	time.Sleep(1 * time.Minute)
	naples.iotaNode.name = in.GetName()
	naples.logger.Println("Bring up request received for : " + in.GetName())

	untarImage := func() error {

		curDir, _ := os.Getwd()
		defer os.Chdir(curDir)
		if err = os.Chdir(Common.ImageArtificatsDirectory); err != nil {
			return nil
		}
		dir, _ := os.Getwd()
		naples.logger.Println("Untar image : " + dir + "/" + in.GetImage())
		untar := []string{"tar", "-xvzf", in.GetImage(), "&&", "sync"}
		if _, stdout, err := Utils.Run(untar, 0, false, true, nil); err != nil {
			naples.logger.Printf("Untar unsuccessfull %v %v", err.Error(), stdout)
			return err
		}
		naples.logger.Println("Untar successfull")

		//TODO : get image from tgz search
		loadImage := []string{"docker", "load", "--input", "naples-docker-v1.tgz", "&&", "sync"}
		if _, stdout, err := Utils.Run(loadImage, 0, false, true, nil); err != nil {
			naples.logger.Printf("Image load failed : %v %v", err.Error(), stdout)
			return err
		}

		naples.logger.Println("Image load successful")
		fileMax := []string{"echo", "300000", ">", "/proc/sys/fs/file-max"}
		if _, stdout, err := Utils.Run(fileMax, 0, false, true, nil); err != nil {
			naples.logger.Printf("Failed to increase filemax: %v %v", err.Error(), stdout)
			return err
		}

		return nil
	}

	if err = untarImage(); err != nil {
		resp := "Failed to utar : " + err.Error()
		naples.logger.Error(resp)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
	}

	macAddress, err := naples.getBaseMacAddresss()
	if err != nil {
		resp := "Could get Mac address of interface"
		naples.logger.Error(resp)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: resp}}, err
	}

	var ip string
	ip, err = naples.bringUpNaples(in.Name, macAddress,
		in.GetImage(), in.GetNaplesControlSimConfig().ControlIntf, in.GetNaplesControlSimConfig().GetVeniceIps())

	if err != nil {
		msg := fmt.Sprintf("Bring up naples failed : %s", err.Error())
		naples.logger.Error(msg)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
	}

	if err := naples.addNodeEntities(in); err != nil {
		msg := fmt.Sprintf("Adding node entities failed : %s", err.Error())
		naples.logger.Error(msg)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}, err
	}

	simConfig := &iota.Node_NaplesControlSimConfig{
		NaplesControlSimConfig: &iota.NaplesControlSimConfig{
			VeniceIps:   in.GetNaplesControlSimConfig().VeniceIps,
			ControlIp:   ip,
			NodeUuid:    macAddress,
			ControlIntf: "eth1"}}

	return &iota.Node{NodeStatus: apiSuccess,
		Name: in.GetName(), IpAddress: ip, Type: in.GetType(),
		NodeInfo: simConfig}, nil
}

//Init initalize node type
func (naples *naplesControlSimNode) Init(in *iota.Node) (resp *iota.Node, err error) {
	return naples.init(in)
}
