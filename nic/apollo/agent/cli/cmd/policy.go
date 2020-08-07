//-----------------------------------------------------------------------------
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

// +build apulu apollo

package cmd

import (
	"bytes"
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
	policyID            string
	ruleID              string
	profileID           string
	tcpIdleTimeout      uint32
	udpIdleTimeout      uint32
	icmpIdleTimeout     uint32
	otherIdleTimeout    uint32
	tcpCnxnSetupTimeout uint32
	tcpHalfCloseTimeout uint32
	tcpCloseTimeout     uint32
	tcpDropTimeout      uint32
	udpDropTimeout      uint32
	icmpDropTimeout     uint32
	otherDropTimeout    uint32
)

var securityPolicyShowCmd = &cobra.Command{
	Use:   "security-policy",
	Short: "show security policy",
	Long:  "show security policy",
	Run:   securityPolicyShowCmdHandler,
}

var securityPolicyShowStatisticsCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show security-policy statistics",
	Long:  "show security-policy statistics",
	Run:   securityPolicyShowStatisticsCmdHandler,
}

var securityProfileShowCmd = &cobra.Command{
	Use:   "security-profile",
	Short: "show security profile",
	Long:  "show security profile",
	Run:   securityProfileShowCmdHandler,
}

var securityRuleShowCmd = &cobra.Command{
	Use:   "security-rule",
	Short: "show security rule",
	Long:  "show security rule",
	Run:   securityRuleShowCmdHandler,
}

var securityProfileUpdateCmd = &cobra.Command{
	Use:   "security-profile",
	Short: "update security profile",
	Long:  "update security profile",
	Run:   securityProfileUpdateCmdHandler,
}

func init() {
	showCmd.AddCommand(securityPolicyShowCmd)
	securityPolicyShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	securityPolicyShowCmd.Flags().Bool("summary", false,
		"Display number of objects")
	securityPolicyShowCmd.Flags().StringVarP(&policyID, "id", "i", "",
		"Specify Security Policy ID")

	securityPolicyShowCmd.AddCommand(securityPolicyShowStatisticsCmd)
	securityPolicyShowStatisticsCmd.Flags().StringVarP(&policyID, "id", "i", "",
		"Specify Security Policy ID")

	showCmd.AddCommand(securityProfileShowCmd)
	securityProfileShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	securityProfileShowCmd.Flags().Bool("summary", false, "Display number of objects")

	showCmd.AddCommand(securityRuleShowCmd)
	securityRuleShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	securityRuleShowCmd.Flags().StringVarP(&policyID, "policy-id", "p", "", "Specify policy ID")
	securityRuleShowCmd.Flags().StringVarP(&ruleID, "rule-id", "r", "", "Specify rule ID")

	debugUpdateCmd.AddCommand(securityProfileUpdateCmd)
	securityProfileUpdateCmd.Flags().StringVarP(&profileID, "profile-id", "p", "", "Specify profile ID")
	securityProfileUpdateCmd.Flags().Uint32Var(&tcpIdleTimeout, "tcp-idle-timeout", 600, "Specify TCP idle timeout (Valid: 5-86400)")
	securityProfileUpdateCmd.Flags().Uint32Var(&udpIdleTimeout, "udp-idle-timeout", 120, "Specify UDP idle timeout (Valid: 5-86400)")
	securityProfileUpdateCmd.Flags().Uint32Var(&icmpIdleTimeout, "icmp-idle-timeout", 15, "Specify ICMP idle timeout (Valid: 5-86400)")
	securityProfileUpdateCmd.Flags().Uint32Var(&otherIdleTimeout, "other-idle-timeout", 90, "Specify Other idle timeout (Valid: 30-86400)")
	securityProfileUpdateCmd.Flags().Uint32Var(&tcpCnxnSetupTimeout, "tcp-cnxn-setup-timeout", 10, "Specify TCP sonnection setup timeout (Valid: 1-60)")
	securityProfileUpdateCmd.Flags().Uint32Var(&tcpHalfCloseTimeout, "tcp-half-close-timeout", 120, "Specify TCP half close timeout (Valid: 1-172800)")
	securityProfileUpdateCmd.Flags().Uint32Var(&tcpCloseTimeout, "tcp-close-timeout", 15, "Specify TCP close timeout (Valid: 1-300)")
	securityProfileUpdateCmd.Flags().Uint32Var(&tcpDropTimeout, "tcp-drop-timeout", 90, "Specify TCP drop timeout (Valid: 1-300)")
	securityProfileUpdateCmd.Flags().Uint32Var(&udpDropTimeout, "udp-drop-timeout", 60, "Specify UDP drop timeout (Valid: 1-172800)")
	securityProfileUpdateCmd.Flags().Uint32Var(&icmpDropTimeout, "icmp-drop-timeout", 30, "Specify ICMP drop timeout (Valid: 1-300)")
	securityProfileUpdateCmd.Flags().Uint32Var(&otherDropTimeout, "other-drop-timeout", 60, "Specify Other drop timeout (Valid: 1-300)")
}

func securityPolicyShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewSecurityPolicySvcClient(c)

	var req *pds.SecurityPolicyGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// Get specific Rule/Policy
		req = &pds.SecurityPolicyGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(policyID).Bytes()},
		}
	} else {
		// Get all Rules/Policies
		req = &pds.SecurityPolicyGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.SecurityPolicyGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting policy failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print the rules/policies
	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		printPolicySummary(len(respMsg.Response))
	} else {
		printPolicyLegend()
		for _, resp := range respMsg.Response {
			printPolicy(resp)
		}
		printPolicySummary(len(respMsg.Response))
	}
}

func printPolicyLegend() {
	fmt.Printf("ICMP T/C : ICMP Type/Code\n\n")
}

func printPolicySummary(count int) {
	fmt.Printf("\nNo. of security policies : %d\n\n", count)
}

func printPolicyRuleHeader() {
	hdrLine := strings.Repeat("-", 203)
	fmt.Println(hdrLine)
	fmt.Printf("%-40s%-9s%-48s%-48s%-12s%-12s%-8s%-10s%-10s%-6s\n",
		"RuleID", "IPProto", "        Source", "     Destination",
		"SrcPort", "DestPort", "ICMP", "Priority", "Stateful", "Action")
	fmt.Printf("%-40s%-9s%-48s%-48s%-12s%-12s%-8s%-10s%-10s%-6s\n",
		"", "", "Prefix | Range | Tag", "Prefix | Range | Tag",
		"Range", "Range", "T/C", "", "", "")
	fmt.Println(hdrLine)
}

