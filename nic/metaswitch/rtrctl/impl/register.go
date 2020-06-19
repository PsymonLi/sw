package impl

import (
	"github.com/spf13/cobra"
)

// CLIParams is parameters for CLI
type CLIParams struct {
	GRPCPort string
}

var cliParams *CLIParams

// RegisterClearNodes registers all the CLI nodes
func RegisterClearNodes(params *CLIParams, base *cobra.Command) {
	cliParams = params

	//bgp commands
	base.AddCommand(bgpClearCmd)
	bgpClearCmd.PersistentFlags().StringVarP(&laddr, "local-addr", "l", "", "Local Address IP")
	bgpClearCmd.PersistentFlags().StringVarP(&paddr, "peer-addr", "p", "", "Peer Address IP")
	bgpClearCmd.PersistentFlags().StringVarP(&option, "option", "o", "", "Option can be either hard or refresh_in or refresh_out or refresh_both")
	bgpClearCmd.PersistentFlags().StringVarP(&afi, "afi", "a", "", "AFI can be ipv4 or l2vpn")
	bgpClearCmd.PersistentFlags().StringVarP(&safi, "safi", "s", "", "SAFI can be unicast or evpn")
	bgpClearCmd.MarkFlagRequired("option")
}

// RegisterDebugNodes registers the routing command tree under debug node
func RegisterDebugNodes(params *CLIParams, base *cobra.Command) {
	cliParams = params

	//routing debug commands
	base.AddCommand(routingDebugCmd)
	routingDebugCmd.AddCommand(routingOpenDebugCmd)
	routingDebugCmd.AddCommand(routingCloseDebugCmd)
	routingDebugCmd.Flags().Uint32Var(&routingBatchSize, "batch-size", 1024, "modify routing batch commit size")
}

// RegisterShowNodes registers all the CLI nodes
func RegisterShowNodes(params *CLIParams, base *cobra.Command) {
	cliParams = params

	//bgp commands
	base.AddCommand(bgpShowCmd)
	bgpShowCmd.PersistentFlags().Bool("json", false, "output in json")
	bgpShowCmd.PersistentFlags().Bool("detail", false, "detailed output")
	bgpShowCmd.AddCommand(peerShowCmd)
	bgpShowCmd.AddCommand(peerAfShowCmd)
	bgpShowCmd.AddCommand(bgpRouteMapShowCmd)
	bgpShowCmd.AddCommand(tsShowCmd)

	bgpShowCmd.AddCommand(bgpPrefixShowCmd)
	bgpPrefixShowCmd.AddCommand(bgpPfxCountersShowCmd)

	bgpShowCmd.AddCommand(bgpIPShowCmd)
	bgpIPShowCmd.AddCommand(bgpIPUnicastShowCmd)
	bgpIPUnicastShowCmd.AddCommand(bgpIPUnicastNlriShowCmd)
	bgpIPUnicastNlriShowCmd.PersistentFlags().StringVarP(&extCommFilter, "ext-comm-filter", "e", "", "extended community filter")
	bgpIPUnicastNlriShowCmd.PersistentFlags().StringVarP(&vnidFilter, "vni-id", "v", "", "l2vni or l3vni filter")
	bgpIPUnicastNlriShowCmd.PersistentFlags().StringVarP(&typeFilter, "type", "t", "", "route type filter")
	bgpIPUnicastNlriShowCmd.PersistentFlags().StringVarP(&nextHopFilter, "next-hop", "n", "", "next hop filter")

	bgpShowCmd.AddCommand(bgpL2vpnShowCmd)
	bgpL2vpnShowCmd.AddCommand(bgpL2vpnEvpnShowCmd)
	bgpL2vpnEvpnShowCmd.AddCommand(bgpL2vpnEvpnNlriShowCmd)
	bgpL2vpnEvpnNlriShowCmd.PersistentFlags().StringVarP(&extCommFilter, "ext-comm-filter", "e", "", "extended community filter")
	bgpL2vpnEvpnNlriShowCmd.PersistentFlags().StringVarP(&vnidFilter, "vni-id", "v", "", "l2vni or l3vni filter")
	bgpL2vpnEvpnNlriShowCmd.PersistentFlags().StringVarP(&typeFilter, "type", "t", "", "route type filter")
	bgpL2vpnEvpnNlriShowCmd.PersistentFlags().StringVarP(&nextHopFilter, "next-hop", "n", "", "next hop filter")

	//evpn commands
	base.AddCommand(evpnShowCmd)
	evpnShowCmd.PersistentFlags().Bool("json", false, "output in json")
	evpnShowCmd.PersistentFlags().Bool("detail", false, "detailed output")

	evpnShowCmd.AddCommand(evpnVrfShowCmd)
	evpnVrfShowCmd.AddCommand(evpnVrfStatusShowCmd)
	evpnVrfShowCmd.AddCommand(evpnVrfRtShowCmd)
	evpnVrfRtShowCmd.AddCommand(evpnVrfRtStatusShowCmd)

	evpnShowCmd.AddCommand(evpnEviShowCmd)
	evpnEviShowCmd.AddCommand(evpnEviStatusShowCmd)
	evpnEviShowCmd.AddCommand(evpnEviRtShowCmd)
	evpnEviRtShowCmd.AddCommand(evpnEviRtStatusShowCmd)

	evpnShowCmd.AddCommand(evpnBdShowCmd)
	evpnBdShowCmd.AddCommand(evpnBdMacIPShowCmd)

	//routing commands
	base.AddCommand(routingShowCmd)
	routingShowCmd.AddCommand(staticTableShowCmd)
	staticTableShowCmd.PersistentFlags().Bool("json", false, "output in json")
	interfaceShowCmd.AddCommand(intfAddrShowCmd)
	intfAddrShowCmd.PersistentFlags().Bool("json", false, "output in json")
	routingShowCmd.AddCommand(routeTableShowCmd)
	routeTableShowCmd.PersistentFlags().Bool("json", false, "output in json")
	routingShowCmd.AddCommand(vrfStatusShowCmd)
	vrfStatusShowCmd.PersistentFlags().Bool("json", false, "output in json")

	//debug commands
	routingShowCmd.AddCommand(internalShowCmd)
	internalShowCmd.AddCommand(activeRouteTableShowCmd)
	activeRouteTableShowCmd.PersistentFlags().Bool("json", false, "output in json")
	internalShowCmd.AddCommand(interfaceShowCmd)
	interfaceShowCmd.AddCommand(intfStatusShowCmd)
	interfaceShowCmd.PersistentFlags().Bool("json", false, "output in json")
	internalShowCmd.AddCommand(redistTableShowCmd)
	redistTableShowCmd.PersistentFlags().Bool("json", false, "output in json")

	evpnShowCmd.AddCommand(internalEvpnShowCmd)
	internalEvpnShowCmd.AddCommand(intEvpnBdShowCmd)
	intEvpnBdShowCmd.AddCommand(evpnBdStatusShowCmd)
	intEvpnBdShowCmd.AddCommand(evpnBdIntfShowCmd)
}
