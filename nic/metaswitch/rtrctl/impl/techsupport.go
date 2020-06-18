//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package impl

import (
	"fmt"

	"github.com/spf13/cobra"
)

var tsShowCmd = &cobra.Command{
	Use:    "techsupport",
	Short:  "collect technical support information",
	Long:   "collect technical support information",
	Hidden: true,
	Run:    tsShowCmdHandler,
}

func init() {
}

func tsShowCmdHandler(cmd *cobra.Command, args []string) {
	fmt.Printf("\n=== Capturing rtrctl techsupport information ===\n")
	cmd.Flags().Set("json", "false")
	cmd.Flags().Set("detail", "true")
	fmt.Printf("\n=== BGP related information ===\n")
	bgpShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== BGP Peer information ===\n")
	bgpPeersShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== BGP Peer AF information ===\n")
	bgpPeersAfShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== BGP ip unicast nlri information ===\n")
	bgpIPUnicastNlriShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== BGP l2vpn evpn nlri information ===\n")
	bgpL2vpnEvpnNlriShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== BGP counters information ===\n")
	bgpPfxCountersShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== BGP route-map information ===\n")
	bgpRouteMapShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== EVPN VRF Status information ===\n")
	evpnVrfStatusShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== EVPN VRF RT Status information ===\n")
	evpnVrfRtStatusShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== EVPN EVI Status information ===\n")
	evpnEviStatusShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== EVPN EVI RT Status information ===\n")
	evpnEviRtStatusShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== EVPN BD MAC-IP Status information ===\n")
	evpnBdMacIPShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== EVPN BD Status information ===\n")
	evpnBdStatusShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== EVPN BD Interface Status information ===\n")
	evpnBdIntfShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== software interface information ===\n")
	routingSwIntfShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== software interface status information ===\n")
	routingIntfStatusShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== routing static-table information ===\n")
	staticTableShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== active routing route-table information ===\n")
	activeRouteTableShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== interface address information ===\n")
	routingIntfAddrShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== routing route-table information ===\n")
	routeTableShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== vrf information ===\n")
	routingVrfStatusShowCmdHandler(cmd, nil)
	fmt.Printf("\n=== routing redistribution ===\n")
	redistTableShowCmdHandler(cmd, nil)
}
