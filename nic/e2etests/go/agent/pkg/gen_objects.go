package pkg

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"

	"gopkg.in/yaml.v2"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/e2etests/go/agent/pkg/libs"
	"github.com/pensando/sw/venice/ctrler/npm/rpcserver/netproto"
	"github.com/pensando/sw/venice/ctrler/tsm/rpcserver/tsproto"
)

var objCache map[string]Object

// networkCache maintains a mapping of network name and the assigned subnet
var networkCache map[string]string

// endpointCache maintains a mapping of ep name and the assigned IP Address
var endpointCache map[string]string

// networkNSCache maintains a mapping of network name and its namespace
var networkNSCache map[string]string

// tunnelCache maintains a mapping of tunnel name and its dest IP Address
var tunnelCache map[string]string

var forceGeneration = false

func genRequired(o *Object) bool {
	if !forceGeneration && len(o.SpecFile) > 0 {
		return false
	}

	return true
}

func GenerateObjectsFromManifest(manifestFile string, sDevices []StationDevice, vlanOffset int, forceGen bool) (*Config, error) {
	objCache = make(map[string]Object)
	networkCache = make(map[string]string)
	networkNSCache = make(map[string]string)
	endpointCache = make(map[string]string)
	tunnelCache = make(map[string]string)
	forceGeneration = forceGen

	dat, err := ioutil.ReadFile(manifestFile)
	if err != nil {
		return nil, err
	}

	var c Config
	err = yaml.Unmarshal(dat, &c)
	if err != nil {
		return nil, err
	}

	// build obj cache for easier lookups
	for _, o := range c.Objects {
		objCache[o.Kind] = o
	}

	// generate configs
	err = c.generateObjs(manifestFile, sDevices, vlanOffset)
	if err != nil {
		return nil, err
	}
	return &c, nil
}

func GetObjectsFromManifest(manifestFile string) (*Config, error) {
	objCache = make(map[string]Object)
	networkCache = make(map[string]string)
	networkNSCache = make(map[string]string)

	dat, err := ioutil.ReadFile(manifestFile)
	if err != nil {
		return nil, err
	}

	var c Config
	err = yaml.Unmarshal(dat, &c)
	if err != nil {
		return nil, err
	}

	// build obj cache for easier lookups
	for _, o := range c.Objects {
		objCache[o.Kind] = o
	}

	for i, o := range c.Objects {
		c.Objects[i] = o
	}
	return &c, nil
}

func (c *Config) generateObjs(manifestFile string, sdevices []StationDevice, vlanOffset int) error {
	for i, o := range c.Objects {
		switch o.Kind {
		case "Namespace":
			genObj, err := c.generateNamespaces(&o, manifestFile)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "Network":
			genObj, err := c.generateNetworks(&o, manifestFile, vlanOffset)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "Tunnel":
			genObj, err := c.generateTunnels(&o, manifestFile)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "Endpoint":
			genObj, err := c.generateEndpoints(&o, manifestFile, sdevices)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "SGPolicy":
			genObj, err := c.generateSGPolicies(&o, manifestFile)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "MirrorSession":
			genObj, err := c.generateMirrorSessions(&o, manifestFile)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "IPSecSAEncrypt":
			genObj, err := c.generateIPSecSAEncrypt(&o, manifestFile)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "IPSecSADecrypt":
			genObj, err := c.generateIPSecSADecrypt(&o, manifestFile)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "IPSecPolicy":
			genObj, err := c.generateIPSecPolicy(&o, manifestFile)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "NatPool":
			genObj, err := c.generateNatPool(&o, manifestFile)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "NatBinding":
			genObj, err := c.generateNatBinding(&o, manifestFile)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		case "NatPolicy":
			genObj, err := c.generateNatPolicy(&o, manifestFile)
			if err != nil {
				return err
			}
			c.Objects[i] = *genObj
		}
	}

	return nil
}

