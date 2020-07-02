package equinix_test

import (
	"encoding/json"
	"fmt"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	iota "github.com/pensando/sw/iota/protos/gogen"
	"github.com/pensando/sw/iota/test/venice/iotakit/model/objects"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/log"
	uuid "github.com/satori/go.uuid"

	yaml "gopkg.in/yaml.v2"

	"time"
)

type Subnet struct {
	TypeMeta string `yaml:"typemeta"`
	ObjMeta  string `yaml:"objmeta"`
	Spec     struct {
		Id         []byte `yaml:"id"`
		VPCId      []byte `yaml:"vpcid"`
		IPV4prefix struct {
			Addr uint32 `yaml:"addr"`
			Len  uint32 `yaml:"len"`
		} `yaml:"v4prefix"`
		IPv4Gateway uint32 `yaml:"ipv4virtualrouterip"`
		FabricEncap struct {
			Type  int32 `yaml:"type"`
			Value struct {
				Val struct {
					Vnid uint32 `yaml:"vnid"`
				} `yaml:"val"`
			} `yaml:"value"`
		} `yaml:"fabricencap"`
		DHCPPolicyId [][]byte `yaml:"dhcppolicyid"`
	} `yaml:"spec"`
	Status struct {
		HwId uint32 `yaml:"hwid"`
	} `yaml:"status"`
	Stats struct {
	} `yaml:"stats"`
}

