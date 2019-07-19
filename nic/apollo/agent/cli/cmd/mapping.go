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
	vpcID     uint32
	mappingIP string
)

var mappingShowCmd = &cobra.Command{
	Use:   "mapping",
	Short: "show Mapping information",
	Long:  "show Mapping object information",
	Run:   mappingShowCmdHandler,
}

func init() {
	//	showCmd.AddCommand(mappingShowCmd)
	mappingShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	mappingShowCmd.Flags().Uint32Var(&vpcID, "vpc-id", 0, "Specify VPC ID")
	mappingShowCmd.Flags().StringVar(&mappingIP, "ip", "0", "Specify mapping IP address")
}

func mappingShowCmdHandler(cmd *cobra.Command, args []string) {
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

	if cmd.Flags().Changed("vpc-id") != cmd.Flags().Changed("ip") {
		fmt.Printf("Cannot specify only one of VPC ID and mapping IP address\n")
		return
	}

	client := pds.NewMappingSvcClient(c)

	var req *pds.MappingGetRequest
	if cmd.Flags().Changed("vpc-id") && cmd.Flags().Changed("ip") {
		// Get specific Mapping
		var key *pds.MappingKey
		key = &pds.MappingKey{
			VPCId:  vpcID,
			IPAddr: utils.IPAddrStrToPDSIPAddr(mappingIP),
		}
		req = &pds.MappingGetRequest{
			Id: []*pds.MappingKey{key},
		}
	} else {
		// Get all Mappings
		req = &pds.MappingGetRequest{
			Id: []*pds.MappingKey{},
		}
	}

	// PDS call
	respMsg, err := client.MappingGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Mapping failed. %v\n", err)
		return
	}

	// Print Mapping
	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	if cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else {
		printMappingHeader()
		for _, resp := range respMsg.Response {
			printMapping(resp)
		}
	}
}

func printMappingHeader() {
	hdrLine := strings.Repeat("-", 97)
	fmt.Println(hdrLine)
	fmt.Printf("%-6s%-16s%-9s%-9s%-20s%-14s%-7s%-16s%-16s%-6s\n",
		"VpcID", "PrivateIP", "SubnetID", "TunnelID", "MAC",
		"Encap", "VnicID", "PublicIP", "ProviderIP", "SvcTag")
	fmt.Println(hdrLine)
}

func printMapping(mapping *pds.Mapping) {
	spec := mapping.GetSpec()
	encapStr := utils.EncapToString(spec.GetEncap())
	fmt.Printf("%-6s%-16s%-9s%-16s%-20s%-14s%-7d%-16s%-16s%-6d\n",
		spec.GetId().GetVPCId(),
		utils.IPAddrToStr(spec.GetId().GetIPAddr()),
		spec.GetSubnetId(), utils.IPAddrToStr(spec.GetTunnelIP()),
		utils.MactoStr(spec.GetMACAddr()),
		encapStr, spec.GetVnicId(),
		utils.IPAddrToStr(spec.GetPublicIP()),
		utils.IPAddrToStr(spec.GetProviderIp()),
		spec.GetServiceTag())
}