func (c *Config) generateNamespaces(o *Object, manifestFile string) (*Object, error) {
	var namespaces []netproto.Namespace
	specFile := "generated/namespaces.json"
	if !genRequired(o) {
		return o, nil
	}

	for i := 0; i < o.Count; i++ {
		name := fmt.Sprintf("%s-%d", o.Name, i)
		ns := netproto.Namespace{
			TypeMeta: api.TypeMeta{Kind: "Namespace"},
			ObjectMeta: api.ObjectMeta{
				Tenant: "default",
				Name:   name,
			},
		}
		namespaces = append(namespaces, ns)
	}
	out, err := json.MarshalIndent(namespaces, "", "  ")
	if err != nil {
		return nil, err
	}

	// Automatically interpret the the base dir of the manifest file as the config dir to dump all the generated files
	configDir, _ := filepath.Split(manifestFile)
	fileName := fmt.Sprintf("%s%s", configDir, specFile)

	// create a generated dir in the config directory to dump the json
	genDir, _ := filepath.Split(fileName)
	if _, err := os.Stat(genDir); os.IsNotExist(err) {
		err = os.MkdirAll(genDir, 0755)
		if err != nil {
			return nil, fmt.Errorf("creating the generated directory failed. Err: %v", err)
		}
	}

	err = ioutil.WriteFile(fileName, out, 0644)
	if err != nil {
		return nil, err
	}
	o.SpecFile = specFile
	return o, nil
}

func (c *Config) generateNetworks(o *Object, manifestFile string, vlanOffset int) (*Object, error) {
	var networks []netproto.Network
	specFile := "generated/networks.json"
	if !genRequired(o) {
		return o, nil
	}

	subnets := libs.GenSubnets(o.Count)

	// Networks need to refer to Namespaces
	namespaceRef := objCache["Namespace"]

	// Infra Network is needed as a wrapper to hold all items infra vrf. Eg: Remote TEP for tunnels, Mirror Session ERSPAN Dest
	infraNw := netproto.Network{
		TypeMeta: api.TypeMeta{Kind: "Network"},
		ObjectMeta: api.ObjectMeta{
			Tenant:    "default",
			Namespace: "infra",
			Name:      "infra-nw",
		},
		Spec: netproto.NetworkSpec{
			IPv4Subnet:  "42.42.42.42/24",
			IPv4Gateway: "42.0.0.1",
			VlanID:      42,
		},
	}
	networkCache["infra-nw"] = "42.42.42.42/24"
	networks = append(networks, infraNw)

	// generate networks distributed evenly across
	for i := 0; i < o.Count; i++ {
		var name, namespace string
		namespace = fmt.Sprintf("%s-%d", namespaceRef.Name, i%namespaceRef.Count)
		name = fmt.Sprintf("%s-%d", o.Name, i)

		subnet := subnets[i]
		_, gwIP, _ := libs.GenIPAddress(subnet, 4)
		nt := netproto.Network{
			TypeMeta: api.TypeMeta{Kind: "Network"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: namespace,
				Name:      name,
			},
			Spec: netproto.NetworkSpec{
				IPv4Subnet:  subnet,
				IPv4Gateway: gwIP,
				VlanID:      uint32(vlanOffset+i) % 4096,
			},
		}
		networks = append(networks, nt)

		// update network cache with the generated subnets for any objects referring to it
		networkCache[name] = subnet
		networkNSCache[name] = namespace
	}

	out, err := json.MarshalIndent(&networks, "", "  ")
	if err != nil {
		return nil, err
	}

	// Automatically interpret the the base dir of the manifest file as the config dir to dump all the generated files
	configDir, _ := filepath.Split(manifestFile)

	fileName := fmt.Sprintf("%s%s", configDir, specFile)

	// create a generated dir in the config directory to dump the json
	genDir, _ := filepath.Split(fileName)
	if _, err := os.Stat(genDir); os.IsNotExist(err) {
		err = os.MkdirAll(genDir, 0755)
		if err != nil {
			return nil, fmt.Errorf("creating the generated directory failed. Err: %v", err)
		}
	}

	err = ioutil.WriteFile(fileName, out, 0644)
	if err != nil {
		return nil, err
	}
	o.SpecFile = specFile
	return o, nil
}