var _ = Describe("IPAM Tests", func() {

	var customIpam string = "dhcp_relay_1"

	BeforeEach(func() {
		// verify cluster is in good health
		Eventually(func() error {
			return ts.model.VerifyClusterStatus()
		}).Should(Succeed())

	})
	AfterEach(func() {

	})

	Context("IPAM Tests", func() {

		It("Default IPAM Policy on VPC", func() {

			tenants, err := getTenants()
			if err != nil {
				return
			}

			for _, t := range tenants {
				log.Infof("Default IPAM Policy on VPC on tenant %v", t)
				vpcc, err := objects.TenantVPCCollection(t, ts.model.ConfigClient(), ts.model.Testbed())
				Expect(err).Should(Succeed())
				vpcName := vpcc.Objs[0].Obj.Name
				defaultIpam := vpcc.Objs[0].Obj.Spec.DefaultIPAMPolicy
				ip, err := objects.GetIPAMPolicy(ts.model.ConfigClient(), defaultIpam, t)
				Expect(err).Should(Succeed())
				serverip := ip.PolicyObj.Spec.DHCPRelay.Servers[0].IPAddress

				// create a Custom IPAM policy
				createIPAMPolicy(customIpam, "", t, serverip)

				// apply customIpam to VPC- this verifies update IPAM on VPC case also
				Expect(vpcc.SetIPAM(customIpam)).Should(Succeed())

				// get network from vpc
				nwc, err := GetNetworkCollectionFromVPC(vpcName, t)
				Expect(err).Should(Succeed())
				selNetwork := nwc.Any(1)

				// verify ipam on subnet
				verifyIPAMonSubnet(selNetwork.Subnets()[0].Name, customIpam, t)

				if ts.tb.HasNaplesHW() {
					// get workload pair and validate datapath
					wpc := ts.model.WorkloadPairs().OnNetwork(selNetwork.Subnets()[0]).Any(1)
					verifyIPAMDataPath(wpc)
				}

				// get back to default policy on VPC
				Expect(vpcc.SetIPAM(defaultIpam)).Should(Succeed())

				// delete Custom IPAM policy
				deleteIPAMPolicy(customIpam, t)
			}
		})

		It("Override/Remove IPAM policy on Subnet", func() {

			tenants, err := getTenants()
			if err != nil {
				return
			}

			for _, t := range tenants {
				log.Infof("Override/Remove IPAM policy on Subnet on tenant %v", t)
				vpcc, err := objects.TenantVPCCollection(t, ts.model.ConfigClient(), ts.model.Testbed())
				Expect(err).Should(Succeed())
				vpcName := vpcc.Objs[0].Obj.Name
				defaultIpam := vpcc.Objs[0].Obj.Spec.DefaultIPAMPolicy
				ip, err := objects.GetIPAMPolicy(ts.model.ConfigClient(), defaultIpam, t)
				Expect(err).Should(Succeed())
				serverip := ip.PolicyObj.Spec.DHCPRelay.Servers[0].IPAddress

				// create a Custom IPAM policy
				createIPAMPolicy(customIpam, "", t, serverip)

				// get network from vpc
				nwc, err := GetNetworkCollectionFromVPC(vpcName, t)
				Expect(err).Should(Succeed())

				// override customIpam on the subnet
				selNetwork := nwc.Any(1)
				Expect(selNetwork.SetIPAMOnNetwork(selNetwork.Subnets()[0], customIpam)).Should(Succeed())

				// verify ipam on subnet
				verifyIPAMonSubnet(selNetwork.Subnets()[0].Name, customIpam, t)

				if ts.tb.HasNaplesHW() {
					// get workload pair and validate datapath
					wpc := ts.model.WorkloadPairs().OnNetwork(selNetwork.Subnets()[0]).Any(1)
					verifyIPAMDataPath(wpc)
				}

				// remove ipam on subnet - it should point to default IPAM policy on VPC
				Expect(selNetwork.SetIPAMOnNetwork(selNetwork.Subnets()[0], "")).Should(Succeed())

				// verify ipam on subnet
				verifyIPAMonSubnet(selNetwork.Subnets()[0].Name, defaultIpam, t)

				if ts.tb.HasNaplesHW() {
					// get workload pair and validate datapath
					wpc := ts.model.WorkloadPairs().OnNetwork(selNetwork.Subnets()[0]).Any(1)
					verifyIPAMDataPath(wpc)
				}

				// delete Custom IPAM policy
				deleteIPAMPolicy(customIpam, t)
			}
		})

		It("Multiple IPAM per naples", func() {

			tenants, err := getTenants()
			if err != nil {
				return
			}

			for _, t := range tenants {
				log.Infof("Multiple IPAM per naples on tenant %v", t)
				vpcc, err := objects.TenantVPCCollection(t, ts.model.ConfigClient(), ts.model.Testbed())
				Expect(err).Should(Succeed())
				vpcName := vpcc.Objs[0].Obj.Name
				defaultIpam := vpcc.Objs[0].Obj.Spec.DefaultIPAMPolicy
				ip, err := objects.GetIPAMPolicy(ts.model.ConfigClient(), defaultIpam, t)
				Expect(err).Should(Succeed())
				serverip := ip.PolicyObj.Spec.DHCPRelay.Servers[0].IPAddress

				// create a Custom IPAM policy
				createIPAMPolicy(customIpam, "", t, serverip)

				// get networks from vpc
				nwc, err := GetNetworkCollectionFromVPC(vpcName, t)
				Expect(err).Should(Succeed())

				if len(nwc.Subnets()) < 2 {
					Skip("Not enough subnets to verify multiple IPAMs per naples")
				}

				selNetwork := nwc.Any(2)
				// override customIpam on one of the subnets
				Expect(selNetwork.SetIPAMOnNetwork(selNetwork.Subnets()[0], customIpam)).Should(Succeed())

				// verify ipam on subnets
				verifyIPAMonSubnet(selNetwork.Subnets()[0].Name, customIpam, t)
				verifyIPAMonSubnet(selNetwork.Subnets()[1].Name, defaultIpam, t)

				if ts.tb.HasNaplesHW() {
					// get workload pair and validate datapath - customIpam
					wpc := ts.model.WorkloadPairs().OnNetwork(selNetwork.Subnets()[0]).Any(1)
					verifyIPAMDataPath(wpc)

					// get workload pair and validate datapath - defaultIpam
					wpc = ts.model.WorkloadPairs().OnNetwork(selNetwork.Subnets()[1]).Any(1)
					verifyIPAMDataPath(wpc)
				}

				// remove ipam on subnet - it should point to default IPAM policy on VPC
				Expect(selNetwork.SetIPAMOnNetwork(selNetwork.Subnets()[0], "")).Should(Succeed())

				// delete Custom IPAM policy
				deleteIPAMPolicy(customIpam, t)
			}

		})

		/*
		Venice Error: Only one DHCP server configuration is supported
		It("Multiple IPAM servers in a policy", func() {

			tenants, err := getTenants()
			if err != nil {
				return
			}

			for _, t := range tenants {
				log.Infof("Default IPAM Policy on VPC on tenant %v", t)
				vpcc, err := objects.TenantVPCCollection(t, ts.model.ConfigClient(), ts.model.Testbed())
				Expect(err).Should(Succeed())
				vpcName := vpcc.Objs[0].Obj.Name
				defaultIpam := vpcc.Objs[0].Obj.Spec.DefaultIPAMPolicy
				ip, err := objects.GetIPAMPolicy(ts.model.ConfigClient(), defaultIpam, t)
				Expect(err).Should(Succeed())
				serverip := ip.PolicyObj.Spec.DHCPRelay.Servers[0].IPAddress

				// create a Custom IPAM policy
				createIPAMPolicy(customIpam, "", t, serverip)
				addServerToIPAMPolicy(customIpam, "", t, "60.1.1.1")

				// apply customIpam to VPC
				Expect(vpcc.SetIPAM(customIpam)).Should(Succeed())

				// get network from vpc
				nwc, err := GetNetworkCollectionFromVPC(vpcName, t)
				Expect(err).Should(Succeed())
				selNetwork := nwc.Any(1)

				// verify ipam on subnet
				verifyIPAMonSubnet(selNetwork.Subnets()[0].Name, customIpam, t)

				if ts.tb.HasNaplesHW() {
					// get workload pair and validate datapath
					wpc := ts.model.WorkloadPairs().OnNetwork(selNetwork.Subnets()[0]).Any(1)
					verifyIPAMDataPath(wpc)
				}

				// get back to default policy on VPC
				Expect(vpcc.SetIPAM(defaultIpam)).Should(Succeed())

				// delete Custom IPAM policy
				deleteIPAMPolicy(customIpam, t)
			}
		})
		*/

		It("Change Servers in IPAM Policy", func() {

			tenants, err := getTenants()
			if err != nil {
				return
			}

			for _, t := range tenants {
				log.Infof("Change Servers in IPAM Policy on tenant %v", t)
				vpcc, err := objects.TenantVPCCollection(t, ts.model.ConfigClient(), ts.model.Testbed())
				Expect(err).Should(Succeed())
				vpcName := vpcc.Objs[0].Obj.Name
				defaultIpam := vpcc.Objs[0].Obj.Spec.DefaultIPAMPolicy
				ip, err := objects.GetIPAMPolicy(ts.model.ConfigClient(), defaultIpam, t)
				Expect(err).Should(Succeed())
				serverip := ip.PolicyObj.Spec.DHCPRelay.Servers[0].IPAddress

				// create a test-policy with random dhcp server
				createIPAMPolicy("test-policy", "", t, "51.1.1.1")

				// apply "test-policy" Ipam to tenant VPC
				Expect(vpcc.SetIPAM("test-policy")).Should(Succeed())

				// get network from vpc
				nwc, err := GetNetworkCollectionFromVPC(vpcName, t)
				Expect(err).Should(Succeed())
				selNetwork := nwc.Any(1)

				// verify ipam on subnet
				verifyIPAMonSubnet(selNetwork.Subnets()[0].Name, "test-policy", t)
				var wpc *objects.WorkloadPairCollection
				if ts.tb.HasNaplesHW() {
					// get workload pair and validate datapath
					wpc = ts.model.WorkloadPairs().OnNetwork(selNetwork.Subnets()[0]).Any(1)
					verifyIPAMDataPathFail(wpc)
				}

				// update test-policy
				createIPAMPolicy("test-policy", "", t, serverip)
				if ts.tb.HasNaplesHW() {
					// get workload pair and validate datapath
					verifyIPAMDataPath(wpc)
				}

				// get back to default policy on VPC
				Expect(vpcc.SetIPAM(defaultIpam)).Should(Succeed())

				// delete test-policy
				deleteIPAMPolicy("test-policy", t)
			}
		})

		It("Remove IPAM policy on VPC", func() {

			tenants, err := getTenants()
			if err != nil {
				return
			}

			for _, t := range tenants {
				log.Infof("Remove IPAM policy on VPC on tenant %v", t)
				vpcc, err := objects.TenantVPCCollection(t, ts.model.ConfigClient(), ts.model.Testbed())
				Expect(err).Should(Succeed())
				vpcName := vpcc.Objs[0].Obj.Name
				defaultIpam := vpcc.Objs[0].Obj.Spec.DefaultIPAMPolicy

				// remove IPAM policy on tenant vpc
				Expect(vpcc.SetIPAM("")).Should(Succeed())

				// get network from vpc
				nwc, err := GetNetworkCollectionFromVPC(vpcName, t)
				Expect(err).Should(Succeed())
				selNetwork := nwc.Any(1)

				// verify ipam on subnet
				verifyNoIPAMonSubnet(selNetwork.Subnets()[0].Name, t)

				// ping should fail as there no ipam policy configured to get DHCP ip for host
				var wpc *objects.WorkloadPairCollection
				if ts.tb.HasNaplesHW() {
					wpc = ts.model.WorkloadPairs().OnNetwork(selNetwork.Subnets()[0]).Any(1)
					verifyIPAMDataPathFail(wpc)
				}

				// get back to default policy on VPC
				Expect(vpcc.SetIPAM(defaultIpam)).Should(Succeed())
				if ts.tb.HasNaplesHW() {
					verifyIPAMDataPath(wpc)
				}

				// get back to default policy on VPC
				Expect(vpcc.SetIPAM(defaultIpam)).Should(Succeed())
			}
		})

	})
})

