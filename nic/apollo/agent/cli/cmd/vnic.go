//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

// +build apulu apollo

package cmd

import (
	"context"
	"fmt"
	"reflect"
	"strings"

	"google.golang.org/grpc"

	uuid "github.com/satori/go.uuid"
	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/apollo/agent/cli/utils"
	"github.com/pensando/sw/nic/apollo/agent/gen/pds"
)

var (
	vnicID     string
	txMirrorID string
	rxMirrorID string
)

var vnicShowCmd = &cobra.Command{
	Use:   "vnic",
	Short: "show vnic information",
	Long:  "show vnic object information",
	Run:   vnicShowCmdHandler,
}

var vnicShowStatisticsCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show vnic statistics",
	Long:  "show vnic statistics",
	Run:   vnicShowStatisticsCmdHandler,
}

var vnicUpdateCmd = &cobra.Command{
	Use:     "vnic",
	Short:   "update vnic object",
	Long:    "update vnic object",
	Run:     vnicUpdateCmdHandler,
	PreRunE: vnicUpdateCmdPreRunE,
}

var vnicClearCmd = &cobra.Command{
	Use:   "vnic",
	Short: "clear vnic information",
	Long:  "clear vnic information",
}

var vnicClearStatsCmd = &cobra.Command{
	Use:   "statistics",
	Short: "clear vnic statistics",
	Long:  "clear vnic statistics",
	Run:   vnicClearStatsCmdHandler,
}

func init() {
	showCmd.AddCommand(vnicShowCmd)
	vnicShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	vnicShowCmd.Flags().Bool("summary", false, "Display number of objects")
	vnicShowCmd.Flags().Bool("brief", false, "Display brief output")
	vnicShowCmd.Flags().StringVarP(&vnicID, "id", "i", "", "Specify vnic ID")
	vnicShowCmd.Flags().MarkHidden("brief")

	vnicShowCmd.AddCommand(vnicShowStatisticsCmd)
	vnicShowStatisticsCmd.Flags().Bool("yaml", false, "Output in yaml")
	vnicShowStatisticsCmd.Flags().StringVarP(&vnicID, "id", "i", "", "Specify vnic ID")

	clearCmd.AddCommand(vnicClearCmd)
	vnicClearCmd.AddCommand(vnicClearStatsCmd)
	vnicClearStatsCmd.Flags().StringVarP(&vnicID, "id", "i", "", "Specify vnic ID")

	debugUpdateCmd.AddCommand(vnicUpdateCmd)
	vnicUpdateCmd.Flags().StringVarP(&vnicID, "id", "i", "", "Specify vnic ID")
	vnicUpdateCmd.Flags().StringVarP(&txMirrorID, "tx-mirror-id", "t", "", "Specify TX Mirror Session IDs in comma separated list")
	vnicUpdateCmd.Flags().StringVarP(&rxMirrorID, "rx-mirror-id", "r", "", "Specify RX Mirror Session IDs in comma separated list")
}

func vnicUpdateCmdPreRunE(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	if !cmd.Flags().Changed("tx-mirror-id") &&
		!cmd.Flags().Changed("rx-mirror-id") {
		return fmt.Errorf("Nothing to update")
	}
	return nil

}

