package cmd

import (
	context2 "context"
	"fmt"
	"net"
	"os"
	"strconv"
	"time"

	"github.com/spf13/cobra"

	cmd "github.com/pensando/sw/iota/svcs/agent/command"
	constants "github.com/pensando/sw/iota/svcs/common"
	"github.com/pensando/sw/iota/test/venice/iotakit/model"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/common"
	Testbed "github.com/pensando/sw/iota/test/venice/iotakit/testbed"
	"github.com/pensando/sw/venice/utils/log"
)

var rootCmd = &cobra.Command{
	Use:   "iotacmd",
	Short: "commandline to interact with iota based venice setup",
}

var iotaServerExec = fmt.Sprintf("%s/src/github.com/pensando/sw/iota/test/venice/start_iota.sh", os.Getenv("GOPATH"))

// Execute executes a command
func Execute() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}

var (
	configFile, topology, timeout, testbed, suite, focus, nofocus                            string
	debugFlag, dryRun, scale, skipSetup, skipInstall, stopOnError, randomTrigger, regression bool
	modeInfo                                                                                 common.ModelInfo
)

var (
	setupTb    *Testbed.TestBed
	setupModel model.SysModelInterface
)

func errorExit(msg string, err error) {
	fmt.Printf("==> FAILED : %s", msg)
	if err != nil {
		fmt.Printf("\n    --> Error: %s", err)
	}
	fmt.Printf("\n")
	os.Exit(1)
}

func init() {
	cobra.OnInitialize(initialize)
	rootCmd.PersistentFlags().StringVar(&topology, "topo", "", "topology to use")
	rootCmd.PersistentFlags().StringVar(&testbed, "testbed", "/warmd.json", "testbed config file to use")
	rootCmd.PersistentFlags().StringVar(&timeout, "timeout", "", "timeout for the action, default is infinite")
	rootCmd.PersistentFlags().BoolVar(&debugFlag, "debug", false, "enable debug mode")
	rootCmd.PersistentFlags().BoolVar(&dryRun, "dry", false, "dry run commands")
	rootCmd.PersistentFlags().BoolVar(&scale, "scale", false, "dry run commands")

}

// Config contains testbed and topology info
type Config struct {
	Topology string `yaml:"topology"`
	Testbed  string `yaml:"testbed"`
	Timeout  string `yaml:"timeout"`
}

func isServerUp() bool {

	addr := net.JoinHostPort("localhost", strconv.Itoa(constants.IotaSvcPort))
	conn, _ := net.DialTimeout("tcp", addr, 2*time.Second)
	if conn != nil {
		fmt.Printf("Iota server is running already.")
		conn.Close()
		return true
	}
	return false
}

func initialize() {
	// find config file if it exists
	os.Setenv("VENICE_DEV", "1")
	os.Setenv("JOB_ID", "1")

	fmt.Printf("using Topology:[%v] testbed:[%v] timeout[%v]\n", topology, testbed, timeout)
	if dryRun {
		fmt.Printf("dry run skipping testbed :winit \n")
		return
	}
	cfg := log.GetDefaultConfig("venice-iota")
	if debugFlag {
		cfg.Debug = true
		cfg.Filter = log.AllowDebugFilter
	}
	log.SetConfig(cfg)

	if isServerUp() {
		if skipSetup {
			os.Setenv("SKIP_SETUP", "1")
		}
	} else {
		skipSetup = false
		//Start IOTA server and do skip setup
		info, err := cmd.ExecCmd([]string{iotaServerExec, testbed}, "", 0, false, true, os.Environ())
		if err != nil {
			fmt.Printf("Failed to start IOTA server %s\n", err)
		}
		fmt.Printf("Started IOTA server...%v", info.Ctx.Stdout)
		//Remove the model info file as iota server has crashed already
		os.Remove(common.ModelInfoFile)
	}

	if skipInstall {
		os.Setenv("SKIP_INSTALL", "1")
	}

	if recipe != "" {
		tb, err := Testbed.NewTestBed(topology, testbed)
		if err != nil {
			errorExit("failed to setup testbed", err)
		}

		setupModel, err = model.NewSysModel(tb)
		if err != nil || setupModel == nil {
			errorExit("failed to setup model", err)
		}

		err = setupModel.SetupDefaultConfig(context2.TODO(), scale, scale)
		if err != nil {
			errorExit("error setting up default config", err)
		}

		//if SKIP install not set, make sure we do skip now
		os.Setenv("SKIP_INSTALL", "1")
		os.Setenv("SKIP_SETUP", "1")

	} else {

	}
}
