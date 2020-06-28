//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	// "net"
	// "os"
	"reflect"
	// "sort"
	"strings"

	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/agent/cmd/halctl/utils"
	"github.com/pensando/sw/nic/agent/netagent/datapath/halproto"
)

const (
	lldpCmdTypeNbrs     = 0
	lldpCmdTypeLocalIfs = 1
	lldpCmdTypeStats    = 2
)

var ()

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

func init() {
	showCmd.AddCommand(lldpShowCmd)
	lldpShowCmd.AddCommand(lldpShowNeighborsCmd)
	lldpShowCmd.AddCommand(lldpShowInterfacesCmd)
	lldpShowCmd.AddCommand(lldpShowStatisticsCmd)

	lldpShowCmd.Flags().Bool("yaml", true, "Output in yaml")
}

func printIfLldpInfo(resp *halproto.InterfaceGetResponse, cmdType int, numIntfs *int) {
	status := resp.GetStatus()
	ifStr := ifRespToStr(resp)

	var lldpStatus *halproto.LldpIfStatus
	if cmdType == lldpCmdTypeStats {
		// lldp show statistics
		stats := resp.GetStats()
		lldpStats := stats.GetUplinkIfStats().GetLldpIfStats()
		lldpStatus = status.GetUplinkInfo().GetLldpStatus().GetLldpIfStatus()

		if len(lldpStatus.GetIfName()) == 0 {
			return
		}
		*numIntfs++

		fmt.Printf("Interface : %s\n", ifStr)
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
		lldpStatus = status.GetUplinkInfo().GetLldpStatus().GetLldpNbrStatus()
	} else if cmdType == lldpCmdTypeLocalIfs {
		// lldp show interfaces
		lldpStatus = status.GetUplinkInfo().GetLldpStatus().GetLldpIfStatus()
	}

	if len(lldpStatus.GetIfName()) == 0 {
		return
	}
	*numIntfs++

	nbrProto := strings.Replace(lldpStatus.GetProto().String(), "LLDP_MODE_", "", -1)
	fmt.Printf("Interface : %s, Port : %s\n", ifStr, portNum)
	rID := lldpStatus.GetRouterId()
	if rID == 0 {
		rID = 1
	}
	fmt.Printf("  LLDP Interface : %s, via: %s, RID: %s, Time: %s\n",
		lldpStatus.GetIfName(), nbrProto, fmt.Sprint(rID), lldpStatus.GetAge())
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

	var req *halproto.InterfaceGetRequest

	if cmd == nil || cmd.Flags().Changed("id") == false {
		req = &halproto.InterfaceGetRequest{}
	} else {
		req = &halproto.InterfaceGetRequest{
			KeyOrHandle: &halproto.InterfaceKeyHandle{
				KeyOrHandle: &halproto.InterfaceKeyHandle_InterfaceId{
					InterfaceId: ifID,
				},
			},
		}
	}

	ifGetReqMsg := &halproto.InterfaceGetRequestMsg{
		Request: []*halproto.InterfaceGetRequest{req},
	}
	client := halproto.NewInterfaceClient(c)

	// HAL call
	respMsg, err := client.InterfaceGet(context.Background(), ifGetReqMsg)
	if err != nil {
		fmt.Printf("Getting if failed. %v\n", err)
		return
	}

	numIntfs := 0
	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetType() != halproto.IfType_IF_TYPE_UPLINK {
				continue
			}
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else {
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetType() != halproto.IfType_IF_TYPE_UPLINK {
				continue
			}
			printIfLldpInfo(resp, cmdType, &numIntfs)
		}
		fmt.Printf("%-24s: %d\n", "Total number of ifs ", numIntfs)
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
