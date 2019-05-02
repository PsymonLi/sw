//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"errors"
	"fmt"
	"hash/fnv"
	"net"
	"os"
	"reflect"
	"strings"

	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/agent/cmd/halctl/utils"
	"github.com/pensando/sw/nic/agent/netagent/datapath/halproto"
)

var (
	ifID           uint64
	ifStatusID     uint64
	ifEncap        string
	ifName         string
	ifSubIP        string
	ifOverlayIP    string
	ifMplsIn       string
	ifMplsOut      uint32
	ifTunnelDestIP string
	ifSourceGw     string
	ifGwMac        string
	ifOverlayMac   string
	ifPfMac        string
	ifIngressBw    uint32
	ifEgressBw     uint32
)

var ifShowCmd = &cobra.Command{
	Use:   "interface",
	Short: "show interface information",
	Long:  "show interface object information",
	Run:   ifShowCmdHandler,
}

var ifShowSpecCmd = &cobra.Command{
	Use:   "spec",
	Short: "show interface spec information",
	Long:  "show interface object spec information",
	Run:   ifShowCmdHandler,
}

var ifShowStatusCmd = &cobra.Command{
	Use:   "status",
	Short: "show interface status information",
	Long:  "show interface object status information",
	Run:   ifShowStatusCmdHandler,
}

var ifUpdateCmd = &cobra.Command{
	Use:          "interface",
	Short:        "Create interface",
	Long:         "Create interface",
	RunE:         ifUpdateCmdHandler,
	SilenceUsage: true,
}

var ifDeleteCmd = &cobra.Command{
	Use:          "interface",
	Short:        "Delete interface",
	Long:         "Delete interface",
	RunE:         ifDeleteCmdHandler,
	SilenceUsage: true,
}

func init() {
	showCmd.AddCommand(ifShowCmd)
	ifShowCmd.AddCommand(ifShowSpecCmd)
	ifShowCmd.AddCommand(ifShowStatusCmd)

	ifShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	ifShowCmd.Flags().Uint64Var(&ifID, "id", 1, "Specify if-id")
	ifShowSpecCmd.Flags().Uint64Var(&ifID, "id", 1, "Specify if-id")
	ifShowStatusCmd.Flags().Uint64Var(&ifStatusID, "id", 1, "Specify if-id")

	debugDeleteCmd.AddCommand(ifDeleteCmd)
	ifDeleteCmd.Flags().StringVar(&ifEncap, "encap", "", "Encap type (Ex: MPLSoUDP)")
	ifDeleteCmd.Flags().StringVar(&ifName, "name", "", "Interface name")
	ifDeleteCmd.MarkFlagRequired("encap")
	ifDeleteCmd.MarkFlagRequired("name")

	debugUpdateCmd.AddCommand(ifUpdateCmd)
	ifUpdateCmd.Flags().StringVar(&ifEncap, "encap", "", "Encap type (Ex: MPLSoUDP)")
	ifUpdateCmd.Flags().StringVar(&ifName, "name", "", "Interface name")
	ifUpdateCmd.Flags().StringVar(&ifSubIP, "substrate-ip", "", "Substrate IPv4 address")
	ifUpdateCmd.Flags().StringVar(&ifOverlayIP, "overlay-ip", "", "Specify overlay IPv4 address in comma separated list (Max of 2 supported). Ex: 1.2.3.4,2.3.4.5")
	ifUpdateCmd.Flags().StringVar(&ifMplsIn, "mpls-in", "", "Specify incoming MPLS label as comma separated list (Max of 2 supported)")
	ifUpdateCmd.Flags().Uint32Var(&ifMplsOut, "mpls-out", 0, "Specify outgoing MPLS label")
	ifUpdateCmd.Flags().StringVar(&ifTunnelDestIP, "tunnel-dest-ip", "", "Tunnel destination IPv4 address")
	ifUpdateCmd.Flags().StringVar(&ifSourceGw, "source-gw", "", "Specify source gateway. Must be IPv4 prefix as a.b.c.d/xx")
	ifUpdateCmd.Flags().StringVar(&ifGwMac, "gw-mac", "", "Specify gateway MAC address as aabb.ccdd.eeff")
	ifUpdateCmd.Flags().Uint32Var(&ifIngressBw, "ingress-bw", 0, "Specify ingress bandwidth in KBytes/sec <0-12500000 KBytes/sec>. 0 means no policer")
	ifUpdateCmd.Flags().Uint32Var(&ifEgressBw, "egress-bw", 0, "Specify egress bandwidth in KBytes/sec <0-12500000 KBytes/sec>. 0 means no policer")
	ifUpdateCmd.Flags().StringVar(&ifOverlayMac, "overlay-mac", "", "Specify overlay MAC address as aabb.ccdd.eeff (optional)")
	ifUpdateCmd.Flags().StringVar(&ifPfMac, "pf-mac", "", "Specify PF MAC address as aabb.ccdd.eeff (optional)")

	ifUpdateCmd.MarkFlagRequired("encap")
	ifUpdateCmd.MarkFlagRequired("name")
	ifUpdateCmd.MarkFlagRequired("substrate-ip")
	ifUpdateCmd.MarkFlagRequired("overlay-ip")
	ifUpdateCmd.MarkFlagRequired("mpls-in")
	ifUpdateCmd.MarkFlagRequired("mpls-out")
	ifUpdateCmd.MarkFlagRequired("tunnel-dest-ip")
	ifUpdateCmd.MarkFlagRequired("source-gw")
	ifUpdateCmd.MarkFlagRequired("gw-mac")
	ifUpdateCmd.MarkFlagRequired("ingress-bw")
	ifUpdateCmd.MarkFlagRequired("egress-bw")
}

