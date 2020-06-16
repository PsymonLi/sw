//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"bytes"
	"context"
	"fmt"
	"reflect"
	"strings"

	uuid "github.com/satori/go.uuid"
	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/apollo/agent/cli/utils"
	"github.com/pensando/sw/nic/apollo/agent/gen/pds"
)

const (
	lldpCmdTypeNbrs	    = 0
	lldpCmdTypeLocalIfs = 1
	lldpCmdTypeStats    = 2
)

var (
	lifID       string
	ifID        string
	ifType      string
	txPolicerID string
)

var lifShowCmd = &cobra.Command{
	Use:   "lif",
	Short: "show lif information",
	Long:  "show lif object information",
	Run:   lifShowCmdHandler,
}

var ifShowCmd = &cobra.Command{
	Use:   "interface",
	Short: "show interface information",
	Long:  "show interface object information",
	Run:   ifShowCmdHandler,
}

var lldpShowCmd = &cobra.Command{
	Use:   "lldp",
	Short: "show lldp information",
	Long:  "show lldp object information",
}

var lldpShowNeighborsCmd = &cobra.Command{
	Use:   "neighbors",
	Short: "show lldp neighbor information",
	Long:  "show lldp neighbor object information",
	Run:   lldpShowNeighborsCmdHandler,
}

var lldpShowInterfacesCmd = &cobra.Command{
	Use:   "interfaces",
	Short: "show lldp local interface information",
	Long:  "show lldp local interface object information",
	Run:   lldpShowInterfacesCmdHandler,
}

var lldpShowStatisticsCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show lldp statistics information",
	Long:  "show lldp statistics object information",
	Run:   lldpShowStatisticsCmdHandler,
	Hidden: true,
}

var lifClearCmd = &cobra.Command{
	Use:   "lif",
	Short: "clear lif information",
	Long:  "clear lif information",
}

var lifClearStatsCmd = &cobra.Command{
	Use:   "statistics",
	Short: "clear lif statistics",
	Long:  "clear lif statistics",
	Run:   lifClearStatsCmdHandler,
}

var ifUpdateCmd = &cobra.Command{
	Use:   "interface",
	Short: "update interface information",
	Long:  "update interface information",
	Run:   ifUpdateCmdHandler,
}

func init() {
	showCmd.AddCommand(lifShowCmd)
	showCmd.AddCommand(ifShowCmd)
	showCmd.AddCommand(lldpShowCmd)
	lldpShowCmd.AddCommand(lldpShowNeighborsCmd)
	lldpShowCmd.AddCommand(lldpShowInterfacesCmd)
	lldpShowCmd.AddCommand(lldpShowStatisticsCmd)

	lifShowCmd.Flags().StringVar(&lifID, "id", "", "Specify Lif ID")
	lifShowCmd.Flags().Bool("yaml", true, "Output in yaml")
	lifShowCmd.Flags().Bool("summary", false, "Display number of objects")
	ifShowCmd.Flags().StringVar(&ifType, "type", "", "Specify interface type (uplink, control, host, l3)")
	ifShowCmd.Flags().StringVar(&ifID, "id", "", "Specify interface ID")
	ifShowCmd.Flags().Bool("yaml", true, "Output in yaml")
	ifShowCmd.Flags().Bool("summary", false, "Display number of objects")
	lldpShowCmd.Flags().Bool("yaml", true, "Output in yaml")

	clearCmd.AddCommand(lifClearCmd)
	lifClearCmd.AddCommand(lifClearStatsCmd)
	lifClearStatsCmd.Flags().StringVar(&lifID, "id", "", "Specify Lif ID")

	debugUpdateCmd.AddCommand(ifUpdateCmd)
	ifUpdateCmd.Flags().StringVar(&ifID, "id", "", "Specify interface ID")
	ifUpdateCmd.Flags().StringVar(&txPolicerID, "tx-policer", "", "Specify tx policer ID")
	ifUpdateCmd.MarkFlagRequired("id")
	ifUpdateCmd.MarkFlagRequired("tx-policer")
}

