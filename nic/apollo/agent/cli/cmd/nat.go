//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	"math"
	"reflect"
	"strings"

	"github.com/gogo/protobuf/types"

	uuid "github.com/satori/go.uuid"
	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/apollo/agent/cli/utils"
	"github.com/pensando/sw/nic/apollo/agent/gen/pds"
)

type myNatPortBlock struct {
	msg *pds.NatPortBlock
}

var natShowCmd = &cobra.Command{
	Use:   "nat",
	Short: "show NAT Port Block information",
	Long:  "show NAT Port Block object information",
	Run:   natShowCmdHandler,
}

var (
	// ID holds NAT PB ID
	natPbId string
)

func init() {
	showCmd.AddCommand(natShowCmd)
	natShowCmd.Flags().StringVarP(&natPbId, "id", "i", "", "Specify NAT Port Block ID")
	natShowCmd.Flags().Bool("yaml", true, "Output in yaml")
	natShowCmd.Flags().Bool("summary", false, "Display number of objects")
}

func natShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to PDS
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the PDS, is PDS running?\n")
		return
	}

	defer c.Close()

	client := pds.NewNatSvcClient(c)
	var req *pds.NatPortBlockGetRequest

	if cmd != nil && cmd.Flags().Changed("id") {
		req = &pds.NatPortBlockGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(natPbId).Bytes()},
		}
	} else {
		req = &pds.NatPortBlockGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.NatPortBlockGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting NAT port block failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	if (cmd != nil) && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		natPbPrintSummary(len(respMsg.Response))
	} else {
		// Print NAT Port Blocks
		printNatPbHeader()
		for _, resp := range respMsg.Response {
			printNatPb(resp)
		}
		natPbPrintSummary(len(respMsg.Response))
	}
}

func natPbPrintSummary(count int) {
	fmt.Printf("\nNo. of NAT port blocks : %d\n\n", count)
}

func printNatPbHeader() {
	hdrLine := strings.Repeat("-", 124)
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-20s%-10s%-10s%-10s%-10s%-10s\n",
		"ID", "Prefix", "Protocol", "Port Lo", "Port Hi", "InUseCnt", "SessionCnt")
	fmt.Println(hdrLine)
}

func printNatPb(nat *pds.NatPortBlock) {
	spec := nat.GetSpec()
	stats := nat.GetStats()
	var ipv4prefix pds.IPv4Prefix
	if spec.GetNatAddress().GetRange() != nil {
		diff := spec.GetNatAddress().GetRange().GetIPv4Range().GetHigh().GetV4Addr() -
			spec.GetNatAddress().GetRange().GetIPv4Range().GetLow().GetV4Addr() + 1
		ipv4prefix.Addr = spec.GetNatAddress().GetRange().GetIPv4Range().GetLow().GetV4Addr()
		ipv4prefix.Len = 32 - uint32(math.Log2(float64(diff)))
	} else {
		ipv4prefix.Addr = spec.GetNatAddress().GetPrefix().GetIPv4Subnet().GetAddr().GetV4Addr()
		ipv4prefix.Len = spec.GetNatAddress().GetPrefix().GetIPv4Subnet().GetLen()
	}
	fmt.Printf("%-40s%-20s%-10s%-10d%-10d%-10d%-10d\n",
		utils.IdToStr(spec.GetId()),
		utils.IPv4PrefixToStr(&ipv4prefix),
		utils.IPPrototoStr(spec.GetProtocol()),
		spec.GetPorts().GetPortLow(),
		spec.GetPorts().GetPortHigh(),
		stats.GetInUseCount(),
		stats.GetSessionCount())
}

// PrintObject interface
func (natMsg myNatPortBlock) PrintHeader() {
	printNatPbHeader()
}

func (natMsg myNatPortBlock) PrintSummary(count int) {
	natPbPrintSummary(count)
}

func (natPb myNatPortBlock) HandleObject(data *types.Any) (done bool) {
	err := types.UnmarshalAny(data, natPb.msg)
	if err != nil {
		fmt.Printf("Command failed with %v error\n", err)
		done = true
		return
	}
	if natPb.msg.GetSpec().GetProtocol() == 0 {
		// Last message
		done = true
		return
	}
	printNatPb(natPb.msg)

	done = false
	return
}
