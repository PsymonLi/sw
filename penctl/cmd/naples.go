//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"strings"

	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/nic/agent/protos/nmd"

	"github.com/spf13/cobra"

	"github.com/pensando/sw/api"
	vldtor "github.com/pensando/sw/venice/utils/apigen/validators"
	"github.com/pensando/sw/venice/utils/strconv"
)

var naplesCmd = &cobra.Command{
	Use:   "naples",
	Short: "Set NAPLES Modes and Profiles",
	Long:  "\n----------------------------\n Set NAPLES configuration \n----------------------------\n",
	RunE:  naplesCmdHandler,
	Args:  naplesCmdValidator,
}

var naplesShowCmd = &cobra.Command{
	Use:   "naples",
	Short: "Show NAPLES Modes and Profiles",
	Long:  "\n-------------------------------------------------------------------\n Show NAPLES configuration \n-------------------------------------------------------------------\n",
	RunE:  naplesShowCmdHandler,
}

var naplesProfileCreateCmd = &cobra.Command{
	Use:   "naples-profile",
	Short: "naples profile object",
	Long:  "\n----------------------------\n Create NAPLES Profiles \n----------------------------\n",
	RunE:  naplesProfileCreateCmdHandler,
	Args:  naplesProfileCreateCmdValidator,
}

var naplesProfileShowCmd = &cobra.Command{
	Use:   "naples-profiles",
	Short: "Show Available NAPLES Profiles",
	Long:  "\n----------------------------\n Show NAPLES Profiles \n----------------------------\n",
	RunE:  naplesProfileShowCmdHandler,
}

var naplesProfileDeleteCmd = &cobra.Command{
	Use:   "naples-profile",
	Short: "naples profile object",
	Long:  "\n----------------------------\n Delete NAPLES Profiles \n----------------------------\n",
	RunE:  naplesProfileDeleteCmdHandler,
	Args:  naplesProfileDeleteCmdValidator,
}

var naplesProfileUpdateCmd = &cobra.Command{
	Use:   "naples-profile",
	Short: "naples profile object",
	Long:  "\n----------------------------\n Update NAPLES Profiles \n----------------------------\n",
	RunE:  naplesProfileUpdateCmdHandler,
	Args:  naplesProfileCreateCmdValidator,
}

var numLifs int32
var controllers []string
var managedBy, managementNetwork, priMac, id, mgmtIP, defaultGW, naplesProfile, profileName, portDefault string
var dnsServers []string

func init() {
	updateCmd.AddCommand(naplesCmd)
	showCmd.AddCommand(naplesShowCmd, naplesProfileShowCmd)
	createCmd.AddCommand(naplesProfileCreateCmd)
	deleteCmd.AddCommand(naplesProfileDeleteCmd)
	updateCmd.AddCommand(naplesProfileUpdateCmd)

	naplesCmd.Flags().StringSliceVarP(&controllers, "controllers", "c", make([]string, 0), "List of controller IP addresses or ids")
	naplesCmd.Flags().StringVarP(&managedBy, "managed-by", "o", "host", "NAPLES Management. host or network")
	naplesCmd.Flags().StringVarP(&managementNetwork, "management-network", "k", "", "Management Network. inband or oob")
	naplesCmd.Flags().StringVarP(&priMac, "primary-mac", "p", "", "Primary mac")
	naplesCmd.Flags().StringVarP(&id, "id", "i", "", "Naples ID")
	naplesCmd.Flags().StringVarP(&mgmtIP, "mgmt-ip", "m", "", "Management IP in CIDR format")
	naplesCmd.Flags().StringVarP(&defaultGW, "default-gw", "g", "", "Default GW for mgmt")
	naplesCmd.Flags().StringVarP(&naplesProfile, "naples-profile", "f", "default", "Active NAPLES Profile")
	naplesCmd.Flags().StringSliceVarP(&dnsServers, "dns-servers", "d", make([]string, 0), "List of DNS servers")
	naplesProfileCreateCmd.Flags().StringVarP(&profileName, "name", "n", "", "Name of the NAPLES profile to be created")
	naplesProfileCreateCmd.Flags().Int32VarP(&numLifs, "num-lifs", "i", 1, "Maximum number of LIFs on the eth device. 1 or 16")
	naplesProfileCreateCmd.Flags().StringVarP(&portDefault, "port-default", "p", "enable", "Set default port admin state for next reboot. (enable | disable)")

	naplesProfileDeleteCmd.Flags().StringVarP(&profileName, "name", "n", "", "Name of the NAPLES profile to be deleted")

	naplesProfileUpdateCmd.Flags().StringVarP(&profileName, "name", "n", "", "Name of the NAPLES profile to be created")
	naplesProfileUpdateCmd.Flags().Int32VarP(&numLifs, "num-lifs", "i", 1, "Maximum number of LIFs on the eth device. 1 or 16")
	naplesProfileUpdateCmd.Flags().StringVarP(&portDefault, "port-default", "p", "enable", "Set default port admin state for next reboot. (enable | disable)")
}