func ifUpdateCmdHandler(cmd *cobra.Command, args []string) {
	// connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewIfSvcClient(c)
	ifReq := &pds.InterfaceGetRequest{
		Id: [][]byte{uuid.FromStringOrNil(ifID).Bytes()},
	}
	ifRespMsg, err := client.InterfaceGet(context.Background(), ifReq)
	if err != nil {
		fmt.Printf("Get interface failed, err %v\n", err)
		return
	}
	if ifRespMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", ifRespMsg.ApiStatus)
		return
	}
	ifResp := ifRespMsg.GetResponse()
	if ifResp[0].GetSpec().GetType() != pds.IfType_IF_TYPE_HOST {
		fmt.Printf("Only update of  host interface supported\n")
		return
	}

	spec := &pds.InterfaceSpec{
		Id:          uuid.FromStringOrNil(ifID).Bytes(),
		Type:        pds.IfType_IF_TYPE_HOST,
		AdminStatus: ifResp[0].GetSpec().GetAdminStatus(),
		Ifinfo: &pds.InterfaceSpec_HostIfSpec{
			HostIfSpec: &pds.HostIfSpec{
				TxPolicer: uuid.FromStringOrNil(txPolicerID).Bytes(),
			},
		},
	}
	req := &pds.InterfaceRequest{
		Request: []*pds.InterfaceSpec{
			spec,
		},
	}
	// PDS call
	_, err = client.InterfaceUpdate(context.Background(), req)
	if err != nil {
		fmt.Printf("Updating interface failed, err %v\n", err)
		return
	}
	fmt.Printf("Interface updated successfully\n")
}

func lifClearStatsCmdHandler(cmd *cobra.Command, args []string) {
	// connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewIfSvcClient(c)
	var req *pds.Id
	if cmd != nil && cmd.Flags().Changed("id") {
		// clear stats for given lif
		req = &pds.Id{
			Id: uuid.FromStringOrNil(lifID).Bytes(),
		}
	} else {
		// clear stats for all vnics
		req = &pds.Id{
			Id: []byte{},
		}
	}

	// PDS call
	_, err = client.LifStatsReset(context.Background(), req)
	if err != nil {
		fmt.Printf("Clearing lif stats failed, err %v\n", err)
		return
	}
}

func ifShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	var req *pds.InterfaceGetRequest
	if cmd != nil && cmd.Flags().Changed("id") == true {
		req = &pds.InterfaceGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(ifID).Bytes()},
		}
	} else if cmd != nil && cmd.Flags().Changed("type") == true {
		if validateIfTypeStr(ifType) != true {
			fmt.Printf("Invalid interface type specified. Must be one of host, l3, uplink, control\n")
			return
		}
		req = &pds.InterfaceGetRequest{
			Id: [][]byte{},
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") == true {
		req = &pds.InterfaceGetRequest{
			Id: [][]byte{},
		}
	} else if cmd != nil && cmd.Flags().Changed("yaml") == true {
		req = &pds.InterfaceGetRequest{
			Id: [][]byte{},
		}
	} else if cmd != nil {
		fmt.Printf("Command requires a flag. Refer to help string\n")
		return
	} else {
		req = &pds.InterfaceGetRequest{
			Id: [][]byte{},
		}
	}

	client := pds.NewIfSvcClient(c)
	respMsg, err := client.InterfaceGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Get Interface failed, err %v\n", err)
		return
	}
	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	fmt.Printf("\n")
	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		var num_intfs [pds.IfType_IF_TYPE_HOST + 1]int
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetType() != pds.IfType_IF_TYPE_NONE {
				num_intfs[resp.GetSpec().GetType()] += 1
			}
		}
		printIfSummary(num_intfs)
	} else if cmd != nil && cmd.Flags().Changed("id") {
		for _, resp := range respMsg.Response {
			respType := resp.GetSpec().GetType().String()
			respType = strings.Replace(respType, "IF_TYPE_", "", -1)
			printIfHeader(respType)
			printIf(resp)
			fmt.Printf("\nNumber of ifs : 1\n")
		}
	} else if cmd != nil && cmd.Flags().Changed("type") {
		printIfHeader(ifType)
		respType := ""
		count := 0
		for _, resp := range respMsg.Response {
			respType = resp.GetSpec().GetType().String()
			respType = strings.Replace(respType, "IF_TYPE_", "", -1)
			if strings.ToLower(respType) == ifType {
				printIf(resp)
				count += 1
			}
		}
		fmt.Printf("\nNumber of %s ifs : %d\n", ifType, count)
	} else {
		// print all if types
		var num_intfs [pds.IfType_IF_TYPE_HOST + 1]int
		printIfHeader("uplink")
		for _, resp := range respMsg.Response {
			respType := resp.GetSpec().GetType().String()
			respType = strings.Replace(respType, "IF_TYPE_", "", -1)
			if strings.ToLower(respType) == "uplink" {
				printIf(resp)
			}
		}
		fmt.Printf("\n")
		printIfHeader("l3")
		for _, resp := range respMsg.Response {
			respType := resp.GetSpec().GetType().String()
			respType = strings.Replace(respType, "IF_TYPE_", "", -1)
			if strings.ToLower(respType) == "l3" {
				printIf(resp)
			}
		}
		fmt.Printf("\n")
		printIfHeader("control")
		for _, resp := range respMsg.Response {
			respType := resp.GetSpec().GetType().String()
			respType = strings.Replace(respType, "IF_TYPE_", "", -1)
			if strings.ToLower(respType) == "control" {
				printIf(resp)
			}
		}
		fmt.Printf("\n")
		printIfHeader("host")
		for _, resp := range respMsg.Response {
			respType := resp.GetSpec().GetType().String()
			respType = strings.Replace(respType, "IF_TYPE_", "", -1)
			num_intfs[resp.GetSpec().GetType()] += 1
			if strings.ToLower(respType) == "host" {
				printIf(resp)
			}
		}
		fmt.Printf("\n")
		printIfSummary(num_intfs)
	}
	fmt.Printf("\n")
}

func validateIfTypeStr(str string) bool {
	switch str {
	case "uplink":
		return true
	case "l3":
		return true
	case "control":
		return true
	case "host":
		return true
	default:
		return false
	}

}

func printIfSummary(count [pds.IfType_IF_TYPE_HOST + 1]int) {
	var i int32
	total := 0
	for i = 1; i <= int32(pds.IfType_IF_TYPE_HOST); i++ {
		x := pds.IfType(i)
		ifStr := strings.ToLower(strings.Replace(strings.Replace(x.String(), "IF_TYPE_", "", -1), "_", "-", -1))
		keyStr := fmt.Sprintf("Number of %s ifs ", ifStr)
		fmt.Printf("%-24s: %d\n", keyStr, count[i])
		total += count[i]
	}
	fmt.Printf("%-24s: %d\n", "Total number of ifs ", total)
}

func printIfHeader(str string) {
	switch str {
	case "uplink":
		hdrLine := strings.Repeat("-", 136)
		fmt.Println(hdrLine)
		fmt.Printf("%-40s%-14s%-14s%-11s%-11s%-40s%-6s\n",
			"ID", "IfIndex", "Interface", "AdminState", "OperState", "Port", "LifID")
		fmt.Println(hdrLine)
	case "control":
		hdrLine := strings.Repeat("-", 128)
		fmt.Println(hdrLine)
		fmt.Printf("%-40s%-14s%-14s%-11s%-11s%-18s%-20s\n",
			"ID", "IfIndex", "Interface", "AdminState", "OperState",
			"IPPrefix", "MACAddress")
		fmt.Println(hdrLine)
	case "l3":
		hdrLine := strings.Repeat("-", 222)
		fmt.Println(hdrLine)
		fmt.Printf("%-40s%-14s%-14s%-11s%-11s%-40s%-40s%-18s%-14s%-20s\n",
			"ID", "IfIndex", "Interface", "AdminState", "OperState", "Port",
			"VPC", "IPPrefix", "Encap", "MACAddress")
		fmt.Println(hdrLine)
	case "host":
		hdrLine := strings.Repeat("-", 168)
		fmt.Println(hdrLine)
		fmt.Printf("%-40s%-14s%-14s%-20s%-40s%-40s\n",
			"ID", "IfIndex", "Interface", "MACAddress", "Lif", "Policer")
		fmt.Println(hdrLine)
	}
}

