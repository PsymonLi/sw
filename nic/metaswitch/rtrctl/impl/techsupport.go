//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package impl

import (
	"fmt"
	"strings"

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

var list []*cobra.Command

func genTechSupport(cmd *cobra.Command) {
	if list == nil {
		list = make([]*cobra.Command, 0)
	}
	if cmd.RunE != nil {
		list = append(list, cmd)
	}
	for _, x := range cmd.Commands() {
		genTechSupport(x)
	}
}

func tsShowCmdHandler(cmd *cobra.Command, args []string) {
	fmt.Printf("\n=== Capturing rtrctl techsupport information ===\n")
	cmd.Flags().Set("json", "false")
	cmd.Flags().Set("detail", "true")
	genTechSupport(cmd.Root())
	for _, myCmd := range list {
		if strings.Contains(strings.ToLower(myCmd.Short), strings.ToLower("show")) {
			fmt.Printf("\n=== " + myCmd.Short + " ===\n")
			myCmd.RunE(cmd, nil)
		}
	}
}
