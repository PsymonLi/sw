//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

// +build apulu apollo

package cmd

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
	showCmd.AddCommand(tsShowCmd)
}

func tsShowCmdHandler(cmd *cobra.Command, args []string) {
	fmt.Printf("\n=== Capturing techsupport information ===\n")
	fmt.Printf("\n=== Flush PDS logs ===\n")
	traceDebugCmdHandler(nil, nil)
	fmt.Printf("\n=== System information ===\n")
	systemShowCmdHandler(nil, nil)
	fmt.Printf("\n=== System packet-buffer statistics ===\n")
	pbShowCmdHandler(nil, nil)
	pbDetailShowCmdHandler(nil, nil)
	fmt.Printf("\n=== System drop statistics ===\n")
	dropShowCmdHandler(nil, nil)
	fmt.Printf("\n=== System queue statistics ===\n")
	systemQueueStatsCmdHandler(nil, nil)
	fmt.Printf("\n=== System queue credits ===\n")
	systemQueueCreditsShowCmdHandler(nil, nil)
	fmt.Printf("\n=== System last level cache statistics ===\n")
	llcShowCmdHandler(nil, nil)
	fmt.Printf("\n=== System datapath assist statistics ===\n")
	datapathAssistStatsShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Table statistics ===\n")
	tableStatsShowCmdHandler(nil, nil)
	fmt.Printf("\n=== NACL Table statistics ===\n")
	naclShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Device information ===\n")
	deviceShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Memory information ===\n")
	memShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Port information ===\n")
	portShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Port status ===\n")
	portShowStatusCmdHandler(nil, nil)
	fmt.Printf("\n=== Xcvr info  ===\n")
	portXcvrShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Interface information ===\n")
	ifShowCmdHandler(nil, nil)
	fmt.Printf("\n=== LIF information ===\n")
	lifShowCmdHandler(nil, nil)
	fmt.Printf("\n=== VPC information ===\n")
	vpcShowCmdHandler(nil, nil)
	fmt.Printf("\n=== VNIC information ===\n")
	vnicShowCmdHandler(nil, nil)
	fmt.Printf("\n=== VNIC statistics===\n")
	vnicShowStatisticsCmdHandler(nil, nil)
	fmt.Printf("\n=== Mirror Session information ===\n")
	mirrorSessionShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Mirror Session statistics===\n")
	mirrorSessionShowStatisticsCmdHandler(nil, nil)
	fmt.Printf("\n=== Subnet information ===\n")
	subnetShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Route information ===\n")
	routeShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Nexthop Group information ===\n")
	nhGroupShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Nexthop information ===\n")
	nhShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Security profile information ===\n")
	securityProfileShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Security policy information ===\n")
	securityPolicyShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Local Mapping information ===\n")
	localMappingInternalHandler(nil, nil)
	fmt.Printf("\n=== Remote L2 Mapping information ===\n")
	remoteL2MappingInternalHandler(nil)
	fmt.Printf("\n=== Remote L3 Mapping information ===\n")
	remoteL3MappingInternalHandler(nil)
	fmt.Printf("\n=== Service mapping information ===\n")
	serviceShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Tunnel information ===\n")
	tunnelShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Flow information ===\n")
	flowShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Flow statistics information ===\n")
	flowStatsShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Session information ===\n")
	sessionShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Policer information ===\n")
	policerShowCmdHandler(nil, nil)
	fmt.Printf("\n=== NAT information ===\n")
	natShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Meter information ===\n")
	meterShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Tag information ===\n")
	tagShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Learn information ===\n")
	learnStatsShowCmdHandler(nil, nil)
	learnMACShowCmdHandler(nil, nil)
	learnIPShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Interrupts ===\n")
	interruptShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Techsupport dump complete ===\n")
}