func printIf(intf *pds.Interface) {
	spec := intf.GetSpec()
	status := intf.GetStatus()
	ifIndex := status.GetIfIndex()
	ifStr := ifIndexToPortIdStr(ifIndex)
	switch spec.GetType() {
	case pds.IfType_IF_TYPE_UPLINK:
		adminState := strings.Replace(spec.GetAdminStatus().String(),
			"IF_STATUS_", "", -1)
		adminState = strings.Replace(adminState, "_", "-", -1)
		operState := strings.Replace(status.GetOperStatus().String(),
			"IF_STATUS_", "", -1)
		operState = strings.Replace(operState, "_", "-", -1)
		lifId := fmt.Sprint(status.GetUplinkIfStatus().GetLifId())
		portNum := utils.IdToStr(spec.GetUplinkSpec().GetPortId())
		fmt.Printf("%-40s0x%-12x%-14s%-11s%-11s%-40s%-6s\n",
			utils.IdToStr(spec.GetId()), ifIndex, ifStr,
			adminState, operState, portNum, lifId)
	case pds.IfType_IF_TYPE_L3:
		adminState := strings.Replace(spec.GetAdminStatus().String(),
			"IF_STATUS_", "", -1)
		adminState = strings.Replace(adminState, "_", "-", -1)
		operState := strings.Replace(status.GetOperStatus().String(),
			"IF_STATUS_", "", -1)
		operState = strings.Replace(operState, "_", "-", -1)
		portNum := utils.IdToStr(spec.GetL3IfSpec().GetPortId())
		vpc := utils.IdToStr(spec.GetL3IfSpec().GetVpcId())
		ipPrefix := utils.IPPrefixToStr(spec.GetL3IfSpec().GetPrefix())
		mac := utils.MactoStr(spec.GetL3IfSpec().GetMACAddress())
		encap := utils.EncapToString(spec.GetL3IfSpec().GetEncap())
		fmt.Printf("%-40s0x%-12x%-14s%-11s%-11s%-40s%-40s%-18s%-14s%-20s\n",
			utils.IdToStr(spec.GetId()), ifIndex, ifStr, adminState, operState,
			portNum, vpc, ipPrefix, encap, mac)
	case pds.IfType_IF_TYPE_CONTROL:
		adminState := strings.Replace(spec.GetAdminStatus().String(),
			"IF_STATUS_", "", -1)
		adminState = strings.Replace(adminState, "_", "-", -1)
		operState := strings.Replace(status.GetOperStatus().String(),
			"IF_STATUS_", "", -1)
		operState = strings.Replace(operState, "_", "-", -1)
		ipPrefix := utils.IPPrefixToStr(spec.GetControlIfSpec().GetPrefix())
		mac := utils.MactoStr(spec.GetControlIfSpec().GetMACAddress())
		fmt.Printf("%-40s0x%-12x%-14s%-11s%-11s%-18s%-20s\n",
			utils.IdToStr(spec.GetId()), ifIndex, ifStr,
			adminState, operState, ipPrefix, mac)
	case pds.IfType_IF_TYPE_HOST:
		policer := utils.IdToStr(spec.GetHostIfSpec().GetTxPolicer())
		lifs := status.GetHostIfStatus().GetLifId()
		first := true
		for _, lif := range lifs {
			if first {
				fmt.Printf("%-40s0x%-12x%-14s%-20s%-40s%-40s\n",
					utils.IdToStr(spec.GetId()), ifIndex,
					status.GetHostIfStatus().GetName(),
					utils.MactoStr(status.GetHostIfStatus().GetMacAddress()),
					utils.IdToStr(lif), policer)
				first = false
			} else {
				fmt.Printf("%-88s%-40s%-40s\n",
					"", utils.IdToStr(lif), "")
			}
		}
	}
}

