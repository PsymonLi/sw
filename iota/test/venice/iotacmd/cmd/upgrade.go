package cmd

import (
	"errors"
	"fmt"
	"os"
	"time"

	constants "github.com/pensando/sw/iota/svcs/common"
	"github.com/pensando/sw/iota/test/venice/iotakit/model"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/common"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	Testbed "github.com/pensando/sw/iota/test/venice/iotakit/testbed"
	"github.com/spf13/cobra"
)

var (
	branch, version, pipeline string
)

func init() {
	rootCmd.AddCommand(upgradeCmd)

	upgradeCmd.PersistentFlags().StringVar(&pipeline, "pipeline", "iris", "pipeline")
	upgradeCmd.AddCommand(upgradeAll)
	upgradeCmd.AddCommand(upgradeVenice)
	upgradeCmd.AddCommand(upgradeNaples)

	upgradeVenice.AddCommand(upgradeVeniceLocal)
	upgradeNaples.AddCommand(upgradeNaplesLocal)
	upgradeAll.AddCommand(upgradeAllLocal)

	upgradeVeniceBranch.PersistentFlags().StringVar(&branch, "branch", "master", "Branch to pull")
	upgradeVeniceBranch.PersistentFlags().StringVar(&version, "version", "latest", "version")
	upgradeNaplesBranch.PersistentFlags().StringVar(&branch, "branch", "master", "Branch to pull")
	upgradeNaplesBranch.PersistentFlags().StringVar(&version, "version", "latest", "version")
	upgradeAllBranch.PersistentFlags().StringVar(&branch, "branch", "master", "Branch to pull")
	upgradeAllBranch.PersistentFlags().StringVar(&version, "version", "latest", "version")
	upgradeVenice.AddCommand(upgradeVeniceBranch)
	upgradeNaples.AddCommand(upgradeNaplesBranch)
	upgradeAll.AddCommand(upgradeAllBranch)
}

var upgradeCmd = &cobra.Command{
	Use:   "upgrade",
	Short: "upgrade venice/naples",
}

var upgradeVenice = &cobra.Command{
	Use:   "venice",
	Short: "upgradeVenice",
}

var upgradeNaples = &cobra.Command{
	Use:   "naples",
	Short: "upgrade Naples",
}

var upgradeVeniceLocal = &cobra.Command{
	Use:   "local",
	Short: "upgrade local venice/naples",
	Run:   upgradeVeniceLocalAction,
}

var upgradeNaplesLocal = &cobra.Command{
	Use:   "local",
	Short: "upgrade venice venice/naples",
	Run:   upgradeNaplesLocalAction,
}

var upgradeAll = &cobra.Command{
	Use:   "all",
	Short: "upgrade all",
}

var upgradeAllLocal = &cobra.Command{
	Use:   "local",
	Short: "upgrade all",
	Run:   upgradeAllLocalAction,
}

var upgradeVeniceBranch = &cobra.Command{
	Use:   "target",
	Short: "upgrade local venice/naples",
	Run:   upgradeVeniceBranchAction,
}

var upgradeNaplesBranch = &cobra.Command{
	Use:   "target",
	Short: "upgrade venice venice/naples",
	Run:   upgradeNaplesBranchAction,
}

var upgradeAllBranch = &cobra.Command{
	Use:   "target",
	Short: "upgrade venice venice/naples",
	Run:   upgradeAllBranchAction,
}

