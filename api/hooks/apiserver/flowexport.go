package impl

import (
	"context"
	"encoding/json"
	"fmt"
	"net"
	"strconv"
	"strings"
	"time"

	govldtr "github.com/asaskevich/govalidator"

	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/monitoring"
	hooksutils "github.com/pensando/sw/api/hooks/apiserver/utils"
	apiintf "github.com/pensando/sw/api/interfaces"
	"github.com/pensando/sw/venice/apiserver"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/log"
)

type flowExpHooks struct {
	svc    apiserver.Service
	logger log.Logger
}

const (
	veniceMaxCollectorsPerPolicy = 2
	veniceMaxPolicySessions      = 8
	veniceMaxNetflowCollectors   = 8
)

func (r *flowExpHooks) validateFlowExportPolicy(ctx context.Context, kv kvstore.Interface, txn kvstore.Txn, key string, oper apiintf.APIOperType, dryRun bool, i interface{}) (interface{}, bool, error) {
	policy, ok := i.(monitoring.FlowExportPolicy)
	if ok != true {
		return i, false, fmt.Errorf("invalid object %T instead of monitoring.FlowExportPolicy", i)
	}
	// Perform validation only if Spec has changed
	// No change to spec indicates status update, for which no need to validate the spec
	oldfp := monitoring.FlowExportPolicy{}
	err := kv.Get(ctx, key, &oldfp)
	if err == nil {
		// found old object, compare spec
		if oper == apiintf.CreateOper {
			// Create on already existing flow export policy
			// return success and let api server take care of this error
			return i, true, nil
		}
	} else if oper == apiintf.UpdateOper {
		// update on non-existing flow export policy
		// return success and let api server take care of this error
		return i, true, nil
	}

	// perform spec validation
	if err := flowexportPolicyValidator(&policy); err != nil {
		return i, false, err
	}

	policyList := monitoring.FlowExportPolicyList{}
	policyKey := strings.TrimSuffix(policyList.MakeKey(string(apiclient.GroupMonitoring)), "/")
	err = kv.List(ctx, policyKey, &policyList)
	if err != nil {
		return nil, false, fmt.Errorf("error retrieving FlowExportPolicy: %v", err)
	}
	// perform global validation across policy
	if err := globalFlowExportValidator(&policy, &policyList); err != nil {
		return i, false, err
	}

	switch oper {
	case apiintf.CreateOper:
		if len(policyList.Items) >= veniceMaxPolicySessions {
			return nil, false, fmt.Errorf("can't configure more than %v FlowExportPolicy", veniceMaxPolicySessions)
		}
	}

	return i, true, nil
}

func registerFlowExpPolicyHooks(svc apiserver.Service, logger log.Logger) {
	r := flowExpHooks{}
	r.svc = svc
	r.logger = logger.WithContext("Service", "flowexphooks")
	logger.Log("msg", "registering Hooks")
	svc.GetCrudService("FlowExportPolicy", apiintf.CreateOper).WithPreCommitHook(r.validateFlowExportPolicy)
	svc.GetCrudService("FlowExportPolicy", apiintf.UpdateOper).WithPreCommitHook(r.validateFlowExportPolicy)
}

