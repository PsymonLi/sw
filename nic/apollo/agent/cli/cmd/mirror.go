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

	uuid "github.com/satori/go.uuid"
	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/apollo/agent/cli/utils"
	"github.com/pensando/sw/nic/apollo/agent/gen/pds"
)

var (
	mirrorsessionID string
	interfaceID     string
	encapType       string
	VpcID           string
	encapVal        uint32
	erspanType      string
	spanType        string
	collectorType   string
	collectorVal    string
	dscp            uint32
	spanID          uint32
	snapLen         uint32
)

var mirrorSessionShowCmd = &cobra.Command{
	Use:     "mirror",
	Short:   "show mirror session information",
	Long:    "show mirror session object information",
	Run:     mirrorSessionShowCmdHandler,
	PreRunE: mirrorSessionShowCmdPreRun,
}

var mirrorSessionShowStatisticsCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show mirror session statistics",
	Long:  "show mirror session statistics",
	Run:   mirrorSessionShowStatisticsCmdHandler,
}

var mirrorSessionDebugCmd = &cobra.Command{
	Use:    "mirror",
	Short:  "debug mirror session",
	Long:   "debug mirror session",
	Hidden: true,
}

var mirrorRspanSessionCreateCmd = &cobra.Command{
	Use:     "rspan",
	Short:   "create mirror rspan session",
	Long:    "create mirror rspan session",
	Run:     mirrorRspanSessionCreateCmdHandler,
	PreRunE: mirrorRspanSessionCreateCmdPreRunE,
}

var mirrorErspanSessionCreateCmd = &cobra.Command{
	Use:     "erspan",
	Short:   "create mirror erspan session",
	Long:    "create mirror erspan session",
	Run:     mirrorErspanSessionCreateCmdHandler,
	PreRunE: mirrorErspanSessionCreateCmdPreRunE,
}

func init() {
	showCmd.AddCommand(mirrorSessionShowCmd)
	mirrorSessionShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	mirrorSessionShowCmd.Flags().Bool("summary", false, "Display number of objects")
	mirrorSessionShowCmd.Flags().StringVarP(&spanType, "type", "t", "", "Specify mirror session type to display (erspan or rspan)")
	mirrorSessionShowCmd.Flags().StringVarP(&mirrorsessionID, "id", "i", "", "Specify Mirror session ID")
	mirrorSessionShowCmd.MarkFlagRequired("type")

	mirrorSessionShowCmd.AddCommand(mirrorSessionShowStatisticsCmd)
	mirrorSessionShowStatisticsCmd.Flags().Bool("yaml", false, "Output in yaml")
	mirrorSessionShowStatisticsCmd.Flags().StringVarP(&mirrorsessionID, "id",
		"i", "", "Specify mirrorSession ID")

	debugCreateCmd.AddCommand(mirrorSessionDebugCmd)
	mirrorSessionDebugCmd.AddCommand(mirrorRspanSessionCreateCmd)
	mirrorRspanSessionCreateCmd.Flags().StringVarP(&ID, "id", "i", "",
		"Specify mirror session ID")
	mirrorRspanSessionCreateCmd.Flags().Uint32Var(&snapLen, "snap-len", 0, "Specify snap length")
	mirrorRspanSessionCreateCmd.Flags().StringVarP(&interfaceID, "interface", "", "",
		"Specify interface (uplink interfaces and host interfaces (aka. lif) are supported)")
	mirrorRspanSessionCreateCmd.Flags().StringVarP(&encapType, "encap-type", "", "none",
		"Specify encap type (dot1q, qinq, mplsoudp, vxlan)")
	mirrorRspanSessionCreateCmd.Flags().Uint32Var(&encapVal, "encap-val", 0,
		"Specify encap value for encap type (dot1q, qinq, mplsoudp, vxlan)")
	mirrorRspanSessionCreateCmd.MarkFlagRequired("id")
	mirrorRspanSessionCreateCmd.MarkFlagRequired("snap-len")
	mirrorRspanSessionCreateCmd.MarkFlagRequired("interface")
	mirrorRspanSessionCreateCmd.MarkFlagRequired("encap-type")
	mirrorRspanSessionCreateCmd.MarkFlagRequired("encap-val")

	mirrorSessionDebugCmd.AddCommand(mirrorErspanSessionCreateCmd)
	mirrorErspanSessionCreateCmd.Flags().StringVarP(&ID, "id", "i", "",
		"Specify mirror session ID")
	mirrorErspanSessionCreateCmd.Flags().Uint32Var(&snapLen, "snap-len", 0, "Specify snap len")
	mirrorErspanSessionCreateCmd.Flags().StringVarP(&erspanType, "erspan-type", "", "none",
		"Specify erspan type (type1, type2, type3)")
	mirrorErspanSessionCreateCmd.Flags().StringVarP(&VpcID, "vpc-id", "v", "", "Specify vpc ID")
	mirrorErspanSessionCreateCmd.Flags().StringVarP(&collectorType, "collector-type", "", "",
		"Specify collector type (tunnel or ip) for ERSPAN mirror session")
	mirrorErspanSessionCreateCmd.Flags().StringVarP(&collectorVal, "collector-val", "", "",
		"Specify tunnel ID (aka TEP) or destination IP for ERSPAN collector")
	mirrorErspanSessionCreateCmd.Flags().Uint32Var(&dscp, "dscp", 0, "Specify DSCP")
	mirrorErspanSessionCreateCmd.Flags().Uint32Var(&spanID, "span-id", 0, "Specify span ID")
	mirrorErspanSessionCreateCmd.MarkFlagRequired("id")
	mirrorErspanSessionCreateCmd.MarkFlagRequired("snap-len")
	mirrorErspanSessionCreateCmd.MarkFlagRequired("interface")
	mirrorErspanSessionCreateCmd.MarkFlagRequired("erspan-type")
	mirrorErspanSessionCreateCmd.MarkFlagRequired("vpc-id")
	mirrorErspanSessionCreateCmd.MarkFlagRequired("collector-type")
	mirrorErspanSessionCreateCmd.MarkFlagRequired("collector-val")
}

