//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

// +build apulu apollo

package cmd

import (
	"context"
	"fmt"
	uuid "github.com/satori/go.uuid"
	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"
	"reflect"
	"strings"
	"google.golang.org/grpc"

	"github.com/pensando/sw/nic/apollo/agent/cli/utils"
	"github.com/pensando/sw/nic/apollo/agent/gen/pds"
)

var (
	vpcId     string
	subnetId  string
	epIP      string
	epMAC     string
	learnMode string
)

// Show commands
var learnShowCmd = &cobra.Command{
	Use:   "learn",
	Short: "show learnt endpoint information",
	Long:  "show learnt endpoint information",
}

var learnStatsShowCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show endpoint learning statistics",
	Long:  "show endpoint learning statistics",
	Run:   learnStatsShowCmdHandler,
}

var learnMACShowCmd = &cobra.Command{
	Use:   "mac",
	Short: "show learnt endpoint MAC information",
	Long:  "show learnt endpoint MAC information",
	Run:   learnMACShowCmdHandler,
}

var learnIPShowCmd = &cobra.Command{
	Use:   "ip",
	Short: "show learnt endpoint IP information",
	Long:  "show learnt endpoint IP information",
	Run:   learnIPShowCmdHandler,
}

// Clear commands
var learnClearCmd = &cobra.Command{
	Use:   "learn",
	Short: "clear learnt endpoint information",
	Long:  "clear learnt endpoint information",
}

var learnMACClearCmd = &cobra.Command{
	Use:   "mac",
	Short: "clear learnt endpoint MAC information in auto learn mode",
	Long:  "clear learnt endpoint MAC information in auto learn mode",
	Run:   learnMACClearCmdHandler,
}

var learnIPClearCmd = &cobra.Command{
	Use:   "ip",
	Short: "clear learnt endpoint IP information in auto learn mode",
	Long:  "clear learnt endpoint IP information in auto learn mode",
	Run:   learnIPClearCmdHandler,
}

var learnStatsClearCmd = &cobra.Command{
	Use:   "statistics",
	Short: "clear endpoint learning statistics",
	Long:  "clear endpoint learning statistics",
	Run:   learnStatsClearCmdHandler,
}

func init() {
	showCmd.AddCommand(learnShowCmd)

	// show learn mac
	learnShowCmd.AddCommand(learnMACShowCmd)
	learnMACShowCmd.Flags().StringVar(&learnMode, "mode", "", "Specify learn mode (auto, notify)")
	learnMACShowCmd.Flags().StringVar(&subnetId, "subnet", "0", "Specify Subnet ID")
	learnMACShowCmd.Flags().StringVar(&epMAC, "mac", "0", "Specify MAC address")
	learnMACShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	learnMACShowCmd.Flags().Bool("summary", false, "Display number of objects")
	learnMACShowCmd.MarkFlagRequired("mode")

	// show learn ip
	learnShowCmd.AddCommand(learnIPShowCmd)
	learnIPShowCmd.Flags().StringVar(&learnMode, "mode", "", "Specify learn mode (auto, notify)")
	learnIPShowCmd.Flags().StringVar(&vpcId, "vpc", "0", "Specify VPC ID")
	learnIPShowCmd.Flags().StringVar(&epIP, "ip", "0", "Specify IP address")
	learnIPShowCmd.Flags().StringVar(&subnetId, "subnet", "0", "Specify Subnet ID")
	learnIPShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	learnIPShowCmd.Flags().Bool("summary", false, "Display number of objects")
	learnIPShowCmd.MarkFlagRequired("mode")

	// show learn statistics
	learnShowCmd.AddCommand(learnStatsShowCmd)
	learnStatsShowCmd.Flags().Bool("yaml", false, "Output in yaml")

	// clear learn mac
	clearCmd.AddCommand(learnClearCmd)
	learnClearCmd.AddCommand(learnMACClearCmd)
	learnMACClearCmd.Flags().StringVar(&subnetId, "subnet", "0", "Specify Subnet ID")
	learnMACClearCmd.Flags().StringVar(&epMAC, "mac", "0", "Specify MAC address")

	// clear learn ip
	learnClearCmd.AddCommand(learnIPClearCmd)
	learnIPClearCmd.Flags().StringVar(&vpcId, "vpc", "0", "Specify VPC ID")
	learnIPClearCmd.Flags().StringVar(&epIP, "ip", "0", "Specify IP address")

	// clear learn statistics
	learnClearCmd.AddCommand(learnStatsClearCmd)
}

