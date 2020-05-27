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

var (
	lifID string
	ifID  string
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

func init() {
	showCmd.AddCommand(lifShowCmd)
	showCmd.AddCommand(ifShowCmd)
	lifShowCmd.Flags().StringVar(&lifID, "id", "", "Specify Lif ID")
	lifShowCmd.Flags().Bool("yaml", true, "Output in yaml")
	lifShowCmd.Flags().Bool("summary", false, "Display number of objects")
	ifShowCmd.Flags().StringVar(&ifID, "id", "", "Specify Lif ID")
	ifShowCmd.Flags().Bool("yaml", true, "Output in yaml")
	ifShowCmd.Flags().Bool("summary", false, "Display number of objects")

	clearCmd.AddCommand(lifClearCmd)
	lifClearCmd.AddCommand(lifClearStatsCmd)
	lifClearStatsCmd.Flags().StringVar(&lifID, "id", "", "Specify Lif ID")
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
		fmt.Printf("Get Lif failed, err %v\n", err)
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
	} else if cmd != nil && cmd.Flags().Changed("id") {
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetType() == pds.IfType_IF_TYPE_NONE {
				fmt.Printf("For ETH type interface, use 'pdsctl show port status --port <id>'\n")
				return
			}
		}
    } else if cmd != nil && cmd.Flags().Changed("summary") {
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetType() != pds.IfType_IF_TYPE_NONE {
				num_intfs += 1
			}
		}
		printIfSummary(num_intfs)
	} else {
		printIfHeader()
		for _, resp := range respMsg.Response {
			if resp.GetSpec().GetType() != pds.IfType_IF_TYPE_NONE {
				num_intfs += 1
				printIf(resp)
			}
		}
		printIfSummary(num_intfs)
	}
}

func printIfSummary(count int) {
	fmt.Printf("\nNo. of ifs : %d\n\n", count)
}

func printIfHeader() {
	hdrLine := strings.Repeat("-", 188)
	fmt.Println(hdrLine)
	fmt.Printf("%-14s%-14s%-11s%-11s%-40s%-6s%-40s%-18s%-14s%-20s\n",
		"IfIndex", "Interface", "AdminState", "OperState", "Port", "LifID",
		"VPC", "IPPrefix", "Encap", "MACAddress")
	fmt.Println(hdrLine)
}

func printIf(intf *pds.Interface) {
	spec := intf.GetSpec()
	status := intf.GetStatus()
	ifIndex := status.GetIfIndex()
	ifStr := ifIndexToPortIdStr(ifIndex)
	adminState := strings.Replace(spec.GetAdminStatus().String(),
		"IF_STATUS_", "", -1)
	adminState = strings.Replace(adminState, "_", "-", -1)
	operState := strings.Replace(status.GetOperStatus().String(),
		"IF_STATUS_", "", -1)
	operState = strings.Replace(operState, "_", "-", -1)
	portNum := ""
	vpc := "-"
	ipPrefix := "-"
	encap := "-"
	mac := "-"
	lifId := "-"
	switch spec.GetType() {
	case pds.IfType_IF_TYPE_UPLINK:
		lifId = fmt.Sprint(status.GetUplinkIfStatus().GetLifId())
		portNum = utils.IdToStr(spec.GetUplinkSpec().GetPortId())
	case pds.IfType_IF_TYPE_L3:
		portNum = utils.IdToStr(spec.GetL3IfSpec().GetPortId())
		vpc = utils.IdToStr(spec.GetL3IfSpec().GetVpcId())
		ipPrefix = utils.IPPrefixToStr(spec.GetL3IfSpec().GetPrefix())
		mac = utils.MactoStr(spec.GetL3IfSpec().GetMACAddress())
		encap = utils.EncapToString(spec.GetL3IfSpec().GetEncap())
	case pds.IfType_IF_TYPE_CONTROL:
		ipPrefix = utils.IPPrefixToStr(spec.GetControlIfSpec().GetPrefix())
		mac = utils.MactoStr(spec.GetControlIfSpec().GetMACAddress())
	}
	fmt.Printf("0x%-12x%-14s%-11s%-11s%-40s%-6s%-40s%-18s%-14s%-20s\n",
		ifIndex, ifStr, adminState, operState, portNum, lifId,
		vpc, ipPrefix, encap, mac)
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