func (c *Config) generateEndpoints(o *Object, manifestFile string, sdevices []StationDevice) (*Object, error) {
	specFile := "generated/endpoints.json"

	var endpoints []netproto.Endpoint
	var epMACAddresses []string
	var err error
	var lifIDs []int
	epNetworkIPMap := make(map[string][]string)

	if !genRequired(o) {
		return o, nil
	}

	curLif := 0
	for i := 0; i < len(sdevices); i++ {
		//TODO : Have to create EP for station devices too.
		lifIDs = append(lifIDs, sdevices[i].LifID)
	}

	// EPs need to refer to Namespaces and Networks
	namespaceRef := objCache["Namespace"]
	networkRef := objCache["Network"]
	endpointsPerNetwork := o.Count / networkRef.Count
	epMACAddresses, err = libs.GenMACAddresses(o.Count + INFRA_EP_COUNT)
	if err != nil {
		return nil, fmt.Errorf("could not generate required MAC Addresses. %v", err)
	}

	// Reserve a block of network subnet IPs for EPs
	for i := 0; i < networkRef.Count; i++ {
		network := fmt.Sprintf("%s-%d", networkRef.Name, i)
		subnet := networkCache[network]
		ipAddrs, _, err := libs.GenIPAddress(subnet, endpointsPerNetwork)
		if err != nil {
			return nil, fmt.Errorf("could not generate %d EP IP Addresses from %v subnet EPs. %v", endpointsPerNetwork, subnet, err)
		}
		epNetworkIPMap[network] = ipAddrs
	}

	// Configure Tunnel EPs in Infra vrf
	infraSubnet := networkCache["infra-nw"]
	infraIPAddress, _, err := libs.GenIPAddress(infraSubnet, INFRA_EP_COUNT)
	if err != nil {
		return nil, fmt.Errorf("could not generate %d EP IP Addresses from infra subnet %v EPs. %v", INFRA_EP_COUNT, infraSubnet, err)
	}

	for i := 0; i < INFRA_EP_COUNT; i++ {
		name := fmt.Sprintf("infra-ep-%d", i)
		ipAddr := infraIPAddress[i]
		infraEP := netproto.Endpoint{
			TypeMeta: api.TypeMeta{Kind: "Endpoint"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: "infra",
				Name:      name,
			},
			Spec: netproto.EndpointSpec{
				NetworkName:   "infra-nw",
				UsegVlan:      uint32(i) + 1,
				IPv4Address:   fmt.Sprintf("%s/32", ipAddr),
				MacAddress:    epMACAddresses[i],
				InterfaceType: "uplink",
				Interface:     fmt.Sprintf("uplink%d", (i%UPLINK_COUNT)+UPLINK_START),
				NodeUUID:      "GWUUID",
			},
		}
		endpoints = append(endpoints, infraEP)

		// update ep cache cache with the generated ip address for the EP
		endpointCache[name] = ipAddr
	}

	for i := 0; i < o.Count; i++ {
		var name, namespace, network, ifType, ifName, nodeUUID, epIP string
		name = fmt.Sprintf("%s-%d", o.Name, i)
		//TODO FIX NW NAMES
		namespace = fmt.Sprintf("%s-%d", namespaceRef.Name, i%namespaceRef.Count)
		network = fmt.Sprintf("%s-%d", networkRef.Name, i%networkRef.Count)

		// Pop elements here instead of maintaining a complex index
		epIP, epNetworkIPMap[network] = epNetworkIPMap[network][0], epNetworkIPMap[network][1:]
		// Offset from Infra EP count index
		epMAC := epMACAddresses[i+INFRA_EP_COUNT]

		if i%o.Count < 16 {
			ifType = "uplink"
			ifName = fmt.Sprintf("uplink%d", ((i/networkRef.Count)%UPLINK_COUNT)+UPLINK_START)
			nodeUUID = "GWUUID" // This will ensure that the EP is remote
		} else {
			ifType = "lif"
			if len(lifIDs) == 0 {
				ifName = fmt.Sprintf("lif%d", (i%LIF_COUNT)+LIF_START)
			} else {
				ifName = fmt.Sprintf("lif%d", lifIDs[curLif])
				curLif = (curLif + 1) % len(lifIDs)
			}
		}
		ep := netproto.Endpoint{
			TypeMeta: api.TypeMeta{Kind: "Endpoint"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: namespace,
				Name:      name,
			},
			Spec: netproto.EndpointSpec{
				NetworkName:   network,
				UsegVlan:      uint32(i) + 1,
				IPv4Address:   fmt.Sprintf("%s/32", epIP),
				MacAddress:    epMAC,
				InterfaceType: ifType,
				Interface:     ifName,
				NodeUUID:      nodeUUID,
			},
		}
		endpoints = append(endpoints, ep)

		// update ep cache cache with the generated ip address for the EP
		endpointCache[name] = epIP

	}

	out, err := json.MarshalIndent(endpoints, "", "  ")
	if err != nil {
		return nil, err
	}

	// Automatically interpret the the base dir of the manifest file as the config dir to dump all the generated files
	configDir, _ := filepath.Split(manifestFile)
	fileName := fmt.Sprintf("%s%s", configDir, specFile)

	// create a generated dir in the config directory to dump the json
	genDir, _ := filepath.Split(fileName)
	if _, err := os.Stat(genDir); os.IsNotExist(err) {
		err = os.MkdirAll(genDir, 0755)
		if err != nil {
			return nil, fmt.Errorf("creating the generated directory failed. Err: %v", err)
		}
	}

	err = ioutil.WriteFile(fileName, out, 0755)
	if err != nil {
		return nil, err
	}
	o.SpecFile = specFile
	return o, nil
}