func printIfLldpInfo(intf *pds.Interface, cmdType int, numIntfs *int) {
	spec := intf.GetSpec()
	status := intf.GetStatus()
	ifIndex := status.GetIfIndex()
	ifStr := ifIndexToPortIdStr(ifIndex)
	portNum := utils.IdToStr(spec.GetUplinkSpec().GetPortId())

	var lldpStatus *pds.LldpIfSpec
	if cmdType == lldpCmdTypeNbrs {
		// show neighbors
		lldpStatus = status.GetUplinkIfStatus().GetLldpStatus().GetLldpNbrStatus()
	} else if cmdType == lldpCmdTypeLocalIfs {
		// lldp show interfaces
		lldpStatus = status.GetUplinkIfStatus().GetLldpStatus().GetLldpIfStatus()
	} else if cmdType == lldpCmdTypeStats {
		// lldp show statistics
		return
	}

	if len(lldpStatus.GetIfName()) == 0 {
		return
	}
	*numIntfs += 1

	nbrProto := strings.Replace(lldpStatus.GetProto().String(),"LLDP_MODE_", "", -1)
	fmt.Printf("Interface : %s, Port : %s\n", ifStr, portNum)
	rId := lldpStatus.GetRouterId()
	if rId == 0 {
	   rId = 1
	}
	fmt.Printf("  LLDP Interface : %s, via: %s, RID: %s, Time: %s\n",
		   lldpStatus.GetIfName(), nbrProto, fmt.Sprint(rId), lldpStatus.GetAge())
	fmt.Printf("    Chassis :\n")
	cid := lldpStatus.GetLldpIfChassisSpec().GetChassisId()
	cidType := strings.Replace(cid.GetType().String(),"LLDPID_SUBTYPE_", "", -1)
	cidType = strings.ToLower(cidType)
	cidValue := cid.GetValue()
	fmt.Printf("      ChassisID  : %s %s\n", cidType, cidValue)
	fmt.Printf("      SysName    : %s\n", lldpStatus.GetLldpIfChassisSpec().GetSysName())
	fmt.Printf("      SysDescr   : %s\n", lldpStatus.GetLldpIfChassisSpec().GetSysDescr())
	fmt.Printf("      MgmtIP     : %s\n",
	    utils.IPAddrToStr(lldpStatus.GetLldpIfChassisSpec().GetMgmtIP()))

	capabilities := lldpStatus.GetLldpIfChassisSpec().GetCapability()
	for _, cap := range capabilities {
	    capType := strings.Replace(cap.GetCapType().String(),"LLDP_CAPTYPE_", "", -1)
	    capType = strings.Title(strings.ToLower(capType))
	    capEnabledStr := "off"
	    if cap.GetCapEnabled() {
		capEnabledStr = "on"
	    }
	    fmt.Printf("      Capability : %s, %s\n", capType, capEnabledStr)
	}
	fmt.Printf("    Port :\n");
	pid := lldpStatus.GetLldpIfPortSpec().GetPortId()
	pidType := strings.Replace(pid.GetType().String(),"LLDPID_SUBTYPE_", "", -1)
	pidType =strings.ToLower(pidType)
	pidValue := pid.GetValue()
	fmt.Printf("      PortID     : %s %s\n", pidType, pidValue)
	fmt.Printf("      PortDescr  : %s\n", lldpStatus.GetLldpIfPortSpec().GetPortDescr())
	ttl := fmt.Sprint(lldpStatus.GetLldpIfPortSpec().GetTtl())
	if cmdType == lldpCmdTypeNbrs {
	    fmt.Printf("      TTL        : %s\n\n", ttl)
	} else {
	    fmt.Printf("    TTL	         : 120\n\n")
	}
}

