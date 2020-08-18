package testbed

import (
	"context"
	"fmt"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"google.golang.org/grpc/connectivity"

	"github.com/pkg/errors"
	"golang.org/x/crypto/ssh"
	"golang.org/x/sync/errgroup"

	iota "github.com/pensando/sw/iota/protos/gogen"
	common "github.com/pensando/sw/iota/svcs/common"
	constants "github.com/pensando/sw/iota/svcs/common"
	apc "github.com/pensando/sw/iota/svcs/common/apc"
	"github.com/pensando/sw/iota/svcs/common/runner"
	vmware "github.com/pensando/sw/iota/svcs/common/vmware"
	"github.com/pensando/sw/venice/utils/log"
)

const (
	restartTimeout = 1200 //300 seconds for node restart
)

func (n *TestNode) setupAgent() error {

	svcName := n.info.Name
	agentURL, err := n.GetAgentURL()
	if err != nil {
		log.Errorf("TOPO SVC | AddNodes | AddNodes call failed to establish GRPC Connection to Agent running on Node: %v. Err: %v", svcName, err)
		msg := fmt.Sprintf("TOPO SVC | AddNodes | AddNodes call failed to establish GRPC Connection to Agent running on Node: %v. Err: %v", svcName, err)
		log.Error(msg)
		return errors.New(msg)
	}

	c, err := common.CreateNewGRPCClient(svcName, agentURL, common.GrpcMaxMsgSize)
	if err != nil {
		msg := fmt.Sprintf("TOPO SVC | AddNodes | AddNodes call failed to establish GRPC Connection to Agent running on Node: %v. Err: %v", svcName, err)
		log.Error(msg)
		return errors.New(msg)
	}

	n.GrpcClient = c
	n.AgentClient = iota.NewIotaAgentApiClient(c.Client)

	return nil
}

func (n *TestNode) installImage() error {

	if n.info.InstallInfo == nil {
		log.Infof("Skipping install for node %v", n.info.Name)
		return nil
	}

	var sshCfg *ssh.ClientConfig
	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
		sshCfg = InitSSHConfig(constants.EsxDataVMUsername, constants.EsxDataVMPassword)
	} else {
		sshCfg = n.info.SSHCfg
	}

	if len(n.info.InstallInfo.Images) != 0 {
		log.Infof("Copying images for node %v", n.info.Name)
		if err := n.CopyTo(sshCfg, constants.ImageArtificatsDirectory, n.info.InstallInfo.Images); err != nil {
			log.Errorf("TOPO SVC | InitTestBed | Failed to copy common artifacts, to TestNode: %v, at IPAddress: %v", n.GetNodeInfo().Name, n.GetNodeInfo().IPAddress)
			return err
		}
	}

	if n.info.InstallInfo.Node != nil {
		log.Infof("Doing install images for node %v", n.info.Name)
		gopath := os.Getenv("GOPATH")
		wsdir := gopath + "/src/github.com/pensando/sw"
		node := n.info.InstallInfo.Node
		cmd := fmt.Sprintf("%s/iota/scripts/boot_naples_v2.py", wsdir)
		cmd += fmt.Sprintf(" --mnic-ip 169.254.0.1")
		cmd += fmt.Sprintf(" --instance-name %v", node.InstanceName)
		cmd += fmt.Sprintf(" --testbed %v", n.info.InstallInfo.TestbedJsonFile)
		if filepath.Base(n.info.InstallInfo.Images[0]) != "naples_fw.tar" {
			cmd += fmt.Sprintf(" --pipeline apulu --image-build equinix")
		}
		cmd += fmt.Sprintf(" --mode hostpin")
		if node.MgmtIntf != "" {
			cmd += fmt.Sprintf(" --mgmt-intf %v", node.MgmtIntf)
		}
		cmd += fmt.Sprintf(" --uuid %s", node.NicUuid)

		cmd += fmt.Sprintf(" --wsdir %s", wsdir)
		cmd += fmt.Sprintf(" --image-manifest %s/images/latest.json", wsdir)

		if node.NoMgmt {
			cmd += fmt.Sprintf(" --no-mgmt")
		}
		if node.AutoDiscoverOnInstall {
			cmd += fmt.Sprintf(" --auto-discover-on-install")
		}
		if n.info.InstallInfo.OnlyReset {
			cmd += fmt.Sprintf(" --reset")
		}

		command := exec.Command("sh", "-c", cmd)
		log.Infof("Running command: %s", cmd)

		// open the out file for writing
		outfile, err := os.Create(fmt.Sprintf("%s/iota/%s-firmware-upgrade.log", wsdir, node.NodeName))
		if err != nil {
			log.Errorf("Error creating log file. Err: %v", err)
			return err
		}
		defer outfile.Close()
		command.Stdout = outfile
		command.Stderr = outfile
		err = command.Start()
		if err != nil {
			log.Errorf("Error running command %s. Err: %v", cmd, err)
			return err
		}

		if err := command.Wait(); err != nil {
			log.Errorf("Error executing boot_naples_v2.py script. Err: %s", err)
			stdout, _ := exec.Command("sh", "-c", "tail -n 100 *upgrade.log").CombinedOutput()
			fmt.Println(stdout)
			return err
		}

		if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
			if err = n.initEsxNode(); err != nil {
				log.Errorf("TOPO SVC | Init ESX node failed after restart  %v", err.Error())
				return err
			}
		}

		var agentBinary string
		if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_FREEBSD {
			agentBinary = constants.IotaAgentBinaryPathFreebsd
		} else {
			agentBinary = constants.IotaAgentBinaryPathLinux
		}
		if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
			if err := n.initEsxNode(); err != nil {
				log.Errorf("TOPO SVC | Init ESX node failed after restart  %v", err.Error())
				return err
			}
		}
		ip, _ := n.GetNodeIP()
		log.Infof("TOPO SVC | Starting IOTA Agent on TestNode: %v, IPAddress: %v", n.Node.Name, ip)
		sudoAgtCmd := fmt.Sprintf("sudo -E %s", constants.DstIotaAgentBinary)
		if err = n.StartAgent(sudoAgtCmd, sshCfg); err != nil {
			log.Errorf("TOPO SVC Failed to start agent binary: %v, on TestNode: %v, at IPAddress: %v", agentBinary, n.Node.Name, n.Node.IpAddress)
			return err
		}

	}
	return nil
}

// check if agent port is accepting connections
func (n *TestNode) isAgentUp() bool {
	if n.GrpcClient.Client.GetState() == connectivity.Shutdown {
		return false
	}
	return true
}