func printPolicy(resp *pds.SecurityPolicy) {
	spec := resp.GetSpec()

	fmt.Printf("%-18s : %-40s\n", "Policy ID",
		utils.IdToStr(spec.GetId()))
	fmt.Printf("%-18s : %-10s\n", "Address Family",
		utils.AddrFamilyToStr(spec.GetAddrFamily()))
	fmt.Printf("%-18s : %-20s\n", "Default FW Action",
		strings.Replace(spec.GetDefaultFWAction().String(), "SECURITY_RULE_ACTION_", "", -1))
	printPolicyRuleHeader()

	for _, rule := range spec.Rules {
		srcIPStr := "*"
		dstIPStr := "*"
		srcPortStr := "*"
		dstPortStr := "*"
		protoStr := "-"
		icmpStr := "-"
		actionStr := "-"
		needSecondLine := false

		l3Match := rule.GetAttrs().GetMatch().GetL3Match()
		switch l3Match.GetProtomatch().(type) {
		case *pds.RuleL3Match_ProtoNum:
			protoStr = fmt.Sprint(l3Match.GetProtoNum())
		case *pds.RuleL3Match_ProtoWildcard:
			protoStr = "*"
			srcPortStr = "-"
			dstPortStr = "-"
		}

		switch l3Match.GetSrcmatch().(type) {
		case *pds.RuleL3Match_SrcPrefix:
			srcIPStr = utils.IPPrefixToStr(l3Match.GetSrcPrefix())
		case *pds.RuleL3Match_SrcRange:
			srcIPStr = utils.IPRangeToStr(l3Match.GetSrcRange())
		case *pds.RuleL3Match_SrcTag:
			srcIPStr = fmt.Sprint(l3Match.GetSrcTag())
		}

		switch l3Match.GetDstmatch().(type) {
		case *pds.RuleL3Match_DstPrefix:
			dstIPStr = utils.IPPrefixToStr(l3Match.GetDstPrefix())
		case *pds.RuleL3Match_DstRange:
			dstIPStr = utils.IPRangeToStr(l3Match.GetDstRange())
		case *pds.RuleL3Match_DstTag:
			dstIPStr = fmt.Sprint(l3Match.GetDstTag())
		}

		l4Match := rule.GetAttrs().GetMatch().GetL4Match()
		switch l4Match.GetL4Info().(type) {
		case *pds.RuleL4Match_Ports:
			srcPortRange := l4Match.GetPorts().GetSrcPortRange()
			dstPortRange := l4Match.GetPorts().GetDstPortRange()
			srcPortStr = fmt.Sprintf("%d-%d",
				srcPortRange.GetPortLow(), srcPortRange.GetPortHigh())
			dstPortStr = fmt.Sprintf("%d-%d",
				dstPortRange.GetPortLow(), dstPortRange.GetPortHigh())
		case *pds.RuleL4Match_TypeCode:
			icmp := l4Match.GetTypeCode()
			typeStr := ""
			codeStr := ""
			switch icmp.GetTypematch().(type) {
			case *pds.ICMPMatch_TypeNum:
				typeStr = fmt.Sprint(icmp.GetTypeNum())
			case *pds.ICMPMatch_TypeWildcard:
				typeStr = "*"
			}
			switch icmp.GetCodematch().(type) {
			case *pds.ICMPMatch_CodeNum:
				codeStr = fmt.Sprint(icmp.GetCodeNum())
			case *pds.ICMPMatch_CodeWildcard:
				codeStr = "*"
			}
			icmpStr = fmt.Sprintf("%s/%s", typeStr, codeStr)
		}

		if len(srcIPStr) > 47 || len(dstIPStr) > 47 {
			needSecondLine = true
		}

		actionStr = rule.GetAttrs().GetAction().String()
		switch actionStr {
		case "SECURITY_RULE_ACTION_ALLOW":
			actionStr = "A"
		case "SECURITY_RULE_ACTION_DENY":
			actionStr = "D"
		default:
			actionStr = "-"
		}

		ruleIDStr := "-"
		if !bytes.Equal(rule.GetId(), make([]byte, len(rule.GetId()))) {
			ruleIDStr = utils.IdToStr(rule.GetId())
		}

		if needSecondLine {
			srcStrs := strings.Split(srcIPStr, "-")
			dstStrs := strings.Split(dstIPStr, "-")
			fmt.Printf("%-40s%-9s%-48s%-48s%-12s%-12s%-8s%-10d%-10s%-6s\n",
				ruleIDStr,
				protoStr, srcStrs[0]+"-", dstStrs[0]+"-",
				srcPortStr, dstPortStr, icmpStr,
				rule.GetAttrs().GetPriority(),
				utils.BoolToString(rule.GetAttrs().GetStateful()),
				actionStr)
			fmt.Printf("%-40s%-9s%-48s%-48s%-50s\n",
				"", "", srcStrs[1], dstStrs[1], "")
		} else {
			fmt.Printf("%-40s%-9s%-48s%-48s%-12s%-12s%-8s%-10d%-10s%-6s\n",
				ruleIDStr,
				protoStr, srcIPStr, dstIPStr, srcPortStr, dstPortStr,
				icmpStr, rule.GetAttrs().GetPriority(),
				utils.BoolToString(rule.GetAttrs().GetStateful()),
				actionStr)
		}
	}
	fmt.Println("")
}

func securityPolicyShowStatisticsCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewSecurityPolicySvcClient(c)
	var req *pds.SecurityPolicyGetRequest
	if cmd != nil && cmd.Flags().Changed("id") {
		// Get specific security policy
		req = &pds.SecurityPolicyGetRequest{
			Id: [][]byte{uuid.FromStringOrNil(policyID).Bytes()},
		}
	} else {
		// Get all security policies
		req = &pds.SecurityPolicyGetRequest{
			Id: [][]byte{},
		}
	}

	// PDS call
	respMsg, err := client.SecurityPolicyGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting Security Policy failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print security policy statistics
	for _, resp := range respMsg.Response {
		spec := resp.GetSpec()
		stats := resp.GetStats()
		if stats != nil {
			fmt.Printf("Policy ID : %s\n", utils.IdToStr(spec.GetId()))
			// print the header
			hdrLine := strings.Repeat("-", 58)
			fmt.Println(hdrLine)
			fmt.Printf("%-40s%-10s\n", "RuleID", "RuleHits")
			fmt.Println(hdrLine)
			for i, rule := range spec.Rules {
				fmt.Printf("%-40s%-10d\n", utils.IdToStr(rule.GetId()),
					stats.GetRuleStats()[i].GetNumRuleHit())
			}
		}
	}
}

