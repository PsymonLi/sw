//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"container/list"
	"context"
	"fmt"
	// "math/rand"
	"os"
	"reflect"
	"strings"

	"github.com/spf13/cobra"
	yaml "gopkg.in/yaml.v2"

	"github.com/pensando/sw/nic/agent/cmd/halctl/utils"
	"github.com/pensando/sw/nic/agent/netagent/datapath/halproto"
	"github.com/pensando/sw/venice/utils/rpckit"
)

var systemShowCmd = &cobra.Command{
	Use:   "system",
	Short: "show system information",
	Long:  "show system information",
	Run:   systemDetailShowCmdHandler,
}

var systemClockShowCmd = &cobra.Command{
	Use:   "clock",
	Short: "show system clock Information",
	Long:  "show system clock Information",
	Run:   systemClockShowCmdHandler,
}

var systemStatsShowCmd = &cobra.Command{
	Use:   "statistics",
	Short: "show system statistics [fte | fte-txrx | table | api | pb | intf | all] (Default: all)",
	Long:  "show system statistics [fte | fte-txrx | table | api | pb | intf | all] (Default: all)",
	Run:   systemStatsShowCmdHandler,
}

var systemDropStatsShowCmd = &cobra.Command{
	Use:   "drop",
	Short: "show system statistics drop [ingress | egress | pb | all] (Default: all)",
	Long:  "show system statistics drop [ingress | egress | pb | all] (Default: all)",
	Run:   systemDropStatsShowCmdHandler,
}

var systemStatsTableShowCmd = &cobra.Command{
	Use:   "table",
	Short: "show system statistics table",
	Long:  "show system statistics table",
	Run:   tableInfoShowCmdHandler,
}

var threadShowCmd = &cobra.Command{
	Use:   "thread",
	Short: "show system threads",
	Long:  "show system threads",
	Run:   threadShowCmdHandler,
}

var queueStatsCmd = &cobra.Command{
	Use:   "queue-statistics",
	Short: "show system queue-statistics",
	Long:  "show system queue-statistics",
	Run:   systemQueueStatsCmdHandler,
}

var inputQueueStatsCmd = &cobra.Command{
	Use:   "input",
	Short: "show system queue-statistics input [buffer-occupancy | peak-occupancy]",
	Long:  "show system queue-statistics input [buffer-occupancy | peak-occupancy]",
	Run:   systemInputQueueStatsCmdHandler,
}

var outputQueueStatsCmd = &cobra.Command{
	Use:   "output",
	Short: "show system queue-statistics output [queue-depth]",
	Long:  "show system queue-statistics output [queue-depth]",
	Run:   systemOutputQueueStatsCmdHandler,
}

func init() {
	showCmd.AddCommand(systemShowCmd)
	systemShowCmd.AddCommand(systemStatsShowCmd)
	systemShowCmd.AddCommand(threadShowCmd)
	systemShowCmd.AddCommand(systemClockShowCmd)
	systemShowCmd.AddCommand(queueStatsCmd)
	queueStatsCmd.AddCommand(inputQueueStatsCmd)
	queueStatsCmd.AddCommand(outputQueueStatsCmd)
	systemStatsShowCmd.AddCommand(systemStatsTableShowCmd)
	systemStatsShowCmd.AddCommand(systemDropStatsShowCmd)

	systemShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	threadShowCmd.Flags().Bool("yaml", false, "Output in yaml")
	queueStatsCmd.PersistentFlags().Uint32Var(&portNum, "port", 0, "Port number")
}

func handleSystemQueueStatsCmd(cmd *cobra.Command, args []string, inputQueue bool, outputQueue bool) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	defer c.Close()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewSystemClient(c.ClientConn)

	portSet := false
	if cmd.Flags().Changed("port") {
		portSet = true
	}

	// HAL call
	var empty *halproto.Empty
	resp, err := client.SystemGet(context.Background(), empty)
	if err != nil {
		fmt.Printf("Getting System Stats failed. %v\n", err)
		return
	}

	if resp.GetApiStatus() != halproto.ApiStatus_API_STATUS_OK {
		fmt.Printf("HAL Returned non OK status. %v\n", resp.GetApiStatus())
		return
	}

	bufferOccupancy := true
	peakOccupancy := true
	queueDepth := true

	if len(args) > 0 {
		if strings.Compare(args[0], "buffer-occupancy") == 0 {
			peakOccupancy = false
			queueDepth = false
		} else if strings.Compare(args[0], "peak-occupancy") == 0 {
			bufferOccupancy = false
			queueDepth = false
		} else if strings.Compare(args[0], "queue-depth") == 0 {
			bufferOccupancy = false
			peakOccupancy = false
		} else {
			fmt.Printf("Invalid argument\n")
			return
		}
	} else {
		if inputQueue == false {
			bufferOccupancy = false
			peakOccupancy = false
		}
		if outputQueue == false {
			queueDepth = false
		}
	}

	systemQueueStatsPrint(resp, portSet, bufferOccupancy, peakOccupancy, queueDepth)
}

func systemQueueStatsCmdHandler(cmd *cobra.Command, args []string) {
	handleSystemQueueStatsCmd(cmd, args, true, true)
}

func systemInputQueueStatsCmdHandler(cmd *cobra.Command, args []string) {
	handleSystemQueueStatsCmd(cmd, args, true, false)
}

func systemOutputQueueStatsCmdHandler(cmd *cobra.Command, args []string) {
	handleSystemQueueStatsCmd(cmd, args, false, true)
}