// attempt to restart agent if not running / crashed
func (n *TestNode) RestartAgent() error {
	var agentBinary string
	var sshCfg *ssh.ClientConfig

	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_FREEBSD {
		agentBinary = constants.IotaAgentBinaryPathFreebsd
	} else {
		agentBinary = constants.IotaAgentBinaryPathLinux
	}

	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
		sshCfg = InitSSHConfig(constants.EsxDataVMUsername, constants.EsxDataVMPassword)
	} else {
		sshCfg = n.info.SSHCfg
	}
	ip, _ := n.GetNodeIP()
	log.Infof("TOPO SVC | RestartAgent | Attempting to restart IOTA agent on TestNode: %v, IPAddress: %v", n.Node.Name, ip)
	sudoAgtCmd := fmt.Sprintf("sudo -E %s", constants.DstIotaAgentBinary)
	if err := n.StartAgent(sudoAgtCmd, sshCfg); err != nil {
		log.Errorf("TOPO SVC | RestartNode | Failed to start agent binary: %v, on TestNode: %v, at IPAddress: %v", agentBinary, n.Node.Name, n.Node.IpAddress)
		return err
	}
	agentURL := fmt.Sprintf("%s:%d", ip, constants.IotaAgentPort)
	c, err := constants.CreateNewGRPCClient(n.Node.Name, agentURL, constants.GrpcMaxMsgSize)
	if err != nil {
		errorMsg := fmt.Sprintf("Could not create GRPC Connection to IOTA Agent. Err: %v", err)
		log.Errorf("TOPO SVC | ReloadNode | ReloadNode call failed to establish GRPC Connection to Agent running on Node: %v. Err: %v", n.Node.Name, errorMsg)
		return err
	}

	n.GrpcClient = c
	oldClient := n.AgentClient
	n.AgentClient = iota.NewIotaAgentApiClient(c.Client)
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Minute)
	defer cancel()
	resp, err := n.AgentClient.ReloadNode(ctx, n.Node)
	log.Infof("TOPO SVC | IpmiNodeControl | IpmiNodeControl Agent . Received Response Msg: %v", resp)
	if err != nil || resp.NodeStatus.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		msg := fmt.Sprintf("Reload node %v failed. Err: %v", n.Node.Name, err)
		if resp.NodeStatus != nil {
			msg = fmt.Sprintf("Reload node %v failed. Err: %v, %v", n.Node.Name, err, resp.NodeStatus.ErrorMsg)
		}
		return fmt.Errorf(msg)
	}

	if n.workloadMap != nil {
		n.workloadMap.Range(func(key interface{}, item interface{}) bool {
			wload := item.(iotaWorkload)
			if wload.workload != nil && wload.workload.GetWorkloadAgent() == oldClient {
				//Return as this is a control vm handle
				wload.workload.SetWorkloadAgent(n.AgentClient)
				return true
			}
			return true
		})
	}
	if !n.isAgentUp() {
		msg := fmt.Sprintf("failed to restart agent")
		return errors.New(msg)
	}
	return nil
}

// AddNode adds a node to the topology
func (n *TestNode) AddNode() error {
	var ip string
	var err error

	if err := n.installImage(); err != nil {
		msg := fmt.Sprintf("Install image on node failed %v : %v", n.info.Name, err)
		log.Error(msg)
		n.RespNode = &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}
		return fmt.Errorf("Install image on node failed %v : %v", n.info.Name, err)
	}

	if err := n.setupAgent(); err != nil {
		msg := fmt.Sprintf("Agent start failed %v : %v", n.info.Name, err)
		log.Error(msg)
		return err
	}

	if ip, err = n.GetNodeIP(); err != nil {
		log.Errorf("TOPO SVC | CopyTo node failed to get node IP")
		return fmt.Errorf("TOPO SVC | CopyTo node failed to get node IP")
	}

	addr := fmt.Sprintf("%s:%d", ip, constants.SSHPort)
	sshclient, err := ssh.Dial("tcp", addr, n.info.SSHCfg)
	if sshclient == nil || err != nil {
		msg := fmt.Sprintf("SSH connect to %v (%v) failed", n.info.Name, n.info.IPAddress)
		log.Error(msg)
		n.RespNode = &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}
		return err
	}

	n.SSHClient = sshclient
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Minute)
	defer cancel()
	resp, err := n.AgentClient.AddNode(ctx, n.Node)
	log.Infof("TOPO SVC | DEBUG | AddNode Agent . Received Response Msg: %v", resp)

	if err != nil {
		msg := fmt.Sprintf("Adding node %v failed. Err: %v", n.info.Name, err)
		log.Errorf(msg)
		n.RespNode = n.Node
		n.RespNode.NodeStatus = &iota.IotaAPIResponse{
			ApiStatus: iota.APIResponseType_API_SERVER_ERROR,
			ErrorMsg:  msg,
		}
		n.SetNodeResponse(n.RespNode)
		return err
	}

	n.RespNode = resp
	if resp.NodeStatus.ApiStatus != iota.APIResponseType_API_STATUS_OK {
		log.Errorf("Adding node %v failed. Agent Returned non ok status: %v", n.info.Name, resp.NodeStatus.ApiStatus)
		return fmt.Errorf("adding node %v failed. Agent Returned non ok status: %v", n.info.Name, resp.NodeStatus.ApiStatus)
	}
	//n.Node.NodeUuid = resp.NodeUuid
	n.SetNodeResponse(resp)
	log.Infof("Node response set for %v", resp.Name)
	return nil
}

func (n *TestNode) waitForNodeUp(timeout time.Duration) error {
	cTimeout := time.After(time.Second * timeout)
	for {
		addr := n.info.IPAddress
		sshCfg := n.info.SSHCfg
		if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
			sshCfg = InitSSHConfig(n.info.Username, n.info.Password)
			addr = n.info.IPAddress
		}

		addr = net.JoinHostPort(addr, "22")
		conn, _ := net.DialTimeout("tcp", addr, 2*time.Second)
		if conn != nil {
			log.Infof("Connected to host : %s", addr)
			conn.Close()
			//Make sure ssh also works
			nrunner := runner.NewRunner(sshCfg)
			command := fmt.Sprintf("date")
			err := nrunner.Run(addr, command, constants.RunCommandForeground)
			if err == nil {
				break
			}
		}
		select {
		case <-cTimeout:
			msg := fmt.Sprintf("Timeout system to be up %s ", addr)
			log.Errorf(msg)
			return errors.New(msg)
		default:
			time.Sleep(100 * time.Millisecond)
		}
	}

	//ESX node will take a little longer
	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
		log.Infof("Waiting for little longer for ESX node to come up..")
		time.Sleep(60 * time.Second)
	} else {
		log.Infof("Waiting for a littel longer for node to come up...")
		time.Sleep(10 * time.Second)
	}

	return nil
}

