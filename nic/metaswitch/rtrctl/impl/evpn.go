//-----------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package impl

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"

	"github.com/spf13/cobra"

	pegasusClient "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	types "github.com/pensando/sw/nic/apollo/agent/gen/pds"
	"github.com/pensando/sw/nic/metaswitch/rtrctl/utils"
)

var evpnShowCmd = &cobra.Command{
	Use:   "evpn",
	Short: "show EVPN related information",
	Long:  "show EVPN related information",
	Args:  cobra.NoArgs,
}

var evpnVrfShowCmd = &cobra.Command{
	Use:   "vrf",
	Short: "show EVPN VRF information",
	Long:  "show EVPN VRF information",
	Args:  cobra.NoArgs,
}

var evpnVrfStatusShowCmd = &cobra.Command{
	Use:   "status",
	Short: "show EVPN VRF Status information",
	Long:  "show EVPN VRF Status information",
	Args:  cobra.NoArgs,
	RunE:  evpnVrfStatusShowCmdHandler,
}

var evpnVrfRtShowCmd = &cobra.Command{
	Use:   "rt",
	Short: "show EVPN VRF RT information",
	Long:  "show EVPN VRF RT information",
	Args:  cobra.NoArgs,
}

var evpnVrfRtStatusShowCmd = &cobra.Command{
	Use:   "status",
	Short: "show EVPN VRF RT Status information",
	Long:  "show EVPN VRF RT Status information",
	Args:  cobra.NoArgs,
	RunE:  evpnVrfRtStatusShowCmdHandler,
}

var evpnEviShowCmd = &cobra.Command{
	Use:   "evi",
	Short: "show EVPN EVI information",
	Long:  "show EVPN EVI information",
	Args:  cobra.NoArgs,
}

var evpnEviStatusShowCmd = &cobra.Command{
	Use:   "status",
	Short: "show EVPN EVI Status information",
	Long:  "show EVPN EVI Status information",
	Args:  cobra.NoArgs,
	RunE:  evpnEviStatusShowCmdHandler,
}

var evpnEviRtShowCmd = &cobra.Command{
	Use:   "rt",
	Short: "show EVPN EVI RT information",
	Long:  "show EVPN EVI RT information",
	Args:  cobra.NoArgs,
}

var evpnEviRtStatusShowCmd = &cobra.Command{
	Use:   "status",
	Short: "show EVPN EVI RT Status information",
	Long:  "show EVPN EVI RT Status information",
	Args:  cobra.NoArgs,
	RunE:  evpnEviRtStatusShowCmdHandler,
}

func init() {

}

const (
	evpnIPVrfFmt    = `%-10s %-16v %-16s`
	evpnIPVrfHdr    = "VPC Id,VNI,RD"
	evpnIPVrfDetStr = `EVPN IP VRF details
------------------------------------
Id              : %s
VPC Id          : %s
VRF VNI         : %d
VRF RD          : %s
------------------------------------
`
)

func evpnVrfStatusShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := pegasusClient.NewEvpnSvcClient(c)

	req := &pegasusClient.EvpnIpVrfGetRequest{}
	respMsg, err := client.EvpnIpVrfGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting evpn vrf failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"
	doDetail := cmd.Flag("detail").Value.String() == "true"

	if !doJSON && !doDetail {
		utils.PrintHeader(evpnIPVrfFmt, evpnIPVrfHdr)
	}
	var ipVrfs []*utils.ShadowEvpnIpVrf
	for _, p := range respMsg.Response {
		ipVrf := utils.NewEvpnIpVrf(p)
		if doJSON {
			ipVrfs = append(ipVrfs, ipVrf)
		} else {
			if doDetail {
				fmt.Printf(evpnIPVrfDetStr, ipVrf.Spec.Id, ipVrf.Spec.VPCId, ipVrf.Spec.VNI, ipVrf.Spec.RD)
			} else {
				fmt.Printf(evpnIPVrfFmt, ipVrf.Spec.VPCId, ipVrf.Spec.VNI, ipVrf.Spec.RD)
				fmt.Printf("\n")
			}
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(ipVrfs, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}

const (
	evpnIPVrfRtFmt    = `%-10s %-16s`
	evpnIPVrfRtHdr    = "VPC Id, RT"
	evpnIPVrfRtDetStr = `EVPN IP VRF RT details
------------------------------------
VPC Id          : %s
VRF RT          : %s
VRF RT-type     : %s
------------------------------------
`
)

func evpnVrfRtStatusShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := pegasusClient.NewEvpnSvcClient(c)

	req := &pegasusClient.EvpnIpVrfRtGetRequest{}
	respMsg, err := client.EvpnIpVrfRtGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting evpn vrf rt failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"
	doDetail := cmd.Flag("detail").Value.String() == "true"

	if !doJSON && !doDetail {
		utils.PrintHeader(evpnIPVrfRtFmt, evpnIPVrfRtHdr)
	}
	var ipVrfRts []*utils.ShadowEvpnIpVrfRt
	for _, p := range respMsg.Response {
		ipVrfRt := utils.NewEvpnIpVrfRt(p)
		if doJSON {
			ipVrfRts = append(ipVrfRts, ipVrfRt)
		} else {
			if doDetail {
				fmt.Printf(evpnIPVrfRtDetStr, ipVrfRt.Spec.VPCId, ipVrfRt.Spec.RT, ipVrfRt.Spec.RTType)
			} else {
				fmt.Printf(evpnIPVrfRtFmt, ipVrfRt.Spec.VPCId, ipVrfRt.Spec.RT)
				fmt.Printf("\n")
			}
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(ipVrfRts, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}

const (
	evpnEviFmt    = `%-10s %-16s %-16s`
	evpnEviHdr    = "Subnet Id,RD,Status"
	evpnEviDetStr = `EVPN EVI details
------------------------------------
Id              : %s
Subnet Id       : %s
Auto RD         : %d
RD              : %s
Auto RT         : %d
RT-Type         : %s
Status          : %s
------------------------------------
`
)

func evpnEviStatusShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := pegasusClient.NewEvpnSvcClient(c)

	req := &pegasusClient.EvpnEviGetRequest{}
	respMsg, err := client.EvpnEviGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting evpn evi failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"
	doDetail := cmd.Flag("detail").Value.String() == "true"

	if !doJSON && !doDetail {
		utils.PrintHeader(evpnEviFmt, evpnEviHdr)
	}
	var evis []*utils.ShadowEvpnEvi
	for _, p := range respMsg.Response {
		evi := utils.NewEvpnEvi(p)
		if doJSON {
			evis = append(evis, evi)
		} else {
			if doDetail {
				fmt.Printf(evpnEviDetStr, evi.Spec.Id, evi.Spec.SubnetId, evi.Spec.AutoRD,
					evi.Status.RD, evi.Spec.AutoRT, evi.Spec.RTType, evi.Status.Status)
			} else {
				fmt.Printf(evpnEviFmt, evi.Spec.SubnetId, evi.Status.RD, evi.Status.Status)
				fmt.Printf("\n")
			}
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(evis, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}

const (
	evpnEviRtFmt    = `%-10s %-16s`
	evpnEviRtHdr    = "Subnet Id, RT"
	evpnEviRtDetStr = `EVPN EVI RT details
------------------------------------
Id              : %s
Subnet Id       : %s
RT              : %s
RT-Type         : %s
------------------------------------
`
)

func evpnEviRtStatusShowCmdHandler(cmd *cobra.Command, args []string) error {
	c, err := utils.CreateNewGRPCClient(cliParams.GRPCPort)
	if err != nil {
		return errors.New("Could not connect to the PDS. Is PDS Running?")
	}
	defer c.Close()
	client := pegasusClient.NewEvpnSvcClient(c)

	req := &pegasusClient.EvpnEviRtGetRequest{}
	respMsg, err := client.EvpnEviRtGet(context.Background(), req)
	if err != nil {
		return fmt.Errorf("Getting evpn evi rt failed (%s)", err)
	}

	if respMsg.ApiStatus != types.ApiStatus_API_STATUS_OK {
		return errors.New("Operation failed with error")
	}

	doJSON := cmd.Flag("json").Value.String() == "true"
	doDetail := cmd.Flag("detail").Value.String() == "true"

	if !doJSON && !doDetail {
		utils.PrintHeader(evpnEviRtFmt, evpnEviRtHdr)
	}
	var eviRts []*utils.ShadowEvpnEviRt
	for _, p := range respMsg.Response {
		eviRt := utils.NewEvpnEviRt(p)
		if doJSON {
			eviRts = append(eviRts, eviRt)
		} else {
			if doDetail {
				fmt.Printf(evpnEviRtDetStr, eviRt.Spec.Id, eviRt.Spec.SubnetId,
					eviRt.Spec.RT, eviRt.Spec.RTType)
			} else {
				fmt.Printf(evpnEviRtFmt, eviRt.Spec.SubnetId, eviRt.Spec.RT)
				fmt.Printf("\n")
			}
		}
	}
	if doJSON {
		b, _ := json.MarshalIndent(eviRts, "", "  ")
		fmt.Println(string(b))
	}
	return nil
}