func (c *Config) generateMirrorSessions(o *Object, manifestFile string) (*Object, error) {
	var mirrors []tsproto.MirrorSession
	var portOffset = 8000
	specFile := "generated/mirrors.json"
	if !genRequired(o) {
		return o, nil
	}

	// Mirror sessions need to refer to Namespaces
	namespaceRef := objCache["Namespace"]

	// Mirror sessions also need EP IP Addresses
	endpointRef := objCache["Endpoint"]
	networkRef := objCache["Network"]
	tunRef := objCache["Tunnel"]

	// generate networks distributed evenly across
	for i := 0; i < o.Count; i++ {
		name := fmt.Sprintf("%s-%d", o.Name, i)
		namespace := fmt.Sprintf("%s-%d", namespaceRef.Name, i%namespaceRef.Count)
		tunnel := fmt.Sprintf("%s-%d", tunRef.Name, i%tunRef.Count)
		greTunDst := tunnelCache[tunnel]

		localEpOffset := 0
		remoteEpOffset := 2
		epOffsetIncrements := endpointRef.Count / networkRef.Count // Gives the #EP per Network

		//networkName := networkCache[fmt.Sprintf("%s-%d", networkRef.Name, i%networkRef.Count)]

		local := fmt.Sprintf("%s-%d", endpointRef.Name, (localEpOffset+epOffsetIncrements*i)%endpointRef.Count)
		remote := fmt.Sprintf("%s-%d", endpointRef.Name, (remoteEpOffset+epOffsetIncrements*i)%endpointRef.Count)

		// Look up EP's IP Address
		localEP := endpointCache[local]
		remoteEP := endpointCache[remote]
		ms := tsproto.MirrorSession{
			TypeMeta: api.TypeMeta{Kind: "MirrorSession"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: namespace,
				Name:      name,
			},
			Spec: tsproto.MirrorSessionSpec{
				Enable:     true,
				PacketSize: 128,
				Collectors: []tsproto.MirrorCollector{
					{
						Type: "ERSPAN",
						ExportCfg: api.ExportConfig{
							Destination: greTunDst,
							Transport:   fmt.Sprintf("TCP/%d", portOffset+i),
						},
					},
				},
				PacketFilters: []string{"ALL_DROPS"},
				MatchRules: []tsproto.MatchRule{
					{
						Src: &tsproto.MatchSelector{
							IPAddresses: []string{"100.100.100.100"},
						},
						Dst: &tsproto.MatchSelector{
							IPAddresses: []string{localEP},
						},
					},
					{
						Src: &tsproto.MatchSelector{
							IPAddresses: []string{"100.100.100.100"},
						},
						Dst: &tsproto.MatchSelector{
							IPAddresses: []string{remoteEP},
						},
					},
				},
			},
		}
		mirrors = append(mirrors, ms)
	}
	out, err := json.MarshalIndent(&mirrors, "", "  ")
	if err != nil {
		return nil, err
	}

	// Automatically interpret the the base dir of the manifest file as the config dir to dump all the generated files
	configDir, _ := filepath.Split(manifestFile)

	fileName := fmt.Sprintf("%s%s", configDir, specFile)

	// create a generated dir in the config directory to dump the json
	genDir, _ := filepath.Split(fileName)
	if _, err := os.Stat(genDir); os.IsNotExist(err) {
		err = os.MkdirAll(genDir, 0755)
		if err != nil {
			return nil, fmt.Errorf("creating the generated directory failed. Err: %v", err)
		}
	}

	err = ioutil.WriteFile(fileName, out, 0644)
	if err != nil {
		return nil, err
	}
	o.SpecFile = specFile
	return o, nil
}
func (c *Config) generateNatPool(o *Object, manifestFile string) (*Object, error) {
	var natPools []netproto.NatPool
	specFile := "generated/nat_pools.json"
	if !genRequired(o) {
		return o, nil
	}

	// Nat Pools sessions need to refer to Namespaces
	namespaceRef := objCache["Namespace"]
	networkRef := objCache["Network"]

	// generate networks distributed evenly across
	for i := 0; i < o.Count; i++ {
		name := fmt.Sprintf("%s-%d", o.Name, i)
		namespace := fmt.Sprintf("%s-%d", namespaceRef.Name, i%namespaceRef.Count)
		network := fmt.Sprintf("%s-%d", networkRef.Name, i%networkRef.Count)
		subnet := networkCache[network]
		addrs, _, err := libs.GenIPAddress(subnet, 8)
		if err != nil {
			return nil, err
		}
		ipRange := fmt.Sprintf("%s-%s", addrs[0], addrs[len(addrs)-1])
		//endpoint := fmt.Sprintf("%s-%d", epRef.Name, i%epRef.Count)
		np := netproto.NatPool{
			TypeMeta: api.TypeMeta{Kind: "NatPool"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: namespace,
				Name:      name,
			},
			Spec: netproto.NatPoolSpec{
				IPRange: ipRange,
			},
		}
		natPools = append(natPools, np)
	}
	out, err := json.MarshalIndent(&natPools, "", "  ")
	if err != nil {
		return nil, err
	}

	// Automatically interpret the the base dir of the manifest file as the config dir to dump all the generated files
	configDir, _ := filepath.Split(manifestFile)

	fileName := fmt.Sprintf("%s%s", configDir, specFile)

	// create a generated dir in the config directory to dump the json
	genDir, _ := filepath.Split(fileName)
	if _, err := os.Stat(genDir); os.IsNotExist(err) {
		err = os.MkdirAll(genDir, 0755)
		if err != nil {
			return nil, fmt.Errorf("creating the generated directory failed. Err: %v", err)
		}
	}

	err = ioutil.WriteFile(fileName, out, 0644)
	if err != nil {
		return nil, err
	}
	o.SpecFile = specFile
	return o, nil
}

