//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	"reflect"
	"strconv"
	"strings"

	uuid "github.com/satori/go.uuid"
	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/apollo/agent/cli/utils"
	"github.com/pensando/sw/nic/apollo/agent/gen/pds"
)

const (
	ifTypeNone        = 0
	ifTypeEth         = 1
	ifTypeEthPC       = 2
	ifTypeTunnel      = 3
	ifTypeUplink      = 5
	ifTypeUplinkPC    = 6
	ifTypeL3          = 7
	ifTypeLif         = 8
	ifTypeLoopback    = 9
	ifTypeControl     = 10
	ifTypeHost        = 11
	ifTypeShift       = 28
	ifSlotShift       = 24
	ifParentPortShift = 16
	ifTypeMask        = 0xF
	ifSlotMask        = 0xF
	ifParentPortMask  = 0xFF
	ifChildPortMask   = 0xFF
	ifNameDelimiter   = "/"
	invalidIfIndex    = 0xFFFFFFFF
)

var (
	portID         string
	portAdminState string
	portFecType    string
	portAutoNeg    string
	portSpeed      string
	portNumLanes   uint32
	portPause      string
	txPause        string
	rxPause        string
	portMtu        uint32
)

var portShowCmd = &cobra.Command{
	Use:   "port",
	Short: "show port information",
	Long:  "show port object information",
}

var portStatsShowCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show port statistics",
	Long:  "show port statistics",
	Run:   portShowCmdHandler,
}

var portStatusShowCmd = &cobra.Command{
	Use:   "status",
	Short: "show port status",
	Long:  "show port status",
	Run:   portShowStatusCmdHandler,
}

var portFsmShowCmd = &cobra.Command{
	Use:   "fsm",
	Short: "show port fsm",
	Long:  "show port fsm",
	Run:   portShowFsmCmdHandler,
}

var portUpdateCmd = &cobra.Command{
	Use:   "port",
	Short: "update port information",
	Long:  "update port information",
	Run:   portUpdateCmdHandler,
}

var portShowXcvrCmd = &cobra.Command{
	Use:   "transceiver",
	Short: "show port transceiver",
	Long:  "show port transceiver",
	Run:   portXcvrShowCmdHandler,
}

var portInternalShowCmd = &cobra.Command{
	Use:   "internal",
	Short: "show internal port information",
	Long:  "show information about the ports on the internal switch",
	Run:   portInternalCmdHandler,
}

var portInternalStatsCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show internal port statistics",
	Long:  "show statistics about the ports on the internal switch",
	Run:   portInternalStatsCmdHandler,
}

var portClearCmd = &cobra.Command{
	Use:   "port",
	Short: "clear port information",
	Long:  "clear port information",
}

var portClearStatsCmd = &cobra.Command{
	Use:   "statistics",
	Short: "clear port statistics",
	Long:  "clear port statistics",
	Run:   portClearStatsCmdHandler,
}

// ValidateNumLanes returns true if the numLanes is between 1 to 4
func ValidateNumLanes(numLanes uint32) bool {
	if numLanes < 1 || numLanes > 4 {
		fmt.Printf("Invalid num-lanes. num-lanes must be in the range 1-4\n")
		return false
	}
	return true
}

// ValidateMtu returns true if the MTU is within 64-9216
func ValidateMtu(mtu uint32) bool {
    if mtu < 64 || mtu > 9216 {
        fmt.Printf("Invalid MTU. MTU must be in the range 64-9216\n")
        return false
    }
    return true
}

func isPauseTypeValid(str string) bool {
    switch str {
    case "link-level":
        return true
    case "link":
        return true
    case "pfc":
        return true
    case "none":
        return true
    default:
        return false
    }
}

func inputToPauseType(str string) pds.PortPauseType {
    switch str {
    case "link-level":
        return pds.PortPauseType_PORT_PAUSE_TYPE_LINK
    case "link":
        return pds.PortPauseType_PORT_PAUSE_TYPE_LINK
    case "pfc":
        return pds.PortPauseType_PORT_PAUSE_TYPE_PFC
    case "none":
        return pds.PortPauseType_PORT_PAUSE_TYPE_NONE
    default:
        return pds.PortPauseType_PORT_PAUSE_TYPE_NONE
    }
}

