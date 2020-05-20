//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
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
	policerID string
)

var policerShowCmd = &cobra.Command{
	Use:   "policer",
	Short: "show policer information",
	Long:  "show policer object information",
	Run:   policerShowCmdHandler,
}

func init() {
	showCmd.AddCommand(policerShowCmd)
	policerShowCmd.Flags().StringVarP(&policerID, "id", "i", "", "Specify policer ID")
	policerShowCmd.Flags().Bool("yaml", true, "Output in yaml")
	policerShowCmd.Flags().Bool("summary", false, "Display number of objects")
}

func policerShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewPolicerSvcClient(c)

	var req *pds.PolicerGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// Get specific policer
		req = &pds.PolicerGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(policerID).Bytes()},
		}
	} else {
		// Get all Mirror sessions
		req = &pds.PolicerGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.PolicerGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting mirror session failed, err %v\n", err)
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
		printPolicerSummary(len(respMsg.Response))
	} else {
		// Print policer
		printPolicerHeader()
		for _, resp := range respMsg.Response {
			printPolicer(resp)
		}
		printPolicerSummary(len(respMsg.Response))
	}
}

func printPolicerSummary(count int) {
	fmt.Printf("\nNo. of policer objects : %d\n\n", count)
}

func printPolicerHeader() {
	hdrLine := strings.Repeat("-", 103)
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-10s%-5s%-14s%-14s%-10s%-10s\n",
		"ID", "Direction", "Type", "PPS/BPS", "Burst", "AcceptCnt", "DropCnt")
	fmt.Println(hdrLine)
}

func printPolicer(p *pds.Policer) {
	spec := p.GetSpec()
	stats := p.GetStats()

	typeStr := ""
	var burst uint64
	var count uint64
	if spec.GetPPSPolicer() != nil {
		typeStr += "PPS"
		count = spec.GetPPSPolicer().GetPacketsPerSecond()
		burst = spec.GetPPSPolicer().GetBurst()
	} else if spec.GetBPSPolicer() != nil {
		typeStr += "BPS"
		count = spec.GetBPSPolicer().GetBytesPerSecond()
		burst = spec.GetBPSPolicer().GetBurst()
	}
	fmt.Printf("%-40s%-10s%-5s%-14d%-14d%-10d%-10d\n",
		utils.IdToStr(spec.GetId()),
		strings.Replace(spec.GetDirection().String(), "POLICER_DIR_", "", -1),
		typeStr, count, burst,
		stats.GetAccept(), stats.GetDrop())
}