func createIPAMPolicy(ipam, vpc, tenant string, serverip string) {
	ipc := ts.model.NewIPAMPolicy(ipam, tenant, vpc, serverip)
	Expect(ipc.Commit()).Should(Succeed())
	// validate policy
	validateIPAMPolicy(ipam, vpc, tenant, serverip)
}

func addServerToIPAMPolicy(ipam, vpc, tenant, serverip string) {

	obj, err := objects.GetIPAMPolicy(ts.model.ConfigClient(), ipam, tenant)
	Expect(err).ShouldNot(HaveOccurred())

	ipc := objects.NewIPAMPolicyCollection(ts.model.ConfigClient(), ts.model.Testbed())
	ipc.PolicyObjs = append(ipc.PolicyObjs, obj)
	ipc.AddServer(ipam, serverip, vpc)
	Expect(ipc.Commit()).Should(Succeed())

	// validate policy
	validateIPAMPolicy(ipam, vpc, tenant, serverip)
}

func deleteIPAMPolicy(ipam, tenant string) {
	ipc := objects.NewIPAMPolicyCollection(ts.model.ConfigClient(), ts.model.Testbed())
	ip, err := objects.GetIPAMPolicy(ts.model.ConfigClient(), ipam, tenant)
	Expect(err).ShouldNot(HaveOccurred())
	ipc.PolicyObjs = append(ipc.PolicyObjs, ip)
	Expect(ipc.Delete()).Should(Succeed())
}