//GetAgentURL get agents URL
func (n *TestNode) GetAgentURL() (string, error) {

	var ip string
	var err error
	if ip, err = n.GetNodeIP(); err != nil {
		return "", err
	}

	agentURL := fmt.Sprintf("%s:%d", ip, constants.IotaAgentPort)
	return agentURL, nil
}

//GetNodeIP gets node IP
func (n *TestNode) GetNodeIP() (string, error) {

	var ip string
	var err error
	var host *vmware.Host

	if n.info.Os != iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
		ip = n.info.IPAddress
	} else {
		host, err = vmware.NewHost(context.Background(), n.info.IPAddress, n.info.Username, n.info.Password)
		if err != nil {
			log.Errorf("TOPO SVC | GetNodeIP | Failed to  connect to ESX host  %v %v %v %v", err.Error(),
				n.info.IPAddress, n.info.Username, n.info.Password)
			return "", err
		}

		if on, _ := host.PoweredOn(n.ctrlVMName()); !on {
			fmt.Printf("Connecting  failed not on \n", n.info.IPAddress, n.info.Username, n.info.Password)
			log.Errorf("TOPO SVC | GetNodeIP | Control VM not powered on")
			return "", errors.New("Control VM not powered on ")
		}
		ip, err = host.GetVMIP(n.ctrlVMName())
		if err != nil {
			fmt.Printf("Connecting  failed %v\n", n.info.IPAddress, n.info.Username, n.info.Password, err.Error())
			log.Errorf("TOPO SVC | GetNodeIP | Failed to get IP address for control VM  %v", err.Error())
			return "", err
		}
	}

	return ip, nil
}

func (n *TestNode) IpmiNodeCommand(method string, useNcsi bool) error {
	var addr, ip, cimcIp string
	var err error
	var sshCfg *ssh.ClientConfig

	log.Infof("using ipmi command %s on node %s", method, n.Node.Name)
	if n.info.CimcIP == "" {
		log.Errorf("user requested ipmi command but no cimc ip in node object")
		return err
	}
	if useNcsi {
		if n.info.CimcNcsiIp == "" {
			log.Errorf("user requested ncsi, but no ncsi ip specified for node %s", n.Node.Name)
			return err
		}
		cimcIp = n.info.CimcNcsiIp
	} else {
		cimcIp = n.info.CimcIP
	}
	cmd := fmt.Sprintf("ipmitool -I lanplus -H %s -U %s -P %s power %s",
		cimcIp, n.info.CimcUserName, n.info.CimcPassword, method)

	splitCmd := strings.Split(cmd, " ")
	if stdout, err := exec.Command(splitCmd[0], splitCmd[1:]...).CombinedOutput(); err != nil {
		log.Errorf("TOPO SVC | ipmi command %s failed %v", method, stdout)
	} else {
		log.Errorf("TOPO SVC | ipmi command %s success", method)
	}

	if n.GrpcClient != nil {
		n.GrpcClient.Client.Close()
		n.GrpcClient = nil
	}

	time.Sleep(60 * time.Second)

	if err = n.waitForNodeUp(restartTimeout); err != nil {
		return err
	}

	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
		if err = n.initEsxNode(); err != nil {
			log.Errorf("TOPO SVC | IpmiNodeCommand | Init ESX node failed :  %v", err.Error())
			return err
		}
		sshCfg = InitSSHConfig(constants.EsxDataVMUsername, constants.EsxDataVMPassword)
	} else {
		sshCfg = InitSSHConfig(n.info.Username, n.info.Password)
	}

	if ip, err = n.GetNodeIP(); err != nil {
		log.Errorf("TOPO SVC | ipmi control node failed")
		return fmt.Errorf("TOPO SVC | Failed to get Node IP")
	}

	addr = fmt.Sprintf("%s:%d", ip, constants.SSHPort)
	sshclient, err := ssh.Dial("tcp", addr, sshCfg)
	if sshclient == nil || err != nil {
		log.Errorf("SSH connect to %v (%v) failed", n.info.Name, n.info.IPAddress)
		return err
	}

	//Give it some time for node to stabalize
	time.Sleep(5 * time.Second)
	n.SSHClient = sshclient

	return nil
}

func inList(src string, list []string) bool {
	for _, s := range list {
		if src == s {
			return true
		}
	}
	return false
}

func (n *TestNode) restoreLocallyManagedWorkloads() error {

	var err error
	//This is the ugliest hack
	//Some workload might be living in this node
	//Point those workload to this new agent client
	//Also bringup workloads
	if n.workloadMap != nil {
		n.workloadMap.Range(func(key interface{}, item interface{}) bool {
			wload := item.(iotaWorkload)
			if wload.workload != nil && wload.workload.GetWorkloadAgent() == n.AgentClient {
				//Return as this is a control vm handle
				return true
			}

			//Bring up other workloads
			in := wload.workloadMsg
			n.logger.Printf("Re-Setting up workload : %s", in.GetWorkloadName())
			resp, rerr := n.setupWorkload(wload.workload, in)

			if err != nil || resp.GetWorkloadStatus().GetApiStatus() != iota.APIResponseType_API_STATUS_OK {
				logger.Errorf("Tearing down workload %v", wload.workload.Name())
				wload.workload.TearDown()
				if resp.GetWorkloadStatus().GetApiStatus() != iota.APIResponseType_API_STATUS_OK {
					err = fmt.Errorf(resp.GetWorkloadStatus().GetErrorMsg())
				} else {
					err = rerr
				}
			}

			for _, intf := range resp.Interfaces {

				if err = wload.workload.SendArpProbe(strings.Split(intf.GetIpPrefix(), "/")[0], intf.Interface,
					0); err != nil {
					msg := fmt.Sprintf("Error in sending arp probe : %s", err.Error())
					n.logger.Error(msg)
					resp = &iota.Workload{WorkloadStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR, ErrorMsg: msg}}
					n.workloadMap.Delete(in.GetWorkloadName())
					wload.workload.TearDown()
				}
			}

			return true
		})
	}

	return err
}

