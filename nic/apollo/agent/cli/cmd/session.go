//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	"io"
	"strings"

	"github.com/spf13/cobra"

	"github.com/pensando/sw/nic/apollo/agent/cli/utils"
	"github.com/pensando/sw/nic/apollo/agent/gen/pds"
)

var (
	sessionStatsID string
	sessionVpcID   uint32
	sessionSrcIP   string
	sessionDstIP   string
	sessionSrcPort uint32
	sessionDstPort uint32
	sessionIPProto uint32
	flowVpcID      uint32
	flowSrcIP      string
	flowDstIP      string
	flowSrcPort    uint32
	flowDstPort    uint32
	flowIPProto    uint32
)

var sessionStatsShowCmd = &cobra.Command{
	Use:   "session-stats",
	Short: "show session statistics",
	Long:  "show session statistics",
	Run:   sessionShowStatsCmdHandler,
}

var flowShowCmd = &cobra.Command{
	Use:   "flow",
	Short: "show flow information",
	Long:  "show flow object information",
	Run:   flowShowCmdHandler,
}

var sessionShowCmd = &cobra.Command{
	Use:   "session",
	Short: "show session information",
	Long:  "show session object information",
	Run:   sessionShowCmdHandler,
}

var flowClearCmd = &cobra.Command{
	Use:   "flow",
	Short: "clear flow information",
	Long:  "clear flow object information",
	Run:   flowClearCmdHandler,
}

var sessionClearCmd = &cobra.Command{
	Use:   "session",
	Short: "clear session information",
	Long:  "clear session object information",
	Run:   sessionClearCmdHandler,
}

func init() {
	showCmd.AddCommand(sessionStatsShowCmd)
	sessionStatsShowCmd.Flags().StringVarP(&sessionStatsID, "session-stats-index", "i", "", "Specify session stats index. Ex: 1-20 or 10")
	sessionStatsShowCmd.MarkFlagRequired("session-stats-index")

	showCmd.AddCommand(sessionShowCmd)
	sessionShowCmd.Flags().Uint32Var(&sessionVpcID, "vpcid", 0, "Specify VPC ID (default is 0)")
	sessionShowCmd.Flags().StringVar(&sessionSrcIP, "srcip", "0.0.0.0", "Specify session src ip")
	sessionShowCmd.Flags().StringVar(&sessionDstIP, "dstip", "0.0.0.0", "Specify session dst ip")
	sessionShowCmd.Flags().Uint32Var(&sessionSrcPort, "srcport", 0, "Specify session src port")
	sessionShowCmd.Flags().Uint32Var(&sessionDstPort, "dstport", 0, "Specify session dst port")
	sessionShowCmd.Flags().Uint32Var(&sessionIPProto, "ipproto", 0, "Specify session IP proto")

	showCmd.AddCommand(flowShowCmd)
	flowShowCmd.Flags().Uint32Var(&flowVpcID, "vpcid", 0, "Specify VPC ID (default is 0)")
	flowShowCmd.Flags().StringVar(&flowSrcIP, "srcip", "0.0.0.0", "Specify flow src ip")
	flowShowCmd.Flags().StringVar(&flowDstIP, "dstip", "0.0.0.0", "Specify flow dst ip")
	flowShowCmd.Flags().Uint32Var(&flowSrcPort, "srcport", 0, "Specify flow src port")
	flowShowCmd.Flags().Uint32Var(&flowDstPort, "dstport", 0, "Specify flow dst port")
	flowShowCmd.Flags().Uint32Var(&flowIPProto, "ipproto", 0, "Specify flow IP proto")

	clearCmd.AddCommand(sessionClearCmd)
	sessionClearCmd.Flags().Uint32Var(&sessionVpcID, "vpcid", 0, "Specify VPC ID (default is 0)")
	sessionClearCmd.Flags().StringVar(&sessionSrcIP, "srcip", "0.0.0.0", "Specify session src ip")
	sessionClearCmd.Flags().StringVar(&sessionDstIP, "dstip", "0.0.0.0", "Specify session dst ip")
	sessionClearCmd.Flags().Uint32Var(&sessionSrcPort, "srcport", 0, "Specify session src port")
	sessionClearCmd.Flags().Uint32Var(&sessionDstPort, "dstport", 0, "Specify session dst port")
	sessionClearCmd.Flags().Uint32Var(&sessionIPProto, "ipproto", 0, "Specify session IP proto")

	clearCmd.AddCommand(flowClearCmd)
	flowClearCmd.Flags().Uint32Var(&flowVpcID, "vpcid", 0, "Specify VPC ID (default is 0)")
	flowClearCmd.Flags().StringVar(&flowSrcIP, "srcip", "0.0.0.0", "Specify flow src ip")
	flowClearCmd.Flags().StringVar(&flowDstIP, "dstip", "0.0.0.0", "Specify flow dst ip")
	flowClearCmd.Flags().Uint32Var(&flowSrcPort, "srcport", 0, "Specify flow src port")
	flowClearCmd.Flags().Uint32Var(&flowDstPort, "dstport", 0, "Specify flow dst port")
	flowClearCmd.Flags().Uint32Var(&flowIPProto, "ipproto", 0, "Specify flow IP proto")
}

func sessionClearCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS. Is PDS Running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewSessionSvcClient(c)

	if cmd.Flags().Changed("vpcid") == false {
		sessionVpcID = 0
	}

	if cmd.Flags().Changed("srcip") == false {
		sessionSrcIP = "0.0.0.0"
	}

	if cmd.Flags().Changed("dstip") == false {
		sessionDstIP = "0.0.0.0"
	}

	if cmd.Flags().Changed("srcport") == false {
		sessionSrcPort = 0
	}

	if cmd.Flags().Changed("dstport") == false {
		sessionDstPort = 0
	}

	if cmd.Flags().Changed("ipproto") == false {
		sessionIPProto = 0
	}

	req := &pds.SessionClearRequest{
		Filter: &pds.SessionFilter{
			Vpc:     sessionVpcID,
			SrcAddr: utils.IPAddrStrToPDSIPAddr(sessionSrcIP),
			DstAddr: utils.IPAddrStrToPDSIPAddr(sessionDstIP),
			SrcPort: sessionSrcPort,
			DstPort: sessionDstPort,
			IPProto: sessionIPProto,
		},
	}

	// PDS call
	respMsg, err := client.SessionClear(context.Background(), req)
	if err != nil {
		fmt.Printf("Clearing sessions failed. %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	fmt.Printf("Clearing sessions succeeded\n")
}

func flowClearCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS. Is PDS Running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewSessionSvcClient(c)

	if cmd.Flags().Changed("vpcid") == false {
		flowVpcID = 0
	}

	if cmd.Flags().Changed("srcip") == false {
		flowSrcIP = "0.0.0.0"
	}

	if cmd.Flags().Changed("dstip") == false {
		flowDstIP = "0.0.0.0"
	}

	if cmd.Flags().Changed("srcport") == false {
		flowSrcPort = 0
	}

	if cmd.Flags().Changed("dstport") == false {
		flowDstPort = 0
	}

	if cmd.Flags().Changed("ipproto") == false {
		flowIPProto = 0
	}

	req := &pds.FlowClearRequest{
		Filter: &pds.FlowFilter{
			Vpc:     flowVpcID,
			SrcAddr: utils.IPAddrStrToPDSIPAddr(flowSrcIP),
			DstAddr: utils.IPAddrStrToPDSIPAddr(flowDstIP),
			SrcPort: flowSrcPort,
			DstPort: flowDstPort,
			IPProto: flowIPProto,
		},
	}

	// PDS call
	respMsg, err := client.FlowClear(context.Background(), req)
	if err != nil {
		fmt.Printf("Clearing flows failed. %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	fmt.Printf("Clearing flows succeeded\n")
}

func flowShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS. Is PDS Running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewSessionSvcClient(c)

	var empty *pds.Empty

	// PDS call
	stream, err := client.FlowGet(context.Background(), empty)
	if err != nil {
		fmt.Printf("Getting flows failed. %v\n", err)
		return
	}

	flowPrintHeader()

	for {
		respMsg, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			fmt.Printf("Getting flow stream failure. %v\n", err)
			return
		}

		if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
			return
		}
		// Print flows
		for _, flow := range respMsg.GetFlow() {
			if flowMatchFilter(cmd, flow) {
				flowPrintEntry(flow)
			}
		}
	}
}