func naplesCmdHandler(cmd *cobra.Command, args []string) error {
	var networkManagementMode nmd.NetworkMode
	var managementMode nmd.MgmtMode

	if managedBy == "host" {
		managementMode = nmd.MgmtMode_HOST
		managementNetwork = ""

		// check if profile exists.
		if err := checkProfileExists(naplesProfile); err != nil {
			return err
		}

	} else {
		managementMode = nmd.MgmtMode_NETWORK
		// TODO : Remove this check once we have means to automatically generated a
		// id on Naples, if id is not explicitly passed for network managed mode.
		if id == "" {
			return errors.New("id parameter cannot be empty")
		}
	}

	if managementNetwork == "inband" {
		networkManagementMode = nmd.NetworkMode_INBAND
	} else if managementNetwork == "oob" {
		networkManagementMode = nmd.NetworkMode_OOB
	}

	naplesCfg := nmd.Naples{
		Spec: nmd.NaplesSpec{
			PrimaryMAC: priMac,
			ID:         id,
			IPConfig: &cluster.IPConfig{
				IPAddress:  mgmtIP,
				DefaultGW:  defaultGW,
				DNSServers: dnsServers,
			},
			Mode:          managementMode.String(),
			NetworkMode:   networkManagementMode.String(),
			Controllers:   controllers,
			NaplesProfile: naplesProfile,
		},
	}

	err := restPost(naplesCfg, "api/v1/naples/")
	if err != nil {
		if strings.Contains(err.Error(), "EOF") {
			if verbose {
				fmt.Println("success")
			}
		} else {
			fmt.Printf("Unable to update naples, error: %v\n", err)
		}
		return err
	}

	if managedBy == "host" {
		fmt.Println("Changes applied. A reboot is required for the changes to take effect")
		return nil
	}

	fmt.Println("Changes applied. A reboot is required for the changes to take effect. Verify that 'penctl show naples' command says REBOOT_PENDING, prior to performing a reboot.")
	return nil
}

func naplesShowCmdHandler(cmd *cobra.Command, args []string) error {
	resp, err := restGet("api/v1/naples/")

	if err != nil {
		if verbose {
			fmt.Println(err.Error())
		}
		return err
	}
	naplesCfg := nmd.Naples{}
	json.Unmarshal(resp, &naplesCfg)
	if verbose {
		fmt.Printf("%+v\n", naplesCfg.Spec)
	}
	return nil
}

func naplesProfileShowCmdHandler(cmd *cobra.Command, args []string) error {
	resp, err := restGet("api/v1/naples/profiles/")

	if err != nil {
		if verbose {
			fmt.Println(err.Error())
		}
		return err
	}
	var profiles []nmd.NaplesProfile
	json.Unmarshal(resp, &profiles)
	if verbose {
		for _, p := range profiles {
			fmt.Println(strings.Trim(strings.Replace(fmt.Sprintf("%+v", p.Spec), " ", "\n", -1), "{}"))
			fmt.Println("----")
		}
	}
	return nil
}