func flowexportPolicyValidator(p *monitoring.FlowExportPolicy) error {
	spec := p.Spec
	if _, err := validateFlowExportInterval(spec.Interval); err != nil {
		return err
	}

	if _, err := validateTemplateInterval(spec.TemplateInterval); err != nil {
		return err
	}

	if err := validateFlowExportFormat(spec.Format); err != nil {
		return err
	}

	matchrules := []monitoring.MatchRule{}
	for _, mr := range spec.MatchRules {
		matchrules = append(matchrules, *mr)
	}
	if err := hooksutils.ValidateMatchRules(matchrules); err != nil {
		return fmt.Errorf("error in match-rule, %v", err)
	}
	for _, mr := range spec.MatchRules {
		allSrc := false
		allDst := false
		if mr.Src != nil {
			if len(mr.Src.IPAddresses) == 0 && len(mr.Src.MACAddresses) == 0 {
				allSrc = true
			}

			for _, ip := range mr.Src.IPAddresses {
				if strings.ToLower(ip) == "any" {
					allSrc = true
				}
			}
			if allSrc && len(mr.Src.IPAddresses) > 1 {
				return fmt.Errorf("Match-all type cannot have multiple src match rules")
			}
			// TBD - Ensure only one of the three is specified? not all.
		}
		if mr.Dst != nil {
			if len(mr.Dst.IPAddresses) == 0 && len(mr.Dst.MACAddresses) == 0 {
				allDst = true
			}
			for _, ip := range mr.Dst.IPAddresses {
				if strings.ToLower(ip) == "any" {
					allDst = true
				}
			}
			if allDst && len(mr.Dst.IPAddresses) > 1 {
				return fmt.Errorf("Match-all type cannot have multiple dst match rules")
			}
		}
	}

	if len(spec.Exports) == 0 {
		return fmt.Errorf("no targets configured")
	}

	if len(spec.Exports) > veniceMaxCollectorsPerPolicy {
		return fmt.Errorf("cannot configure more than %d targets", veniceMaxCollectorsPerPolicy)
	}

	feTargets := map[string]bool{}
	for _, export := range spec.Exports {
		if key, err := json.Marshal(export); err == nil {
			ks := string(key)
			if _, ok := feTargets[ks]; ok {
				return fmt.Errorf("found duplicate target %v", export.Destination)
			}
			feTargets[ks] = true
		}

		if export.Gateway != "" && !govldtr.IsIPv4(export.Gateway) {
			return fmt.Errorf("Gateway can be empty or must be a valid IPv4 address")
		}

		dest := export.Destination
		if dest == "" {
			return fmt.Errorf("destination is empty")
		}

		netIP, _, err := net.ParseCIDR(dest)
		if err == nil {
			dest = netIP.String()
		} else {
			netIP = net.ParseIP(dest)
		}

		if netIP == nil {
			// treat it as hostname and resolve
			s, err := net.LookupHost(dest)
			if err != nil || len(s) == 0 {
				return fmt.Errorf("failed to resolve name {%s}, error: %s", dest, err)
			}
		}

		if _, err := parsePortProto(export.Transport); err != nil {
			return err
		}
	}

	return nil
}

func validateFlowExportInterval(s string) (time.Duration, error) {

	interval, err := time.ParseDuration(s)
	if err != nil {
		return interval, fmt.Errorf("invalid interval %s", s)
	}

	if interval < time.Second {
		return interval, fmt.Errorf("too small interval %s", s)
	}

	if interval > 24*time.Hour {
		return interval, fmt.Errorf("too large interval %s", s)
	}

	return interval, nil
}

func validateTemplateInterval(s string) (time.Duration, error) {

	interval, err := time.ParseDuration(s)
	if err != nil {
		return interval, fmt.Errorf("invalid template interval %s", s)
	}

	if interval < time.Minute {
		return interval, fmt.Errorf("too small template interval %s", s)
	}

	if interval > 30*time.Minute {
		return interval, fmt.Errorf("too large template interval %s", s)
	}

	return interval, nil
}

func validateFlowExportFormat(s string) error {
	if strings.ToUpper(s) != "IPFIX" {
		return fmt.Errorf("invalid format %s", s)
	}
	return nil
}

func parsePortProto(src string) (uint32, error) {
	s := strings.Split(src, "/")
	if len(s) != 2 {
		return 0, fmt.Errorf("transport should be in protocol/port format")
	}

	if strings.ToUpper(s[0]) != "UDP" {
		return 0, fmt.Errorf("invalid protocol in %s", src)
	}

	port, err := strconv.Atoi(s[1])
	if err != nil {
		return 0, fmt.Errorf("invalid port in %s", src)
	}
	if uint(port) > uint(^uint16(0)) {
		return 0, fmt.Errorf("invalid port in %s", src)
	}

	return uint32(port), nil
}

func globalFlowExportValidator(newfp *monitoring.FlowExportPolicy, policyList *monitoring.FlowExportPolicyList) error {
	totalCollectors := len(newfp.Spec.Exports)
	for _, policy := range policyList.Items {
		if policy.Name == newfp.Name {
			continue
		}
		totalCollectors = totalCollectors + len(newfp.Spec.Exports)
	}
	if totalCollectors > veniceMaxNetflowCollectors {
		return fmt.Errorf("can't configure more than %v netflow collectors", veniceMaxNetflowCollectors)
	}
	return nil
}