func getDeviceLearnMode(conn *grpc.ClientConn) pds.LearnMode {
	var err error

	if conn == nil {
		// Connect to PDS
		conn, err = utils.CreateNewGRPCClient()
		if err != nil {
			fmt.Printf("Could not connect to the PDS, is PDS running?\n")
			return pds.LearnMode_LEARN_MODE_NONE
		}
		defer conn.Close()
	}

	client := pds.NewDeviceSvcClient(conn)
	req := &pds.DeviceGetRequest{}
	// PDS call to get device info
	resp, err := client.DeviceGet(context.Background(), req)
	if (err != nil) || (resp.ApiStatus != pds.ApiStatus_API_STATUS_OK) {
		fmt.Printf("Getting Device information failed")
		return pds.LearnMode_LEARN_MODE_NONE
	}

	spec := resp.GetResponse().GetSpec()
	if spec == nil {
		fmt.Printf("Getting Device information failed")
		return pds.LearnMode_LEARN_MODE_NONE
	}

	return spec.GetLearnSpec().GetLearnMode()
}

func learnModeToStr(mode pds.LearnMode) string {
	switch(mode) {
	case pds.LearnMode_LEARN_MODE_NOTIFY:
		return "notify"
	case pds.LearnMode_LEARN_MODE_AUTO:
		return "auto"
	default:
		return "none"
	}
}

// Validate if command is supported in current device learn mode
func validateLearnCmdMode(conn *grpc.ClientConn, cmdMode string) bool {
	deviceMode := getDeviceLearnMode(conn)
	// Compare device mode and command mode
	deviceModeStr := learnModeToStr(deviceMode)
	if deviceModeStr != cmdMode {
		fmt.Printf("Command unsupported in device mode %s\n", deviceModeStr)
		return false
	}
	return true
}

func ValidLearnMode(mode string) bool {
	validModes := []string{"auto", "notify"}

	for _, m := range validModes {
		if mode == m {
			return true
		}
	}
	return false
}

func buildLearnMACGetRequest(cmd *cobra.Command) (*pds.LearnMACRequest, bool) {
	var detail bool
	var req *pds.LearnMACRequest

	detail = false
	req = nil
	switch(learnMode) {
	case "auto":
		if cmd != nil && cmd.Flags().Changed("mac") && cmd.Flags().Changed("subnet") {
			// Get specific entry
			req = &pds.LearnMACRequest{
				Key: []*pds.LearnMACKey{
					&pds.LearnMACKey {
						Mackey: &pds.LearnMACKey_KeyAuto {
							KeyAuto: &pds.LearnMACKeyAuto {
								SubnetId: uuid.FromStringOrNil(subnetId).Bytes(),
								MACAddr:  utils.MACAddrStrToUint64(epMAC),
							},
						},
					},
				},
			}
			detail = true
		} else {
			// Get all entries
			req = &pds.LearnMACRequest{
				Key: []*pds.LearnMACKey{},
			}
			detail = false
		}
	case "notify":
		// Get all entries
		req = &pds.LearnMACRequest{
			Key: []*pds.LearnMACKey{},
		}
	default:
	}
	return req, detail
}

func learnMACShowCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	if cmd != nil && cmd.Flags().Changed("mode") && !ValidLearnMode(learnMode) {
		fmt.Printf("Invalid mode specified. Mode should be one of (auto, notify)\n")
		return
	}

	if cmd != nil && learnMode == "notify" && (cmd.Flags().Changed("subnet") || cmd.Flags().Changed("mac")) {
		fmt.Printf("Invalid filters used. Subnet and MAC address filters can be used only in Auto mode\n")
		return
	}

	if cmd != nil && (cmd.Flags().Changed("subnet") != cmd.Flags().Changed("mac")) {
		fmt.Printf("Cannot specify only one of Subnet ID and endpoint MAC address\n")
		return
	}

	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	if cmd != nil {
		// Skip validation for techsupport
		if validateLearnCmdMode(c, learnMode) != true {
			return
		}
	}

	client := pds.NewLearnSvcClient(c)
	req, detail := buildLearnMACGetRequest(cmd)

	// PDS call
	resp, err := client.LearnMACGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting MAC information failed, err %v\n", err)
		return
	}

	if resp.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		return
	}

	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, respData := range resp.Response {
			respType := reflect.ValueOf(respData)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		printLearnMACSummary(len(resp.Response))
	} else {
		printLearnMAC(resp, detail, learnMode)
		printLearnMACSummary(len(resp.Response))
	}
}

func buildLearnIPGetRequest(cmd *cobra.Command) (*pds.LearnIPGetRequest) {
	var req *pds.LearnIPGetRequest

	req = nil
	switch(learnMode) {
	case "auto":
		if cmd != nil && cmd.Flags().Changed("ip") && cmd.Flags().Changed("vpc") {
			// Get entry by VPC/IP key
			req = &pds.LearnIPGetRequest{
				Filter: &pds.LearnIPGetRequest_Key{
					Key: &pds.LearnIPKey{
						Ipkey: &pds.LearnIPKey_KeyAuto{
							KeyAuto: &pds.LearnIPKeyAuto{
								VPCId:  uuid.FromStringOrNil(vpcId).Bytes(),
								IPAddr: utils.IPAddrStrToPDSIPAddr(epIP),
							},
						},
					},
				},
			}
		} else if cmd != nil && cmd.Flags().Changed("subnet") {
			// Get entries belonging to subnet
			req = &pds.LearnIPGetRequest{
				Filter: &pds.LearnIPGetRequest_SubnetId{
					SubnetId: uuid.FromStringOrNil(subnetId).Bytes(),
				},
			}
		} else {
			// Get all entries
			req = &pds.LearnIPGetRequest{}
		}
	case "notify":
		// Get all entries
		req = &pds.LearnIPGetRequest{}
	default:
	}
	return req
}

func learnIPShowCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	if cmd != nil && cmd.Flags().Changed("mode") && !ValidLearnMode(learnMode) {
		fmt.Printf("Invalid mode specified. Mode should be one of (auto, notify)\n")
		return
	}

	if cmd != nil && learnMode == "notify" && (cmd.Flags().Changed("vpc") ||
		cmd.Flags().Changed("ip") || cmd.Flags().Changed("subnet")) {
		fmt.Printf("Invalid filters used. VPC, IP Address, Subnet filters can be used only in Auto mode\n")
		return
	}

	if cmd != nil && (cmd.Flags().Changed("vpc") != cmd.Flags().Changed("ip")) {
		fmt.Printf("Cannot specify only one of VPC ID and endpoint IP address\n")
		return
	}

	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	if cmd != nil {
		// Skip validation for techsupport
		if validateLearnCmdMode(c, learnMode) != true {
			return
		}
	}

	client := pds.NewLearnSvcClient(c)
	req := buildLearnIPGetRequest(cmd)

	// PDS call to get learn info
	resp, err := client.LearnIPGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting IP information failed, err %v\n", err)
		return
	}

	if resp.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		return
	}

	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, respData := range resp.Response {
			respType := reflect.ValueOf(respData)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		printLearnIPSummary(len(resp.Response))
	} else {
		printLearnIP(resp, learnMode)
		printLearnIPSummary(len(resp.Response))
	}
}