func naplesProfileCreateCmdHandler(cmd *cobra.Command, args []string) error {
	var portState string

	if portDefault == "enable" {
		portState = nmd.PortAdminState_PORT_ADMIN_STATE_ENABLE.String()
	} else if portDefault == "disable" {
		portState = nmd.PortAdminState_PORT_ADMIN_STATE_DISABLE.String()
	}

	profile := nmd.NaplesProfile{
		TypeMeta: api.TypeMeta{Kind: "NaplesProfile"},
		ObjectMeta: api.ObjectMeta{
			Name: profileName,
		},
		Spec: nmd.NaplesProfileSpec{
			NumLifs:          numLifs,
			DefaultPortAdmin: portState,
		},
	}

	err := restPost(profile, "api/v1/naples/profiles/")
	if err != nil {
		if strings.Contains(err.Error(), "EOF") {
			if verbose {
				fmt.Println("success")
			}
		} else {
			fmt.Println("Unable to create profile.")
			fmt.Println("Error:", err.Error())
		}
		return err
	}
	return err
}

func naplesProfileDeleteCmdHandler(cmd *cobra.Command, args []string) error {
	// check if profile exists.
	if err := checkProfileExists(profileName); err != nil {
		return err
	}

	// check if currently attached profile is being deleted
	if err := checkAttachedProfile(profileName); err != nil {
		return err
	}

	url := fmt.Sprintf("api/v1/naples/profiles/%s", profileName)
	_, err := restDelete(url)
	return err
}

func naplesProfileCreateCmdValidator(cmd *cobra.Command, args []string) (err error) {
	if len(profileName) == 0 {
		err = errors.New("must specify a naples profile name")
		return
	}

	if !(numLifs == 1 || numLifs == 16) {
		err = errors.New("number of LIFs not supported. --num-lifs should either be 1 or 16")
	}
	return
}

func naplesProfileUpdateCmdHandler(cmd *cobra.Command, args []string) error {
	var portState string

	// check if currently attached profile is being deleted
	if err := checkAttachedProfile(profileName); err != nil {
		fmt.Printf("%v is currently applied, all changes will be applicable after reboot only\n", profileName)
	}

	if portDefault == "enable" {
		portState = nmd.PortAdminState_PORT_ADMIN_STATE_ENABLE.String()
	} else if portDefault == "disable" {
		portState = nmd.PortAdminState_PORT_ADMIN_STATE_DISABLE.String()
	}

	profile := nmd.NaplesProfile{
		TypeMeta: api.TypeMeta{Kind: "NaplesProfile"},
		ObjectMeta: api.ObjectMeta{
			Name: profileName,
		},
		Spec: nmd.NaplesProfileSpec{
			NumLifs:          numLifs,
			DefaultPortAdmin: portState,
		},
	}

	err := restPut(profile, "api/v1/naples/profiles/")
	if err != nil {
		if strings.Contains(err.Error(), "EOF") {
			if verbose {
				fmt.Println("success")
			}
		} else {
			fmt.Println("Unable to update profile.")
			fmt.Println("Error:", err.Error())
		}
		return err
	}
	return err
}

