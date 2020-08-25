//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

// +build apulu

package cmd

import (
	"fmt"
	"os"
	"os/exec"

	"github.com/spf13/cobra"
)

// pdsctl commands to internal commands mapping
var cmdMap map[string][]string = map[string][]string{
	// todo: make vppctl location common for sim and hw
	"pdsctl show vpc internal": []string{
		"vpp/tools/vppctl-sim.sh show pds vpc",
		"vpp/tools/vppctl-sim.sh show pds vpc status",
	},
}

func cmdMapCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	mappedCmds, found := cmdMap[cmd.CommandPath()]
	if !found {
		fmt.Println("Error: Invalid cmd")
		return
	}

	for _, mappedCmd := range mappedCmds {
		execCmd := exec.Command("sh", "-c", mappedCmd)
		execCmd.Stdout = os.Stdout
		execCmd.Stderr = os.Stderr
		err := execCmd.Run()
		if err != nil {
			fmt.Printf("Error: Failed to run %s, err %s\n", mappedCmd, err)
		}
	}
}
