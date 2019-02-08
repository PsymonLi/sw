package common

import (
	"fmt"
	"os"
	"time"
)

// global constants
const (

	// IotaSvcPort is the default IOTA Server Port
	IotaSvcPort = 60000

	// IotaAgentPort is the default IOTA Agent Port
	IotaAgentPort = 60001

	// IotaAgentBinaryName is the name of of the IOTA Agent Binary
	IotaAgentBinaryName = "iota_agent"

	// IotaAgentBinaryNameLinux is the name of of the IOTA Agent Binary for Linux
	IotaAgentBinaryNameLinux = "linux/" + IotaAgentBinaryName

	// IotaAgentBinaryNameFreebsd is the name of of the IOTA Agent Binary for Freebsd
	IotaAgentBinaryNameFreebsd = "freebsd/" + IotaAgentBinaryName

	//NicFinderConfFileName
	NicFinderConfFileName = "nic.conf"

	// VlansPerTestBed vlans that a testbed can manage
	VlansPerTestBed = 10

	// SSHPort captures the port where SSH is running
	SSHPort = 22

	// DstIotaAgentDir captures the top level dir where all the agent copies happen
	DstIotaAgentDir = "/pensando/iota"

	// DstIotaEntitiesDir has all workload related data for each workload
	DstIotaEntitiesDir = DstIotaAgentDir + "/entities"

	// DstIotaDBDir has persistance for db.
	DstIotaDBDir = DstIotaAgentDir + "/db"

	//MakeClusterTimeout waits for 5 minutes for the cluster to be up
	MakeClusterTimeout = time.Duration(time.Minute * 5)

	//WorkloadsPerNode captures the number of endpoints per node.
	WorkloadsPerNode = 4

	// IOTARandomSeed captures the fixed seed that IOTA uses to generate IP Address and MAC Addresses
	IOTARandomSeed = 42

	// IotaWorkloadImage is the docker image that will be used for workloads
	IotaWorkloadImage = "registry.test.pensando.io:5000/pensando/iota/centos:1.1"

	// EsxControlVmName Esx Control VM name
	EsxControlVMName = "iota-control-vm"

	// EsxDataVmName Esx Data VM prefix
	EsxDataVMPrefix = "iota-data-vm-"

	// EsxDataVmName Esx Data nw prefix
	EsxDataNWPrefix = "iota-data-nw-"

	// EsxControlVMImage image to use
	EsxControlVMImage = "build-114"

	//DstNicFinderConf file to be used on node.
	DstNicFinderConf = DstIotaAgentDir + "/nic.conf"

	// EsxControlVmCpus Esx Control VM Cpus
	EsxControlVMCpus = 4

	// EsxControlVmMemory Esx Control VM memory (4g)
	EsxControlVMMemory = 4

	// BuildItURL
	BuildItURL = "buildit.test.pensando.io"

	// ControlVmImageDirectory
	ControlVMImageDirectory = "/tmp"

	// DataVmImageDirectory
	DataVMImageDirectory = "/pensando/iota"

	NaplesMnicIP            = "169.254.0.1"
	CtrlVMNaplesInterfaceIP = "169.254.0.2/24"

	EsxDataVMUsername            = "vm"
	EsxDataVMPassword            = "vm"
	EsxDataVMInterface           = "eth1"
	EsxCtrlVMMgmtInterface       = "eth0"
	EsxCtrlVMNaplesMgmtInterface = "eth1"
	EsxDefaultNetwork            = "iota-def-network"
	EsxVMNetwork                 = "iota-vm-network"
	EsxDataNetwork               = "iota-data-network"
	EsxIotaCtrlSwitch            = "iota-ctrl-switch"
	EsxIotaDataSwitch            = "iota-data-switch"
	EsxNaplesMgmtNetwork         = "iota-naples-mgmt-network"
	EsxIotaNaplesMgmtSwitch      = "iota-naples-mgmt-switch"
	EsxDefaultNetworkVlan        = 4093
	EsxVMNetworkVlan             = 4092
)

// incrementing constants. List all constants whose value you don't care here
const (
	// RunCommandForeground runs a command in the foreground
	RunCommandForeground = iota

	// RunCommandBackground runs a command in the background
	RunCommandBackground
)

// global derived vars  from the constants
var (
	// IotaAgentBinaryPathLinux captures the location of the build IOTA Agent Binary for Linux
	IotaAgentBinaryPathLinux = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/bin/agent/%s", os.Getenv("GOPATH"), IotaAgentBinaryNameLinux)

	// IotaAgentBinaryPathFreebsd captures the location of the build IOTA Agent Binary for Freebsd
	IotaAgentBinaryPathFreebsd = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/bin/agent/%s", os.Getenv("GOPATH"), IotaAgentBinaryNameFreebsd)

	//NicFinderConf nic finder conf
	NicFinderConf = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/scripts/%s", os.Getenv("GOPATH"), NicFinderConfFileName)

	// DstIotaAgentBinary captures the location of agent on the remote nodes
	DstIotaAgentBinary = fmt.Sprintf("%s/%s", DstIotaAgentDir, IotaAgentBinaryName)

	// BuildItBinary
	BuildItBinary = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/images/buildit", os.Getenv("GOPATH"))

	// CleanupCommands lists the clean up commands required to clean up an IOTA node.
	CleanupCommands = []string{
		`sudo /pensando/iota/INSTALL.sh --clean-only`,
		`sudo systemctl stop pen-cmd`,
		`sudo docker rm -fv $(docker ps -aq)`,
		`sudo docker system prune -f`,
		`sudo rm /etc/hosts`,
		`sudo pkill iota*`,
		`sudo pkill bootstrap`,
		`sudo pkill python`,
		`sudo rm -rf /pensando/iota*`,
		`sudo docker ps`,
		`sudo docker rmi -f \$(docker images -aq)`,
		`sudo rm -rf /pensando/run/naples`,
		`sudo iptables -F`,
		`sudo systemctl restart docker`,
		//Delete all sub interfaces
		`sudo ip link | grep "_" | grep @ | cut -d'@' -f1  | cut -d':' -f2 |  awk  '{print $2}' |  xargs -I {} sudo ip link del {}`,
		`sudo ifconfig | grep \": flags\" | cut -d':' -f1 | sed -n 's/\([.]\)/./p' | xargs -I {} sudo ifconfig {} destroy`,
	}

	EsxControlVMNetworks = []string{"VM Network", EsxDefaultNetwork, EsxDefaultNetwork, EsxDefaultNetwork}

	EsxDataVMNetworks = []string{"VM Network", EsxVMNetwork, EsxDefaultNetwork, EsxDefaultNetwork}
)