func learnStatsShowCmdHandler(cmd *cobra.Command, args []string) {
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

	var req *pds.Empty
	client := pds.NewLearnSvcClient(c)

	// PDS call
	resp, err := client.LearnStatsGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting statistics failed, err %v\n", err)
		return
	}

	if resp.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		return
	}

	if cmd != nil && cmd.Flags().Changed("yaml") {
		respType := reflect.ValueOf(resp.Stats)
		b, _ := yaml.Marshal(respType.Interface())
		fmt.Println(string(b))
		fmt.Println("---")
	} else {
		printLearnStats(resp)
	}
}

func learnMACClearCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	// Command is valid only in auto mode
	if validateLearnCmdMode(c, "auto") != true {
		return
	}

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	if cmd != nil && (cmd.Flags().Changed("subnet") != cmd.Flags().Changed("mac")) {
		fmt.Printf("Cannot specify only one of Subnet ID and endpoint MAC address\n")
		return
	}

	var req *pds.LearnMACRequest
	if cmd != nil && cmd.Flags().Changed("mac") && cmd.Flags().Changed("subnet") {
		// Get specific entry
		req = &pds.LearnMACRequest{
			Key: []*pds.LearnMACKey{
				&pds.LearnMACKey{
					Mackey: &pds.LearnMACKey_KeyAuto {
						KeyAuto: &pds.LearnMACKeyAuto {
							SubnetId: uuid.FromStringOrNil(subnetId).Bytes(),
							MACAddr:  utils.MACAddrStrToUint64(epMAC),
						},
					},
				},
			},
		}
	} else {
		// Clear all entries
		req = &pds.LearnMACRequest{
			Key: []*pds.LearnMACKey{},
		}
	}

	client := pds.NewLearnSvcClient(c)

	// PDS call
	resp, err := client.LearnMACClear(context.Background(), req)
	if err != nil {
		fmt.Printf("Clear learnt MAC(s) failed, err %v\n", err)
		return
	}

	if resp.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		return
	}
	fmt.Printf("Clearing MAC(s) succeeded\n")
}

func learnIPClearCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}
	defer c.Close()

	// Command is valid only in auto mode
	if validateLearnCmdMode(c, "auto") != true {
		return
	}

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	if cmd != nil && (cmd.Flags().Changed("vpc") != cmd.Flags().Changed("ip")) {
		fmt.Printf("Cannot specify only one of VPC ID and endpoint IP address\n")
		return
	}

	var req *pds.LearnIPRequest
	if cmd != nil && cmd.Flags().Changed("ip") && cmd.Flags().Changed("vpc") {
		// Get specific entry
		req = &pds.LearnIPRequest{
			Key: []*pds.LearnIPKey{
				&pds.LearnIPKey{
					Ipkey: &pds.LearnIPKey_KeyAuto{
						KeyAuto: &pds.LearnIPKeyAuto{
							VPCId:  uuid.FromStringOrNil(vpcId).Bytes(),
							IPAddr: utils.IPAddrStrToPDSIPAddr(epIP),
						},
					},
				},
			},
		}
	} else {
		// Clear all entries
		req = &pds.LearnIPRequest{
			Key: []*pds.LearnIPKey{},
		}
	}

	client := pds.NewLearnSvcClient(c)

	// PDS call
	resp, err := client.LearnIPClear(context.Background(), req)
	if err != nil {
		fmt.Printf("Clearing learnt IP(s) failed, err %v\n", err)
		return
	}

	if resp.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		return
	}
	fmt.Printf("Clearing IP(s) succeeded\n")
}

func learnStatsClearCmdHandler(cmd *cobra.Command, args []string) {
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

	var empty *pds.Empty
	client := pds.NewLearnSvcClient(c)

	// PDS call
	_, err = client.LearnStatsClear(context.Background(), empty)
	if err != nil {
		fmt.Printf("Clearing statistics failed, err %v\n", err)
		return
	}
	fmt.Printf("Clearing statistics succeeded\n")
}

func LearnStateToStr(state pds.EpState) string {
	var stateStr string

	switch state {
	case pds.EpState_EP_STATE_LEARNING:
		stateStr = "Learning"
	case pds.EpState_EP_STATE_CREATED:
		stateStr = "Created"
	case pds.EpState_EP_STATE_PROBING:
		stateStr = "Probing"
	case pds.EpState_EP_STATE_UPDATING:
		stateStr = "Updating"
	case pds.EpState_EP_STATE_DELETING:
		stateStr = "Deleting"
	case pds.EpState_EP_STATE_DELETED:
		stateStr = "Deleted"
	default:
	}
	return stateStr
}