// ipmi control of node.
func (n *TestNode) IpmiNodeControl(name string, restoreState bool, method string, useNcsi bool) error {
	var agentBinary string
	var err error

	if n.Node == nil {
		n.Node = &iota.Node{}
	}
	var methods = []string{"cycle", "soft", "reset", "off"}
	if inList(method, methods) {
		resp, err := n.AgentClient.SaveNode(context.Background(), n.Node)
		log.Infof("TOPO SVC | DEBUG | SaveNode Agent %v(%v) Received Response Msg: %v",
			n.Node.GetName(), n.Node.IpAddress, resp)
		if err != nil {
			log.Errorf("Saving node %v failed. Err: %v", n.Node.Name, err)
			return err
		}
	}

	if n.GrpcClient != nil {
		n.GrpcClient.Client.Close()
	}
	if err = n.IpmiNodeCommand(method, useNcsi); err != nil {
		log.Errorf("ipmi control of node %v failed. Err: %v", n.Node.Name, err)
		return err
	}

	if method == "off" {
		return nil
	}

	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_FREEBSD {
		agentBinary = constants.IotaAgentBinaryPathFreebsd
	} else {
		agentBinary = constants.IotaAgentBinaryPathLinux
	}

	var sshCfg *ssh.ClientConfig
	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
		sshCfg = InitSSHConfig(constants.EsxDataVMUsername, constants.EsxDataVMPassword)
	} else {
		sshCfg = n.info.SSHCfg
	}
	ip, _ := n.GetNodeIP()
	log.Infof("TOPO SVC | IpmiNodeControl | Starting IOTA Agent on TestNode: %v, IPAddress: %v", n.Node.Name, ip)
	sudoAgtCmd := fmt.Sprintf("sudo -E %s", constants.DstIotaAgentBinary)
	if err = n.StartAgent(sudoAgtCmd, sshCfg); err != nil {
		log.Errorf("TOPO SVC | IpmiNodeControl | Failed to start agent binary: %v, on TestNode: %v, at IPAddress: %v", agentBinary, n.Node.Name, n.Node.IpAddress)
		return err
	}

	agentURL := fmt.Sprintf("%s:%d", ip, constants.IotaAgentPort)
	c, err := constants.CreateNewGRPCClient(n.Node.Name, agentURL, constants.GrpcMaxMsgSize)
	if err != nil {
		errorMsg := fmt.Sprintf("Could not create GRPC Connection to IOTA Agent. Err: %v", err)
		log.Errorf("TOPO SVC | IpmiNodeControl | IpmiNodeControl call failed to establish GRPC Connection to Agent running on Node: %v. Err: %v", n.Node.Name, errorMsg)
		return err
	}

	n.GrpcClient = c
	oldClient := n.AgentClient
	n.AgentClient = iota.NewIotaAgentApiClient(c.Client)
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Minute)
	defer cancel()
	if restoreState {
		resp, err := n.AgentClient.ReloadNode(ctx, n.Node)
		log.Infof("TOPO SVC | IpmiNodeControl | IpmiNodeControl Agent . Received Response Msg: %v", resp)
		if err != nil || resp.NodeStatus.ApiStatus != iota.APIResponseType_API_STATUS_OK {
			msg := fmt.Sprintf("Reload node %v failed. Err: %v", n.Node.Name, err)
			if resp.NodeStatus != nil {
				msg = fmt.Sprintf("Reload node %v failed. Err: %v, %v", n.Node.Name, err, resp.NodeStatus.ErrorMsg)
			}
			return fmt.Errorf(msg)
		}
	}

	if n.workloadMap != nil {
		n.workloadMap.Range(func(key interface{}, item interface{}) bool {
			wload := item.(iotaWorkload)
			if wload.workload != nil && wload.workload.GetWorkloadAgent() == oldClient {
				//Return as this is a control vm handle
				wload.workload.SetWorkloadAgent(n.AgentClient)
				return true
			}
			return true
		})
	}

	return err
}