func GetNetworkCollectionFromVPC(vpc, tenant string) (*objects.NetworkCollection, error) {
	return objects.VpcNetworkCollection(tenant, vpc, ts.model.ConfigClient())
}

func VerifyIPAMPropagation(ipam, tenant string) error {

	npc, err := ts.model.Naples().SelectByTenant(tenant)
	Expect(err).ShouldNot(HaveOccurred())

	veniceIpam, err := objects.GetIPAMPolicy(ts.model.ConfigClient(), ipam, tenant)
	if err != nil {
		log.Errorf("Error getting IPAMPolicies Err: %v", err)
		return err
	}
	p := veniceIpam.PolicyObj
	if p.Status.PropagationStatus.GenerationID != p.ObjectMeta.GenerationID {
		log.Warnf("Propagation generation id did not match: Meta: %+v, PropagationStatus: %+v", p.ObjectMeta, p.Status.PropagationStatus)
		return fmt.Errorf("Propagation generation id did not match")
	}

	totalNaples := int32(len(npc.FakeNodes)) + int32(len(npc.Nodes))
	if (p.Status.PropagationStatus.Updated != totalNaples) || (p.Status.PropagationStatus.Pending != 0) {
		log.Warnf("Propagation status incorrect: PropagationStatus: %+v, TotalNaples: %v", p.Status.PropagationStatus, totalNaples)
		return fmt.Errorf("Propagation status was incorrect")
	}

	log.Infof("Propagation status matched: PropagationStatus: %+v, TotalNaples: %v", p.Status.PropagationStatus, totalNaples)
	return nil
}