func (c *Config) generateTunnels(o *Object, manifestFile string) (*Object, error) {
	var tunnels []netproto.Tunnel
	specFile := "generated/tunnels.json"
	if !genRequired(o) {
		return o, nil
	}
	endpointRef := objCache["Endpoint"]
	namespaceRef := objCache["Namespace"]

	// generate gre tunnels to distribute evenly across namespaces.
	for i := 0; i < o.Count; i++ {
		name := fmt.Sprintf("%s-%d", o.Name, i)
		endpoint := fmt.Sprintf("%s-%d", endpointRef.Name, i%endpointRef.Count)
		endpointIP := endpointCache[endpoint]
		tun := netproto.Tunnel{
			TypeMeta: api.TypeMeta{Kind: "Tunnel"},
			ObjectMeta: api.ObjectMeta{
				Tenant:    "default",
				Namespace: fmt.Sprintf("%s-%d", namespaceRef.Name, i%namespaceRef.Count),
				Name:      name,
			},
			Spec: netproto.TunnelSpec{
				Type:        "GRE",
				AdminStatus: "UP",
				Src:         "100.100.100.100", // Fixed currently
				Dst:         endpointIP,
			},
		}
		tunnels = append(tunnels, tun)

		// Update tunnel cache with its dest ip
		tunnelCache[name] = tun.Spec.Dst
	}
	out, err := json.MarshalIndent(&tunnels, "", "  ")
	if err != nil {
		return nil, err
	}

	// Automatically interpret the the base dir of the manifest file as the config dir to dump all the generated files
	configDir, _ := filepath.Split(manifestFile)

	fileName := fmt.Sprintf("%s%s", configDir, specFile)

	// create a generated dir in the config directory to dump the json
	genDir, _ := filepath.Split(fileName)
	if _, err := os.Stat(genDir); os.IsNotExist(err) {
		err = os.MkdirAll(genDir, 0755)
		if err != nil {
			return nil, fmt.Errorf("creating the generated directory failed. Err: %v", err)
		}
	}

	err = ioutil.WriteFile(fileName, out, 0644)
	if err != nil {
		return nil, err
	}
	o.SpecFile = specFile
	return o, nil
}

func (c *Config) generateNatBinding(o *Object, manifestFile string) (*Object, error) {
	// TODO add auto gen.
	return o, nil
}

func (c *Config) generateNatPolicy(o *Object, manifestFile string) (*Object, error) {
	// TODO add auto gen.
	return o, nil
}

func (c *Config) generateSGPolicies(o *Object, manifestFile string) (*Object, error) {
	// TODO add auto gen.
	return o, nil
}

func (c *Config) generateIPSecSAEncrypt(o *Object, manifestFile string) (*Object, error) {
	// TODO add auto gen.
	return o, nil
}

func (c *Config) generateIPSecSADecrypt(o *Object, manifestFile string) (*Object, error) {
	// TODO add auto gen.
	return o, nil
}

func (c *Config) generateIPSecPolicy(o *Object, manifestFile string) (*Object, error) {
	// TODO add auto gen.
	return o, nil
}