func vnicUpdateCmdHandler(cmd *cobra.Command, args []string) {
	// connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewVnicSvcClient(c)
	getReq := &pds.VnicGetRequest{
		Id: [][]byte{uuid.FromStringOrNil(vnicID).Bytes()},
	}

	// PDS call
	getRespMsg, err := client.VnicGet(context.Background(), getReq)
	if err != nil {
		fmt.Printf("Getting Vnic failed, err %v\n", err)
		return
	}
	if getRespMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", getRespMsg.ApiStatus)
		return
	}
	getResp := getRespMsg.GetResponse()
	vnicSpec := getResp[0].GetSpec()
	txMirror := vnicSpec.GetTxMirrorSessionId()
	rxMirror := vnicSpec.GetRxMirrorSessionId()
	if cmd.Flags().Changed("tx-mirror-id") {
		ids := strings.Split(txMirrorID, ",")
		txMirror = make([][]byte, len(ids), len(ids))
		for i := 0; i < len(ids); i++ {
			txMirror[i] = uuid.FromStringOrNil(ids[i]).Bytes()
		}
	}
	if cmd.Flags().Changed("rx-mirror-id") {
		ids := strings.Split(rxMirrorID, ",")
		rxMirror = make([][]byte, len(ids), len(ids))
		for i := 0; i < len(ids); i++ {
			rxMirror[i] = uuid.FromStringOrNil(ids[i]).Bytes()
		}
	}
	req := &pds.VnicSpec{
		Id:                    vnicSpec.GetId(),
		SubnetId:              vnicSpec.GetSubnetId(),
		VnicEncap:             vnicSpec.GetVnicEncap(),
		MACAddress:            vnicSpec.GetMACAddress(),
		SourceGuardEnable:     vnicSpec.GetSourceGuardEnable(),
		FabricEncap:           vnicSpec.GetFabricEncap(),
		TxMirrorSessionId:     txMirror,
		RxMirrorSessionId:     rxMirror,
		SwitchVnic:            vnicSpec.GetSwitchVnic(),
		V4MeterId:             vnicSpec.GetV4MeterId(),
		V6MeterId:             vnicSpec.GetV6MeterId(),
		IngV4SecurityPolicyId: vnicSpec.GetIngV4SecurityPolicyId(),
		IngV6SecurityPolicyId: vnicSpec.GetIngV6SecurityPolicyId(),
		EgV4SecurityPolicyId:  vnicSpec.GetEgV4SecurityPolicyId(),
		EgV6SecurityPolicyId:  vnicSpec.GetEgV6SecurityPolicyId(),
		HostIf:                vnicSpec.GetHostIf(),
		TxPolicerId:           vnicSpec.GetTxPolicerId(),
		RxPolicerId:           vnicSpec.GetRxPolicerId(),
		Primary:               vnicSpec.GetPrimary(),
		HostName:              vnicSpec.GetHostName(),
		MaxSessions:           vnicSpec.GetMaxSessions(),
		FlowLearnEn:           vnicSpec.GetFlowLearnEn(),
		MeterEn:               vnicSpec.GetMeterEn(),
	}
	reqMsg := &pds.VnicRequest{
		Request: []*pds.VnicSpec{
			req,
		},
	}

	// PDS call
	respMsg, err := client.VnicUpdate(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Updating Vnic failed, err %v\n", err)
		return
	}
	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}
	fmt.Printf("Updating Vnic succeeded\n")
}

func vnicClearStatsCmdHandler(cmd *cobra.Command, args []string) {
	// connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewVnicSvcClient(c)
	var req *pds.Id
	if cmd != nil && cmd.Flags().Changed("id") {
		// clear stats for given vnic
		req = &pds.Id{
			Id: uuid.FromStringOrNil(vnicID).Bytes(),
		}
	} else {
		// clear stats for all vnics
		req = &pds.Id{
			Id: []byte{},
		}
	}

	// PDS call
	_, err = client.VnicStatsReset(context.Background(), req)
	if err != nil {
		fmt.Printf("Clearing vnic stats failed, err %v\n", err)
		return
	}
}

func vnicShowCmdHandler(cmd *cobra.Command, args []string) {
	respMsg := &pds.VnicGetResponse{}

	var req *pds.VnicGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// Get specific Vnic
		req = &pds.VnicGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(vnicID).Bytes()},
		}
	} else {
		// Get all Vnics
		req = &pds.VnicGetRequest{
			Id: [][]byte{},
		}
	}

	AgentTransport, err := GetAgentTransport(cmd)
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		return
	}

	if AgentTransport == AGENT_TRANSPORT_UDS {
		err = HandleSvcReqConfigMsg(pds.ServiceRequestOp_SERVICE_OP_READ,
			req, respMsg)
	} else {
		// Connect to PDS
		var c *grpc.ClientConn
		c, err = utils.CreateNewGRPCClient()
		if err != nil {
			fmt.Printf("Could not connect to the PDS, is PDS running?\n")
			return
		}
		defer c.Close()

		if len(args) > 0 {
			fmt.Printf("Invalid argument\n")
			return
		}

		client := pds.NewVnicSvcClient(c)

		// PDS call
		respMsg, err = client.VnicGet(context.Background(), req)
	}

	if err != nil {
		fmt.Printf("Getting Vnic failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print Vnics
	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		printVnicSummary(len(respMsg.Response))
	} else if cmd != nil && cmd.Flags().Changed("brief") {
		printVnicHeader()
		for _, resp := range respMsg.Response {
			printVnic(resp)
		}
		printVnicSummary(len(respMsg.Response))
	} else {
		for _, resp := range respMsg.Response {
			printVnicDetail(resp)
		}
		printVnicSummary(len(respMsg.Response))
	}
}