func mirrorSessionShowCmdPreRun(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}

	validArgs := []string{"rspan", "erspan"}
	if cmd.Flags().Changed("type") && !checkValidArgs(spanType, validArgs) {
		return fmt.Errorf("Invalid argument for \"type\", please choose from [%s]",
			strings.Join(validArgs, ", "))
	}
	return nil
}

func mirrorSessionShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewMirrorSvcClient(c)

	var req *pds.MirrorSessionGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// Get specific Mirror session
		req = &pds.MirrorSessionGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(mirrorsessionID).Bytes()},
		}
	} else {
		// Get all Mirror sessions
		req = &pds.MirrorSessionGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.MirrorSessionGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting mirror session failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print mirror sessions
	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		countRspan, countErspan := getMirrorSessionsCount(respMsg)
		if strings.ToLower(spanType) == "rspan" {
			printMirrorSessionSummary(countRspan)
		} else {
			printMirrorSessionSummary(countErspan)
		}
	} else {
		countRspan, countErspan := getMirrorSessionsCount(respMsg)
		if strings.ToLower(spanType) == "rspan" {
			printMirrorSessionRspanHeader()
			for _, resp := range respMsg.Response {
				printRspanMirrorSession(resp)
			}
			printMirrorSessionSummary(countRspan)
		} else {
			printMirrorSessionErspanHeader()
			for _, resp := range respMsg.Response {
				printErspanMirrorSession(resp)
			}
			printMirrorSessionSummary(countErspan)
		}
	}
}

func getMirrorSessionsCount(respMsg *pds.MirrorSessionGetResponse) (int, int) {
	countRspan := 0
	countErspan := 0
	for _, resp := range respMsg.Response {
		spec := resp.GetSpec()
		switch spec.GetMirrordst().(type) {
		case *pds.MirrorSessionSpec_RspanSpec:
			countRspan += 1
		case *pds.MirrorSessionSpec_ErspanSpec:
			countErspan += 1
		}
	}
	return countRspan, countErspan
}

func printMirrorSessionSummary(count int) {
	fmt.Printf("\nNo. of mirror objects : %d\n\n", count)
}

func printMirrorSessionRspanHeader() {
	hdrLine := strings.Repeat("-", 106)
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-10s%-16s%-40s\n",
		"ID", "Snap Len", "Encap", "Interface")
	fmt.Println(hdrLine)
}

func printMirrorSessionErspanHeader() {
	hdrLine := strings.Repeat("-", 164)
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-10s%-40s%-44s%-10s%-10s%-10s\n",
		"ID", "Type", "VPC ID", "Tunnel ID / Destination IP", "Span ID",
		"Snap Len", "DSCP")
	fmt.Println(hdrLine)
}

func printRspanMirrorSession(ms *pds.MirrorSession) {
	spec := ms.GetSpec()
	switch spec.GetMirrordst().(type) {
	case *pds.MirrorSessionSpec_RspanSpec:
		encapStr := utils.EncapToString(spec.GetRspanSpec().GetEncap())
		fmt.Printf("%-40s%-10d%-16s%-40s\n",
			utils.IdToStr(spec.GetId()),
			spec.GetSnapLen(), encapStr,
			utils.IdToStr(spec.GetRspanSpec().GetInterface()))
	}
}

func printErspanMirrorSession(ms *pds.MirrorSession) {
	spec := ms.GetSpec()

	switch spec.GetMirrordst().(type) {
	case *pds.MirrorSessionSpec_ErspanSpec:
		erspanSpec := spec.GetErspanSpec()
		if erspanSpec == nil {
			return
		}

		dstStr := ""
		switch spec.GetErspanSpec().GetErspandst().(type) {
		case *pds.ERSpanSpec_TunnelId:
			dstStr = utils.IdToStr(erspanSpec.GetTunnelId())
		case *pds.ERSpanSpec_DstIP:
			dstStr = utils.IPAddrToStr(erspanSpec.GetDstIP())
		}

		erspanTypeStr := strings.Replace(erspanSpec.GetType().String(), "ERSPAN_TYPE_", "", -1)

		fmt.Printf("%-40s%-10s%-40s%-44s%-10d%-10d%-10d\n",
			utils.IdToStr(spec.GetId()),
			erspanTypeStr, utils.IdToStr(erspanSpec.GetVPCId()),
			dstStr, erspanSpec.GetSpanId(),
			spec.GetSnapLen(), erspanSpec.GetDscp())
	}
}

