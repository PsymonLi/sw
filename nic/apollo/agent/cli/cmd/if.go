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
	lldpCmdTypeNbrs     = 0
	lldpCmdTypeLocalIfs = 1
	lldpCmdTypeStats    = 2
)

var (
	lifID               string
	ifID                string
	ifType              string
	txPolicerID         string
	ifPpsTrackingAction string
)

var lifShowCmd = &cobra.Command{
	Use:   "lif",
	Short: "show lif information",
	Long:  "show lif object information",
	Run:   lifShowCmdHandler,
}

var lifStatsShowCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show lif statistics",
	Long:  "show lif object information",
	Run:   lifStatsShowCmdHandler,
}

var ifShowCmd = &cobra.Command{
	Use:     "interface",
	Short:   "show interface information",
	Long:    "show interface object information",
	Run:     ifShowCmdHandler,
	PreRunE: ifShowCmdPreRun,
}

var ifStatsShowCmd = &cobra.Command{
	Use:     "pps-statistics",
	Short:   "show interface pps-statistics",
	Long:    "show interface pps-statistics",
	Run:     ifStatsShowCmdHandler,
	PreRunE: ifStatsShowCmdPreRun,
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

var ifDebugCmd = &cobra.Command{
	Use:   "interface",
	Short: "interface debug commands",
	Long:  "interface debug commands",
}

var ifPpsTrackingCmd = &cobra.Command{
	Use:   "pps-tracking",
	Short: "interface PPS tracking",
	Long:  "interface PPS tracking",
	Run:   ifPpsTrackingCmdHandler,
}

func init() {
	showCmd.AddCommand(lifShowCmd)
	showCmd.AddCommand(ifShowCmd)
	showCmd.AddCommand(lldpShowCmd)
	lldpShowCmd.AddCommand(lldpShowNeighborsCmd)
	lldpShowCmd.AddCommand(lldpShowInterfacesCmd)
	lldpShowCmd.AddCommand(lldpShowStatisticsCmd)
	lldpShowCmd.Flags().Bool("yaml", true, "Output in yaml")

	lifShowCmd.Flags().StringVar(&lifID, "id", "", "Specify Lif ID")
	lifShowCmd.Flags().Bool("yaml", true, "Output in yaml")
	lifShowCmd.Flags().Bool("summary", false, "Display number of objects")

	lifShowCmd.AddCommand(lifStatsShowCmd)
	lifStatsShowCmd.Flags().StringVar(&lifID, "id", "", "Specify Lif ID")

	ifShowCmd.Flags().StringVar(&ifType, "type", "", "Specify interface type (uplink, control, host, l3)")
	ifShowCmd.Flags().StringVar(&ifID, "id", "", "Specify interface ID")
	ifShowCmd.Flags().Bool("yaml", true, "Output in yaml")
	ifShowCmd.Flags().Bool("summary", false, "Display number of objects")

	ifShowCmd.AddCommand(ifStatsShowCmd)
	ifStatsShowCmd.Flags().StringVar(&ifType, "type", "uplink", "Specify interface type (uplink)")
	ifStatsShowCmd.Flags().StringVar(&ifID, "id", "", "Specify interface ID")

	clearCmd.AddCommand(lifClearCmd)
	lifClearCmd.AddCommand(lifClearStatsCmd)
	lifClearStatsCmd.Flags().StringVar(&lifID, "id", "", "Specify Lif ID")

	debugUpdateCmd.AddCommand(ifUpdateCmd)
	ifUpdateCmd.Flags().StringVar(&ifID, "id", "", "Specify interface ID")
	ifUpdateCmd.Flags().StringVar(&txPolicerID, "tx-policer", "", "Specify tx policer ID")
	ifUpdateCmd.MarkFlagRequired("id")
	ifUpdateCmd.MarkFlagRequired("tx-policer")

	debugCmd.AddCommand(ifDebugCmd)
	ifDebugCmd.AddCommand(ifPpsTrackingCmd)
	ifPpsTrackingCmd.Flags().StringVar(&ifPpsTrackingAction, "action", "", "Specify interface pps tracking action (enable | disable)")
	ifPpsTrackingCmd.MarkFlagRequired("action")
}

func ifPpsTrackingCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewDebugSvcClient(c)
	var empty *pds.Empty
	if strings.ToLower(ifPpsTrackingAction) == "enable" {
		_, err = client.InterfacePpsTrackingEnable(context.Background(), empty)
		if err != nil {
			fmt.Printf("Interface pps tracking enable failed, err %v\n", err)
			return
		}
		fmt.Printf("Interface pps tracking enabled\n")
	} else if strings.ToLower(ifPpsTrackingAction) == "disable" {
		_, err = client.InterfacePpsTrackingDisable(context.Background(), empty)
		if err != nil {
			fmt.Printf("Interface pps tracking disable failed, err %v\n", err)
			return
		}
		fmt.Printf("Interface pps tracking disabled\n")
	} else {
		fmt.Printf("Invalid argument specified. Refer to help string\n")
		cmd.Help()
	}
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
	if ifResp[0].GetSpec().GetHostIfSpec() == nil {
		fmt.Printf("Only update of  host interface supported\n")
		return
	}

	spec := &pds.InterfaceSpec{
		Id:          uuid.FromStringOrNil(ifID).Bytes(),
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

func ifShowCmdPreRun(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}

	if cmd.Flags().Changed("type") && !validateIfTypeStr(ifType) {
		return fmt.Errorf("Invalid argument for \"type\", please choose from [host, l3, uplink, control]\n")
	}
	if !cmd.Flags().Changed("id") &&
		!cmd.Flags().Changed("type") &&
		!cmd.Flags().Changed("summary") {
		return fmt.Errorf("Required flag(s) \"type\" or \"id\" have/has not been set\n")
	}
	return nil
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
		var intfType pds.IfType
		for _, resp := range respMsg.Response {
			switch resp.GetSpec().GetIfinfo().(type) {
			case *pds.InterfaceSpec_UplinkSpec:
				intfType = pds.IfType_IF_TYPE_UPLINK
			case *pds.InterfaceSpec_L3IfSpec:
				intfType = pds.IfType_IF_TYPE_L3
			case *pds.InterfaceSpec_ControlIfSpec:
				intfType = pds.IfType_IF_TYPE_CONTROL
			case *pds.InterfaceSpec_HostIfSpec:
				intfType = pds.IfType_IF_TYPE_HOST
			default:
				intfType = pds.IfType_IF_TYPE_NONE
			}
			if intfType != pds.IfType_IF_TYPE_NONE {
				num_intfs[intfType] += 1
			}
		}
		printIfSummary(num_intfs)
	} else if cmd != nil && cmd.Flags().Changed("id") {
		for _, resp := range respMsg.Response {
			switch resp.GetSpec().GetIfinfo().(type) {
			case *pds.InterfaceSpec_UplinkSpec:
				printIfHeader("uplink")
			case *pds.InterfaceSpec_L3IfSpec:
				printIfHeader("l3")
			case *pds.InterfaceSpec_ControlIfSpec:
				printIfHeader("control")
			case *pds.InterfaceSpec_HostIfSpec:
				printIfHeader("host")
			default:
			}
			printIf(resp)
		}
	} else if cmd != nil && cmd.Flags().Changed("type") {
		printIfHeader(ifType)
		count := 0
		for _, resp := range respMsg.Response {
			if (ifType == "uplink" && resp.GetSpec().GetUplinkSpec() != nil) ||
				(ifType == "l3" && resp.GetSpec().GetL3IfSpec() != nil) ||
				(ifType == "control" && resp.GetSpec().GetControlIfSpec() != nil) ||
				(ifType == "host" && resp.GetSpec().GetHostIfSpec() != nil) {
				printIf(resp)
				count += 1
			}
		}
		fmt.Printf("\nNumber of %s interfaces : %d\n", ifType, count)
	} else {
		// print all if types
		var num_intfs [pds.IfType_IF_TYPE_HOST + 1]int
		printIfHeader("uplink")
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetUplinkSpec() != nil {
				printIf(resp)
				num_intfs[pds.IfType_IF_TYPE_UPLINK] += 1
			}
		}
		fmt.Printf("\n")
		printIfHeader("l3")
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetL3IfSpec() != nil {
				printIf(resp)
				num_intfs[pds.IfType_IF_TYPE_L3] += 1
			}
		}
		fmt.Printf("\n")
		printIfHeader("control")
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetControlIfSpec() != nil {
				printIf(resp)
				num_intfs[pds.IfType_IF_TYPE_CONTROL] += 1
			}
		}
		fmt.Printf("\n")
		printIfHeader("host")
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetHostIfSpec() != nil {
				printIf(resp)
				num_intfs[pds.IfType_IF_TYPE_HOST] += 1
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
		keyStr := fmt.Sprintf("Number of %s interfaces ", ifStr)
		fmt.Printf("%-32s    : %d\n", keyStr, count[i])
		total += count[i]
	}
	fmt.Printf("%-32s    : %d\n", "Total number of interfaces ", total)
}

