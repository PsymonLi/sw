//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"strings"

	"github.com/spf13/cobra"

	nmd "github.com/pensando/sw/nic/agent/protos/nmd"
)

var lldpCmd = &cobra.Command{
	Use:   "lldp",
	Short: "lldp info",
	Long:  "\n-----------------------------------------\n Show LLDP \n-----------------------------------------\n",
	Args:  cobra.NoArgs,
}

var lldpNeighborsCmd = &cobra.Command{
	Use:   "neighbors",
	Short: "lldp neighbors info",
	Long:  "\n-----------------------------------------\n Show LLDP neighbors \n-----------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  lldpNeighborsCmdHandler,
}

var lldpInterfacesCmd = &cobra.Command{
	Use:   "interfaces",
	Short: "lldp interfaces info",
	Long:  "\n-----------------------------------------\n Show LLDP interfaces \n-----------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  lldpInterfacesCmdHandler,
}

var lldpStatisticsCmd = &cobra.Command{
	Use:   "statistics",
	Short: "lldp statistics info",
	Long:  "\n-----------------------------------------\n Show LLDP statistics \n-----------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  lldpStatisticsCmdHandler,
}

func init() {
	showCmd.AddCommand(lldpCmd)
	lldpCmd.AddCommand(lldpNeighborsCmd)
	lldpCmd.AddCommand(lldpInterfacesCmd)
	lldpCmd.AddCommand(lldpStatisticsCmd)
}

func lldpNeighborsCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "halctlshowlldpneighbors",
		Opts:       strings.Join([]string{""}, ""),
	}
	return naplesExecCmd(v)
}

func lldpInterfacesCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "halctlshowlldpinterfaces",
		Opts:       strings.Join([]string{""}, ""),
	}
	return naplesExecCmd(v)
}

func lldpStatisticsCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "halctlshowlldpstatistics",
		Opts:       strings.Join([]string{""}, ""),
	}
	return naplesExecCmd(v)
}