func ifDeleteCmdHandler(cmd *cobra.Command, args []string) error {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewInterfaceClient(c)

	if strings.Compare(ifEncap, "MPLSoUDP") != 0 {
		return errors.New("Invalid encap type specified")
	}
	intfID := Hash(ifName)

	req := &halproto.InterfaceDeleteRequest{
		KeyOrHandle: &halproto.InterfaceKeyHandle{
			KeyOrHandle: &halproto.InterfaceKeyHandle_InterfaceId{
				InterfaceId: intfID,
			},
		},
	}

	ifDeleteReqMsg := &halproto.InterfaceDeleteRequestMsg{
		Request: []*halproto.InterfaceDeleteRequest{req},
	}

	// HAL call
	respMsg, err := client.InterfaceDelete(context.Background(), ifDeleteReqMsg)
	if err != nil {
		return err
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			errStr := fmt.Sprintf("Operation failed with %v error", resp.ApiStatus)
			return errors.New(errStr)
		}
		fmt.Printf("Interface delete succeeded\n")
	}

	return nil
}

func ifUpdateCmdHandler(cmd *cobra.Command, args []string) error {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	defer c.Close()

	client := halproto.NewInterfaceClient(c)

	var substrateIP uint32
	var overlayIP [2]uint32
	var tunnelDestIP uint32
	var mplsOut halproto.MplsTag
	var sourceGWPrefix uint32
	var sourceGWPrefixLen uint32
	var gwMac uint64
	var pfMac uint64
	var overlayMac uint64
	var intfID uint64

	if strings.Compare(ifEncap, "MPLSoUDP") != 0 {
		return errors.New("Invalid encap type specified")
	}

	overlay := strings.Split(ifOverlayIP, ",")
	numOverlay := len(overlay)
	for i := 0; i < numOverlay; i++ {
		if len(strings.Split(overlay[i], ".")) != 4 {
			return errors.New("Invalid overlay IP address specified")
		}
		overlayIP[i] = utils.IPAddrStrtoUint32(overlay[i])
	}

	mplsInStr := strings.Split(ifMplsIn, ",")
	numMplsIn := len(mplsInStr)
	mplsInArray := make([]*halproto.MplsTag, numMplsIn)
	for i := 0; i < numMplsIn; i++ {
		var mplsInArrayIndex halproto.MplsTag
		fmt.Sscanf(mplsInStr[i], "%d", &mplsInArrayIndex.Label)
		mplsInArrayIndex.Exp = 0
		mplsInArrayIndex.Ttl = 64
		mplsInArray[i] = &mplsInArrayIndex
	}

	if len(strings.Split(ifSubIP, ".")) != 4 {
		return errors.New("Invalid substrate IP address specified")
	}
	substrateIP = utils.IPAddrStrtoUint32(ifSubIP)

	if len(strings.Split(ifTunnelDestIP, ".")) != 4 {
		return errors.New("Invalid tunnel destination IP address specified")
	}
	tunnelDestIP = utils.IPAddrStrtoUint32(ifTunnelDestIP)

	mac, err := net.ParseMAC(ifGwMac)
	if err != nil {
		return errors.New("Invalid gateway MAC")
	}
	gwMac = uint64(mac[0])<<40 | uint64(mac[1])<<32 | uint64(mac[2])<<24 | uint64(mac[3])<<16 | uint64(mac[4])<<8 | uint64(mac[5])

	if cmd.Flags().Changed("pf-mac") {
		mac, err = net.ParseMAC(ifPfMac)
		if err != nil {
			return errors.New("Invalid PF MAC")
		}
		pfMac = uint64(mac[0])<<40 | uint64(mac[1])<<32 | uint64(mac[2])<<24 | uint64(mac[3])<<16 | uint64(mac[4])<<8 | uint64(mac[5])
	} else {
		pfMac = 0
	}

	if cmd.Flags().Changed("overlay-mac") {
		mac, err = net.ParseMAC(ifOverlayMac)
		if err != nil {
			return errors.New("Invalid overlay MAC")
		}
		overlayMac = uint64(mac[0])<<40 | uint64(mac[1])<<32 | uint64(mac[2])<<24 | uint64(mac[3])<<16 | uint64(mac[4])<<8 | uint64(mac[5])
	} else {
		overlayMac = 0
	}

	sourceGwStr := strings.Split(ifSourceGw, "/")
	if len(strings.Split(sourceGwStr[0], ".")) != 4 {
		return errors.New("Invalid source gateway prefix specified")
	}

	fmt.Sscanf(sourceGwStr[1], "%d", &sourceGWPrefixLen)
	if sourceGWPrefixLen < 0 || sourceGWPrefixLen > 32 {
		return errors.New("Invalid source gateway prefix specified")
	}
	sourceGWPrefix = utils.IPAddrStrtoUint32(sourceGwStr[0])

	if ifIngressBw > 12500000 {
		return errors.New("Invalid ingress BW. Valid range is 0-12500000 KBytes/sec")
	}

	if ifEgressBw > 12500000 {
		return errors.New("Invalid egress BW. Valid range is 0-12500000 KBytes/sec")
	}

	mplsOut.Label = ifMplsOut
	mplsOut.Exp = 0
	mplsOut.Ttl = 64

	intfID = Hash(ifName)

	index := 0
	overlayArray := make([]*halproto.IPAddress, numOverlay)
	var overlayArrayIndex *halproto.IPAddress
	for index < numOverlay {
		overlayArrayIndex = &halproto.IPAddress{
			IpAf: halproto.IPAddressFamily_IP_AF_INET,
			V4OrV6: &halproto.IPAddress_V4Addr{
				V4Addr: overlayIP[index],
			},
		}
		overlayArray[index] = overlayArrayIndex
		index++
	}

	sourceGW := &halproto.Address{
		Address: &halproto.Address_Prefix{
			Prefix: &halproto.IPSubnet{
				Subnet: &halproto.IPSubnet_Ipv4Subnet{
					Ipv4Subnet: &halproto.IPPrefix{
						Address: &halproto.IPAddress{
							IpAf: halproto.IPAddressFamily_IP_AF_INET,
							V4OrV6: &halproto.IPAddress_V4Addr{
								V4Addr: sourceGWPrefix,
							},
						},
						PrefixLen: sourceGWPrefixLen,
					},
				},
			},
		},
	}

	mplsReq := &halproto.IfTunnelProprietaryMpls{
		SubstrateIp: &halproto.IPAddress{
			IpAf: halproto.IPAddressFamily_IP_AF_INET,
			V4OrV6: &halproto.IPAddress_V4Addr{
				V4Addr: substrateIP,
			},
		},
		OverlayIp: overlayArray,
		MplsIf:    mplsInArray,
		TunnelDestIp: &halproto.IPAddress{
			IpAf: halproto.IPAddressFamily_IP_AF_INET,
			V4OrV6: &halproto.IPAddress_V4Addr{
				V4Addr: tunnelDestIP,
			},
		},
		MplsTag:    &mplsOut,
		SourceGw:   sourceGW,
		IngressBw:  ifIngressBw,
		EgressBw:   ifEgressBw,
		GwMacDa:    gwMac,
		LifName:    ifName,
		OverlayMac: overlayMac,
		PfMac:      pfMac,
	}
	req := &halproto.InterfaceSpec{
		KeyOrHandle: &halproto.InterfaceKeyHandle{
			KeyOrHandle: &halproto.InterfaceKeyHandle_InterfaceId{
				InterfaceId: intfID,
			},
		},
		Type: halproto.IfType_IF_TYPE_TUNNEL,
		IfInfo: &halproto.InterfaceSpec_IfTunnelInfo{
			IfTunnelInfo: &halproto.IfTunnelInfo{
				EncapType: halproto.IfTunnelEncapType_IF_TUNNEL_ENCAP_TYPE_PROPRIETARY_MPLS,
				EncapInfo: &halproto.IfTunnelInfo_PropMplsInfo{
					PropMplsInfo: mplsReq,
				},
			},
		},
	}

	ifCreateReqMsg := &halproto.InterfaceRequestMsg{
		Request: []*halproto.InterfaceSpec{req},
	}

	// HAL call
	respMsg, err := client.InterfaceCreate(context.Background(), ifCreateReqMsg)
	if err != nil {
		return err
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			errStr := fmt.Sprintf("Operation failed with %v error", resp.ApiStatus)
			return errors.New(errStr)
		}
		fmt.Printf("Interface update succeeded. Interface ID is %d\n", intfID)
	}

	return nil
}