func init() {
	showCmd.AddCommand(portShowCmd)
	portShowCmd.AddCommand(portStatsShowCmd)
	portStatsShowCmd.Flags().StringVarP(&portID, "port", "p", "", "Specify port uuid")
	portStatsShowCmd.Flags().Bool("yaml", true, "Output in yaml")
	portShowCmd.AddCommand(portStatusShowCmd)
	portStatusShowCmd.Flags().StringVarP(&portID, "port", "p", "", "Specify port uuid")
	portStatusShowCmd.Flags().Bool("yaml", true, "Output in yaml")
	portStatusShowCmd.Flags().Bool("summary", false, "Display number of objects")
	portShowCmd.AddCommand(portShowXcvrCmd)
	portShowXcvrCmd.Flags().StringVarP(&portID, "port", "p", "", "Specify port uuid")
	portShowXcvrCmd.Flags().Bool("yaml", true, "Output in yaml")
	portShowXcvrCmd.Flags().Bool("summary", false, "Display number of objects")
	portShowCmd.AddCommand(portInternalShowCmd)
	portInternalShowCmd.PersistentFlags().StringVarP(&portID, "port", "p", "", "Specify port number. eg 1 to 7 for internal ports")
	portInternalShowCmd.AddCommand(portInternalStatsCmd)
	portShowCmd.AddCommand(portFsmShowCmd)
	portFsmShowCmd.Flags().StringVarP(&portID, "port", "p", "", "Specify port uuid")

	debugUpdateCmd.AddCommand(portUpdateCmd)
	portUpdateCmd.Flags().StringVarP(&portID, "port", "p", "", "Specify port uuid")
	portUpdateCmd.Flags().StringVarP(&portAdminState, "admin-state", "a", "up", "Set port admin state - up, down")
	portUpdateCmd.Flags().StringVar(&portFecType, "fec-type", "none", "Specify fec-type - rs, fc, none")
	portUpdateCmd.Flags().StringVar(&portAutoNeg, "auto-neg", "enable", "Enable or disable auto-neg using enable | disable")
	portUpdateCmd.Flags().StringVar(&portSpeed, "speed", "", "Set port speed - none, 1g, 10g, 25g, 40g, 50g, 100g")
	portUpdateCmd.Flags().Uint32Var(&portNumLanes, "num-lanes", 4, "Specify number of lanes")
	portUpdateCmd.Flags().StringVar(&portPause, "pause-type", "none", "Specify pause-type - link, pfc, none")
	portUpdateCmd.Flags().StringVar(&txPause, "tx-pause", "disable", "Enable or disable TX pause using enable | disable")
	portUpdateCmd.Flags().StringVar(&rxPause, "rx-pause", "disable", "Enable or disable RX pause using enable | disable")
	portUpdateCmd.Flags().Uint32Var(&portMtu, "mtu", 0, "Specify port MTU")
	portUpdateCmd.MarkFlagRequired("port")

	clearCmd.AddCommand(portClearCmd)
	portClearCmd.AddCommand(portClearStatsCmd)
	portClearStatsCmd.Flags().StringVarP(&portID, "port", "p", "", "Specify port uuid")
}

func portClearStatsCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewPortSvcClient(c)
	var req *pds.Id
	if cmd != nil && cmd.Flags().Changed("port") {
		// clear stats for given port
		req = &pds.Id{
			Id: uuid.FromStringOrNil(portID).Bytes(),
		}
	} else {
		// clear stats for all ports
		req = &pds.Id{
			Id: []byte{},
		}
	}

	// PDS call
	_, err = client.PortStatsReset(context.Background(), req)
	if err != nil {
		fmt.Printf("Clearing port stats failed, err %v\n", err)
		return
	}
}