func LearnPktTypeToStr(pktType pds.LearnPktType) string {
	var pktTypeStr string

	switch pktType {
	case pds.LearnPktType_LEARN_PKT_TYPE_GARP_ANNOUNCE:
		pktTypeStr = "GARP announces"
	case pds.LearnPktType_LEARN_PKT_TYPE_ARP_PROBE:
		pktTypeStr = "ARP probes"
	case pds.LearnPktType_LEARN_PKT_TYPE_ARP_REQUEST:
		pktTypeStr = "ARP requests"
	case pds.LearnPktType_LEARN_PKT_TYPE_GARP_REPLY:
		pktTypeStr = "GARP replies"
	case pds.LearnPktType_LEARN_PKT_TYPE_ARP_REPLY:
		pktTypeStr = "ARP replies"
	case pds.LearnPktType_LEARN_PKT_TYPE_RARP_REQUEST:
		pktTypeStr = "RARP requests"
	case pds.LearnPktType_LEARN_PKT_TYPE_RARP_REPLY:
		pktTypeStr = "RARP replies"
	case pds.LearnPktType_LEARN_PKT_TYPE_DHCP_DISCOVER:
		pktTypeStr = "DHCP discovers"
	case pds.LearnPktType_LEARN_PKT_TYPE_DHCP_REQUEST:
		pktTypeStr = "DHCP requests"
	case pds.LearnPktType_LEARN_PKT_TYPE_DHCP_ACK:
		pktTypeStr = "DHCP acks"
	case pds.LearnPktType_LEARN_PKT_TYPE_IPV4:
		pktTypeStr = "IPV4 packets"
	default:
		pktTypeStr = "Unknown"
	}
	return pktTypeStr
}

func LearnPktDropReasonToStr(reason pds.LearnPktDropReason) string {
	var reasonStr string

	switch reason {
	case pds.LearnPktDropReason_LEARN_PKTDROP_REASON_PARSE_ERR:
		reasonStr = "Parse errors"
	case pds.LearnPktDropReason_LEARN_PKTDROP_REASON_RES_ALLOC_FAIL:
		reasonStr = "Resource allocation failures"
	case pds.LearnPktDropReason_LEARN_PKTDROP_REASON_LEARNING_FAIL:
		reasonStr = "Learning failures"
	case pds.LearnPktDropReason_LEARN_PKTDROP_REASON_MBUF_ERR:
		reasonStr = "Internal packet buffer errors"
	case pds.LearnPktDropReason_LEARN_PKTDROP_REASON_TX_FAIL:
		reasonStr = "Tx failures"
	case pds.LearnPktDropReason_LEARN_PKTDROP_REASON_ARP_REPLY:
		reasonStr = "ARP replies"
	case pds.LearnPktDropReason_LEARN_PKTDROP_REASON_RARP:
		reasonStr = "RARP"
	default:
		reasonStr = "Unknown"
	}
	return reasonStr
}

func LearnValidationTypeToStr(valType pds.LearnValidationType) string {
	var valStr string

	switch valType {
	case pds.LearnValidationType_LEARN_CHECK_MAC_LIMIT:
		valStr = "Exceeded MAC limit"
	case pds.LearnValidationType_LEARN_CHECK_IP_LIMIT:
		valStr = "Exceeded IP limit"
	case pds.LearnValidationType_LEARN_CHECK_IP_IN_SUBNET:
		valStr = "IP address does not belong to subnet"
	default:
		valStr = "Unknown"
	}
	return valStr
}

