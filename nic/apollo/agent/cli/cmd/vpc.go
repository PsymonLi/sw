//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	"reflect"
	"strings"

	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/apollo/agent/cli/utils"
	"github.com/pensando/sw/nic/apollo/agent/gen/pds"
)

var (
	// ID holds VPC ID
	ID uint32
)

var vpcShowCmd = &cobra.Command{
	Use:   "vpc",
	Short: "show VPC information",
	Long:  "show VPC object information",
	Run:   vpcShowCmdHandler,
}

func init() {
	showCmd.AddCommand(vpcShowCmd)
	vpcShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	vpcShowCmd.Flags().Uint32VarP(&ID, "id", "i", 0, "Specify VPC ID")
}

func vpcShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewVPCSvcClient(c)

	var req *pds.VPCGetRequest
	if cmd.Flags().Changed("id") {
		// Get specific VPC
		req = &pds.VPCGetRequest{
			Id: []uint32{ID},
		}
	} else {
		// Get all VPCs
		req = &pds.VPCGetRequest{
			Id: []uint32{},
		}
	}

	// PDS call
	respMsg, err := client.VPCGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting VPC failed. %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print VPCs
	if cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else {
		printVPCHeader()
		for _, resp := range respMsg.Response {
			printVPC(resp)
		}
	}
}

func printVPCHeader() {
	hdrLine := strings.Repeat("-", 34)
	fmt.Println(hdrLine)
	fmt.Printf("%-6s%-10s%-18s\n", "ID", "Type", "IPPrefix")
	fmt.Println(hdrLine)
}

func printVPC(vpc *pds.VPC) {
	spec := vpc.GetSpec()
	fmt.Printf("%-6d%-10s%-18s\n",
		spec.GetId(),
		strings.Replace(spec.GetType().String(), "VPC_TYPE_", "", -1),
		utils.IPPrefixToStr(spec.GetV4Prefix()))
}
