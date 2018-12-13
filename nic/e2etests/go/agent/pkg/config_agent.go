package pkg

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"reflect"
	"time"

	"path/filepath"

	"github.com/pensando/sw/api/generated/monitoring"
	"github.com/pensando/sw/nic/agent/netagent/ctrlerif/restapi"
	"github.com/pensando/sw/nic/agent/netagent/protos/netproto"
	"github.com/pensando/sw/venice/ctrler/tsm/rpcserver/tsproto"
	"github.com/pensando/sw/venice/utils/netutils"
)

func ConfigAgent(c *Config, manifestFile string) error {

	agentConfig, err := GetAgentConfig(c, manifestFile)
	if err != nil {
		return err
	}
	agentConfig.push()
	return nil
}

type AgentConfig struct {
	Namespaces         []netproto.Namespace
	Networks           []netproto.Network
	Endpoints          []netproto.Endpoint
	SgPolicies         []netproto.SGPolicy
	IPSecSAEncryption  []netproto.IPSecSAEncrypt
	IPSecSADecryption  []netproto.IPSecSADecrypt
	IPSecPolicies      []netproto.IPSecPolicy
	MirrorSessions     []tsproto.MirrorSession
	NatPools           []netproto.NatPool
	NatBindings        []netproto.NatBinding
	NatPolicies        []netproto.NatPolicy
	Tunnels            []netproto.Tunnel
	TCPProxies         []netproto.TCPProxyPolicy
	FlowExportPolicies []monitoring.FlowExportPolicy
	restApiMap         map[reflect.Type]string
}

//GetAgentConfig Get Configuration in agent Format to be consumed by traffic gen.
func GetAgentConfig(c *Config, manifestFile string) (*AgentConfig, error) {
	agentConfig := AgentConfig{}
	agentConfig.restApiMap = make(map[reflect.Type]string)
	for _, o := range c.Objects {
		err := o.populateAgentConfig(manifestFile, &agentConfig)
		if err != nil {
			fmt.Println("Stuff failed at getting cgfg: ", err)
			return nil, err
		}
	}
	return &agentConfig, nil
}

func (o *Object) populateAgentConfig(manifestFile string, agentCfg *AgentConfig) error {

	// Automatically interpret the the base dir of the manifest file as the config dir to dump all the generated files
	configDir, _ := filepath.Split(manifestFile)
	specFile := fmt.Sprintf("%s%s", configDir, o.SpecFile)

	dat, err := ioutil.ReadFile(specFile)
	if err != nil {
		return err
	}

	kindMap := map[string]interface{}{
		"Namespace":        &agentCfg.Namespaces,
		"Network":          &agentCfg.Networks,
		"Endpoint":         &agentCfg.Endpoints,
		"SGPolicy":         &agentCfg.SgPolicies,
		"MirrorSession":    &agentCfg.MirrorSessions,
		"IPSecSAEncrypt":   &agentCfg.IPSecSAEncryption,
		"IPSecSADecrypt":   &agentCfg.IPSecSADecryption,
		"IPSecPolicy":      &agentCfg.IPSecPolicies,
		"NatPool":          &agentCfg.NatPools,
		"NatBinding":       &agentCfg.NatBindings,
		"NatPolicy":        &agentCfg.NatPolicies,
		"Tunnel":           &agentCfg.Tunnels,
		"TCPProxyPolicy":   &agentCfg.TCPProxies,
		"FlowExportPolicy": &agentCfg.FlowExportPolicies,
	}

	err = json.Unmarshal(dat, kindMap[o.Kind])
	if err != nil {
		return err
	}

	agentCfg.restApiMap[reflect.TypeOf(kindMap[o.Kind])] = o.RestEndpoint

	return nil
}

func (agentCfg *AgentConfig) push() error {
	doConfig := func(config interface{}, restURL string) error {
		var resp restapi.Response
		err := netutils.HTTPPost(restURL, config, &resp)
		if err != nil {
			agentCfg.printErr(err, resp)
			return err
		}
		time.Sleep(time.Millisecond)
		return nil
	}

	fmt.Printf("Creating %d Namespaces...\n", len(agentCfg.Namespaces))
	restURL := fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.Namespaces)])
	for _, ns := range agentCfg.Namespaces {
		doConfig(ns, restURL)

	}

	fmt.Printf("Creating %d Networks...\n", len(agentCfg.Networks))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.Networks)])
	for _, nt := range agentCfg.Networks {
		doConfig(nt, restURL)
	}
	fmt.Printf("Creating %d Endpoints...\n", len(agentCfg.Endpoints))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.Endpoints)])
	for _, ep := range agentCfg.Endpoints {
		doConfig(ep, restURL)
	}

	fmt.Printf("Configuring %d Tunnels...\n", len(agentCfg.Tunnels))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.Tunnels)])
	for _, ms := range agentCfg.Tunnels {
		doConfig(ms, restURL)
	}

	fmt.Printf("Configuring %d SGPolicies...\n", len(agentCfg.SgPolicies))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.SgPolicies)])
	for _, ep := range agentCfg.SgPolicies {
		doConfig(ep, restURL)
	}

	fmt.Printf("Configuring %d Mirror Sessions...\n", len(agentCfg.MirrorSessions))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.MirrorSessions)])
	for _, ms := range agentCfg.MirrorSessions {
		doConfig(ms, restURL)
	}

	fmt.Printf("Configuring %d IPSec SA Encrypt Rules...\n", len(agentCfg.IPSecSAEncryption))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.IPSecSAEncryption)])
	for _, saEncrypt := range agentCfg.IPSecSAEncryption {
		doConfig(saEncrypt, restURL)
	}

	fmt.Printf("Configuring %d IPSec SA Decrypt Rules...\n", len(agentCfg.IPSecSADecryption))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.IPSecSADecryption)])
	for _, saDecrypt := range agentCfg.IPSecSADecryption {
		doConfig(saDecrypt, restURL)
	}

	fmt.Printf("Configuring %d IPSec Policies...\n", len(agentCfg.IPSecPolicies))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.IPSecPolicies)])
	for _, ipSec := range agentCfg.IPSecPolicies {
		doConfig(ipSec, restURL)
	}

	fmt.Printf("Configuring %d Nat Pools...\n", len(agentCfg.NatPools))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.NatPools)])
	for _, np := range agentCfg.NatPools {
		doConfig(np, restURL)
	}

	fmt.Printf("Configuring %d Flow Export Policies...\n", len(agentCfg.FlowExportPolicies))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.FlowExportPolicies)])
	for _, fe := range agentCfg.FlowExportPolicies {
		doConfig(fe, restURL)
	}

	fmt.Printf("Configuring %d TCP Proxies...\n", len(agentCfg.TCPProxies))
	restURL = fmt.Sprintf("%s%s", AGENT_URL,
		agentCfg.restApiMap[reflect.TypeOf(&agentCfg.TCPProxies)])
	for _, tcp := range agentCfg.TCPProxies {
		doConfig(tcp, restURL)
	}
	return nil
}

func (o *AgentConfig) printErr(err error, resp restapi.Response) {
	fmt.Printf("Agent configuration failed with. Err: %v\n", err)
	fmt.Println("######### RESPONSE #########")
	b, _ := json.MarshalIndent(resp, "", "   ")
	fmt.Println(string(b))
}
