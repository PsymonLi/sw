//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

// +build athena

package cmd

import (
	"fmt"

	"github.com/spf13/cobra"
)

var tsShowCmd = &cobra.Command{
	Use:   "techsupport",
	Short: "collect technical support information",
	Long:  "collect technical support information",
	Run:   tsShowCmdHandler,
}

func init() {
	showCmd.AddCommand(tsShowCmd)
}

func tsShowCmdHandler(cmd *cobra.Command, args []string) {
	fmt.Printf("\n=== Capturing techsupport information ===\n")
	fmt.Printf("\n=== System information ===\n")
	systemShowCmdHandler(nil, nil)
	fmt.Printf("\n=== System packet-buffer statistics ===\n")
	pbShowCmdHandler(nil, nil)
	pbDetailShowCmdHandler(nil, nil)
	fmt.Printf("\n=== System drop statistics ===\n")
	dropShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Port information ===\n")
	portShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Port status ===\n")
	portShowStatusCmdHandler(nil, nil)
	fmt.Printf("\n=== Interface information ===\n")
	ifShowCmdHandler(nil, nil)
	fmt.Printf("\n=== LIF information ===\n")
	lifShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Interrupts ===\n")
	interruptShowCmdHandler(nil, nil)
	fmt.Printf("\n=== Techsupport dump complete ===\n")
}