func checkValidArgs(arg string, validArgs []string) bool {
	for _, validArg := range validArgs {
		if strings.ToLower(arg) == validArg {
			return true
		}
	}

	return false
}

func mirrorErspanSessionCreateCmdPreRunE(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	validArgs := []string{"type1", "type2", "type3"}
	if cmd.Flags().Changed("erspan-type") && !checkValidArgs(erspanType, validArgs) {
		return fmt.Errorf("Invalid argument for \"erspan-type\", please choose from [%s]",
			strings.Join(validArgs, ", "))
	}

	validArgs_Collector := []string{"ip", "tunnel"}
	if cmd.Flags().Changed("collector-type") && !checkValidArgs(collectorType, validArgs_Collector) {
		return fmt.Errorf("Invalid argument for \"collector-type\", please choose from [%s]",
			strings.Join(validArgs_Collector, ", "))
	}
	return nil
}

func mirrorRspanSessionCreateCmdPreRunE(cmd *cobra.Command, args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	validArgs := []string{"dot1q", "qinq", "mplsoudp", "vxlan"}
	if cmd.Flags().Changed("encap-type") && !checkValidArgs(encapType, validArgs) {
		return fmt.Errorf("Invalid argument for \"encap-type\", please choose from [%s]",
			strings.Join(validArgs, ", "))
	}
	return nil
}

func mirrorRspanSessionCreateCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewMirrorSvcClient(c)

	req := &pds.MirrorSessionSpec{
		Id:      uuid.FromStringOrNil(ID).Bytes(),
		SnapLen: snapLen,
	}

	// RSPAN
	req.Mirrordst = &pds.MirrorSessionSpec_RspanSpec{
		RspanSpec: &pds.RSpanSpec{
			Interface: uuid.FromStringOrNil(interfaceID).Bytes(),
			Encap: &pds.Encap{
				Type:  utils.StringToEncapType(encapType),
				Value: utils.StringToEncapVal(encapType, encapVal),
			},
		},
	}

	reqMsg := &pds.MirrorSessionRequest{
		Request: []*pds.MirrorSessionSpec{
			req,
		},
	}

	// PDS call
	respMsg, err := client.MirrorSessionCreate(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Creating mirror session failed, err %v\n", err)
		return
	}
	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}
	fmt.Println("Creating mirror session succeeded")
	return
}

func mirrorErspanSessionCreateCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewMirrorSvcClient(c)

	req := &pds.MirrorSessionSpec{
		Id:      uuid.FromStringOrNil(ID).Bytes(),
		SnapLen: snapLen,
	}

	// ERSpan
	if strings.ToLower(collectorType) == "tunnel" {
		req.Mirrordst = &pds.MirrorSessionSpec_ErspanSpec{
			ErspanSpec: &pds.ERSpanSpec{
				Type:   utils.StringToERspanType(erspanType),
				VPCId:  uuid.FromStringOrNil(VpcID).Bytes(),
				Dscp:   dscp,
				SpanId: spanID,
				Erspandst: &pds.ERSpanSpec_TunnelId{
					TunnelId: uuid.FromStringOrNil(collectorVal).Bytes(),
				},
			},
		}
	} else {
		req.Mirrordst = &pds.MirrorSessionSpec_ErspanSpec{
			ErspanSpec: &pds.ERSpanSpec{
				Type:   utils.StringToERspanType(erspanType),
				VPCId:  uuid.FromStringOrNil(VpcID).Bytes(),
				Dscp:   dscp,
				SpanId: spanID,
				Erspandst: &pds.ERSpanSpec_DstIP{
					DstIP: utils.IPAddrStrToPDSIPAddr(collectorVal),
				},
			},
		}
	}
	reqMsg := &pds.MirrorSessionRequest{
		Request: []*pds.MirrorSessionSpec{
			req,
		},
	}

	// PDS call
	respMsg, err := client.MirrorSessionCreate(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Creating mirror session failed, err %v\n", err)
		return
	}
	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}
	fmt.Println("Creating mirror session succeeded")
	return
}

func mirrorSessionShowStatisticsCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewMirrorSvcClient(c)

	var req *pds.MirrorSessionGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// Get specific Mirror Session 
		req = &pds.MirrorSessionGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(mirrorsessionID).Bytes()},
		}
	} else {
		// Get all Mirror Sessions 
		req = &pds.MirrorSessionGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.MirrorSessionGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Mirror session Get failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print Mirror Sessions 
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
			fmt.Printf("Mirror Session ID: %s\n", utils.IdToStr(spec.GetId()))
			fmt.Printf("\tPackets    : %d\n", stats.GetPacketCount())
			fmt.Printf("\tBytes      : %d\n", stats.GetByteCount())
		}
	}
}