func portUpdateCmdHandler(cmd *cobra.Command, args []string) {
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

	if cmd.Flags().Changed("admin-state") == false &&
		cmd.Flags().Changed("auto-neg") == false &&
		cmd.Flags().Changed("speed") == false &&
		cmd.Flags().Changed("fec-type") == false &&
		cmd.Flags().Changed("num-lanes") == false &&
		cmd.Flags().Changed("mtu") == false &&
		cmd.Flags().Changed("pause-type") == false &&
		cmd.Flags().Changed("tx-pause") == false &&
		cmd.Flags().Changed("rx-pause") == false {
		fmt.Printf("Command arguments not provided correctly, refer to help string for guidance\n")
		return
	}

	var mtu uint32
	var debounceTimeout uint32
	var numLanes uint32
	adminState := inputToAdminState("none")
	speed := inputToSpeed("none")
	fecType := inputToFecType("none")
	autoNeg := false
	pauseType := pds.PortPauseType_PORT_PAUSE_TYPE_NONE
	loopbackMode := pds.PortLoopBackMode_PORT_LOOPBACK_MODE_NONE
	portType := pds.PortType_PORT_TYPE_NONE
	txPauseEn := false
	rxPauseEn := false

	if cmd.Flags().Changed("fec-type") == true {
		if isFecTypeValid(portFecType) == false {
			fmt.Printf("Command arguments not provided correctly, refer to help string for guidance\n")
			return
		}
		fecType = inputToFecType(portFecType)
	}

	if cmd.Flags().Changed("auto-neg") == true {
		if strings.Compare(portAutoNeg, "disable") == 0 {
			autoNeg = false
		} else if strings.Compare(portAutoNeg, "enable") == 0 {
			autoNeg = true
		} else {
			fmt.Printf("Command arguments not provided correctly, refer to help string for guidance\n")
			return
		}
	}

	if cmd.Flags().Changed("admin-state") == true {
		if isAdminStateValid(portAdminState) == false {
			fmt.Printf("Command arguments not provided correctly, refer to help string for guidance\n")
			return
		}
		adminState = inputToAdminState(portAdminState)
	}

	if cmd.Flags().Changed("speed") == true {
		if isSpeedValid(strings.ToUpper(portSpeed)) == false {
			fmt.Printf("Command arguments not provided correctly, refer to help string for guidance\n")
			return
		}
		speed = inputToSpeed(strings.ToUpper(portSpeed))
	}

	if cmd.Flags().Changed("num-lanes") == true {
		if ValidateNumLanes(portNumLanes) == false {
			return
		}
		numLanes = portNumLanes
	}

    if cmd.Flags().Changed("pause-type") == true {
        if isPauseTypeValid(portPause) == false {
            fmt.Printf("Command arguments not provided correctly. Refer to help string for guidance\n")
            return
        }
        pauseType = inputToPauseType(portPause)
    }

    if cmd.Flags().Changed("tx-pause") == true {
        if strings.Compare(txPause, "disable") == 0 {
            txPauseEn = false
        } else if strings.Compare(txPause, "enable") == 0 {
            txPauseEn = true
        } else {
            fmt.Printf("Command arguments not provided correctly. Refer to help string for guidance\n")
            return
        }
    }

   if cmd.Flags().Changed("rx-pause") == true {
        if strings.Compare(rxPause, "disable") == 0 {
            rxPauseEn = false
        } else if strings.Compare(rxPause, "enable") == 0 {
            rxPauseEn = true
        } else {
            fmt.Printf("Command arguments not provided correctly. Refer to help string for guidance\n")
            return
        }
    }

    if cmd.Flags().Changed("mtu") == true {
        if ValidateMtu(portMtu) == false {
            return
        }
        mtu = portMtu
    }

	client := pds.NewPortSvcClient(c)

	getReq := &pds.PortGetRequest{
		Id: [][]byte{uuid.FromStringOrNil(portID).Bytes()},
	}
	getRespMsg, err := client.PortGet(context.Background(), getReq)
	if err != nil {
		fmt.Printf("Getting Port failed, err %v\n", err)
		return
	}

	if getRespMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", getRespMsg.ApiStatus)
		return
	}

	for _, resp := range getRespMsg.Response {
		if cmd.Flags().Changed("fec-type") == false {
			fecType = resp.GetSpec().GetFECType()
		}
		if cmd.Flags().Changed("auto-neg") == false && cmd.Flags().Changed("speed") == false {
			autoNeg = resp.GetSpec().GetAutoNegEn()
		}
		if cmd.Flags().Changed("admin-state") == false {
			adminState = resp.GetSpec().GetAdminState()
		}
		if cmd.Flags().Changed("speed") == false {
			speed = resp.GetSpec().GetSpeed()
		}
		debounceTimeout = resp.GetSpec().GetDeBounceTimeout()
		if cmd.Flags().Changed("mtu") == false {
			mtu = resp.GetSpec().GetMtu()
		}
		if cmd.Flags().Changed("pause-type") == false {
			pauseType = resp.GetSpec().GetPauseType()
		}
		loopbackMode = resp.GetSpec().GetLoopbackMode()
		if cmd.Flags().Changed("num-lanes") == false {
			numLanes = resp.GetSpec().GetNumLanes()
		}
		portType = resp.GetSpec().GetType()
		if cmd.Flags().Changed("tx-pause") == false {
			txPauseEn = resp.GetSpec().GetTxPauseEn()
		}
		if cmd.Flags().Changed("rx-pause") == false {
			rxPauseEn = resp.GetSpec().GetRxPauseEn()
		}
	}

	var req *pds.PortUpdateRequest

	req = &pds.PortUpdateRequest{
		Spec: &pds.PortSpec{
			Id:              uuid.FromStringOrNil(portID).Bytes(),
			AdminState:      adminState,
			Type:            portType,
			Speed:           speed,
			FECType:         fecType,
			AutoNegEn:       autoNeg,
			DeBounceTimeout: debounceTimeout,
			Mtu:             mtu,
			PauseType:       pauseType,
			TxPauseEn:       txPauseEn,
			RxPauseEn:       rxPauseEn,
			LoopbackMode:    loopbackMode,
			NumLanes:        numLanes,
		},
	}

	// PDS call
	respMsg, err := client.PortUpdate(context.Background(), req)
	if err != nil {
		fmt.Printf("Update Port failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	fmt.Printf("Update port succeeded\n")
}

func isAdminStateValid(str string) bool {
	switch str {
	case "up":
		return true
	case "down":
		return true
	default:
		return false
	}
}

func inputToAdminState(str string) pds.PortAdminState {
	switch str {
	case "up":
		return pds.PortAdminState_PORT_ADMIN_STATE_UP
	case "down":
		return pds.PortAdminState_PORT_ADMIN_STATE_DOWN
	default:
		return pds.PortAdminState_PORT_ADMIN_STATE_NONE
	}
}

func isFecTypeValid(str string) bool {
	switch str {
	case "none":
		return true
	case "rs":
		return true
	case "fc":
		return true
	default:
		return false
	}
}

func inputToFecType(str string) pds.PortFecType {
	switch str {
	case "none":
		return pds.PortFecType_PORT_FEC_TYPE_NONE
	case "rs":
		return pds.PortFecType_PORT_FEC_TYPE_RS
	case "fc":
		return pds.PortFecType_PORT_FEC_TYPE_FC
	default:
		return pds.PortFecType_PORT_FEC_TYPE_NONE
	}
}

func isSpeedValid(str string) bool {
	switch str {
	case "none":
		return true
	case "1G":
		return true
	case "10G":
		return true
	case "25G":
		return true
	case "40G":
		return true
	case "50G":
		return true
	case "100G":
		return true
	default:
		return false
	}
}

func inputToSpeed(str string) pds.PortSpeed {
	switch str {
	case "none":
		return pds.PortSpeed_PORT_SPEED_NONE
	case "1G":
		return pds.PortSpeed_PORT_SPEED_1G
	case "10G":
		return pds.PortSpeed_PORT_SPEED_10G
	case "25G":
		return pds.PortSpeed_PORT_SPEED_25G
	case "40G":
		return pds.PortSpeed_PORT_SPEED_40G
	case "50G":
		return pds.PortSpeed_PORT_SPEED_50G
	case "100G":
		return pds.PortSpeed_PORT_SPEED_100G
	default:
		return pds.PortSpeed_PORT_SPEED_NONE
	}
}

func portShowStatusCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewPortSvcClient(c)
	yamlOutput := (cmd != nil) && cmd.Flags().Changed("yaml")

	var req *pds.PortGetRequest
	if cmd != nil && cmd.Flags().Changed("port") {
		req = &pds.PortGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(portID).Bytes()},
		}
	} else {
		// Get all Ports
		req = &pds.PortGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.PortGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Port failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	if yamlOutput {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		printPortStatusSummary(len(respMsg.Response))
	} else {
		printPortStatusHeader()
		// Print Ports
		for _, resp := range respMsg.Response {
			printPortStatus(resp)
		}
		printPortStatusSummary(len(respMsg.Response))
	}
}

