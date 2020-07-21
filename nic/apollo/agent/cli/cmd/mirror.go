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

var mirrorSessionDeleteCmd = &cobra.Command{
	Use:    "mirror",
	Short:  "delete mirror session",
	Long:   "delete mirror session",
	Run:    mirrorSessionDeleteCmdHandler,
	Hidden: true,
}

var mirrorSessionUpdateCmd = &cobra.Command{
	Use:    "mirror",
	Short:  "update mirror session",
	Long:   "update mirror session",
	Hidden: true,
}

var mirrorSessionCreateCmd = &cobra.Command{
	Use:    "mirror",
	Short:  "create mirror session",
	Long:   "create mirror session",
	Hidden: true,
}

var mirrorRspanSessionCreateCmd = &cobra.Command{
	Use:     "rspan",
	Short:   "create RSPAN mirror session",
	Long:    "create RSPAN mirror session",
	Run:     mirrorRspanSessionCreateCmdHandler,
	PreRunE: mirrorRspanSessionCreateCmdPreRunE,
}

var mirrorErspanSessionCreateCmd = &cobra.Command{
	Use:     "erspan",
	Short:   "create ERSPAN mirror session",
	Long:    "create ERSPAN mirror session",
	Run:     mirrorErspanSessionCreateCmdHandler,
	PreRunE: mirrorErspanSessionCreateCmdPreRunE,
}

var mirrorRspanSessionUpdateCmd = &cobra.Command{
	Use:     "rspan",
	Short:   "update RSPAN mirror session",
	Long:    "update RSPAN mirror session",
	Run:     mirrorRspanSessionUpdateCmdHandler,
	PreRunE: mirrorRspanSessionUpdateCmdPreRunE,
}