//RestartNode Restart node
func (n *TestNode) RestartNode(method string, useNcsi bool) error {

	var command string
	var addr, ip, cimcIp string
	var err error
	var sshCfg *ssh.ClientConfig
	var nrunner *runner.Runner

	log.Infof("Restarting node: %v", n.info.Name)

	if useNcsi {
		if n.info.CimcNcsiIp == "" {
			log.Errorf("user requested ncsi, but no ncsi ip specified for node %s", n.Node.Name)
			return err
		}
		cimcIp = n.info.CimcNcsiIp
	} else {
		cimcIp = n.info.CimcIP
	}

	if method == "" || method == "reboot" {
		if err = n.waitForNodeUp(1); err != nil && n.info.CimcIP != "" {
			//Node not up, do a cimc reset
			cmd := fmt.Sprintf("ipmitool -I lanplus -H %s -U %s -P %s power cycle",
				cimcIp, n.info.CimcUserName, n.info.CimcPassword)

			splitCmd := strings.Split(cmd, " ")
			if stdout, err := exec.Command(splitCmd[0], splitCmd[1:]...).CombinedOutput(); err != nil {
				log.Errorf("TOPO SVC | Reset Node Failed %v", string(stdout))
			} else {
				log.Errorf("TOPO SVC | Reset Node Success")
			}
		} else {
			switch n.info.Os {
			case iota.TestBedNodeOs_TESTBED_NODE_OS_ESX:
				//First shutdown control node
				if ip, err = n.GetNodeIP(); err == nil {
					sshCfg = InitSSHConfig(constants.EsxDataVMUsername, constants.EsxDataVMPassword)
					nrunner = runner.NewRunner(sshCfg)
					log.Infof("Restart ctrl vm  %v %v %v",
						n.info.Username, n.info.Password, ip)
					addr = fmt.Sprintf("%s:%d", ip, constants.SSHPort)
					command = fmt.Sprintf("sudo -E sync && sudo -E shutdown -h now")
					nrunner.Run(addr, command, constants.RunCommandForegroundNoShell)
				}
				command = fmt.Sprintf("reboot && sleep 30")
			case iota.TestBedNodeOs_TESTBED_NODE_OS_WINDOWS:
				command = fmt.Sprintf(common.WindowsPowerShell + " Restart-Computer")
			default:
				command = fmt.Sprintf("sudo -E sync && sudo -E shutdown -r now")
			}
			addr = fmt.Sprintf("%s:%d", n.info.IPAddress, constants.SSHPort)
			sshCfg = InitSSHConfig(n.info.Username, n.info.Password)
			log.Infof("Restart Host machine with %v %v %v",
				n.info.Username, n.info.Password, n.info.IPAddress)
			nrunner = runner.NewRunner(sshCfg)
			nrunner.Run(addr, command, constants.RunCommandForegroundNoShell)
		}
	} else if method == "ipmi" {
		log.Infof("restarting node %s using ipmi", n.Node.Name)
		if cimcIp == "" {
			log.Errorf("user requested ipmi reset but no cimc ip in node object")
		} else {
			if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_WINDOWS {
				// windows needs graceful shutdown
				command = fmt.Sprintf(common.WindowsPowerShell + " Restart-Computer")
				addr = fmt.Sprintf("%s:%d", n.info.IPAddress, constants.SSHPort)
				sshCfg = InitSSHConfig(n.info.Username, n.info.Password)
				log.Infof("Shut down windows machine with %v %v %v",
					n.info.Username, n.info.Password, n.info.IPAddress)
				nrunner = runner.NewRunner(sshCfg)
				nrunner.Run(addr, command, constants.RunCommandForegroundNoShell)
				// Dell needs longer than 30s
				time.Sleep(60 * time.Second)
			}
			cmd := fmt.Sprintf("ipmitool -I lanplus -H %s -U %s -P %s power cycle",
				cimcIp, n.info.CimcUserName, n.info.CimcPassword)

			splitCmd := strings.Split(cmd, " ")
			if stdout, err := exec.Command(splitCmd[0], splitCmd[1:]...).CombinedOutput(); err != nil {
				log.Errorf("TOPO SVC | IPMI Power Cycle Node Failed %v", string(stdout))
			} else {
				log.Errorf("TOPO SVC | IPMI Power Cycle Node Success")
			}
		}
	} else if method == "apc" {
		if n.info.ApcInfo == nil {
			log.Errorf("user requested apc power cycle but node %s missing apc info", n.Node.Name)
		} else {
			log.Infof("restarting node %s using apc power cycle(ip:%s, user:%s, pass:%s)", n.Node.Name, n.info.ApcInfo.Ip, n.info.ApcInfo.Username, n.info.ApcInfo.Password)
			if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_WINDOWS {
				// windows needs graceful shutdown
				command = fmt.Sprintf(common.WindowsPowerShell + " Restart-Computer")
				addr = fmt.Sprintf("%s:%d", n.info.IPAddress, constants.SSHPort)
				sshCfg = InitSSHConfig(n.info.Username, n.info.Password)
				log.Infof("Shut down windows machine with %v %v %v",
					n.info.Username, n.info.Password, n.info.IPAddress)
				nrunner = runner.NewRunner(sshCfg)
				nrunner.Run(addr, command, constants.RunCommandForegroundNoShell)
				time.Sleep(30 * time.Second)
			}
			h, err := apc.Dial(n.info.ApcInfo.Ip, n.info.ApcInfo.Username, n.info.ApcInfo.Password, os.Stdout)
			if err != nil {
				log.Errorf("failed to connect to apc %s with username %s and password %s. error was: %v", n.info.ApcInfo.Ip, n.info.ApcInfo.Username, n.info.ApcInfo.Password, err)
			} else {
				defer h.Close()
				if err := h.PowerOff(n.info.ApcInfo.Port); err != nil {
					log.Errorf("failed to power off port %s of apc %s. error was: %v", n.info.ApcInfo.Port, n.info.ApcInfo.Ip, err)
				}
				time.Sleep(30 * time.Second)
				if err := h.PowerOn(n.info.ApcInfo.Port); err != nil {
					log.Errorf("failed to power on port %s of apc %s. error was: %v", n.info.ApcInfo.Port, n.info.ApcInfo.Ip, err)
				}
				time.Sleep(2 * time.Minute)
				cmd := fmt.Sprintf("ipmitool -I lanplus -H %s -U %s -P %s power on",
					n.info.CimcIP, n.info.CimcUserName, n.info.CimcPassword)
				splitCmd := strings.Split(cmd, " ")
				if stdout, err := exec.Command(splitCmd[0], splitCmd[1:]...).CombinedOutput(); err != nil {
					log.Errorf("TOPO SVC | Failed to call ipmi power on: %v", string(stdout))
				} else {
					log.Errorf("TOPO SVC | ipmi power on Success")
				}
			}
		}
	}

	if n.GrpcClient != nil {
		n.GrpcClient.Client.Close()
		n.GrpcClient = nil
	}
	time.Sleep(60 * time.Second)

	if err = n.waitForNodeUp(restartTimeout); err != nil {
		return err
	}

	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
		if err = n.initEsxNode(); err != nil {
			log.Errorf("TOPO SVC | RestartNode | Init ESX node failed :  %v", err.Error())
			return err
		}
		sshCfg = InitSSHConfig(constants.EsxDataVMUsername, constants.EsxDataVMPassword)
	} else {
		sshCfg = InitSSHConfig(n.info.Username, n.info.Password)
	}

	if ip, err = n.GetNodeIP(); err != nil {
		log.Errorf("TOPO SVC | Restart node failed")
		return fmt.Errorf("TOPO SVC | Failed to get Node IP")
	}

	addr = fmt.Sprintf("%s:%d", ip, constants.SSHPort)
	sshclient, err := ssh.Dial("tcp", addr, sshCfg)
	if sshclient == nil || err != nil {
		log.Errorf("SSH connect to %v (%v) failed", n.info.Name, n.info.IPAddress)
		return err
	}

	//Give it some time for node to stabilize
	time.Sleep(5 * time.Second)
	n.SSHClient = sshclient

	return nil
}

func (n *TestNode) AgentDBGobFiles() []string {
	agentDBFiles := []string{
		constants.DstIotaDBDir + n.Node.Name + ".gob",
		constants.DstIotaDBDir + n.Node.Name + "_workloads" + ".gob",
	}
	return agentDBFiles
}

func (n *TestNode) SavedDBGobFiles() []string {
	agentDBFiles := []string{
		constants.LocalAgentDBFolder + n.Node.Name + ".gob",
		constants.LocalAgentDBFolder + n.Node.Name + "_workloads" + ".gob",
	}
	return agentDBFiles
}

// SaveNode saves and downloads the node-context to local-fs
func (n *TestNode) SaveNode(cfg *ssh.ClientConfig) error {
	resp, err := n.AgentClient.SaveNode(context.Background(), n.Node)
	log.Infof("TOPO SVC | DEBUG | SaveNode Agent %v(%v) Received Response Msg: %v",
		n.Node.GetName(), n.Node.IpAddress, resp)
	if err != nil {
		log.Errorf("Saving node %v failed. Err: %v", n.Node.Name, err)
		return err
	}
	// Download agent context to local-fs
	log.Infof("TOPO SVC | SaveNode | Downloading IotaAgent DB for %v", n.Node.Name)

	for _, file := range n.AgentDBGobFiles() {
		if err := n.CopyFrom(cfg, constants.LocalAgentDBFolder, []string{file}); err != nil {
			//Workload files may be missing, ignore the error for now
			log.Errorf("TOPO SVC | InitTestBed | Failed to download agent db from TestNode: %v, IPAddress: %v", n.Node.Name, n.Node.IpAddress)
		}

	}

	return nil
}