func securityProfileShowCmdHandler(cmd *cobra.Command, args []string) {
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

	client := pds.NewSecurityPolicySvcClient(c)

	var req *pds.SecurityProfileGetRequest
	// Get profiles - its a singleton
	req = &pds.SecurityProfileGetRequest{
		Id: [][]byte{},
	}

	// PDS call
	respMsg, err := client.SecurityProfileGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting policy failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print the profiles
	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else if cmd != nil && cmd.Flags().Changed("summary") {
		printProfileSummary(len(respMsg.Response))
	} else {
		for _, resp := range respMsg.Response {
			printProfile(resp)
		}
		printProfileSummary(len(respMsg.Response))
	}
}

func printProfileSummary(count int) {
	fmt.Printf("\nNo. of security profiles : %d\n\n", count)
}

func printProfile(resp *pds.SecurityProfile) {
	spec := resp.GetSpec()
	if spec == nil {
		return
	}

	fmt.Printf("%-26s : %s\n", "ID",
		utils.IdToStr(spec.GetId()))

	fmt.Printf("%-26s : %t\n", "Connection Track Enable",
		spec.GetConnTrackEn())

	fmt.Printf("%-26s : %s\n", "Default FW Action",
		strings.Replace(spec.GetDefaultFWAction().String(),
			"SECURITY_RULE_ACTION_", "", -1))

	fmt.Printf("%-26s : %d\n", "TCP Idle Timeout",
		spec.GetTCPIdleTimeout())
	fmt.Printf("%-26s : %d\n", "UDP Idle Timeout",
		spec.GetUDPIdleTimeout())
	fmt.Printf("%-26s : %d\n", "ICMP Idle Timeout",
		spec.GetICMPIdleTimeout())
	fmt.Printf("%-26s : %d\n", "Other Idle Timeout",
		spec.GetOtherIdleTimeout())

	fmt.Printf("%-26s : %d\n", "TCP Conn. Setup Timeout",
		spec.GetTCPCnxnSetupTimeout())
	fmt.Printf("%-26s : %d\n", "TCP Half Close Timeout",
		spec.GetTCPHalfCloseTimeout())
	fmt.Printf("%-26s : %d\n", "TCP Close Timeout",
		spec.GetTCPCloseTimeout())

	fmt.Printf("%-26s : %d\n", "TCP Drop Timeout",
		spec.GetTCPDropTimeout())
	fmt.Printf("%-26s : %d\n", "UDP Drop Timeout",
		spec.GetUDPDropTimeout())
	fmt.Printf("%-26s : %d\n", "ICMP Drop Timeout",
		spec.GetICMPDropTimeout())
	fmt.Printf("%-26s : %d\n", "Other Drop Timeout",
		spec.GetOtherDropTimeout())
}

func securityRuleShowCmdHandler(cmd *cobra.Command, args []string) {
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

	if cmd != nil && (!cmd.Flags().Changed("policy-id") || !cmd.Flags().Changed("rule-id")) {
		fmt.Printf("Command arguments not provided, policy-id and rule-id are required for this CLI\n")
		return
	}

	client := pds.NewSecurityPolicySvcClient(c)

	// Get security-rule - returns a singleton object
	var req *pds.SecurityRuleGetRequest
	req = &pds.SecurityRuleGetRequest{
		Id: []*pds.SecurityPolicyRuleId{
			&pds.SecurityPolicyRuleId{
				Id:               uuid.FromStringOrNil(ruleID).Bytes(),
				SecurityPolicyId: uuid.FromStringOrNil(policyID).Bytes(),
			},
		},
	}

	// PDS call
	respMsg, err := client.SecurityRuleGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting security rule failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	// Print the rules
	if cmd != nil && cmd.Flags().Changed("yaml") {
		for _, resp := range respMsg.Response {
			respType := reflect.ValueOf(resp)
			b, _ := yaml.Marshal(respType.Interface())
			fmt.Println(string(b))
			fmt.Println("---")
		}
	} else {
		for _, resp := range respMsg.Response {
			printRule(resp)
		}
	}
}