func ifShowCmdHandler(cmd *cobra.Command, args []string) {
	if cmd.Flags().Changed("yaml") {
		ifDetailShowCmdHandler(cmd, args)
		return
	}

	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewInterfaceClient(c)

	defer c.Close()

	if len(args) > 0 {
		if strings.Compare(args[0], "spec") != 0 {
			fmt.Printf("Invalid argument\n")
			return
		}
	}

	var req *halproto.InterfaceGetRequest
	if cmd.Flags().Changed("id") {
		// Get specific if
		req = &halproto.InterfaceGetRequest{
			KeyOrHandle: &halproto.InterfaceKeyHandle{
				KeyOrHandle: &halproto.InterfaceKeyHandle_InterfaceId{
					InterfaceId: ifID,
				},
			},
		}
	} else {
		// Get all ifs
		req = &halproto.InterfaceGetRequest{}
	}
	ifGetReqMsg := &halproto.InterfaceGetRequestMsg{
		Request: []*halproto.InterfaceGetRequest{req},
	}

	// HAL call
	respMsg, err := client.InterfaceGet(context.Background(), ifGetReqMsg)
	if err != nil {
		fmt.Printf("Getting if failed. %v\n", err)
		return
	}

	// Print Header
	ifShowHeader()

	// Print IFs
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
			continue
		}
		ifShowOneResp(resp)
	}
}