// ReloadNode saves and reboots the nodes.
func (n *TestNode) ReloadNode(name string, restoreState bool, method string, useNcsi bool) error {
	var agentBinary string

	if n.Node == nil {
		n.Node = &iota.Node{}
	}

	var sshCfg *ssh.ClientConfig
	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
		sshCfg = InitSSHConfig(constants.EsxDataVMUsername, constants.EsxDataVMPassword)
	} else {
		sshCfg = n.info.SSHCfg
	}

	if err := n.SaveNode(sshCfg); err != nil {
		return err
	}

	if n.GrpcClient != nil {
		n.GrpcClient.Client.Close()
		n.GrpcClient = nil
	}
	if err := n.RestartNode(method, useNcsi); err != nil {
		log.Errorf("Restart node %v failed. Err: %v", n.Node.Name, err)
		return err
	}

	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_FREEBSD {
		agentBinary = constants.IotaAgentBinaryPathFreebsd
	} else {
		agentBinary = constants.IotaAgentBinaryPathLinux
	}

	if n.info.Os == iota.TestBedNodeOs_TESTBED_NODE_OS_ESX {
		sshCfg = InitSSHConfig(constants.EsxDataVMUsername, constants.EsxDataVMPassword)
	} else {
		sshCfg = n.info.SSHCfg
	}
	ip, _ := n.GetNodeIP()
	log.Infof("TOPO SVC | ReloadNode | Starting IOTA Agent on TestNode: %v, IPAddress: %v", n.Node.Name, ip)
	sudoAgtCmd := fmt.Sprintf("sudo -E %s", constants.DstIotaAgentBinary)
	if err := n.StartAgent(sudoAgtCmd, sshCfg); err != nil {
		log.Errorf("TOPO SVC | RestartNode | Failed to start agent binary: %v, on TestNode: %v, at IPAddress: %v", agentBinary, n.Node.Name, n.Node.IpAddress)
		return err
	}

	agentURL := fmt.Sprintf("%s:%d", ip, constants.IotaAgentPort)
	c, err := constants.CreateNewGRPCClient(n.Node.Name, agentURL, constants.GrpcMaxMsgSize)
	if err != nil {
		errorMsg := fmt.Sprintf("Could not create GRPC Connection to IOTA Agent. Err: %v", err)
		log.Errorf("TOPO SVC | ReloadNode | ReloadNode call failed to establish GRPC Connection to Agent running on Node: %v. Err: %v", n.Node.Name, errorMsg)
		return err
	}

	n.GrpcClient = c
	oldClient := n.AgentClient
	n.AgentClient = iota.NewIotaAgentApiClient(c.Client)
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Minute)
	defer cancel()
	if restoreState {
		resp, err := n.AgentClient.ReloadNode(ctx, n.Node)
		log.Infof("TOPO SVC | ReloadNode | ReloadNode Agent . Received Response Msg: %v", resp)
		if err != nil || resp.NodeStatus.ApiStatus != iota.APIResponseType_API_STATUS_OK {
			msg := fmt.Sprintf("Reload node %v failed. Err: %v", n.Node.Name, err)
			if resp.NodeStatus != nil {
				msg = fmt.Sprintf("Reload node %v failed. Err: %v, %v", n.Node.Name, err, resp.NodeStatus.ErrorMsg)
			}
			return fmt.Errorf(msg)
		}
	}

	if n.workloadMap != nil {
		n.workloadMap.Range(func(key interface{}, item interface{}) bool {
			wload := item.(iotaWorkload)
			if wload.workload != nil && wload.workload.GetWorkloadAgent() == oldClient {
				//Return as this is a control vm handle
				wload.workload.SetWorkloadAgent(n.AgentClient)
				return true
			}
			return true
		})
	}

	return nil
}

// ReloadNode saves and reboots the nodes.
func (n *VcenterNode) ReloadNode(name string, restoreState bool, method string, useNcsi bool) error {
	pool, _ := errgroup.WithContext(context.Background())
	for _, mn := range n.managedNodes {
		mn := mn
		name := name

		var vdc *VcenterDatacenter
	L:
		for _, dc := range n.dcMap {
			for _, dmn := range dc.managedNodes {
				if dmn.GetNodeInfo().Name == name {
					vdc = dc
					break L
				}
			}
		}
		if vdc == nil {
			log.Errorf("TOPO SVC |  Reload Node | Failed to find DC for node %v", name)
			return fmt.Errorf("TOPO SVC |  Reload Node | Failed to find DC for node %v", name)
		}
		pool.Go(func() error {
			if mn.GetNodeInfo().Name == name && mn.GetNodeAgent() != nil {
				log.Infof("Reloading vcenter node %v , name %v", n.GetNodeInfo().Name, name)
				err := vdc.cl.DeleteHost(mn.GetNodeInfo().IPAddress)
				if err != nil {
					log.Errorf("TOPO SVC |  Reload Node | Failed to disconnect host from cluster %v", err.Error())
					return err
				}

				err = mn.ReloadNode(name, restoreState, method, useNcsi)
				if err != nil {
					return errors.Wrapf(err, "Reload failed for node %v", name)
				}
				//Readd all workloads in the node too
				//TOOD

				sslThumbprint, err := mn.GetSSLThumbprint()
				if err != nil {
					log.Errorf("TOPO SVC | InitTestbed  | Failed to get ssl thumbprint %v", err.Error())
					return err
				}

				err = vdc.cl.AddHost(mn.GetNodeInfo().IPAddress,
					mn.GetNodeInfo().Username, mn.GetNodeInfo().Password, sslThumbprint)
				if err != nil {
					log.Errorf("TOPO SVC |  Reload Node | Failed to Add host to cluster after reboot %v", err.Error())
					return err
				}

				intfs, err := mn.GetHostInterfaces()
				if err != nil {
					return err
				}

				log.Infof("Adding pnic %v of host %v to dvs", intfs, mn.GetNodeInfo().Name)
				hostSpecs := []vmware.DVSwitchHostSpec{
					vmware.DVSwitchHostSpec{
						Name:  mn.GetNodeInfo().IPAddress,
						Pnics: intfs,
					},
				}
				dvsSpec := vmware.DVSwitchSpec{Hosts: hostSpecs,
					Name: mn.GetNodeInfo().DistributedSwitch, Cluster: mn.GetNodeInfo().ClusterName,
					Version:  constants.DvsVersion,
					MaxPorts: 10,
					Pvlans: []vmware.DvsPvlanPair{vmware.DvsPvlanPair{Primary: constants.VcenterPvlanStart,
						Secondary: constants.VcenterPvlanStart, Type: "promiscuous"}}}
				err = vdc.hdl.AddDvs(dvsSpec)
				if err != nil {
					log.Errorf("TOPO SVC | InitTestbed  | Error add DVS with host spec after reload %v", err.Error())
					return err
				}

				err = mn.ReAddWorkloads()
				if err != nil {
					log.Errorf("TOPO SVC | InitTestbed  | Error re-adding workloads %v", err.Error())
					return err
				}

			} else {
				log.Infof("Skipping Reloading vcenter node %v , name %v", n.GetNodeInfo().Name, name)
			}
			return nil
		})

		return pool.Wait()
	}

	return errors.New("Node not found for reload")
}

