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
)

var mirrorSessionShowCmd = &cobra.Command{
	Use:   "mirror",
	Short: "show mirror session information",
	Long:  "show mirror session object information",
	Run:   mirrorSessionShowCmdHandler,
}

func init() {
	showCmd.AddCommand(mirrorSessionShowCmd)
	mirrorSessionShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	mirrorSessionShowCmd.Flags().Bool("summary", false, "Display number of objects")
	mirrorSessionShowCmd.Flags().StringVarP(&mirrorsessionID, "id", "i", "", "Specify Mirror session ID")
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
		printMirrorSessionSummary(len(respMsg.Response))
	} else {
		printMirrorSessionRspanHeader()
		for _, resp := range respMsg.Response {
			printRspanMirrorSession(resp)
		}
		printMirrorSessionErspanHeader()
		for _, resp := range respMsg.Response {
			printErspanMirrorSession(resp)
		}
		printMirrorSessionSummary(len(respMsg.Response))
	}
}

func printMirrorSessionSummary(count int) {
	fmt.Printf("\nNo. of mirror objects : %d\n\n", count)
}

func printMirrorSessionRspanHeader() {
	hdrLine := strings.Repeat("-", 84)
	rspanLine := strings.Repeat(" ", 23)
	rspanLine += "RSPAN SESSIONS"
	rspanLine += strings.Repeat(" ", 23)
	fmt.Println(hdrLine)
	fmt.Println(rspanLine)
	fmt.Println(hdrLine)
	fmt.Printf("%-6s%-10s%-10s%-16s%-40s\n",
		"ID", "SpanType", "snapLen", "Encap", "Interface")
	fmt.Println(hdrLine)
}

func printMirrorSessionErspanHeader() {
	hdrLine := strings.Repeat("-", 64)
	erspanLine := strings.Repeat(" ", 22)
	erspanLine += "ERSPAN  SESSIONS"
	erspanLine += strings.Repeat(" ", 22)
	fmt.Println(hdrLine)
	fmt.Println(erspanLine)
	fmt.Println(hdrLine)
	fmt.Printf("%-6s%-10s%-10s%-36s\n",
		"ID", "SpanType", "snapLen", "ERSpanDst")
	fmt.Println(hdrLine)
}

func printRspanMirrorSession(ms *pds.MirrorSession) {
	spec := ms.GetSpec()
	spanTypeStr := ""
	if spec.GetRspanSpec() != nil {
		spanTypeStr += "RSPAN"
	} else if spec.GetErspanSpec() != nil {
		spanTypeStr += "ERSPAN"
	}
	if spanTypeStr == "RSPAN" {
		encapStr := utils.EncapToString(spec.GetRspanSpec().GetEncap())
		fmt.Printf("%-6d%-10s%-10d%-16s%-40s\n",
			spec.GetId(), spanTypeStr,
			spec.GetSnapLen(), encapStr,
			utils.IdToStr(spec.GetRspanSpec().GetInterface()))
	}
}

func printErspanMirrorSession(ms *pds.MirrorSession) {
	spec := ms.GetSpec()
	spanTypeStr := ""
	if spec.GetRspanSpec() != nil {
		spanTypeStr += "RSPAN"
	} else if spec.GetErspanSpec() != nil {
		spanTypeStr += "ERSPAN"
	}
	if spanTypeStr == "ERSPAN" {
		fmt.Printf("%-6d%-10s%-10d%-36s\n",
			spec.GetId(), spanTypeStr, spec.GetSnapLen(),
			spec.GetErspanSpec().GetTunnelId())
	}
}