func printPortStatusSummary(count int) {
	fmt.Printf("\nNo. of ports : %d\n\n", count)
}

func printPortStatusHeader() {
	hdrLine := strings.Repeat("-", 196)
	fmt.Println("MAC-Info: MAC ID/MAC Channel/Num lanes")
	fmt.Println("FEC-Type: FC - FireCode, RS - ReedSolomon")
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-10s%-12s%-7s%-10s%-10s%-10s"+"%-6s%-7s%-7s%-10s%-12s"+"%-18s%-12s%-20s%-10s\n",
		"Id", "Name", "IfIndex", "Speed", "MAC-Info", "FEC", "AutoNeg",
		"MTU", "Pause", "Pause", "Debounce", "State",
		"Transceiver", "NumLinkDown", "LinkSM", "Loopback")
	fmt.Printf("%-40s%-10s%-12s%-7s%-10s%-10s%-10s"+"%-6s%-7s%-7s%-10s%-12s"+"%-18s%-12s%-20s%-10s\n",
		"", "", "", "", "", "Cfg/Oper", "Cfg/Oper",
		"", "Type", "Tx/Rx", "(msecs)", "Admin/Oper",
		"", "", "", "")
	fmt.Println(hdrLine)
}

func printPortStatus(resp *pds.Port) {
	spec := resp.GetSpec()
	status := resp.GetStatus()
	linkStatus := status.GetLinkStatus()
	stats := resp.GetStats()

	if spec == nil {
		fmt.Printf("Error! Port spec cannot be nil\n")
		return
	}

	if status == nil {
		fmt.Printf("Error! Port status cannot be nil\n")
		return
	}

	if linkStatus == nil {
		fmt.Printf("Error! Port link status cannot be nil\n")
		return
	}

	if stats == nil {
		fmt.Printf("Error! Port stats cannot be nil\n")
		return
	}

	adminState := spec.GetAdminState()
	operStatus := linkStatus.GetOperState()
	xcvrStatus := status.GetXcvrStatus()
	speed := spec.GetSpeed()
	fecTypeCfg := spec.GetFECType()
	fecTypeOper := linkStatus.GetFECType()
	mtu := spec.GetMtu()
	debounce := spec.GetDeBounceTimeout()

	adminStateStr := strings.Replace(adminState.String(), "PORT_ADMIN_STATE_", "", -1)
	operStatusStr := strings.Replace(operStatus.String(), "PORT_OPER_STATUS_", "", -1)
	speedStr := strings.Replace(speed.String(), "PORT_SPEED_", "", -1)
	fecCfgStr := strings.Replace(fecTypeCfg.String(), "PORT_FEC_TYPE_", "", -1)
	fecOperStr := strings.Replace(fecTypeOper.String(), "PORT_FEC_TYPE_", "", -1)
	loopbackModeStr := strings.Replace(spec.GetLoopbackMode().String(), "PORT_LOOPBACK_MODE_", "", -1)
	pauseStr := strings.Replace(spec.GetPauseType().String(), "PORT_PAUSE_TYPE_", "", -1)
	macStr := fmt.Sprintf("%d/%d/%d", status.GetMacId(), status.GetMacCh(), linkStatus.GetNumLanes())
	fsmStr := strings.Replace(status.GetFSMState().String(), "PORT_LINK_FSM_", "", -1)

	xcvrPortNum := xcvrStatus.GetPort()
	xcvrState := xcvrStatus.GetState()
	xcvrPid := xcvrStatus.GetPid()
	xcvrStateStr := "-"
	xcvrPidStr := "-"
	xcvrStr := ""
	if xcvrPortNum > 0 && (strings.Compare(xcvrState.String(), "65535") != 0) {
		// Strip XCVR_STATE_ from the state
		xcvrStateStr = strings.Replace(strings.Replace(xcvrState.String(), "XCVR_STATE_", "", -1), "_", "-", -1)
		// Strip XCVR_PID_ from the pid
		xcvrPidStr = strings.Replace(strings.Replace(xcvrPid.String(), "XCVR_PID_", "", -1), "_", "-", -1)
	}

	if strings.Compare(xcvrStateStr, "SPROM-READ") == 0 {
		xcvrStr = xcvrPidStr
	} else {
		xcvrStr = xcvrStateStr
	}

	outStr := fmt.Sprintf("%-40s%-10s0x%-10x%-7s%-10s%4s/%-5s",
		utils.IdToStr(spec.GetId()),
		ifIndexToPortIdStr(status.GetIfIndex()), status.GetIfIndex(),
		speedStr, macStr, fecCfgStr, fecOperStr)
	outStr += fmt.Sprintf("%2s/%-7s%-6d%-7s%2s/%-4s",
		utils.BoolToString(spec.GetAutoNegEn()), utils.BoolToString(linkStatus.GetAutoNegEn()),
		mtu, pauseStr, utils.BoolToString(spec.GetTxPauseEn()), utils.BoolToString(spec.GetRxPauseEn()))
	outStr += fmt.Sprintf("%-10d%4s/%-7s%-18s%-12d%-20s%-10s",
		debounce, adminStateStr, operStatusStr, xcvrStr, stats.GetNumLinkDown(),
		fsmStr, loopbackModeStr)
	fmt.Printf(outStr + "\n")
}

func portShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewPortSvcClient(c)
	yamlOutput := (cmd != nil) && cmd.Flags().Changed("yaml")

	var req *pds.PortGetRequest
	if cmd != nil && cmd.Flags().Changed("port") {
		req = &pds.PortGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(portID).Bytes()},
		}
	} else {
		// Get all Ports
		req = &pds.PortGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.PortGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Port failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	if yamlOutput {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else {
		printPortStatsHeader()
		// Print Ports
		for _, resp := range respMsg.Response {
			printPortStats(resp)
		}
	}
}

func printPortStatsHeader() {
	hdrLine := strings.Repeat("-", 37)
	fmt.Println(hdrLine)
	fmt.Printf("%-8s%-25s%-5s\n", "PortId", "Field", "Count")
	fmt.Println(hdrLine)
}

func portIDStrToIfIndex(portIDStr string) uint32 {
	var slotIndex uint32
	var portIndex uint32
	var ifIndex uint32

	portIDStr = strings.ToLower(portIDStr)
	n, err := fmt.Sscanf(portIDStr, "eth%d/%d", &slotIndex, &portIndex)
	if err != nil || n != 2 {
		return invalidIfIndex
	}
	ifIndex = 1 << ifTypeShift // 1 is Eth port
	ifIndex |= slotIndex << ifSlotShift
	ifIndex |= portIndex << ifParentPortShift
	ifIndex |= 1 // Default child port

	return ifIndex
}

func ifIndexToSlot(ifIndex uint32) uint32 {
	return (ifIndex >> ifSlotShift) & ifSlotMask
}

func ifIndexToParentPort(ifIndex uint32) uint32 {
	return (ifIndex >> ifParentPortShift) & ifParentPortMask
}

func ifIndexToChildPort(ifIndex uint32) uint32 {
	return ifIndex & ifChildPortMask
}

func ifIndexToIfType(ifindex uint32) string {
	ifType := (ifindex >> ifTypeShift) & ifTypeMask
	switch ifType {
	case ifTypeNone:
		return "None"
	case ifTypeEth:
		return "Eth"
	case ifTypeEthPC:
		return "EthPC"
	case ifTypeTunnel:
		return "Tunnel"
	case ifTypeLoopback:
		return "lo"
	case ifTypeUplink:
		return "Uplink"
	case ifTypeUplinkPC:
		return "UplinkPC"
	case ifTypeL3:
		return "L3"
	case ifTypeLif:
		return "Lif"
	case ifTypeHost:
		return "Host"
	case ifTypeControl:
		return "Control"
	}
	return "None"
}

func ifIndexToID(ifIndex uint32) uint32 {
	return ifIndex &^ (ifTypeMask << ifTypeShift)
}

func ifIndexToPortIdStr(ifIndex uint32) string {
	if ifIndex == 0 {
		return "-"
	}
	ifType := (ifIndex >> ifTypeShift) & ifTypeMask
	ifTypeStr := ifIndexToIfType(ifIndex)
	switch ifType {
	case ifTypeEth, ifTypeUplink:
		slotStr := strconv.FormatUint(uint64(ifIndexToSlot(ifIndex)), 10)
		parentPortStr := strconv.FormatUint(uint64(ifIndexToParentPort(ifIndex)), 10)
		return ifTypeStr + slotStr + ifNameDelimiter + parentPortStr
	case ifTypeHost, ifTypeEthPC, ifTypeTunnel, ifTypeL3, ifTypeLif, ifTypeLoopback, ifTypeControl:
		return ifTypeStr + strconv.FormatUint(uint64(ifIndexToID(ifIndex)), 10)
	}
	return "-"
}

func printPortStats(resp *pds.Port) {
	first := true
	macStats := resp.GetStats().GetMacStats()
	mgmtMacStats := resp.GetStats().GetMgmtMacStats()

	fmt.Printf("%-8s", ifIndexToPortIdStr(resp.GetStatus().GetIfIndex()))
	for _, s := range macStats {
		if first == false {
			fmt.Printf("%-8s", "")
		}
		fmt.Printf("%-25s%-5d\n",
			strings.Replace(s.GetType().String(), "_", " ", -1),
			s.GetCount())
		first = false
	}

	first = true
	for _, s := range mgmtMacStats {
		if first == false {
			fmt.Printf("%-8s", "")
		}
		str := strings.Replace(s.GetType().String(), "MGMT_MAC_", "", -1)
		str = strings.Replace(str, "_", " ", -1)
		fmt.Printf("%-25s%-5d\n", str, s.GetCount())
		first = false
	}
}

func portXcvrShowSummary(count int) {
	fmt.Printf("\nNo. of ports : %d\n\n", count)
}

func portXcvrShowResp(resp *pds.Port) {
	status := resp.GetStatus()
	if status == nil {
		fmt.Printf("Error! Port status cannot be nil\n")
		return
	}

	xcvrStatus := status.GetXcvrStatus()
	if xcvrStatus == nil {
		// No xcvr connected to this phy port
		return
	}

	xcvrPortNum := xcvrStatus.GetPort()
	xcvrState := xcvrStatus.GetState()
	xcvrPid := xcvrStatus.GetPid()

	xcvrStateStr := "-"
	xcvrPidStr := "-"

	if xcvrPortNum <= 0 {
		return
	}

	// Strip XCVR_STATE_ from the state
	xcvrStateStr = strings.Replace(strings.Replace(xcvrState.String(), "XCVR_STATE_", "", -1), "_", "-", -1)

	// Strip XCVR_PID_ from the pid
	xcvrPidStr = strings.Replace(strings.Replace(xcvrPid.String(), "XCVR_PID_", "", -1), "_", "-", -1)

	xcvrSprom := xcvrStatus.GetXcvrSprom()

	lengthOm3 := 0
	vendorRev := ""
	lengthSmfKm := int(xcvrSprom[14])
	lengthOm2 := int(xcvrSprom[16])
	lengthOm1 := int(xcvrSprom[17])
	lengthDac := int(xcvrSprom[18])
	vendorName := string(xcvrSprom[20:36])
	vendorPn := string(xcvrSprom[40:56])
	vendorSn := string(xcvrSprom[68:84])

	if strings.Contains(xcvrPid.String(), "QSFP") {
		// convert from units of 2m to meters
		lengthOm3 = int(xcvrSprom[15]) * 2

		vendorRev = string(xcvrSprom[56:58])
	} else {
		lengthOm3 = int(xcvrSprom[19])
		vendorRev = string(xcvrSprom[56:60])

		// convert from units of 10m to meters
		lengthOm1 *= 10
		lengthOm2 *= 10
		lengthOm3 *= 10
	}

	fmt.Printf("%-30s: %d\n", "Port", xcvrPortNum)
	fmt.Printf("%-30s: %s\n", "Id", utils.IdToStr(resp.GetSpec().GetId()))
	fmt.Printf("%-30s: %s\n", "State", xcvrStateStr)
	fmt.Printf("%-30s: %s\n", "PID", xcvrPidStr)
	fmt.Printf("%-30s: %d KM\n", "Length Single Mode Fiber", lengthSmfKm)
	fmt.Printf("%-30s: %d Meters\n", "Length 62.5um OM1 Fiber", lengthOm1)
	fmt.Printf("%-30s: %d Meters\n", "Length 50um   OM2 Fiber", lengthOm2)
	fmt.Printf("%-30s: %d Meters\n", "Length 50um   OM3 Fiber", lengthOm3)
	fmt.Printf("%-30s: %d Meters\n", "Length Copper", lengthDac)
	fmt.Printf("%-30s: %s\n", "vendor name", vendorName)
	fmt.Printf("%-30s: %s\n", "vendor part number", vendorPn)
	fmt.Printf("%-30s: %s\n", "vendor revision", vendorRev)
	fmt.Printf("%-30s: %s\n\n", "vendor serial number", vendorSn)
}

func portXcvrShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewPortSvcClient(c)
	yamlOutput := (cmd != nil) && cmd.Flags().Changed("yaml")

	var req *pds.PortGetRequest
	if cmd != nil && cmd.Flags().Changed("port") {
		req = &pds.PortGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(portID).Bytes()},
		}
	} else {
		// Get all Ports
		req = &pds.PortGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.PortGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Port failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	if yamlOutput {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		portXcvrShowSummary(len(respMsg.Response))
	} else {
		for _, resp := range respMsg.Response {
			portXcvrShowResp(resp)
		}
		portXcvrShowSummary(len(respMsg.Response))
	}
}

func portInternalCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewDebugSvcClient(c)
	if (cmd != nil) && cmd.Flags().Changed("yaml") {
		fmt.Printf("yaml option not supported for internal ports")
		return
	}

	var req *pds.InternalPortRequest
	var intPortNum uint64

	if cmd != nil && cmd.Flags().Changed("port") {
		intPortNum, err = strconv.ParseUint(portID, 10, 8)
		if (err != nil) || (intPortNum < 1) || (intPortNum > 7) {
			fmt.Printf("Invalid argument, port number must be between 1 - 7 for internal ports\n")
			return
		}
		// Get port info for specified port
		req = &pds.InternalPortRequest{
			PortNumber: uint32(intPortNum),
		}
	} else {
		// Get all Ports
		req = &pds.InternalPortRequest{}
	}

	reqMsg := &pds.InternalPortRequestMsg{
		Request: []*pds.InternalPortRequest{req},
	}

	// HAL call
	respMsg, err := client.InternalPortGet(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Getting Internal port status failed, err %v\n", err)
		return
	}

	hdrLine := strings.Repeat("-", 90)
	fmt.Println(hdrLine)
	fmt.Printf("%-10s%-25s%-15s%-10s%-12s%-10s%-10s\n", "PortNo", "Descr", "Status", "Speed", "Mode", "Pause", "FlowCtrl")
	fmt.Println(hdrLine)

	// Print Result
	for _, resp := range respMsg.Response {
		fmt.Printf("%-10d%-25s%-15s%-10s%-12s%-10v%-10v\n",
			resp.GetPortNumber(), resp.GetPortDescr(),
			strings.TrimPrefix(resp.GetInternalStatus().GetPortStatus().String(), "IF_STATUS_"),
			strings.TrimPrefix(resp.GetInternalStatus().GetPortSpeed().String(), "PORT_SPEED_"),
			strings.TrimSuffix(resp.GetInternalStatus().GetPortMode().String(), "_DUPLEX"),
			resp.GetInternalStatus().GetPortTxPaused(), resp.GetInternalStatus().GetPortFlowCtrl())
	}
}