// GetNodeMsg returns nodettate
func (n *VcenterNode) GetNodeMsg(ctx context.Context, name string) *iota.Node {
	for _, mn := range n.managedNodes {
		if mn.GetNodeInfo().Name == name {
			return mn.GetNodeMsg(ctx, name)
		}
	}

	return n.RespNode
}

// GetWorkloads returns workloads
func (n *VcenterNode) GetWorkloads(name string) []*iota.Workload {
	for _, mn := range n.managedNodes {
		if mn.GetNodeInfo().Name == name {
			return mn.GetWorkloads(name)
		}
	}

	//default return empty
	return []*iota.Workload{}
}

//SetNodeMsg sets node msg
func (n *TestNode) SetNodeMsg(msg *iota.Node) {
	n.Node = msg
	n.info.Name = msg.Name
	n.info.InstallInfo = msg.InstallInfo
}

func (n *TestNode) SetNodeResponse(msg *iota.Node) {
	n.RespNode = msg
}

//AddNetworks add network
func (n *TestNode) AddNetworks(ctx context.Context, networkMsg *iota.NetworksMsg) (*iota.NetworksMsg, error) {

	networkMsg.ApiResponse = &iota.IotaAPIResponse{
		ApiStatus: iota.APIResponseType_API_SERVER_ERROR,
		ErrorMsg:  "Node does not support add network",
	}

	return networkMsg, nil
}

//GetNodeMsg gets node msg
func (n *TestNode) GetNodeMsg(ctx context.Context, name string) *iota.Node {
	log.Infof("TOPO SVC | DEBUG | GetNodeMsg for %v ", name)
	return n.RespNode
}

// ReInitNode re-init agent/node
func (n *TestNode) ReInitNode(ctx context.Context, name string) *iota.Node {
	resp, err := n.AgentClient.ReInitNode(ctx, n.Node)
	if err != nil {
		log.Errorf("TOPO SVC | GetNodeMsg  | Error in ReInitNode invocation %v", err.Error())
		return n.RespNode
	}
	n.SetNodeResponse(resp)
	//This is the ugliest hack
	//Some workload might be living in this node
	//Point those workload to this new agent client
	if n.workloadMap != nil {
		n.workloadMap.Range(func(key interface{}, item interface{}) bool {
			wload := item.(iotaWorkload)
			if wload.workload != nil && wload.workload.GetWorkloadAgent() != n.AgentClient {
				wload.workload.SetWorkloadAgent(n.AgentClient)
			}
			return true
		})
	}
	log.Infof("Node response updated for %v", n.RespNode.Name)
	return n.RespNode
}

//GetWorkloads returns all workloads for nodes.
func (n *TestNode) GetWorkloads(name string) []*iota.Workload {
	msg := []*iota.Workload{}
	n.workloadMap.Range(func(key interface{}, item interface{}) bool {
		wload, ok := item.(iotaWorkload)
		if ok && wload.workloadMsg != nil {
			msg = append(msg, wload.workloadMsg)
		}
		return true
	})

	return msg
}

//GetNodeInfo gets node info
func (n *TestNode) GetNodeInfo() NodeInfo {
	return n.info
}

//NodeController controller for this node
func (n *TestNode) NodeController() TestNodeInterface {
	return n.controlNode
}

//SetNodeController sets referencs to control node
func (n *TestNode) SetNodeController(node TestNodeInterface) {
	n.controlNode = node
}

//GetControlledNode gets the node which is being controlled by this node
func (n *TestNode) GetControlledNode(ip string) TestNodeInterface {
	return nil
}

//CheckHealth  checks health of node
func (n *TestNode) CheckHealth(ctx context.Context, health *iota.NodeHealth) (*iota.NodeHealth, error) {
	if n.GrpcClient == nil || n.GrpcClient.Client == nil ||
		(n.GrpcClient.Client.GetState() != connectivity.Ready && n.GrpcClient.Client.GetState() != connectivity.Idle) {
		health.HealthCode = iota.NodeHealth_NODE_DOWN
		health.NodeName = n.GetNodeInfo().Name
		health.Message = fmt.Sprintf("Node %v connectivity is down", n.GetNodeInfo().Name)
	}
	return n.AgentClient.CheckHealth(ctx, health)
}

//NodeConnector returns connector
func (n *TestNode) NodeConnector() interface{} {
	return n.connector
}

//SetConnector set the connnector
func (n *TestNode) SetConnector(connector interface{}) {
	n.connector = connector
}

//GetManagedNodes get all managed nodes
func (n *TestNode) GetManagedNodes() []TestNodeInterface {
	return nil
}

//AssocaiateIndependentNode adds a node which is provisioned independently too
func (n *TestNode) AssocaiateIndependentNode(node TestNodeInterface) error {
	return nil
}

//GetNodeAgent Retruns node agent
func (n *TestNode) GetNodeAgent() iota.IotaAgentApiClient {
	return n.AgentClient
}

//SetNodeAgent Retruns node agent
func (n *TestNode) SetNodeAgent(agent iota.IotaAgentApiClient) {
	n.AgentClient = agent
}

//SetDC set dc
func (n *TestNode) SetDC(dc string) {
	n.info.DCName = dc
}

//SetCluster sets cluster
func (n *TestNode) SetCluster(cl string) {
	n.info.ClusterName = cl
}

//SetSwitch sets switch
func (n *TestNode) SetSwitch(sw string) {
	n.info.DistributedSwitch = sw
}

//RunTriggerLocally run trigger locally for the node
func (n *TestNode) RunTriggerLocally() {
	n.triggerLocal = true
}

// EntityCopy does copy of items to/from entity.
func (n *TestNode) EntityCopy(cfg *ssh.ClientConfig, req *iota.EntityCopyMsg) (*iota.EntityCopyMsg, error) {
	log.Infof("TOPO SVC | DEBUG | EntityCopy. Received Request Msg: %v", req)
	defer log.Infof("TOPO SVC | DEBUG | EntityCopy Returned: %v", req)

	node := n

	req.ApiResponse.ApiStatus = iota.APIResponseType_API_STATUS_OK
	if req.Direction == iota.CopyDirection_DIR_IN {

		dstDir := common.DstIotaEntitiesDir + "/" + req.GetEntityName() + "/" + req.GetDestDir() + "/"
		if err := node.CopyTo(cfg, dstDir, req.GetFiles()); err != nil {
			log.Errorf("TOPO SVC | EntityCopy | Failed to copy files to entity:  %v on node : %v (%v)",
				req.GetEntityName(), node.GetNodeInfo().Name, err.Error())
			req.ApiResponse.ApiStatus = iota.APIResponseType_API_BAD_REQUEST
			req.ApiResponse.ErrorMsg = fmt.Sprintf("Failed to copy files to entity:  %v on node : %v",
				req.GetEntityName(), node.GetNodeInfo().Name)
		}
	} else if req.Direction == iota.CopyDirection_DIR_OUT {
		files := []string{}
		srcDir := common.DstIotaEntitiesDir + "/" + req.GetEntityName() + "/"
		for _, file := range req.GetFiles() {
			files = append(files, srcDir+file)
		}

		if err := node.CopyFrom(cfg, req.GetDestDir(), files); err != nil {
			log.Errorf("TOPO SVC | EntityCopy | Failed to copy files from entity:  %v on node : %v (%v)",
				req.GetEntityName(), node.GetNodeInfo().Name, err.Error())
			req.ApiResponse.ApiStatus = iota.APIResponseType_API_BAD_REQUEST
		}

	} else {
		errMsg := fmt.Sprintf("No direction specified for entity copy")
		req.ApiResponse = &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST,
			ErrorMsg: errMsg}
		return req, nil
	}

	return req, nil
}