func systemQueueStatsPrint(resp *halproto.SystemResponse, portSet bool, bufferOccupancy bool, peakOccupancy bool, queueDepth bool) {
	portStats := resp.GetStats().GetPacketBufferStats().GetPortStats()
	var str string

	if bufferOccupancy == true {
		fmt.Printf("Buffer Occupancy:\n")
		qosQueueStatsHeaderPrint()
		for _, port := range portStats {
			if portSet == true {
				if port.GetPacketBufferPort().GetPortNum() != portNum {
					continue
				}
			}
			qVal := [16]uint32{}
			qVal2 := [16]uint32{}
			if port.GetPacketBufferPort().GetPortType() == 3 {
				fmt.Printf("%-5d|", port.GetPacketBufferPort().GetPortNum())
			} else {
				fmt.Printf("%-5s|", strings.Replace(port.GetPacketBufferPort().GetPortType().String(), "PACKET_BUFFER_PORT_TYPE_", "", -1))
			}
			for _, input := range port.GetQosQueueStats().GetInputQueueStats() {
				qIndex := input.GetInputQueueIdx()
				if qIndex < 16 {
					qVal[qIndex] = input.GetBufferOccupancy()
				} else {
					qVal2[qIndex-16] = input.GetBufferOccupancy()
				}
			}
			str = fmt.Sprintf("%-6v\n", qVal)
			str = strings.Replace(str, "[", "", -1)
			str = strings.Replace(str, "]", "", -1)
			fmt.Printf("%s\n", str)
			fmt.Printf("     |")
			str = fmt.Sprintf("%-6v\n", qVal2)
			str = strings.Replace(str, "[", "", -1)
			str = strings.Replace(str, "]", "", -1)
			fmt.Printf("%s\n", str)
		}
	}

	if peakOccupancy == true {
		fmt.Printf("Peak Occupancy:\n")
		qosQueueStatsHeaderPrint()
		for _, port := range portStats {
			if portSet == true {
				if port.GetPacketBufferPort().GetPortNum() != portNum {
					continue
				}
			}
			qVal := [16]uint32{}
			qVal2 := [16]uint32{}
			if port.GetPacketBufferPort().GetPortType() == 3 {
				fmt.Printf("%-5d|", port.GetPacketBufferPort().GetPortNum())
			} else {
				fmt.Printf("%-5s|", strings.Replace(port.GetPacketBufferPort().GetPortType().String(), "PACKET_BUFFER_PORT_TYPE_", "", -1))
			}
			for _, input := range port.GetQosQueueStats().GetInputQueueStats() {
				qIndex := input.GetInputQueueIdx()
				if qIndex < 16 {
					qVal[qIndex] = input.GetPeakOccupancy()
				} else {
					qVal2[qIndex-16] = input.GetPeakOccupancy()
				}
			}
			str = fmt.Sprintf("%-6v\n", qVal)
			str = strings.Replace(str, "[", "", -1)
			str = strings.Replace(str, "]", "", -1)
			fmt.Printf("%s\n", str)
			fmt.Printf("     |")
			str = fmt.Sprintf("%-6v\n", qVal2)
			str = strings.Replace(str, "[", "", -1)
			str = strings.Replace(str, "]", "", -1)
			fmt.Printf("%s\n", str)
		}
	}

	if queueDepth == true {
		fmt.Printf("Queue Depth:\n")
		qosQueueStatsHeaderPrint()
		for _, port := range portStats {
			if portSet == true {
				if port.GetPacketBufferPort().GetPortNum() != portNum {
					continue
				}
			}
			qVal := [16]uint32{}
			qVal2 := [16]uint32{}
			if port.GetPacketBufferPort().GetPortType() == 3 {
				fmt.Printf("%-5d|", port.GetPacketBufferPort().GetPortNum())
			} else {
				fmt.Printf("%-5s|", strings.Replace(port.GetPacketBufferPort().GetPortType().String(), "PACKET_BUFFER_PORT_TYPE_", "", -1))
			}
			for _, output := range port.GetQosQueueStats().GetOutputQueueStats() {
				qIndex := output.GetOutputQueueIdx()
				if qIndex < 16 {
					qVal[qIndex] = output.GetQueueDepth()
				} else {
					qVal2[qIndex-16] = output.GetQueueDepth()
				}
			}
			str = fmt.Sprintf("%-6v\n", qVal)
			str = strings.Replace(str, "[", "", -1)
			str = strings.Replace(str, "]", "", -1)
			fmt.Printf("%s\n", str)
			fmt.Printf("     |")
			str = fmt.Sprintf("%-6v\n", qVal2)
			str = strings.Replace(str, "[", "", -1)
			str = strings.Replace(str, "]", "", -1)
			fmt.Printf("%s\n", str)
		}
	}
}

func handleSystemDetailShowCmd(cmd *cobra.Command, ofile *os.File) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	defer c.Close()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewSystemClient(c.ClientConn)

	// HAL call
	var empty *halproto.Empty
	resp, err := client.SystemGet(context.Background(), empty)
	if err != nil {
		fmt.Printf("Getting System Stats failed. %v\n", err)
		return
	}

	if resp.GetApiStatus() != halproto.ApiStatus_API_STATUS_OK {
		fmt.Printf("HAL Returned non OK status. %v\n", resp.GetApiStatus())
		return
	}

	// Print Response
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

func systemDetailShowCmdHandler(cmd *cobra.Command, args []string) {
	if cmd.Flags().Changed("yaml") {
		handleSystemDetailShowCmd(cmd, nil)
	} else {
		fmt.Printf("Only --yaml option supported\n")
	}
}

func systemDropStatsShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	defer c.Close()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewSystemClient(c.ClientConn)

	// Check the args
	ingressDrop := false
	egressDrop := false
	pbDrop := false

	if len(args) > 0 {
		if strings.Compare(args[0], "ingress") == 0 {
			ingressDrop = true
		} else if strings.Compare(args[0], "egress") == 0 {
			egressDrop = true
		} else if strings.Compare(args[0], "pb") == 0 {
			pbDrop = true
		} else if strings.Compare(args[0], "all") == 0 {
			ingressDrop = true
			egressDrop = true
			pbDrop = true
		} else {
			fmt.Printf("Invalid argument\n")
			return
		}
	} else {
		ingressDrop = true
		egressDrop = true
		pbDrop = true
	}

	// HAL call
	var empty *halproto.Empty
	resp, err := client.SystemGet(context.Background(), empty)
	if err != nil {
		fmt.Printf("Getting System Stats failed. %v\n", err)
		return
	}

	if resp.GetApiStatus() != halproto.ApiStatus_API_STATUS_OK {
		fmt.Printf("HAL Returned non OK status. %v\n", resp.GetApiStatus())
		return
	}

	if ingressDrop {
		// Print Header
		fmt.Println("\nSystem Drop Stats:")
		systemDropStatsShowHeader()

		// Print System Drop Stats
		for _, entry := range resp.GetStats().GetDropStats().DropEntries {
			systemDropStatsShowEntry(entry)
		}
	}

	if egressDrop {
		// Print Header
		fmt.Println("\nSystem Egress Drop Stats:")
		systemDropStatsShowHeader()

		// Print System Egress Drop Stats
		for _, entry := range resp.GetStats().GetEgressDropStats().DropEntries {
			systemEgressDropStatsShowEntry(entry)
		}
	}

	if pbDrop {
		// Print Header
		fmt.Println("\nSystem Packet Buffer Drop Stats:")
		systemPbDropStatsShowHeader()

		// Print System Packet Buffer Drop Stats
		for _, portEntry := range resp.GetStats().GetPacketBufferStats().GetPortStats() {
			systemPbDropStatsShowPortEntry(portEntry)
		}
	}
}

func systemPbDropStatsShowHeader() {
	hdrLine := strings.Repeat("-", 50)
	fmt.Println(hdrLine)
	fmt.Printf("%-8s%-9s%-26s%-5s\n",
		"PortNum", "PortType", "Reason", "Count")
	fmt.Println(hdrLine)
}

func systemPbDropStatsShowPortEntry(entry *halproto.PacketBufferPortStats) {
	portType := strings.ToLower(strings.Replace(entry.GetPacketBufferPort().GetPortType().String(),
		"PACKET_BUFFER_PORT_TYPE_", "", -1))
	portNum := entry.GetPacketBufferPort().GetPortNum()

	for _, dropStatsEntry := range entry.GetBufferStats().GetDropCounts().GetStatsEntries() {
		fmt.Printf("%-8d%-9s%-30s%-5d\n",
			portNum, portType,
			strings.ToLower(strings.Replace(dropStatsEntry.GetReasons().String(), "_", " ", -1)),
			dropStatsEntry.GetDropCount())
	}

	for _, dropStatsEntry := range entry.GetOflowFifoStats().GetDropCounts().GetEntry() {
		fmt.Printf("%-8d%-9s%-30s%-5d\n",
			portNum, portType,
			strings.ToLower(strings.Replace(dropStatsEntry.GetType().String(), "_", " ", -1)),
			dropStatsEntry.GetCount())
	}
}

func systemStatsShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	defer c.Close()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}

	// Check the args
	table := false
	fte := false
	api := false
	pb := false
	intf := false
	fteTxRx := false

	if len(args) > 0 {
		if strings.Compare(args[0], "table") == 0 {
			table = true
		} else if strings.Compare(args[0], "fte") == 0 {
			fte = true
		} else if strings.Compare(args[0], "api") == 0 {
			api = true
		} else if strings.Compare(args[0], "pb") == 0 {
			pb = true
		} else if strings.Compare(args[0], "intf") == 0 {
			intf = true
		} else if strings.Compare(args[0], "fte-txrx") == 0 {
			fteTxRx = true
		} else if strings.Compare(args[0], "all") == 0 {
			table = true
			fte = true
			api = true
			pb = true
			intf = true
			fteTxRx = true
		} else {
			fmt.Printf("Invalid argument\n")
			return
		}
	} else {
		table = true
		fte = true
		api = true
		pb = true
		intf = true
		fteTxRx = true
	}

	if intf {
		intfStatsShow(c)
	}

	client := halproto.NewSystemClient(c.ClientConn)

	// HAL call
	var empty *halproto.Empty
	resp, err := client.SystemGet(context.Background(), empty)
	if err != nil {
		fmt.Printf("Getting System Stats failed. %v\n", err)
		return
	}

	if resp.GetApiStatus() != halproto.ApiStatus_API_STATUS_OK {
		fmt.Printf("HAL Returned non OK status. %v\n", resp.GetApiStatus())
		return
	}

	if table {
		// Print Header
		fmt.Println("\nSystem Table Stats:")
		systemTableStatsShowHeader()

		// Print Table Stats
		for _, entry := range resp.GetStats().GetTableStats().TableStats {
			systemTableStatsShowEntry(entry)
		}
	}

	if api {
		respMsg, err := client.ApiStatsGet(context.Background(), empty)
		if err != nil {
			fmt.Printf("Getting API Stats failed. %v\n", err)
			return
		}

		// Print Header
		apiStatsShowHeader()

		// Print API Stats
		for _, entry := range respMsg.ApiEntries {
			apiStatsEntryShow(entry)
		}
	}

	if fte {
		fmt.Println("\nFTE Stats:")
		fteStatsShow(resp.GetStats())

		fmt.Println("\nSession Summary Stats:")
		sessionSummaryStatsShow(resp.GetStats())
	}

	if fteTxRx {
		//fmt.Println("\nFTE TxRx Stats: Disabled")
		fteTxRxStatsShow(resp.GetStats())
	}

	if pb {
		var dmaIn uint32
		var dmaOut uint32
		var ingIn uint32
		var ingOut uint32
		var egrIn uint32
		var egrOut uint32
		uplinkIn := []uint32{0, 0, 0, 0, 0, 0, 0, 0, 0}
		uplinkOut := []uint32{0, 0, 0, 0, 0, 0, 0, 0, 0}

		for _, entry := range resp.GetStats().GetPacketBufferStats().PortStats {
			if entry.GetPacketBufferPort().GetPortType() == halproto.PacketBufferPortType_PACKET_BUFFER_PORT_TYPE_DMA {
				dmaIn = entry.GetBufferStats().GetSopCountIn()
				dmaOut = entry.GetBufferStats().GetSopCountOut()
			} else if entry.GetPacketBufferPort().GetPortType() == halproto.PacketBufferPortType_PACKET_BUFFER_PORT_TYPE_P4IG {
				ingIn = entry.GetBufferStats().GetSopCountIn()
				ingOut = entry.GetBufferStats().GetSopCountOut()
			} else if entry.GetPacketBufferPort().GetPortType() == halproto.PacketBufferPortType_PACKET_BUFFER_PORT_TYPE_P4EG {
				egrIn = entry.GetBufferStats().GetSopCountIn()
				egrOut = entry.GetBufferStats().GetSopCountOut()
			} else if entry.GetPacketBufferPort().GetPortType() == halproto.PacketBufferPortType_PACKET_BUFFER_PORT_TYPE_UPLINK {
				uplinkIn[entry.GetPacketBufferPort().GetPortNum()] = entry.GetBufferStats().GetSopCountIn()
				uplinkOut[entry.GetPacketBufferPort().GetPortNum()] = entry.GetBufferStats().GetSopCountOut()
			}
		}
		pbStatsShow(dmaIn, dmaOut,
			ingIn, ingOut,
			egrIn, egrOut,
			uplinkIn, uplinkOut)
	}
}

func systemClockShowCmdHandler(cmd *cobra.Command, args []string) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	defer c.Close()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewDebugClient(c.ClientConn)

	var empty *halproto.Empty

	// HAL call
	resp, err := client.ClockGet(context.Background(), empty)
	if err != nil {
		fmt.Printf("Clock get failed. %v\n", err)
		return
	}

	if resp.GetApiStatus() != halproto.ApiStatus_API_STATUS_OK {
		fmt.Printf("HAL Returned non OK status. %v\n", resp.GetApiStatus())
		return
	}

	spec := resp.GetSpec()
	fmt.Println("\nSystem Clock Information:")
	fmt.Printf("\n%s%-15d\n", "Hardware Clock (in nanoseconds)    :", spec.GetHardwareClock())
	fmt.Printf("%s%-15d\n", "Software Delta                     :", spec.GetSoftwareDelta())
	fmt.Printf("%s%-15d\n", "Software Clock (in nanoseconds)    :", spec.GetSoftwareClock())
}

func leftArrow() uint32 {
	return 8592
	//return 11013
}

func rightArrow() uint32 {
	return 8594
	// return 10145
}

func upArrow() uint32 {
	return 8595
	// return 11014
}

func downArrow() uint32 {
	return 8593
	// return 11015
}

