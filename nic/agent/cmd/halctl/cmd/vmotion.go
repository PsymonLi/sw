//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	"os"
	"strings"

	"github.com/spf13/cobra"

	"github.com/pensando/sw/nic/agent/cmd/halctl/utils"
	"github.com/pensando/sw/nic/agent/netagent/datapath/halproto"
)

var vmotionShowCmd = &cobra.Command{
	Use:   "vmotion",
	Short: "show vMotion information",
	Long:  "show vMotion information",
	Run:   vmotionShowCmdHandler,
}

func init() {
	showCmd.AddCommand(vmotionShowCmd)
}

func vmotionShowResponse(resp *halproto.VmotionDebugResponse) {
	if !resp.GetVmotionEnable() {
		fmt.Printf("vMotion is disabled\n")
		return
	}

	hdrLine := strings.Repeat("-", 70)

	// History vMotions
	fmt.Println(hdrLine)
	fmt.Println("History of vMotions")
	fmt.Println(hdrLine)
	for _, ep := range resp.GetHistoryEp() {
		fmt.Printf("EndPoint: %s\n", ep.GetMacAddress())
		fmt.Printf("  HomingHost:%s, Type:%d, State:%d, Flags:0x%x, SM State:%d\n",
			ep.GetOldHomingHostIp(), ep.GetMigrationType(), ep.GetVmotionState(), ep.GetFlags(),
			ep.GetState())
		fmt.Printf("  SyncCnt:%d (Msg:%d) TermSyncCnt:%d (Msg:%d)\n",
			ep.GetSyncSessCnt(), ep.GetSyncCnt(), ep.GetTermSyncSessCnt(), ep.GetTermSyncCnt())
		fmt.Printf("  Times - Start:[%s], TermSync:[%s] End:[%s]\n", ep.GetStartTime(),
			ep.GetTermSyncTime(), ep.GetEndTime())
		fmt.Println(hdrLine)
	}
	fmt.Println(hdrLine)

	// Active vMotions
	fmt.Println("\n\nActive vMotions")
	fmt.Println(hdrLine)
	for _, ep := range resp.GetEp() {
		fmt.Printf("EndPoint: %s\n", ep.GetMacAddress())
		fmt.Printf("  HomingHost:%s, Type:%d, State:%d, Flags:0x%x, SM State:%d\n",
			ep.GetOldHomingHostIp(), ep.GetMigrationType(), ep.GetVmotionState(), ep.GetFlags(),
			ep.GetState())
		fmt.Printf("  SyncCnt:%d (Msg:%d) TermSyncCnt:%d (Msg:%d)\n",
			ep.GetSyncSessCnt(), ep.GetSyncCnt(), ep.GetTermSyncSessCnt(), ep.GetTermSyncCnt())
		fmt.Printf("  Times - Start:[%s] TermSync:[%s]\n", ep.GetStartTime(), ep.GetTermSyncTime())
		fmt.Println(hdrLine)
	}
	fmt.Println(hdrLine)

	// vMotion active Endpoints
	fmt.Println("\n\n Migrated Endpoints")
	fmt.Println(hdrLine)
	fmt.Printf("%-20s%-12s%-17s\n", "EndPoint", "useg-vlan", "MigrationState")
	fmt.Println(hdrLine)
	for _, ep := range resp.GetActiveEp() {
		fmt.Printf("%-20s%-12d%-17d\n", ep.GetMacAddress(), ep.GetUsegVlan(), ep.GetMigrationState())
	}
	fmt.Println(hdrLine)

	// Statistics
	fmt.Println("\n\n vMotion Statistics")
	fmt.Println(hdrLine)
	fmt.Printf("%-24s: %-6d\n", "Total vMotion", resp.GetStats().GetTotalVmotion())
	fmt.Printf("%-24s: %-6d\n", "Total In vMotion", resp.GetStats().GetMigInVmotion())
	fmt.Printf("%-24s: %-6d\n", "Total Out vMotion", resp.GetStats().GetMigOutVmotion())
	fmt.Printf("%-24s: %-6d\n", "Total Success Migration", resp.GetStats().GetMigSuccess())
	fmt.Printf("%-24s: %-6d\n", "Total Failed Migration", resp.GetStats().GetMigFailed())
	fmt.Printf("%-24s: %-6d\n", "Total Aborted Migration", resp.GetStats().GetMigAborted())
	fmt.Printf("%-24s: %-6d\n", "Total Timeout Migration", resp.GetStats().GetMigTimeout())
	fmt.Printf("%-24s: %-6d\n", "Total Cold Migration", resp.GetStats().GetMigCold())
	fmt.Println(hdrLine)
}

func vmotionShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewInternalClient(c)

	req := &halproto.VmotionDebugSendRequest{}

	reqMsg := &halproto.VmotionDebugSendRequestMsg{
		Request: []*halproto.VmotionDebugSendRequest{req},
	}

	// HAL call
	respMsg, err := client.VmotionDebugReq(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("vMotion debug dump send request failed. %v\n", err)
		return
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
			continue
		}
		vmotionShowResponse(resp)
	}
}