var mirrorErspanSessionUpdateCmd = &cobra.Command{
	Use:     "erspan",
	Short:   "update ERSPAN mirror session",
	Long:    "update ERSPAN mirror session",
	Run:     mirrorErspanSessionUpdateCmdHandler,
	PreRunE: mirrorErspanSessionUpdateCmdPreRunE,
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

	debugCreateCmd.AddCommand(mirrorSessionCreateCmd)
	mirrorSessionCreateCmd.AddCommand(mirrorRspanSessionCreateCmd)
	mirrorRspanSessionCreateCmd.Flags().StringVarP(&mirrorsessionID, "id", "i", "",
		"Specify mirror session ID")
	mirrorRspanSessionCreateCmd.Flags().Uint32Var(&snapLen, "snap-len", 0, "Specify snap length")
	mirrorRspanSessionCreateCmd.Flags().StringVarP(&interfaceID, "interface", "", "",
		"Specify interface (uplink interfaces and host interfaces (aka. lif) are supported)")
	mirrorRspanSessionCreateCmd.Flags().StringVarP(&encapType, "encap-type", "", "none",
		"Specify encap type (dot1q, qinq, mplsoudp, vxlan)")
	mirrorRspanSessionCreateCmd.Flags().Uint32Var(&encapVal, "encap-val", 0,
		"Specify encap value for encap type (dot1q, qinq, mplsoudp, vxlan)")
	mirrorRspanSessionCreateCmd.MarkFlagRequired("id")
	mirrorRspanSessionCreateCmd.MarkFlagRequired("interface")

	debugUpdateCmd.AddCommand(mirrorSessionUpdateCmd)
	mirrorSessionUpdateCmd.AddCommand(mirrorRspanSessionUpdateCmd)
	mirrorRspanSessionUpdateCmd.Flags().StringVarP(&mirrorsessionID, "id", "i", "",
		"Specify mirror session ID")
	mirrorRspanSessionUpdateCmd.Flags().Uint32Var(&snapLen, "snap-len", 0, "Specify snap length")
	mirrorRspanSessionUpdateCmd.Flags().StringVarP(&interfaceID, "interface", "", "",
		"Specify interface (uplink interfaces and host interfaces (aka. lif) are supported)")
	mirrorRspanSessionUpdateCmd.Flags().StringVarP(&encapType, "encap-type", "", "none",
		"Specify encap type (dot1q, qinq, mplsoudp, vxlan)")
	mirrorRspanSessionUpdateCmd.Flags().Uint32Var(&encapVal, "encap-val", 0,
		"Specify encap value for encap type (dot1q, qinq, mplsoudp, vxlan)")
	mirrorRspanSessionUpdateCmd.MarkFlagRequired("id")

	mirrorSessionCreateCmd.AddCommand(mirrorErspanSessionCreateCmd)
	mirrorErspanSessionCreateCmd.Flags().StringVarP(&mirrorsessionID, "id", "i", "",
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
	mirrorErspanSessionCreateCmd.MarkFlagRequired("vpc-id")
	mirrorErspanSessionCreateCmd.MarkFlagRequired("collector-type")
	mirrorErspanSessionCreateCmd.MarkFlagRequired("collector-val")

	mirrorSessionUpdateCmd.AddCommand(mirrorErspanSessionUpdateCmd)
	mirrorErspanSessionUpdateCmd.Flags().StringVarP(&mirrorsessionID, "id", "i", "",
		"Specify mirror session ID")
	mirrorErspanSessionUpdateCmd.Flags().Uint32Var(&snapLen, "snap-len", 0, "Specify snap len")
	mirrorErspanSessionUpdateCmd.Flags().StringVarP(&erspanType, "erspan-type", "", "none",
		"Specify erspan type (type1, type2, type3)")
	mirrorErspanSessionUpdateCmd.Flags().StringVarP(&VpcID, "vpc-id", "v", "", "Specify vpc ID")
	mirrorErspanSessionUpdateCmd.Flags().StringVarP(&collectorType, "collector-type", "", "",
		"Specify collector type (tunnel or ip) for ERSPAN mirror session")
	mirrorErspanSessionUpdateCmd.Flags().StringVarP(&collectorVal, "collector-val", "", "",
		"Specify tunnel ID (aka TEP) or destination IP for ERSPAN collector")
	mirrorErspanSessionUpdateCmd.Flags().Uint32Var(&dscp, "dscp", 0, "Specify DSCP")
	mirrorErspanSessionUpdateCmd.Flags().Uint32Var(&spanID, "span-id", 0, "Specify span ID")
	mirrorErspanSessionUpdateCmd.MarkFlagRequired("id")

	debugDeleteCmd.AddCommand(mirrorSessionDeleteCmd)
	mirrorSessionDeleteCmd.Flags().StringVarP(&mirrorsessionID, "id", "i", "",
		"Specify mirror session ID")
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
		Id:      uuid.FromStringOrNil(mirrorsessionID).Bytes(),
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

func mirrorErspanSessionUpdateCmdPreRunE(cmd *cobra.Command,
	args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	if !cmd.Flags().Changed("snap-len") &&
		!cmd.Flags().Changed("erspan-type") &&
		!cmd.Flags().Changed("vpc-id") &&
		!cmd.Flags().Changed("collector-type") &&
		!cmd.Flags().Changed("collector-val") &&
		!cmd.Flags().Changed("dscp") &&
		!cmd.Flags().Changed("span-id") {
		return fmt.Errorf("Nothing to update")
	}
	validArgs_Collector := []string{"ip", "tunnel"}
	if cmd.Flags().Changed("collector-type") &&
		!checkValidArgs(collectorType, validArgs_Collector) {
		return fmt.Errorf("Invalid argument for \"collector-type\""+
			", please choose from [%s]",
			strings.Join(validArgs_Collector, ", "))
	}
	validArgs := []string{"type1", "type2", "type3"}
	if cmd.Flags().Changed("erspan-type") &&
		!checkValidArgs(erspanType, validArgs) {
		return fmt.Errorf("Invalid argument for \"erspan-type\""+
			", please choose from [%s]", strings.Join(validArgs, ", "))
	}
	return nil
}

func mirrorErspanSessionUpdateCmdHandler(cmd *cobra.Command, args []string) {
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

	var getReq *pds.MirrorSessionGetRequest
	// Get specific Mirror Session
	getReq = &pds.MirrorSessionGetRequest{
		Id: [][]byte{uuid.FromStringOrNil(mirrorsessionID).Bytes()},
	}

	// PDS call
	getRespMsg, err := client.MirrorSessionGet(context.Background(), getReq)
	if err != nil {
		fmt.Printf("Mirror session Get failed, err %v\n", err)
		return
	}
	if getRespMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", getRespMsg.ApiStatus)
		return
	}
	getResp := getRespMsg.GetResponse()
	if getResp[0].GetSpec().GetErspanSpec() == nil {
		fmt.Printf("Mirror session to be updated is not of type ERSPAN\n")
		return
	}

	// construct update request
	reqSnapLen := getResp[0].GetSpec().GetSnapLen()
	reqType := getResp[0].GetSpec().GetErspanSpec().GetType()
	reqVpcID := getResp[0].GetSpec().GetErspanSpec().GetVPCId()
	reqDscp := getResp[0].GetSpec().GetErspanSpec().GetDscp()
	reqSpanID := getResp[0].GetSpec().GetErspanSpec().GetSpanId()
	reqDstIP := getResp[0].GetSpec().GetErspanSpec().GetDstIP()
	reqTunnelID := getResp[0].GetSpec().GetErspanSpec().GetTunnelId()
	if cmd.Flags().Changed("snap-len") {
		reqSnapLen = snapLen
	}
	if cmd.Flags().Changed("erspan-type") {
		reqType = utils.StringToERspanType(erspanType)
	}
	if cmd.Flags().Changed("vpc-id") {
		reqVpcID = uuid.FromStringOrNil(VpcID).Bytes()
	}
	if cmd.Flags().Changed("collector-type") {
		if strings.ToLower(collectorType) == "tunnel" {
			reqDstIP = nil
			reqTunnelID = uuid.FromStringOrNil(collectorVal).Bytes()
		} else {
			reqTunnelID = nil
			reqDstIP = utils.IPAddrStrToPDSIPAddr(collectorVal)
		}
	}
	if cmd.Flags().Changed("dscp") {
		reqDscp = dscp
	}
	if cmd.Flags().Changed("span-id") {
		reqSpanID = spanID
	}
	req := &pds.MirrorSessionSpec{
		Id:      uuid.FromStringOrNil(mirrorsessionID).Bytes(),
		SnapLen: reqSnapLen,
	}
	if reqDstIP == nil {
		req.Mirrordst = &pds.MirrorSessionSpec_ErspanSpec{
			ErspanSpec: &pds.ERSpanSpec{
				Type:   reqType,
				VPCId:  reqVpcID,
				Dscp:   reqDscp,
				SpanId: reqSpanID,
				Erspandst: &pds.ERSpanSpec_TunnelId{
					TunnelId: reqTunnelID,
				},
			},
		}
	} else {
		req.Mirrordst = &pds.MirrorSessionSpec_ErspanSpec{
			ErspanSpec: &pds.ERSpanSpec{
				Type:   reqType,
				VPCId:  reqVpcID,
				Dscp:   reqDscp,
				SpanId: reqSpanID,
				Erspandst: &pds.ERSpanSpec_DstIP{
					DstIP: reqDstIP,
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
	respMsg, err := client.MirrorSessionUpdate(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Updating mirror session failed, err %v\n", err)
		return
	}
	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Update operation failed with %v error\n", respMsg.ApiStatus)
		return
	}
	fmt.Println("Updating mirror session succeeded")
	return
}

func mirrorRspanSessionUpdateCmdPreRunE(cmd *cobra.Command,
	args []string) error {
	if cmd == nil {
		return fmt.Errorf("Invalid argument")
	}
	if !cmd.Flags().Changed("snap-len") &&
		!cmd.Flags().Changed("interface") &&
		!cmd.Flags().Changed("encap-type") &&
		!cmd.Flags().Changed("encap-val") {
		return fmt.Errorf("Nothing to update")
	}
	if cmd.Flags().Changed("encap-type") != cmd.Flags().Changed("encap-val") {
		return fmt.Errorf("Encap type and encap value " +
			"both are needed for update")
	}
	validArgs := []string{"dot1q", "qinq", "mplsoudp", "vxlan"}
	if cmd.Flags().Changed("encap-type") &&
		!checkValidArgs(encapType, validArgs) {
		return fmt.Errorf("Invalid argument for \"encap-type\""+
			", please choose from [%s]", strings.Join(validArgs, ", "))
	}
	return nil
}

func mirrorRspanSessionUpdateCmdHandler(cmd *cobra.Command, args []string) {
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

	var getReq *pds.MirrorSessionGetRequest
	// Get specific Mirror Session
	getReq = &pds.MirrorSessionGetRequest{
		Id: [][]byte{uuid.FromStringOrNil(mirrorsessionID).Bytes()},
	}

	// PDS call
	getRespMsg, err := client.MirrorSessionGet(context.Background(), getReq)
	if err != nil {
		fmt.Printf("Mirror session Get failed, err %v\n", err)
		return
	}
	if getRespMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", getRespMsg.ApiStatus)
		return
	}
	getResp := getRespMsg.GetResponse()
	if getResp[0].GetSpec().GetRspanSpec() == nil {
		fmt.Printf("Mirror session to be updated is not of type RSPAN\n")
		return
	}

	// construct update message
	reqSnapLen := getResp[0].GetSpec().GetSnapLen()
	reqInterface := getResp[0].GetSpec().GetRspanSpec().GetInterface()
	reqEncap := getResp[0].GetSpec().GetRspanSpec().GetEncap()
	if cmd.Flags().Changed("snap-len") {
		reqSnapLen = snapLen
	}
	if cmd.Flags().Changed("interface") {
		reqInterface = uuid.FromStringOrNil(interfaceID).Bytes()
	}
	if cmd.Flags().Changed("encap-type") {
		reqEncap.Type = utils.StringToEncapType(encapType)
		reqEncap.Value = utils.StringToEncapVal(encapType, encapVal)
	}
	req := &pds.MirrorSessionSpec{
		Id:      uuid.FromStringOrNil(mirrorsessionID).Bytes(),
		SnapLen: reqSnapLen,
	}
	req.Mirrordst = &pds.MirrorSessionSpec_RspanSpec{
		RspanSpec: &pds.RSpanSpec{
			Interface: reqInterface,
			Encap:     reqEncap,
		},
	}
	reqMsg := &pds.MirrorSessionRequest{
		Request: []*pds.MirrorSessionSpec{
			req,
		},
	}

	// PDS call
	respMsg, err := client.MirrorSessionUpdate(context.Background(), reqMsg)
	if err != nil {
		fmt.Printf("Updating mirror session failed, err %v\n", err)
		return
	}
	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Update operation failed with %v error\n", respMsg.ApiStatus)
		return
	}
	fmt.Println("Updating mirror session succeeded")
	return
}

func mirrorSessionDeleteCmdHandler(cmd *cobra.Command, args []string) {
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

	req := &pds.MirrorSessionDeleteRequest{
		Id: [][]byte{uuid.FromStringOrNil(mirrorsessionID).Bytes()},
	}

	// PDS call
	respMsg, err := client.MirrorSessionDelete(context.Background(), req)
	if err != nil {
		fmt.Printf("Deleting mirror session failed, err %v\n", err)
		return
	}
	if respMsg.ApiStatus[0] != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}
	fmt.Println("Deleting mirror session succeeded")
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
		Id:      uuid.FromStringOrNil(mirrorsessionID).Bytes(),
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
