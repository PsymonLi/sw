//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package impl

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"

	"github.com/spf13/cobra"

	types "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	msTypes "github.com/pensando/sw/nic/metaswitch/gen/agent/pds_ms"
	"github.com/pensando/sw/nic/metaswitch/rtrctl/utils"
)

var routingShowCmd = &cobra.Command{
	Use:   "routing",
	Short: "show Routing related information",
	Long:  "show Routing related information",
	Args:  cobra.NoArgs,
}

var interfaceShowCmd = &cobra.Command{
	Use:   "interface",
	Short: "show software interface information",
	Long:  "show software interface information",
	Args:  cobra.NoArgs,
	RunE:  routingSwIntfShowCmdHandler,
}

var intfStatusShowCmd = &cobra.Command{
	Use:   "status",
	Short: "show interface status information",
	Long:  "show interface status information",
	Args:  cobra.NoArgs,
	RunE:  routingIntfStatusShowCmdHandler,
}

func init() {

}

const (
	rtgIntfStatusGlobalStr = `-----------------------------------
Routing Interface Status
-----------------------------------
Oper Status            : %v
Name                   : %v
Description            : %v
Type                   : %v
MacAddr                : %v
Mtu                    : %v
Loopback Mode          : %v
Flap Count             : %v
Oper Reason            : %v
-----------------------------------
`
)

func routingIntfStatusShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := types.NewLimSvcClient(c)

	req := &types.LimIfStatusGetRequest{}
	respMsg, err := client.LimIfStatusGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting routing interface status failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"

	intfs := []*utils.ShadowLimIfStatus{}
	for _, i := range respMsg.Response {
		intf := utils.NewLimIfStatusGetResp(i.Status)
		if doJSON {
			intfs = append(intfs, intf)
		} else {
			fmt.Printf(rtgIntfStatusGlobalStr, intf.OperStatus, intf.Name, intf.Description, intf.Type, intf.MacAddr, intf.Mtu, intf.LoopBackMode, intf.FlapCount, intf.OperReason)
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(intfs, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}

var staticTableShowCmd = &cobra.Command{
	Use:   "static-table",
	Short: "show routing static-table information",
	Long:  "show routing static-table information",
	Args:  cobra.NoArgs,
	RunE:  staticTableShowCmdHandler,
}

const (
	staticTableGlobalStr = `-----------------------------------
Static Table
-----------------------------------
DestAddr                   : %v
DestPrefixLen              : %v
NHAddr                     : %v
State                      : %v
Override                   : %v
AdminDist                  : %v
-----------------------------------
`
)

func staticTableShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := types.NewCPRouteSvcClient(c)

	req := &types.CPStaticRouteGetRequest{}
	respMsg, err := client.CPStaticRouteGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting static table failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"

	cpsrs := []*utils.ShadowCPStaticRoute{}
	for _, i := range respMsg.Response {
		cpsr := utils.NewCPStaticRoute(i)
		if doJSON {
			cpsrs = append(cpsrs, cpsr)
		} else {
			fmt.Printf(staticTableGlobalStr,
				cpsr.Spec.DestAddr,
				cpsr.Spec.PrefixLen,
				cpsr.Spec.NextHopAddr,
				cpsr.Spec.State,
				cpsr.Spec.Override,
				cpsr.Spec.AdminDist)
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(cpsrs, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}

var osTableShowCmd = &cobra.Command{
	Use:   "os",
	Short: "show os information",
	Long:  "show os information",
	Args:  cobra.NoArgs,
}

var intRoutingShowCmd = &cobra.Command{
	Use:   "routing",
	Short: "show Routing related information",
	Long:  "show Routing related information",
	Args:  cobra.NoArgs,
}

var osRouteTableShowCmd = &cobra.Command{
	Use:   "route-table",
	Short: "show os routing route-table information",
	Long:  "show os routing route-table information",
	Args:  cobra.NoArgs,
	RunE:  osRouteTableShowCmdHandler,
}

const (
	osRouteTableGlobalStr = `-----------------------------------
OS Route Table
-----------------------------------
RouteTableId               : %v
DestAddr                   : %v
DestPrefixLen              : %v
NHAddr                     : %v
IfIndex                    : %v
Type                       : %v
Proto                      : %v
Age                        : %v
Metric1                    : %v
Connected                  : %v
AdminDistance              : %v
-----------------------------------
`
)

func osRouteTableShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := msTypes.NewDebugPdsMsSvcClient(c)

	req := &msTypes.CPActiveRouteGetRequest{}
	respMsg, err := client.CPActiveRouteGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting route table failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"

	cpars := []*utils.ShadowCPActiveRoute{}
	for _, i := range respMsg.Response {
		cpar := utils.NewCPActiveRoute(i)
		if doJSON {
			cpars = append(cpars, cpar)
		} else {
			fmt.Printf(osRouteTableGlobalStr,
				cpar.Status.RouteTableId,
				cpar.Status.DestAddr,
				cpar.Status.DestPrefixLen,
				cpar.Status.NHAddr,
				cpar.Status.IfIndex,
				cpar.Status.Type,
				cpar.Status.Proto,
				cpar.Status.Age,
				cpar.Status.Metric1,
				cpar.Status.Connected,
				cpar.Status.AdminDistance)
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(cpars, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}

var intfAddrShowCmd = &cobra.Command{
	Use:   "address",
	Short: "show interface address information",
	Long:  "show interface address information",
	Args:  cobra.NoArgs,
	RunE:  routingIntfAddrShowCmdHandler,
}

const (
	rtgIntfAddrGlobalStr = `-----------------------------------
Routing Interface Address
-----------------------------------
IfId                   : %v
IP Addr                : %v
Oper Status            : %v
-----------------------------------
`
)

func routingIntfAddrShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := types.NewLimSvcClient(c)

	req := &types.LimIfAddrTableGetRequest{}
	respMsg, err := client.LimIfAddrTableGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting routing interface address failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"

	intfs := []*utils.ShadowLimIfAddrTableStatus{}
	for _, i := range respMsg.Response {
		intf := utils.NewLimIfAddrTableGetResponse(i.Status)
		if doJSON {
			intfs = append(intfs, intf)
		} else {
			fmt.Printf(rtgIntfAddrGlobalStr, intf.IfIndex, intf.IPAddr, intf.OperStatus)
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(intfs, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}

var routeTableShowCmd = &cobra.Command{
	Use:   "route-table",
	Short: "show routing route-table information",
	Long:  "show routing route-table information",
	Args:  cobra.NoArgs,
	RunE:  routeTableShowCmdHandler,
}

const (
	routeTableGlobalStr = `-----------------------------------
Route Table
-----------------------------------
RouteTableId               : %v
DestAddr                   : %v
DestPrefixLen              : %v
NHAddr                     : %v
IfIndex                    : %v
Type                       : %v
Proto                      : %v
Age                        : %v
Metric1                    : %v
FibRoute                   : %v
Connected                  : %v
LooseNextHop               : %v
AdminDistance              : %v
-----------------------------------
`
)

func routeTableShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := types.NewCPRouteSvcClient(c)

	req := &types.CPRouteGetRequest{}
	respMsg, err := client.CPRouteGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting route table failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"

	cprs := []*utils.ShadowCPRoute{}
	for _, i := range respMsg.Response {
		cpr := utils.NewCPRoute(i)
		if doJSON {
			cprs = append(cprs, cpr)
		} else {
			fmt.Printf(routeTableGlobalStr,
				cpr.Status.RouteTableId,
				cpr.Status.DestAddr,
				cpr.Status.DestPrefixLen,
				cpr.Status.NHAddr,
				cpr.Status.IfIndex,
				cpr.Status.Type,
				cpr.Status.Proto,
				cpr.Status.Age,
				cpr.Status.Metric1,
				cpr.Status.FibRoute,
				cpr.Status.Connected,
				cpr.Status.LooseNextHop,
				cpr.Status.AdminDistance)
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(cprs, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}

var vrfStatusShowCmd = &cobra.Command{
	Use:   "vrf",
	Short: "show vrf information",
	Long:  "show vrf information",
	Args:  cobra.NoArgs,
	RunE:  routingVrfStatusShowCmdHandler,
}

const (
	rtgVrfStatusGlobalStr = `-----------------------------------
Routing Vrf Status
-----------------------------------
EntityIndex            : %v
VrfName                : %v
Description            : %v
OperStatus             : %v
OperReason             : %v
NumInterfaces          : %v
-----------------------------------
`
)

func routingVrfStatusShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := types.NewLimSvcClient(c)

	req := &types.LimVrfGetRequest{}
	respMsg, err := client.LimVrfGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting routing vrf status failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"

	vrfs := []*utils.ShadowLimVrfStatus{}
	for _, i := range respMsg.Response {
		vrf := utils.NewLimVrfGetResp(i.Status)
		if doJSON {
			vrfs = append(vrfs, vrf)
		} else {
			fmt.Printf(rtgVrfStatusGlobalStr, vrf.EntityIndex, vrf.VrfName, vrf.Desc, vrf.OperStatus, vrf.OperReason, vrf.NumInterfaces)
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(vrfs, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}

var redistTableShowCmd = &cobra.Command{
	Use:   "redist",
	Short: "show routing redistribution information",
	Long:  "show routing redistribution information",
	Args:  cobra.NoArgs,
	RunE:  redistTableShowCmdHandler,
}

const (
	redistTableGlobalStr = `-----------------------------------
Redistribution Table
-----------------------------------
FteIndex                   : %v
EntryId                    : %v
RuleUsageCount             : %v
AddrFilter                 : %v
-----------------------------------
`
)

func redistTableShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := types.NewCPRouteSvcClient(c)

	req := &types.CPRouteRedistGetRequest{}
	respMsg, err := client.CPRouteRedistGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting static table failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"

	cprrs := []*utils.ShadowCPRouteRedist{}
	for _, i := range respMsg.Response {
		cprr := utils.NewCPRouteRedist(i)
		if doJSON {
			cprrs = append(cprrs, cprr)
		} else {
			fmt.Printf(redistTableGlobalStr,
				cprr.Status.FteIndex,
				cprr.Status.EntryId,
				cprr.Status.RuleUsageCount,
				cprr.Status.AddrFilter)
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(cprrs, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}

const (
	rtgSwIntfGlobalStr = `-----------------------------------
Routing Software Interface
-----------------------------------
EntityIndex            : %v
Type                   : %v
Index                  : %v
OperStatus             : %v
Name                   : %v
IfIndex                : %v
-----------------------------------
`
)

func routingSwIntfShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := types.NewLimSvcClient(c)

	req := &types.LimSwIfGetRequest{}
	respMsg, err := client.LimSwIfGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting routing software interface failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"

	intfs := []*utils.ShadowLimSwIfStatus{}
	for _, i := range respMsg.Response {
		intf := utils.NewLimSwIfGetResponse(i.Status)
		if doJSON {
			intfs = append(intfs, intf)
		} else {
			fmt.Printf(rtgSwIntfGlobalStr, intf.EntityIndex, intf.Type, intf.Index, intf.OperStatus, intf.Name, intf.IfIndex)
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(intfs, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}
