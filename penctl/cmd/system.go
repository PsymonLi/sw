//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"fmt"
	"strings"

	"github.com/spf13/cobra"

	nmd "github.com/pensando/sw/nic/agent/protos/nmd"
)

var tabularFormat bool

var getSysMemCmd = &cobra.Command{
	Use:   "system-memory-usage",
	Short: "Show free/used memory on Distributed Service Card (in MB)",
	Long:  "\n-----------------------------------------\n Show Free/Used Memory on Distributed Service Card (in MB)\n-----------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  getSysMemCmdHandler,
}

var getProcMemInfoCmd = &cobra.Command{
	Use:   "proc-meminfo",
	Short: "Check /proc/meminfo file on Distributed Service Card",
	Long:  "\n------------------------------------\n Check /proc/meminfo file on Distributed Service Card \n------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  getProcMemInfoCmdHandler,
}

var getNaplesInfoCmd = &cobra.Command{
	Use:   "version",
	Short: "Get all Naples Information",
	Long:  "\n------------------------------------\n Get all Naples Information \n------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  getNaplesInfo,
}

var getSystemCmd = &cobra.Command{
	Use:   "system",
	Short: "show system information",
	Long:  "\n------------------------------------\n show system information \n------------------------------------\n",
	Args:  cobra.NoArgs,
}

var getSystemQueueStatsCmd = &cobra.Command{
	Use:   "queue-statistics",
	Short: "show system queue-statistics",
	Long:  "\n------------------------------------\n show system queue-statistics \n------------------------------------\n",
	Args:  cobra.NoArgs,
	Run:   getSystemQueueStatsCmdHandler,
}

var getAlomPresentCmd = &cobra.Command{
	Use:   "alompresent",
	Short: "check if alom is present or not",
	Long:  "\n------------------------------------\n checks to see if alom is present or not \n------------------------------------\n",
	Args:  cobra.NoArgs,
	RunE:  getAlomPresentCmdHandler,
}

func init() {
	showCmd.AddCommand(getSysMemCmd)
	showCmd.AddCommand(getProcMemInfoCmd)
	showCmd.AddCommand(getSystemCmd)
	showCmd.AddCommand(getNaplesInfoCmd)
	getSystemCmd.AddCommand(getAlomPresentCmd)
	getSystemCmd.AddCommand(getSystemQueueStatsCmd)
	showCmd.PersistentFlags().BoolVarP(&tabularFormat, "tabular", "t", false, "display in table format")
}

func getSysMemCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "getsysmem",
		Opts:       strings.Join([]string{""}, ""),
	}
	return naplesExecCmd(v)
}

func getProcMemInfoCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "getprocmeminfo",
		Opts:       strings.Join([]string{""}, ""),
	}
	return naplesExecCmd(v)
}

func getSystemQueueStatsCmdHandler(cmd *cobra.Command, args []string) {
	//halctlStr := "halctl show system queue-statistics "
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "halctlshowsystemqueuestats",
		Opts:       "",
	}

	resp, err := restGetWithBody(v, "cmd/v1/naples/")
	if err != nil {
		fmt.Println(err)
		return
	}

	if len(resp) > 3 {
		s := strings.Replace(string(resp), `\n`, "\n", -1)
		fmt.Println(s)
	}

	return
}

func getAlomPresentCmdHandler(cmd *cobra.Command, args []string) error {
	v := &nmd.DistributedServiceCardCmdExecute{
		Executable: "systemctlshow",
		Opts:       "",
	}
	return naplesExecCmd(v)
}