func verifyIPAMonSubnet(subnet, ipam, tenant string) error {

	var data Subnet
	var iData []netproto.IPAMPolicy
	var ipam_uuid []string

	Eventually(func() error {
		return VerifyIPAMPropagation(ipam, tenant)
	}).Should(Succeed())

	if ts.scaleData {
		log.Infof("Skip per naples checks for scale setup")
		return nil
	}

	// get network
	veniceNw, err := ts.model.GetNetwork(tenant, subnet)
	Expect(err).ShouldNot(HaveOccurred())
	nw_uuid := veniceNw.VeniceNetwork.GetUUID()

	// get ipam
	veniceIpam, err := objects.GetIPAMPolicy(ts.model.ConfigClient(), ipam, tenant)
	Expect(err).ShouldNot(HaveOccurred())

	var matchUUID = func(s string) bool {
		err = yaml.Unmarshal([]byte(s), &data)
		Expect(err).ShouldNot(HaveOccurred())
		for i, d := range data.Spec.DHCPPolicyId {
			uid, _ := uuid.FromBytes(d)
			if ipam_uuid[i] != uid.String() {
				log.Infof("pdsctl show subnet op: [%v] && yaml: [%+v]", s, data)
				log.Errorf("IPAM UUID (netagent):%v doesn't match with Subnet UUID(pdsctl) :%v", ipam_uuid, uid.String())
				return false
			}
		}
		return true
	}

	var matchServerIps = func(s string) bool {
		err = json.Unmarshal([]byte(s), &iData)
		for i, srv := range iData[0].Spec.DHCPRelay.Servers {
			if srv.IPAddress != veniceIpam.PolicyObj.Spec.DHCPRelay.Servers[i].IPAddress {
				log.Infof("IPAM curl op: [%+v] && json: [%+v]", s, iData[0])
				log.Errorf("Venice IP :%v doesn't match with Netagent IP :%v", srv.IPAddress, veniceIpam.PolicyObj.Spec.DHCPRelay.Servers[i].IPAddress)
				return false
			}
		}
		ipam_uuid = iData[0].Status.IPAMPolicyIDs
		return true
	}

	// wait for Naples to finish configuring
	time.Sleep(5 * time.Second)

	// fetch network from pdsctl. network should have updated IPAM Policy
	ts.model.ForEachFakeNaples(func(nc *objects.NaplesCollection) error {
		cmd := "curl localhost:9007/api/ipam-policies/"
		cmdOut, err := ts.model.RunFakeNaplesBackgroundCommand(nc, cmd)
		Expect(err).ShouldNot(HaveOccurred())
		cmdResp, _ := cmdOut.([]*iota.Command)
		for _, cmdLine := range cmdResp {
			Expect(matchServerIps(cmdLine.Stdout)).Should(BeTrue())
		}

		cmd = "/naples/nic/bin/pdsctl show subnet --id " + nw_uuid + " --yaml"
		cmdOut, err = ts.model.RunFakeNaplesBackgroundCommand(nc, cmd)
		Expect(err).ShouldNot(HaveOccurred())
		cmdResp, _ = cmdOut.([]*iota.Command)
		for _, cmdLine := range cmdResp {
			Expect(matchUUID(cmdLine.Stdout)).Should(BeTrue())
		}
		return nil
	})
	ts.model.ForEachNaples(func(nc *objects.NaplesCollection) error {
		cmd := "curl localhost:9007/api/ipam-policies/"
		cmdOut, err := ts.model.RunNaplesCommand(nc, cmd)
		Expect(err).ShouldNot(HaveOccurred())
		for _, cmdLine := range cmdOut {
			Expect(matchServerIps(cmdLine)).Should(BeTrue())
		}

		cmd = "/nic/bin/pdsctl show subnet --id " + nw_uuid + " --yaml"
		cmdOut, err = ts.model.RunNaplesCommand(nc, cmd)
		Expect(err).ShouldNot(HaveOccurred())
		for _, cmdLine := range cmdOut {
			Expect(matchUUID(cmdLine)).Should(BeTrue())
		}
		return nil
	})
	return nil
}