func LearnEventTypeToStr(eventType pds.LearnEventType) string {
	var eventStr string

	switch eventType {
	case pds.LearnEventType_LEARN_EVENT_NEW_LOCAL:
		eventStr = "New local learn"
	case pds.LearnEventType_LEARN_EVENT_NEW_REMOTE:
		eventStr = "New remote learn"
	case pds.LearnEventType_LEARN_EVENT_L2L_MOVE:
		eventStr = "Local to local move"
	case pds.LearnEventType_LEARN_EVENT_R2L_MOVE:
		eventStr = "Remote to local move"
	case pds.LearnEventType_LEARN_EVENT_L2R_MOVE:
		eventStr = "Local to remote move"
	case pds.LearnEventType_LEARN_EVENT_R2R_MOVE:
		eventStr = "Remote to remote move"
	case pds.LearnEventType_LEARN_EVENT_DELETE:
		eventStr = "Remote delete"
	default:
		eventStr = "Unknown"
	}
	return eventStr
}

func LearnApiOpTypeToStr(opType pds.LearnApiOpType) string {
	var opStr string

	switch opType {
	case pds.LearnApiOpType_LEARN_API_OP_CREATE:
		opStr = "Create"
	case pds.LearnApiOpType_LEARN_API_OP_DELETE:
		opStr = "Delete"
	case pds.LearnApiOpType_LEARN_API_OP_UPDATE:
		opStr = "Update"
	default:
		opStr = "Unknown"
	}
	return opStr
}

func printLearnMACSummary(count int) {
	fmt.Printf("\nNo. of learnt endpoint MAC information : %d\n\n", count)
}

func printLearnNotifyMACHeader() {
	hdrLine := strings.Repeat("-", 94)
	fmt.Println(hdrLine)
	fmt.Printf("%-20s%-40s%-14s%-10s%-8s\n",
		"MACAddr", "HostIf", "Encap", "IP count", "TTL(s)")
	fmt.Println(hdrLine)
}

func printLearnAutoMACHeader() {
	hdrLine := strings.Repeat("-", 130)
	fmt.Println(hdrLine)
	fmt.Printf("%-20s%-40s%-40s%-10s%-10s%-8s\n",
		"MACAddr", "SubnetId", "VnicId", "State", "IP count",
		"TTL(s)")
	fmt.Println(hdrLine)
}

func printLearnAutoMAC(resp *pds.LearnMACGetResponse, detail bool) {
	if len(resp.GetResponse()) > 0 {
		printLearnAutoMACHeader()
	}
	for _, MACEntry := range resp.GetResponse() {
		fmt.Printf("%-20s%-40s%-40s%-10s%-10d%-8d\n",
			utils.MactoStr(MACEntry.GetEntryAuto().GetKey().GetMACAddr()),
			utils.IdToStr(MACEntry.GetEntryAuto().GetKey().GetSubnetId()),
			utils.IdToStr(MACEntry.GetEntryAuto().GetVnicId()),
			LearnStateToStr(MACEntry.GetEntryAuto().GetState()),
			len(MACEntry.GetEntryAuto().GetIPInfo()),
			MACEntry.GetEntryAuto().GetTTL())
		if detail == true {
			if len(MACEntry.GetEntryAuto().GetIPInfo()) > 0 {
				fmt.Printf("	IPs learnt:\n")
			}
			for _, IPKey := range MACEntry.GetEntryAuto().GetIPInfo() {
				fmt.Printf("		%s\n",
					utils.IPAddrToStr(IPKey.GetIPAddr()))
			}
		}
	}
}

func printLearnNotifyMAC(resp *pds.LearnMACGetResponse) {
	if len(resp.GetResponse()) > 0 {
		printLearnNotifyMACHeader()
	}
	for _, MACEntry := range resp.GetResponse() {
		fmt.Printf("%-20s%-40s%-14s%-10d%-8d\n",
			utils.MactoStr(MACEntry.GetEntryNotify().GetKey().GetMACAddr()),
			utils.IdToStr(MACEntry.GetEntryNotify().GetKey().GetHostIf()),
			utils.EncapToString(MACEntry.GetEntryNotify().GetEncap()),
			len(MACEntry.GetEntryNotify().GetIPInfo()),
			MACEntry.GetEntryNotify().GetTTL())
	}
}