func printIfHeader(str string) {
	switch strings.ToLower(str) {
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
		hdrLine := strings.Repeat("-", 178)
		fmt.Println(hdrLine)
		fmt.Printf("%-40s%-14s%-14s%-20s%-40s%-40s%-10s\n",
			"ID", "IfIndex", "Interface", "MACAddress", "Lif", "Policer", "AdminState")
		fmt.Println(hdrLine)
	}
}

func printIf(intf *pds.Interface) {
	spec := intf.GetSpec()
	status := intf.GetStatus()
	ifIndex := status.GetIfIndex()
	ifStr := ifIndexToPortIdStr(ifIndex)
	switch spec.GetIfinfo().(type) {
	case *pds.InterfaceSpec_UplinkSpec:
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
	case *pds.InterfaceSpec_L3IfSpec:
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
	case *pds.InterfaceSpec_ControlIfSpec:
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
	case *pds.InterfaceSpec_HostIfSpec:
		policer := utils.IdToStr(spec.GetHostIfSpec().GetTxPolicer())
		lifs := status.GetHostIfStatus().GetLifId()
		adminState := strings.Replace(spec.GetAdminStatus().String(),
			"IF_STATUS_", "", -1)
		first := true
		for _, lif := range lifs {
			if first {
				fmt.Printf("%-40s0x%-12x%-14s%-20s%-40s%-40s%-10s\n",
					utils.IdToStr(spec.GetId()), ifIndex,
					status.GetHostIfStatus().GetName(),
					utils.MactoStr(status.GetHostIfStatus().GetMacAddress()),
					utils.IdToStr(lif), policer, adminState)
				first = false
			} else {
				fmt.Printf("%-88s%-40s%-40s%-10s\n",
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

	var lldpStatus *pds.LldpIfStatus
	if cmdType == lldpCmdTypeStats {
		// lldp show statistics
		stats := intf.GetStats()
		lldpStats := stats.GetUplinkIfStats().GetLldpIfStats()
		lldpStatus = status.GetUplinkIfStatus().GetLldpStatus().GetLldpIfStatus()

		if len(lldpStatus.GetIfName()) == 0 {
			return
		}
		*numIntfs += 1

		fmt.Printf("Interface : %s, Port : %s\n", ifStr, portNum)
		fmt.Printf("  LLDP Interface : %s\n", lldpStatus.GetIfName())
		fmt.Printf("    TxCount        : %s\n", fmt.Sprint(lldpStats.GetTxCount()))
		fmt.Printf("    RxCount        : %s\n", fmt.Sprint(lldpStats.GetRxCount()))
		fmt.Printf("    RxDiscarded    : %s\n", fmt.Sprint(lldpStats.GetRxDiscarded()))
		fmt.Printf("    RxUnrecognized : %s\n", fmt.Sprint(lldpStats.GetRxUnrecognized()))
		fmt.Printf("    AgeoutCount    : %s\n", fmt.Sprint(lldpStats.GetAgeoutCount()))
		fmt.Printf("    InsertCount    : %s\n", fmt.Sprint(lldpStats.GetInsertCount()))
		fmt.Printf("    DeleteCount    : %s\n\n", fmt.Sprint(lldpStats.GetDeleteCount()))
		return
	}

	if cmdType == lldpCmdTypeNbrs {
		// show neighbors
		lldpStatus = status.GetUplinkIfStatus().GetLldpStatus().GetLldpNbrStatus()
	} else if cmdType == lldpCmdTypeLocalIfs {
		// lldp show interfaces
		lldpStatus = status.GetUplinkIfStatus().GetLldpStatus().GetLldpIfStatus()
	}

	if len(lldpStatus.GetIfName()) == 0 {
		return
	}
	*numIntfs += 1

	nbrProto := strings.Replace(lldpStatus.GetProto().String(), "LLDP_MODE_", "", -1)
	fmt.Printf("Interface : %s, Port : %s\n", ifStr, portNum)
	rId := lldpStatus.GetRouterId()
	if rId == 0 {
		rId = 1
	}
	fmt.Printf("  LLDP Interface : %s, via: %s, RID: %s, Time: %s\n",
		lldpStatus.GetIfName(), nbrProto, fmt.Sprint(rId), lldpStatus.GetAge())
	fmt.Printf("    Chassis :\n")
	cid := lldpStatus.GetLldpIfChassisStatus().GetChassisId()
	cidType := strings.Replace(cid.GetType().String(), "LLDPID_SUBTYPE_", "", -1)
	cidType = strings.ToLower(cidType)
	cidValue := cid.GetValue()
	fmt.Printf("      ChassisID  : %s %s\n", cidType, cidValue)
	fmt.Printf("      SysName    : %s\n", lldpStatus.GetLldpIfChassisStatus().GetSysName())
	fmt.Printf("      SysDescr   : %s\n", lldpStatus.GetLldpIfChassisStatus().GetSysDescr())
	fmt.Printf("      MgmtIP     : %s\n",
		utils.IPAddrToStr(lldpStatus.GetLldpIfChassisStatus().GetMgmtIP()))

	capabilities := lldpStatus.GetLldpIfChassisStatus().GetCapability()
	for _, cap := range capabilities {
		capType := strings.Replace(cap.GetCapType().String(), "LLDP_CAPTYPE_", "", -1)
		capType = strings.Title(strings.ToLower(capType))
		capEnabledStr := "off"
		if cap.GetCapEnabled() {
			capEnabledStr = "on"
		}
		fmt.Printf("      Capability : %s, %s\n", capType, capEnabledStr)
	}
	fmt.Printf("    Port :\n")
	pid := lldpStatus.GetLldpIfPortStatus().GetPortId()
	pidType := strings.Replace(pid.GetType().String(), "LLDPID_SUBTYPE_", "", -1)
	pidType = strings.ToLower(pidType)
	pidValue := pid.GetValue()
	fmt.Printf("      PortID     : %s %s\n", pidType, pidValue)
	fmt.Printf("      PortDescr  : %s\n", lldpStatus.GetLldpIfPortStatus().GetPortDescr())
	ttl := fmt.Sprint(lldpStatus.GetLldpIfPortStatus().GetTtl())
	if cmdType == lldpCmdTypeNbrs {
		fmt.Printf("      TTL        : %s\n", ttl)
	} else {
		fmt.Printf("    TTL	         : %s\n", ttl)
	}
	unknownTlvs := lldpStatus.GetLldpUnknownTlvStatus()
	if len(unknownTlvs) > 0 {
		fmt.Printf("    Unknown TLVs :\n")
		for _, tlv := range unknownTlvs {
			ouiStr := tlv.GetOui()
			subType := fmt.Sprint(tlv.GetSubtype())
			len := fmt.Sprint(tlv.GetLen())
			value := fmt.Sprint(tlv.GetValue())
			fmt.Printf("      TLV        : OUI %s, SubType %s, Len %s, %s\n", ouiStr, subType, len, value)
		}
	}
	fmt.Printf("\n")
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
			if resp.GetSpec().GetUplinkSpec() != nil {
				printIfLldpInfo(resp, cmdType, &num_intfs)
			}
		}
		fmt.Printf("%-24s: %d\n", "Total number of interfaces ", num_intfs)
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

func hostIfGetNameFromKey(key []byte) string {
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

	req := &pds.InterfaceGetRequest{
		Id: [][]byte{key},
	}

	client := pds.NewIfSvcClient(c)
	respMsg, err := client.InterfaceGet(context.Background(), req)
	if err != nil {
		return "-"
	}
	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		return "-"
	}
	resp := respMsg.Response[0]
	if resp.GetStatus().GetHostIfStatus() == nil {
		return "-"
	}
	return resp.GetStatus().GetHostIfStatus().GetName()
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

func ifStatsShowCmdPreRun(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	if cmd.Flags().Changed("type") &&
		strings.ToLower(ifType) != "uplink" {
		return fmt.Errorf("Invalid argument for \"type\", only uplink supported for now\n")
	}
	return nil
}

func ifStatsShowCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	var filter *pds.InterfaceDumpFilter
	if cmd != nil && cmd.Flags().Changed("id") {
		filter = &pds.InterfaceDumpFilter{
			Ifinfo: &pds.InterfaceDumpFilter_Id{
				Id: &pds.CommandUUID{
					Id: uuid.FromStringOrNil(ifID).Bytes(),
				},
			},
		}
	} else {
		filter = &pds.InterfaceDumpFilter{
			Ifinfo: &pds.InterfaceDumpFilter_IfType{
				IfType: pds.IfType_IF_TYPE_UPLINK,
			},
		}
	}

	// handle cmd
	cmdResp, err := HandleSvcReqCommandMsg(pds.Command_CMD_IF_STATS_DUMP, filter)
	if err != nil {
		fmt.Printf("Command failed with %v error\n", err)
		return
	}
	if cmdResp.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Command failed with %v error\n", cmdResp.ApiStatus)
		return
	}
}

func lifStatsShowCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	var filter *pds.CommandUUID
	if cmd != nil && cmd.Flags().Changed("id") {
		filter = &pds.CommandUUID{
			Id: uuid.FromStringOrNil(lifID).Bytes(),
		}
	} else {
		filter = nil
	}

	// handle cmd
	var cmdResp *pds.ServiceResponseMessage
	var err error
	if filter == nil {
		cmdResp, err = HandleSvcReqCommandMsg(pds.Command_CMD_LIF_STATS_DUMP, nil)
	} else {
		cmdResp, err = HandleSvcReqCommandMsg(pds.Command_CMD_LIF_STATS_DUMP, filter)
	}
	if err != nil {
		fmt.Printf("Command failed with %v error\n", err)
		return
	}
	if cmdResp.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Command failed with %v error\n", cmdResp.ApiStatus)
		return
	}
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
	hdrLine := strings.Repeat("-", 180)
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-10s%-15s%-20s%-40s%-25s%-10s%-11s%-9s\n",
		"ID", "IfIndex", "Name", "MAC Address", "PinnedInterface",
		"Type", "OperState", "AdminState", "VnicHwId")
	fmt.Println(hdrLine)
}

func printLif(lif *pds.Lif) {
	spec := lif.GetSpec()
	status := lif.GetStatus()
	lifType := strings.Replace(spec.GetType().String(), "LIF_TYPE_", "", -1)
	lifType = strings.Replace(lifType, "_", "-", -1)
	state := strings.Replace(status.GetStatus().String(), "IF_STATUS_", "", -1)
	name := status.GetName()
	if name == "" {
		name = "-"
	}
	vnicId := fmt.Sprint(status.GetVnicIndex())
	if vnicId == "65535" {
		vnicId = "-"
	}
	adminState := strings.Replace(status.GetAdminState().String(), "IF_STATUS_", "", -1)
	fmt.Printf("%-40s%-10x%-15s%-20s%-40s%-25s%-10s%-11s%-9s\n",
		utils.IdToStr(spec.GetId()), status.GetIfIndex(),
		name, utils.MactoStr(spec.GetMacAddress()),
		utils.IdToStr(spec.GetPinnedInterface()), lifType, state,
		adminState, vnicId)

}