func verifyNoIPAMonSubnet(subnet, tenant string) error {

	if ts.scaleData {
		log.Infof("Skip per naples checks for scale setup")
		return nil
	}

	var data Subnet
	// get network
	veniceNw, err := ts.model.GetNetwork(tenant, subnet)
	Expect(err).ShouldNot(HaveOccurred())
	nw_uuid := veniceNw.VeniceNetwork.GetUUID()

	// wait for Naples to finish configuring
	time.Sleep(5 * time.Second)

	var allZeroByteArray = func(b []byte) bool {
		for _, v := range b {
			if v != 0 {
				return false
			}
		}
		return true
	}

	// fetch network from pdsctl. network should have updated IPAM Policy
	ts.model.ForEachFakeNaples(func(nc *objects.NaplesCollection) error {
		cmd := "/naples/nic/bin/pdsctl show subnet --id " + nw_uuid + " --yaml"
		cmdOut, err := ts.model.RunFakeNaplesBackgroundCommand(nc, cmd)
		Expect(err).ShouldNot(HaveOccurred())
		cmdResp, _ := cmdOut.([]*iota.Command)
		for _, cmdLine := range cmdResp {
			err = yaml.Unmarshal([]byte(cmdLine.Stdout), &data)
			Expect(err).ShouldNot(HaveOccurred())
			Expect(allZeroByteArray(data.Spec.DHCPPolicyId[0])).Should(BeTrue())
		}
		return nil
	})
	ts.model.ForEachNaples(func(nc *objects.NaplesCollection) error {
		cmd := "/nic/bin/pdsctl show subnet --id " + nw_uuid + " --yaml"
		cmdOut, err := ts.model.RunNaplesCommand(nc, cmd)
		Expect(err).ShouldNot(HaveOccurred())
		for _, cmdLine := range cmdOut {
			err = yaml.Unmarshal([]byte(cmdLine), &data)
			Expect(err).ShouldNot(HaveOccurred())
			Expect(allZeroByteArray(data.Spec.DHCPPolicyId[0])).Should(BeTrue())
		}
		return nil
	})
	return nil
}

func validateIPAMPolicy(name, vrf, tenant string, ip string) {
	obj, err := objects.GetIPAMPolicy(ts.model.ConfigClient(), name, tenant)
	// for now, validate by getting from venice by name
	// can add netagent validation if reqd
	Expect(err).ShouldNot(HaveOccurred())

	match := false
	for _, s := range obj.PolicyObj.Spec.DHCPRelay.Servers {
		if s.IPAddress == ip {
			match = true
			break
		}
	}
	Expect(match == true).Should(BeTrue())
}

func verifyIPAMDataPath(wpc *objects.WorkloadPairCollection) {
	Expect(len(wpc.Pairs) != 0).Should(BeTrue())

	// Get dynamic IP for workloadPair
	err := wpc.WorkloadPairGetDynamicIps(ts.model.Testbed())
	Expect(err).Should(Succeed())

	Eventually(func() error {
		return ts.model.PingPairs(wpc)
	}).Should(Succeed())
}

func verifyIPAMDataPathFail(wpc *objects.WorkloadPairCollection) {
	Expect(len(wpc.Pairs) != 0).Should(BeTrue())

	// Get dynamic IP for workloadPair
	err := wpc.WorkloadPairGetDynamicIps(ts.model.Testbed())
	Expect(err).ShouldNot(Succeed())
}

func getTenants() ([]string, error) {
	var tenants []string
	ten, err := ts.model.ConfigClient().ListTenant()
	if err != nil {
		return tenants, err
	}

	if len(ten) == 0 {
		return tenants, fmt.Errorf("not enough tenants to list")
	}

	for _, t := range ten {
		if t.Name != "default" {
			tenants = append(tenants, t.Name)
		}
	}

	return tenants, nil

}