func ifGetAllStr() map[uint64]string {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewInterfaceClient(c)

	defer c.Close()

	var req *halproto.InterfaceGetRequest
	req = &halproto.InterfaceGetRequest{}

	ifGetReqMsg := &halproto.InterfaceGetRequestMsg{
		Request: []*halproto.InterfaceGetRequest{req},
	}

	var m map[uint64]string
	m = make(map[uint64]string)

	// HAL call
	respMsg, err := client.InterfaceGet(context.Background(), ifGetReqMsg)
	if err != nil {
		fmt.Printf("Getting if failed. %v\n", err)
		return m
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
			return m
		}

		id := resp.GetSpec().GetKeyOrHandle().GetInterfaceId()
		m[id] = ifRespToStr(resp)
	}

	return m
}

func ifGetStrFromID(ifID []uint64) (int, []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewInterfaceClient(c)

	defer c.Close()

	var ifStr []string

	index := 0
	reqArray := make([]*halproto.InterfaceGetRequest, len(ifID))
	for index < len(ifID) {
		var req halproto.InterfaceGetRequest
		req = halproto.InterfaceGetRequest{
			KeyOrHandle: &halproto.InterfaceKeyHandle{
				KeyOrHandle: &halproto.InterfaceKeyHandle_InterfaceId{
					InterfaceId: ifID[index],
				},
			},
		}
		reqArray[index] = &req
		index++
	}

	ifGetReqMsg := &halproto.InterfaceGetRequestMsg{
		Request: reqArray,
	}

	// HAL call
	respMsg, err := client.InterfaceGet(context.Background(), ifGetReqMsg)
	if err != nil {
		fmt.Printf("Getting if failed. %v\n", err)
		return -1, nil
	}

	index = 0
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
			return -1, nil
		}

		ifStr = append(ifStr, fmt.Sprintf("%s-%d",
			strings.ToLower(ifTypeToStr(resp.GetSpec().GetType())),
			ifID[index]))
	}

	return 0, ifStr
}

func ifShowStatusCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewInterfaceClient(c)
	defer c.Close()

	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	var req *halproto.InterfaceGetRequest
	if cmd.Flags().Changed("id") {
		// Get specific if
		req = &halproto.InterfaceGetRequest{
			KeyOrHandle: &halproto.InterfaceKeyHandle{
				KeyOrHandle: &halproto.InterfaceKeyHandle_InterfaceId{
					InterfaceId: ifID,
				},
			},
		}
	} else {
		// Get all ifs
		req = &halproto.InterfaceGetRequest{}
	}
	ifGetReqMsg := &halproto.InterfaceGetRequestMsg{
		Request: []*halproto.InterfaceGetRequest{req},
	}

	// HAL call
	respMsg, err := client.InterfaceGet(context.Background(), ifGetReqMsg)
	if err != nil {
		fmt.Printf("Getting if failed. %v\n", err)
		return
	}

	// Print Header
	ifShowStatusHeader()

	// Print IFs
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
			continue
		}
		ifShowStatusOneResp(resp)
	}
}

func handleIfDetailShowCmd(cmd *cobra.Command, ofile *os.File) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewInterfaceClient(c)

	defer c.Close()

	var req *halproto.InterfaceGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// Get specific if
		req = &halproto.InterfaceGetRequest{
			KeyOrHandle: &halproto.InterfaceKeyHandle{
				KeyOrHandle: &halproto.InterfaceKeyHandle_InterfaceId{
					InterfaceId: ifID,
				},
			},
		}
	} else {
		// Get all ifs
		req = &halproto.InterfaceGetRequest{}
	}
	ifGetReqMsg := &halproto.InterfaceGetRequestMsg{
		Request: []*halproto.InterfaceGetRequest{req},
	}

	// HAL call
	respMsg, err := client.InterfaceGet(context.Background(), ifGetReqMsg)
	if err != nil {
		fmt.Printf("Getting if failed. %v\n", err)
		return
	}

	// Print IFs
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", resp.ApiStatus)
			continue
		}
		respType := reflect.ValueOf(resp)
		b, _ := yaml.Marshal(respType.Interface())
		if ofile != nil {
			if _, err := ofile.WriteString(string(b) + "\n"); err != nil {
				fmt.Printf("Failed to write to file %s, err : %v\n",
					ofile.Name(), err)
			}
		} else {
			fmt.Println(string(b) + "\n")
			fmt.Println("---")
		}
	}
}

