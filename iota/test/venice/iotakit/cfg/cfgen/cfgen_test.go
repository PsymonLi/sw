package cfgen

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"testing"

	"github.com/pensando/sw/api/generated/cluster"
	"github.com/pensando/sw/venice/globals"
)

func TestCfgenPolicyGen(t *testing.T) {
	cfg := DefaultCfgenParams
	cfg.NetworkSecurityPolicyParams.NumPolicies = 1
	cfg.NetworkSecurityPolicyParams.NumRulesPerPolicy = 5
	cfg.WorkloadParams.WorkloadsPerHost = 4
	cfg.AppParams.NumApps = 1
	cfg.NumDNSAlgs = 10
	cfg.NumRoutingConfigs = 2
	cfg.NumUnderlayRoutingConfigs = 2
	cfg.NumNeighbors = 2
	cfg.NumUnderlayNeighbors = 2
	cfg.NumOfTenants = 2
	cfg.NumOfVRFsPerTenant = 2
	cfg.NumOfSubnetsPerVpc = 2
	cfg.NumOfIPAMPsPerTenant = 1
	cfg.WorkloadParams.InterfacesPerWorkload = 2
	cfg.MirrorSessionParams.NumSessionMirrors = 10

	cfg.NumOfTenants = 50
	cfg.NumOfVRFsPerTenant = 1
	cfg.NumOfSubnetsPerVpc = 6
	cfg.NumOfIPAMPsPerTenant = 1
	cfg.NumUnderlayRoutingConfigs = 1 // Same as other AS number
	cfg.NumUnderlayNeighbors = 1      //TOR AS nubr

	cfg.NumOfTenants = 50
	cfg.NumOfVRFsPerTenant = 1
	cfg.NumOfSubnetsPerVpc = 50
	cfg.NumOfIPAMPsPerTenant = 1
	cfg.NumUnderlayRoutingConfigs = 1                   // Same as other AS number
	cfg.NumUnderlayNeighbors = 1                        //TOR AS nubr
	cfg.NetworkSecurityPolicyParams.NumPolicies = 10000 // 200 Policy Per tenant. Each Subnet, 2 ingress, 2 egress
	cfg.NetworkSecurityPolicyParams.NumRulesPerPolicy = 128
	cfg.NetworkSecurityPolicyParams.NumIPPairsPerRule = 1
	cfg.NetworkSecurityPolicyParams.NumAppsPerRules = 1

	// create smartnic macs from a template
	smartnics := []*cluster.DistributedServiceCard{}
	snicsFile := os.Getenv("SNICS_FILE")
	if snicsFile != "" {
		smnicList := &cluster.DistributedServiceCardList{}
		jdata, err := ioutil.ReadFile(snicsFile)
		if err != nil {
			panic(fmt.Sprintf("file %s, error %s", snicsFile, err))
		}
		err = json.Unmarshal(jdata, smnicList)
		if err != nil {
			panic(fmt.Sprintf("error unmarshing json data %s", string(jdata)))
		}
		for _, smnic := range smnicList.Items {
			smartnics = append(smartnics, smnic)
		}
	} else {
		smnic := smartNicTemplate
		smnicCtx := newIterContext()
		for ii := 0; ii < 128; ii++ {
			tSmartNIC := smnicCtx.transform(smnic).(*cluster.DistributedServiceCard)
			smartnics = append(smartnics, tSmartNIC)
		}
	}
	cfg.Smartnics = [][]*cluster.DistributedServiceCard{smartnics}

	// generate the configuration now
	cfg.Do()

	// verify and keep the data in some file
	ofile, err := os.OpenFile("/tmp/cfg.json", os.O_RDWR|os.O_CREATE, 0755)
	if err != nil {
		panic(err)
	}

	for _, network := range cfg.ConfigItems.Networks {
		network.Spec.IngressSecurityPolicy = []string{}
		network.Spec.EgressSecurityPolicy = []string{}
	}

	for tenIDx, ten := range cfg.ConfigItems.Tenants {
		if ten.Name == globals.DefaultTenant {
			continue
		}

		nwIDx := 0
		for _, network := range cfg.ConfigItems.Subnets {
			if network.Tenant == ten.Name {
				for i := 0; i < 4; i++ {
					pol := cfg.ConfigItems.SGPolicies[tenIDx*len(cfg.ConfigItems.Tenants)*(4)+nwIDx*4+i]
					pol.Tenant = ten.Name
					fmt.Printf("Ten name %v %v %v\n", pol.Name, pol.Tenant, tenIDx*len(cfg.ConfigItems.Tenants)*(4)+nwIDx*4+i)
					if i%2 == 0 {
						network.Spec.IngressSecurityPolicy = append(network.Spec.IngressSecurityPolicy, pol.Name)
					} else {
						network.Spec.EgressSecurityPolicy = append(network.Spec.EgressSecurityPolicy, pol.Name)
					}
				}
				nwIDx++
			}
		}
	}
	for _, o := range cfg.ConfigItems.Networks {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}
	for _, o := range cfg.ConfigItems.Hosts {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}
	for _, o := range cfg.ConfigItems.Workloads {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}
	for _, o := range cfg.ConfigItems.TenantWorkloads {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}

	for _, o := range cfg.ConfigItems.Apps {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}

	for _, o := range cfg.ConfigItems.SGPolicies {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}

	for _, o := range cfg.ConfigItems.Mirrors {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}

	if j, err := json.MarshalIndent(cfg.Fwprofile, "", "  "); err == nil {
		ofile.Write(j)
		ofile.WriteString("\n")
	}

	for _, o := range cfg.ConfigItems.RouteConfig {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}

	for _, o := range cfg.ConfigItems.UnderlayRouteConfig {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}

	for _, o := range cfg.ConfigItems.Tenants {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}

	for _, o := range cfg.ConfigItems.VRFs {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}

	for _, o := range cfg.ConfigItems.IPAMPs {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}

	for _, o := range cfg.ConfigItems.Subnets {
		if j, err := json.MarshalIndent(o, "", "  "); err == nil {
			ofile.Write(j)
			ofile.WriteString("\n")
		}
	}

}
