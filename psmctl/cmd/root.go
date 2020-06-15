package cmd

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
)

// rootCmd represents the base command when called without any subcommands
var rootCmd = &cobra.Command{
	Use:               "psmctl",
	Short:             "Pensando PSM CLI",
	Long:              "\n------------------------------------------\n Pensando Policy and Services Manager CLI \n------------------------------------------\n",
	DisableAutoGenTag: true,
	SilenceUsage:      true,
	Args:              cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println(cmd.Help())
	},
}

// Execute adds all child commands to the root command and sets flags appropriately.
// This is called by main.main(). It only needs to happen once in the rootCmd.
func Execute() {
	//TODO: handle doc generation genDocs()
	if err := rootCmd.Execute(); err != nil {
		os.Exit(1)
	}
}

var verbose bool

func init() {
	rootCmd.PersistentFlags().BoolVarP(&verbose, "verbose", "v", false, "display psmctl debug log")
}

func verbosePrintf(format string, a ...interface{}) {
	if verbose {
		fmt.Printf(format+"\n", a...)
	}
}

func errorPrintf(format string, a ...interface{}) {
	fmt.Printf(format+"\n", a...)
	if !verbose {
		fmt.Println("Rerun with --verbose/-v flag for detailed logs.")
	}
}