func pbStatsShow(dmaIn uint32, dmaOut uint32,
	ingIn uint32, ingOut uint32,
	egrIn uint32, egrOut uint32,
	portIn []uint32, portOut []uint32) {

	// Randomized values
	// dmaIn = uint32(rand.Intn(65535))
	// dmaOut = uint32(rand.Intn(65535))
	// ingIn = uint32(rand.Intn(65535))
	// ingOut = uint32(rand.Intn(65535))
	// egrIn = uint32(rand.Intn(65535))
	// egrOut = uint32(rand.Intn(65535))
	// for i := 0; i < 9; i++ {
	// 	portIn[i] = uint32(rand.Intn(65535))
	// 	portOut[i] = uint32(rand.Intn(65535))
	// }

	fmt.Println()

	fmt.Println("Packet Buffer (PB) Stats:")

	// P4+ block
	fmt.Print(" ")
	fmt.Print(strings.Repeat(" ", 5))
	fmt.Println(strings.Repeat("-", 129))
	// 2nd line
	fmt.Print(strings.Repeat(" ", 5))
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 28))
	fmt.Print("TX-DMA")
	fmt.Print(strings.Repeat(" ", 29))
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 28))
	fmt.Print("RX-DMA")
	fmt.Print(strings.Repeat(" ", 28))
	fmt.Printf("%c\n", 65372) // Vert. bar

	// 3rd line
	fmt.Print(" ")
	fmt.Print(strings.Repeat("-", 4))
	fmt.Print(" ")
	fmt.Print(strings.Repeat("-", 129))
	fmt.Print(" ")
	fmt.Println(strings.Repeat("-", 4))

	// 4th line
	printDMAStats(dmaIn, dmaOut)
	// 5th line
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 3))
	blankSpaces(127)
	fmt.Print(strings.Repeat(" ", 3))
	fmt.Printf("%c\n", 65372) // Vert. bar
	// 6th line
	printP4OutStats(ingOut, egrOut)
	// 7th line
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(" 4 ")
	blankSpaces(127)
	fmt.Print(" 4 ")
	fmt.Printf("%c\n", 65372) // Vert. bar
	// 8th line
	printPacketBuffer()
	// 9th line
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(" N ")
	blankSpaces(127)
	fmt.Print(" G ")
	fmt.Printf("%c\n", 65372) // Vert. bar
	// 10th line
	printP4InStats(ingIn, egrIn)
	// 11th line
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 3))
	blankSpaces(127)
	fmt.Print(strings.Repeat(" ", 3))
	fmt.Printf("%c\n", 65372) // Vert. bar

	printOddUplinkStats(portIn, portOut)

	// Hor line
	fmt.Print(" ")
	fmt.Print(strings.Repeat("-", 4))
	fmt.Print(" ")
	fmt.Print(strings.Repeat("-", 12))

	// Print dotted line for uplinks
	for i := 0; i < 9; i++ {
		fmt.Printf("%c", 9679) // Circle
		fmt.Print(strings.Repeat("-", 12))
	}
	fmt.Print(" ")
	fmt.Println(strings.Repeat("-", 4))

	printUplinkNum()

	printEvenUplinkStats(portIn, portOut)

	fmt.Println()
	fmt.Println()
}

func printOddUplinkStats(portIn []uint32, portOut []uint32) {
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 3))
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 18))
	fmt.Printf("%5d", portIn[1])
	fmt.Print(strings.Repeat(" ", 3))
	fmt.Printf("%-5d", portOut[1])

	for i := 3; i < 9; i = i + 2 {
		fmt.Print(strings.Repeat(" ", 13))
		fmt.Printf("%5d", portIn[i])
		fmt.Print(strings.Repeat(" ", 3))
		fmt.Printf("%-5d", portOut[i])
	}

	fmt.Print(strings.Repeat(" ", 18))
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 3))
	fmt.Printf("%c\n", 65372) // Vert. bar
}

func printEvenUplinkStats(portIn []uint32, portOut []uint32) {
	fmt.Print(" ")
	fmt.Print(strings.Repeat(" ", 5))
	fmt.Print(strings.Repeat(" ", 6))
	fmt.Printf("%5d", portIn[0])
	fmt.Print(strings.Repeat(" ", 3))
	fmt.Printf("%-5d", portOut[0])

	for i := 2; i < 10; i = i + 2 {
		fmt.Print(strings.Repeat(" ", 13))
		fmt.Printf("%5d", portIn[i])
		fmt.Print(strings.Repeat(" ", 3))
		fmt.Printf("%-5d", portOut[i])
	}
}

func printUplinkNum() {
	fmt.Print(" ")
	fmt.Print(strings.Repeat(" ", 5))
	fmt.Print(strings.Repeat(" ", 10))

	fmt.Printf("%c 0 %c", downArrow(), upArrow()) // Up arrow,  Down arrow
	for i := 1; i < 9; i++ {
		fmt.Print(strings.Repeat(" ", 8))
		fmt.Printf("%c %d %c", downArrow(), i, upArrow()) // Up arrow,  Down arrow
	}
	fmt.Println()
}

func printBlock(ingIn uint32, ingOut uint32, egrIn uint32, egrOut uint32) {
	// ----- First Line ------
	fmt.Printf("%c", 65372) // Vert. bar
	// Indent
	spaceLine := strings.Repeat(" ", 30)
	fmt.Print(spaceLine)

	// Ingress top bar
	horLine := strings.Repeat("-", 10)
	fmt.Print(horLine)

	spaceLine = strings.Repeat(" ", 40)
	fmt.Print(spaceLine)

	// Egress top bar
	horLine = strings.Repeat("-", 10)
	fmt.Print(horLine)

	// Spaces after egress
	spaceLine = strings.Repeat(" ", 37)
	fmt.Print(spaceLine)

	fmt.Printf("%c\n", 65372) // Vert. bar

	// ----- Second Line ------
	fmt.Printf("%c", 65372) // Vert. bar
	// Indent
	spaceLine = strings.Repeat(" ", 21)
	fmt.Print(spaceLine)

	fmt.Printf("%5d ", ingIn)

	fmt.Printf("%c ", rightArrow()) // Right arrow

	// Ingress left bar
	fmt.Printf("%c", 65372) // Vert. bar

	// Ingress spaces
	fmt.Printf("Ingress ")

	// Ingress right bar
	fmt.Printf("%c", 65372) // Vert. bar

	fmt.Printf("%c", rightArrow()) // Right arrow

	fmt.Printf(" %-5d", ingOut)

	spaceLine = strings.Repeat(" ", 23)
	fmt.Print(spaceLine)

	fmt.Printf("%5d ", egrIn)

	fmt.Printf("%c ", rightArrow()) // Right arrow

	// Egress left bar
	fmt.Printf("%c", 65372) // Vert. bar

	// Egress spaces
	fmt.Printf(" Egress ")

	// Egress right bar
	fmt.Printf("%c", 65372) // Vert. bar

	fmt.Printf("%c", rightArrow()) // Right arrow

	fmt.Printf(" %-5d", egrOut)

	// Spaces after egress
	spaceLine = strings.Repeat(" ", 29)
	fmt.Print(spaceLine)

	fmt.Printf("%c\n", 65372) // Vert. bar

	// ----- Third Line ------
	fmt.Printf("%c", 65372) // Vert. bar
	// Indent
	spaceLine = strings.Repeat(" ", 30)
	fmt.Print(spaceLine)

	// Ingress top bar
	horLine = strings.Repeat("-", 10)
	fmt.Print(horLine)

	spaceLine = strings.Repeat(" ", 40)
	fmt.Print(spaceLine)

	// Egress top bar
	horLine = strings.Repeat("-", 10)
	fmt.Print(horLine)

	// Spaces after egress
	spaceLine = strings.Repeat(" ", 37)
	fmt.Print(spaceLine)

	fmt.Printf("%c\n", 65372) // Vert. bar

}

func printPacketBuffer() {
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(" I ")
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 58))
	fmt.Print("Packet Buffer")
	fmt.Print(strings.Repeat(" ", 56))
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(" E ")
	fmt.Printf("%c\n", 65372) // Vert. bar
}