func printLearnMAC(resp *pds.LearnMACGetResponse,
		detail bool, mode string) {
	if mode == "auto" {
		printLearnAutoMAC(resp, detail)
	} else if mode == "notify" {
		printLearnNotifyMAC(resp)
	}
}

func printLearnIPSummary(count int) {
	fmt.Printf("\nNo. of learnt endpoint IP information : %d\n\n", count)
}

func printLearnAutoIPHeader() {
	hdrLine := strings.Repeat("-", 140)
	fmt.Println(hdrLine)
	fmt.Printf("%-20s%-40s%-20s%-40s%-10s%-8s\n",
		"IPAddr", "VpcID", "MACAddr", "SubnetId", "State", "TTL(s)")
	fmt.Println(hdrLine)
}

func printLearnNotifyIPHeader() {
	hdrLine := strings.Repeat("-", 90)
	fmt.Println(hdrLine)
	fmt.Printf("%-20s%-40s%-20s%-8s\n",
		"IPAddr", "HostIf", "MACAddr", "TTL(s)")
	fmt.Println(hdrLine)
}

func printLearnAutoIP(resp *pds.LearnIPGetResponse) {
	if len(resp.GetResponse()) > 0 {
		printLearnAutoIPHeader()
	}
	for _, IPEntry := range resp.GetResponse() {
		fmt.Printf("%-20s%-40s%-20s%-40s%-10s%-8d\n",
			utils.IPAddrToStr(IPEntry.GetEntryAuto().GetKey().GetIPAddr()),
			utils.IdToStr(IPEntry.GetEntryAuto().GetKey().GetVPCId()),
			utils.MactoStr(IPEntry.GetEntryAuto().GetMACInfo().GetMACAddr()),
			utils.IdToStr(IPEntry.GetEntryAuto().GetMACInfo().GetSubnetId()),
			LearnStateToStr(IPEntry.GetEntryAuto().GetState()),
			IPEntry.GetEntryAuto().GetTTL())
	}
}

func printLearnNotifyIP(resp *pds.LearnIPGetResponse) {
	if len(resp.GetResponse()) > 0 {
		printLearnNotifyIPHeader()
	}
	for _, IPEntry := range resp.GetResponse() {
		fmt.Printf("%-20s%-40s%-20s%-8d\n",
			utils.IPAddrToStr(IPEntry.GetEntryNotify().GetKey().GetIPAddr()),
			utils.IdToStr(IPEntry.GetEntryNotify().GetKey().GetHostIf()),
			utils.MactoStr(IPEntry.GetEntryNotify().GetMACAddr()),
			IPEntry.GetEntryNotify().GetTTL())
	}
}

func printLearnIP(resp *pds.LearnIPGetResponse, mode string) {
	if mode == "auto" {
		printLearnAutoIP(resp)
	} else if mode == "notify" {
		printLearnNotifyIP(resp)
	}
}

func printLearnEventStats(levents []*pds.LearnEvents, header string, err bool) {
	var errStr string

	if len(levents) > 0 {
		fmt.Printf("%s:\n", header)
	}
	if err {
		errStr = " errors"
	} else {
		errStr = ""
	}
	for _, levent := range levents {
		if levent.GetEventType() == pds.LearnEventType_LEARN_EVENT_NONE {
			continue
		}
		fmt.Printf("	# %-30s: %-20d\n",
			LearnEventTypeToStr(levent.GetEventType())+errStr,
			levent.GetCount())
	}
}

