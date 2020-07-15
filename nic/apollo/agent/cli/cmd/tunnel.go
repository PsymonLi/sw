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
	tunnelID  string
	tunnelMAC string
)

var tunnelShowCmd = &cobra.Command{
	Use:   "tunnel",
	Short: "show tunnel information",
	Long:  "show Tunnel object information",
	Run:   tunnelShowCmdHandler,
}

var tunnelUpdateCmd = &cobra.Command{
	Use:   "tunnel",
	Short: "update tunnel information",
	Long:  "update tunnel object information",
	Run:   tunnelUpdateCmdHandler,
}

func init() {
	showCmd.AddCommand(tunnelShowCmd)
	tunnelShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	tunnelShowCmd.Flags().Bool("summary", false, "Display number of objects")
	tunnelShowCmd.Flags().StringVarP(&tunnelID, "id", "i", "", "Specify Tunnel ID")
	debugUpdateCmd.AddCommand(tunnelUpdateCmd)
	tunnelUpdateCmd.Flags().StringVarP(&tunnelID, "id", "i", "", "Specify Tunnel ID")
	tunnelUpdateCmd.Flags().StringVar(&tunnelMAC, "mac", "", "Specify remote MAC address")
	tunnelUpdateCmd.MarkFlagRequired("id")
	tunnelUpdateCmd.MarkFlagRequired("mac")
}

func tunnelUpdateCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewTunnelSvcClient(c)

	// Get Tunnel
	req := &pds.TunnelGetRequest{
		Id: [][]byte{uuid.FromStringOrNil(tunnelID).Bytes()},
	}

	// PDS call
	respMsg, err := client.TunnelGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Tunnel failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	for _, resp := range respMsg.Response {
		spec := &pds.TunnelSpec{
			Id:                    resp.GetSpec().GetId(),
			VPCId:                 resp.GetSpec().GetVPCId(),
			LocalIP:               resp.GetSpec().GetLocalIP(),
			RemoteIP:              resp.GetSpec().GetRemoteIP(),
			Type:                  resp.GetSpec().GetType(),
			Encap:                 resp.GetSpec().GetEncap(),
			Nat:                   resp.GetSpec().GetNat(),
			RemoteService:         resp.GetSpec().GetRemoteService(),
			RemoteServiceEncap:    resp.GetSpec().GetRemoteServiceEncap(),
			RemoteServicePublicIP: resp.GetSpec().GetRemoteServicePublicIP(),
			ToS:                   resp.GetSpec().GetToS(),
			Nh:                    resp.GetSpec().GetNh(),
			MACAddress:            utils.MACAddrStrToUint64(tunnelMAC),
		}
		updReq := &pds.TunnelRequest{
			Request: []*pds.TunnelSpec{
				spec,
			},
		}
		updRespMsg, err := client.TunnelUpdate(context.Background(), updReq)
		if err != nil {
			fmt.Printf("Tunnel update failed, err %v\n", err)
			return
		}
		if updRespMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
			return
		}
		fmt.Printf("Tunnel update successful\n")
	}
}
func tunnelShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewTunnelSvcClient(c)

	var req *pds.TunnelGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// Get specific Tunnel
		req = &pds.TunnelGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(tunnelID).Bytes()},
		}
	} else {
		// Get all Tunnels
		req = &pds.TunnelGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.TunnelGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Tunnel failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print Tunnels
	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		printTunnelSummary(len(respMsg.Response))
	} else {
		printTunnelHeader()
		for _, resp := range respMsg.Response {
			printTunnel(resp)
		}
		printTunnelSummary(len(respMsg.Response))
	}
}

func printTunnelSummary(count int) {
	fmt.Printf("\nNo. of tunnels : %d\n\n", count)
}

func printTunnelHeader() {
	hdrLine := strings.Repeat("-", 227)
	fmt.Printf("Legend\n")
	fmt.Printf("NhType: U-NH (Underlay Nexthop), U-ECMP (Underlay ECMP), O (Overlay), D (Drop)\n")
	fmt.Printf("Type: WL (Workload), I-DC (Inter-Datacenter), IGW (Internet Gateway), S (Service)\n")
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-40s%-40s%-20s%-10s%-40s%-10s%-16s%-6s%-5s\n",
		"ID", "VpcID", "RemoteIP", "DMAC", "NhType",
		"NhID", "Type", "Encap", "ToS", "HW ID")
	fmt.Println(hdrLine)
}

func printTunnel(tunnel *pds.Tunnel) {
	spec := tunnel.GetSpec()
	encapStr := utils.EncapToString(spec.GetEncap())
	tunnelType := strings.Replace(spec.GetType().String(), "TUNNEL_TYPE_", "", -1)
	tunnelType = strings.ToLower(strings.Replace(tunnelType, "_", "-", -1))
	nhType := "-"
	nhID := "-"
	switch tunnelType {
	case "workload":
		tunnelType = "WL"
	case "igw":
		tunnelType = "IGW"
	case "inter-dc":
		tunnelType = "I-DC"
	case "service":
		tunnelType = "S"
	}
	if spec.GetNexthopId() != nil {
		nhType = "U-NH"
		nhID = utils.IdToStr(spec.GetNexthopId())
	} else if spec.GetNexthopGroupId() != nil {
		nhType = "U-ECMP"
		nhID = utils.IdToStr(spec.GetNexthopGroupId())
	} else if spec.GetTunnelId() != nil {
		nhType = "O"
		nhID = utils.IdToStr(spec.GetTunnelId())
	} else if spec.GetDropNexthop() != nil {
		nhType = "D"
	}
	fmt.Printf("%-40s%-40s%-40s%-20s%-10s%-40s%-10s%-16s%-6d%-5d\n",
		utils.IdToStr(spec.GetId()),
		utils.IdToStr(spec.GetVPCId()),
		utils.IPAddrToStr(spec.GetRemoteIP()),
		utils.MactoStr(spec.GetMACAddress()),
		nhType, nhID,
		tunnelType, encapStr,
		spec.GetToS(), tunnel.GetStatus().GetHwId())
}