// EntityCopy does copy of items to/from entity.
func (n *VcenterNode) EntityCopy(cfg *ssh.ClientConfig, req *iota.EntityCopyMsg) (*iota.EntityCopyMsg, error) {
	log.Infof("TOPO SVC | DEBUG | EntityCopy. Received Request Msg: %v", req)
	defer log.Infof("TOPO SVC | DEBUG | EntityCopy Returned: %v", req)

	mnode, ok := n.managedNodes[req.NodeName]
	if !ok {
		msg := fmt.Sprintf("Did not find  node  for copy %s, which already exists ", req.NodeName)
		log.Error(msg)
		return nil, errors.New(msg)
	}

	item, ok := mnode.(*EsxNode).workloadMap.Load(req.GetEntityName())
	if !ok {
		length := 0

		mnode.(*EsxNode).workloadMap.Range(func(_, _ interface{}) bool {
			length++

			return true
		})

		msg := fmt.Sprintf("Did not find  workload  for copy %s, which already exists %p %v", req.GetEntityName(), mnode, length)
		log.Error(msg)
		return nil, errors.New(msg)
	}

	node := item.(iotaWorkload)
	req.ApiResponse.ApiStatus = iota.APIResponseType_API_STATUS_OK
	if req.Direction == iota.CopyDirection_DIR_IN {

		dstDir := common.DstIotaEntitiesDir + "/" + req.GetEntityName() + "/" + req.GetDestDir() + "/"
		if err := node.CopyTo(cfg, dstDir, req.GetFiles()); err != nil {
			log.Errorf("TOPO SVC | EntityCopy | Failed to copy files to entity:  %v on node : %v (%v)",
				req.GetEntityName(), node.name, err.Error())
			req.ApiResponse.ApiStatus = iota.APIResponseType_API_BAD_REQUEST
			req.ApiResponse.ErrorMsg = fmt.Sprintf("Failed to copy files to entity:  %v on node : %v",
				req.GetEntityName(), node.name)
		}
	} else if req.Direction == iota.CopyDirection_DIR_OUT {
		files := []string{}
		srcDir := common.DstIotaEntitiesDir + "/" + req.GetEntityName() + "/"
		for _, file := range req.GetFiles() {
			files = append(files, srcDir+file)
		}

		if err := node.CopyFrom(cfg, req.GetDestDir(), files); err != nil {
			log.Errorf("TOPO SVC | EntityCopy | Failed to copy files from entity:  %v on node : %v (%v)",
				req.GetEntityName(), node.name, err.Error())
			req.ApiResponse.ApiStatus = iota.APIResponseType_API_BAD_REQUEST
		}

	} else {
		errMsg := fmt.Sprintf("No direction specified for entity copy")
		req.ApiResponse = &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST,
			ErrorMsg: errMsg}
		return req, nil
	}

	return req, nil
}

//GetHostInterfaces get host interfaces
func (n *TestNode) GetHostInterfaces() ([]string, error) {
	msg := n.GetNodeMsg(nil, n.GetNodeInfo().Name)

	log.Infof("Get host intf %v %v", n.GetNodeInfo().Name, msg)
	if msg.Type == iota.PersonalityType_PERSONALITY_NAPLES ||
		msg.Type == iota.PersonalityType_PERSONALITY_NAPLES_DVS ||
		msg.Type == iota.PersonalityType_PERSONALITY_NAPLES_BOND {
		for _, config := range msg.GetNaplesConfigs().GetConfigs() {
			if config == nil {
				log.Errorf("No naples config found")
				continue
			}
			for _, intf := range config.GetHostIntfs() {
				if intf.GetType() == iota.InterfaceType_INTERFACE_TYPE_NATIVE {
					return intf.Interfaces, nil
				}
			}
			log.Errorf("No host-interfaces found for this naples")
			continue
		}
		return nil, errors.New("No naples config found or no host-interfaces")
	}

	if msg.Type == iota.PersonalityType_PERSONALITY_THIRD_PARTY_NIC ||
		msg.Type == iota.PersonalityType_PERSONALITY_THIRD_PARTY_NIC_DVS {
		if msg.GetThirdPartyNicConfig() == nil {
			return nil, errors.New("No third party nic config found")
		}
		return msg.GetThirdPartyNicConfig().HostIntfs, nil
	}
	return nil, errors.New("Invalid personality to get host interfaces")
}

//GetSSLThumbprint get ssl thumbprint
func (n *TestNode) GetSSLThumbprint() (string, error) {
	msg := n.GetNodeMsg(nil, n.GetNodeInfo().Name)

	if msg.EsxConfig != nil {
		return msg.EsxConfig.SslThumbprint, nil
	}

	return "", errors.New("Invalid personality to get thumbprint")
}

//SetupNode setup node
func (n *TestNode) SetupNode() error {
	return nil
}

//MoveWorkloads workload move not supported.
func (n *TestNode) MoveWorkloads(ctx context.Context, req *iota.WorkloadMoveMsg) (*iota.WorkloadMoveMsg, error) {

	req.ApiResponse = &iota.IotaAPIResponse{
		ApiStatus: iota.APIResponseType_API_BAD_REQUEST,
		ErrorMsg:  fmt.Sprintf("Workload Move is not implemented by node %v", req.OrchestratorNode),
	}

	return req, errors.New("Workload move not supported")
}

//RemoveNetworks not supported for other nodes
func (n *TestNode) RemoveNetworks(ctx context.Context, req *iota.NetworksMsg) (*iota.NetworksMsg, error) {

	req.ApiResponse = &iota.IotaAPIResponse{
		ApiStatus: iota.APIResponseType_API_BAD_REQUEST,
		ErrorMsg:  fmt.Sprintf("Remove network is not implemented by node %v", req.Switch),
	}

	return req, errors.New("Remove network  supported")
}
