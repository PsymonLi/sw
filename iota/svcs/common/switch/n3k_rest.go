package DataSwitch

import (
	"encoding/json"
	"fmt"
	"strconv"
	"strings"

	"github.com/greenpau/go-cisco-nx-api/pkg/client"
	"github.com/pensando/sw/venice/utils/log"
)

// define Rest API instance of Nexus 3k
type nexus3kRest struct {
	username string
	password string
	ip       string
	clt      *client.Client
	breakout bool
}

func newNexus3kRest(ip, username, password string) (Switch, error) {
	clt := client.NewClient()

	err := clt.SetHost(ip)
	if err != nil {
		return nil, err
	}

	err = clt.SetUsername(username)
	if err != nil {
		return nil, err
	}

	err = clt.SetPassword(password)
	if err != nil {
		return nil, err
	}

	_, err = clt.GetSystemInfo()
	if err != nil {
		return nil, err
	}

	n3kInst := &nexus3kRest{username: username, password: password, ip: ip, clt: clt}

	return n3kInst, nil
}

func (sw *nexus3kRest) runConfigCommands(cmds []string) error {
	log.Infof("Configuring commands: %v\n", cmds)
	resp, err := sw.clt.Configure(cmds)
	if err != nil {
		log.Errorf("Switch return error %s", err.Error())
		return err
	}

	for i, r := range resp {
		if r.Error != nil {
			log.Errorf("Switch return error for command %s: %v", cmds[i], r.Error.Message)
			return fmt.Errorf("ERROR: switch returned failure for command %s: %v", cmds[i], r.Error.Message)
		}
	}
	return nil
}

func (sw *nexus3kRest) Disconnect() {
	// do nothing
}