func printP4InStats(ingIn uint32, egrIn uint32) {
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(" G ")
	fmt.Printf("%c", 65372)                     // Vert. bar
	fmt.Printf(" %c %-5d", rightArrow(), ingIn) // -> arrow
	fmt.Print(strings.Repeat(" ", 111))
	fmt.Printf("%5d %c ", egrIn, leftArrow()) // <- arrow
	fmt.Printf("%c", 65372)                   // Vert. bar
	fmt.Print(" R ")
	fmt.Printf("%c\n", 65372) // Vert. bar
}

func printP4OutStats(ingOut uint32, egrOut uint32) {
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(" P ")
	fmt.Printf("%c", 65372)                     // Vert. bar
	fmt.Printf(" %c %-5d", leftArrow(), ingOut) // <- arrow
	fmt.Print(strings.Repeat(" ", 111))
	fmt.Printf("%5d %c ", egrOut, rightArrow()) // -> arrow
	fmt.Printf("%c", 65372)                     // Vert. bar
	fmt.Print(" P ")
	fmt.Printf("%c\n", 65372) // Vert. bar
}

func printDMAStats(dmaIn uint32, dmaOut uint32) {
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 3))
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 25))
	fmt.Printf("%5d %c", dmaIn, upArrow()) // Down arrow
	fmt.Print(strings.Repeat(" ", 58))
	fmt.Printf("%5d %c", dmaOut, downArrow()) // Up arrow
	fmt.Print(strings.Repeat(" ", 30))
	fmt.Printf("%c", 65372) // Vert. bar
	fmt.Print(strings.Repeat(" ", 3))
	fmt.Printf("%c\n", 65372) // Vert. bar
}

func blankSpaces(num int) {
	fmt.Printf("%c", 65372) // Vert. bar
	blankLine := strings.Repeat(" ", num)
	fmt.Print(blankLine)
	fmt.Printf("%c", 65372) // Vert. bar
}

func blankLine(num int) {
	fmt.Printf("%c", 65372) // Vert. bar
	blankLine := strings.Repeat(" ", num)
	fmt.Print(blankLine)
	fmt.Printf("%c\n", 65372) // Vert. bar
}

func systemDropStatsShowHeader() {
	hdrLine := strings.Repeat("-", 70)
	fmt.Println(hdrLine)
	fmt.Printf("%-50s%-12s\n",
		"Reason", "Drop Count")
	fmt.Println(hdrLine)
}

func dropReasonToString(reasons *halproto.DropReasons) string {
	if reasons.GetDropMalformedPkt() {
		return "Drop Malformed Pkt"
	}
	if reasons.GetDropParserIcrcError() {
		return "Drop Parser ICRC Error"
	}
	if reasons.GetDropParseLenError() {
		return "Drop Parse Len Error"
	}
	if reasons.GetDropHardwareError() {
		return "Drop Hardware Error"
	}
	if reasons.GetDropInputMapping() {
		return "Drop Input Mapping"
	}
	if reasons.GetDropInputMappingDejavu() {
		return "Drop Input Mapping Deja Vu"
	}
	if reasons.GetDropMultiDestNotPinnedUplink() {
		return "Drop Multi Dest Not Pinned Uplink"
	}
	if reasons.GetDropFlowHit() {
		return "Drop Flow Hit"
	}
	if reasons.GetDropFlowMiss() {
		return "Drop Flow Miss"
	}
	if reasons.GetDropNacl() {
		return "Drop Nacl"
	}
	if reasons.GetDropIpsg() {
		return "Drop IP SG"
	}
	if reasons.GetDropIpNormalization() {
		return "Drop IP Normalization"
	}
	if reasons.GetDropTcpNormalization() {
		return "Drop TCP Normalization"
	}
	if reasons.GetDropTcpRstWithInvalidAckNum() {
		return "Drop TCP RST with Invalid ACK Num"
	}
	if reasons.GetDropTcpNonSynFirstPkt() {
		return "Drop TCP Non-SYN First Pkt"
	}
	if reasons.GetDropIcmpNormalization() {
		return "Drop ICMP Normalization"
	}
	if reasons.GetDropInputPropertiesMiss() {
		return "Drop Input Properties Miss"
	}
	if reasons.GetDropTcpOutOfWindow() {
		return "Drop TCP Out-of-window"
	}
	if reasons.GetDropTcpSplitHandshake() {
		return "Drop TCP Split Handshake"
	}
	if reasons.GetDropTcpWinZeroDrop() {
		return "Drop TCP Window Zero"
	}
	if reasons.GetDropTcpDataAfterFin() {
		return "Drop TCP Data after FIN"
	}
	if reasons.GetDropTcpNonRstPktAfterRst() {
		return "Drop TCP non-RST Pkt after RST"
	}
	if reasons.GetDropTcpInvalidResponderFirstPkt() {
		return "Drop TCP Invalid Responder First Pkt"
	}
	if reasons.GetDropTcpUnexpectedPkt() {
		return "Drop TCP Unexpected Pkt"
	}
	if reasons.GetDropSrcLifMismatch() {
		return "Drop Src Lif Mismatch"
	}
	if reasons.GetDropVfIpLabelMismatch() {
		return "Drop Vf IP Label Mismatch"
	}
	if reasons.GetDropVfBadRrDstIp() {
		return "Drop Vf Bad RR Dst IP"
	}
	return "Invalid"
}

func systemDropStatsShowEntry(entry *halproto.DropStatsEntry) {
	str := dropReasonToString(entry.GetReasons())
	if strings.Compare(str, "Invalid") == 0 {
		return
	}
	fmt.Printf("%-50s%-12d\n",
		str,
		entry.GetDropCount())
}

func egressDropReasonToString(reasons *halproto.EgressDropReasons) string {
	if reasons.GetDropOutputMapping() {
		return "Drop Output Mapping"
	}
	if reasons.GetDropPruneSrcPort() {
		return "Drop Prune Src Port"
	}
	if reasons.GetDropMirror() {
		return "Drop Mirror"
	}
	if reasons.GetDropPolicer() {
		return "Drop Policer"
	}
	if reasons.GetDropCopp() {
		return "Drop Copp"
	}
	if reasons.GetDropChecksumErr() {
		return "Drop Checksum Error"
	}
	return "Invalid"
}

func systemEgressDropStatsShowEntry(entry *halproto.EgressDropStatsEntry) {
	str := egressDropReasonToString(entry.GetReasons())
	if strings.Compare(str, "Invalid") == 0 {
		return
	}
	fmt.Printf("%-50s%-12d\n",
		str,
		entry.GetDropCount())
}

func systemTableStatsShowHeader() {
	hdrLine := strings.Repeat("-", 155)
	fmt.Println(hdrLine)
	fmt.Printf("%-22s%-30s%-12s%-12s%-12s%-16s%-12s%-12s%-12s%-12s\n",
		"Type", "Name", "Size", "Oflow Size", "Entries",
		"Oflow Entries", "Inserts", "Insert Err", "Deletes", "Delete Err")
	fmt.Println(hdrLine)
}

