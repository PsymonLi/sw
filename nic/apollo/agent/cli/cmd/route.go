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
	routeID uint32
)

var routeShowCmd = &cobra.Command{
	Use:   "route",
	Short: "show Route information",
	Long:  "show Route object information",
	Run:   routeShowCmdHandler,
}

func init() {
	//	showCmd.AddCommand(routeShowCmd)
	routeShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	routeShowCmd.Flags().Uint32VarP(&routeID, "route-id", "i", 0, "Specify Route ID")
}

func routeShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewRouteSvcClient(c)

	var req *pds.RouteTableGetRequest
	if cmd.Flags().Changed("id") {
		// Get specific Route
		req = &pds.RouteTableGetRequest{
			Id: []uint32{routeID},
		}
	} else {
		// Get all Routes
		req = &pds.RouteTableGetRequest{
			Id: []uint32{},
		}
	}

	// PDS call
	respMsg, err := client.RouteTableGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Route failed. %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print Routes
	if cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else {
		printRouteHeader()
		for _, resp := range respMsg.Response {
			printRoute(resp)
		}
	}
}

func printRouteHeader() {
	hdrLine := strings.Repeat("-", 60)
	fmt.Println(hdrLine)
	fmt.Printf("%-6s%-6s%-20s%-12s%-16s\n",
		"ID", "IPAF", "Prefix", "NextHopType", "NextHopValue")
	fmt.Println(hdrLine)
}

func printRoute(rt *pds.RouteTable) {
	spec := rt.GetSpec()
	routes := spec.GetRoutes()
	first := true

	fmt.Printf("%-6d%-6s", spec.GetId(),
		strings.Replace(spec.GetAf().String(), "IP_AF_", "", -1))

	for _, route := range routes {
		if first != true {
			fmt.Printf("%-6s%-6s", "", "")
		}
		switch route.GetNh().(type) {
		case *pds.Route_NextHop:
			fmt.Printf("%-20s%-12s%-16s\n",
				utils.IPPrefixToStr(route.GetPrefix()),
				"IP",
				utils.IPAddrToStr(route.GetNextHop()))
			first = false
		case *pds.Route_NexthopId:
			fmt.Printf("%-20s%-12s%-16d\n",
				utils.IPPrefixToStr(route.GetPrefix()),
				"ID",
				route.GetNexthopId)
			first = false
		case *pds.Route_VPCId:
			fmt.Printf("%-20s%-12s%-16d\n",
				utils.IPPrefixToStr(route.GetPrefix()),
				"VPC",
				route.GetVPCId())
			first = false
		case *pds.Route_TunnelId:
			fmt.Printf("%-20s%-12s%-16d\n",
				utils.IPPrefixToStr(route.GetPrefix()),
				"TEP",
				route.GetTunnelId())
			first = false
		default:
		}
	}
}
