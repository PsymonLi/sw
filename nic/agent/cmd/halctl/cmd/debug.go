//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	"os"
	"strings"

	"github.com/spf13/cobra"

	"github.com/pensando/sw/nic/agent/cmd/halctl/utils"
	"github.com/pensando/sw/nic/agent/netagent/datapath/halproto"
)

var (
	traceLevel          string
	secProfID           uint32
	connTrack           string
	ipNormalization     string
	tcpNormalization    string
	tcpTimeout          uint32
	udpTimeout          uint32
	icmpTimeout         uint32
	idleTimeout         uint32
	tcpCxnsetupTimeout  uint32
	tcpHalfcloseTimeout uint32
	regID               uint32
	regAddr             uint64
	regData             uint32
	regInstance         uint32
	platPbPause         string
	spanThreshold       uint32
	xcvrValid           string
	tcpProxyMss         uint32
	tcpProxyCwndInitial uint32
	tcpProxyCwndIdle    uint32
	maxSession          uint64
)

var debugCmd = &cobra.Command{
	Use:   "debug",
	Short: "set debug options",
	Long:  "set debug options",
}

var debugCreateCmd = &cobra.Command{
	Use:   "create",
	Short: "create object",
	Long:  "create object",
}

var debugDeleteCmd = &cobra.Command{
	Use:   "delete",
	Short: "delete object",
	Long:  "delete object",
}

var debugUpdateCmd = &cobra.Command{
	Use:   "update",
	Short: "update object",
	Long:  "update object",
}

var memoryDebugCmd = &cobra.Command{
	Use:   "memory",
	Short: "debug memory",
	Long:  "debug memory",
	Run:   memoryDebugCmdHandler,
}

var regDebugCmd = &cobra.Command{
	Use:   "register",
	Short: "set register data",
	Long:  "set register data",
	Run:   regDebugCmdHandler,
}

var regShowCmd = &cobra.Command{
	Use:   "register",
	Short: "show register data",
	Long:  "show register data",
	Run:   regShowCmdHandler,
}

var traceDebugCmd = &cobra.Command{
	Use:   "trace",
	Short: "set debug trace level",
	Long:  "set debug trace level",
	Run:   traceDebugCmdHandler,
}

var traceShowCmd = &cobra.Command{
	Use:   "trace",
	Short: "show trace level",
	Long:  "show trace level",
	Run:   traceShowCmdHandler,
}

var flushLogsDebugCmd = &cobra.Command{
	Use:   "flush",
	Short: "flush logs",
	Long:  "flush logs",
	Run:   flushLogsDebugCmdHandler,
}

var fwDebugCmd = &cobra.Command{
	Use:   "firewall",
	Short: "set firewall options",
	Long:  "set firewall options",
}

var secProfDebugCmd = &cobra.Command{
	Use:   "security-profile",
	Short: "set firewall security profile options",
	Long:  "set firewall security profile options",
	Run:   fwSecProfDebugCmdHandler,
}

var tcpProxyDebugCmd = &cobra.Command{
	Use:   "tcp-proxy",
	Short: "set tcp-proxy options",
	Long:  "set tcp-proxy options",
	Run:   tcpProxyDebugCmdHandler,
}

var testDebugCmd = &cobra.Command{
	Use:    "test",
	Short:  "test options",
	Long:   "test options",
	Hidden: true,
}

var platDebugCmd = &cobra.Command{
	Use:   "platform",
	Short: "set platform options",
	Long:  "set platform options",
}

var pbPlatDebugCmd = &cobra.Command{
	Use:   "packet-buffer",
	Short: "set packet-buffer options",
	Long:  "set packet-buffer options",
	Run:   pbPlatDebugCmdHandler,
}

var testSendFinDebugCmd = &cobra.Command{
	Use:    "send-fin",
	Short:  "test sending FINs to local EPs",
	Long:   "test sending FINs to local EPs",
	Hidden: true,
	Run:    testSendFinDebugCmdHandler,
}

var platLlcDebugCmd = &cobra.Command{
	Use:   "llc-cache-setup",
	Short: "debug platform llc-cache-setup [cache-read|cache-write|scratchpad-access|cache-hit|cache-miss|partial-write|cache-maint-op|eviction|retry-needed|retry-access|disable]",
	Long:  "debug platform llc-cache-setup [cache-read|cache-write|scratchpad-access|cache-hit|cache-miss|partial-write|cache-maint-op|eviction|retry-needed|retry-access|disable]",
	Run:   llcSetupCmdHandler,
}

