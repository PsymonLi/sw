package common

import (
	"fmt"
	"os"
)

//ModelType type
type ModelType int

const (
	//DefaultModel for GS
	DefaultModel ModelType = 0
	//VcenterModel for Vcenter
	VcenterModel = 1

	//CloudModel for equnix
	CloudModel = 2

	//BaseNetModel for classic
	BaseNetModel = 3
)

//NetworkType networkTypes
type NetworkType int

const (
	//DefaultNetworkType
	DefaultNetworkType NetworkType = 0
	//VmotionNetworkType vmotion
	VmotionNetworkType NetworkType = 1
)

//NetworkSpec to ceate
type NetworkSpec struct {
	Name   string
	Switch string
	Nodes  []string
	NwType NetworkType
}

//MockVeniceURL mock venice url
const MockVeniceURL = "localhost:9443"

// Vmotion subnet used for vmotion
const VmotionSubnet = "169.254.0"

//ModelInfoFile file
var ModelInfoFile = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/logs/model.json", os.Getenv("GOPATH"))

type RunRandomTrigger func(percent int) error

//TriggerIntf some trigger that testcase could use to run
type TriggerIntf interface {
	TriggerNaplesUpgrade(int) error
	TriggerSnapshotRestore(int) error
	TriggerHostReboot(int) error
	TriggerVeniceReboot(int) error
	TriggerVenicePartition(int) error
	TriggerDeleteAddConfig(int) error
	TriggerLinkFlap(percent int) error
	RunRandomTrigger(percent int) error
}

//RolloutSpec rollout spec
type RolloutSpec struct {
	BundleType    string
	Local         bool
	TargetLatest  bool
	TargetBranch  string
	TargetVersion string
	SkipDSC       bool
	SkipVenice    bool
	Pipeline      string
	SkipSimDSC    bool
	UpgradeType   string
}

//ModelInfo infor about mode
type ModelInfo struct {
	TestbedID   string
	Type        string
	TopoFile    string
	TestbedFile string
}