func portShowStatsHeader() {
	hdrLine := strings.Repeat("-", 30)
	fmt.Println(hdrLine)
	fmt.Printf("%-25s%-5s\n", "Field", "Count")
	fmt.Println(hdrLine)
}

func portInternalStatsCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	// PDS Debug call
	client := pds.NewDebugSvcClient(c)

	var req *pds.InternalPortRequest
	var intPortNum uint64

	if cmd != nil && cmd.Flags().Changed("port") {
		intPortNum, err = strconv.ParseUint(portID, 10, 8)
		if (err != nil) || (intPortNum < 1) || (intPortNum > 7) {
			fmt.Printf("Invalid argument, port number must be between 1 - 7 for internal ports\n")
			return
		}
		// Get port info for specified port
		req = &pds.InternalPortRequest{
			PortNumber: uint32(intPortNum),
		}
	} else {
		// Get all Ports
		req = &pds.InternalPortRequest{}
	}

	reqMsg := &pds.InternalPortRequestMsg{
		Request: []*pds.InternalPortRequest{req},
	}

	// HAL call
	respMsg, err := client.InternalPortGet(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Getting Internal port status failed, err %v\n", err)
		return
	}

	hdrLine := strings.Repeat("-", 30)
	portShowStatsHeader()
	// Print Result
	for _, resp := range respMsg.Response {
		fmt.Printf("\nstats for port : %v\n\n", resp.GetPortNumber())
		stats := resp.GetStats()
		respVal := reflect.ValueOf(stats).Elem()
		respType := respVal.Type()
		for i := 0; i < respType.NumField(); i++ {
			fmt.Printf("%-15s: %-20v\n", respType.Field(i).Name, respVal.Field(i).Interface())
		}
		fmt.Println(hdrLine)
	}
}

func portShowFsmCmdHandler(cmd *cobra.Command, args []string) {
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

	var filter *pds.CommandUUID
	if cmd != nil && cmd.Flags().Changed("port") {
		filter = &pds.CommandUUID{
			Id: uuid.FromStringOrNil(portID).Bytes(),
		}
	}
	var cmdResp *pds.ServiceResponseMessage

	// handle command
	if filter == nil {
		cmdResp, err = HandleSvcReqCommandMsg(pds.Command_CMD_PORT_FSM_DUMP, nil)
	} else {
		cmdResp, err = HandleSvcReqCommandMsg(pds.Command_CMD_PORT_FSM_DUMP, filter)
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