var platDatapathCacheDebugCmd = &cobra.Command{
	Use:   "datapath-cache",
	Short: "debug platform datapath-cache [p4-ingress|p4-egress|p4-all|p4plus-rxdma|p4plus-txdma|p4plus-all|all] [enable|disable]",
	Long:  "debug platform datapath-cache [p4-ingress|p4-egress|p4-all|p4plus-rxdma|p4plus-txdma|p4plus-all|all] [enable|disable]",
	Run:   datapathCacheDebugCmdHandler,
}

var xcvrDebugCmd = &cobra.Command{
	Use:   "transceiver",
	Short: "debug transceiver",
	Long:  "debug transceiver",
	Run:   xcvrDebugCmdHandler,
}

var sessionCtrlDebugCmd = &cobra.Command{
	Use:   "session-ctrl",
	Short: "debug session --max-session ",
	Long:  "debug session --max-session",
	Run:   sessionCtrlDebugCmdHandler,
}

func init() {
	rootCmd.AddCommand(debugCmd)
	debugCmd.AddCommand(traceDebugCmd)
	debugCmd.AddCommand(fwDebugCmd)
	debugCmd.AddCommand(tcpProxyDebugCmd)
	debugCmd.AddCommand(platDebugCmd)
	debugCmd.AddCommand(regDebugCmd)
	debugCmd.AddCommand(debugCreateCmd)
	debugCmd.AddCommand(debugUpdateCmd)
	debugCmd.AddCommand(debugDeleteCmd)
	debugCmd.AddCommand(testDebugCmd)
	debugCmd.AddCommand(memoryDebugCmd)
	debugCmd.AddCommand(sessionCtrlDebugCmd)
	traceDebugCmd.AddCommand(flushLogsDebugCmd)
	fwDebugCmd.AddCommand(secProfDebugCmd)
	showCmd.AddCommand(traceShowCmd)
	showCmd.AddCommand(regShowCmd)
	testDebugCmd.AddCommand(testSendFinDebugCmd)

	// debug platform llc-cache-setup
	platDebugCmd.AddCommand(platLlcDebugCmd)

	// debug platform packet-buffer
	platDebugCmd.AddCommand(pbPlatDebugCmd)

	// debug platform datapath-cache
	platDebugCmd.AddCommand(platDatapathCacheDebugCmd)

	traceDebugCmd.Flags().StringVar(&traceLevel, "level", "none", "Specify trace level")
	sessionCtrlDebugCmd.Flags().Uint64Var(&maxSession, "max-session", 0, "Max sessions to handle per fte 1 - 131072")
	secProfDebugCmd.Flags().Uint32Var(&secProfID, "id", 0, "Specify firewall security profile ID")
	secProfDebugCmd.Flags().StringVar(&connTrack, "conntrack", "off", "Turn connection tracking on/off")
	secProfDebugCmd.Flags().Uint32Var(&tcpTimeout, "tcp-timeout", 3600, "TCP session aging timeout in range 0-172800 (0 means no aging)")
	secProfDebugCmd.Flags().Uint32Var(&udpTimeout, "udp-timeout", 30, "UDP session aging timeout in range 0-172800 (0 means no aging)")
	secProfDebugCmd.Flags().Uint32Var(&icmpTimeout, "icmp-timeout", 6, "ICMP session aging timeout in range 0-172800 (0 means no aging)")
	secProfDebugCmd.Flags().Uint32Var(&idleTimeout, "idle-timeout", 90, "Session aging timeout for non TCP/UDP/ICMP in range 0-172800 (0 means no aging)")
	secProfDebugCmd.Flags().Uint32Var(&tcpCxnsetupTimeout, "tcp-cxnsetup-timeout", 30, "TCP Connection setup timeout for 3-way handshake in range 0-60 (0 means no timeout)")
	secProfDebugCmd.Flags().Uint32Var(&tcpHalfcloseTimeout, "tcp-halfclose-timeout", 120, "TCP Half close timeout when FIN is received on one direction in range 0-172800 (0 means no timeout)")
	secProfDebugCmd.Flags().StringVar(&ipNormalization, "ip-normalization", "off", "Turn IP Normalization on/off")
	secProfDebugCmd.Flags().StringVar(&tcpNormalization, "tcp-normalization", "off", "Turn TCP Normalization on/off")
	secProfDebugCmd.MarkFlagRequired("id")

	tcpProxyDebugCmd.Flags().Uint32Var(&tcpProxyMss, "mss", 9216, "TCP Maximum Segment Size in range 68-10000")
	tcpProxyDebugCmd.Flags().Uint32Var(&tcpProxyCwndInitial, "cwnd-initial", 10, "TCP initial window size in range 1-14")
	tcpProxyDebugCmd.Flags().Uint32Var(&tcpProxyCwndIdle, "cwnd-idle", 10, "TCP window size after idle timer is hit in range 1-14")

	regDebugCmd.Flags().Uint32Var(&regID, "reg-id", 0, "Specify register ID")
	regDebugCmd.Flags().Uint64Var(&regAddr, "reg-addr", 0, "Specify register address")
	regDebugCmd.Flags().Uint32Var(&regData, "data", 0, "Specify data to be written")
	regDebugCmd.Flags().Uint32Var(&regInstance, "instance", 0, "Specify register instance")
	regShowCmd.Flags().Uint32Var(&regID, "reg-id", 0, "Specify register ID")
	regShowCmd.Flags().Uint64Var(&regAddr, "reg-addr", 0, "Specify register address")
	regShowCmd.Flags().Uint32Var(&regInstance, "instance", 0, "Specify register instance")

	pbPlatDebugCmd.Flags().StringVar(&platPbPause, "pause", "", "Enable or Disable packet-buffer pause using enable | disable")
	pbPlatDebugCmd.Flags().Uint32Var(&spanThreshold, "span-threshold", 900, "Span queue threshold")

	memoryDebugCmd.Flags().Bool("memory-trim", false, "Reclaim free memory from malloc")
	memoryDebugCmd.MarkFlagRequired("memory-trim")

	// debug transceiver
	debugCmd.AddCommand(xcvrDebugCmd)

	// debug transceiver --valid-check <enable|disable>
	xcvrDebugCmd.Flags().StringVar(&xcvrValid, "valid-check", "enable", "Enable/Disable transceiver valid checks for links")
}

func memoryDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		return
	}
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	client := halproto.NewDebugClient(c)

	var empty *halproto.Empty

	// HAL call
	_, err = client.MemoryTrim(context.Background(), empty)
	if err != nil {
		fmt.Printf("Memory trim failed. %v\n", err)
		return
	}

	fmt.Printf("Memory trim succeeded\n")
}

func pbPlatDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewDebugClient(c)

	var pauseMsg *halproto.PacketBufferPause
	if cmd.Flags().Changed("pause") == true {
		pause := false
		if strings.Compare(platPbPause, "enable") == 0 {
			pause = true
		} else if strings.Compare(platPbPause, "disable") == 0 {
			pause = false
		} else {
			fmt.Printf("Invalid argument. Refer to help string\n")
			return
		}
		pauseMsg = &halproto.PacketBufferPause{
			Pause: pause,
		}
	}

	var spanMsg *halproto.PacketBufferSpan
	if cmd.Flags().Changed("span-threshold") == true {
		spanMsg = &halproto.PacketBufferSpan{
			SpanThreshold: spanThreshold,
		}
	}

	req := &halproto.PacketBufferRequest{
		Spec: &halproto.PacketBufferSpec{
			Pause: pauseMsg,
			Span:  spanMsg,
		},
	}

	reqMsg := &halproto.PacketBufferRequestMsg{
		Request: []*halproto.PacketBufferRequest{req},
	}

	// HAL call
	respMsg, err := client.PacketBufferUpdate(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Packet buffer update failed. %v\n", err)
		return
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		}
	}
}

func regShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewDebugClient(c)

	var req *halproto.RegisterRequest

	if cmd.Flags().Changed("reg-id") {
		fmt.Printf("Not yet supported\n")
		return
	} else if cmd.Flags().Changed("reg-addr") {
		if cmd.Flags().Changed("instance") == false {
			fmt.Printf("Instance needs to be specified with --instance flag\n")
			return
		}
		req = &halproto.RegisterRequest{
			IdNameOrAddr: &halproto.RegisterRequest_Addr{
				Addr: regAddr,
			},
			Instance: regInstance,
		}
	} else {
		fmt.Printf("Argument needs to be specified. Refer to help string\n")
		return
	}

	reqMsg := &halproto.RegisterRequestMsg{
		Request: []*halproto.RegisterRequest{req},
	}

	// HAL call
	respMsg, err := client.RegisterGet(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Register get failed. %v\n", err)
		return
	}

	regPrintHeader()

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		} else {
			fmt.Printf("%-12s%-12s\n",
				resp.GetData().GetAddress(), resp.GetData().GetValue())
		}
	}
}

