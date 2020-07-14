package cmd

import (
	"fmt"
	// "strings"

	"github.com/spf13/cobra"
)

var tsShowCmd = &cobra.Command{
	Use:         "techsupport",
	Short:       "collect technical support information",
	Long:        "collect technical support information",
	Annotations: map[string]string{"techsupport": "false"},
	Run:         tsShowCmdHandler,
}

// default location of the output file
var (
	outDir  = "/tmp/hal_techsupport"
	outFile = "hal-cmds.txt"
)

func init() {
	showCmd.AddCommand(tsShowCmd)
	tsShowCmd.Flags().StringVar(&outDir, "out", outDir, "output directory")
}

var list []*cobra.Command

func genTechSupport(cmd *cobra.Command) {
	// fmt.Printf("\n Getting TS for: " + cmd.Short + "Use Line: " + cmd.UseLine())
	if list == nil {
		list = make([]*cobra.Command, 0)
	}
	val, ok := cmd.Annotations["techsupport"]
	// fmt.Printf("\n val: " + val + " ok: " + strconv.FormatBool(ok) + "\n")
	if !ok || val == "true" {
		if cmd.Run != nil {
			fmt.Printf("\n Adding cmd: " + cmd.UseLine() + "\n")
			list = append(list, cmd)
		}
		for _, x := range cmd.Commands() {
			genTechSupport(x)
		}
	}
}

func tsShowCmdHandler(cmd *cobra.Command, args []string) {
	fmt.Printf("\n=== Capturing halctl techsupport information ===\n")
	cmd.Flags().Set("json", "false")
	cmd.Flags().Set("detail", "true")
	genTechSupport(cmd.Parent())
	for _, myCmd := range list {
		fmt.Printf("\nCmd: " + myCmd.UseLine() + "\n")
		myCmd.Run(myCmd, nil)
	}

	fmt.Printf("Flush HAL logs\n\n")
	flushLogsDebugCmdHandler(nil, nil)
}