func printRule(resp *pds.SecurityRule) {
	spec := resp.GetSpec()
	if spec == nil {
		return
	}

	attr := spec.GetAttrs()
	if attr == nil {
		return
	}

	fmt.Printf("%-30s : %s\n", "Rule ID", utils.IdToStr(spec.GetId()))
	fmt.Printf("%-30s : %s\n", "Policy ID", utils.IdToStr(spec.GetSecurityPolicyId()))
	fmt.Printf("%-30s : %t\n", "Stateful", attr.GetStateful())
	fmt.Printf("%-30s : %d\n\n", "Priority", attr.GetPriority())

	// L3 info:
	// Print Protocol (Protocol Number or Wildcard)
	// Print Source (Prefix or Range or Tag)
	// Print Destination (Prefix or Range or Tag)
	fmt.Printf("L3 Match:\n")
	l3Match := attr.GetMatch().GetL3Match()
	switch l3Match.GetProtomatch().(type) {
	case *pds.RuleL3Match_ProtoNum:
		fmt.Printf("%-30s : %d\n", "Protocol Number", l3Match.GetProtoNum())
	case *pds.RuleL3Match_ProtoWildcard:
		fmt.Printf("%-30s : %s\n", "Protocol Match",
			strings.Replace(l3Match.GetProtoWildcard().String(), "MATCH_", "", -1))
	default:
		fmt.Printf("%-30s : %s\n", "Protocol Number", "-")
	}

	switch l3Match.GetSrcmatch().(type) {
	case *pds.RuleL3Match_SrcPrefix:
		fmt.Printf("%-30s : %s\n", "Source Match Type", "Prefix")
		fmt.Printf("%-30s : %s\n", "Source Prefix", utils.IPPrefixToStr(l3Match.GetSrcPrefix()))
	case *pds.RuleL3Match_SrcRange:
		switch l3Match.GetSrcRange().GetRange().(type) {
		case *pds.AddressRange_IPv4Range:
			fmt.Printf("%-30s : %s\n", "Source Match Type", "Range")
			ipRange := l3Match.GetSrcRange().GetIPv4Range()
			fmt.Printf("%-30s : %s\n", "Source Range Low",
				utils.IPAddrToStr(ipRange.GetLow()))
			fmt.Printf("%-30s : %s\n", "Source Range High",
				utils.IPAddrToStr(ipRange.GetHigh()))
		case *pds.AddressRange_IPv6Range:
			fmt.Printf("%-30s : %s\n", "Source Match Type", "Range")
			ipRange := l3Match.GetSrcRange().GetIPv6Range()
			fmt.Printf("%-30s : %s\n", "Source Range Low",
				utils.IPAddrToStr(ipRange.GetLow()))
			fmt.Printf("%-30s : %s\n", "Source Range High",
				utils.IPAddrToStr(ipRange.GetHigh()))
		default:
			fmt.Printf("%-30s : %s\n", "Source Match Type", "Range")
			fmt.Printf("%-30s : %s\n", "Source Range Low", "-")
			fmt.Printf("%-30s : %s\n", "Source Range High", "-")
		}

	case *pds.RuleL3Match_SrcTag:
		fmt.Printf("%-30s : %s\n", "Source Match Type", "Tag")
		fmt.Printf("%-30s : %d\n", "Source Tag", l3Match.GetSrcTag())
	default:
		fmt.Printf("%-30s : %s\n", "Source Match Type", "-")
	}

	switch l3Match.GetDstmatch().(type) {
	case *pds.RuleL3Match_DstPrefix:
		fmt.Printf("%-30s : %s\n", "Destination Match Type", "Prefix")
		fmt.Printf("%-30s : %s\n", "Destination Prefix",
			utils.IPPrefixToStr(l3Match.GetDstPrefix()))
	case *pds.RuleL3Match_DstRange:
		switch l3Match.GetDstRange().GetRange().(type) {
		case *pds.AddressRange_IPv4Range:
			fmt.Printf("%-30s : %s\n", "Destination Match Type", "Range")
			ipRange := l3Match.GetDstRange().GetIPv4Range()
			fmt.Printf("%-30s : %s\n", "Destination Range Low",
				utils.IPAddrToStr(ipRange.GetLow()))
			fmt.Printf("%-30s : %s\n", "Destination Range High",
				utils.IPAddrToStr(ipRange.GetHigh()))
		case *pds.AddressRange_IPv6Range:
			fmt.Printf("%-30s : %s\n", "Destination Match Type", "Range")
			ipRange := l3Match.GetDstRange().GetIPv6Range()
			fmt.Printf("%-30s : %s\n", "Destination Range Low",
				utils.IPAddrToStr(ipRange.GetLow()))
			fmt.Printf("%-30s : %s\n", "Destination Range High",
				utils.IPAddrToStr(ipRange.GetHigh()))
		default:
			fmt.Printf("%-30s : %s\n", "Destination Match Type", "Range")
			fmt.Printf("%-30s : %s\n", "Destination Range Low", "-")
			fmt.Printf("%-30s : %s\n", "Destination Range High", "-")
		}
	case *pds.RuleL3Match_DstTag:
		fmt.Printf("%-30s : %s\n", "Destination Match Type", "Tag")
		fmt.Printf("%-30s : %d\n", "Destination Tag", l3Match.GetDstTag())
	default:
		fmt.Printf("%-30s : %s\n", "Destination Match Type", "-")
	}

	// L4 info:
	// Print Source Port: Low to High
	// Print Dest Port: Low to High
	// Print ICMP info
	fmt.Printf("\nL4 Match:\n")
	l4Match := attr.GetMatch().GetL4Match()
	switch l4Match.GetL4Info().(type) {
	case *pds.RuleL4Match_Ports:
		fmt.Printf("%-30s : %s\n", "Match Type", "Port")
		srcPortRange := l4Match.GetPorts().GetSrcPortRange()
		dstPortRange := l4Match.GetPorts().GetDstPortRange()
		fmt.Printf("%-30s : %d\n", "Source Port Low",
			srcPortRange.GetPortLow())
		fmt.Printf("%-30s : %d\n", "Source Port High",
			srcPortRange.GetPortHigh())
		fmt.Printf("%-30s : %d\n", "Destination Port Low",
			dstPortRange.GetPortLow())
		fmt.Printf("%-30s : %d\n", "Destination Port High",
			dstPortRange.GetPortHigh())
	case *pds.RuleL4Match_TypeCode:
		fmt.Printf("%-30s : %s\n", "Match Type", "ICMP")
		switch l4Match.GetTypeCode().GetTypematch().(type) {
		case *pds.ICMPMatch_TypeNum:
			fmt.Printf("%-30s : %d\n", "Type number",
				l4Match.GetTypeCode().GetTypeNum())
		case *pds.ICMPMatch_TypeWildcard:
			fmt.Printf("%-30s : %s\n", "Type",
				strings.Replace(l4Match.GetTypeCode().GetTypeWildcard().String(), "MATCH_", "", -1))
		default:
			fmt.Printf("%-30s : %s\n", "Type Number", "-")
		}

		switch l4Match.GetTypeCode().GetCodematch().(type) {
		case *pds.ICMPMatch_CodeNum:
			fmt.Printf("%-30s : %d\n", "Code number",
				l4Match.GetTypeCode().GetCodeNum())
		case *pds.ICMPMatch_CodeWildcard:
			fmt.Printf("%-30s : %s\n", "Code",
				strings.Replace(l4Match.GetTypeCode().GetCodeWildcard().String(), "MATCH_", "", -1))
		default:
		}
	default:
		fmt.Printf("%-30s : %s\n", "Match Type", "-")
	}

	fmt.Printf("\n%-30s : %s\n", "Security Rule Action",
		strings.Replace(attr.GetAction().String(), "SECURITY_RULE_ACTION_", "", -1))
}

