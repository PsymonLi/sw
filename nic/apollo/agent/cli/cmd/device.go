//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	"reflect"
	"strings"

	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/apollo/agent/cli/utils"
	"github.com/pensando/sw/nic/apollo/agent/gen/pds"
)

var (
	deviceProfile string
	deviceTimeout uint32
)

var deviceShowCmd = &cobra.Command{
	Use:   "device",
	Short: "show Device information",
	Long:  "show Device object information",
	Run:   deviceShowCmdHandler,
}

var deviceUpdateCmd = &cobra.Command{
	Use:   "device",
	Short: "update Device information",
	Long:  "update Device object information",
	Run:   deviceUpdateCmdHandler,
}

func init() {
	showCmd.AddCommand(deviceShowCmd)
	deviceShowCmd.Flags().Bool("yaml", false, "Output in yaml")

	debugCmd.AddCommand(deviceUpdateCmd)
	deviceUpdateCmd.Flags().StringVar(&deviceProfile, "profile", "default", "Specify device profile (Ex: default, p1, p2)")
	deviceUpdateCmd.Flags().Uint32Var(&deviceTimeout, "learn-age-timeout", 300, "Specify device aging timeout for learned MAC/IP in secs (Valid: 30-86400)")
}

func deviceUpdateCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS. Is PDS Running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	if cmd.Flags().Changed("profile") == false &&
		cmd.Flags().Changed("learn-age-timeout") == false {
		fmt.Printf("No arguments specified. Refer to help string\n")
		return
	}

	var req *pds.Empty

	client := pds.NewDeviceSvcClient(c)

	// PDS call
	resp, err := client.DeviceGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Device failed. %v\n", err)
		return
	}

	spec := resp.GetResponse().GetSpec()

	var updateSpec *pds.DeviceSpec
	if cmd.Flags().Changed("profile") {
		if isValidDeviceProfile(deviceProfile) == false {
			fmt.Printf("Invalid device profile specified. Must be one of default, p1 & p2\n")
			return
		}
		updateSpec = &pds.DeviceSpec{
			IPAddr:           spec.GetIPAddr(),
			MACAddr:          spec.GetMACAddr(),
			GatewayIP:        spec.GetGatewayIP(),
			DevOperMode:      spec.GetDevOperMode(),
			BridgingEn:       spec.GetBridgingEn(),
			LearningEn:       spec.GetLearningEn(),
			OverlayRoutingEn: spec.GetOverlayRoutingEn(),
			LearnAgeTimeout:  spec.GetLearnAgeTimeout(),
			Profile:          inputToDeviceProfile(deviceProfile),
		}
	} else if cmd.Flags().Changed("learn-age-timeout") {
		if deviceTimeout < 30 || deviceTimeout > 86400 {
			fmt.Printf("Invalid learning age timeout specified. Valid range 30-86400")
			return
		}
		updateSpec = &pds.DeviceSpec{
			IPAddr:           spec.GetIPAddr(),
			MACAddr:          spec.GetMACAddr(),
			GatewayIP:        spec.GetGatewayIP(),
			DevOperMode:      spec.GetDevOperMode(),
			BridgingEn:       spec.GetBridgingEn(),
			LearningEn:       spec.GetLearningEn(),
			OverlayRoutingEn: spec.GetOverlayRoutingEn(),
			Profile:          spec.GetProfile(),
			LearnAgeTimeout:  deviceTimeout,
		}
	}

	updateReq := &pds.DeviceRequest{
		Request: updateSpec,
	}

	updateResp, updateErr := client.DeviceUpdate(context.Background(), updateReq)
	if updateErr != nil {
		fmt.Printf("Device update failed. %v\n", updateErr)
	}

	if updateResp.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", updateResp.ApiStatus)
		return
	}

	fmt.Printf("Device update succeeded.\n")
}

func isValidDeviceProfile(str string) bool {
	str = strings.ToLower(str)

	if strings.Compare(str, "default") == 0 {
		return true
	} else if strings.Compare(str, "p1") == 0 {
		return true
	} else if strings.Compare(str, "p2") == 0 {
		return true
	}

	return false
}

func inputToDeviceProfile(str string) pds.DeviceProfile {
	str = strings.ToLower(str)

	if strings.Compare(str, "default") == 0 {
		return pds.DeviceProfile_DEVICE_PROFILE_DEFAULT
	} else if strings.Compare(str, "p1") == 0 {
		return pds.DeviceProfile_DEVICE_PROFILE_P1
	} else if strings.Compare(str, "p2") == 0 {
		return pds.DeviceProfile_DEVICE_PROFILE_P2
	} else {
		return pds.DeviceProfile_DEVICE_PROFILE_DEFAULT
	}
}

func deviceShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS. Is PDS Running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewDeviceSvcClient(c)

	var req *pds.Empty

	// PDS call
	resp, err := client.DeviceGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Device failed. %v\n", err)
		return
	}

	// Print Device
	if resp.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		return
	}

	if cmd != nil && cmd.Flags().Changed("yaml") {
		respType := reflect.ValueOf(resp)
		b, _ := yaml.Marshal(respType.Interface())
		fmt.Println(string(b))
		fmt.Println("---")
	} else {
		printDeviceHeader()
		printDevice(resp)
	}
}

func printDeviceHeader() {
	hdrLine := strings.Repeat("-", 156)
	fmt.Println(hdrLine)
	fmt.Printf("%-16s%-20s%-16s%-10s%-12s%-12s%-16s%-10s%-18s%-20s%-6s\n",
		"IPAddr", "MACAddr", "GatewayIP",
		"Profile", "BridgingEn", "LearningEn", "LearnAgeTimeout",
		"OperMode", "OverlayRoutingEn", "FRU MAC", "Memory")
	fmt.Println(hdrLine)
}

func printDevice(resp *pds.DeviceGetResponse) {
	spec := resp.GetResponse().GetSpec()
	status := resp.GetResponse().GetStatus()
	memoryStr := fmt.Sprintf("%dG", status.GetMemory())
	fmt.Printf("%-16s%-20s%-16s%-10s%-12t%-12t%-16d%-10s%-18t%-20s%-6s\n",
		utils.IPAddrToStr(spec.GetIPAddr()),
		utils.MactoStr(spec.GetMACAddr()),
		utils.IPAddrToStr(spec.GetGatewayIP()),
		strings.Replace(spec.GetProfile().String(), "DEVICE_PROFILE_", "", -1),
		spec.GetBridgingEn(), spec.GetLearningEn(), spec.GetLearnAgeTimeout(),
		strings.Replace(spec.GetDevOperMode().String(), "DEVICE_OPER_MODE_", "", -1),
		spec.GetOverlayRoutingEn(), utils.MactoStr(status.GetSystemMACAddress()),
		memoryStr)
}