func commonUpgrade(spec common.RolloutSpec) error {

	skipSetup = true
	readParams()
	spec.Pipeline = pipeline
	spec.SkipSimDSC = true
	fmt.Printf("\nReading current setup....\n")

	os.Setenv("SKIP_SETUP", "1")
	tb, err := Testbed.NewTestBed(topology, testbed)
	if err != nil {
		errorExit("failed to setup testbed", err)
	}

	setupModel, err = model.NewSysModel(tb)
	if err != nil || setupModel == nil {
		errorExit("failed to setup model", err)
	}

	fmt.Printf("\nPerforming upgrade ....\n")

	numNaples := 0
	setupModel.ForEachNaples(func(nc *objects.NaplesCollection) error {
		_, err := setupModel.RunNaplesCommand(nc, "touch /data/upgrade_to_same_firmware_allowed")
		numNaples++
		return err
	})

	defer setupModel.ForEachNaples(func(nc *objects.NaplesCollection) error {
		_, err := setupModel.RunNaplesCommand(nc, "rm /data/upgrade_to_same_firmware_allowed")
		return err
	})

	setupModel.ForEachFakeNaples(func(nc *objects.NaplesCollection) error {
		numNaples++
		return nil
	})

	simDone := make(chan error)
	upgradeSim := func() {
		if spec.SkipDSC {
			simDone <- nil
			return
		}
		if err := setupModel.AssociateHosts(); err != nil {
			simDone <- err
			return
		}
		for _, host := range setupModel.Hosts().FakeHosts {
			setupModel.Testbed().CopyToHost(host.Name(), []string{setupModel.Testbed().Topo.NaplesSimImage}, constants.ImageArtificatsDirectory)
		}

		simDone <- setupModel.ReloadFakeHosts(setupModel.Hosts())
	}

	rollout, err := setupModel.GetRolloutObject(spec, false)
	if err != nil {
		return err
	}

	err = setupModel.PerformRollout(rollout, true, "upgrade-bundle")
	if err != nil {
		return err
	}

	go upgradeSim()

	err = setupModel.VerifyRolloutStatus(rollout.Name)
	if err != nil {
		return fmt.Errorf("Rollout failed %v", err)
	}

	err = <-simDone
	if err != nil {
		return fmt.Errorf("Error upgrading sim %v", err)
	}
	TimedOutEvent := time.After(60 * time.Second)
L:
	for true {
		select {
		case <-TimedOutEvent:
			return errors.New("Error waiting cluster to be healthy, timed out")
		default:
			err = setupModel.VerifyClusterStatus()
			if err == nil {
				break L
			}
			time.Sleep(5 * time.Second)
		}
	}

	fmt.Printf("Upgrade successfully done.")

	return nil
}

func upgradeNaplesBranchAction(cmd *cobra.Command, args []string) {
	fmt.Printf("Upgrade venice Branch action....")
	fmt.Printf("Doing Naples  using image %v:%v", branch, version)
	err := commonUpgrade(common.RolloutSpec{
		TargetBranch:  branch,
		TargetVersion: version,
		SkipVenice:    true,
	})

	if err != nil {
		fmt.Printf("Doing venice upgrade failed %v", err)
	}
}

func upgradeVeniceBranchAction(cmd *cobra.Command, args []string) {
	// TBD
	fmt.Printf("Upgrade venice Branch action....")
	fmt.Printf("Doing Venice using image %v:%v", branch, version)
	err := commonUpgrade(common.RolloutSpec{
		TargetBranch:  branch,
		TargetVersion: version,
		SkipDSC:       true,
	})

	if err != nil {
		fmt.Printf("Doing venice upgrade failed %v", err)
	}
}

func upgradeAllLocalAction(cmd *cobra.Command, args []string) {

	fmt.Printf("Doing Venice & Naples upgrade using local image......")
	err := commonUpgrade(common.RolloutSpec{
		Local: true,
	})

	if err != nil {
		fmt.Printf("Doing upgrade all failed %v", err)
	}
}

func upgradeAllBranchAction(cmd *cobra.Command, args []string) {
	// TBD
	fmt.Printf("Upgrade  All branch action....")

	fmt.Printf("Doing Venice & Naples upgrade using image %v:%v", branch, version)
	err := commonUpgrade(common.RolloutSpec{
		TargetBranch:  branch,
		TargetVersion: version,
	})

	if err != nil {
		fmt.Printf("Doing venice upgrade failed %v", err)
	}
}

func upgradeVeniceLocalAction(cmd *cobra.Command, args []string) {
	fmt.Printf("Doing Venice upgrade using local image......")
	err := commonUpgrade(common.RolloutSpec{
		Local:   true,
		SkipDSC: true,
	})

	if err != nil {
		fmt.Printf("Doing venice upgrade all failed %v", err)
	}
}

func upgradeNaplesLocalAction(cmd *cobra.Command, args []string) {

	fmt.Printf("Doing Naples upgrade using local image......")
	err := commonUpgrade(common.RolloutSpec{
		Local:      true,
		SkipVenice: true,
	})

	if err != nil {
		fmt.Printf("Doing naples upgrade all failed %v", err)
	}
}
