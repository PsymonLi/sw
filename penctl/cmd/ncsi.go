//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"strings"

	"github.com/spf13/cobra"

	nmd "github.com/pensando/sw/nic/agent/protos/nmd"
)

var ncsiCmd = &cobra.Command{
	Use:   "ncsi",
	Short: "ncsi info",
	Long:  "\n-----------------------------------------\n Show NCSI \n-----------------------------------------\n",
	Args:  cobra.NoArgs,
}

var ncsiChannelCmd = &cobra.Command{
	Use:   "channel",
	Short: "ncsi channel info",
	Long:  "\n-----------------------------------------\n Show NCSI channel \n-----------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  ncsiChannelCmdHandler,
}

var ncsiVlanCmd = &cobra.Command{
	Use:   "vlan",
	Short: "ncsi vlan info",
	Long:  "\n-----------------------------------------\n Show NCSI vlan \n-----------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  ncsiVlanCmdHandler,
}

var ncsiMacCmd = &cobra.Command{
	Use:   "mac",
	Short: "ncsi mac info",
	Long:  "\n-----------------------------------------\n Show NCSI mac \n-----------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  ncsiMacCmdHandler,
}

var ncsiMcastCmd = &cobra.Command{
	Use:   "mcast",
	Short: "ncsi mcast info",
	Long:  "\n-----------------------------------------\n Show NCSI mcast \n-----------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  ncsiMcastCmdHandler,
}

var ncsiBcastCmd = &cobra.Command{
	Use:   "bcast",
	Short: "ncsi bcast info",
	Long:  "\n-----------------------------------------\n Show NCSI bcast \n-----------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  ncsiBcastCmdHandler,
}

func init() {
	showCmd.AddCommand(ncsiCmd)
	ncsiCmd.AddCommand(ncsiChannelCmd)
	ncsiCmd.AddCommand(ncsiVlanCmd)
	ncsiCmd.AddCommand(ncsiMacCmd)
	ncsiCmd.AddCommand(ncsiMcastCmd)
	ncsiCmd.AddCommand(ncsiBcastCmd)
}

func ncsiChannelCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "halctlshowncsichannel",
		Opts:       strings.Join([]string{""}, ""),
	}
	return naplesExecCmd(v)
}

func ncsiVlanCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "halctlshowncsivlan",
		Opts:       strings.Join([]string{""}, ""),
	}
	return naplesExecCmd(v)
}

func ncsiMacCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "halctlshowncsimac",
		Opts:       strings.Join([]string{""}, ""),
	}
	return naplesExecCmd(v)
}

func ncsiMcastCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "halctlshowncsimcast",
		Opts:       strings.Join([]string{""}, ""),
	}
	return naplesExecCmd(v)
}

func ncsiBcastCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "halctlshowncsibcast",
		Opts:       strings.Join([]string{""}, ""),
	}
	return naplesExecCmd(v)
}