func systemTableStatsShowEntry(entry *halproto.TableStatsEntry) {
	fmt.Printf("%-22s%-30s%-12d%-12d%-12d%-16d%-12d%-12d%-12d%-12d\n",
		entry.GetTableType().String(),
		entry.GetTableName(),
		entry.GetTableSize(),
		entry.GetOverflowTableSize(),
		entry.GetEntriesInUse(),
		entry.GetOverflowEntriesInUse(),
		entry.GetNumInserts(),
		entry.GetNumInsertErrors(),
		entry.GetNumDeletes(),
		entry.GetNumDeleteErrors())
}

func fteTxRxStatsShow(stats *halproto.Stats) {
	var pmdstats *halproto.PMDStats

	pmdstats = stats.GetPmdStats()
	if pmdstats == nil {
		return
	}

	var fteid int
	if pmdstats.FteInfo == nil {
		return
	}
	for _, fteinfo := range pmdstats.FteInfo {
		fmt.Printf("%s%-3d\n", "FTE ", fteid)
		for _, qinfo := range fteinfo.Qinfo {
			fmt.Printf("%s\n", strings.Repeat("-", 125))
			fmt.Printf("Qtype : %s, Qid : %d, Base addr : %#x, PC idx : %#x, "+
				"PC idx addr : %#x, Valid bit expected : %d\n",
				halproto.WRingType_name[int32(qinfo.GetQueueType())],
				qinfo.GetQueueId(), qinfo.GetBaseAddr(), qinfo.GetPcIndex(),
				qinfo.GetPcIndexAddr(), qinfo.GetValidBitValue())
			fmt.Printf("%s\n", strings.Repeat("-", 125))
			ctr := qinfo.GetCtr()
			fmt.Printf("%s%-15d\n", "Tx pkts                                 : ", ctr.GetSendPkts())
			fmt.Printf("%s%-15d\n", "Rx pkts                                 : ", ctr.GetRecvPkts())
			fmt.Printf("%s%-15d\n", "Rx semaphore write errors               : ", ctr.GetRxSemWrErr())
			fmt.Printf("%s%-15d\n", "Rx slot value read errors               : ", ctr.GetRxSlotValueReadErr())
			fmt.Printf("%s%-15d\n", "Rx descriptor read errors               : ", ctr.GetRxDescrReadErr())
			fmt.Printf("%s%-15d\n", "Rx descriptor to header errors          : ", ctr.GetRxDescrToHdrErr())
			fmt.Printf("%s%-15d\n", "Rx descriptor free errors               : ", ctr.GetRxDescrFreeErr())
			fmt.Printf("%s%-15d\n", "Tx descriptor free errors               : ", ctr.GetTxDescrFreeErr())
			fmt.Printf("%s%-15d\n", "Tx page allocation errors               : ", ctr.GetTxPageAllocErr())
			fmt.Printf("%s%-15d\n", "Tx page copy errors                     : ", ctr.GetTxPageCopyErr())
			fmt.Printf("%s%-15d\n", "Tx descriptor programming errors        : ", ctr.GetTxDescrPgmErr())
			fmt.Printf("%s%-15d\n", "Tx send errors                          : ", ctr.GetTxSendErr())
			fmt.Printf("%s%-15d\n", "Poll count                              : ", ctr.GetPollCount())
			fmt.Printf("%s%-15d\n", "Rx descriptor out of bound errors       : ", ctr.GetRxDescrAddrOob())
			fmt.Printf("%s%-15d\n", "Tx doorbell errors                      : ", ctr.GetTxDoorbellErr())

			fmt.Printf("\n")
		}
		fmt.Printf("\n")
		fmt.Printf("FTE Local\n")
		glbl := fteinfo.GetGlbal()
		fmt.Printf("%s%-15d\n", "GC pindex           : ", glbl.GetGcPindex())
		fmt.Printf("%s%-15d\n", "Cpu Tx Page Pindex  : ", glbl.GetCpuTxPagePindex())
		fmt.Printf("%s%-15d\n", "Cpu Tx Page Cindex  : ", glbl.GetCpuTxPageCindex())
		fmt.Printf("%s%-15d\n", "Cpu Tx Descr Pindex : ", glbl.GetCpuTxDescrPindex())
		fmt.Printf("%s%-15d\n", "Cpu Tx Descr Cindex : ", glbl.GetCpuTxDescrCindex())
		fmt.Printf("%s%-15d\n", "Cpu Rx DPR Cindex   : ", glbl.GetCpuRxDprCindex())
		fmt.Printf("%s%-15d\n", "Cpu Rx DPR SEM Cindex   : ", glbl.GetCpuRxDprSemCindex())
		fmt.Printf("%s%-15d\n", "Cpu Rx DPR Descr Free Errors : ", glbl.GetCpuRxDprDescrFreeErr())

		fteid++
	}
	fmt.Printf("\n")
}

func fteStatsShow(stats *halproto.Stats) {
	var ftestats *halproto.FTEStats

	ftestats = stats.GetFteStats()
	if ftestats == nil {
		return
	}

	var fteid int

	for _, ftestatsinfo := range ftestats.FteStatsInfo {
		fmt.Printf("\n%s\n", strings.Repeat("-", 15))
		fmt.Printf("%s%-3d\n", "FTE ID   : ", fteid)
		fmt.Printf("%s\n", strings.Repeat("-", 15))
		fmt.Printf("\n%s%-15d\n", "Connection per-second          :", ftestatsinfo.GetConnPerSecond())
		fmt.Printf("%s%-15d\n", "Max. Connection per-second     :", ftestatsinfo.GetMaxConnPerSec())
		fmt.Printf("%s%-15d\n", "Flow-miss Packets		:", ftestatsinfo.GetFlowMissPkts())
		fmt.Printf("%s%-15d\n", "Redir Packets			:", ftestatsinfo.GetRedirPkts())
		fmt.Printf("%s%-15d\n", "Cflow Packets			:", ftestatsinfo.GetCflowPkts())
		fmt.Printf("%s%-15d\n", "TCP Close Packets		:", ftestatsinfo.GetTcpClosePkts())
		fmt.Printf("%s%-15d\n", "TLS Proxy Packets		:", ftestatsinfo.GetTlsProxyPkts())
		fmt.Printf("%s%-15d\n", "Softq Reqs			:", ftestatsinfo.GetSoftqReqs())
		fmt.Printf("%s%-15d\n", "Queued Tx Packets		:", ftestatsinfo.GetQueuedTxPkts())
		fmt.Printf("\n%s\n", "FTE Error Count: ")
		hdrLine := strings.Repeat("-", 56)
		fmt.Println(hdrLine)
		fmt.Printf("%25s%25s\n", "Error Type", "Drop Count")
		fmt.Println(hdrLine)
		if ftestatsinfo.FteErrors != nil {
			for _, fteerr := range ftestatsinfo.FteErrors {
				if fteerr != nil {
					if fteerr.GetCount() != 0 {
						fmt.Printf("%25s%25d\n", fteerr.GetFteError(), fteerr.GetCount())
					}
				}
			}
		}

		fmt.Printf("\n%s\n", "FTE Feature Stats:")
		hdrLines := strings.Repeat("-", 100)
		fmt.Println(hdrLines)
		fmt.Printf("%25s%25s%25s%25s\n", "Feature Name", "Drop Count", "Drop Reason", "Drops per-reason")
		fmt.Println(hdrLines)
		if ftestatsinfo.FeatureStats != nil {
			for _, featurestats := range ftestatsinfo.FeatureStats {
				if featurestats != nil {
					fmt.Printf("%25s%25d", featurestats.GetFeatureName(), featurestats.GetDropPkts())
					for _, fteerr := range featurestats.DropReason {
						if fteerr.GetCount() != 0 {
							fmt.Printf("%25s%25d\n", fteerr.GetFteError(), fteerr.GetCount())
						}
					}
				}
				fmt.Printf("\n")
			}
			fmt.Printf("\n")
		}
		fteid++
	}
}