func lldpShowCmdHandler(cmd *cobra.Command, args []string, cmdType int) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	var req *pds.InterfaceGetRequest

	if cmd == nil || cmd.Flags().Changed("id") == false {
		req = &pds.InterfaceGetRequest{
			Id: [][]byte{},
		}
	} else {
		req = &pds.InterfaceGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(ifID).Bytes()},
		}
	}

	client := pds.NewIfSvcClient(c)

	respMsg, err := client.InterfaceGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Get Interface failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	num_intfs := 0
	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else {
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetType() == pds.IfType_IF_TYPE_UPLINK {
				printIfLldpInfo(resp, cmdType, &num_intfs)
			}
		}
		fmt.Printf("%-24s: %d\n", "Total number of ifs ", num_intfs)
	}
}

func lldpShowNeighborsCmdHandler(cmd *cobra.Command, args []string) {
     lldpShowCmdHandler(cmd, args, lldpCmdTypeNbrs)
}

func lldpShowInterfacesCmdHandler(cmd *cobra.Command, args []string) {
     lldpShowCmdHandler(cmd, args, lldpCmdTypeLocalIfs)
}

func lldpShowStatisticsCmdHandler(cmd *cobra.Command, args []string) {
     lldpShowCmdHandler(cmd, args, lldpCmdTypeStats)
}

func lifGetNameFromKey(key []byte) string {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		return "-"
	}
	defer c.Close()

	invalidUuid := make([]byte, 16)
	if bytes.Equal(key, invalidUuid) {
		return "-"
	}

	req := &pds.LifGetRequest{
		Id: [][]byte{key},
	}

	client := pds.NewIfSvcClient(c)
	respMsg, err := client.LifGet(context.Background(), req)
	if err != nil {
		return "-"
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		return "-"
	}
	resp := respMsg.Response[0]
	return resp.GetStatus().GetName()
}

func lifShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	var req *pds.LifGetRequest

	if cmd == nil || cmd.Flags().Changed("id") == false {
		req = &pds.LifGetRequest{
			Id: [][]byte{},
		}
	} else {
		req = &pds.LifGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(lifID).Bytes()},
		}
	}

	client := pds.NewIfSvcClient(c)

	respMsg, err := client.LifGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Get Lif failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		printLifSummary(len(respMsg.Response))
	} else {
		printLifHeader()
		for _, resp := range respMsg.Response {
			printLif(resp)
		}
		printLifSummary(len(respMsg.Response))
	}
}

func printLifSummary(count int) {
	fmt.Printf("\nNo. of lifs : %d\n\n", count)
}

func printLifHeader() {
	hdrLine := strings.Repeat("-", 172)
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-10s%-15s%-20s%-40s%-25s%-11s%-11s\n",
		"ID", "IfIndex", "Name", "MAC Address", "PinnedInterface",
		"Type", "Oper State", "Admin State")
	fmt.Println(hdrLine)
}

func printLif(lif *pds.Lif) {
	spec := lif.GetSpec()
	status := lif.GetStatus()
	lifType := strings.Replace(spec.GetType().String(), "LIF_TYPE_", "", -1)
	lifType = strings.Replace(lifType, "_", "-", -1)
	state := strings.Replace(status.GetStatus().String(), "IF_STATUS_", "", -1)
	adminState := strings.Replace(status.GetAdminState().String(), "IF_STATUS_", "", -1)
	fmt.Printf("%-40s%-10x%-15s%-20s%-40s%-25s%-11s%-11s\n",
		utils.IdToStr(spec.GetId()), status.GetIfIndex(),
		status.GetName(), utils.MactoStr(spec.GetMacAddress()),
		utils.IdToStr(spec.GetPinnedInterface()), lifType, state,
		adminState)

}
