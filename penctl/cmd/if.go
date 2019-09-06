//-----------------------------------------------------------------------------
// {C} Copyright 2018 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"errors"
	"fmt"
	"strings"

	"github.com/spf13/cobra"

	nmd "github.com/pensando/sw/nic/agent/protos/nmd"
)

var (
	ifEncap        string
	ifName         string
	ifSubIP        string
	ifOverlayIP    string
	ifMplsIn       string
	ifMplsOut      uint32
	ifTunnelDestIP string
	ifSourceGw     string
	ifGwMac        string
	ifPfMac        string
	ifOverlayMac   string
	ifIngressBw    uint32
	ifEgressBw     uint32
)

var ifUpdateCmd = &cobra.Command{
	Use:   "interface",
	Short: "Create interface",
	Long:  "Create interface",
	RunE:  ifUpdateCmdHandler,
}

var ifDeleteCmd = &cobra.Command{
	Use:   "interface",
	Short: "Delete interface",
	Long:  "Delete interface",
	RunE:  ifDeleteCmdHandler,
}

var ifShowCmd = &cobra.Command{
	Use:   "interface",
	Short: "Show interface",
	Long:  "Show interface",
	Run:   ifShowCmdHandler,
}

var tunnelShowCmd = &cobra.Command{
	Use:   "tunnel",
	Short: "Show tunnel interface",
	Long:  "Show tunnel interface",
	Run:   ifTunnelShowCmdHandler,
}

var mplsoudpShowCmd = &cobra.Command{
	Use:   "mplsoudp",
	Short: "Show MPLSoUDP tunnel interface",
	Long:  "Show MPLSoUDP tunnel interface",
	Run:   mplsoudpShowCmdHandler,
}

func init() {
	showCmd.AddCommand(ifShowCmd)
	ifShowCmd.AddCommand(tunnelShowCmd)
	tunnelShowCmd.AddCommand(mplsoudpShowCmd)

	deleteCmd.AddCommand(ifDeleteCmd)
	ifDeleteCmd.Flags().StringVar(&ifEncap, "encap", "", "Encap type (Ex: MPLSoUDP)")
	ifDeleteCmd.Flags().StringVar(&ifName, "name", "", "Interface name")
	ifDeleteCmd.MarkFlagRequired("encap")
	ifDeleteCmd.MarkFlagRequired("name")

	updateCmd.AddCommand(ifUpdateCmd)

	ifUpdateCmd.Flags().StringVar(&ifEncap, "encap", "", "Encap type (Ex: MPLSoUDP)")
	ifUpdateCmd.Flags().StringVar(&ifName, "name", "", "Interface name")
	ifUpdateCmd.Flags().StringVar(&ifSubIP, "substrate-ip", "", "Substrate IPv4 address")
	ifUpdateCmd.Flags().StringVar(&ifOverlayIP, "overlay-ip", "", "Specify overlay IPv4 address in comma separated list (Max of 2 supported). Ex: 1.2.3.4,2.3.4.5")
	ifUpdateCmd.Flags().StringVar(&ifMplsIn, "mpls-in", "", "Specify incoming MPLS label as comma separated list (Max of 2 supported)")
	ifUpdateCmd.Flags().Uint32Var(&ifMplsOut, "mpls-out", 0, "Specify outgoing MPLS label")
	ifUpdateCmd.Flags().StringVar(&ifTunnelDestIP, "tunnel-dest-ip", "", "Tunnel destination IPv4 address")
	ifUpdateCmd.Flags().StringVar(&ifSourceGw, "source-gw", "", "Specify source gateway. Must be IPv4 prefix as a.b.c.d/nn")
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

func handleIfShowCmd(intf bool, tunnel bool, mpls bool) {
	halctlStr := ""
	if intf == true {
		halctlStr = "halctl show interface"
	} else if tunnel == true {
		halctlStr = "halctl show interface tunnel"
	} else if mpls == true {
		halctlStr = "halctl show interface tunnel mplsoudp"
	}

	v := &nmd.NaplesCmdExecute{
		Executable: strings.Replace(halctlStr, " ", "", -1),
		Opts:       strings.Join([]string{""}, ""),
	}

	naplesExecCmd(v)
}

func ifShowCmdHandler(cmd *cobra.Command, args []string) {
	handleIfShowCmd(true, false, false)
}

func ifTunnelShowCmdHandler(cmd *cobra.Command, args []string) {
	handleIfShowCmd(false, true, false)
}

func mplsoudpShowCmdHandler(cmd *cobra.Command, args []string) {
	handleIfShowCmd(false, false, true)
}

func ifDeleteCmdHandler(cmd *cobra.Command, args []string) error {
	halctlStr := "halctl debug delete interface --encap " + ifEncap + " --name " + ifName

	execCmd := strings.Fields(halctlStr)
	v := &nmd.NaplesCmdExecute{
		Executable: "halctldebugdeleteinterface",
		Opts:       strings.Join(execCmd[4:], " "),
	}

	if err := naplesExecCmd(v); err != nil {
		str := err.Error()
		str = strings.Replace(str, "exit status 1:Error: ", "", -1)
		str = strings.Replace(str, "\\n", "", -1)
		str = strings.Replace(str, "\n", "", -1)
		str = strings.Replace(str, "\"", "", -1)
		return errors.New(str)
	}

	return nil
}

func ifUpdateCmdHandler(cmd *cobra.Command, args []string) error {
	if ifIngressBw > 12500000 {
		return errors.New("Invalid ingress BW. Valid range is 0-12500000 KBytes/sec")
	}

	if ifEgressBw > 12500000 {
		return errors.New("Invalid egress BW. Valid range is 0-12500000 KBytes/sec")
	}

	halctlStr := "halctl debug update interface --encap " + ifEncap + " --substrate-ip " + ifSubIP + " --overlay-ip " + ifOverlayIP + " --mpls-in " + ifMplsIn + " --mpls-out " + fmt.Sprint(ifMplsOut) + " --tunnel-dest-ip " + ifTunnelDestIP + " --source-gw " + ifSourceGw + " --gw-mac " + ifGwMac + " --ingress-bw " + fmt.Sprint(ifIngressBw) + " --egress-bw " + fmt.Sprint(ifEgressBw) + " --name " + ifName

	if cmd.Flags().Changed("overlay-mac") {
		halctlStr += " --overlay-mac " + ifOverlayMac
	}

	if cmd.Flags().Changed("pf-mac") {
		halctlStr += " --pf-mac " + ifPfMac
	}

	execCmd := strings.Fields(halctlStr)
	v := &nmd.NaplesCmdExecute{
		Executable: "halctldebugupdateinterface",
		Opts:       strings.Join(execCmd[4:], " "),
	}
	if err := naplesExecCmd(v); err != nil {
		str := err.Error()
		str = strings.Replace(str, "exit status 1:Error: ", "", -1)
		str = strings.Replace(str, "\\n", "", -1)
		str = strings.Replace(str, "\n", "", -1)
		str = strings.Replace(str, "\"", "", -1)
		return errors.New(str)
	}

	return nil
}