func sessionSummaryStatsShow(stats *halproto.Stats) {
	var sessstats *halproto.SessionSummaryStats

	sessstats = stats.GetSessionStats()

	hdrLine := strings.Repeat("-", 56)
	fmt.Println(hdrLine)
	fmt.Printf("%25s%25s\n", "Session Type", "Count")
	fmt.Println(hdrLine)
	fmt.Printf("%25s%25d\n", "L2", sessstats.GetL2Sessions())
	fmt.Printf("%25s%25d\n", "TCP", sessstats.GetTcpSessions())
	fmt.Printf("%25s%25d\n", "UDP", sessstats.GetUdpSessions())
	fmt.Printf("%25s%25d\n", "ICMP", sessstats.GetIcmpSessions())
	fmt.Printf("%25s%25d\n", "DROP", sessstats.GetDropSessions())
	fmt.Printf("%25s%25d\n", "AGED", sessstats.GetAgedSessions())
	fmt.Printf("%25s%25d\n", "TCP connection timeouts", sessstats.GetNumConnectionTimeoutSessions())
	fmt.Printf("%25s%25d\n", "TCP Resets on SFW Reject", sessstats.GetAgedSessions())
	fmt.Printf("%25s%25d\n", "ICMP Errors on SFW Reject", sessstats.GetAgedSessions())
	fmt.Printf("%25s%25d\n", "Session create failures", sessstats.GetNumSessionCreateErrors())
}

func apiStatsShowHeader() {
	hdrLine := strings.Repeat("-", 100)
	fmt.Println(hdrLine)
	fmt.Printf("%-55s%-12s%-12s%-12s\n",
		"API", "Num Calls", "Num Success", "Num Fail")
	fmt.Println(hdrLine)
}

func apiStatsEntryShow(entry *halproto.ApiStatsEntry) {
	fmt.Printf("%-55s%-12d%-12d%-12d\n",
		entry.GetApiType().String(),
		entry.GetNumApiCall(),
		entry.GetNumApiSuccess(),
		entry.GetNumApiFail())
}

func threadShowCmdHandler(cmd *cobra.Command, args []string) {
	if cmd.Flags().Changed("yaml") {
		threadDetailShowCmdHandler(cmd, args)
		return
	}

	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	defer c.Close()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewDebugClient(c.ClientConn)

	var empty *halproto.Empty

	// HAL call
	respMsg, err := client.ThreadGet(context.Background(), empty)
	if err != nil {
		fmt.Printf("Thread get failed. %v\n", err)
		return
	}

	threadShowHeader()

	// Print Response
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("HAL Returned non OK status. %v\n", resp.ApiStatus)
			continue
		}
		threadShowEntry(resp)
	}
}

func threadShowHeader() {
	hdrLine := strings.Repeat("-", 150)
	fmt.Println(hdrLine)
	fmt.Printf("%-20s%-10s%-12s%-14s%-14s%-10s%-20s%-10s%-21s%-10s%-12s\n",
		"Name", "Id", "PThreadId", "CtrlCoreMask", "DataCoreMask", "Prio", "SchedPolicy", "Running", "Role", "CoreMask", "LastHb(ms)")
	fmt.Println(hdrLine)
}

func threadShowEntry(resp *halproto.ThreadResponse) {
	spec := resp.GetSpec()
	status := resp.GetStatus()

	fmt.Printf("%-20s%-10d%-12d%-#14x%-#14x%-10d%-20s%-10t%-21s%-#10x%-12d\n",
		spec.GetName(), spec.GetId(), spec.GetPthreadId(),
		resp.GetControlCoreMask(), resp.GetDataCoreMask(),
		spec.GetPrio(), spec.GetSchedPolicy().String(),
		spec.GetRunning(), spec.GetRole().String(),
		spec.GetCoreMask(), status.GetLastHb()/1000)
}

func threadDetailShow(ofile *os.File) {
	// Connect to HAL
	c, err := utils.CreateNewGRPCClient()
	defer c.Close()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}
	client := halproto.NewDebugClient(c.ClientConn)

	var empty *halproto.Empty

	// HAL call
	respMsg, err := client.ThreadGet(context.Background(), empty)
	if err != nil {
		fmt.Printf("Thread get failed. %v\n", err)
		return
	}

	// Print Response
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("HAL Returned non OK status. %v\n", resp.ApiStatus)
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

func threadDetailShowCmdHandler(cmd *cobra.Command, args []string) {
	threadDetailShow(nil)
}

func intfStatsHeader() {
	fmt.Printf("\n")
	hdrLine := strings.Repeat("-", 150)
	fmt.Printf("Note: In/Out from reference of PB\n")
	fmt.Println(hdrLine)
	fmt.Printf("%-15s%-29s%-31s%s%-29s%-31s\n", "", "", "In", " | ", "", "Out")
	fmt.Printf("%-15s%-5s%-15s%-5s%-15s%-5s%-15s | %-5s%-15s%-5s%-15s%-5s%-15s\n",
		"", "", "UC", "", "MC", "", "BC", "", "UC", "", "MC", "", "BC")
	fmt.Printf("%-15s%-10s%-10s%-10s%-10s%-10s%-10s | %-10s%-10s%-10s%-10s%-10s%-10s\n",
		"IF", "UC", "Drops", "MC", "Drops", "BC", "Drops", "UC", "Drops", "MC", "Drops", "BC", "Drops")
	fmt.Println(hdrLine)
}

func intfUplinkShowOneResp(resp *halproto.PortGetResponse) {
	portStr := fmt.Sprint(resp.GetSpec().GetKeyOrHandle().GetPortId())

	intfStr := ""
	if portStr == "9" {
		intfStr += "Uplink-OOB"
	} else {
		intfStr += "Uplink-" + portStr
	}

	var rxUc uint64
	var rxMc uint64
	var rxBc uint64
	var txUc uint64
	var txMc uint64
	var txBc uint64

	macStats := resp.GetStats().GetMacStats()
	for _, s := range macStats {
		switch s.GetType() {
		case halproto.MacStatsType_FRAMES_RX_UNICAST:
			rxUc = s.GetCount()
		case halproto.MacStatsType_FRAMES_RX_MULTICAST:
			rxMc = s.GetCount()
		case halproto.MacStatsType_FRAMES_RX_BROADCAST:
			rxBc = s.GetCount()
		//case halproto.MacStatsType_FRAMES_RX_ALL:
		//	rxAll = s.GetCount()
		case halproto.MacStatsType_FRAMES_TX_UNICAST:
			txUc = s.GetCount()
		case halproto.MacStatsType_FRAMES_TX_MULTICAST:
			txMc = s.GetCount()
		case halproto.MacStatsType_FRAMES_TX_BROADCAST:
			txBc = s.GetCount()
		//case halproto.MacStatsType_FRAMES_TX_ALL:
		//	txAll = s.GetCount()
		default:
			continue
		}
	}
	fmt.Printf("%-15s%-10d%-10s%-10d%-10s%-10d%-10s   %-10d%-10s%-10d%-10s%-10d%-10s\n",
		intfStr,
		rxUc, "---", rxMc, "---", rxBc, "---",
		txUc, "---", txMc, "---", txBc, "---")

}

