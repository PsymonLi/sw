//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	"reflect"

	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/apollo/agent/cli/utils"
	"github.com/pensando/sw/nic/apollo/agent/gen/pds"
)

var (
	tagID uint32
)

var tagShowCmd = &cobra.Command{
	Use:   "tag",
	Short: "show Tag information",
	Long:  "show Tag object information",
	Run:   tagShowCmdHandler,
}

func init() {
	showCmd.AddCommand(tagShowCmd)
	tagShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	tagShowCmd.Flags().Uint32VarP(&tagID, "id", "i", 0, "Specify tag policy ID")
}

func tagShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewTagSvcClient(c)

	var req *pds.TagGetRequest
	if cmd.Flags().Changed("id") {
		// Get specific Tag
		req = &pds.TagGetRequest{
			Id: []uint32{tagID},
		}
	} else {
		// Get all Tags
		req = &pds.TagGetRequest{
			Id: []uint32{},
		}
	}

	// PDS call
	respMsg, err := client.TagGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Tag failed. %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print Vnics
	if cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	}
}
