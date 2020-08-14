// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

// +build apulu

package apulu

import (
	"context"
	"encoding/json"
	"fmt"
	"net"
	"strconv"
	"strings"

	"github.com/pkg/errors"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/apulu/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	"github.com/pensando/sw/nic/agent/protos/tpmprotos"
	operdapi "github.com/pensando/sw/nic/infra/operd/daemon/gen/operd"
	"github.com/pensando/sw/venice/utils/log"
)

// HandleFwlogPolicyConfig creates a fwlog policy
func HandleFwlogPolicyConfig(infraAPI types.InfraAPI, client operdapi.SyslogSvcClient, oper types.Operation, policy tpmprotos.FwlogPolicy) error {
	switch oper {
	case types.Create:
		return createFwlogPolicyConfigHandler(infraAPI, client, policy)
	case types.Update:
		return updateFwlogPolicyConfigHandler(client, policy)
	case types.Delete:
		return deleteFwlogPolicyConfigHandler(client, policy)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createFwlogPolicyConfigHandler(infraAPI types.InfraAPI, client operdapi.SyslogSvcClient, policy tpmprotos.FwlogPolicy) error {
	err := validatePolicy(policy)
	if err != nil {
		log.Errorf("invalid fwlogPolicy, err: %v", err)
	}

	reqs := getCollectors(infraAPI, policy)
	for _, req := range reqs {
		resp, err := client.SyslogConfigCreate(context.Background(), &req)
		if err != nil || resp == nil {
			log.Errorf("fwlogPolicy Create returned error, req: %v err: %v", req, err)
			return errors.Wrapf(err, "failed to create fwlogPolicy in Operd")
		}

		if err := utils.HandleErr(types.Create, resp.ApiStatus, err, fmt.Sprintf("FwlogPolicy create Failed")); err != nil {
			return err
		}

		log.Infof("fwlogPolicy create %v returned | Status: %s", req, resp.ApiStatus)
	}

	return nil
}

func updateFwlogPolicyConfigHandler(client operdapi.SyslogSvcClient, policy tpmprotos.FwlogPolicy) error {
	return nil
}

func deleteFwlogPolicyConfigHandler(client operdapi.SyslogSvcClient, policy tpmprotos.FwlogPolicy) error {
	return nil
}

func validatePolicy(policy tpmprotos.FwlogPolicy) error {
	s := policy.Spec
	if len(s.Targets) == 0 {
		return fmt.Errorf("no collectors configured")
	}

	if _, ok := monitoring.MonitoringExportFormat_vvalue[s.Format]; !ok {
		return fmt.Errorf("invalid format %v", s.Format)
	}

	if s.Config != nil {
		if _, ok := monitoring.SyslogFacility_vvalue[s.Config.FacilityOverride]; !ok {
			return fmt.Errorf("invalid facility override %v", s.Config.FacilityOverride)
		}

		if s.Config.Prefix != "" {
			return fmt.Errorf("prefix is not allowed in firewall log")
		}
	}

	collectors := map[string]bool{}
	for _, c := range s.Targets {
		if key, err := json.Marshal(c); err == nil {
			ks := string(key)
			if _, ok := collectors[ks]; ok {
				return fmt.Errorf("found duplicate collector %v %v", c.Destination, c.Transport)
			}
			collectors[ks] = true
		}

		if c.Destination == "" {
			return fmt.Errorf("cannot configure empty collector")
		}

		netIP, _, err := net.ParseCIDR(c.Destination)
		if err != nil {
			netIP = net.ParseIP(c.Destination)
		}

		if netIP == nil {
			// treat it as hostname and resolve
			if _, err := net.LookupHost(c.Destination); err != nil {
				return fmt.Errorf("failed to resolve name %s, error: %v", c.Destination, err)
			}
		}

		tr := strings.Split(c.Transport, "/")
		if len(tr) != 2 {
			return fmt.Errorf("transport should be in protocol/port format")
		}

		if _, ok := map[string]bool{
			"tcp": true,
			"udp": true,
		}[strings.ToLower(tr[0])]; !ok {
			return fmt.Errorf("invalid protocol %v\n Accepted protocols: TCP, UDP", tr[0])
		}

		port, err := strconv.Atoi(tr[1])
		if err != nil {
			return fmt.Errorf("invalid port %v", tr[1])
		}

		if uint(port) > uint(^uint16(0)) {
			return fmt.Errorf("invalid port %v (> %d)", port, ^uint16(0))
		}
	}

	return nil
}

func getCollectors(infraAPI types.InfraAPI, policy tpmprotos.FwlogPolicy) []operdapi.SyslogConfigRequest {
	collectors := []operdapi.SyslogConfigRequest{}

	s := policy.Spec
	for _, target := range s.Targets {
		dest := target.Destination
		transport := strings.Split(target.Transport, "/")
		proto := transport[0]
		port, _ := strconv.Atoi(transport[1])

		var format operdapi.SyslogProtocol
		if strings.Contains(policy.Spec.Format, "bsd") {
			format = operdapi.SyslogProtocol_BSD
		} else {
			format = operdapi.SyslogProtocol_RFC
		}

		var facility uint32
		if policy.Spec.Config != nil {
			facility = uint32(monitoring.SyslogFacility_vvalue[policy.Spec.Config.FacilityOverride])
		}

		key := []string{policy.ObjectMeta.Name, policy.Spec.Format, proto, dest, fmt.Sprintf("%d", port)}
		name := strings.Join(key, ":")

		sc := operdapi.SyslogConfigRequest{
			Config: &operdapi.SyslogConfig{
				ConfigName: name,
				RemoteAddr: dest,
				RemotePort: uint32(port),
				Protocol:   format,
				Facility:   facility,
				Hostname:   infraAPI.GetConfig().DSCID,
				AppName:    "operd",
				ProcID:     "-",
			},
		}
		collectors = append(collectors, sc)
	}

	return collectors
}