func ifDetailShowCmdHandler(cmd *cobra.Command, args []string) {
	if len(args) > 0 {
		fmt.Printf("Invalid argument\n")
		return
	}

	handleIfDetailShowCmd(cmd, nil)
}

func ifShowHeader() {
	fmt.Printf("\n")
	fmt.Printf("Id:    Interface ID         Handle: IF's handle\n")
	fmt.Printf("Ifype: Interface type\n")
	hdrLine := strings.Repeat("-", 42)
	fmt.Println(hdrLine)
	fmt.Printf("%-22s%-10s%-10s\n",
		"Id", "Handle", "IfType")
	fmt.Println(hdrLine)
}

func ifShowOneResp(resp *halproto.InterfaceGetResponse) {
	fmt.Printf("%-22d%-10d%-10s",
		resp.GetSpec().GetKeyOrHandle().GetInterfaceId(),
		resp.GetStatus().GetIfHandle(),
		ifTypeToStr(resp.GetSpec().GetType()))
	fmt.Printf("\n")
}

func ifRespToStr(resp *halproto.InterfaceGetResponse) string {
	switch resp.GetSpec().GetType() {
	case halproto.IfType_IF_TYPE_ENIC:
		// Get name from Lif
		lifID := resp.GetSpec().GetIfEnicInfo().GetLifKeyOrHandle().GetLifId()
		return lifIDGetName(lifID)
	case halproto.IfType_IF_TYPE_UPLINK:
		return fmt.Sprintf("Uplink-%d",
			resp.GetSpec().GetIfUplinkInfo().GetPortNum())
	case halproto.IfType_IF_TYPE_UPLINK_PC:
		return fmt.Sprintf("UplinkPC-%d",
			resp.GetSpec().GetKeyOrHandle().GetInterfaceId())
	case halproto.IfType_IF_TYPE_TUNNEL:
		return fmt.Sprintf("Tunnel-%d",
			resp.GetSpec().GetKeyOrHandle().GetInterfaceId())
	case halproto.IfType_IF_TYPE_CPU:
		return fmt.Sprintf("CPU-%d",
			resp.GetSpec().GetKeyOrHandle().GetInterfaceId())
	case halproto.IfType_IF_TYPE_APP_REDIR:
		return fmt.Sprintf("AppRedir-%d",
			resp.GetSpec().GetKeyOrHandle().GetInterfaceId())
	default:
		return "Invalid"
	}
}

func ifTypeToStr(ifType halproto.IfType) string {
	switch ifType {
	case halproto.IfType_IF_TYPE_ENIC:
		return "Enic"
	case halproto.IfType_IF_TYPE_UPLINK:
		return "Uplink"
	case halproto.IfType_IF_TYPE_UPLINK_PC:
		return "UplinkPC"
	case halproto.IfType_IF_TYPE_TUNNEL:
		return "Tunnel"
	case halproto.IfType_IF_TYPE_CPU:
		return "CPU"
	case halproto.IfType_IF_TYPE_APP_REDIR:
		return "AppRedir"
	default:
		return "Invalid"
	}
}

func ifShowStatusHeader() {
	fmt.Printf("\n")
	fmt.Printf("Handle:    Interface handle         Status:    Interface status \n")
	hdrLine := strings.Repeat("-", 13)
	fmt.Println(hdrLine)
	fmt.Printf("%-7s%-6s\n",
		"Handle", "Status")
	fmt.Println(hdrLine)
}

func ifShowStatusOneResp(resp *halproto.InterfaceGetResponse) {
	fmt.Printf("%-7d%-6s",
		resp.GetStatus().GetIfHandle(),
		strings.ToLower(strings.Replace(resp.GetStatus().GetIfStatus().String(), "IF_STATUS_", "", -1)))
	fmt.Printf("\n")
}

// Hash generates a 64bit hash for a string
func Hash(s string) uint64 {
	h := fnv.New64a()
	h.Write([]byte(s))
	return h.Sum64()
}