func naplesCmdValidator(cmd *cobra.Command, args []string) (err error) {
	// Host Mode Validations
	switch managedBy {
	case "host":
		if len(managementNetwork) != 0 || len(controllers) != 0 || len(defaultGW) != 0 || len(dnsServers) != 0 || len(mgmtIP) != 0 || len(priMac) != 0 {
			err = errors.New("specified options, --managementNetwork, --controllers, --default-gw, --dns-servers, --mgmt-ip --primary-mac are not applicable when NAPLES is managed by host")
			return
		}

		if len(id) != 0 {
			vErr := vldtor.HostAddr(id)
			if vErr != nil {
				err = fmt.Errorf("invalid id %v: %s", id, vErr.Error())
				return
			}
		}
		return

	case "network":
		if len(managementNetwork) == 0 {
			err = fmt.Errorf("network management mode needs an accompanying --management-network. Supported values are inband and oob")
		}

		if !(managementNetwork == "oob" || managementNetwork == "inband") {
			err = fmt.Errorf("invalid management network %v specified. Must be inband or oob", managementNetwork)
			return
		}

		for _, c := range controllers {
			if vldtor.HostAddr(c) != nil {
				err = fmt.Errorf("invalid controller %v specified. Must be either IP Addresses or FQDNs", c)
				return
			}
		}

		if len(defaultGW) != 0 && vldtor.IPAddr(defaultGW) != nil {
			err = fmt.Errorf("invalid default gateway %v", defaultGW)
			return
		}

		for _, d := range dnsServers {
			if vldtor.IPAddr(d) != nil {
				err = fmt.Errorf("invalid dns server %v specified. Must be a valid IP Addresses", d)
				return
			}
		}

		if len(mgmtIP) != 0 && vldtor.CIDR(mgmtIP) != nil {
			err = fmt.Errorf("invalid management IP %v specified. Must be in CIDR Format", mgmtIP)
			return
		}

		if len(priMac) != 0 {
			priMac, err = strconv.ParseMacAddr(priMac)
			if err != nil {
				err = fmt.Errorf("invalid MAC Address %v specified. Conversion failed. Err : %v", priMac, err)
				return
			}
		}

		if len(naplesProfile) != 0 && naplesProfile != "default" {
			err = fmt.Errorf("naples profile is not applicable when NAPLES is manged by network")
		}

		if len(id) != 0 {
			vErr := vldtor.HostAddr(id)
			if vErr != nil {
				err = fmt.Errorf("invalid id %v: %s", id, vErr.Error())
				return
			}
			return
		}
		return

	default:
		err = fmt.Errorf("invalid --managed-by  %v flag specified. Must be either host or network", managedBy)
		err = fmt.Errorf("invalid --managed-by  %v flag specified. Must be either host or network", managedBy)
		return
	}
}

func naplesProfileDeleteCmdValidator(cmd *cobra.Command, args []string) error {
	if len(profileName) == 0 {
		return errors.New("must specify a naples profile name")
	}

	if profileName == "default" {
		return errors.New("deleting default profile is disallowed")
	}

	if len(portDefault) == 0 {
		return errors.New("port default must be passed")
	}

	if portDefault != "enable" && portDefault != "disable" {
		return errors.New("invalid port default value set")
	}

	return nil
}

// TODO remove this when penctl can display server sent errors codes. This fails due to revproxy and only on verbose mode gives a non descript 500 Internal Server Error
func checkProfileExists(profileName string) error {
	var ok bool
	baseURL, _ := getNaplesURL()
	url := fmt.Sprintf("%s/api/v1/naples/profiles/", baseURL)

	resp, err := penHTTPClient.Get(url)
	if err != nil {
		if strings.Contains(err.Error(), httpsSignature) {
			err = fmt.Errorf("Naples is part of a cluster, authentication token required")
		}
		fmt.Println("Failed to get existing profiles. Err: ", err)
		return fmt.Errorf("Failed to get existing profiles. Err: %v", err)
	}
	defer resp.Body.Close()

	body, _ := ioutil.ReadAll(resp.Body)
	var profiles []*nmd.NaplesProfile
	json.Unmarshal(body, &profiles)

	for _, p := range profiles {
		if profileName == p.Name {
			ok = true
			break
		}
	}

	if !ok {
		return fmt.Errorf("specified profile %v doesn't exist", profileName)
	}

	return nil
}

func checkAttachedProfile(profileName string) error {
	baseURL, _ := getNaplesURL()
	url := fmt.Sprintf("%s/api/v1/naples/", baseURL)

	resp, err := penHTTPClient.Get(url)
	if err != nil {
		if strings.Contains(err.Error(), httpsSignature) {
			err = fmt.Errorf("Naples is part of a cluster, authentication token required")
		}
		fmt.Println("Failed to get existing profiles. Err: ", err)
		return fmt.Errorf("failed to get existing profiles. Err: %v", err)
	}
	defer resp.Body.Close()

	body, _ := ioutil.ReadAll(resp.Body)
	var naplesCfg nmd.Naples
	json.Unmarshal(body, &naplesCfg)

	if naplesCfg.Spec.NaplesProfile == profileName {
		return fmt.Errorf("naples profile %v is currently in use", profileName)
	}

	return nil
}
