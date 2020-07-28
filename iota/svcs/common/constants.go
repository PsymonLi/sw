package common

import (
	"fmt"
	"os"
	"time"
)

// global constants
const (
	GrpcMaxMsgSize = 1024 * 1024 * 16

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

	//NicFinderConfFileName
	PenNicFinderScriptName = "pen_nics.py"

	//PenctlAuthTokenFileName auth token filename
	PenctlAuthTokenFileName = "penctl.token"

	// VlansPerTestBed vlans that a testbed can manage
	VlansPerTestBed = 36

	// MaxVlanRange max vlan range to be used for allocation
	MaxVlanRange = 3700

	// SSHPort captures the port where SSH is running
	SSHPort = 22

	// TBOfflineErrorCode error code for tb offline
	TBOfflineErrorCode = 10

	// DstIotaAgentDir captures the top level dir where all the agent copies happen
	DstIotaAgentDir = "/pensando/iota"

	// NaplesAuthTokenFile naples auth token file
	NaplesAuthTokenFile = "/pensando/iota/naples_auth_token"

	// DstIotaEntitiesDir has all workload related data for each workload
	DstIotaEntitiesDir = DstIotaAgentDir + "/entities"

	// DstIotaDBDir has persistance for db.
	DstIotaDBDir = DstIotaAgentDir + "/db/"

	//MakeClusterTimeout waits for 5 minutes for the cluster to be up
	MakeClusterTimeout = time.Duration(time.Minute * 5)

	// MaxConcurrentNaplesToBringup
	MaxConcurrentNaplesToBringup = 10

	//WorkloadsPerNode captures the number of endpoints per node.
	WorkloadsPerNode = 4

	// IOTARandomSeed captures the fixed seed that IOTA uses to generate IP Address and MAC Addresses
	IOTARandomSeed = 42

	// IotaWorkloadImage is the docker image that will be used for workloads
	IotaWorkloadImage = "registry.test.pensando.io:5000/pensando/iota/centos:1.1"

	EsxControlVMNamePrefix = "iota-control-vm-"

	// EsxDataVmName Esx Data VM prefix
	EsxDataVMPrefix = "iota-data-vm-"

	// EsxDataVmName Esx Data nw prefix
	EsxDataNWPrefix = "iota-data-nw-"

	// EsxControlVMImage image to use
	EsxControlVMImage = "build-114"

	//DstNicFinderConf file to be used on node.
	DstNicFinderConf = DstIotaAgentDir + "/" + NicFinderConfFileName

	//DstEsxNicFinderScript file to be used on node.
	DstNicFinderScript = DstIotaAgentDir + "/" + PenNicFinderScriptName

	DstIotaNicFinderCommand = "python3 " + DstIotaAgentDir + "/" + PenNicFinderScriptName

	// EsxControlVmCpus Esx Control VM Cpus
	EsxControlVMCpus = 4

	// EsxControlVmMemory Esx Control VM memory (4g)
	EsxControlVMMemory = 4

	// EsxDataVMCpus Esx Data VM Cpus
	EsxDataVMCpus = 1

	// EsxDataVMMemory Esx Data VM memory (2g)
	EsxDataVMMemory = 2

	// BuildItURL
	BuildItURL = "buildit.test.pensando.io"

	// ImageArtificatsDirectory
	ImageArtificatsDirectory = "/pensando/iota/images"

	// ControlVmImageDirectory
	ControlVMImageDirectory = "/tmp"

	// DataVmImageDirectory
	DataVMImageDirectory = "/tmp"

	CtrlVMNaplesInterfaceIP = "169.254.0.2/24"

	DefaultHostUsername = "vm"
	DefaultHostPassword = "vm"

	EsxDataVMUsername            = "vm"
	EsxDataVMPassword            = "vm"
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

	InbandVlanStart = 3700
	InbandVlanEnd   = 3799

	// MaxVlanRange max vlan range to be used for allocation
	MaxDataVlanRange = 3700

	// UserPassword is test user password
	UserPassword = "Pensando0$"

	//after a duration of this time if the server doesn't see any
	//activity it pings client to see if the transport is still alive.
	GrpcServerTransportKeepaliveTimeSeconds = time.Second * 90

	//after having pinged for keepalive check, the server waits for a
	//duration of Timeout and if no activity is seen even after that
	//the connection is closed.
	GrpcServerTransportKeepaliveTimeoutSeconds = time.Second * 300

	//after a duration of this time if the client doesn't see any
	//activity it pings server to see if the transport is still alive.
	GrpcClientTransportKeepaliveTimeSeconds = time.Second * 90

	//after having pinged for keepalive check, the client waits for
	//a duration of Timeout and if no activity is seen even after
	//that the connection is closed.
	GrpcClientTransportKeepaliveTimeoutSeconds = time.Second * 300

	DefaultCimcUserName = "admin"

	DefaultCimcPassword = "N0isystem$"

	IotaVmotionPortgroup = "iota-vmotion-pg"

	ReservedPGVlanCount = 1002

	IotaVmotionSwitch = "vSwitch0"

	VcenterPvlanPairCount = 64

	VcenterPvlanStart = 2048

	DvsVersion = "6.6.0"

	// WindowsPowerShell Windows Powershell Command
	WindowsPowerShell = "/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"

	// WindowsPortMappingFile
	WindowsPortMappingFile = "/pensando/iota/name-mapping.json"
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
	NaplesMnicIP = "169.254.0.1"
	// IotaAgentBinaryPathLinux captures the location of the build IOTA Agent Binary for Linux
	IotaAgentBinaryPathLinux = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/bin/agent/%s", os.Getenv("GOPATH"), IotaAgentBinaryNameLinux)

	// IotaAgentBinaryPathFreebsd captures the location of the build IOTA Agent Binary for Freebsd
	IotaAgentBinaryPathFreebsd = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/bin/agent/%s", os.Getenv("GOPATH"), IotaAgentBinaryNameFreebsd)

	IotaScript = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/scripts/", os.Getenv("GOPATH"))

	//NicFinderConf nic finder conf
	NicFinderConf = IotaScript + NicFinderConfFileName

	NicFinderScript = IotaScript + PenNicFinderScriptName

	K8sInstallScript = IotaScript + "kube-install.sh"

	PenctlAuthTokenFile = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/scripts/%s", os.Getenv("GOPATH"), PenctlAuthTokenFileName)

	// DstIotaAgentBinary captures the location of agent on the remote nodes
	DstIotaAgentBinary = fmt.Sprintf("%s/%s", DstIotaAgentDir, IotaAgentBinaryName)

	// DataVmImageDirectory
	IotaServerImgDir = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/images", os.Getenv("GOPATH"))

	// IotaServerDir directory associated for iota server cache
	IotaServerDir = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/logs", os.Getenv("GOPATH"))

	// DstIotaEntitiesDir has all workload related data for each workload
	IotaServerEntitiesDir = IotaServerDir + "/entities"

	AssetPullBin = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/bin/asset-pull", os.Getenv("GOPATH"))

	// BuildItBinary
	BuildItBinary = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/images/buildit", os.Getenv("GOPATH"))

	// Runner/LocalFS location to store iota-agent DB
	LocalAgentDBFolder = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/agentdb/", os.Getenv("GOPATH"))

	// CleanupCommands lists the clean up commands required to clean up an IOTA node.
	CleanupCommands = []string{
		`sudo -E kubeadm reset -f`,
		`sudo -E /pensando/iota/images/INSTALL.sh --clean-only`,
		`sudo -E systemctl stop pen-cmd`,
		`sudo -E docker rm -fv $(docker ps -aq)`,
		`sudo -E docker system prune -f`,
		`sudo -E rm /etc/hosts`,
		`sudo -E pkill iota*`,
		`sudo -E kill -9 $(pgrep iota)`,
		`sudo -E pkill bootstrap`,
		`sudo -E kill -9 $(pgrep bootstrap)`,
		`sudo -E pkill python`,
		`sudo -E kill -9 $(pgrep python)`,
		`mv /pensando/iota/images /tmp`,
		`sudo -E rm -rf /pensando/*`,
		`sudo -E mkdir -p /pensando/iota `,
		`sudo -E chown vm:vm /pensando/iota`,
		`mv /tmp/images /pensando/iota`,
		`mkdir -p /pensando/iota/images`,
		`sudo -E docker ps`,
		`sudo -E docker rmi -f \$(docker images -aq)`,
		`sudo -E rm -rf /pensando/run/naples`,
		`sudo -E iptables -F`,
		`sudo -E systemctl restart docker`,
		//Delete all sub interfaces
		`sudo -E ip link | grep "_" | grep @ | cut -d'@' -f1  | cut -d':' -f2 |  awk  '{print $2}' |  xargs -I {} sudo -E ip link del {}`,
		`sudo -E ifconfig | grep \": flags\" | cut -d':' -f1 | sed -n 's/\([.]\)/./p' | xargs -I {} sudo -E ifconfig {} destroy`,
	}

	EsxControlVMNetworks = []string{"VM Network", EsxDefaultNetwork, EsxDefaultNetwork, EsxDefaultNetwork}

	EsxDataVMNetworks = []string{"VM Network", EsxDefaultNetwork, EsxDefaultNetwork, EsxDefaultNetwork}

	EsxDataVMInterfaces = []string{"eth1", "eth2", "eth3"}
)