func uplinkStatsShow(c *rpckit.RPCClient) {
	client := halproto.NewPortClient(c.ClientConn)
	var req *halproto.PortGetRequest
	req = &halproto.PortGetRequest{}
	portGetReqMsg := &halproto.PortGetRequestMsg{
		Request: []*halproto.PortGetRequest{req},
	}

	// HAL call
	respMsg, err := client.PortGet(context.Background(), portGetReqMsg)
	if err != nil {
		fmt.Printf("Getting Port failed. %v\n", err)
		return
	}

	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("HAL Returned non OK status. %v\n", resp.ApiStatus)
			continue
		}
		intfUplinkShowOneResp(resp)
	}
}

func intfMnicLifShowOneResp(resp *halproto.LifGetResponse) {
	// status := resp.GetStatus()
	// hwLifID := status.GetHwLifId()
	lifName := resp.GetSpec().GetName()
	stats := resp.GetStats()
	rxStats := stats.GetDataLifStats().GetRxStats()
	txStats := stats.GetDataLifStats().GetTxStats()
	if strings.Contains(lifName, "mnic") || strings.Contains(lifName, "accel") || strings.Contains(lifName, "admin") {
		fmt.Printf("%-15s%-10d%-10d%-10d%-10d%-10d%-10d   %-10d%-10d%-10d%-10d%-10d%-10d\n",
			lifName,
			txStats.GetUnicastFramesOk(),
			txStats.GetUnicastFramesDrop(),
			txStats.GetMulticastFramesOk(),
			txStats.GetMulticastFramesDrop(),
			txStats.GetBroadcastFramesOk(),
			txStats.GetBroadcastFramesDrop(),
			rxStats.GetUnicastFramesOk(),
			rxStats.GetUnicastFramesDrop(),
			rxStats.GetMulticastFramesOk(),
			rxStats.GetMulticastFramesDrop(),
			rxStats.GetBroadcastFramesOk(),
			rxStats.GetBroadcastFramesDrop())
	}
}

func intfProxyLifShowOneResp(resp *halproto.LifGetResponse) {
	// status := resp.GetStatus()
	// hwLifID := status.GetHwLifId()
	lifName := resp.GetSpec().GetName()
	stats := resp.GetStats()
	rxStats := stats.GetDataLifStats().GetRxStats()
	txStats := stats.GetDataLifStats().GetTxStats()
	if strings.Contains(lifName, "proxy") {
		fmt.Printf("%-15s%-10d%-10d%-10d%-10d%-10d%-10d   %-10d%-10d%-10d%-10d%-10d%-10d\n",
			lifName,
			txStats.GetUnicastFramesOk(),
			txStats.GetUnicastFramesDrop(),
			txStats.GetMulticastFramesOk(),
			txStats.GetMulticastFramesDrop(),
			txStats.GetBroadcastFramesOk(),
			txStats.GetBroadcastFramesDrop(),
			rxStats.GetUnicastFramesOk(),
			rxStats.GetUnicastFramesDrop(),
			rxStats.GetMulticastFramesOk(),
			rxStats.GetMulticastFramesDrop(),
			rxStats.GetBroadcastFramesOk(),
			rxStats.GetBroadcastFramesDrop())
	}
}

func intfHostLifShowOneResp(resp *halproto.LifGetResponse) {
	// status := resp.GetStatus()
	// hwLifID := status.GetHwLifId()
	lifName := resp.GetSpec().GetName()
	stats := resp.GetStats()
	rxStats := stats.GetDataLifStats().GetRxStats()
	txStats := stats.GetDataLifStats().GetTxStats()
	if !strings.Contains(lifName, "mnic") && !strings.Contains(lifName, "proxy") {
		fmt.Printf("%-15s%-10d%-10d%-10d%-10d%-10d%-10d   %-10d%-10d%-10d%-10d%-10d%-10d\n",
			lifName,
			txStats.GetUnicastFramesOk(),
			txStats.GetUnicastFramesDrop(),
			txStats.GetMulticastFramesOk(),
			txStats.GetMulticastFramesDrop(),
			txStats.GetBroadcastFramesOk(),
			txStats.GetBroadcastFramesDrop(),
			rxStats.GetUnicastFramesOk(),
			rxStats.GetUnicastFramesDrop(),
			rxStats.GetMulticastFramesOk(),
			rxStats.GetMulticastFramesDrop(),
			rxStats.GetBroadcastFramesOk(),
			rxStats.GetBroadcastFramesDrop())
	}
}

func lifStatsShow(c *rpckit.RPCClient) {
	client := halproto.NewInterfaceClient(c.ClientConn)
	var req *halproto.LifGetRequest
	req = &halproto.LifGetRequest{}
	lifGetReqMsg := &halproto.LifGetRequestMsg{
		Request: []*halproto.LifGetRequest{req},
	}

	mnicList := list.New()
	proxyList := list.New()
	hostList := list.New()

	// HAL call
	respMsg, err := client.LifGet(context.Background(), lifGetReqMsg)
	if err != nil {
		fmt.Printf("Getting Lif failed. %v\n", err)
		return
	}
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("HAL Returned non OK status. %v\n", resp.ApiStatus)
			continue
		}
		lifName := resp.GetSpec().GetName()
		if strings.Contains(lifName, "mnic") || strings.Contains(lifName, "accel") || strings.Contains(lifName, "admin") {
			mnicList.PushBack(resp)
		} else if strings.Contains(lifName, "proxy") {
			proxyList.PushBack(resp)
		} else {
			hostList.PushBack(resp)
		}
	}

	for e := mnicList.Front(); e != nil; e = e.Next() {
		intfMnicLifShowOneResp(e.Value.(*halproto.LifGetResponse))
	}
	for e := proxyList.Front(); e != nil; e = e.Next() {
		intfProxyLifShowOneResp(e.Value.(*halproto.LifGetResponse))
	}
	for e := hostList.Front(); e != nil; e = e.Next() {
		intfHostLifShowOneResp(e.Value.(*halproto.LifGetResponse))
	}
}

func intfStatsShow(c *rpckit.RPCClient) {
	// Display Header
	intfStatsHeader()

	// Uplink Mac stats show
	uplinkStatsShow(c)

	// Lif Stats show
	lifStatsShow(c)

}