func printVnicSummary(count int) {
	fmt.Printf("\nNo. of vnics : %d\n\n", count)
}

func printVnicHeader() {
	hdrLine := strings.Repeat("-", 159)
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-40s%-14s%-20s%-10s%-14s%-11s%-10s\n",
		"VnicID", "SubnetID", "VnicEncap", "MAC", "SrcGuard", "FabricEncap",
		"SwitchVnic", "HostIf")
	fmt.Println(hdrLine)
}

func printVnic(vnic *pds.Vnic) {
	spec := vnic.GetSpec()
	fabricEncapStr := utils.EncapToString(spec.GetFabricEncap())
	vnicEncapStr := utils.EncapToString(spec.GetVnicEncap())
	ifName := "-"
	if len(spec.GetHostIf()) > 0 {
		ifName = hostIfGetNameFromKey(spec.GetHostIf())
	}

	fmt.Printf("%-40s%-40s%-14s%-20s%-10t%-14s%-11t%-10s\n",
		utils.IdToStr(spec.GetId()),
		utils.IdToStr(spec.GetSubnetId()), vnicEncapStr,
		utils.MactoStr(spec.GetMACAddress()), spec.GetSourceGuardEnable(),
		fabricEncapStr, spec.GetSwitchVnic(), ifName)
}

func printVnicDetail(vnic *pds.Vnic) {
	spec := vnic.GetSpec()
	if spec == nil {
		fmt.Printf("-")
		return
	}

	ingressV4Policy := spec.GetIngV4SecurityPolicyId()
	egressV4Policy := spec.GetEgV4SecurityPolicyId()
	ingressV6Policy := spec.GetIngV6SecurityPolicyId()
	egressV6Policy := spec.GetEgV6SecurityPolicyId()
	v4Meter := spec.GetV4MeterId()
	v6Meter := spec.GetV6MeterId()
	txPolicer := spec.GetTxPolicerId()
	rxPolicer := spec.GetRxPolicerId()
	isPrimary := spec.GetPrimary()
	hostName := spec.GetHostName()
	maxSessions := spec.GetMaxSessions()
	flowLearn := spec.GetFlowLearnEn()
	meterEn := spec.GetMeterEn()
	fabricEncapStr := utils.EncapToString(spec.GetFabricEncap())
	vnicEncapStr := utils.EncapToString(spec.GetVnicEncap())
	ifName := "-"
	if len(spec.GetHostIf()) > 0 {
		ifName = hostIfGetNameFromKey(spec.GetHostIf())
	}
	srcGuardStr := "Disabled"
	if spec.GetSourceGuardEnable() {
		srcGuardStr = "Enabled"
	}

	fmt.Printf("%-30s    : %s\n", "Vnic ID",
		utils.IdToStr(spec.GetId()))
	fmt.Printf("%-30s    : %s\n", "Subnet ID",
		utils.IdToStr(spec.GetSubnetId()))
	fmt.Printf("%-30s    : %s\n", "Vnic Encap", vnicEncapStr)
	fmt.Printf("%-30s    : %s\n", "MAC address",
		utils.MactoStr(spec.GetMACAddress()))
	fmt.Printf("%-30s    : %s\n", "Source Guard", srcGuardStr)
	fmt.Printf("%-30s    : %s\n", "Fabric Encap", fabricEncapStr)
	if len(spec.GetRxMirrorSessionId()) != 0 {
		keyStr := fmt.Sprintf("%-30s    : ", "Rx Mirror Session")
		for _, session := range spec.GetRxMirrorSessionId() {
			fmt.Printf("%-36s%s\n", keyStr, utils.IdToStr(session))
			keyStr = ""
		}
	} else {
		fmt.Printf("%-30s    : %s\n", "Rx Mirror Session", "-")
	}
	if len(spec.GetTxMirrorSessionId()) != 0 {
		keyStr := fmt.Sprintf("%-30s    : ", "Tx Mirror Session")
		for _, session := range spec.GetTxMirrorSessionId() {
			fmt.Printf("%-36s%s\n", keyStr, utils.IdToStr(session))
			keyStr = ""
		}
	} else {
		fmt.Printf("%-30s    : %s\n", "Tx Mirror Session", "-")
	}
	fmt.Printf("%-30s    : %t\n", "Switch Vnic", spec.GetSwitchVnic())
	fmt.Printf("%-30s    : %s\n", "Host Interface", ifName)
	fmt.Printf("%-30s    : %s\n", "Host Name", hostName)
	fmt.Printf("%-30s    : %t\n", "Primary Vnic", isPrimary)
	fmt.Printf("%-30s    : %d\n", "Maximum Sessions", maxSessions)
	fmt.Printf("%-30s    : %t\n", "Flow Learn Enabled", flowLearn)
	fmt.Printf("%-30s    : %t\n", "Meter Enabled", meterEn)
	fmt.Printf("%-30s    : %s\n", "IPv4 Meter ID:", utils.IdToStr(v4Meter))
	fmt.Printf("%-30s    : %s\n", "IPv6 Meter ID:", utils.IdToStr(v6Meter))
	fmt.Printf("%-30s    : %s\n", "Rx Policer ID:", utils.IdToStr(rxPolicer))
	fmt.Printf("%-30s    : %s\n", "Tx Policer ID:", utils.IdToStr(txPolicer))
	if len(ingressV4Policy) != 0 {
		keyStr := fmt.Sprintf("%-30s    : ", "Ingress IPv4 Security Group ID")
		for i := 0; i < len(ingressV4Policy); i++ {
			fmt.Printf("%-36s%s\n", keyStr, utils.IdToStr(ingressV4Policy[i]))
			keyStr = ""
		}
	} else {
		fmt.Printf("%-30s    : %s\n", "Ingress IPv4 Security Group ID", "-")
	}
	if len(ingressV6Policy) != 0 {
		keyStr := fmt.Sprintf("%-30s    : ", "Ingress IPv6 Security Group ID")
		for i := 0; i < len(ingressV6Policy); i++ {
			fmt.Printf("%-36s%s\n", keyStr, utils.IdToStr(ingressV6Policy[i]))
			keyStr = ""
		}
	} else {
		fmt.Printf("%-30s    : %s\n", "Ingress IPv6 Security Group ID", "-")
	}
	if len(egressV4Policy) != 0 {
		keyStr := fmt.Sprintf("%-30s    : ", "Egress IPv4 Security Group ID")
		for i := 0; i < len(egressV4Policy); i++ {
			fmt.Printf("%-36s%s\n", keyStr, utils.IdToStr(egressV4Policy[i]))
			keyStr = ""
		}
	} else {
		fmt.Printf("%-30s    : %s\n", "Egress IPv4 Security Group ID", "-")
	}
	if len(egressV6Policy) != 0 {
		keyStr := fmt.Sprintf("%-30s    : ", "Egress IPv6 Security Group ID")
		for i := 0; i < len(egressV6Policy); i++ {
			fmt.Printf("%-36s%s\n", keyStr, utils.IdToStr(egressV6Policy[i]))
			keyStr = ""
		}
	} else {
		fmt.Printf("%-30s    : %s\n", "Egress IPv6 Security Group ID", "-")
	}

	lineStr := strings.Repeat("-", 60)
	fmt.Println(lineStr)
}

func vnicShowStatisticsCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := pds.NewVnicSvcClient(c)

	var req *pds.VnicGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// Get specific Vnic
		req = &pds.VnicGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(vnicID).Bytes()},
		}
	} else {
		// Get all Vnics
		req = &pds.VnicGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.VnicGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Vnic failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print Vnics
	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else {
		for _, resp := range respMsg.Response {
			spec := resp.GetSpec()
			stats := resp.GetStats()
			fmt.Printf("Vnic ID: %s\n", utils.IdToStr(spec.GetId()))
			fmt.Printf("\tTxBytes           : %d\n", stats.GetTxBytes())
			fmt.Printf("\tTxPackets         : %d\n", stats.GetTxPackets())
			fmt.Printf("\tRxBytes           : %d\n", stats.GetRxBytes())
			fmt.Printf("\tRxPackets         : %d\n", stats.GetRxPackets())
			fmt.Printf("\tMeterTxBytes      : %d\n", stats.GetMeterTxBytes())
			fmt.Printf("\tMeterTxPackets    : %d\n", stats.GetMeterTxPackets())
			fmt.Printf("\tMeterRxBytes      : %d\n", stats.GetMeterRxBytes())
			fmt.Printf("\tMeterRxPackets    : %d\n", stats.GetMeterRxPackets())
			fmt.Printf("\tActiveSessions    : %d\n", stats.GetActiveSessions())
		}
	}
}