func (sw *nexus3kRest) SetNativeVlan(port string, vlan int) error {
	//first create vlan
	err := sw.ConfigureVlans(strconv.Itoa(vlan), true)
	if err != nil {
		return err
	}

	cmds := []string{
		"interface " + port,
		"switchport trunk native vlan " + strconv.Itoa(vlan),
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) UnsetNativeVlan(port string, vlan int) error {
	cmds := []string{
		"interface " + port,
		"no switchport trunk native vlan " + strconv.Itoa(vlan),
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) LinkOp(port string, shutdown bool) error {
	cmds := []string{
		"interface " + port,
	}

	if shutdown {
		cmds = append(cmds, "shutdown")
	} else {
		cmds = append(cmds, "no shutdown")
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) ConfigureVlans(vlans string, igmpEnabled bool) error {
	cmds := []string{
		"spanning-tree mode mst",
		"vlan " + vlans,
		fmt.Sprintf("vlan config %s", vlans),
	}

	if igmpEnabled {
		cmds = append(cmds, "ip igmp snooping")
	} else {
		cmds = append(cmds, "no ip igmp snooping")
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) SetSpeed(port string, speed PortSpeed) error {
	if speed == Speed50g {
		return nil
	}

	cmds := []string{
		"interface " + port,
		"speed " + (portSpeedValue(speed)).String(),
	}
	if speed == SpeedAuto {
		cmds = append(cmds, "negotiate auto")
	} else {
		cmds = append(cmds, "no negotiate auto")
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) SetFlowControlReceive(port string, enable bool) error {
	cmds := []string{
		"interface " + port,
	}

	if enable {
		cmds = append(cmds, "flowcontrol receive on")
	} else {
		cmds = append(cmds, "flowcontrol receive off")
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) SetFlowControlSend(port string, enable bool) error {
	cmds := []string{
		"interface " + port,
	}

	if enable {
		cmds = append(cmds, "flowcontrol send on")
	} else {
		cmds = append(cmds, "flowcontrol send off")
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) SetMtu(port string, mtu uint32) error {
	cmds := []string{
		"interface " + port,
		fmt.Sprintf("mtu %v", mtu),
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) DisableIGMP(vlanRange string) error {
	//make sure vlans created
	err := sw.ConfigureVlans(vlanRange, false)
	if err != nil {
		return err
	}

	return nil
}

func (sw *nexus3kRest) SetTrunkVlanRange(port string, vlanRange string) error {
	//first create vlans
	err := sw.ConfigureVlans(vlanRange, true)
	if err != nil {
		return err
	}
	cmds := []string{
		"interface " + port,
		"switchport trunk allowed vlan " + vlanRange,
		//for faster convergence
		"spanning-tree port type edge trunk",
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) UnsetTrunkVlanRange(port string, vlanRange string) error {
	cmds := []string{
		"interface " + port,
		"no switchport trunk allowed vlan " + vlanRange,
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) SetTrunkMode(port string) error {
	cmds := []string{
		"interface " + port,
		"switchport",
		"switchport mode trunk",
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) UnsetTrunkMode(port string) error {
	cmds := []string{
		"interface " + port,
		"no channel-group",
		"no switchport mode trunk",
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) CheckSwitchConfiguration(port string, mode PortMode, status PortStatus, speed PortSpeed) (string, error) {
	// skipped
	return "", nil
}

func (sw *nexus3kRest) DoQosConfig(qosCfg *QosConfig) error {
	sysInfo, err := sw.clt.GetSystemInfo()
	if err != nil {
		return err
	}

	pfcSupport := true
	if !strings.Contains(sysInfo.ChassisID, "Nexus3000") {
		// Nexus9000 doesn't fully support PFC COS. Skip it.
		pfcSupport = false
	}

	cmds := []string{
		"policy-map type network-qos " + qosCfg.Name,
	}

	if len(qosCfg.Classes) != 0 {
		for _, qosClass := range qosCfg.Classes {
			cmds = append(cmds, "class type network-qos "+qosClass.Name)
			if qosClass.Mtu != 0 {
				cmds = append(cmds, fmt.Sprintf("mtu %v", qosClass.Mtu))
			}

			if qosClass.PfsCos <= 7 && pfcSupport {
				cmds = append(cmds, fmt.Sprintf("pause pfc-cos %v", qosClass.PfsCos))
			}

			cmds = append(cmds, "exit")
		}
	}

	cmds = append(cmds, "system qos")
	cmds = append(cmds, "service-policy type network-qos "+qosCfg.Name)
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) SetPortQueuing(port string, enable bool, params string) error {

	cmds := []string{
		"interface " + port,
	}
	if enable {
		cmds = append(cmds, fmt.Sprintf("service-policy type queuing output %s", params))
	} else {
		cmds = append(cmds, fmt.Sprintf("no service-policy type queuing output %s", params))
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) SetPortQos(port string, enable bool, params string) error {
	cmds := []string{
		"interface " + port,
	}
	if enable {
		cmds = append(cmds, fmt.Sprintf("service-policy type qos input %s", params))
	} else {
		cmds = append(cmds, fmt.Sprintf("no service-policy type qos input %s", params))
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) SetPortPause(port string, enable bool) error {
	cmds := []string{
		"interface " + port,
	}
	if enable {
		cmds = append(cmds, "flowcontrol send on", "flowcontrol receive on")
	} else {
		cmds = append(cmds, "no flowcontrol send on", "no flowcontrol receive on")
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) SetPortPfc(port string, enable bool) error {
	cmds := []string{
		"interface " + port,
	}
	if enable {
		cmds = append(cmds, "no flowcontrol send on", "no flowcontrol receive on", "priority-flow-control mode on")
	} else {
		cmds = append(cmds, "no priority-flow-control mode on")
	}
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) DoDscpConfig(dscpConfig *DscpConfig) error {

	var cmds []string
	if len(dscpConfig.Classes) != 0 {
		for _, dscpQosClass := range dscpConfig.Classes {
			cmds = append(cmds, "class-map type qos match-any "+dscpQosClass.Name)
			cmds = append(cmds, fmt.Sprintf("match cos %v", dscpQosClass.Cos))
			cmds = append(cmds, fmt.Sprintf("match dscp %v", dscpQosClass.Dscp))
			cmds = append(cmds, "exit")
		}
	}

	cmds = append(cmds, "policy-map type qos "+dscpConfig.Name)

	if len(dscpConfig.Classes) != 0 {
		for _, dscpQosClass := range dscpConfig.Classes {
			cmds = append(cmds, "class "+dscpQosClass.Name)
			cmds = append(cmds, fmt.Sprintf("set qos-group %v", dscpQosClass.Cos))
			cmds = append(cmds, "exit")
		}
	}

	/*cmds = append(cmds, "system qos")
	cmds = append(cmds, "service-policy type network-qos "+dscpConfig.Name)*/
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) DoQueueConfig(queueConfig *QueueConfig) error {
	cmds := []string{
		"policy-map type queuing ", queueConfig.Name,
	}
	if len(queueConfig.Classes) != 0 {
		for _, queueQosClass := range queueConfig.Classes {
			cmds = append(cmds, "class type queuing "+queueQosClass.Name)
			cmds = append(cmds, fmt.Sprintf("priority level %v", queueQosClass.Priority))
			cmds = append(cmds, fmt.Sprintf("bandwidth remaining percent %v", queueQosClass.Percent))
			cmds = append(cmds, "random-detect minimum-threshold 50 kbytes maximum-threshold 500 kbytes drop-probability 80 weight 15 ecn")
			cmds = append(cmds, "shape min 20 gbps max 20 gbps")
			cmds = append(cmds, "exit")
		}
	}
	cmds = append(cmds, "system qos")
	cmds = append(cmds, "service-policy type network-qos "+queueConfig.Name)
	return sw.runConfigCommands(cmds)
}

func (sw *nexus3kRest) SetBreakoutMode(port string) error {
	parts := strings.Split(port, "/")
	_, err := sw.clt.GetGeneric("interface breakout module 1 port " + parts[1] + " map 50g-2x")
	if err != nil {
		log.Errorf("Failed to set breakout mode. %s", err.Error())
		return err
	}
	cmds := []string{
		"interface " + parts[0] + "/" + parts[1] + "/1",
		"fec off",
		"mtu 9216",
	}
	sw.clt.Configure(cmds)
	return nil
}

func (sw *nexus3kRest) UnsetBreakoutMode(port string) error {
	parts := strings.Split(port, "/")
	_, err := sw.clt.GetGeneric("no interface breakout module 1 port " + parts[1] + " map 50g-2x")
	if err != nil {
		log.Errorf("Failed to unset breakout mode. %s", err.Error())
		return err
	}
	return nil
}

func (sw *nexus3kRest) CreatePortChannel(portChannelNumber string, mtu uint32, nativeVlan uint32, trunkVlanRange string, ports []string) error {
	pcCreateCmds := []string{
		"interface port-channel " + portChannelNumber,
	}
	pcCreateCmds = append(pcCreateCmds, "switchport mode trunk")
	pcCreateCmds = append(pcCreateCmds, fmt.Sprintf("mtu %v", mtu))
	pcCreateCmds = append(pcCreateCmds, fmt.Sprintf("switchport trunk allowed vlan %v,%v", trunkVlanRange, nativeVlan))
	pcCreateCmds = append(pcCreateCmds, fmt.Sprintf("switchport trunk native vlan %v", nativeVlan))
	pcCreateCmds = append(pcCreateCmds, "exit")

	err := sw.runConfigCommands(pcCreateCmds)
	if err != nil {
		log.Errorf("Failed to create port-channel %s", err.Error())
		return err
	}

	for _, port := range ports {
		memberCmds := []string{
			"interface " + port,
			"channel-group " + portChannelNumber,
			"exit",
		}
		err := sw.runConfigCommands(memberCmds)
		if err != nil {
			log.Errorf("Failed to update port-channel member %s %s", port, err.Error())
			return err
		}
	}
	return nil
}

func (sw *nexus3kRest) DeletePortChannel(portChannelNumber string, ports []string) error {
	for _, port := range ports {
		memberCmds := []string{
			"interface " + port,
			"no channel-group",
			"exit",
		}
		err := sw.runConfigCommands(memberCmds)
		if err != nil {
			// ignore and continue
			// return err
		}
	}

	pcDeleteCmds := []string{
		"no interface port-channel " + portChannelNumber,
	}
	return sw.runConfigCommands(pcDeleteCmds)
}

type lldpNeighborN3k struct {
	NeighHdr        string `json:"neigh_hdr"`
	NeighCount      string `json:"neigh_count"`
	TABLENborDetail struct {
		ROWNborDetail struct {
			ChassisType       string `json:"chassis_type"`
			ChassisID         string `json:"chassis_id"`
			PortType          string `json:"port_type"`
			PortID            string `json:"port_id"`
			LPortID           string `json:"l_port_id"`
			PortDesc          string `json:"port_desc"`
			SysName           string `json:"sys_name"`
			SysDesc           string `json:"sys_desc"`
			TTL               string `json:"ttl"`
			SystemCapability  string `json:"system_capability"`
			EnabledCapability string `json:"enabled_capability"`
			MgmtAddrType      string `json:"mgmt_addr_type"`
			MgmtAddr          string `json:"mgmt_addr"`
			MgmtAddrIpv6Type  string `json:"mgmt_addr_ipv6_type"`
			MgmtAddrIpv6      string `json:"mgmt_addr_ipv6"`
			InvalidVlanID     string `json:"invalid_vlan_id"`
		} `json:"ROW_nbor_detail"`
	} `json:"TABLE_nbor_detail"`
}

func (sw *nexus3kRest) GetLLDPOutput(port string) (LLDPPortConfig, error) {

	cmd := fmt.Sprintf("show lldp neighbors interface %v detail", port)
	output, err := sw.clt.GetGeneric(cmd)

	var respJSON client.JSONRPCResponse
	err = json.Unmarshal(output, &respJSON)
	if err != nil {
		return LLDPPortConfig{}, err
	}

	var body client.JSONRPCResponseBody
	err = json.Unmarshal(respJSON.Result, &body)
	if err != nil {
		fmt.Printf("%v\n", string(respJSON.Result))
		return LLDPPortConfig{}, err
	}

	var lldp lldpNeighborN3k
	err = json.Unmarshal(body.Body, &lldp)
	if err != nil {
		return LLDPPortConfig{}, err
	}

	return LLDPPortConfig{
		MgmtAddr:     lldp.TABLENborDetail.ROWNborDetail.MgmtAddr,
		PortDesc:     lldp.TABLENborDetail.ROWNborDetail.PortDesc,
		SysDesc:      lldp.TABLENborDetail.ROWNborDetail.SysDesc,
		SysName:      lldp.TABLENborDetail.ROWNborDetail.SysName,
		PortID:       port,
		MgmtAddrIpv6: lldp.TABLENborDetail.ROWNborDetail.MgmtAddrIpv6,
	}, nil
}
