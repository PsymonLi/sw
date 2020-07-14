package cmd

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	"github.com/olekukonko/tablewriter"
	iota "github.com/pensando/sw/iota/protos/gogen"
	"github.com/pensando/sw/iota/test/venice/iotakit/model"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/common"
	Testbed "github.com/pensando/sw/iota/test/venice/iotakit/testbed"
	"github.com/spf13/cobra"
)

func init() {
	rootCmd.AddCommand(showCmd)
	showCmd.AddCommand(showTestbedCmd)
	showTestbedCmd.AddCommand(showWorkloadCmd)
}

var showCmd = &cobra.Command{
	Use:   "show",
	Short: "Show information",
	Run:   showInfo,
}

var showTestbedCmd = &cobra.Command{
	Use:   "testbed",
	Short: "Show testbed information",
	Run:   showTestBedAction,
}

var showWorkloadCmd = &cobra.Command{
	Use:   "workload",
	Short: "Show running workload information",
	Run:   showWorkloadAction,
}

func initModel() {

	skipSetup = true
	suite = "dont-care"
	readParams()
	os.Setenv("SKIP_SETUP", "1")
	os.Setenv("SKIP_CONFIG", "1")
	tb, err := Testbed.NewTestBed(topology, testbed)
	if err != nil {
		errorExit("failed to setup testbed", err)
	}

	setupModel, err = model.NewSysModel(tb)
	if err != nil || setupModel == nil {
		errorExit("failed to setup model", err)
	}

	err = setupModel.AssociateHosts()
	if err != nil {
		errorExit("Error associating hosts %v", err)
	}

	err = setupModel.InitConfig(scale, scale)
	if err != nil {
		errorExit("Init config failed %v", err)
	}
}

//ReadModel read the model if present
func ReadModel() (common.ModelInfo, error) {

	modeInfo := common.ModelInfo{}

	jsonFile, err := os.OpenFile(common.ModelInfoFile, os.O_RDONLY, 0755)
	if err != nil {
		return modeInfo, err
	}
	byteValue, _ := ioutil.ReadAll(jsonFile)

	err = json.Unmarshal(byteValue, &modeInfo)
	if err != nil {
		return modeInfo, err
	}
	jsonFile.Close()
	return modeInfo, nil

}

func showInfo(cmd *cobra.Command, args []string) {

	skipSetup = true
	suite = "dont-care"
	readParams()
	table := tablewriter.NewWriter(os.Stdout)
	table.SetHeader([]string{"Testbed-ID", "Model", "Testbed-File", "Topology"})
	table.SetAutoMergeCells(true)
	table.SetBorder(false) // Set Border to false
	table.SetRowLine(true) // Enable row line
	// Change table lines
	table.SetCenterSeparator("*")
	table.SetColumnSeparator("╪")
	table.SetRowSeparator("-")
	data := [][]string{}

	data = append(data, []string{modeInfo.TestbedID, modeInfo.Type, modeInfo.TestbedFile,
		strings.Split(filepath.Base(modeInfo.TopoFile), ".")[0]})

	table.AppendBulk(data) // Add Bulk Data
	fmt.Printf("\n")
	table.Render()

}

func showTestBedAction(cmd *cobra.Command, args []string) {

	skipSetup = true
	initModel()

	table := tablewriter.NewWriter(os.Stdout)
	table.SetHeader([]string{"Name", "Type", "Personality", "CIMC", "HostIP", "NaplesIP", "Console"})
	table.SetAutoMergeCells(true)
	table.SetBorder(false) // Set Border to false
	table.SetRowLine(true) // Enable row line
	// Change table lines
	table.SetCenterSeparator("*")
	table.SetColumnSeparator("╪")
	table.SetRowSeparator("-")
	data := [][]string{}
	for _, node := range setupModel.Testbed().Nodes {
		switch node.Personality {
		case iota.PersonalityType_PERSONALITY_VENICE_BM:
			fallthrough
		case iota.PersonalityType_PERSONALITY_VENICE:
			data = append(data, []string{node.NodeName, node.Type.String(), node.Personality.String(), node.InstanceParams().NodeCimcIP,
				node.InstanceParams().NodeMgmtIP, "N/A", "N/A"})
		case iota.PersonalityType_PERSONALITY_NAPLES:
			fallthrough
		case iota.PersonalityType_PERSONALITY_NAPLES_DVS:
			for _, nic := range node.InstanceParams().Nics {
				data = append(data, []string{node.NodeName, node.Type.String(), node.Personality.String(), node.InstanceParams().NodeCimcIP,
					node.InstanceParams().NodeMgmtIP, nic.MgmtIP,
					nic.ConsoleIP + ":" + nic.ConsolePort})

			}
		case iota.PersonalityType_PERSONALITY_VCENTER_NODE:
			data = append(data, []string{node.NodeName, node.Type.String(), node.Personality.String(), node.InstanceParams().NodeCimcIP,
				node.InstanceParams().NodeMgmtIP, "N/A", "N/A"})
		}
	}
	table.AppendBulk(data) // Add Bulk Data
	fmt.Printf("\n")
	table.Render()
}

func showWorkloadAction(cmd *cobra.Command, args []string) {
	initModel()

	err := setupModel.AssociateWorkloads()
	if err != nil {
		errorExit("Error associating workloads %v", err)
	}
	table := tablewriter.NewWriter(os.Stdout)
	table.SetHeader([]string{"Name", "Host", "HostIP", "Type", "MgmtIP", "DataIP", "NaplesIP", "Console"})
	table.SetAutoMergeCells(true)
	table.SetBorder(false) // Set Border to false
	table.SetRowLine(true) // Enable row line
	// Change table lines
	table.SetCenterSeparator("*")
	table.SetColumnSeparator("╪")
	table.SetRowSeparator("-")
	data := [][]string{}
	nodeMap := make(map[string]*Testbed.TestNode)
	for _, node := range setupModel.Testbed().Nodes {
		nodeMap[node.NodeName] = node
	}
	for _, wrk := range setupModel.Workloads().Workloads {
		node, _ := nodeMap[wrk.NodeName()]
		data = append(data, []string{wrk.Name(), wrk.NodeName(), node.NodeMgmtIP, wrk.GetIotaWorkload().GetWorkloadType().String(),
			wrk.GetMgmtIP(), wrk.GetIP(), node.InstanceParams().Nics[0].MgmtIP,
			node.InstanceParams().Nics[0].ConsoleIP + ":" + node.InstanceParams().Nics[0].ConsolePort})
	}
	table.AppendBulk(data) // Add Bulk Data
	fmt.Printf("\n")
	table.Render()
}
