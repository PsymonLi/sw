package cmd

import (
	"fmt"
	"os"

	"github.com/pensando/sw/iota/test/venice/iotakit/model/common"
	"github.com/spf13/cobra"
)

func init() {
	rootCmd.AddCommand(runCommand)
	runCommand.PersistentFlags().BoolVar(&skipSetup, "skip-setup", false, "skips setup")
	runCommand.PersistentFlags().BoolVar(&skipInstall, "skip-install", false, "skips naples install")
	runCommand.PersistentFlags().BoolVar(&regression, "regression", false, "run regression testcases")
	runCommand.PersistentFlags().BoolVar(&stopOnError, "stop-on-error", false, "stops on error")
	runCommand.PersistentFlags().StringVar(&suite, "suite", "", "suite to run")
	runCommand.PersistentFlags().StringVar(&focus, "focus", "", "Focus test")
	runCommand.PersistentFlags().BoolVar(&randomTrigger, "random-trigger", false, "run random trigger")
}

var runCommand = &cobra.Command{
	Use:   "run",
	Short: "Run Tests from specific suite",
	Run:   runCommandAction,
}

func readParams() {

	var err error

	if suite == "" {
		fmt.Printf("suite should be specified\n")
		os.Exit(1)
	}
	if skipSetup {
		modeInfo, err = ReadModel()
		if err != nil {
			fmt.Printf("Model not initialized..")
			os.Exit(1)
		}
		topology = modeInfo.TopoFile
		testbed = modeInfo.TestbedFile
	} else {
		if topology == "" {
			fmt.Printf("topology should be specified\n")
			os.Exit(1)
		}
		topology = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/test/venice/iotakit/topos/%s.topo", os.Getenv("GOPATH"), topology)
		if testbed == "" {
			fmt.Printf("testbed should be specified\n")
			os.Exit(1)
		}

		//remove teh file as we are starting fresh
		os.Remove(common.ModelInfoFile)
	}

}
func runCommandAction(cmd *cobra.Command, args []string) {

	os.Setenv("VENICE_DEV", "1")
	os.Setenv("JOB_ID", "1")

	readParams()

	st := testsuite{name: suite, path: suiteDirectory + "/" + suite, focus: focus,
		scaleData: scale, runRandomTrigger: randomTrigger, stopOnError: stopOnError, regression: regression}

	err := st.run(skipSetup, skipInstall, false, false, "")
	if err != nil {
		errorExit("Error running suite", nil)
	}

}