func regPrintHeader() {
	hdrLine := strings.Repeat("-", 24)
	fmt.Println(hdrLine)
	fmt.Printf("%-12s%-12s\n",
		"RegAddr", "Value")
	fmt.Println(hdrLine)
}

func regDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewDebugClient(c)

	var req *halproto.RegisterRequest

	if cmd.Flags().Changed("data") == false {
		fmt.Printf("Data to be written needs to be specified using --data flag\n")
		return
	}

	if cmd.Flags().Changed("reg-id") {
		fmt.Printf("Not yet supported\n")
		return
	} else if cmd.Flags().Changed("reg-addr") {
		req = &halproto.RegisterRequest{
			IdNameOrAddr: &halproto.RegisterRequest_Addr{
				Addr: regAddr,
			},
			Instance: regInstance,
			RegData:  regData,
		}
	}

	reqMsg := &halproto.RegisterRequestMsg{
		Request: []*halproto.RegisterRequest{req},
	}

	// HAL call
	respMsg, err := client.RegisterUpdate(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Register update failed. %v\n", err)
		return
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		} else {
			fmt.Printf("Register write success\n")
		}
	}
}

func datapathCacheDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewDebugClient(c)

	var enable bool
	var sramType halproto.HbmSramType

	if len(args) != 2 {
		fmt.Printf("Arguments required. Use -h to get list of arguments\n")
		return
	}

	if strings.Compare(args[0], "p4-ingress") == 0 {
		sramType = halproto.HbmSramType_SRAM_P4_INGRESS
	} else if strings.Compare(args[0], "p4-egress") == 0 {
		sramType = halproto.HbmSramType_SRAM_P4_EGRESS
	} else if strings.Compare(args[0], "p4-all") == 0 {
		sramType = halproto.HbmSramType_SRAM_P4_ALL
	} else if strings.Compare(args[0], "p4plus-rxdma") == 0 {
		sramType = halproto.HbmSramType_SRAM_P4PLUS_RXDMA
	} else if strings.Compare(args[0], "p4plus-txdma") == 0 {
		sramType = halproto.HbmSramType_SRAM_P4PLUS_TXDMA
	} else if strings.Compare(args[0], "p4plus-all") == 0 {
		sramType = halproto.HbmSramType_SRAM_P4PLUS_ALL
	} else if strings.Compare(args[0], "all") == 0 {
		sramType = halproto.HbmSramType_SRAM_ALL
	} else {
		fmt.Printf("Invalid argument\n")
		return
	}

	if strings.Compare(args[1], "enable") == 0 {
		enable = true
	} else if strings.Compare(args[1], "disable") == 0 {
		enable = false
	} else {
		fmt.Printf("Invalid argument\n")
		return
	}

	req := &halproto.HbmCacheRequest{
		CacheRegions: &halproto.HbmCacheRequest_Sram{
			Sram: &halproto.HbmCacheSram{
				Type:   sramType,
				Enable: enable,
			},
		},
	}

	reqMsg := &halproto.HbmCacheRequestMsg{
		Request: []*halproto.HbmCacheRequest{req},
	}

	// HAL call
	respMsg, err := client.HbmCacheSetup(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("HBM cache setup failed. %v\n", err)
		return
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		} else {
			fmt.Printf("HBM cache setup success\n")
		}
	}
}

func xcvrDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()

	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}

	defer c.Close()

	client := halproto.NewDebugClient(c)

	if cmd.Flags().Changed("valid-check") == false {
		fmt.Printf("Command arguments not provided correctly. Refer to help string for guidance\n")
		return
	}

	enable := true

	var empty *halproto.Empty

	if cmd.Flags().Changed("valid-check") == true {
		if strings.Compare(xcvrValid, "enable") == 0 {
			// HAL call
			_, err = client.XcvrValidCheckEnable(context.Background(), empty)
			enable = true
		} else if strings.Compare(xcvrValid, "disable") == 0 {
			// HAL call
			_, err = client.XcvrValidCheckDisable(context.Background(), empty)
			enable = false
		} else {
			fmt.Printf("Command arguments not provided correctly. Refer to help string for guidance\n")
			return
		}
	}

	enableStr := "enable"

	if enable == true {
		enableStr = "enable"
	} else {
		enableStr = "disable"
	}

	if err != nil {
		fmt.Printf("Transceiver valid check %s failed. %v\n", enableStr, err)
		return
	}

	fmt.Printf("Transceiver valid check %s success\n", enableStr)
}

func traceShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewDebugClient(c)

	var empty *halproto.Empty

	// HAL call
	respMsg, err := client.TraceGet(context.Background(), empty)
	if err != nil {
		fmt.Printf("Getting Trace failed. %v\n", err)
		return
	}

	// Print Trace Level
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
			continue
		}
		traceShowResp(resp)
	}
}

func traceDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewDebugClient(c)

	var traceReqMsg *halproto.TraceRequestMsg

	if cmd.Flags().Changed("level") {
		if isTraceLevelValid(traceLevel) != true {
			fmt.Printf("Invalid argument\n")
			return
		}
		var req *halproto.TraceSpec
		// Set Trace
		req = &halproto.TraceSpec{
			TraceLevel: inputToTraceLevel(traceLevel),
		}
		traceReqMsg = &halproto.TraceRequestMsg{
			Request: []*halproto.TraceSpec{req},
		}
	} else {
		fmt.Printf("Argument required. Set level using '--level ...' flag\n")
		return
	}

	// HAL call
	respMsg, err := client.TraceUpdate(context.Background(), traceReqMsg)
	if err != nil {
		fmt.Printf("Set trace level failed. %v\n", err)
		return
	}

	// Print Trace Level
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
			continue
		}
		traceShowResp(resp)
	}
}

func isTraceLevelValid(level string) bool {
	switch level {
	case "none":
		return true
	case "error":
		return true
	case "debug":
		return true
	case "verbose":
		return true
	default:
		return false
	}
}

func inputToTraceLevel(level string) halproto.TraceLevel {
	switch level {
	case "none":
		return halproto.TraceLevel_TRACE_LEVEL_NONE
	case "error":
		return halproto.TraceLevel_TRACE_LEVEL_ERROR
	case "debug":
		return halproto.TraceLevel_TRACE_LEVEL_DEBUG
	case "verbose":
		return halproto.TraceLevel_TRACE_LEVEL_VERBOSE
	default:
		return halproto.TraceLevel_TRACE_LEVEL_NONE
	}
}

func traceLevelToStr(level halproto.TraceLevel) string {
	switch level {
	case halproto.TraceLevel_TRACE_LEVEL_NONE:
		return "None"
	case halproto.TraceLevel_TRACE_LEVEL_ERROR:
		return "Error"
	case halproto.TraceLevel_TRACE_LEVEL_DEBUG:
		return "Debug"
	case halproto.TraceLevel_TRACE_LEVEL_VERBOSE:
		return "Verbose"
	default:
		return "Invalid"
	}
}

func traceShowResp(resp *halproto.TraceResponse) {
	fmt.Printf("Trace level set to %-12s\n", resp.GetTraceLevel())
}

func flushLogsDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewDebugClient(c)

	var empty *halproto.Empty

	// HAL call
	respMsg, err := client.FlushLogs(context.Background(), empty)
	if err != nil {
		fmt.Printf("Flushing logs failed. %v\n", err)
		return
	}

	// Print Response
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
			continue
		}
		if cmd != nil {
			fmt.Println("Flushing logs succeeded")
		}
	}
}

func llcSetupCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewDebugClient(c)

	var llcType halproto.LlcCounterType

	if len(args) > 0 {
		if strings.Compare(args[0], "cache-read") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_CACHE_READ
		} else if strings.Compare(args[0], "cache-write") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_CACHE_WRITE
		} else if strings.Compare(args[0], "scratchpad-access") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_SCRATCHPAD_ACCESS
		} else if strings.Compare(args[0], "cache-hit") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_CACHE_HIT
		} else if strings.Compare(args[0], "cache-miss") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_CACHE_MISS
		} else if strings.Compare(args[0], "partial-write") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_PARTIAL_WRITE
		} else if strings.Compare(args[0], "cache-maint-op") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_CACHE_MAINT_OP
		} else if strings.Compare(args[0], "eviction") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_EVICTION
		} else if strings.Compare(args[0], "retry-needed") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_RETRY_NEEDED
		} else if strings.Compare(args[0], "retry-access") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_RETRY_ACCESS
		} else if strings.Compare(args[0], "disable") == 0 {
			llcType = halproto.LlcCounterType_LLC_COUNTER_CACHE_NONE
		} else {
			fmt.Printf("Invalid argument\n")
			return
		}
	} else {
		fmt.Printf("Command needs an argument. Refer to help string.\n")
		return
	}

	req := &halproto.LlcSetupRequest{
		Type: llcType,
	}
	reqMsg := &halproto.LlcSetupRequestMsg{
		Request: []*halproto.LlcSetupRequest{req},
	}

	// HAL call
	respMsg, err := client.LlcSetup(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Llc setup failed. %v\n", err)
		return
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		} else {
			str := strings.ToLower(strings.Replace(llcType.String(), "LLC_COUNTER_", "", -1))
			str = strings.Replace(str, "_", "-", -1)
			if strings.Compare(str, "cache-none") == 0 {
				fmt.Printf("LLC tracking disabled\n")
			} else {
				fmt.Printf("LLC set to track %s\n", str)
			}
		}
		return
	}
}

func fwSecProfDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewNwSecurityClient(c)

	var reqMsg *halproto.SecurityProfileRequestMsg

	if cmd.Flags().Changed("id") {
		var secProf *halproto.SecurityProfileSpec

		// Get security profile
		var getReqMsg *halproto.SecurityProfileGetRequestMsg
		var getReq *halproto.SecurityProfileGetRequest

		getReq = &halproto.SecurityProfileGetRequest{
			KeyOrHandle: &halproto.SecurityProfileKeyHandle{
				KeyOrHandle: &halproto.SecurityProfileKeyHandle_ProfileId{
					ProfileId: secProfID,
				},
			},
		}

		getReqMsg = &halproto.SecurityProfileGetRequestMsg{
			Request: []*halproto.SecurityProfileGetRequest{getReq},
		}

		// HAL call
		getRespMsg, err := client.SecurityProfileGet(context.Background(), getReqMsg)
		if err != nil {
			fmt.Printf("Get Security Profile for failed for id. %v\n", err)
			return
		}

		for _, getResp := range getRespMsg.Response {
			if getResp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
				fmt.Printf("HAL Returned non OK status for profile get %v\n", getResp.ApiStatus)
				continue
			}
			secProf = getResp.GetSpec()
		}

		if cmd.Flags().Changed("conntrack") {
			if isConnTrackValid(connTrack) != true {
				fmt.Printf("Invalid argument\n")
				return
			}
			// Set conn tracking
			secProf.CnxnTrackingEn = inputToConnTrack(connTrack)
		}
		if cmd.Flags().Changed("ip-normalization") {
			if isConnTrackValid(ipNormalization) != true {
				fmt.Printf("Invalid argument\n")
				return
			}
			// Set conn tracking
			secProf.IpNormalizationEn = inputToConnTrack(ipNormalization)
		}
		if cmd.Flags().Changed("tcp-normalization") {
			if isConnTrackValid(tcpNormalization) != true {
				fmt.Printf("Invalid argument\n")
				return
			}
			// Set conn tracking
			secProf.TcpNormalizationEn = inputToConnTrack(tcpNormalization)
		}
		if cmd.Flags().Changed("tcp-timeout") {
			if isTimeoutValid("TCP", tcpTimeout) != true {
				fmt.Printf("Invalid argument\n")
				return
			}
			secProf.TcpTimeout = tcpTimeout
		}
		if cmd.Flags().Changed("udp-timeout") {
			if isTimeoutValid("UDP", udpTimeout) != true {
				fmt.Printf("Invalid argument\n")
				return
			}
			secProf.UdpTimeout = udpTimeout
		}
		if cmd.Flags().Changed("icmp-timeout") {
			if isTimeoutValid("ICMP", icmpTimeout) != true {
				fmt.Printf("Invalid argument\n")
				return
			}
			secProf.IcmpTimeout = icmpTimeout
		}
		if cmd.Flags().Changed("idle-timeout") {
			if isTimeoutValid("SESSION-IDLE", idleTimeout) != true {
				fmt.Printf("Invalid argument\n")
				return
			}
			secProf.SessionIdleTimeout = idleTimeout
		}
		if cmd.Flags().Changed("tcp-cxnsetup-timeout") {
			if isTimeoutValid("TCP-CXN-SETUP", tcpCxnsetupTimeout) != true {
				fmt.Printf("Invalid argument\n")
				return
			}
			secProf.TcpCnxnSetupTimeout = tcpCxnsetupTimeout
		}
		if cmd.Flags().Changed("tcp-halfclose-timeout") {
			if isTimeoutValid("TCP-HALF-CLOSE", tcpHalfcloseTimeout) != true {
				fmt.Printf("Invalid argument\n")
				return
			}
			secProf.TcpHalfClosedTimeout = tcpHalfcloseTimeout
		}
		reqMsg = &halproto.SecurityProfileRequestMsg{
			Request: []*halproto.SecurityProfileSpec{secProf},
		}
	} else {
		fmt.Printf("Argument required. Set security profile ID using '--id ...' flag\n")
		return
	}

	// HAL call
	_, err = client.SecurityProfileUpdate(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Set conn tracking failed. %v\n", err)
		return
	}
}

func tcpProxyDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewTcpProxyClient(c)

	defer c.Close()

	tcpGlobalCfg := &halproto.TcpProxyGlobalCfg{}

	if cmd.Flags().Changed("mss") {
		tcpGlobalCfg.Mss = tcpProxyMss
	}
	if cmd.Flags().Changed("cwnd-initial") {
		tcpGlobalCfg.CwndInitial = tcpProxyCwndInitial
	}
	if cmd.Flags().Changed("cwnd-idle") {
		tcpGlobalCfg.CwndIdle = tcpProxyCwndIdle
	}

	globalCfgReqMsg := &halproto.TcpProxyGlobalCfgRequestMsg{
		Request: []*halproto.TcpProxyGlobalCfg{tcpGlobalCfg},
	}

	// HAL call
	respMsg, err := client.TcpProxyGlobalCfgCreate(context.Background(), globalCfgReqMsg)
	if err != nil {
		fmt.Printf("Set conn tracking failed. %v\n", err)
		return
	}

	if respMsg.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
		fmt.Printf("HAL Returned error status. %v\n", respMsg.ApiStatus)
		os.Exit(1)
	}
}

func isConnTrackValid(str string) bool {
	switch str {
	case "on":
		return true
	case "off":
		return true
	default:
		return false
	}
}

func inputToConnTrack(str string) bool {
	switch str {
	case "on":
		return true
	case "off":
		return false
	}

	return false
}

func isTimeoutValid(str string, val uint32) bool {
	switch str {
	case "TCP-IDLE":
		if val > 172800 {
			return false
		}
	case "TCP-CXN-SETUP":
		if val > 60 {
			return false
		}
	case "TCP-HALF-CLOSE":
		if val > 172800 {
			return false
		}
	case "UDP-IDLE":
		if val > 172800 {
			return false
		}
	case "SESSION-IDLE":
		if val > 172800 {
			return false
		}
	case "ICMP-IDLE":
		if val > 172800 {
			return false
		}
	}

	return true
}

func getConnTrack(resp *halproto.SecurityProfileGetResponse) string {
	switch resp.GetSpec().GetCnxnTrackingEn() {
	case true:
		return "on"
	case false:
		return "off"
	}

	return "off"
}

func testSendFinDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewInternalClient(c)

	req := &halproto.TestSendFinRequest{}

	reqMsg := &halproto.TestSendFinRequestMsg{
		Request: []*halproto.TestSendFinRequest{req},
	}

	// HAL call
	respMsg, err := client.TestSendFinReq(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Test send FIN request failed. %v\n", err)
		return
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
		}
	}
}

func sessionCtrlDebugCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()
	client := halproto.NewDebugClient(c)
	if cmd.Flags().Changed("max-session") {
		req := &halproto.SessionCtrlSpec{
			MaxSession: maxSession,
		}
		sessionReqMsg := &halproto.SessionCtrlRequestMsg{
			Spec: []*halproto.SessionCtrlSpec{req},
		}

		_, err := client.SessionCtrlUpdate(context.Background(), sessionReqMsg)
		if err != nil {
			fmt.Printf("SessionCtrl Update failed. %v\n", err)
			return
		}
	}
	fmt.Println("Success: SessionCtrl Update successfull.")
}