func securityProfileUpdateCmdHandler(cmd *cobra.Command, args []string) {
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

	if !isValidTimeoutArgument(cmd) {
		return
	}

	client := pds.NewSecurityPolicySvcClient(c)

	var req *pds.SecurityProfileGetRequest
	// Get specific profile
	req = &pds.SecurityProfileGetRequest{
		Id: [][]byte{uuid.FromStringOrNil(profileID).Bytes()},
	}

	// PDS call
	respMsg, err := client.SecurityProfileGet(context.Background(), req)
	if err != nil {
		fmt.Printf("Getting policy failed, err %v\n", err)
		return
	}

	if respMsg.ApiStatus != pds.ApiStatus_API_STATUS_OK {
		fmt.Printf("Operation failed with %v error\n", respMsg.ApiStatus)
		return
	}

	for _, resp := range respMsg.Response {
		spec := resp.GetSpec()
		if spec == nil || utils.IdToStr(spec.GetId()) != profileID {
			continue
		}

		updateProfileSpec := spec
		updateRequired := false

		if cmd.Flags().Changed("tcp-idle-timeout") && spec.GetTCPIdleTimeout() != tcpIdleTimeout {
			updateRequired = true
			updateProfileSpec.TCPIdleTimeout = tcpIdleTimeout
		}
		if cmd.Flags().Changed("udp-idle-timeout") && spec.GetUDPIdleTimeout() != udpIdleTimeout {
			updateRequired = true
			updateProfileSpec.UDPIdleTimeout = udpIdleTimeout
		}
		if cmd.Flags().Changed("icmp-idle-timeout") && spec.GetICMPIdleTimeout() != icmpIdleTimeout {
			updateRequired = true
			updateProfileSpec.ICMPIdleTimeout = icmpIdleTimeout
		}
		if cmd.Flags().Changed("other-idle-timeout") && spec.GetOtherIdleTimeout() != otherIdleTimeout {
			updateRequired = true
			updateProfileSpec.OtherIdleTimeout = otherIdleTimeout
		}
		if cmd.Flags().Changed("tcp-cnxn-setup-timeout") && spec.GetTCPCnxnSetupTimeout() != tcpCnxnSetupTimeout {
			updateRequired = true
			updateProfileSpec.TCPCnxnSetupTimeout = tcpCnxnSetupTimeout
		}
		if cmd.Flags().Changed("tcp-half-close-timeout") && spec.GetTCPHalfCloseTimeout() != tcpHalfCloseTimeout {
			updateRequired = true
			updateProfileSpec.TCPHalfCloseTimeout = tcpHalfCloseTimeout
		}
		if cmd.Flags().Changed("tcp-close-timeout") && spec.GetTCPCloseTimeout() != tcpCloseTimeout {
			updateRequired = true
			updateProfileSpec.TCPCloseTimeout = tcpCloseTimeout
		}
		if cmd.Flags().Changed("tcp-drop-timeout") && spec.GetTCPDropTimeout() != tcpDropTimeout {
			updateRequired = true
			updateProfileSpec.TCPDropTimeout = tcpDropTimeout
		}
		if cmd.Flags().Changed("udp-drop-timeout") && spec.GetUDPDropTimeout() != udpDropTimeout {
			updateRequired = true
			updateProfileSpec.UDPDropTimeout = udpDropTimeout
		}
		if cmd.Flags().Changed("icmp-drop-timeout") && spec.GetICMPDropTimeout() != icmpDropTimeout {
			updateRequired = true
			updateProfileSpec.ICMPDropTimeout = icmpDropTimeout
		}
		if cmd.Flags().Changed("other-drop-timeout") && spec.GetOtherDropTimeout() != otherDropTimeout {
			updateRequired = true
			updateProfileSpec.OtherDropTimeout = otherDropTimeout
		}

		if !updateRequired {
			fmt.Printf("Security-profile update not required, values provided match with current\n")
			return
		}

		updateReq := &pds.SecurityProfileRequest{
			Request: []*pds.SecurityProfileSpec{
				updateProfileSpec,
			},
		}

		updateResp, updateErr := client.SecurityProfileUpdate(context.Background(), updateReq)
		if updateErr != nil {
			fmt.Printf("Security-profile update failed, err %v\n", updateErr)
			return
		}

		if updateResp.ApiStatus != pds.ApiStatus_API_STATUS_OK {
			fmt.Printf("Operation failed with %v error\n", updateResp.ApiStatus)
			return
		}

		fmt.Printf("Security-profile update succeeded\n")
		return
	}

	fmt.Printf("No matching profile id found\n")
}