func flowMatchFilter(cmd *cobra.Command, flow *pds.Flow) bool {
	key := flow.GetKey()

	if cmd.Flags().Changed("vpcid") == false {
		if flow.GetVpc() != flowVpcID {
			return false
		}
	}

	if cmd.Flags().Changed("srcip") == false {
		if strings.Compare(utils.IPAddrToStr(key.GetSrcAddr()), flowSrcIP) != 0 {
			return false
		}
	}

	if cmd.Flags().Changed("dstip") == false {
		if strings.Compare(utils.IPAddrToStr(key.GetDstAddr()), flowDstIP) != 0 {
			return false
		}
	}

	if cmd.Flags().Changed("srcport") == false {
		if key.GetSrcPort() != flowSrcPort {
			return false
		}
	}

	if cmd.Flags().Changed("dstport") == false {
		if key.GetDstPort() != flowDstPort {
			return false
		}
	}

	if cmd.Flags().Changed("ipproto") == false {
		if key.GetIPProto() != flowIPProto {
			return false
		}
	}

	return true
}

func flowPrintHeader() {
	hdrLine := strings.Repeat("-", 131)
	fmt.Println(hdrLine)
	fmt.Printf("%-6s%-40s%-40s%-8s%-8s%-8s%-5s%-11s%-5s\n",
		"VPCId", "SrcAddr", "DstAddr", "SrcPort",
		"DstPort", "IPProto", "Role", "SessionIdx", "Epoch")
	fmt.Println(hdrLine)
}

func flowPrintEntry(flow *pds.Flow) {
	key := flow.GetKey()
	fmt.Printf("%-6d%-40s%-40s%-8d%-8d%-8d%-5d%-11d%-5d\n",
		flow.GetVpc(),
		utils.IPAddrToStr(key.GetSrcAddr()),
		utils.IPAddrToStr(key.GetDstAddr()),
		key.GetSrcPort(), key.GetDstPort(),
		key.GetIPProto(), flow.GetFlowRole(),
		flow.GetSessionIdx(), flow.GetEpoch())
}

func sessionShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS. Is PDS Running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewSessionSvcClient(c)

	var empty *pds.Empty

	// PDS call
	stream, err := client.SessionGet(context.Background(), empty)
	if err != nil {
		fmt.Printf("Getting sessions failed. %v\n", err)
		return
	}

	sessionPrintHeader()

	for {
		respMsg, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			fmt.Printf("Getting session stream failure. %v\n", err)
		}

		if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
			continue
		}
		// Print sessions
		for _, session := range respMsg.GetSession() {
			sessionPrintEntry(session)
		}
	}
}

func sessionPrintHeader() {
}

func sessionPrintEntry(session *pds.Session) {
}

func sessionShowStatsCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS. Is PDS Running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewDebugSvcClient(c)

	var statsIDLow uint32
	var statsIDHigh uint32

	n, _ := fmt.Sscanf(sessionStatsID, "%d-%d", &statsIDLow, &statsIDHigh)
	if n != 2 {
		n, _ = fmt.Sscanf(sessionStatsID, "%d", &statsIDLow)
		if n != 1 {
			fmt.Printf("Invalid session statistics index provided. Refer to help string\n")
			return
		}
		statsIDHigh = statsIDLow
	}

	if statsIDLow > statsIDHigh {
		fmt.Printf("Invalid session statistics index provided. Refer to help string")
		return
	}

	req := &pds.SessionStatsGetRequest{
		StatsIndexLow:  statsIDLow,
		StatsIndexHigh: statsIDHigh,
	}

	// PDS call
	respMsg, err := client.SessionStatsGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting session statistics failed. %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	sessionStatsPrintHeader()
	sessionStatsPrintEntry(respMsg)
}

func sessionStatsPrintHeader() {
	hdrLine := strings.Repeat("-", 86)
	fmt.Println(hdrLine)
	fmt.Printf("%-6s%-20s%-20s%-20s%-20s\n",
		"ID", "InitiatorPkts", "InitiatorBytes", "ResponderPkts", "ResponderBytes")
	fmt.Println(hdrLine)
}

func sessionStatsPrintEntry(resp *pds.SessionStatsGetResponse) {
	for _, stats := range resp.GetStats() {
		fmt.Printf("%-6d%-20d%-20d%-20d%-20d\n",
			stats.GetStatsIndex(),
			stats.GetInitiatorFlowPkts(),
			stats.GetInitiatorFlowBytes(),
			stats.GetResponderFlowPkts(),
			stats.GetResponderFlowBytes())
	}
}
