package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"

	"os"
	"os/exec"
)

// RunnerParams are the parameters passed to simulator
type RunnerParams struct {
	// Instances are the number of simulators to be started
	Instances int

	// StartPort
	StartPort int

	// ConfigPath is the IOTA generated config file
	ConfigPath string

	// OrchSimPath
	OrchSimPath string
}

var (
	params RunnerParams
)

func validateParams() error {
	if len(params.ConfigPath) == 0 || len(params.OrchSimPath) == 0 {
		return fmt.Errorf("Scale config path and orch-sim path must be passed")
	}

	if _, err := os.Stat(params.ConfigPath); err != nil {
		return err
	}

	if _, err := os.Stat(params.OrchSimPath); err != nil {
		return err
	}

	return nil
}

func runCmd(cmd string, background bool) error {
	bin := exec.Command("sh", "-c", cmd)

	if background {
		return bin.Start()
	}

	_, err := bin.Output()
	return err
}

func main() {
	instPtr := flag.Int("n", 1, "instances of compute orchestrator simulator to be created")
	startPortPtr := flag.Int("p", 10000, "Start port")
	flag.StringVar(&params.ConfigPath, "c", "", "IOTA generated config file")
	flag.StringVar(&params.OrchSimPath, "o", "", "Path of the orch-sim binary")

	flag.Parse()

	params.Instances = *instPtr
	params.StartPort = *startPortPtr

	if err := validateParams(); err != nil {
		fmt.Printf("\nParameter validation failed. Err : %v\n", err)
		return
	}

	err := runCmd("pkill -SIGKILL orch-sim", false)
	if err != nil {
		fmt.Printf("Failed to kill all orch-sim. Err : %v", err)
	}

	jsonFile, err := os.Open(params.ConfigPath)
	if err != nil {
		fmt.Printf("Err : %v", err)
		return
	}

	defer jsonFile.Close()

	byteValue, _ := ioutil.ReadAll(jsonFile)
	var result map[string]interface{}
	json.Unmarshal([]byte(byteValue), &result)

	hosts := result["Hosts"].([]interface{})
	hostCount := len(hosts)
	hostPerSim := int(hostCount / params.Instances)
	curHostIdx := 0
	for i := 0; i < params.Instances; i++ {
		// ./orch-sim -port 20000 -c scale-cfg.json -s 3 -e 6
		port := params.StartPort + i
		hostStartIdx := curHostIdx
		hostEndIdx := curHostIdx + hostPerSim - 1
		cmd := fmt.Sprintf("%v -port %d -c %s -s %d -e %d 1>orch-%d.stdout 2>orch-%d.stderr", params.OrchSimPath, port, params.ConfigPath, hostStartIdx, hostEndIdx, port, port)
		fmt.Printf("\n%s\n", cmd)
		err := runCmd(cmd, true)
		if err != nil {
			fmt.Printf("failed to run command. Err : %v", err)
		}
		curHostIdx = curHostIdx + hostPerSim
	}

}
