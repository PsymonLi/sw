//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	"os"
	"strconv"
	"strings"

	"github.com/spf13/cobra"

	"github.com/pensando/sw/nic/agent/cmd/halctl/utils"
	"github.com/pensando/sw/nic/agent/netagent/datapath/halproto"
)

var clearCmd = &cobra.Command{
	Use:   "clear",
	Short: "Clear Commands",
	Long:  "Clear Commands",
}

var systemClearCmd = &cobra.Command{
	Use:   "system",
	Short: "Clear system information",
	Long:  "Clear system information",
}

var systemStatsClearCmd = &cobra.Command{
	Use:   "statistics",
	Short: "clear system statistics [fte | pb | all] (Default: all)",
	Long:  "clear system statistics [fte | pb | all] (Default: all)",
	Run:   systemStatsClearCmdHandler,
}

var systemIngressDropStatsClearCmd = &cobra.Command{
	Use:   "ingress-drop",
	Short: "clear system statistics ingress-drop",
	Long:  "clear system statistics ingress-drop",
	Run:   systemIngressDropStatsClearCmdHandler,
}

var systemEgressDropStatsClearCmd = &cobra.Command{
	Use:   "egress-drop",
	Short: "clear system statistics egress-drop",
	Long:  "clear system statistics egress-drop",
	Run:   systemEgressDropStatsClearCmdHandler,
}

var systemLifStatsClearCmd = &cobra.Command{
	Use:   "lif",
	Short: "clear system statistics lif",
	Long:  "clear system statistics lif",
	Run:   systemLifStatsClearCmdHandler,
}

var systemPortStatsClearCmd = &cobra.Command{
	Use:   "port",
	Short: "clear system statistics port",
	Long:  "clear system statistics port",
	Run:   systemPortStatsClearCmdHandler,
}

var systemInternalPortStatsClearCmd = &cobra.Command{
	Use:   "internal",
	Short: "clear system statistics port internal",
	Long:  "clear system statistics port internal",
	Run:   systemInternalPortStatsClearCmdHandler,
}

var systemCoppStatsClearCmd = &cobra.Command{
	Use:   "copp",
	Short: "clear system statistics copp",
	Long:  "clear system statistics copp",
	Run:   systemCoppStatsClearCmdHandler,
}

func init() {
	rootCmd.AddCommand(clearCmd)
	clearCmd.AddCommand(systemClearCmd)
	systemClearCmd.AddCommand(systemStatsClearCmd)
	systemStatsClearCmd.AddCommand(systemIngressDropStatsClearCmd)
	systemStatsClearCmd.AddCommand(systemEgressDropStatsClearCmd)
	systemStatsClearCmd.AddCommand(systemLifStatsClearCmd)
	systemStatsClearCmd.AddCommand(systemPortStatsClearCmd)
	systemStatsClearCmd.AddCommand(systemCoppStatsClearCmd)

	systemPortStatsClearCmd.AddCommand(systemInternalPortStatsClearCmd)

	systemLifStatsClearCmd.Flags().Uint64Var(&lifID, "id", 0, "Specify lif-id")
	systemPortStatsClearCmd.PersistentFlags().StringVar(&portNum, "port", "", "Specify port number. eg eth1/1 or 1 to 7 for internal ports")

	systemCoppStatsClearCmd.Flags().StringVar(&coppType, "copptype", "flow-miss", "Specify copp type")
	systemCoppStatsClearCmd.Flags().Uint64Var(&coppHandle, "handle", 0, "Specify copp handle")

	clearCmd.AddCommand(platformClearCmd)
	platformClearCmd.AddCommand(platformHbmClearCmd)
	platformHbmClearCmd.AddCommand(platformHbmLlcStatsClearCmd)
}

func systemStatsClearCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	defer c.Close()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewSystemClient(c)

	// Check the args
	fte := false
	pb := false

	if len(args) > 0 {
		if strings.Compare(args[0], "fte") == 0 {
			fte = true
		} else if strings.Compare(args[0], "pb") == 0 {
			pb = true
		} else if strings.Compare(args[0], "all") == 0 {
			fte = true
			pb = true
		} else {
			fmt.Printf("Invalid argument\n")
			return
		}
	} else {
		fte = true
		pb = true
	}

	var empty *halproto.Empty

	if fte {
		_, err := client.ClearFteStats(context.Background(), empty)
		if err != nil {
			fmt.Printf("Clearing FTE Stats failed. %v\n", err)
			return
		}
	}

	if pb {
		_, err := client.ClearPbStats(context.Background(), empty)
		if err != nil {
			fmt.Printf("Clearing Pb Stats failed. %v\n", err)
			return
		}
	}
}

func systemIngressDropStatsClearCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	defer c.Close()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewSystemClient(c)

	var empty *halproto.Empty

	_, reterr := client.ClearIngressDropStats(context.Background(), empty)
	if reterr != nil {
		fmt.Printf("Clearing Ingress Drop Stats failed. %v\n", reterr)
		return
	}
}

func systemEgressDropStatsClearCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	defer c.Close()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewSystemClient(c)

	var empty *halproto.Empty

	_, reterr := client.ClearEgressDropStats(context.Background(), empty)
	if reterr != nil {
		fmt.Printf("Clearing Egress Drop Stats failed. %v\n", reterr)
		return
	}
}

func systemLifStatsClearCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewInterfaceClient(c)

	defer c.Close()

	var req *halproto.LifGetRequest
	if cmd.Flags().Changed("id") {
		// Get specific lif
		req = &halproto.LifGetRequest{
			KeyOrHandle: &halproto.LifKeyHandle{
				KeyOrHandle: &halproto.LifKeyHandle_LifId{
					LifId: lifID,
				},
			},
		}
	} else {
		// Get all Lifs
		req = &halproto.LifGetRequest{}
	}

	lifGetReqMsg := &halproto.LifGetRequestMsg{
		Request: []*halproto.LifGetRequest{req},
	}

	// HAL call
	respMsg, err := client.LifGet(context.Background(), lifGetReqMsg)
	if err != nil {
		fmt.Printf("Getting Lif failed. %v\n", err)
		return
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("LifGet Operation failed with %v error\n", resp.ApiStatus)
			continue
		}
		resp.GetSpec().StatsReset = true
		lifReqMsg := &halproto.LifRequestMsg{
			Request: []*halproto.LifSpec{resp.GetSpec()},
		}

		updateRespMsg, err := client.LifUpdate(context.Background(), lifReqMsg)
		if err != nil {
			fmt.Printf("Update Lif failed. %v\n", err)
			continue
		}

		for _, updateResp := range updateRespMsg.Response {
			if updateResp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
				fmt.Printf("LifUpdate Operation failed with %v error\n", updateResp.ApiStatus)
				continue
			}
		}
	}
}

func handlePortStatsClearCmd(cmd *cobra.Command, ofile *os.File) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewPortClient(c)

	var req *halproto.PortGetRequest

	if cmd != nil && cmd.Flags().Changed("port") {
		// Get port info for specified port
		req = &halproto.PortGetRequest{
			KeyOrHandle: &halproto.PortKeyHandle{
				KeyOrHandle: &halproto.PortKeyHandle_PortId{
					PortId: portIDStrToIfIndex(portNum),
				},
			},
		}
	} else {
		// Get all Ports
		req = &halproto.PortGetRequest{}
	}

	portGetReqMsg := &halproto.PortGetRequestMsg{
		Request: []*halproto.PortGetRequest{req},
	}

	// HAL call
	respMsg, err := client.PortGet(context.Background(), portGetReqMsg)
	if err != nil {
		fmt.Printf("Getting Port failed. %v\n", err)
		return
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
			continue
		}

		var portSpec *halproto.PortSpec

		portSpec = &halproto.PortSpec{
			KeyOrHandle: &halproto.PortKeyHandle{
				KeyOrHandle: &halproto.PortKeyHandle_PortId{
					PortId: resp.GetSpec().GetKeyOrHandle().GetPortId(),
				},
			},

			PortType:      resp.GetSpec().GetPortType(),
			AdminState:    resp.GetSpec().GetAdminState(),
			PortSpeed:     resp.GetSpec().GetPortSpeed(),
			NumLanes:      resp.GetSpec().GetNumLanes(),
			FecType:       resp.GetSpec().GetFecType(),
			AutoNegEnable: resp.GetSpec().GetAutoNegEnable(),
			DebounceTime:  resp.GetSpec().GetDebounceTime(),
			Mtu:           resp.GetSpec().GetMtu(),
			Pause:         resp.GetSpec().GetPause(),
			TxPauseEnable: resp.GetSpec().GetTxPauseEnable(),
			RxPauseEnable: resp.GetSpec().GetRxPauseEnable(),
			MacStatsReset: true,
		}

		portUpdateReqMsg := &halproto.PortRequestMsg{
			Request: []*halproto.PortSpec{portSpec},
		}

		// HAL call
		updateRespMsg, err := client.PortUpdate(context.Background(), portUpdateReqMsg)
		if err != nil {
			fmt.Printf("Update Port failed. %v\n", err)
			continue
		}

		for _, updateResp := range updateRespMsg.Response {
			if updateResp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
				fmt.Printf("Operation failed with %v error\n", updateResp.ApiStatus)
				continue
			}
		}
	}
}

func systemPortStatsClearCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 1 {
		fmt.Printf("Invalid argument\n")
		return
	}
	handlePortStatsClearCmd(cmd, nil)
}

func systemInternalPortStatsClearCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 1 {
		fmt.Printf("Invalid argument\n")
		return
	}
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	// HAL call
	client := halproto.NewInternalClient(c)

	var req *halproto.InternalPortRequest
	var intPortNum uint64

	if cmd != nil && cmd.Flags().Changed("port") {
		intPortNum, err = strconv.ParseUint(portNum, 10, 8)
		if (err != nil) || (intPortNum < 1) || (intPortNum > 7) {
			fmt.Printf("Invalid argument. port number must be between 1 - 7 for internal ports\n")
			return
		}
		// Get port info for specified port
		req = &halproto.InternalPortRequest{
			PortNumber: uint32(intPortNum),
		}
	} else {
		// Get all Ports
		req = &halproto.InternalPortRequest{}
	}

	reqMsg := &halproto.InternalPortRequestMsg{
		Request: []*halproto.InternalPortRequest{req},
	}

	// HAL call
	_, err = client.InternalPortStatsClear(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Clearing Internal port status failed. %v\n", err)
		return
	}

}

func systemCoppStatsClearCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewQOSClient(c)

	var req *halproto.CoppClearStatsRequest
	if cmd != nil && cmd.Flags().Changed("copptype") {
		if isCoppTypeValid(coppType) != true {
			fmt.Printf("Invalid argument\n")
			return
		}
		// ClearStats specific copp
		req = &halproto.CoppClearStatsRequest{
			KeyOrHandle: &halproto.CoppKeyHandle{
				KeyOrHandle: &halproto.CoppKeyHandle_CoppType{
					CoppType: inputToCoppType(coppType),
				},
			},
		}
	} else if cmd != nil && cmd.Flags().Changed("handle") {
		// ClearStats specific copp
		req = &halproto.CoppClearStatsRequest{
			KeyOrHandle: &halproto.CoppKeyHandle{
				KeyOrHandle: &halproto.CoppKeyHandle_CoppHandle{
					CoppHandle: coppHandle,
				},
			},
		}
	} else {
		// ClearStats all copp
		req = &halproto.CoppClearStatsRequest{}
	}

	reqMsg := &halproto.CoppClearStatsRequestMsg{
		Request: []*halproto.CoppClearStatsRequest{req},
	}

	// HAL call
	_, err = client.CoppClearStats(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Clearing Copp Stats failed. %v\n", err)
		return
	}
}

var platformClearCmd = &cobra.Command{
	Use:   "platform",
	Short: "clear platform hbm llc-stats",
	Long:  "clear platform hmb llc-stats",
}

var platformHbmClearCmd = &cobra.Command{
	Use:   "hbm",
	Short: "clear platform hbm llc-stats",
	Long:  "clear platform hmb llc-stats",
}

var platformHbmLlcStatsClearCmd = &cobra.Command{
	Use:   "llc-stats",
	Short: "clear platform hbm llc-stats",
	Long:  "clear platform hmb llc-stats",
	Run:   llcStatsClearCmdHandler,
}

func llcStatsClearCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewDebugClient(c)

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	var empty *halproto.Empty

	// HAL call
	_, err = client.LlcClear(context.Background(), empty)
	if err != nil {
		fmt.Printf("Llc clear failed. %v\n", err)
		return
	}
}