func isValidTimeoutArgument(cmd *cobra.Command) bool {

	if cmd == nil {
		return false
	}

	if !cmd.Flags().Changed("profile-id") {
		fmt.Printf("Command arguments not provided correctly, profile-id and one of the timeout arguments are required for this CLI\n")
		return false
	}

	if !cmd.Flags().Changed("tcp-idle-timeout") &&
		!cmd.Flags().Changed("udp-idle-timeout") &&
		!cmd.Flags().Changed("icmp-idle-timeout") &&
		!cmd.Flags().Changed("other-idle-timeout") &&
		!cmd.Flags().Changed("tcp-cnxn-setup-timeout") &&
		!cmd.Flags().Changed("tcp-half-close-timeout") &&
		!cmd.Flags().Changed("tcp-close-timeout") &&
		!cmd.Flags().Changed("tcp-drop-timeout") &&
		!cmd.Flags().Changed("udp-drop-timeout") &&
		!cmd.Flags().Changed("icmp-drop-timeout") &&
		!cmd.Flags().Changed("other-drop-timeout") {
		fmt.Printf("No timeout arguments specified, refer to help string\n")
		return false
	}

	if cmd.Flags().Changed("tcp-idle-timeout") {
		if tcpIdleTimeout < 5 || tcpIdleTimeout > 86400 {
			fmt.Printf("Invalid range for tcp-idle-timeout, refer to help string\n")
			return false
		}
	}

	if cmd.Flags().Changed("udp-idle-timeout") {
		if udpIdleTimeout < 5 || udpIdleTimeout > 86400 {
			fmt.Printf("Invalid range for udp-idle-timeout, refer to help string\n")
			return false
		}
	}

	if cmd.Flags().Changed("icmp-idle-timeout") {
		if icmpIdleTimeout < 5 || icmpIdleTimeout > 86400 {
			fmt.Printf("Invalid range for icmp-idle-timeout, refer to help string\n")
			return false
		}
	}

	if cmd.Flags().Changed("other-idle-timeout") {
		if otherIdleTimeout < 30 || otherIdleTimeout > 86400 {
			fmt.Printf("Invalid range for other-idle-timeout, refer to help string\n")
			return false
		}
	}

	if cmd.Flags().Changed("tcp-cnxn-setup-timeout") {
		if tcpCnxnSetupTimeout < 1 || tcpCnxnSetupTimeout > 60 {
			fmt.Printf("Invalid range for tcp-cnxn-setup-timeout, refer to help string\n")
			return false
		}
	}

	if cmd.Flags().Changed("tcp-half-close-timeout") {
		if tcpHalfCloseTimeout < 1 || tcpHalfCloseTimeout > 172800 {
			fmt.Printf("Invalid range for tcp-half-close-timeout, refer to help string\n")
			return false
		}
	}

	if cmd.Flags().Changed("tcp-close-timeout") {
		if tcpCloseTimeout < 1 || tcpCloseTimeout > 300 {
			fmt.Printf("Invalid range for tcp-close-timeout, refer to help string\n")
			return false
		}
	}

	if cmd.Flags().Changed("tcp-drop-timeout") {
		if tcpDropTimeout < 1 || tcpDropTimeout > 300 {
			fmt.Printf("Invalid range for tcp-drop-timeout, refer to help string\n")
			return false
		}
	}

	if cmd.Flags().Changed("udp-drop-timeout") {
		if udpDropTimeout < 1 || udpDropTimeout > 172800 {
			fmt.Printf("Invalid range for udp-drop-timeout, refer to help string\n")
			return false
		}
	}

	if cmd.Flags().Changed("icmp-drop-timeout") {
		if icmpDropTimeout < 1 || icmpDropTimeout > 300 {
			fmt.Printf("Invalid range for icmp-drop-timeout, refer to help string\n")
			return false
		}
	}

	if cmd.Flags().Changed("other-drop-timeout") {
		if otherDropTimeout < 1 || otherDropTimeout > 300 {
			fmt.Printf("Invalid range for other-drop-timeout, refer to help string\n")
			return false
		}
	}

	return true
}
