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
	subnetID uint32
)

var subnetShowCmd = &cobra.Command{
	Use:   "subnet",
	Short: "show Subnet information",
	Long:  "show Subnet object information",
	Run:   subnetShowCmdHandler,
}

func init() {
	showCmd.AddCommand(subnetShowCmd)
	subnetShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	subnetShowCmd.Flags().Uint32VarP(&subnetID, "id", "i", 0, "Specify Subnet ID")
}

func subnetShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewSubnetSvcClient(c)

	var req *pds.SubnetGetRequest
	if cmd.Flags().Changed("id") {
		// Get specific Subnet
		req = &pds.SubnetGetRequest{
			Id: []uint32{subnetID},
		}
	} else {
		// Get all Subnets
		req = &pds.SubnetGetRequest{
			Id: []uint32{},
		}
	}

	// PDS call
	respMsg, err := client.SubnetGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Subnet failed. %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print Subnets
	if cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else {
		printSubnetHeader()
		for _, resp := range respMsg.Response {
			printSubnet(resp)
		}
	}
}

func printSubnetHeader() {
	hdrLine := strings.Repeat("-", 180)
	fmt.Printf("\n")
	fmt.Printf("V4RtTblID - IPv4 Route Table ID                V6RtTblID - IPv6 Route Table ID\n")
	fmt.Printf("IngV4SGID - Ingress IPv4 Security Group ID     IngV6SGID - Ingress IPv6 Security Group ID\n")
	fmt.Printf("EgV4SGID  - Egress IPv4 Security Group ID      EgV6SGID  - Egress IPv6 Security Group ID\n")
	fmt.Println(hdrLine)
	fmt.Printf("%-6s%-6s%-20s%-20s%-16s%-16s%-20s%-10s%-10s%-14s%-14s%-14s%-14s\n",
		"ID", "VpcID", "IPv4Prefix", "IPv6Prefix", "VRouterIPv4", "VRouterIPv6", "VRouterMAC", "V4RtTblID",
		"V6RtTblID", "IngV4SGID", "IngV6SGID", "EgV4SGID", "EgV6SGID")
	fmt.Println(hdrLine)
}

func printSubnet(subnet *pds.Subnet) {
	spec := subnet.GetSpec()
	fmt.Printf("%-6d%-6d%-20s%-20s%-16s%-16s%-20s%-10d%-10d%-14d%-14d%-14d%-14d\n",
		spec.GetId(), spec.GetVPCId(), utils.IPPrefixToStr(spec.GetV4Prefix()),
		utils.IPPrefixToStr(spec.GetV6Prefix()),
		utils.Uint32IPAddrtoStr(spec.GetIPv4VirtualRouterIP()),
		utils.ByteIPv6AddrtoStr(spec.GetIPv6VirtualRouterIP()),
		utils.MactoStr(spec.GetVirtualRouterMac()),
		spec.GetV4RouteTableId(), spec.GetV6RouteTableId(),
		spec.GetIngV4SecurityPolicyId(),
		spec.GetIngV6SecurityPolicyId(),
		spec.GetEgV4SecurityPolicyId(),
		spec.GetEgV6SecurityPolicyId())
}