func printLearnApiStats(ops []*pds.LearnApiOps, header string, err bool) {
	var errStr string

	if len(ops) > 0 {
		fmt.Printf("%s:\n", header)
	}
	if err {
		errStr = " errors"
	} else {
		errStr = ""
	}
	for _, op := range ops {
		fmt.Printf("	# %-30s: %-20d\n",
			LearnApiOpTypeToStr(op.GetApiOpType())+errStr,
			op.GetCount())
	}
}
func printLearnStats(resp *pds.LearnStatsGetResponse) {
	stats := resp.GetStats()

	fmt.Printf("Packet counters:\n")
	fmt.Printf("	# Packets received              : %-20d\n", stats.GetPktsRcvd())
	fmt.Printf("	# Packets sent                  : %-20d\n", stats.GetPktsSent())
	fmt.Printf("	# Packet send errors            : %-20d\n", stats.GetPktSendErrors())
	fmt.Printf("	# Arp probes sent               : %-20d\n", stats.GetArpProbesSent())
	fmt.Printf("	# Arp probe send errors         : %-20d\n", stats.GetArpProbeSendErrors())
	fmt.Printf("	# Buffers allocated             : %-20d\n", stats.GetPktBufferAlloc())
	fmt.Printf("	# Buffer allocation errors      : %-20d\n", stats.GetPktBufferAllocErrors())
	fmt.Printf("	# Buffers available             : %-20d\n", stats.GetPktBufferAvailable())

	if len(stats.GetRcvdPktTypes()) > 0 {
		fmt.Printf("	# Received pkt type counters:\n")
	}
	for _, rcvdPktType := range stats.GetRcvdPktTypes() {
		fmt.Printf("	      %-26s: %-20d\n",
			LearnPktTypeToStr(rcvdPktType.GetPktType()),
			rcvdPktType.GetCount())
	}

	if len(stats.GetDropStats()) > 0 {
		fmt.Printf("	# Drop counters:\n")
	}
	for _, dropStats := range stats.GetDropStats() {
		fmt.Printf("	      %-26s: %-20d\n",
			LearnPktDropReasonToStr(dropStats.GetReason()),
			dropStats.GetNumDrops())
	}

	fmt.Printf("Aging counters:\n")
	fmt.Printf("	# IP mapping ageouts            : %-20d\n", stats.GetIpAgeouts())
	fmt.Printf("	# MAC ageouts                   : %-20d\n", stats.GetMacAgeouts())
	fmt.Printf("	# IP mapping ageout errors      : %-20d\n", stats.GetIpAgeoutErrors())
	fmt.Printf("	# MAC ageout errors             : %-20d\n", stats.GetMacAgeoutErrors())

	printLearnEventStats(stats.GetMacLearnEvents(), "MAC learn and move counters", false)
	printLearnEventStats(stats.GetMacLearnErrors(), "MAC learn and move errors", true)
	printLearnEventStats(stats.GetIpLearnEvents(), "IP learn and move counters", false)
	printLearnEventStats(stats.GetIpLearnErrors(), "IP learn and move errors", true)

	if len(stats.GetValidationErrors()) > 0 {
		fmt.Printf("Learn validation error counters:\n")
	}
	for _, valErr := range stats.GetValidationErrors() {
		fmt.Printf("	# %-26s: %-20d\n",
			LearnValidationTypeToStr(valErr.GetValidationType()),
			valErr.GetCount())
	}

	printLearnApiStats(stats.GetVnicOps(), "VNIC API", false)
	printLearnApiStats(stats.GetVnicOpErrors(), "VNIC API errors", true)
	printLearnApiStats(stats.GetRemoteL2Mappings(), "Remote MAC mapping API", false)
	printLearnApiStats(stats.GetRemoteL2MappingErrors(), "Remote MAC mapping API errors", true)
	printLearnApiStats(stats.GetLocalL3Mappings(), "Local IP mapping API", false)
	printLearnApiStats(stats.GetLocalL3MappingErrors(), "Local IP mapping API errors", true)
	printLearnApiStats(stats.GetRemoteL3Mappings(), "Remote IP mapping API", false)
	printLearnApiStats(stats.GetRemoteL3MappingErrors(), "Remote IP mapping API errors", true)
}

func learnTechsupportHandler() {
	learnMode = learnModeToStr(getDeviceLearnMode(nil))
	switch(learnMode) {
	case "none":
		break
	default:
		learnStatsShowCmdHandler(nil, nil)
		learnMACShowCmdHandler(nil, nil)
		learnIPShowCmdHandler(nil, nil)
	}
}
